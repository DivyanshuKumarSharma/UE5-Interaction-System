#include "InteractablePickup.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "InteractionLab.h"
#include "InventoryComponent.h"

#define LOCTEXT_NAMESPACE "InteractionLab"

AInteractablePickup::AInteractablePickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// Players walk through pickups, but interaction traces must still hit them.
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	Mesh->SetCollisionResponseToChannel(ECC_Interaction, ECR_Block);

	Spin = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("Spin"));
	Spin->RotationRate = FRotator(0.f, 90.f, 0.f);

	ObjectName = LOCTEXT("PickupName", "Coin");
}

bool AInteractablePickup::CanInteract_Implementation(APawn* InteractingPawn) const
{
	return InteractingPawn && InteractingPawn->FindComponentByClass<UInventoryComponent>();
}

FInteractionData AInteractablePickup::GetInteractionData_Implementation(APawn* InteractingPawn) const
{
	FInteractionData Data;
	Data.Verb = LOCTEXT("PickupVerb", "Pick up");
	Data.ObjectName = Quantity > 1
		? FText::Format(LOCTEXT("PickupNameQuantity", "{0} x{1}"), ObjectName, Quantity)
		: ObjectName;
	Data.HoldDuration = HoldDuration;
	return Data;
}

void AInteractablePickup::Interact_Implementation(APawn* InteractingPawn)
{
	UInventoryComponent* Inventory = InteractingPawn ? InteractingPawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	if (!Inventory)
	{
		return;
	}

	Inventory->AddItem(ItemId, Quantity);
	OnCollected(InteractingPawn);
	Destroy();
}

#undef LOCTEXT_NAMESPACE
