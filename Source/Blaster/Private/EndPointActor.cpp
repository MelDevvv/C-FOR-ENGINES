
#include "EndPointActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Tank.h"

AEndPointActor::AEndPointActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // create endpoint trigger
    Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));

    SetRootComponent(Sphere);

    Sphere->InitSphereRadius(200.0f);

    Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Sphere->SetCollisionObjectType(ECC_WorldDynamic);

    Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    Sphere->SetGenerateOverlapEvents(true);

    // create visual mesh
    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));

    StaticMesh->SetupAttachment(Sphere);

    StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StaticMesh->SetGenerateOverlapEvents(false);
}

void AEndPointActor::BeginPlay()
{
    Super::BeginPlay();

    // recover references if necessary
    if (!IsValid(Sphere))
    {
        Sphere = FindComponentByClass<USphereComponent>();
    }

    if (!IsValid(StaticMesh))
    {
        StaticMesh = FindComponentByClass<UStaticMeshComponent>();
    }

    // do not continue if the trigger is missing
    if (!IsValid(Sphere))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("%s: Endpoint Sphere component is missing!"),
            *GetName()
        );

        return;
    }

    // register overlap event
    Sphere->OnComponentBeginOverlap.AddUniqueDynamic(
        this,
        &AEndPointActor::HandleSphereBeginOverlap
    );

    // apply initial visibility
    if (IsValid(StaticMesh))
    {
        StaticMesh->SetVisibility(bAutoActivate, true);
    }

    // apply initial collision
    Sphere->SetCollisionEnabled(
        bAutoActivate
            ? ECollisionEnabled::QueryOnly
            : ECollisionEnabled::NoCollision
    );

    bEndpointActivated = bAutoActivate;
    bEndReachedAlready = false;

    // notify listeners if automatically activated
    if (bAutoActivate)
    {
        EEndActivated.Broadcast();
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s: Endpoint initialized. AutoActivate=%s"),
        *GetName(),
        bAutoActivate ? TEXT("true") : TEXT("false")
    );
}

void AEndPointActor::ActivateEndpoint()
{
    if (bEndpointActivated || !IsValid(Sphere))
    {
        return;
    }

    bEndpointActivated = true;

    if (IsValid(StaticMesh))
    {
        StaticMesh->SetVisibility(true, true);
    }

    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    EEndActivated.Broadcast();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s: Endpoint activated."),
        *GetName()
    );
}

void AEndPointActor::HandleSphereBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    if (!bEndpointActivated || bEndReachedAlready)
    {
        return;
    }

    ATank* Tank = Cast<ATank>(OtherActor);

    if (!IsValid(Tank))
    {
        return;
    }

    bEndReachedAlready = true;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s: Tank reached endpoint."),
        *GetName()
    );

    EEndReached.Broadcast();
}
