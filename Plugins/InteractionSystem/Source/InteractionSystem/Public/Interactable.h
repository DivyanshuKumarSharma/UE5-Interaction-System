#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionTypes.h"
#include "Interactable.generated.h"

class APawn;

UINTERFACE(MinimalAPI, BlueprintType, meta = (DisplayName = "Interactable"))
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement on any actor (C++ or Blueprint) to make it interactable.
 *
 * An interface rather than a base class so that interactability can be added to actors
 * that already have a parent (characters, static mesh actors, vehicles) without forcing
 * them into one inheritance chain.
 *
 * Network contract:
 *  - CanInteract / GetInteractionData run on the owning client (to drive the prompt) AND on
 *    the server (to validate). They must not have side effects.
 *  - OnFocusBegin / OnFocusEnd are local and cosmetic only; they never run on a dedicated server.
 *  - Interact runs on the server only. Replicate any resulting state (e.g. RepNotify).
 *
 * Always call through the generated Execute_ wrappers (IInteractable::Execute_Interact(Actor, Pawn))
 * so Blueprint implementations are dispatched correctly.
 */
class INTERACTIONSYSTEM_API IInteractable
{
	GENERATED_BODY()

public:
	/** Whether InteractingPawn may interact right now (e.g. false for a locked door). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(APawn* InteractingPawn) const;

	/** Prompt text and press/hold settings. May depend on state and on who is asking. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FInteractionData GetInteractionData(APawn* InteractingPawn) const;

	/** The local player started looking at this actor. Cosmetic only. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnFocusBegin(APawn* InteractingPawn);

	/** The local player stopped looking at this actor. Cosmetic only. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnFocusEnd(APawn* InteractingPawn);

	/** Perform the interaction. Called on the server after validation. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(APawn* InteractingPawn);

protected:
	// Declared explicitly so C++ implementers inherit sensible defaults (CanInteract = true)
	// instead of the UHT-generated ones, and only override what they need.
	virtual bool CanInteract_Implementation(APawn* InteractingPawn) const;
	virtual FInteractionData GetInteractionData_Implementation(APawn* InteractingPawn) const;
	virtual void OnFocusBegin_Implementation(APawn* InteractingPawn);
	virtual void OnFocusEnd_Implementation(APawn* InteractingPawn);
	virtual void Interact_Implementation(APawn* InteractingPawn);
};
