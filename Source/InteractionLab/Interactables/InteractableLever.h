#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "InteractableLever.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLeverToggledSignature, bool, bIsOn);

/**
 * Toggles itself and forwards the new state to any number of ISwitchable target actors.
 *
 * The lever never knows what its targets are: anything implementing ISwitchable works,
 * including Blueprint-only actors. Designers can also bind OnToggled in the level Blueprint.
 */
UCLASS(Blueprintable)
class AInteractableLever : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AInteractableLever();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Lever")
	bool IsOn() const { return bIsOn; }

	/** Fires on every machine when the lever changes state. */
	UPROPERTY(BlueprintAssignable, Category = "Lever")
	FLeverToggledSignature OnToggled;

protected:
	//~ IInteractable
	virtual FInteractionData GetInteractionData_Implementation(APawn* InteractingPawn) const override;
	virtual void Interact_Implementation(APawn* InteractingPawn) override;
	//~ End IInteractable

	UFUNCTION()
	void OnRep_IsOn();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> HandlePivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HandleMesh;

	/**
	 * Actors to switch. Soft references: in a World Partition level a hard actor reference would
	 * force the target to load with the lever. Unloaded targets are skipped.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lever")
	TArray<TSoftObjectPtr<AActor>> Targets;

	/** Handle tilt either side of upright. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lever", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "deg"))
	float HandleAngle = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lever", meta = (ClampMin = "1.0", Units = "deg/s"))
	float HandleSpeed = 240.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lever|Interaction")
	FText ObjectName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lever|Interaction", meta = (ClampMin = "0.0", Units = "s"))
	float HoldDuration = 0.f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_IsOn)
	bool bIsOn = false;

	float GetTargetPitch() const { return bIsOn ? -HandleAngle : HandleAngle; }
};
