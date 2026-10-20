#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "InteractionTypes.h"
#include "InteractionComponent.generated.h"

class APawn;
class AController;
class UInputAction;
class UEnhancedInputComponent;
class UInteractionPromptWidget;
struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractionFocusChangedSignature, AActor*, FocusedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractionHoldSignature, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInteractionPerformedSignature, AActor*, Target, APawn*, InteractingPawn);

/**
 * Gives a pawn the ability to interact with IInteractable actors.
 *
 * Lives on the pawn (not the controller) so it follows possession and can be added to any
 * pawn class, C++ or Blueprint, without changing its parent class.
 *
 * Client side (locally controlled player only):
 *  - Focus detection runs on a throttled component tick (FocusUpdateInterval) instead of every
 *    frame. A few frames of focus latency are imperceptible, and remote proxies / the server's
 *    copies of other players never trace at all.
 *  - Press vs hold is decided by the focused actor's FInteractionData, not by the input asset,
 *    so each interactable can have its own hold duration.
 *
 * Server side:
 *  - The client only requests an interaction. ServerInteract re-validates availability, reach,
 *    line of sight and hold time before calling IInteractable::Interact.
 */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class INTERACTIONSYSTEM_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	//~ UActorComponent
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent

	/** Binds InteractAction (Started/Completed/Canceled) to Start/StopInteract. Call from SetupPlayerInputComponent. */
	void SetupInputBindings(UEnhancedInputComponent* EnhancedInputComponent);

	/** Input pressed. Interacts immediately, or starts a hold if the focused actor requires one. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StartInteract();

	/** Input released. Cancels an in-progress hold. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StopInteract();

	/** The interactable currently under the player's aim, if any. Local only. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	/** Prompt data of the focused actor, cached at the last focus update. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	const FInteractionData& GetFocusedData() const { return FocusedData; }

	/** Whether the focused actor reported CanInteract at the last focus update. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool CanInteractWithFocus() const { return FocusedActor.IsValid() && bFocusedCanInteract; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsHolding() const { return bIsHolding; }

	/** 0..1 progress of the current hold, 0 when not holding. Cheap enough to poll from UI every frame. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	float GetHoldProgress() const;

	/** Display name of the key currently mapped to InteractAction (e.g. "E"), for prompts. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetInteractKeyText() const;

	/** Re-run the focus trace immediately instead of waiting for the next update. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ForceFocusUpdate();

	/** Fires when the focused actor changes, or when its prompt data / availability changes. Local only. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FInteractionFocusChangedSignature OnFocusChanged;

	/** A hold-to-interact started. Local only. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FInteractionHoldSignature OnHoldStarted;

	/** A hold-to-interact was released early or lost focus. Local only. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FInteractionHoldSignature OnHoldCanceled;

	/** A validated interaction was performed. Server only. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FInteractionPerformedSignature OnInteracted;

	// --- Settings. Edit per pawn class in the Details panel; nothing here is hard-coded. ---

	/** Input action that triggers interaction. A plain digital action with no Hold trigger: hold timing comes from the interactable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Input")
	TObjectPtr<UInputAction> InteractAction;

	/** How far in front of the pawn things can be interacted with (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Trace", meta = (ClampMin = "10.0", Units = "cm"))
	float InteractionDistance = 250.f;

	/** Radius of the forgiving sphere sweep used when the precise line trace misses. 0 = line trace only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Trace", meta = (ClampMin = "0.0", Units = "cm"))
	float TraceRadius = 15.f;

	/** Collision channel used for focus traces and server line-of-sight checks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Seconds between focus traces. 0 traces every frame. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Trace", meta = (ClampMin = "0.0", Units = "s"))
	float FocusUpdateInterval = 0.05f;

	/** Render the focused actor's meshes into custom depth so a post-process material can outline them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Highlight")
	bool bHighlightFocusedActor = true;

	/** Stencil value written while focused. Must match the outline post-process material. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Highlight", meta = (ClampMin = "1", ClampMax = "255", EditCondition = "bHighlightFocusedActor"))
	int32 HighlightStencilValue = 252;

	/**
	 * Prompt widget created for the local player. Soft reference: it is async-loaded only when a
	 * pawn becomes locally player-controlled, so dedicated servers and AI never load UI assets.
	 * Leave empty to drive your own UI from OnFocusChanged and the getters above.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|UI")
	TSoftClassPtr<UInteractionPromptWidget> PromptWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|UI")
	int32 PromptZOrder = 10;

	/** Extra reach the server allows on top of InteractionDistance, to absorb movement since the client traced. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Networking", meta = (ClampMin = "0.0", Units = "cm"))
	float ServerDistanceTolerance = 75.f;

	/** How much shorter than HoldDuration a hold may be on the server's clock, to absorb network jitter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Networking", meta = (ClampMin = "0.0", Units = "s"))
	float HoldTimeTolerance = 0.2f;

protected:
	// --- Focus (local) ---

	/** Runs the focus trace and updates focus state, highlight and listeners. */
	void UpdateFocus();

	/** Returns the interactable under the player's aim, or null. */
	AActor* TraceForInteractable() const;

	/** Where the aim ray starts and points. Uses the controller's view (the camera for players). */
	bool GetAimRay(FVector& OutStart, FVector& OutDirection) const;

	void SetFocusedActor(AActor* NewFocus);
	void ClearFocus();
	void SetHighlight(AActor* Actor, bool bEnabled) const;

	// --- Hold (local) ---

	void BeginHold(AActor* Target, float Duration);
	void CompleteHold();
	void CancelHold();

	// --- Server ---

	/** Records when a hold started so ServerInteract can check that it lasted long enough. */
	UFUNCTION(Server, Reliable)
	void ServerBeginHold(AActor* Target);

	/** Requests an interaction. Reliable: rare, and a dropped "open door" would feel like a bug. */
	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* Target);

	/** Server-side rule checks. Returns false with a reason instead of trusting the client. */
	bool ValidateInteraction(AActor* Target, FString& OutFailureReason) const;

	// --- UI (local) ---

	void CreatePromptWidget();
	void OnPromptWidgetClassLoaded();
	void DestroyPromptWidget();

	// --- Possession ---

	/** Enables focus detection only while owned by a locally controlled player. */
	void RefreshLocalControlState();

	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	APawn* GetPawn() const;
	bool IsLocalPlayerPawn() const;

	static bool IsInteractable(const AActor* Actor);

private:
	/** Weak: the focused actor can be destroyed at any time (e.g. a pickup collected by another player). */
	TWeakObjectPtr<AActor> FocusedActor;
	FInteractionData FocusedData;
	bool bHasFocus = false;
	bool bFocusedCanInteract = false;
	bool bIsLocalPlayer = false;

	// Local hold state
	TWeakObjectPtr<AActor> HoldTarget;
	FTimerHandle HoldTimerHandle;
	double HoldStartTime = 0.0;
	float HoldDuration = 0.f;
	bool bIsHolding = false;

	UPROPERTY(Transient)
	TObjectPtr<UInteractionPromptWidget> PromptWidget;

	TSharedPtr<FStreamableHandle> PromptWidgetLoadHandle;

	// Server hold bookkeeping
	TWeakObjectPtr<AActor> ServerHoldTarget;
	double ServerHoldStartTime = 0.0;
};
