#include "BaseEnemyTurret.h"

#include "Tank.h"
#include "HealthComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Particles/ParticleSystemComponent.h"

#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Actor.h"
#include "CollisionShape.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"
#include "UObject/FieldIterator.h"
#include "UObject/ScriptDelegates.h"

ABaseEnemyTurret::ABaseEnemyTurret()
{
    PrimaryActorTick.bCanEverTick = true;
    SetCanBeDamaged(true);
    NativeHealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("NativeHealthComponent"));
    DetectionZoneVisualization = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionZoneVisualization"));
    DetectionZoneVisualization->SetSphereRadius(EnemyDetectionRadius);
    DetectionZoneVisualization->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DetectionZoneVisualization->SetGenerateOverlapEvents(false);
    DetectionZoneVisualization->SetHiddenInGame(true);
    DetectionZoneVisualization->SetVisibility(true);

    // This is the effect referenced by the original BP_BaseEnemyTurret asset.
    static ConstructorHelpers::FObjectFinder<UParticleSystem> DeathEffectAsset(
        TEXT("/Game/Assets/Effects/PS_EnemyDeathExplosion.PS_EnemyDeathExplosion")
    );

    if (DeathEffectAsset.Succeeded())
    {
        NativeDeathEffect = DeathEffectAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UParticleSystem> MachineGunBeamAsset(
        TEXT("/Game/Assets/Effects/PS_LineTraceBeam.PS_LineTraceBeam"));
    if (MachineGunBeamAsset.Succeeded())
    {
        MachineGunBeamEffect = MachineGunBeamAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UParticleSystem> MachineGunMissAsset(
        TEXT("/Game/Assets/Effects/PS_RocketExplosion.PS_RocketExplosion"));
    if (MachineGunMissAsset.Succeeded())
    {
        MachineGunMissEffect = MachineGunMissAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UParticleSystem> LightningBlastAsset(
        TEXT("/Game/Assets/Effects/PS_LightingBlast.PS_LightingBlast"));
    if (LightningBlastAsset.Succeeded())
    {
        LightningBlastEffect = LightningBlastAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UParticleSystem> LightningHitAsset(
        TEXT("/Game/Assets/Effects/PS_LightningHit.PS_LightningHit"));
    if (LightningHitAsset.Succeeded())
    {
        LightningHitEffect = LightningHitAsset.Object;
    }

    static ConstructorHelpers::FClassFinder<AActor> RocketClassAsset(
        TEXT("/Game/Scripts/Gameplay/Weapons/BP_Rocket"));
    if (RocketClassAsset.Succeeded())
    {
        RocketProjectileClass = RocketClassAsset.Class;
    }
}

void ABaseEnemyTurret::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    CacheBlueprintComponents();
    if (DetectionZoneVisualization)
    {
        if (RootComponent && DetectionZoneVisualization->GetAttachParent() != RootComponent)
            DetectionZoneVisualization->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        DetectionZoneVisualization->SetSphereRadius(EnemyDetectionRadius);
        DetectionZoneVisualization->SetHiddenInGame(true);
        DetectionZoneVisualization->SetVisibility(true);
    }

    // Reproduce BP_BaseEnemyTurret's Construction Script:
    // Health = MaxHP and DangerZone world scale = (Range, Range, 50).
    const float MaxHP = ReadBlueprintFloat(FName(TEXT("MaxHP")), 50.0f);
    const float Range = ReadBlueprintFloat(FName(TEXT("Range")), 1500.0f);

    WriteBlueprintFloat(FName(TEXT("Health")), MaxHP);

    if (CachedDangerZone && Range > 0.0f)
    {
        CachedDangerZone->SetWorldScale3D(FVector(Range, Range, 50.0f));
    }
}

void ABaseEnemyTurret::BeginPlay()
{
    Super::BeginPlay();

    SetCanBeDamaged(true);
    CacheBlueprintComponents();
    InitialiseNativeHealthComponent();
    FindPlayerTank();
    EnemyPatrolOrigin = GetActorLocation();
    bEnemyPatrolOriginSet = true;
    PickEnemyPatrolTarget();
    EnemyPatrolWaitRemaining = 0.0f;
    BuildEnemyNavigationPath(EnemyPatrolTarget);

    // BP_BaseEnemyTurret originally waited a random 0.0-0.9 seconds,
    // then started a repeating timer using its FireRate Blueprint variable.
    const float FireRate = FMath::Max(
        0.05f,
        ReadBlueprintFloat(FName(TEXT("FireRate")), 1.0f)
    );
    const float InitialDelay = FMath::FRandRange(0.0f, 0.9f) + FireRate;

    GetWorldTimerManager().SetTimer(
        FireTimerHandle,
        this,
        &ABaseEnemyTurret::FireCheck,
        FireRate,
        true,
        InitialDelay
    );
}

void ABaseEnemyTurret::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!IsValid(CachedTank))
    {
        FindPlayerTank();
    }

    // Run recoil/timeline transforms first. RocketShake writes the TurretSpin
    // relative rotation each frame; if aim runs before it, that write can
    // overwrite the freshly calculated target rotation.
    UpdateNativeMachineGunShake(DeltaTime);
    UpdateNativeLightningAttack(DeltaTime);
    UpdateNativeRocketShake(DeltaTime);
    UpdateEnemyMovementAI(DeltaTime);

    // Aim last so the final transform for this frame always faces the player.
    // The child Arrow/Arrow1 launch points inherit this rotation, so rockets
    // use the same yaw direction as the turret.
    UpdateTurretAim();
}

