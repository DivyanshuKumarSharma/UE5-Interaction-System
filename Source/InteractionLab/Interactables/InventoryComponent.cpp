#include "InventoryComponent.h"

#include "InteractionLab.h"
#include "Net/UnrealNetwork.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UInventoryComponent, Items, COND_OwnerOnly);
}

void UInventoryComponent::AddItem(FName ItemId, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || ItemId.IsNone() || Count <= 0)
	{
		return;
	}

	FInventoryEntry* Entry = Items.FindByPredicate([ItemId](const FInventoryEntry& E) { return E.ItemId == ItemId; });
	if (Entry)
	{
		Entry->Count += Count;
	}
	else
	{
		Items.Add({ ItemId, Count });
	}

	UE_LOG(LogInteractionLab, Log, TEXT("%s now has %d x %s"), *GetNameSafe(GetOwner()), GetItemCount(ItemId), *ItemId.ToString());

	// Server-side listeners (and a listen-server host's own UI) don't get the RepNotify.
	OnInventoryChanged.Broadcast();
}

int32 UInventoryComponent::GetItemCount(FName ItemId) const
{
	const FInventoryEntry* Entry = Items.FindByPredicate([ItemId](const FInventoryEntry& E) { return E.ItemId == ItemId; });
	return Entry ? Entry->Count : 0;
}

void UInventoryComponent::OnRep_Items()
{
	OnInventoryChanged.Broadcast();
}
