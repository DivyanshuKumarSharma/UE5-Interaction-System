#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Count = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryChangedSignature);

/**
 * Deliberately minimal item counter, enough to show a pickup changing another actor's state.
 *
 * Server-authoritative: items are only added on the server and replicated to the owning client
 * only (COND_OwnerOnly), since nobody else needs to know what's in your pockets. A TArray of
 * structs is used because TMap can't be replicated.
 */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void AddItem(FName ItemId, int32 Count = 1);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventoryEntry>& GetItems() const { return Items; }

	/** Fires on the server and the owning client whenever counts change. */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FInventoryChangedSignature OnInventoryChanged;

protected:
	UFUNCTION()
	void OnRep_Items();

private:
	UPROPERTY(ReplicatedUsing = OnRep_Items)
	TArray<FInventoryEntry> Items;
};
