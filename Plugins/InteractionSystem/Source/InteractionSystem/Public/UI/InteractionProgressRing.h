#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "InteractionProgressRing.generated.h"

class SInteractionProgressRing;

/** UMG wrapper so designers can place and style the ring in the widget editor. */
UCLASS(meta = (DisplayName = "Interaction Progress Ring"))
class INTERACTIONSYSTEM_API UInteractionProgressRing : public UWidget
{
	GENERATED_BODY()

public:
	/** 0..1 fill amount. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progress", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Percent = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance", meta = (ClampMin = "1.0"))
	float Thickness = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FLinearColor FillColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FLinearColor BackgroundColor = FLinearColor(1.f, 1.f, 1.f, 0.2f);

	UFUNCTION(BlueprintCallable, Category = "Progress")
	void SetPercent(float InPercent);

	//~ UWidget
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif
	//~ End UWidget

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	TSharedPtr<SInteractionProgressRing> MyRing;
};