void ABaseEnemyTurret::CacheBlueprintComponents()
{
    TArray<UActorComponent*> Components;
    GetComponents<UActorComponent>(Components);

    CachedTurretSpin = nullptr;
    CachedDangerZone = nullptr;

    for (UActorComponent* Component : Components)
    {
        if (!IsValid(Component))
        {
            continue;
        }

        const FString ComponentName = Component->GetName();
        const FString ComponentClassName = Component->GetClass()->GetName();

        if (Component->GetFName() == FName(TEXT("TurretSpin")) ||
            ComponentName.Contains(TEXT("TurretSpin")))
        {
            CachedTurretSpin = Cast<USceneComponent>(Component);
        }
        else if (Component->GetFName() == FName(TEXT("DangerZone")) ||
                 ComponentName.Contains(TEXT("DangerZone")))
        {
            CachedDangerZone = Cast<USceneComponent>(Component);
        }

    }

    if (!CachedTurretSpin)
    {
        UE_LOG(LogTemp, Error,
            TEXT("%s: Could not find the Blueprint component named TurretSpin."),
            *GetName());
    }

    if (!CachedDangerZone)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("%s: Could not find DangerZone; its construction scale was not updated."),
            *GetName());
    }

    // Preserve and locate the two authored charge particle components from
    // BP_Enemy_Lightning. They remain Blueprint-owned; C++ only drives params.
    TArray<UParticleSystemComponent*> ParticleComponents;
    GetComponents<UParticleSystemComponent>(ParticleComponents);
    for (UParticleSystemComponent* ParticleComponent : ParticleComponents)
    {
        if (!IsValid(ParticleComponent))
        {
            continue;
        }
        const FName ComponentName = ParticleComponent->GetFName();
        if (ComponentName == FName(TEXT("LightingCharge")))
        {
            CachedLightningCharge = ParticleComponent;
        }
        else if (ComponentName == FName(TEXT("LightingCharge1")))
        {
            CachedLightningCharge1 = ParticleComponent;
        }
    }

    // The native component is created in the C++ constructor. The legacy
    // Blueprint component is optional and is no longer required for damage.
}

void ABaseEnemyTurret::InitialiseNativeHealthComponent()
{
    if (!IsValid(NativeHealthComponent))
    {
        UE_LOG(LogTemp, Error, TEXT("%s: NativeHealthComponent is missing."), *GetName());
        return;
    }

    // Keep the existing BP_BaseEnemyTurret MaxHP value as the tuning source,
    // but all damage/death behaviour now lives in UHealthComponent.
    const float ConfiguredMaxHP = FMath::Max(1.0f, ReadBlueprintFloat(FName(TEXT("MaxHP")), 50.0f));
    NativeHealthComponent->MaxHealth = ConfiguredMaxHP;
    NativeHealthComponent->ResetHealth();
    // EDeath is the canonical original Blueprint event. OnDeath remains
    // available for existing native consumers, but binding both here would
    // spawn the enemy death effect twice.
    NativeHealthComponent->EDeath.AddUniqueDynamic(this, &ABaseEnemyTurret::HandleHealthDeathWithContext);

    UE_LOG(LogTemp, Log, TEXT("%s: Native health ready. MaxHealth=%.1f"), *GetName(), ConfiguredMaxHP);
}

void ABaseEnemyTurret::FindPlayerTank()
{
    AActor* FoundTarget = nullptr;

    // Prefer a valid TankREF assigned by the existing Blueprint/gameplay.
    if (UObject* ExistingReference = ReadBlueprintObject(FName(TEXT("TankREF"))))
    {
        FoundTarget = Cast<AActor>(ExistingReference);
    }

    // Resolve the possessed pawn (not just a Character) so targeting works
    // regardless of the exact player pawn subclass.
    if (!IsValid(FoundTarget))
    {
        FoundTarget = UGameplayStatics::GetPlayerPawn(this, 0);
    }
    if (!IsValid(FoundTarget))
    {
        FoundTarget = UGameplayStatics::GetPlayerCharacter(this, 0);
    }

    // Fallback for a tank that exists in the level but is not possessed yet.
    if (!IsValid(FoundTarget) && GetWorld())
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATank::StaticClass(), FoundActors);
        for (AActor* Actor : FoundActors)
        {
            if (IsValid(Actor))
            {
                FoundTarget = Actor;
                break;
            }
        }
    }

    CachedTank = IsValid(FoundTarget) ? FoundTarget : nullptr;
    WriteBlueprintObject(FName(TEXT("TankREF")), CachedTank);

    UE_LOG(LogTemp, Log, TEXT("%s: turret target = %s"), *GetName(),
        IsValid(CachedTank) ? *CachedTank->GetName() : TEXT("<NOT FOUND>"));
}


void ABaseEnemyTurret::PickEnemyPatrolTarget()
{
    if (!bEnemyPatrolOriginSet)
    {
        EnemyPatrolOrigin = GetActorLocation();
        bEnemyPatrolOriginSet = true;
    }

    const FVector2D RandomDirection = FMath::RandPointInCircle(EnemyPatrolRadius);
    EnemyPatrolTarget = EnemyPatrolOrigin + FVector(RandomDirection.X, RandomDirection.Y, 0.0f);
    EnemyPatrolTarget.Z = GetActorLocation().Z;
    bEnemyPatrolTargetValid = true;
}


