#include "InteractableDoor.h"

#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "InteractionLab"

AInteractableDoor::AInteractableDoor()
{
	// Ticks only while swinging.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	SetReplicatingMovement(false);
	// Placed doors start dormant: clients already have their initial state from the map, and the
	// server skips them entirely until SetOpen flushes dormancy.
	NetDormancy = DORM_Initial;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
	FrameMesh->SetupAttachment(Root);

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(Root);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(Hinge);

	ObjectName = LOCTEXT("DoorName", "Door");
}

void AInteractableDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInteractableDoor, TargetYaw);
	DOREPLIFETIME(AInteractableDoor, bRequiresSwitch);
}

bool AInteractableDoor::CanInteract_Implementation(APawn* InteractingPawn) const
{
	return !bRequiresSwitch;
}

FInteractionData AInteractableDoor::GetInteractionData_Implementation(APawn* InteractingPawn) const
{
	FInteractionData Data;
	Data.ObjectName = ObjectName;
	Data.HoldDuration = HoldDuration;
	if (bRequiresSwitch)
	{
		Data.Verb = LOCTEXT("DoorLocked", "Locked");
	}
	else
	{
		Data.Verb = IsOpen() ? LOCTEXT("DoorClose", "Close") : LOCTEXT("DoorOpen", "Open");
	}
	return Data;
}

void AInteractableDoor::Interact_Implementation(APawn* InteractingPawn)
{
	SetOpen(!IsOpen(), InteractingPawn);
}

void AInteractableDoor::SetSwitchState_Implementation(bool bOn, AActor* Source)
{
	SetOpen(bOn, Source);
}

void AInteractableDoor::SetOpen(bool bOpen, AActor* OpenedBy)
{
	if (!HasAuthority() || bOpen == IsOpen())
	{
		return;
	}

	float NewTargetYaw = 0.f;
	if (bOpen)
	{
		// +yaw swings a panel lying along +Y towards -X, i.e. away from someone standing on the +X side.
		float Direction = 1.f;
		if (bSwingAwayFromOpener && OpenedBy)
		{
			const FVector ToOpener = OpenedBy->GetActorLocation() - GetActorLocation();
			Direction = FVector::DotProduct(GetActorForwardVector(), ToOpener) >= 0.f ? 1.f : -1.f;
		}
		NewTargetYaw = Direction * OpenAngle;
	}

	FlushNetDormancy();
	TargetYaw = NewTargetYaw;

	// RepNotifies don't fire on the server in C++, so run the same response by hand. The server
	// animates too, so the door's collision matches what clients see.
	OnRep_TargetYaw();
}

void AInteractableDoor::OnRep_TargetYaw()
{
	// Restart from wherever the panel is now, so reversing mid-swing is smooth.
	StartYaw = CurrentYaw;
	SwingElapsed = 0.f;
	SetActorTickEnabled(true);

	OnDoorStateChanged(IsOpen());
}

void AInteractableDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SwingElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(SwingElapsed / SwingDuration, 0.f, 1.f);
	const float Eased = SwingCurve ? SwingCurve->GetFloatValue(Alpha) : FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);

	CurrentYaw = FMath::Lerp(StartYaw, TargetYaw, Eased);
	Hinge->SetRelativeRotation(FRotator(0.f, CurrentYaw, 0.f));

	if (Alpha >= 1.f)
	{
		SetActorTickEnabled(false);
	}
}

#undef LOCTEXT_NAMESPACE
