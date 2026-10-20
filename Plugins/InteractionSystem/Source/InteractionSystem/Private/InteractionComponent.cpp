#include "InteractionComponent.h"

#include "Components/MeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/AssetManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Interactable.h"
#include "InteractionSystemLog.h"
#include "UI/InteractionPromptWidget.h"

namespace InteractionDebug
{
	static TAutoConsoleVariable<int32> CVarDebug(
		TEXT("Interaction.Debug"),
		0,
		TEXT("Interaction system debug drawing.\n")
		TEXT(" 0: off\n")
		TEXT(" 1: draw focus traces and focused actor info\n")
		TEXT(" 2: also draw server-side validation checks"),
		ECVF_Cheat);

	static int32 GetLevel()
	{
#if ENABLE_DRAW_DEBUG
		return CVarDebug.GetValueOnGameThread();
#else
		return 0;
#endif
	}
}

UInteractionComponent::UInteractionComponent()
{
	// Ticking is switched on in RefreshLocalControlState only for the local player's pawn.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// Required for the Server RPCs. The component has no replicated properties, so the
	// bandwidth cost is nil; the pawn's owning connection is what lets the client call them.
	SetIsReplicatedByDefault(true);
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	SetComponentTickInterval(FocusUpdateInterval);

	if (APawn* Pawn = GetPawn())
	{
		// Possession can happen before or after BeginPlay (and on clients arrives via replication),
		// so listen for it rather than assuming the controller is ready now.
		Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UInteractionComponent::HandleControllerChanged);
	}
	else
	{
		UE_LOG(LogInteraction, Warning, TEXT("%s must be owned by a Pawn (owner: %s)."), *GetName(), *GetNameSafe(GetOwner()));
	}

	RefreshLocalControlState();
}

void UInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APawn* Pawn = GetPawn())
	{
		Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UInteractionComponent::HandleControllerChanged);
	}

	CancelHold();
	ClearFocus();
	DestroyPromptWidget();

	Super::EndPlay(EndPlayReason);
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFocus();
}

void UInteractionComponent::ForceFocusUpdate()
{
	if (bIsLocalPlayer)
	{
		UpdateFocus();
	}
}

void UInteractionComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	RefreshLocalControlState();
}

void UInteractionComponent::RefreshLocalControlState()
{
	const bool bWasLocalPlayer = bIsLocalPlayer;
	bIsLocalPlayer = IsLocalPlayerPawn();

	if (bIsLocalPlayer == bWasLocalPlayer)
	{
		return;
	}

	SetComponentTickEnabled(bIsLocalPlayer);

	if (bIsLocalPlayer)
	{
		CreatePromptWidget();
	}
	else
	{
		CancelHold();
		ClearFocus();
		DestroyPromptWidget();
	}
}

void UInteractionComponent::CreatePromptWidget()
{
	if (PromptWidget || PromptWidgetLoadHandle.IsValid() || PromptWidgetClass.IsNull())
	{
		return;
	}

	if (PromptWidgetClass.Get())
	{
		OnPromptWidgetClassLoaded();
		return;
	}

	// The delegate is bound weakly to this UObject, so it is safe if we're destroyed mid-load.
	PromptWidgetLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		PromptWidgetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &UInteractionComponent::OnPromptWidgetClassLoaded));
}

void UInteractionComponent::OnPromptWidgetClassLoaded()
{
	PromptWidgetLoadHandle.Reset();

	const APawn* Pawn = GetPawn();
	APlayerController* PC = Pawn ? Pawn->GetController<APlayerController>() : nullptr;
	UClass* WidgetClass = PromptWidgetClass.Get();
	if (!bIsLocalPlayer || PromptWidget || !PC || !WidgetClass)
	{
		return;
	}

	PromptWidget = CreateWidget<UInteractionPromptWidget>(PC, WidgetClass);
	if (PromptWidget)
	{
		PromptWidget->SetInteractionComponent(this);
		// Player screen (not the whole viewport) so split-screen players each get their own prompt.
		PromptWidget->AddToPlayerScreen(PromptZOrder);
	}
}