bool ABaseEnemyTurret::TryEnemyDodgeIncomingProjectile()
{
    if (!GetWorld() || EnemyDodgeCooldownRemaining > 0.0f)
    {
        return false;
    }

    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(TEXT("PlayerProjectile")), Actors);
    if (Actors.IsEmpty())
    {
        // Fallback: inspect actors with ProjectileMovement; avoids requiring a tag.
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), Actors);
    }

    const FVector EnemyLocation = GetActorLocation();
    for (AActor* ProjectileActor : Actors)
    {
        if (!IsValid(ProjectileActor) || ProjectileActor == this || ProjectileActor == CachedTank)
        {
            continue;
        }

        UProjectileMovementComponent* ProjectileMovement =
            ProjectileActor->FindComponentByClass<UProjectileMovementComponent>();
        if (!IsValid(ProjectileMovement) || ProjectileMovement->Velocity.IsNearlyZero())
        {
            continue;
        }

        const FVector Velocity = ProjectileMovement->Velocity.GetSafeNormal();
        const FVector ToEnemy = EnemyLocation - ProjectileActor->GetActorLocation();
        const float AlongTrajectory = FVector::DotProduct(ToEnemy, Velocity);
        if (AlongTrajectory < 0.0f || AlongTrajectory > 650.0f)
        {
            continue;
        }

        const FVector ClosestPoint = ProjectileActor->GetActorLocation() + Velocity * AlongTrajectory;
        if (FVector::DistSquared(ClosestPoint, EnemyLocation) > FMath::Square(160.0f))
        {
            continue;
        }

        FVector SideDirection = FVector::CrossProduct(Velocity, FVector::UpVector).GetSafeNormal();
        if (SideDirection.IsNearlyZero())
        {
            SideDirection = GetActorRightVector();
        }
        if (FMath::RandBool())
        {
            SideDirection *= -1.0f;
        }

        EnemyDodgeTarget = EnemyLocation + SideDirection * EnemyDodgeDistance;
        EnemyDodgeTarget.Z = EnemyLocation.Z;
        bEnemyDodgeActive = true;
        EnemyDodgeRemaining = EnemyDodgeDuration;
        EnemyDodgeCooldownRemaining = EnemyDodgeCooldown;
        return true;
    }

    return false;
}


void ABaseEnemyTurret::ClearEnemyNavigationPath()
{
    EnemyCachedPathPoints.Reset();
    EnemyCachedPathIndex = 0;
    EnemyPathRebuildRemaining = 0.0f;
}


bool ABaseEnemyTurret::BuildEnemyNavigationPath(const FVector& Destination)
{
    if (!GetWorld())
    {
        ClearEnemyNavigationPath();
        return false;
    }

    UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
        GetWorld(), GetActorLocation(), Destination, this);

    if (!Path || !Path->IsValid() || Path->PathPoints.Num() < 2)
    {
        ClearEnemyNavigationPath();
        return false;
    }

    EnemyCachedPathPoints = Path->PathPoints;
    EnemyCachedPathIndex = 1; // Skip the path's starting point.
    EnemyPathRebuildRemaining = 0.35f;
    return true;
}


bool ABaseEnemyTurret::MoveEnemyAlongNavigationPath(float Speed, float DeltaTime)
{
    if (EnemyCachedPathPoints.Num() < 2 || EnemyCachedPathIndex >= EnemyCachedPathPoints.Num())
    {
        return false;
    }

    FVector Waypoint = EnemyCachedPathPoints[EnemyCachedPathIndex];
    FVector ToWaypoint = Waypoint - GetActorLocation();
    ToWaypoint.Z = 0.0f;

    if (ToWaypoint.SizeSquared() < FMath::Square(45.0f))
    {
        ++EnemyCachedPathIndex;
        if (EnemyCachedPathIndex >= EnemyCachedPathPoints.Num())
        {
            ClearEnemyNavigationPath();
            return false;
        }
        Waypoint = EnemyCachedPathPoints[EnemyCachedPathIndex];
        ToWaypoint = Waypoint - GetActorLocation();
        ToWaypoint.Z = 0.0f;
    }

    if (ToWaypoint.IsNearlyZero())
    {
        return true;
    }

    const FVector Step = ToWaypoint.GetSafeNormal() * Speed * DeltaTime;
    FHitResult SweepHit;
    AddActorWorldOffset(Step, true, &SweepHit);
    if (SweepHit.IsValidBlockingHit())
    {
        ClearEnemyNavigationPath();
        return false;
    }
    return true;
}


