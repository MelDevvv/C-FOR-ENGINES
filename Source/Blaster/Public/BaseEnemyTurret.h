#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseEnemyTurret.generated.h"

class ATank;
class AController;
class UActorComponent;
class USceneComponent;
class UParticleSystem;

/**
 * Shared native parent for BP_BaseEnemyTurret and its weapon-specific children.
 * Existing Blueprint variables (FireRate, Range, MaxHP, Health, TankREF) are
 * deliberately accessed by reflection so they do not collide with existing
 * Blueprint-generated properties when BP_BaseEnemyTurret is reparented.
 */
UCLASS()
class BLASTER_API ABaseEnemyTurret : public AActor
{
    GENERATED_BODY()

public:
    ABaseEnemyTurret();

    virtual void Tick(float DeltaTime) override;
    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Effects")
    TObjectPtr<UParticleSystem> NativeDeathEffect;

private:
    // Cached Blueprint SCS components. Names are intentionally different from
    // the component variables in BP_BaseEnemyTurret to avoid property clashes.
    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> CachedTurretSpin;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> CachedDangerZone;

    // The supplied project uses BPC_HealthComponent (a Blueprint component
    // whose native parent is UActorComponent), not UHealthComponent.
    UPROPERTY(Transient)
    TObjectPtr<UActorComponent> CachedHealthActorComponent;

    UPROPERTY(Transient)
    TObjectPtr<ATank> CachedTank;

    FTimerHandle FireTimerHandle;

    void CacheBlueprintComponents();
    void InitialiseBlueprintHealthComponent();
    void FindPlayerTank();
    void UpdateTurretAim();
    void FireCheck();
    void ExecuteChildTurretFire();

    float ReadBlueprintFloat(FName VariableName, float DefaultValue) const;
    void WriteBlueprintFloat(FName VariableName, float Value);
    void WriteBlueprintObject(FName VariableName, UObject* Value);

    UFUNCTION()
    void HandleHealthDeath();

    // Legacy BPC_HealthComponent.EDeath signature: (Controller, Actor).
    UFUNCTION()
    void HandleHealthDeathWithContext(AController* InstigatedBy, AActor* DamageCauser);
};