void UInteractionComponent::DestroyPromptWidget()
{
	if (PromptWidgetLoadHandle.IsValid())
	{
		PromptWidgetLoadHandle->CancelHandle();
		PromptWidgetLoadHandle.Reset();
	}
	if (PromptWidget)
	{
		PromptWidget->RemoveFromParent();
		PromptWidget = nullptr;
	}
}

void UInteractionComponent::SetupInputBindings(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (!EnhancedInputComponent)
	{
		return;
	}
	if (!InteractAction)
	{
		UE_LOG(LogInteraction, Warning, TEXT("%s on %s has no InteractAction set; interaction input will not work."),
			*GetName(), *GetNameSafe(GetOwner()));
		return;
	}

	EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &UInteractionComponent::StartInteract);
	EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &UInteractionComponent::StopInteract);
	EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Canceled, this, &UInteractionComponent::StopInteract);
}

void UInteractionComponent::StartInteract()
{
	if (!bIsLocalPlayer || bIsHolding)
	{
		return;
	}

	// Refresh now rather than trusting a result up to FocusUpdateInterval old.
	UpdateFocus();

	AActor* Target = FocusedActor.Get();
	if (!Target || !bFocusedCanInteract)
	{
		return;
	}

	if (FocusedData.IsHold())
	{
		BeginHold(Target, FocusedData.HoldDuration);
	}
	else
	{
		ServerInteract(Target);
	}
}

void UInteractionComponent::StopInteract()
{
	CancelHold();
}

void UInteractionComponent::BeginHold(AActor* Target, float Duration)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bIsHolding = true;
	HoldTarget = Target;
	HoldDuration = Duration;
	HoldStartTime = World->GetTimeSeconds();

	// A one-shot timer completes the hold exactly on time without ticking every frame.
	World->GetTimerManager().SetTimer(HoldTimerHandle, this, &UInteractionComponent::CompleteHold, Duration, false);

	ServerBeginHold(Target);
	OnHoldStarted.Broadcast(Target);
}

void UInteractionComponent::CompleteHold()
{
	AActor* Target = HoldTarget.Get();
	const bool bStillValid = Target && Target == FocusedActor.Get() && bFocusedCanInteract;

	bIsHolding = false;
	HoldTarget.Reset();

	if (bStillValid)
	{
		ServerInteract(Target);
	}
	else
	{
		OnHoldCanceled.Broadcast(Target);
	}
}

void UInteractionComponent::CancelHold()
{
	if (!bIsHolding)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HoldTimerHandle);
	}

	AActor* Target = HoldTarget.Get();
	bIsHolding = false;
	HoldTarget.Reset();

	// No server RPC needed: the server only uses the hold start time as a lower bound, and the
	// next ServerBeginHold overwrites it.
	OnHoldCanceled.Broadcast(Target);
}

float UInteractionComponent::GetHoldProgress() const
{
	const UWorld* World = GetWorld();
	if (!bIsHolding || !World || HoldDuration <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(static_cast<float>((World->GetTimeSeconds() - HoldStartTime) / HoldDuration), 0.f, 1.f);
}

FText UInteractionComponent::GetInteractKeyText() const
{
	const APawn* Pawn = GetPawn();
	const APlayerController* PC = Pawn ? Pawn->GetController<APlayerController>() : nullptr;
	const UEnhancedInputLocalPlayerSubsystem* Subsystem = PC
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer())
		: nullptr;

	if (!Subsystem || !InteractAction)
	{
		return FText::GetEmpty();
	}

	// Asks the active mapping contexts, so rebinding the key updates the prompt automatically.
	// Prefers keyboard/mouse; a production version would follow the last-used input device.
	const TArray<FKey> Keys = Subsystem->QueryKeysMappedToAction(InteractAction);
	const FKey* Chosen = Keys.FindByPredicate([](const FKey& Key) { return !Key.IsGamepadKey(); });
	if (!Chosen && Keys.Num() > 0)
	{
		Chosen = &Keys[0];
	}
	return Chosen ? Chosen->GetDisplayName(/*bLongDisplayName*/ false) : FText::GetEmpty();
}