void ABaseEnemyTurret::UpdateEnemyMovementAI(float DeltaTime)
{
    if (!GetWorld())
    {
        return;
    }

    EnemyDodgeCooldownRemaining = FMath::Max(0.0f, EnemyDodgeCooldownRemaining - DeltaTime);
    EnemyPatrolWaitRemaining = FMath::Max(0.0f, EnemyPatrolWaitRemaining - DeltaTime);
    EnemyCombatMoveRemaining = FMath::Max(0.0f, EnemyCombatMoveRemaining - DeltaTime);
    EnemyPathRebuildRemaining = FMath::Max(0.0f, EnemyPathRebuildRemaining - DeltaTime);
    EnemyProjectileScanRemaining -= DeltaTime;

    if (EnemyProjectileScanRemaining <= 0.0f)
    {
        EnemyProjectileScanRemaining = 0.25f;
        if (!bEnemyDodgeActive)
        {
            TryEnemyDodgeIncomingProjectile();
        }
    }

    if (bEnemyDodgeActive)
    {
        EnemyDodgeRemaining -= DeltaTime;
        if (EnemyDodgeRemaining <= 0.0f || FVector::DistSquared2D(GetActorLocation(), EnemyDodgeTarget) < FMath::Square(40.0f))
        {
            bEnemyDodgeActive = false;
            ClearEnemyNavigationPath();
        }
        else
        {
            if (EnemyCachedPathPoints.IsEmpty() || EnemyPathRebuildRemaining <= 0.0f)
            {
                BuildEnemyNavigationPath(EnemyDodgeTarget);
            }
            if (!MoveEnemyAlongNavigationPath(EnemyDodgeSpeed, DeltaTime))
            {
                // If navigation cannot reach the dodge point, try a short collision-swept sidestep.
                FVector Side = (EnemyDodgeTarget - GetActorLocation()).GetSafeNormal2D();
                if (!Side.IsNearlyZero())
                {
                    FHitResult Hit;
                    AddActorWorldOffset(Side * EnemyDodgeSpeed * DeltaTime, true, &Hit);
                    if (Hit.IsValidBlockingHit())
                    {
                        bEnemyDodgeActive = false;
                    }
                }
            }
            return;
        }
    }

    const float PlayerDistance = IsValid(CachedTank)
        ? FVector::Dist(GetActorLocation(), CachedTank->GetActorLocation())
        : TNumericLimits<float>::Max();
    const bool bPlayerDetected = IsValid(CachedTank) && PlayerDistance <= EnemyDetectionRadius;

    if (bPlayerDetected)
    {
        EnemyLastKnownPlayerLocation = CachedTank->GetActorLocation();
        bEnemyHasLastKnownPlayerLocation = true;
        EnemyLostPlayerSearchRemaining = EnemySearchDuration;

        // If the player is retreating beyond close combat distance, pursue their
        // live position. Do not let a random lateral combat target stall pursuit.
        if (PlayerDistance > 1100.0f)
        {
            EnemyPatrolWaitRemaining = 0.0f;
            bEnemyCombatMoveTargetValid = false;
            if (EnemyCachedPathPoints.IsEmpty() || EnemyPathRebuildRemaining <= 0.0f)
            {
                BuildEnemyNavigationPath(CachedTank->GetActorLocation());
            }
            if (!MoveEnemyAlongNavigationPath(EnemyChaseSpeed, DeltaTime))
            {
                // Navigation fallback: collision-swept movement toward the tank.
                FVector ChaseDirection = (CachedTank->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
                FHitResult ChaseHit;
                AddActorWorldOffset(ChaseDirection * EnemyChaseSpeed * DeltaTime, true, &ChaseHit);
                if (ChaseHit.IsValidBlockingHit())
                {
                    ClearEnemyNavigationPath();
                }
            }
            return;
        }

        // Close combat: keep moving laterally and reposition instead of stopping.
        EnemyPatrolWaitRemaining = 0.0f;
        if (!bEnemyCombatMoveTargetValid || EnemyCombatMoveRemaining <= 0.0f || EnemyCachedPathPoints.IsEmpty())
        {
            const FVector PlayerLocation = CachedTank->GetActorLocation();
            FVector AwayFromPlayer = (GetActorLocation() - PlayerLocation).GetSafeNormal2D();
            if (AwayFromPlayer.IsNearlyZero())
            {
                AwayFromPlayer = GetActorRightVector();
            }
            const FVector Side = FVector::CrossProduct(AwayFromPlayer, FVector::UpVector).GetSafeNormal();
            const float SideSign = FMath::RandBool() ? 1.0f : -1.0f;
            const float DesiredDistance = FMath::Clamp(PlayerDistance, 650.0f, 1100.0f);
            EnemyCombatMoveTarget = PlayerLocation + AwayFromPlayer * DesiredDistance + Side * SideSign * FMath::RandRange(350.0f, 650.0f);
            EnemyCombatMoveTarget.Z = GetActorLocation().Z;
            bEnemyCombatMoveTargetValid = BuildEnemyNavigationPath(EnemyCombatMoveTarget);
            EnemyCombatMoveRemaining = FMath::RandRange(1.2f, 2.4f);
        }
        if (bEnemyCombatMoveTargetValid && !MoveEnemyAlongNavigationPath(EnemyPatrolSpeed * 1.35f, DeltaTime))
        {
            bEnemyCombatMoveTargetValid = false;
            ClearEnemyNavigationPath();
        }
        return;
    }

    // If the player escaped the detection radius, pursue the last seen position
    // for a short search period, then sweep nearby points before resuming patrol.
    if (bEnemyHasLastKnownPlayerLocation && EnemyLostPlayerSearchRemaining > 0.0f)
    {
        EnemyLostPlayerSearchRemaining = FMath::Max(0.0f, EnemyLostPlayerSearchRemaining - DeltaTime);
        bEnemyCombatMoveTargetValid = false;
        if (EnemyCachedPathPoints.IsEmpty() || EnemyPathRebuildRemaining <= 0.0f)
        {
            BuildEnemyNavigationPath(EnemyLastKnownPlayerLocation);
        }
        if (!MoveEnemyAlongNavigationPath(EnemyChaseSpeed, DeltaTime))
        {
            // Reached or couldn't reach last known position; search locally.
            EnemyPatrolOrigin = EnemyLastKnownPlayerLocation;
            bEnemyPatrolOriginSet = true;
            bEnemyPatrolTargetValid = false;
            EnemyLostPlayerSearchRemaining = 0.0f;
            ClearEnemyNavigationPath();
        }
        return;
    }
    bEnemyHasLastKnownPlayerLocation = false;

    bEnemyCombatMoveTargetValid = false;
    if (EnemyPatrolWaitRemaining > 0.0f)
    {
        return;
    }

    if (!bEnemyPatrolTargetValid || FVector::DistSquared2D(GetActorLocation(), EnemyPatrolTarget) < FMath::Square(100.0f))
    {
        EnemyPatrolWaitRemaining = FMath::RandRange(EnemyPatrolWaitMin, EnemyPatrolWaitMax);
        PickEnemyPatrolTarget();
        ClearEnemyNavigationPath();
        return;
    }

    if (EnemyCachedPathPoints.IsEmpty() || EnemyPathRebuildRemaining <= 0.0f)
    {
        BuildEnemyNavigationPath(EnemyPatrolTarget);
    }

    if (!MoveEnemyAlongNavigationPath(EnemyPatrolSpeed, DeltaTime))
    {
        // Fallback patrol movement so AI still starts moving if a nav query fails.
        FVector PatrolDirection = (EnemyPatrolTarget - GetActorLocation()).GetSafeNormal2D();
        FHitResult PatrolHit;
        AddActorWorldOffset(PatrolDirection * EnemyPatrolSpeed * DeltaTime, true, &PatrolHit);
        if (PatrolHit.IsValidBlockingHit() || PatrolDirection.IsNearlyZero())
        {
            EnemyPatrolWaitRemaining = FMath::RandRange(EnemyPatrolWaitMin, EnemyPatrolWaitMax);
            bEnemyPatrolTargetValid = false;
            ClearEnemyNavigationPath();
        }
    }
}


void ABaseEnemyTurret::UpdateTurretAim()
{
    if (!IsValid(CachedTank))
    {
        FindPlayerTank();
    }
    if (!IsValid(CachedTurretSpin))
    {
        CacheBlueprintComponents();
    }
    if (!IsValid(CachedTank) || !IsValid(CachedTurretSpin))
    {
        return;
    }

    const FVector TurretLocation = CachedTurretSpin->GetComponentLocation();
    FVector TargetLocation = CachedTank->GetActorLocation();
    TargetLocation.Z = TurretLocation.Z;
    const FVector ToTarget = TargetLocation - TurretLocation;
    if (ToTarget.IsNearlyZero())
    {
        return;
    }

    const FRotator CurrentWorldRotation = CachedTurretSpin->GetComponentRotation();
    const FRotator DesiredWorldRotation(0.0f, ToTarget.Rotation().Yaw, 0.0f);
    const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
    const FRotator SmoothedRotation = FMath::RInterpTo(
        CurrentWorldRotation, DesiredWorldRotation, DeltaSeconds, 4.0f);

    // Always apply the aim after recoil updates in Tick. The launch arrows are
    // children of the rotating turret assembly and inherit this world yaw.
    CachedTurretSpin->SetWorldRotation(SmoothedRotation);
}

void ABaseEnemyTurret::FireCheck()
{
    if (!IsValid(CachedTank))
    {
        FindPlayerTank();
    }

    if (!IsValid(CachedTank) || !GetWorld())
    {
        return;
    }

    // Separate awareness radius from weapon range: enemies only start firing
    // when the player is within the shorter AI detection radius.
    if (FVector::DistSquared(GetActorLocation(), CachedTank->GetActorLocation()) >
        FMath::Square(EnemyDetectionRadius))
    {
        return;
    }

    // Reproduce the base Blueprint's Multi Sphere Trace By Channel:
    // Start = ActorLocation, End = ActorLocation + (0,0,1),
    // Radius = Range, Trace Channel = custom Player channel, Ignore Self.
    const float Range = FMath::Max(
        0.0f,
        ReadBlueprintFloat(FName(TEXT("Range")), 1500.0f)
    );

    if (Range <= 0.0f)
    {
        return;
    }

    const FVector Start = GetActorLocation();
    const FVector End = Start + FVector(0.0f, 0.0f, 1.0f);

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(BaseEnemyTurretPlayerDetection),
        false,
        this
    );
    QueryParams.AddIgnoredActor(this);

    TArray<FHitResult> Hits;

    // ECC_GameTraceChannel1 is named "Player" in this project's DefaultEngine.ini.
    const bool bAnyHit = GetWorld()->SweepMultiByChannel(
        Hits,
        Start,
        End,
        FQuat::Identity,
        ECC_GameTraceChannel1,
        FCollisionShape::MakeSphere(Range),
        QueryParams
    );

    if (!bAnyHit)
    {
        return;
    }

    // Only fire the child weapon when the detected actor is the player tank.
    const bool bPlayerTankHit = Hits.ContainsByPredicate(
        [this](const FHitResult& Hit)
        {
            return Hit.GetActor() == CachedTank;
        }
    );

    if (bPlayerTankHit)
    {
        ExecuteChildTurretFire();
    }
}

