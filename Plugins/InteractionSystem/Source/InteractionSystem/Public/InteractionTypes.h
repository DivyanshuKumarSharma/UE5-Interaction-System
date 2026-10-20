#pragma once

#include "CoreMinimal.h"
#include "InteractionTypes.generated.h"

/**
 * Everything the UI and the interaction component need to know about an interactable
 * at a given moment. Returned by IInteractable::GetInteractionData so it can change at
 * runtime (e.g. a door returns "Open" or "Close" depending on its state).
 */
USTRUCT(BlueprintType)
struct INTERACTIONSYSTEM_API FInteractionData
{
	GENERATED_BODY()

	/** Action shown in the prompt, e.g. "Open". Falls back to "Interact" when empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText Verb;

	/** What is being interacted with, e.g. "Door". Optional. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText ObjectName;

	/** Seconds the input must be held to interact. 0 means a single press. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", Units = "s"))
	float HoldDuration = 0.f;

	bool IsHold() const { return HoldDuration > UE_KINDA_SMALL_NUMBER; }

	/** "Verb ObjectName", built from a format string so translators control word order. */
	FText GetPromptText() const;

	/** Used to avoid re-broadcasting UI updates when nothing visible changed. */
	bool IsEquivalentTo(const FInteractionData& Other) const;
};
