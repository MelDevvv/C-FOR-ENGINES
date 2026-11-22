#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseEnemyTurret.generated.h"

class ATank;
class AController;
class UActorComponent;
class USceneComponent;
class UParticleSystem;
class UParticleSystemComponent;
class UArrowComponent;
class UHealthComponent;
class USphereComponent;

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
    // cached blueprint scs components names are intentionally different from
    // the component variables in bpbaseenemyturret to avoid property clashes
    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> CachedTurretSpin;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> CachedDangerZone;

    UPROPERTY(VisibleAnywhere, Category = "Enemy|AI|Detection Zone")
    TObjectPtr<USphereComponent> DetectionZoneVisualization;

    // native health component used by the previously working enemy damage path
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Health", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UHealthComponent> NativeHealthComponent;

    UPROPERTY(Transient)
    TObjectPtr<UArrowComponent> CachedMachineGunArrow;

    UPROPERTY(Transient)
    TObjectPtr<UParticleSystem> MachineGunBeamEffect;

    UPROPERTY(Transient)
    TObjectPtr<UParticleSystem> MachineGunMissEffect;

    UPROPERTY(Transient)
    TObjectPtr<UParticleSystemComponent> CachedLightningCharge;

    UPROPERTY(Transient)
    TObjectPtr<UParticleSystemComponent> CachedLightningCharge1;

    UPROPERTY(Transient)
    TObjectPtr<UParticleSystem> LightningBlastEffect;

    UPROPERTY(Transient)
    TObjectPtr<UParticleSystem> LightningHitEffect;

    UPROPERTY(Transient)
    TObjectPtr<UArrowComponent> CachedRocketArrow0;

    UPROPERTY(Transient)
    TObjectPtr<UArrowComponent> CachedRocketArrow1;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> CachedRocketMesh0;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> CachedRocketMesh1;

    UPROPERTY(Transient)
    TSubclassOf<AActor> RocketProjectileClass;

    float RocketShakeElapsed = 0.0f;
    bool bRocketShakeActive = false;

    float LightningAttackElapsed = 0.0f;
    bool bLightningAttackActive = false;
    FVector LightningTurretBaseRelativeLocation = FVector::ZeroVector;
    FRotator LightningTurretBaseRelativeRotation = FRotator::ZeroRotator;
    bool bHasLightningTurretBaseTransform = false;

    float MachineGunShakeElapsed = 0.0f;
    bool bMachineGunShakeActive = false;
    FVector MachineGunTurretSpinBaseRelativeLocation = FVector::ZeroVector;
    bool bHasMachineGunTurretSpinBaseLocation = false;

    UPROPERTY(Transient)
    TObjectPtr<AActor> CachedTank;

    FTimerHandle FireTimerHandle;


    // shared movement ai inherited by all enemy child types
    FVector EnemyPatrolOrigin = FVector::ZeroVector;
    FVector EnemyPatrolTarget = FVector::ZeroVector;
    FVector EnemyDodgeTarget = FVector::ZeroVector;
    FVector EnemyCombatMoveTarget = FVector::ZeroVector;
    FVector EnemyLastKnownPlayerLocation = FVector::ZeroVector;
    float EnemyLostPlayerSearchRemaining = 0.0f;
    bool bEnemyHasLastKnownPlayerLocation = false;
    TArray<FVector> EnemyCachedPathPoints;
    int32 EnemyCachedPathIndex = 0;
    float EnemyPathRebuildRemaining = 0.0f;
    float EnemyCombatMoveRemaining = 0.0f;
    bool bEnemyCombatMoveTargetValid = false;
    float EnemyPatrolWaitRemaining = 0.0f;
    float EnemyDodgeRemaining = 0.0f;
    float EnemyDodgeCooldownRemaining = 0.0f;
    float EnemyProjectileScanRemaining = 0.0f;
    bool bEnemyDodgeActive = false;
    bool bEnemyPatrolTargetValid = false;
    bool bEnemyPatrolOriginSet = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|AI|Detection Zone", meta = (AllowPrivateAccess = "true", ClampMin = "100.0", Units = "cm"))
    float EnemyDetectionRadius = 800.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|AI|Movement", meta = (ClampMin = "100.0", Units = "cm"))
    float EnemyPatrolRadius = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|AI|Movement", meta = (ClampMin = "100.0", Units = "cm"))
    float EnemyDodgeDistance = 450.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|AI|Movement", meta = (ClampMin = "0.1"))
    float EnemyDodgeDuration = 0.4f;

    UPROPERTY(EditAnywhere, Category = "Enemy|AI|Movement", meta = (ClampMin = "0.1"))
    float EnemyDodgeCooldown = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|AI|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
    float EnemyPatrolSpeed = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|AI|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
    float EnemyChaseSpeed = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|AI|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
    float EnemyDodgeSpeed = 900.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|AI|Movement", meta = (ClampMin = "0.0"))
    float EnemyPatrolWaitMin = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|AI|Movement", meta = (ClampMin = "0.0"))
    float EnemyPatrolWaitMax = 4.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy|AI|Search", meta = (ClampMin = "0.0", Units = "s"))
    float EnemySearchDuration = 6.0f;

    void CacheBlueprintComponents();
    void InitialiseNativeHealthComponent();
    void FindPlayerTank();
    void UpdateTurretAim();
    void UpdateEnemyMovementAI(float DeltaTime);
    bool BuildEnemyNavigationPath(const FVector& Destination);
    bool MoveEnemyAlongNavigationPath(float Speed, float DeltaTime);
    void ClearEnemyNavigationPath();
    void PickEnemyPatrolTarget();
    bool TryEnemyDodgeIncomingProjectile();
    void FireCheck();
    void ExecuteChildTurretFire();
    void UpdateNativeMachineGunShake(float DeltaTime);
    void ExecuteNativeMachineGunFire();
    void ExecuteNativeLightningFire();
    void UpdateNativeLightningAttack(float DeltaTime);
    void CacheRocketComponents();
    void ExecuteNativeRocketFire();
    void UpdateNativeRocketShake(float DeltaTime);

    bool ReadBlueprintBool(FName VariableName, bool DefaultValue) const;
    void WriteBlueprintBool(FName VariableName, bool Value);

    float ReadBlueprintFloat(FName VariableName, float DefaultValue) const;
    void WriteBlueprintFloat(FName VariableName, float Value);
    UObject* ReadBlueprintObject(FName VariableName) const;
    void WriteBlueprintObject(FName VariableName, UObject* Value);

    UFUNCTION()
    void HandleHealthDeath();

    // legacy bpchealthcomponentedeath signature controller actor
    UFUNCTION()
    void HandleHealthDeathWithContext(AController* InstigatedBy, AActor* DamageCauser);
};
