
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EndPointActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEndPointEvent);

UCLASS(Blueprintable)
class BLASTER_API AEndPointActor : public AActor
{
	GENERATED_BODY()

public:

	AEndPointActor();

	// Whether the endpoint starts activated.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Endpoint")
	bool bAutoActivate = true;

	// Endpoint trigger sphere.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Endpoint")
	TObjectPtr<USphereComponent> Sphere;

	// Visual endpoint mesh.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Endpoint")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	// Broadcast when the player reaches the endpoint.
	UPROPERTY(BlueprintAssignable, Category = "Endpoint|Events")
	FEndPointEvent EEndReached;

	// Broadcast when the endpoint becomes active.
	UPROPERTY(BlueprintAssignable, Category = "Endpoint|Events")
	FEndPointEvent EEndActivated;

	UFUNCTION(BlueprintCallable, Category = "Endpoint")
	void ActivateEndpoint();

protected:

	virtual void BeginPlay() override;

private:

	UFUNCTION()
	void HandleSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	bool bEndpointActivated = false;

	bool bEndReachedAlready = false;
};
