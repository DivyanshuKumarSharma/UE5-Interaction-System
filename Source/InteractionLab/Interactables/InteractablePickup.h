#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "InteractablePickup.generated.h"

class UStaticMeshComponent;
class URotatingMovementComponent;

/**
 * An item that adds itself to the interacting pawn's UInventoryComponent and is destroyed.
 * Destruction replicates, so every client sees it disappear; the inventory change only
 * replicates to the collector.
 */
UCLASS(Blueprintable)
class AInteractablePickup : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AInteractablePickup();

protected:
	//~ IInteractable
	virtual bool CanInteract_Implementation(APawn* InteractingPawn) const override;
	virtual FInteractionData GetInteractionData_Implementation(APawn* InteractingPawn) const override;
	virtual void Interact_Implementation(APawn* InteractingPawn) override;
	//~ End IInteractable

	/** Server-side hook before destruction, e.g. to spawn a replicated effect. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
	void OnCollected(APawn* Collector);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Cosmetic spin; runs locally on each machine, never replicated. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URotatingMovementComponent> Spin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FName ItemId = TEXT("Coin");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup|Interaction")
	FText ObjectName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup|Interaction", meta = (ClampMin = "0.0", Units = "s"))
	float HoldDuration = 0.f;
};
