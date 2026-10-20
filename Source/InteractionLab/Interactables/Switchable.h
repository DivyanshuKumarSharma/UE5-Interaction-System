#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Switchable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class USwitchable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Something a lever, button or pressure plate can turn on and off.
 *
 * Senders only know this interface, never the receiver's class, so a lever can drive a door,
 * a light or a Blueprint-only actor with no casting and no compile-time coupling.
 * Called on the server; receivers replicate whatever state results.
 */
class ISwitchable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Switch")
	void SetSwitchState(bool bOn, AActor* Source);

protected:
	virtual void SetSwitchState_Implementation(bool bOn, AActor* Source) {}
};