void ABaseEnemyTurret::UpdateNativeMachineGunShake(float DeltaTime)
{
    if (!GetClass()->GetName().Contains(TEXT("Enemy_MachineGun")) ||
        !IsValid(CachedTurretSpin))
    {
        return;
    }

    if (!bHasMachineGunTurretSpinBaseLocation)
    {
        MachineGunTurretSpinBaseRelativeLocation =
            CachedTurretSpin->GetRelativeLocation();
        bHasMachineGunTurretSpinBaseLocation = true;
    }

    if (!bMachineGunShakeActive)
    {
        return;
    }

    MachineGunShakeElapsed = FMath::Min(MachineGunShakeElapsed + DeltaTime, 5.0f);
    const float Alpha = FMath::Clamp(MachineGunShakeElapsed / 5.0f, 0.0f, 1.0f);

    // The exported GunShake track is 0 -> -2 -> 0. Apply the motion to the
    // authored pivot location instead of replacing its component transform.
    const float TurretMovement = -2.0f * FMath::Sin(PI * Alpha);
    // Keep the authored pivot centred. The old conversion added a constant
    // +7 units on Y, which displaced the turret throughout repeated firing.
    const FVector ShakeOffset(0.0f, 0.0f, TurretMovement);

    CachedTurretSpin->SetRelativeLocation(
        MachineGunTurretSpinBaseRelativeLocation + ShakeOffset,
        false,
        nullptr,
        ETeleportType::None
    );

    if (Alpha >= 1.0f)
    {
        bMachineGunShakeActive = false;
        CachedTurretSpin->SetRelativeLocation(
            MachineGunTurretSpinBaseRelativeLocation,
            false,
            nullptr,
            ETeleportType::None
        );
    }
}

