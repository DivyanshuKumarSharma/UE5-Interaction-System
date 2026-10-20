#include "UI/InteractionPromptWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "InteractionComponent.h"
#include "UI/InteractionProgressRing.h"

void UInteractionPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::Collapsed);
	if (bAnchorToTarget)
	{
		SetAlignmentInViewport(FVector2D(0.5f, 1.f)); // bottom-centre sits on the anchor point
	}
}

void UInteractionPromptWidget::NativeDestruct()
{
	if (UInteractionComponent* Component = InteractionComponent.Get())
	{
		Component->OnFocusChanged.RemoveDynamic(this, &UInteractionPromptWidget::HandleFocusChanged);
	}
	Super::NativeDestruct();
}

void UInteractionPromptWidget::SetInteractionComponent(UInteractionComponent* InComponent)
{
	if (UInteractionComponent* Old = InteractionComponent.Get())
	{
		Old->OnFocusChanged.RemoveDynamic(this, &UInteractionPromptWidget::HandleFocusChanged);
	}

	InteractionComponent = InComponent;

	if (InComponent)
	{
		InComponent->OnFocusChanged.AddDynamic(this, &UInteractionPromptWidget::HandleFocusChanged);
	}
	RefreshPrompt();
}

UInteractionComponent* UInteractionPromptWidget::GetInteractionComponent() const
{
	return InteractionComponent.Get();
}

void UInteractionPromptWidget::HandleFocusChanged(AActor* FocusedActor)
{
	RefreshPrompt();
}

void UInteractionPromptWidget::RefreshPrompt()
{
	const UInteractionComponent* Component = InteractionComponent.Get();
	const bool bHasFocus = Component && Component->GetFocusedActor();

	if (!bHasFocus)
	{
		if (bPromptActive)
		{
			bPromptActive = false;
			SetVisibility(ESlateVisibility::Collapsed);
			OnPromptHidden();
		}
		return;
	}

	const FInteractionData& Data = Component->GetFocusedData();
	const bool bCanInteract = Component->CanInteractWithFocus();

	ActionText->SetText(Data.GetPromptText());
	KeyText->SetText(Component->GetInteractKeyText());
	KeyText->SetVisibility(bCanInteract ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	SetRenderOpacity(bCanInteract ? 1.f : UnavailableOpacity);

	if (HoldRing)
	{
		HoldRing->SetPercent(0.f);
		HoldRing->SetVisibility(Data.IsHold() && bCanInteract ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (!bPromptActive)
	{
		bPromptActive = true;
		if (bAnchorToTarget)
		{
			// Place it before the first paint so it doesn't flash at its previous position.
			UpdateScreenPosition();
		}
		// HitTestInvisible: a prompt must never steal mouse input from the game.
		SetVisibility(ESlateVisibility::HitTestInvisible);
		OnPromptShown();
	}

	OnPromptUpdated(Data, bCanInteract);
}

void UInteractionPromptWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const UInteractionComponent* Component = InteractionComponent.Get();
	if (!bPromptActive || !Component)
	{
		return;
	}

	// Polled rather than event-driven: progress changes every frame while holding anyway.
	if (HoldRing)
	{
		HoldRing->SetPercent(Component->GetHoldProgress());
	}

	if (bAnchorToTarget)
	{
		// Fade via ColorAndOpacity, not visibility: a Hidden widget isn't painted, so it would
		// also stop ticking and never come back when the anchor returns on screen.
		SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, UpdateScreenPosition() ? 1.f : 0.f));
	}
}

bool UInteractionPromptWidget::UpdateScreenPosition()
{
	const UInteractionComponent* Component = InteractionComponent.Get();
	AActor* Target = Component ? Component->GetFocusedActor() : nullptr;
	APlayerController* PC = GetOwningPlayer();
	if (!Target || !PC)
	{
		return false;
	}

	const FBox Bounds = Target->GetComponentsBoundingBox();
	const FVector Anchor = Bounds.IsValid
		? FVector(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Max.Z) + AnchorOffset
		: Target->GetActorLocation() + AnchorOffset;

	// Widget-space (DPI-scaled) position relative to this player's viewport, so it is correct
	// in split screen and at any resolution.
	FVector2D ScreenPosition;
	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Anchor, ScreenPosition, /*bPlayerViewportRelative*/ true))
	{
		return false;
	}

	SetPositionInViewport(ScreenPosition, /*bRemoveDPIScale*/ false);
	return true;
}
