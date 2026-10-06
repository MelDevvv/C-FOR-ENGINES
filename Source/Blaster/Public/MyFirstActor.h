#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyFirstActor.generated.h"
class UBoxComponent;
UCLASS(Abstract)
class BLASTER_API AMyFirstActor : public AActor
{
GENERATED_BODY()
public:
AMyFirstActor();
private:
UPROPERTY(EditAnywhere)
TObjectPtr<UBoxComponent> _collider;
UPROPERTY(EditAnywhere)
TObjectPtr<UStaticMeshComponent> _staticMesh;
};