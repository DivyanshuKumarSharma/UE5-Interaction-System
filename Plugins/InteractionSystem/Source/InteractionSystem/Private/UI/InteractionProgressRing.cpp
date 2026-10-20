#include "UI/InteractionProgressRing.h"

#include "UI/SInteractionProgressRing.h"

#define LOCTEXT_NAMESPACE "InteractionSystem"

void UInteractionProgressRing::SetPercent(float InPercent)
{
	Percent = FMath::Clamp(InPercent, 0.f, 1.f);
	if (MyRing.IsValid())
	{
		MyRing->SetPercent(Percent);
	}
}

TSharedRef<SWidget> UInteractionProgressRing::RebuildWidget()
{
	MyRing = SNew(SInteractionProgressRing)
		.Percent(Percent)
		.Thickness(Thickness)
		.FillColor(FillColor)
		.BackgroundColor(BackgroundColor);
	return MyRing.ToSharedRef();
}

void UInteractionProgressRing::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	// Push designer-edited values to the Slate widget (also runs on every edit in the UMG designer).
	if (MyRing.IsValid())
	{
		MyRing->SetPercent(Percent);
		MyRing->SetThickness(Thickness);
		MyRing->SetFillColor(FillColor);
		MyRing->SetBackgroundColor(BackgroundColor);
	}
}

void UInteractionProgressRing::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyRing.Reset();
}

#if WITH_EDITOR
const FText UInteractionProgressRing::GetPaletteCategory()
{
	return LOCTEXT("PaletteCategory", "Interaction");
}
#endif

#undef LOCTEXT_NAMESPACE
