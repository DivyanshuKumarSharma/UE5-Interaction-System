#include "Interactable.h"

// Defaults so C++ implementers only override what they need.

bool IInteractable::CanInteract_Implementation(APawn* InteractingPawn) const
{
	return true;
}

FInteractionData IInteractable::GetInteractionData_Implementation(APawn* InteractingPawn) const
{
	return FInteractionData();
}

void IInteractable::OnFocusBegin_Implementation(APawn* InteractingPawn)
{
}

void IInteractable::OnFocusEnd_Implementation(APawn* InteractingPawn)
{
}

void IInteractable::Interact_Implementation(APawn* InteractingPawn)
{
}
