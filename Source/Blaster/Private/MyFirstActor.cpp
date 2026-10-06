#include "MyFirstActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
AMyFirstActor::AMyFirstActor()
{
	_collider = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = _collider;
	_staticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	_staticMesh->SetupAttachment(_collider);
}