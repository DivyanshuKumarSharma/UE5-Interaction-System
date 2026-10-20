#include "InteractionTypes.h"

#define LOCTEXT_NAMESPACE "InteractionSystem"

FText FInteractionData::GetPromptText() const
{
	const FText EffectiveVerb = Verb.IsEmpty() ? LOCTEXT("DefaultVerb", "Interact") : Verb;
	if (ObjectName.IsEmpty())
	{
		return EffectiveVerb;
	}
	return FText::Format(LOCTEXT("PromptFormat", "{0} {1}"), EffectiveVerb, ObjectName);
}

bool FInteractionData::IsEquivalentTo(const FInteractionData& Other) const
{
	return Verb.EqualTo(Other.Verb)
		&& ObjectName.EqualTo(Other.ObjectName)
		&& FMath::IsNearlyEqual(HoldDuration, Other.HoldDuration);
}

#undef LOCTEXT_NAMESPACE