void UInteractionComponent::ServerBeginHold_Implementation(AActor* Target)
{
	ServerHoldTarget = Target;
	ServerHoldStartTime = GetWorld()->GetTimeSeconds();
}

void UInteractionComponent::ServerInteract_Implementation(AActor* Target)
{
	FString FailureReason;
	if (!ValidateInteraction(Target, FailureReason))
	{
		// Not a kick: failures here are normally just latency (the door moved, the pickup was taken).
		UE_LOG(LogInteraction, Log, TEXT("Rejected interaction by %s with %s: %s"),
			*GetNameSafe(GetOwner()), *GetNameSafe(Target), *FailureReason);
		return;
	}

	ServerHoldTarget.Reset();

	APawn* Pawn = GetPawn();
	IInteractable::Execute_Interact(Target, Pawn);
	OnInteracted.Broadcast(Target, Pawn);
}

bool UInteractionComponent::ValidateInteraction(AActor* Target, FString& OutFailureReason) const
{
	APawn* Pawn = GetPawn();
	UWorld* World = GetWorld();
	if (!Pawn || !World)
	{
		OutFailureReason = TEXT("no pawn");
		return false;
	}
	if (!IsInteractable(Target))
	{
		OutFailureReason = TEXT("target is not interactable");
		return false;
	}
	if (!IInteractable::Execute_CanInteract(Target, Pawn))
	{
		OutFailureReason = TEXT("CanInteract returned false");
		return false;
	}

	// Reach: measured to the closest point of the target's collision bounds, not its origin,
	// because origins often sit at a hinge or the floor, far from the surface the player aimed at.
	const FVector PawnLocation = Pawn->GetActorLocation();
	FBox Bounds = Target->GetComponentsBoundingBox();
	if (!Bounds.IsValid)
	{
		Bounds = FBox(Target->GetActorLocation(), Target->GetActorLocation());
	}
	const float MaxReach = InteractionDistance + ServerDistanceTolerance;
	const float DistSq = static_cast<float>(Bounds.ComputeSquaredDistanceToPoint(PawnLocation));
	const bool bInReach = DistSq <= FMath::Square(MaxReach);

	// Line of sight: from the pawn's eyes, not the client's camera (which the server can't trust).
	// Accept if either the bounds centre or the closest bounds point is visible.
	const FVector Eyes = Pawn->GetPawnViewLocation();
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionValidate), false, Pawn);
	auto HasLineOfSight = [&](const FVector& Point)
	{
		FHitResult Hit;
		const bool bBlocked = World->LineTraceSingleByChannel(Hit, Eyes, Point, TraceChannel, Params);
		return !bBlocked || Hit.GetActor() == Target;
	};
	const FVector Center = Bounds.GetCenter();
	const FVector Closest = Bounds.GetClosestPointTo(Eyes);
	const bool bVisible = bInReach && (HasLineOfSight(Center) || HasLineOfSight(Closest));

#if ENABLE_DRAW_DEBUG
	if (InteractionDebug::GetLevel() > 1)
	{
		DrawDebugBox(World, Center, Bounds.GetExtent(), bInReach ? FColor::Cyan : FColor::Red, false, 2.f);
		DrawDebugLine(World, Eyes, Center, bVisible ? FColor::Cyan : FColor::Red, false, 2.f, 0, 1.5f);
		DrawDebugSphere(World, PawnLocation, MaxReach, 24, FColor::Blue, false, 2.f);
	}