void ABaseEnemyTurret::ExecuteNativeMachineGunFire()
{
    if (!IsValid(CachedMachineGunArrow))
    {
        TArray<UArrowComponent*> Arrows;
        GetComponents<UArrowComponent>(Arrows);
        for (UArrowComponent* Arrow : Arrows)
        {
            if (IsValid(Arrow) &&
                (Arrow->GetFName() == FName(TEXT("Arrow")) ||
                 Arrow->GetName().Contains(TEXT("Arrow"))))
            {
                CachedMachineGunArrow = Arrow;
                break;
            }
        }
    }

    if (!IsValid(CachedMachineGunArrow) || !GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Machine gun Arrow component not found."), *GetName());
        return;
    }

    const FVector Start = CachedMachineGunArrow->GetComponentLocation();
    const FVector End = Start + CachedMachineGunArrow->GetForwardVector() * 500.0f;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(NativeMachineGunFire), false, this);
    Params.AddIgnoredActor(this);

    const ECollisionChannel Channel =
        UEngineTypes::ConvertToCollisionChannel(ETraceTypeQuery::TraceTypeQuery1);

    FHitResult Hit;

    // A small sphere sweep is more forgiving than a zero-width line and helps
    // the trace catch the tank's central collision body.
    constexpr float TraceRadius = 18.0f;
    const bool bHit = GetWorld()->SweepSingleByChannel(
        Hit,
        Start,
        End,
        FQuat::Identity,
        Channel,
        FCollisionShape::MakeSphere(TraceRadius),
        Params
    );

    const FVector BeamEnd = bHit ? Hit.ImpactPoint : End;

    if (bHit && Hit.GetActor())
    {
        UGameplayStatics::ApplyDamage(
            Hit.GetActor(), 1.0f, GetInstigatorController(), this, nullptr
        );

        // Play the impact burst at the actual contact point, not at the
        // far endpoint of the trace. Orient it along the surface normal.
        if (MachineGunMissEffect)
        {
            const FRotator ImpactRotation =
                Hit.ImpactNormal.IsNearlyZero()
                    ? FRotator::ZeroRotator
                    : Hit.ImpactNormal.Rotation();

            UGameplayStatics::SpawnEmitterAtLocation(
                GetWorld(),
                MachineGunMissEffect,
                Hit.ImpactPoint,
                ImpactRotation,
                FVector(0.25f),
                true
            );
        }
    }

    if (MachineGunBeamEffect)
    {
        if (UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(), MachineGunBeamEffect, Start, FRotator::ZeroRotator,
            FVector::OneVector, true))
        {
            Beam->SetBeamTargetPoint(0, BeamEnd, 0);
        }
    }

    if (!bHit && MachineGunMissEffect)
    {
        // Preserve the original end-of-trace effect for a genuine miss.
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(), MachineGunMissEffect, End, FRotator::ZeroRotator,
            FVector(0.25f), true
        );
    }

    MachineGunShakeElapsed = 0.0f;
    bMachineGunShakeActive = true;
}

void ABaseEnemyTurret::UpdateNativeLightningAttack(float DeltaTime)
{
    if (!bLightningAttackActive || !GetClass()->GetName().Contains(TEXT("Enemy_Lightning")))
    {
        return;
    }

    LightningAttackElapsed = FMath::Min(LightningAttackElapsed + DeltaTime, 5.0f);
    const float Alpha = FMath::Clamp(LightningAttackElapsed / 5.0f, 0.0f, 1.0f);
    const float Buildup = FMath::Sin(PI * Alpha);
    const float TurretMovement = -2.0f * Buildup;

    // The exported Blueprint timeline drives LT1/LT2 from a 0..1 buildup
    // track (multipliers 40 and 300) and offsets the turret pivot while firing.
    if (IsValid(CachedLightningCharge))
    {
        CachedLightningCharge->SetFloatParameter(FName(TEXT("LT1")), Buildup * 40.0f);
        CachedLightningCharge->SetFloatParameter(FName(TEXT("LT2")), Buildup * 300.0f);
    }
    if (IsValid(CachedLightningCharge1))
    {
        CachedLightningCharge1->SetFloatParameter(FName(TEXT("LT1")), Buildup * 40.0f);
        CachedLightningCharge1->SetFloatParameter(FName(TEXT("LT2")), Buildup * 300.0f);
    }

    if (IsValid(CachedTurretSpin))
    {
        // Match the original BP_Enemy_Lightning graph exactly: the
        // SetRelativeLocationAndRotation node writes X=0, Y=0 and
        // Z=(TurretMovement * 5) + 7. It does NOT add that offset to the
        // component's existing relative location. Adding it to the authored
        // transform caused the whole turret to sit too high above its base.
        const FVector OriginalBlueprintRelativeLocation(
            0.0f, 0.0f, (TurretMovement * 5.0f) + 7.0f);

        const FRotator ExistingRelativeRotation = CachedTurretSpin->GetRelativeRotation();
        FRotator OriginalBlueprintRelativeRotation(
            0.0f,
            0.0f - TurretMovement,
            ExistingRelativeRotation.Yaw);

        CachedTurretSpin->SetRelativeLocationAndRotation(
            OriginalBlueprintRelativeLocation, OriginalBlueprintRelativeRotation);
    }

    if (Alpha >= 1.0f)
    {
        bLightningAttackActive = false;
        if (IsValid(CachedLightningCharge))
        {
            CachedLightningCharge->SetFloatParameter(FName(TEXT("LT1")), 0.0f);
            CachedLightningCharge->SetFloatParameter(FName(TEXT("LT2")), 0.0f);
        }
        if (IsValid(CachedLightningCharge1))
        {
            CachedLightningCharge1->SetFloatParameter(FName(TEXT("LT1")), 0.0f);
            CachedLightningCharge1->SetFloatParameter(FName(TEXT("LT2")), 0.0f);
        }
    }
}

void ABaseEnemyTurret::ExecuteNativeLightningFire()
{
    if (!GetWorld())
    {
        return;
    }

    // Match the authored Blueprint: blast at the turret, sphere trace for
    // Pawn objects using Range, then apply 5 damage and attach the hit burst.
    if (LightningBlastEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(), LightningBlastEffect, GetActorLocation(), FRotator::ZeroRotator,
            FVector::OneVector, true);
    }

    const float Radius = FMath::Max(0.0f, ReadBlueprintFloat(FName(TEXT("Range")), 500.0f));
    if (Radius > 0.0f)
    {
        FCollisionObjectQueryParams ObjectQuery;
        ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NativeLightningSphereTrace), false, this);
        QueryParams.AddIgnoredActor(this);

        TArray<FHitResult> Hits;
        const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
        const FVector TraceStart = GetActorLocation();
        const FVector TraceEnd = TraceStart + FVector(0.0f, 0.0f, 1.0f);
        GetWorld()->SweepMultiByObjectType(Hits, TraceStart, TraceEnd, FQuat::Identity,
            ObjectQuery, Sphere, QueryParams);

        TSet<AActor*> DamagedActors;
        for (const FHitResult& Hit : Hits)
        {
            AActor* HitActor = Hit.GetActor();
            if (!IsValid(HitActor) || HitActor == this || DamagedActors.Contains(HitActor))
            {
                continue;
            }
            DamagedActors.Add(HitActor);
            UGameplayStatics::ApplyDamage(HitActor, 5.0f, GetInstigatorController(), this, nullptr);

            if (LightningHitEffect && IsValid(Hit.GetComponent()))
            {
                UGameplayStatics::SpawnEmitterAttached(
                    LightningHitEffect, Hit.GetComponent(), NAME_None, FVector::ZeroVector,
                    FRotator::ZeroRotator, FVector::OneVector,
                    EAttachLocation::SnapToTarget, true);
            }
        }
    }

    LightningAttackElapsed = 0.0f;
    bLightningAttackActive = true;
}


