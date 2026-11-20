#include "Rocket.h"
#include "Tank.h"
#include "BaseEnemyTurret.h"

#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "UObject/ConstructorHelpers.h"

ARocket::ARocket()
{
    PrimaryActorTick.bCanEverTick = false;
    SetCanBeDamaged(true);

    static ConstructorHelpers::FObjectFinder<UParticleSystem> ExplosionAsset(
        TEXT("/Game/Assets/Effects/PS_RocketExplosion.PS_RocketExplosion")
    );

    if (ExplosionAsset.Succeeded())
    {
        ExplosionEffect = ExplosionAsset.Object;
    }
}

void ARocket::BeginPlay()
{
    Super::BeginPlay();

    CacheBlueprintComponents();

    if (!IsValid(CachedSphere))
    {
        UE_LOG(LogTemp, Error,
            TEXT("%s: BP_Rocket's Sphere component was not found; impact handling cannot be bound."),
            *GetName());
        return;
    }

    // Preserve the Blueprint Sphere's collision profile and authored settings.
    // Only bind the native equivalent of OnComponentBeginOverlap.
    CachedSphere->OnComponentBeginOverlap.AddUniqueDynamic(
        this, &ARocket::HandleSphereBeginOverlap);
}

void ARocket::CacheBlueprintComponents()
{
    TArray<UActorComponent*> Components;
    GetComponents<UActorComponent>(Components);

    CachedSphere = nullptr;
    CachedProjectileMovement = nullptr;
    CachedRocketBaseTemplate = nullptr;

    for (UActorComponent* Component : Components)
    {
        if (!IsValid(Component))
        {
            continue;
        }

        const FName ComponentName = Component->GetFName();

        if (ComponentName == FName(TEXT("Sphere")))
        {
            CachedSphere = Cast<USphereComponent>(Component);
        }
        else if (ComponentName == FName(TEXT("ProjectileMovement")))
        {
            CachedProjectileMovement = Cast<UProjectileMovementComponent>(Component);
        }
        else if (ComponentName == FName(TEXT("RocketBase")))
        {
            if (UParticleSystemComponent* RocketBase = Cast<UParticleSystemComponent>(Component))
            {
                CachedRocketBaseTemplate = RocketBase->Template;
            }
        }
    }

    if (!IsValid(CachedProjectileMovement))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("%s: ProjectileMovement component was not found. The authored movement component should be retained in BP_Rocket."),
            *GetName());
    }

    if (!IsValid(CachedRocketBaseTemplate))
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("%s: RocketBase particle component/template was not found; continuing without changing the visual component."),
            *GetName());
    }
}

void ARocket::HandleSphereBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    if (bImpactHandled || !IsValid(OverlappedComponent))
    {
        return;
    }

    // A player-fired rocket should ignore its owner and seek enemy turrets.
    if (OtherActor == GetOwner() || OtherActor == GetInstigator())
    {
        return;
    }
    HandleImpact(OtherActor);
}

void ARocket::SetHomingTargetComponent(USceneComponent* TargetComponent, float HomingAcceleration)
{
    if (!IsValid(CachedProjectileMovement))
    {
        TArray<UActorComponent*> Components;
        GetComponents<UActorComponent>(Components);
        for (UActorComponent* Component : Components)
        {
            if (Component && Component->GetFName() == FName(TEXT("ProjectileMovement")))
            {
                CachedProjectileMovement = Cast<UProjectileMovementComponent>(Component);
                break;
            }
        }
    }
    if (CachedProjectileMovement && IsValid(TargetComponent))
    {
        CachedProjectileMovement->HomingTargetComponent = TargetComponent;
        CachedProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
        CachedProjectileMovement->bIsHomingProjectile = true;
        CachedProjectileMovement->Activate(true);
    }
}

void ARocket::HandleImpact(AActor* OverlapActor)
{
    if (bImpactHandled)
    {
        return;
    }

    bImpactHandled = true;

    if (!GetWorld())
    {
        Destroy();
        return;
    }

    const FVector TraceStart = GetActorLocation();
    const FVector TraceEnd = TraceStart + FVector(0.0f, 0.0f, 1.0f);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RocketImpactSphereTrace), false, this);
    QueryParams.bTraceComplex = false;
    QueryParams.AddIgnoredActor(this);
    if (GetOwner()) QueryParams.AddIgnoredActor(GetOwner());
    if (GetInstigator()) QueryParams.AddIgnoredActor(GetInstigator());

    AActor* DamageTarget = nullptr;
    if (Cast<ATank>(GetOwner()) || Cast<ATank>(GetInstigator()))
    {
        // Player-fired rockets target enemy turret actors rather than the Player trace channel.
        FCollisionObjectQueryParams ObjectParams;
        ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
        ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
        ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
        TArray<FHitResult> Hits;
        GetWorld()->SweepMultiByObjectType(Hits, TraceStart, TraceEnd, FQuat::Identity,
            ObjectParams, FCollisionShape::MakeSphere(TraceRadius), QueryParams);
        float BestDistance = TNumericLimits<float>::Max();
        for (const FHitResult& Hit : Hits)
        {
            ABaseEnemyTurret* Enemy = Cast<ABaseEnemyTurret>(Hit.GetActor());
            if (IsValid(Enemy))
            {
                const float Distance = FVector::DistSquared(TraceStart, Hit.ImpactPoint);
                if (Distance < BestDistance) { BestDistance = Distance; DamageTarget = Enemy; }
            }
        }
        if (!DamageTarget && Cast<ABaseEnemyTurret>(OverlapActor)) DamageTarget = OverlapActor;
    }
    else
    {
        // Enemy-fired rockets retain the original Blueprint Player trace.
        const ECollisionChannel TraceChannel = UEngineTypes::ConvertToCollisionChannel(ETraceTypeQuery::TraceTypeQuery3);
        FHitResult Hit;
        const bool bHit = GetWorld()->SweepSingleByChannel(Hit, TraceStart, TraceEnd,
            FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(TraceRadius), QueryParams);
        if (bHit) DamageTarget = Hit.GetActor();
    }

    if (IsValid(DamageTarget))
    {
        // Keep the original rocket damage against the player, but deal 50 damage to enemies.
        const float DamageToApply = Cast<ABaseEnemyTurret>(DamageTarget) ? 50.0f : ImpactDamage;
        UGameplayStatics::ApplyDamage(DamageTarget, DamageToApply, GetInstigatorController(), this, nullptr);
    }

    // The original graph always spawns the explosion and destroys the rocket
    // after the trace/damage sequence, including when the trace misses.
    if (ExplosionEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            ExplosionEffect,
            GetActorLocation(),
            FRotator::ZeroRotator,
            FVector::OneVector,
            true
        );
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("%s: PS_RocketExplosion asset could not be loaded."), *GetName());
    }

    Destroy();
}
