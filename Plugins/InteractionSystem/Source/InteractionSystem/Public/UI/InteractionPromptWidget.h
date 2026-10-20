#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionTypes.h"
#include "InteractionPromptWidget.generated.h"

class UTextBlock;
class UInteractionComponent;
class UInteractionProgressRing;

/**
 * Base class for the "E  Open Door" prompt. C++ owns the behaviour (what to show, when, where);
 * a Widget Blueprint subclass owns the look, binding its widgets by name via BindWidget.
 *
 * Talks to UInteractionComponent only through its public getters and OnFocusChanged, so the
 * component has no knowledge of layout and a project could swap in a completely different UI.
 *
 * When bAnchorToTarget is set, the widget follows the focused actor on screen. In that mode the
 * Blueprint's root must size to content (Overlay / Horizontal Box / Size Box), not a Canvas Panel,
 * because a canvas has no desired size and would collapse to 0x0.
 */
UCLASS(Abstract)
class INTERACTIONSYSTEM_API UInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called by the component that created this widget. */
	void SetInteractionComponent(UInteractionComponent* InComponent);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	UInteractionComponent* GetInteractionComponent() const;

protected:
	//~ UUserWidget
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~ End UUserWidget

	UFUNCTION()
	void HandleFocusChanged(AActor* FocusedActor);

	void RefreshPrompt();
	bool UpdateScreenPosition();

	/** Shows the key mapped to the interact action, e.g. "E". */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidget))
	TObjectPtr<UTextBlock> KeyText;

	/** Shows "Verb ObjectName", e.g. "Open Door". */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidget))
	TObjectPtr<UTextBlock> ActionText;

	/** Optional ring that fills during hold-to-interact; hidden for press interactions. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UInteractionProgressRing> HoldRing;

	/** Follow the focused actor on screen. If false, the Blueprint's own layout decides placement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bAnchorToTarget = true;

	/** World-space offset from the top of the focused actor's bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (EditCondition = "bAnchorToTarget"))
	FVector AnchorOffset = FVector(0.f, 0.f, 20.f);

	/** Render opacity used while the focused actor reports CanInteract = false (e.g. "Locked"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UnavailableOpacity = 0.55f;

	/** Designer hooks for animations/sounds. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPromptShown();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPromptHidden();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPromptUpdated(const FInteractionData& Data, bool bCanInteract);

private:
	TWeakObjectPtr<UInteractionComponent> InteractionComponent;
	bool bPromptActive = false;
};