void ABaseEnemyTurret::CacheRocketComponents()
{
    if (!IsValid(CachedRocketArrow0) || !IsValid(CachedRocketArrow1) ||
        !IsValid(CachedRocketMesh0) || !IsValid(CachedRocketMesh1))
    {
        TArray<UArrowComponent*> Arrows;
        GetComponents<UArrowComponent>(Arrows);
        for (UArrowComponent* Arrow : Arrows)
        {
            if (!IsValid(Arrow)) continue;
            if (Arrow->GetFName() == FName(TEXT("Arrow"))) CachedRocketArrow0 = Arrow;
            else if (Arrow->GetFName() == FName(TEXT("Arrow1"))) CachedRocketArrow1 = Arrow;
        }

        TArray<USceneComponent*> SceneComponents;
        GetComponents<USceneComponent>(SceneComponents);
        for (USceneComponent* Component : SceneComponents)
        {
            if (!IsValid(Component)) continue;
            if (Component->GetFName() == FName(TEXT("StaticMesh_0"))) CachedRocketMesh0 = Component;
            else if (Component->GetFName() == FName(TEXT("StaticMesh1"))) CachedRocketMesh1 = Component;
        }
    }
}

void ABaseEnemyTurret::ExecuteNativeRocketFire()
{
    if (!GetWorld())
    {
        return;
    }

    CacheRocketComponents();

    const bool bUseArrow0 = ReadBlueprintBool(FName(TEXT("Point")), false);
    UArrowComponent* LaunchArrow = bUseArrow0 ? CachedRocketArrow0 : CachedRocketArrow1;

    if (!IsValid(LaunchArrow))
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Rocket launch arrow component not found (Arrow/Arrow1)."), *GetName());
        return;
    }

    if (!RocketProjectileClass)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Could not load /Game/Scripts/Gameplay/Weapons/BP_Rocket."), *GetName());
        return;
    }

    // The original Blueprint alternates launch points using Point:
    // Point=false launches from Arrow1 and sets Point=true;
    // Point=true launches from Arrow and sets Point=false.
    WriteBlueprintBool(FName(TEXT("Point")), !bUseArrow0);

    FVector AimTarget = IsValid(CachedTank) ? CachedTank->GetActorLocation() : GetActorLocation();
    AimTarget.Z -= 20.0f;

    const FVector SpawnLocation = LaunchArrow->GetComponentLocation();
    const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(SpawnLocation, AimTarget);

    // The Blueprint takes Roll and Yaw from the Arrow's world rotation,
    // but takes Pitch from Find Look at Rotation.
    FRotator SpawnRotation = LaunchArrow->GetComponentRotation();
    SpawnRotation.Pitch = LookAtRotation.Pitch;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined;

    AActor* SpawnedRocket = GetWorld()->SpawnActor<AActor>(
        RocketProjectileClass,
        FTransform(SpawnRotation, SpawnLocation, FVector::OneVector),
        SpawnParams);

    // BP_Rocket owns the ProjectileMovement component. Enable Unreal's native
    // homing on the spawned instance so the projectile steers after launch,
    // while preserving the Blueprint's original speed, gravity and overlap logic.
    AActor* HomingTarget = IsValid(CachedTank) ? Cast<AActor>(CachedTank) : nullptr;
    if (IsValid(SpawnedRocket) && IsValid(HomingTarget))
    {
        UProjectileMovementComponent* ProjectileMovement =
            SpawnedRocket->FindComponentByClass<UProjectileMovementComponent>();
        USceneComponent* TargetComponent = HomingTarget->GetRootComponent();

        if (IsValid(ProjectileMovement) && IsValid(TargetComponent))
        {
            ProjectileMovement->HomingTargetComponent = TargetComponent;
            ProjectileMovement->HomingAccelerationMagnitude = 3500.0f;
            ProjectileMovement->bIsHomingProjectile = true;
            ProjectileMovement->Activate(true);

            UE_LOG(LogTemp, Verbose, TEXT("%s: Rocket %s homing towards %s."),
                *GetName(), *SpawnedRocket->GetName(), *HomingTarget->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("%s: Spawned BP_Rocket but could not configure homing (ProjectileMovement=%s, TargetComponent=%s)."),
                *GetName(), IsValid(ProjectileMovement) ? TEXT("valid") : TEXT("missing"),
                IsValid(TargetComponent) ? TEXT("valid") : TEXT("missing"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Rocket spawned without homing because no valid TankREF target was available."), *GetName());
    }

    // The child Blueprint starts the two authored timelines after spawning.
    // The toggled Point selects which of the two rocket-rig meshes receives
    // its short recoil animation.
    RocketShakeElapsed = 0.0f;
    bRocketShakeActive = true;
}

void ABaseEnemyTurret::UpdateNativeRocketShake(float DeltaTime)
{
    if (!bRocketShakeActive || !GetClass()->GetName().Contains(TEXT("Enemy_Rocket")))
    {
        return;
    }

    CacheRocketComponents();

    RocketShakeElapsed = FMath::Min(RocketShakeElapsed + DeltaTime, 5.0f);
    const float Alpha = FMath::Clamp(RocketShakeElapsed / 5.0f, 0.0f, 1.0f);

    // Exported RocketShake/GunShake timelines are 5 seconds long. The visible
    // track has a 0 -> -2 -> 0 recoil shape; use a smooth half-wave between
    // those endpoints to reproduce the authored movement without Blueprint timelines.
    const float TrackValue = -2.0f * FMath::Sin(PI * Alpha);

    if (IsValid(CachedTurretSpin))
    {
        const FVector OriginalGraphLocation(0.0f, 0.0f, TrackValue + 7.0f);
        const float ExistingYaw = CachedTurretSpin->GetRelativeRotation().Yaw;
        // FRotator constructor order is Pitch, Yaw, Roll.
        // The original Blueprint sets Roll=0, Pitch=(0 - TurretMovement),
        // and preserves RelativeRotation.Yaw. Do not put yaw in the Roll slot.
        const FRotator OriginalGraphRotation(-TrackValue, ExistingYaw, 0.0f);
        CachedTurretSpin->SetRelativeLocationAndRotation(
            OriginalGraphLocation, OriginalGraphRotation);
    }

    // After Point is toggled by Turret Fire, true drives StaticMesh_0
    // (RocketShake); false drives StaticMesh1 (Timeline_0).
    const bool bUseMesh0 = ReadBlueprintBool(FName(TEXT("Point")), false);
    USceneComponent* RecoilMesh = bUseMesh0 ? CachedRocketMesh0 : CachedRocketMesh1;
    if (IsValid(RecoilMesh))
    {
        RecoilMesh->SetRelativeLocation(FVector(0.0f, TrackValue, 0.0f));
    }

    if (Alpha >= 1.0f)
    {
        bRocketShakeActive = false;
        if (IsValid(CachedTurretSpin))
        {
            const float ExistingYaw = CachedTurretSpin->GetRelativeRotation().Yaw;
            // Reset pitch and roll while retaining yaw in FRotator's Yaw slot.
            CachedTurretSpin->SetRelativeLocationAndRotation(
                FVector(0.0f, 0.0f, 7.0f),
                FRotator(0.0f, ExistingYaw, 0.0f));
        }
        const bool bFinishOnMesh0 = ReadBlueprintBool(FName(TEXT("Point")), false);
        USceneComponent* FinishedMesh = bFinishOnMesh0 ? CachedRocketMesh0 : CachedRocketMesh1;
        if (IsValid(FinishedMesh))
        {
            FinishedMesh->SetRelativeLocation(FVector::ZeroVector);
        }
    }
}

void ABaseEnemyTurret::ExecuteChildTurretFire()
{
    if (GetClass()->GetName().Contains(TEXT("Enemy_MachineGun")))
    {
        ExecuteNativeMachineGunFire();
        return;
    }

    if (GetClass()->GetName().Contains(TEXT("Enemy_Lightning")))
    {
        ExecuteNativeLightningFire();
        return;
    }

    if (GetClass()->GetName().Contains(TEXT("Enemy_Rocket")))
    {
        ExecuteNativeRocketFire();
        return;
    }

    // Turret Fire is deliberately retained as a Blueprint function in
    // BP_BaseEnemyTurret. Each child Blueprint overrides it with its own
    // weapon (Machine Gun line trace, Lightning effects, or BP_Rocket spawn).
    // Reflection calls the most-derived Blueprint implementation.
    const FName CandidateFunctionNames[] =
    {
        FName(TEXT("TurretFire")),
        FName(TEXT("Turret Fire")),
        FName(TEXT("Turret_Fire"))
    };

    for (const FName FunctionName : CandidateFunctionNames)
    {
        if (UFunction* FireFunction = FindFunction(FunctionName))
        {
            ProcessEvent(FireFunction, nullptr);
            return;
        }
    }

    UE_LOG(LogTemp, Error,
        TEXT("%s: Could not find the Blueprint function 'Turret Fire'. Keep the empty function in BP_BaseEnemyTurret and the overrides in its children."),
        *GetName());
}

void ABaseEnemyTurret::HandleHealthDeath()
{
    GetWorldTimerManager().ClearTimer(FireTimerHandle);

    if (NativeDeathEffect && GetWorld())
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            NativeDeathEffect,
            GetActorTransform(),
            true
        );
    }

    Destroy();
}

