#include "InteractableLever.h"

#include "Components/StaticMeshComponent.h"
#include "InteractionLab.h"
#include "Net/UnrealNetwork.h"
#include "Switchable.h"

#define LOCTEXT_NAMESPACE "InteractionLab"

AInteractableLever::AInteractableLever()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	SetReplicatingMovement(false);
	NetDormancy = DORM_Initial;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(Root);

	HandlePivot = CreateDefaultSubobject<USceneComponent>(TEXT("HandlePivot"));
	HandlePivot->SetupAttachment(Root);

	HandleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleMesh"));
	HandleMesh->SetupAttachment(HandlePivot);

	ObjectName = LOCTEXT("LeverName", "Lever");
}

void AInteractableLever::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInteractableLever, bIsOn);
}

void AInteractableLever::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Snap the handle to its resting pose, so it also looks right in the editor viewport.
	HandlePivot->SetRelativeRotation(FRotator(GetTargetPitch(), 0.f, 0.f));
}

FInteractionData AInteractableLever::GetInteractionData_Implementation(APawn* InteractingPawn) const
{
	FInteractionData Data;
	Data.Verb = LOCTEXT("LeverVerb", "Pull");
	Data.ObjectName = ObjectName;
	Data.HoldDuration = HoldDuration;
	return Data;
}

void AInteractableLever::Interact_Implementation(APawn* InteractingPawn)
{
	FlushNetDormancy();
	bIsOn = !bIsOn;
	OnRep_IsOn();

	// Talk to targets through the interface only; the lever has no idea what they are.
	for (const TSoftObjectPtr<AActor>& SoftTarget : Targets)
	{
		AActor* Target = SoftTarget.Get();
		if (!Target)
		{
			UE_LOG(LogInteractionLab, Verbose, TEXT("%s: target %s is not loaded, skipping."), *GetName(), *SoftTarget.ToString());
			continue;
		}
		if (!Target->Implements<USwitchable>())
		{
			UE_LOG(LogInteractionLab, Warning, TEXT("%s: target %s does not implement Switchable."), *GetName(), *Target->GetName());
			continue;
		}
		ISwitchable::Execute_SetSwitchState(Target, bIsOn, this);
	}
}

void AInteractableLever::OnRep_IsOn()
{
	SetActorTickEnabled(true);
	OnToggled.Broadcast(bIsOn);
}

void AInteractableLever::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FRotator Current = HandlePivot->GetRelativeRotation();
	const FRotator Target(GetTargetPitch(), 0.f, 0.f);
	const FRotator Next = FMath::RInterpConstantTo(Current, Target, DeltaSeconds, HandleSpeed);
	HandlePivot->SetRelativeRotation(Next);

	if (Next.Equals(Target, 0.1f))
	{
		SetActorTickEnabled(false);
	}
}

#undef LOCTEXT_NAMESPACE
