#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rocket.generated.h"

class USphereComponent;
class UPrimitiveComponent;
class UProjectileMovementComponent;
class UParticleSystem;

/**
 * Native gameplay implementation for the BP_Rocket projectile.
 *
 * BP_Rocket should retain its existing Blueprint SCS components (Sphere,
 * RocketBase and ProjectileMovement) and be reparented to this class. The
 * native class intentionally discovers those components instead of creating
 * replacements, preserving their authored transforms and movement settings.
 */
UCLASS()
class BLASTER_API ARocket : public AActor
{
    GENERATED_BODY()

public:
    ARocket();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<USphereComponent> CachedSphere;

    UPROPERTY(Transient)
    TObjectPtr<UProjectileMovementComponent> CachedProjectileMovement;

    UPROPERTY(Transient)
    TObjectPtr<UParticleSystem> CachedRocketBaseTemplate;

    UPROPERTY(EditDefaultsOnly, Category = "Rocket|Impact")
    TObjectPtr<UParticleSystem> ExplosionEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Rocket|Impact", meta = (ClampMin = "0.0"))
    float ImpactDamage = 5.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Rocket|Impact", meta = (ClampMin = "0.0"))
    float TraceRadius = 50.0f;

    bool bImpactHandled = false;

    void CacheBlueprintComponents();

    UFUNCTION()
    void HandleSphereBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    void HandleImpact();
};
