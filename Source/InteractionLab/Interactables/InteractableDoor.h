#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "Switchable.h"
#include "InteractableDoor.generated.h"

class UCurveFloat;
class UStaticMeshComponent;

/**
 * A hinged door that swings open away from whoever opened it.
 *
 * Replication: the only replicated state is TargetYaw (0 = closed, +/-OpenAngle = open), with a
 * RepNotify that starts a local animation on every machine. Movement itself is never replicated,
 * which costs one float per state change instead of a transform stream while swinging.
 * The door is dormant until it changes, so the server doesn't consider it for replication at all
 * while idle.
 *
 * Meshes are assigned in a Blueprint subclass to keep content references out of C++. The door
 * panel should extend along +Y from the Hinge component's origin, with the door's front facing +X.
 */
UCLASS(Blueprintable)
class AInteractableDoor : public AActor, public IInteractable, public ISwitchable
{
	GENERATED_BODY()

public:
	AInteractableDoor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpen() const { return !FMath::IsNearlyZero(TargetYaw); }

	/** Opens or closes the door. Server only; OpenedBy decides the swing direction. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Door")
	void SetOpen(bool bOpen, AActor* OpenedBy = nullptr);

protected:
	//~ IInteractable
	virtual bool CanInteract_Implementation(APawn* InteractingPawn) const override;
	virtual FInteractionData GetInteractionData_Implementation(APawn* InteractingPawn) const override;
	virtual void Interact_Implementation(APawn* InteractingPawn) override;
	//~ End IInteractable

	//~ ISwitchable
	virtual void SetSwitchState_Implementation(bool bOn, AActor* Source) override;
	//~ End ISwitchable

	UFUNCTION()
	void OnRep_TargetYaw();

	/** Cosmetic hook for sounds/VFX. Runs on every machine when the door starts moving. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Door")
	void OnDoorStateChanged(bool bNowOpen);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FrameMesh;

	/** Pivot the panel rotates around. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Hinge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (ClampMin = "1.0", ClampMax = "180.0", Units = "deg"))
	float OpenAngle = 95.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (ClampMin = "0.05", Units = "s"))
	float SwingDuration = 0.6f;

	/** Optional 0..1 easing curve. A hard reference is fine: it's tiny and needed whenever a door exists. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UCurveFloat> SwingCurve;

	/** Swing away from the player who opened it, so it never swings into them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	bool bSwingAwayFromOpener = true;

	/** Can only be operated by a switch (e.g. a lever); players see it as locked. Replicated so the client prompt is right. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Door")
	bool bRequiresSwitch = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Interaction")
	FText ObjectName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Interaction", meta = (ClampMin = "0.0", Units = "s"))
	float HoldDuration = 0.f;

private:
	/** Replicated door state. A single value keeps "open" and "which way" atomic. */
	UPROPERTY(ReplicatedUsing = OnRep_TargetYaw)
	float TargetYaw = 0.f;

	// Local animation state, never replicated.
	float StartYaw = 0.f;
	float CurrentYaw = 0.f;
	float SwingElapsed = 0.f;
};
