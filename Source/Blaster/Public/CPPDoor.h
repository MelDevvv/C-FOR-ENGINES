#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CPPDoor.generated.h"

class USceneComponent;

/**
 * Native BP_Door implementation.
 * BP_Door can retain its existing Blueprint-only BPI_Interact interface.
 * The native Interact UFunction provides the matching reflected entry point,
 * so the interface call reaches this class without any EventGraph nodes.
 */
UCLASS(Blueprintable)
class BLASTER_API ACPPDoor : public AActor
{
    GENERATED_BODY()

public:
    ACPPDoor();

    // must match the existing bpiinteract function name and parameter
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void Interact(AActor* InteractorPawn);
    virtual void Interact_Implementation(AActor* InteractorPawn);

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaSeconds) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> DoorMeshComponent;

    FVector ClosedWorldLocation = FVector::ZeroVector;
    FVector MoveStartLocation = FVector::ZeroVector;
    FVector MoveTargetLocation = FVector::ZeroVector;

    float MoveElapsed = 0.0f;
    float MoveDuration = 0.5f;
    bool bIsMoving = false;
    bool bMoveWillOpen = false;

    FVector ReadBlueprintVector(FName PropertyName, const FVector& Fallback) const;
    bool ReadBlueprintBool(FName PropertyName, bool Fallback) const;
    void WriteBlueprintVector(FName PropertyName, const FVector& Value);
    void WriteBlueprintBool(FName PropertyName, bool Value);
    void StartDoorMovement(bool bOpen);
    void FinishDoorMovement();
};