#endif

	if (!bInReach)
	{
		OutFailureReason = FString::Printf(TEXT("out of reach (%.0f > %.0f cm)"), FMath::Sqrt(DistSq), MaxReach);
		return false;
	}
	if (!bVisible)
	{
		OutFailureReason = TEXT("no line of sight");
		return false;
	}

	// Hold: the server's own clock must agree the input was held long enough.
	const FInteractionData Data = IInteractable::Execute_GetInteractionData(Target, Pawn);
	if (Data.IsHold())
	{
		const double Held = World->GetTimeSeconds() - ServerHoldStartTime;
		if (ServerHoldTarget.Get() != Target || Held < Data.HoldDuration - HoldTimeTolerance)
		{
			OutFailureReason = FString::Printf(TEXT("hold too short (%.2fs of %.2fs)"), Held, Data.HoldDuration);
			return false;
		}
	}

	return true;
}

void UInteractionComponent::UpdateFocus()
{
	AActor* NewFocus = TraceForInteractable();
	SetFocusedActor(NewFocus);

	// Availability and prompt text can change while focused (a lever unlocks a door, a door
	// flips from "Open" to "Close"), so re-query each update and only notify on change.
	if (AActor* Focus = FocusedActor.Get())
	{
		APawn* Pawn = GetPawn();
		const FInteractionData NewData = IInteractable::Execute_GetInteractionData(Focus, Pawn);
		const bool bNewCanInteract = IInteractable::Execute_CanInteract(Focus, Pawn);

		if (bNewCanInteract != bFocusedCanInteract || !NewData.IsEquivalentTo(FocusedData))
		{
			FocusedData = NewData;
			bFocusedCanInteract = bNewCanInteract;
			OnFocusChanged.Broadcast(Focus);
		}
	}

	// Holding requires continuously looking at a target that still accepts interaction.
	if (bIsHolding && (HoldTarget.Get() != FocusedActor.Get() || !bFocusedCanInteract))
	{
		CancelHold();
	}

#if ENABLE_DRAW_DEBUG
	if (InteractionDebug::GetLevel() > 0)
	{
		if (AActor* Focus = FocusedActor.Get())
		{
			const FString Info = FString::Printf(TEXT("%s\n%s%s%s"),
				*Focus->GetName(),
				*FocusedData.GetPromptText().ToString(),
				FocusedData.IsHold() ? *FString::Printf(TEXT(" (hold %.1fs)"), FocusedData.HoldDuration) : TEXT(""),
				bFocusedCanInteract ? TEXT("") : TEXT("\n[unavailable]"));
			DrawDebugString(GetWorld(), Focus->GetActorLocation() + FVector(0.f, 0.f, 50.f), Info, nullptr,
				bFocusedCanInteract ? FColor::Green : FColor::Orange, FMath::Max(FocusUpdateInterval, 0.f) + 0.02f, true);
		}
	}
#endif
}

bool UInteractionComponent::GetAimRay(FVector& OutStart, FVector& OutDirection) const
{
	const APawn* Pawn = GetPawn();
	const AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	if (!Controller)
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	OutDirection = ViewRotation.Vector();

	// In third person the camera sits a spring-arm length behind the character. Starting the ray
	// where it passes closest to the pawn means (a) reach is measured from the character, not the
	// camera, and (b) objects between the camera and the character can't be focused.
	// In first person this distance is ~0, so the same code works for both.
	const float DistanceAlongRay = FMath::Max(0.f, FVector::DotProduct(Pawn->GetActorLocation() - ViewLocation, OutDirection));
	OutStart = ViewLocation + OutDirection * DistanceAlongRay;
	return true;
}

AActor* UInteractionComponent::TraceForInteractable() const
{
	UWorld* World = GetWorld();
	FVector Start, Direction;
	if (!World || !GetAimRay(Start, Direction))
	{
		return nullptr;
	}

	const FVector End = Start + Direction * InteractionDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionFocus), /*bTraceComplex*/ false, GetOwner());

	// Precise line trace first: what the crosshair is actually on wins.
	FHitResult Hit;
	const bool bLineHit = World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);
	AActor* Result = (bLineHit && IsInteractable(Hit.GetActor())) ? Hit.GetActor() : nullptr;

	// Forgiving sphere sweep second, only up to whatever the line hit, so it can't reach through walls.
	FHitResult SweepHit;
	bool bSweepHit = false;
	if (!Result && TraceRadius > 0.f)
	{
		const FVector SweepEnd = bLineHit ? Hit.Location : End;
		bSweepHit = World->SweepSingleByChannel(SweepHit, Start, SweepEnd, FQuat::Identity, TraceChannel,
			FCollisionShape::MakeSphere(TraceRadius), Params);
		if (bSweepHit && IsInteractable(SweepHit.GetActor()))
		{
			Result = SweepHit.GetActor();
		}
	}

