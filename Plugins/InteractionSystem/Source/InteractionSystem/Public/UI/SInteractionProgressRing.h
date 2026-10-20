#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/**
 * Circular progress indicator drawn with anti-aliased line segments.
 * Pure Slate so it needs no material or texture; fills clockwise from 12 o'clock.
 */
class INTERACTIONSYSTEM_API SInteractionProgressRing : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SInteractionProgressRing)
		: _Percent(0.f)
		, _Thickness(4.f)
		, _FillColor(FLinearColor::White)
		, _BackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.2f))
		, _Segments(64)
	{}
		SLATE_ARGUMENT(float, Percent)
		SLATE_ARGUMENT(float, Thickness)
		SLATE_ARGUMENT(FLinearColor, FillColor)
		SLATE_ARGUMENT(FLinearColor, BackgroundColor)
		SLATE_ARGUMENT(int32, Segments)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetPercent(float InPercent);
	void SetThickness(float InThickness);
	void SetFillColor(const FLinearColor& InColor);
	void SetBackgroundColor(const FLinearColor& InColor);

	//~ SWidget
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	//~ End SWidget

private:
	void PaintArc(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
		float Fraction, const FLinearColor& Color, const FWidgetStyle& InWidgetStyle, bool bEnabled) const;

	float Percent = 0.f;
	float Thickness = 4.f;
	FLinearColor FillColor;
	FLinearColor BackgroundColor;
	int32 Segments = 64;
};