void ABaseEnemyTurret::HandleHealthDeathWithContext(
    AController* InstigatedBy,
    AActor* DamageCauser
)
{
    // The original Blueprint TurretDestroyed event receives these two values.
    // Its current logic only plays the death effect and destroys this actor.
    HandleHealthDeath();
}


bool ABaseEnemyTurret::ReadBlueprintBool(FName VariableName, bool DefaultValue) const
{
    if (const FBoolProperty* Property = FindFProperty<FBoolProperty>(GetClass(), VariableName))
    {
        return Property->GetPropertyValue_InContainer(this);
    }
    return DefaultValue;
}

void ABaseEnemyTurret::WriteBlueprintBool(FName VariableName, bool Value)
{
    if (FBoolProperty* Property = FindFProperty<FBoolProperty>(GetClass(), VariableName))
    {
        Property->SetPropertyValue_InContainer(this, Value);
    }
}

float ABaseEnemyTurret::ReadBlueprintFloat(
    FName VariableName,
    float DefaultValue
) const
{
    if (const FFloatProperty* Property = FindFProperty<FFloatProperty>(
        GetClass(), VariableName))
    {
        return Property->GetPropertyValue_InContainer(this);
    }

    return DefaultValue;
}

void ABaseEnemyTurret::WriteBlueprintFloat(
    FName VariableName,
    float Value
)
{
    if (FFloatProperty* Property = FindFProperty<FFloatProperty>(
        GetClass(), VariableName))
    {
        Property->SetPropertyValue_InContainer(this, Value);
    }
}

UObject* ABaseEnemyTurret::ReadBlueprintObject(FName VariableName) const
{
    if (FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(GetClass(), VariableName))
    {
        return Property->GetObjectPropertyValue_InContainer(this);
    }
    return nullptr;
}

void ABaseEnemyTurret::WriteBlueprintObject(
    FName VariableName,
    UObject* Value
)
{
    if (FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(
        GetClass(), VariableName))
    {
        Property->SetObjectPropertyValue_InContainer(this, Value, 0);
    }
}
