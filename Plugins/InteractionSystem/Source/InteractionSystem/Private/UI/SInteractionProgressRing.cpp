#include "UI/SInteractionProgressRing.h"

#include "Rendering/DrawElements.h"

void SInteractionProgressRing::Construct(const FArguments& InArgs)
{
	Percent = FMath::Clamp(InArgs._Percent, 0.f, 1.f);
	Thickness = InArgs._Thickness;
	FillColor = InArgs._FillColor;
	BackgroundColor = InArgs._BackgroundColor;
	Segments = FMath::Max(8, InArgs._Segments);
}

void SInteractionProgressRing::SetPercent(float InPercent)
{
	const float Clamped = FMath::Clamp(InPercent, 0.f, 1.f);
	if (!FMath::IsNearlyEqual(Clamped, Percent))
	{
		Percent = Clamped;
		// Paint-only invalidation: size doesn't change, so no layout pass is needed.
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SInteractionProgressRing::SetThickness(float InThickness)
{
	Thickness = InThickness;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SInteractionProgressRing::SetFillColor(const FLinearColor& InColor)
{
	FillColor = InColor;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SInteractionProgressRing::SetBackgroundColor(const FLinearColor& InColor)
{
	BackgroundColor = InColor;
	Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D SInteractionProgressRing::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(48.f, 48.f);
}

int32 SInteractionProgressRing::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const bool bEnabled = ShouldBeEnabled(bParentEnabled);

	if (BackgroundColor.A > 0.f)
	{
		PaintArc(OutDrawElements, LayerId, AllottedGeometry, 1.f, BackgroundColor, InWidgetStyle, bEnabled);
	}
	if (Percent > 0.f)
	{
		PaintArc(OutDrawElements, LayerId + 1, AllottedGeometry, Percent, FillColor, InWidgetStyle, bEnabled);
	}
	return LayerId + 1;
}

void SInteractionProgressRing::PaintArc(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
	float Fraction, const FLinearColor& Color, const FWidgetStyle& InWidgetStyle, bool bEnabled) const
{
	const FVector2f Size = FVector2f(Geometry.GetLocalSize());
	const FVector2f Center = Size * 0.5f;
	const float Radius = FMath::Max(0.f, FMath::Min(Size.X, Size.Y) * 0.5f - Thickness * 0.5f);

	const int32 NumSegments = FMath::Max(1, FMath::CeilToInt(Segments * Fraction));
	const float StartAngle = -HALF_PI; // 12 o'clock
	const float Sweep = TWO_PI * Fraction;

	TArray<FVector2f> Points;
	Points.Reserve(NumSegments + 1);
	for (int32 i = 0; i <= NumSegments; ++i)
	{
		const float Angle = StartAngle + Sweep * (static_cast<float>(i) / NumSegments);
		Points.Emplace(Center.X + Radius * FMath::Cos(Angle), Center.Y + Radius * FMath::Sin(Angle));
	}

	const ESlateDrawEffect Effect = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	const FLinearColor Tint = Color * InWidgetStyle.GetColorAndOpacityTint();
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), MoveTemp(Points), Effect, Tint, /*bAntialias*/ true, Thickness);
}