#if ENABLE_DRAW_DEBUG
	if (InteractionDebug::GetLevel() > 0)
	{
		const float Life = FMath::Max(FocusUpdateInterval, 0.f) + 0.02f;
		const FColor Color = Result ? FColor::Green : FColor::Red;
		const FVector LineEnd = bLineHit ? Hit.Location : End;
		DrawDebugLine(World, Start, LineEnd, Color, false, Life, 0, 1.f);
		if (bLineHit)
		{
			DrawDebugPoint(World, Hit.Location, 8.f, Color, false, Life);
		}
		if (bSweepHit)
		{
			DrawDebugSphere(World, SweepHit.Location, TraceRadius, 12, Result ? FColor::Green : FColor::Yellow, false, Life);
		}
	}
#endif

	return Result;
}

void UInteractionComponent::SetFocusedActor(AActor* NewFocus)
{
	AActor* OldFocus = FocusedActor.Get();

	// bHasFocus catches the case where the focused actor was destroyed: the weak pointer is now
	// null, but listeners still need to hear that focus was lost.
	if (NewFocus == OldFocus && bHasFocus == (NewFocus != nullptr))
	{
		return;
	}

	APawn* Pawn = GetPawn();

	if (OldFocus)
	{
		SetHighlight(OldFocus, false);
		IInteractable::Execute_OnFocusEnd(OldFocus, Pawn);
	}

	FocusedActor = NewFocus;
	bHasFocus = (NewFocus != nullptr);
	FocusedData = FInteractionData();
	bFocusedCanInteract = false;

	if (NewFocus)
	{
		FocusedData = IInteractable::Execute_GetInteractionData(NewFocus, Pawn);
		bFocusedCanInteract = IInteractable::Execute_CanInteract(NewFocus, Pawn);
		SetHighlight(NewFocus, true);
		IInteractable::Execute_OnFocusBegin(NewFocus, Pawn);
	}

	OnFocusChanged.Broadcast(NewFocus);
}

void UInteractionComponent::ClearFocus()
{
	SetFocusedActor(nullptr);
}

void UInteractionComponent::SetHighlight(AActor* Actor, bool bEnabled) const
{
	if (!bHighlightFocusedActor || !Actor)
	{
		return;
	}

	// Custom depth is per-component render state and is never replicated, so each client
	// highlights only what its own player is looking at.
	TInlineComponentArray<UMeshComponent*> Meshes(Actor);
	for (UMeshComponent* Mesh : Meshes)
	{
		Mesh->SetRenderCustomDepth(bEnabled);
		if (bEnabled)
		{
			Mesh->SetCustomDepthStencilValue(HighlightStencilValue);
		}
	}
}

APawn* UInteractionComponent::GetPawn() const
{
	return GetOwner<APawn>();
}

bool UInteractionComponent::IsLocalPlayerPawn() const
{
	const APawn* Pawn = GetPawn();
	return Pawn && Pawn->IsLocallyControlled() && Pawn->IsPlayerControlled();
}

bool UInteractionComponent::IsInteractable(const AActor* Actor)
{
	// Implements<> checks the class, so it works for Blueprint-only implementers too.
	// The destruction check matters on the server: two players can request the same pickup in
	// one frame, and the second request then references an actor that is already being destroyed.
	return IsValid(Actor) && !Actor->IsActorBeingDestroyed() && Actor->Implements<UInteractable>();
}
