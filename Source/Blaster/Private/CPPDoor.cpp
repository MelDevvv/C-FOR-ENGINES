#include "CPPDoor.h"

#include "Components/SceneComponent.h"
#include "UObject/UnrealType.h"

ACPPDoor::ACPPDoor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void ACPPDoor::BeginPlay()
{
    Super::BeginPlay();

    // bpdoors existing component is named cube find it without replacing
    // the blueprint component or changing any placedinstance references
    TArray<USceneComponent*> SceneComponents;
    GetComponents<USceneComponent>(SceneComponents);
    for (USceneComponent* Component : SceneComponents)
    {
        if (IsValid(Component) && Component->GetFName() == TEXT("Cube"))
        {
            DoorMeshComponent = Component;
            break;
        }
    }

    if (!IsValid(DoorMeshComponent))
    {
        DoorMeshComponent = GetRootComponent();
    }

    if (IsValid(DoorMeshComponent))
    {
        // the original blueprint beginplay stores cubes initial world
        // location in doorclosed do the same natively
        ClosedWorldLocation = DoorMeshComponent->GetComponentLocation();
        WriteBlueprintVector(TEXT("DoorClosed"), ClosedWorldLocation);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: BP_Door component 'Cube' was not found."), *GetName());
    }

    SetActorTickEnabled(false);
}

void ACPPDoor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bIsMoving || !IsValid(DoorMeshComponent))
    {
        SetActorTickEnabled(false);
        return;
    }

    MoveElapsed = FMath::Min(MoveElapsed + DeltaSeconds, MoveDuration);
    const float Alpha = MoveDuration > KINDA_SMALL_NUMBER
        ? FMath::Clamp(MoveElapsed / MoveDuration, 0.0f, 1.0f)
        : 1.0f;

    // tdoor is a 0to1 linear curve with a 05 second duration
    DoorMeshComponent->SetWorldLocation(FMath::Lerp(MoveStartLocation, MoveTargetLocation, Alpha));

    if (Alpha >= 1.0f)
    {
        FinishDoorMovement();
    }
}

void ACPPDoor::Interact_Implementation(AActor* InteractorPawn)
{
    // match the blueprint is valid check ignore a nullinvalid interactor
    if (!IsValid(InteractorPawn) || !IsValid(DoorMeshComponent) || bIsMoving)
    {
        return;
    }

    const bool bCurrentlyOpen = ReadBlueprintBool(TEXT("bIsOpen"), false);
    StartDoorMovement(!bCurrentlyOpen);
}

FVector ACPPDoor::ReadBlueprintVector(FName PropertyName, const FVector& Fallback) const
{
    const FStructProperty* Property = FindFProperty<FStructProperty>(GetClass(), PropertyName);
    if (!Property || Property->Struct != TBaseStructure<FVector>::Get())
    {
        return Fallback;
    }

    const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(this);
    return ValuePtr ? *static_cast<const FVector*>(ValuePtr) : Fallback;
}

bool ACPPDoor::ReadBlueprintBool(FName PropertyName, bool Fallback) const
{
    const FBoolProperty* Property = FindFProperty<FBoolProperty>(GetClass(), PropertyName);
    return Property ? Property->GetPropertyValue_InContainer(this) : Fallback;
}

void ACPPDoor::WriteBlueprintVector(FName PropertyName, const FVector& Value)
{
    if (FStructProperty* Property = FindFProperty<FStructProperty>(GetClass(), PropertyName))
    {
        if (Property->Struct == TBaseStructure<FVector>::Get())
        {
            Property->CopyCompleteValue_InContainer(this, &Value);
        }
    }
}

void ACPPDoor::WriteBlueprintBool(FName PropertyName, bool Value)
{
    if (FBoolProperty* Property = FindFProperty<FBoolProperty>(GetClass(), PropertyName))
    {
        Property->SetPropertyValue_InContainer(this, Value);
    }
}

void ACPPDoor::StartDoorMovement(bool bOpen)
{
    if (!IsValid(DoorMeshComponent))
    {
        return;
    }

    const FVector OpenOffset = ReadBlueprintVector(TEXT("DoorOpenOffset"), FVector(0.0f, 0.0f, -100.0f));

    MoveStartLocation = DoorMeshComponent->GetComponentLocation();
    MoveTargetLocation = bOpen ? ClosedWorldLocation + OpenOffset : ClosedWorldLocation;
    MoveElapsed = 0.0f;
    bMoveWillOpen = bOpen;
    bIsMoving = true;

    SetActorTickEnabled(true);
}

void ACPPDoor::FinishDoorMovement()
{
    if (IsValid(DoorMeshComponent))
    {
        DoorMeshComponent->SetWorldLocation(MoveTargetLocation);
    }

    WriteBlueprintBool(TEXT("bIsOpen"), bMoveWillOpen);
    bIsMoving = false;
    SetActorTickEnabled(false);
}
