#include "BaseEnemyTurret.h"

#include "Tank.h"
#include "HealthComponent.h"
#include "Components/ArrowComponent.h"
#include "Particles/ParticleSystemComponent.h"

#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "CollisionShape.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
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
}

void ABaseEnemyTurret::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    CacheBlueprintComponents();

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

    UpdateTurretAim();
    UpdateNativeMachineGunShake(DeltaTime);
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
    ATank* FoundTank = Cast<ATank>(
        UGameplayStatics::GetPlayerCharacter(this, 0)
    );

    // Match the original Blueprint's GetAllActorsOfClass fallback.
    if (!IsValid(FoundTank) && GetWorld())
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(
            GetWorld(),
            ATank::StaticClass(),
            FoundActors
        );

        for (AActor* Actor : FoundActors)
        {
            FoundTank = Cast<ATank>(Actor);
            if (IsValid(FoundTank))
            {
                break;
            }
        }
    }

    CachedTank = IsValid(FoundTank) ? FoundTank : nullptr;

    // The Rocket child Blueprint uses the inherited TankREF variable.
    // Write it without declaring a duplicate native UPROPERTY.
    WriteBlueprintObject(FName(TEXT("TankREF")), CachedTank);
}

void ABaseEnemyTurret::UpdateTurretAim()
{
    if (!IsValid(CachedTank) || !IsValid(CachedTurretSpin))
    {
        return;
    }

    FVector Direction =
        CachedTank->GetActorLocation() -
        CachedTurretSpin->GetComponentLocation();

    // The original Blueprint sets roll and pitch to zero and interpolates yaw.
    Direction.Z = 0.0f;

    if (Direction.IsNearlyZero())
    {
        return;
    }

    FRotator TargetRotation = Direction.Rotation();
    TargetRotation.Pitch = 0.0f;
    TargetRotation.Roll = 0.0f;

    const FRotator NewRotation = FMath::RInterpTo(
        CachedTurretSpin->GetComponentRotation(),
        TargetRotation,
        GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f,
        2.0f
    );

    CachedTurretSpin->SetWorldRotation(NewRotation);
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

void ABaseEnemyTurret::ExecuteChildTurretFire()
{
    if (GetClass()->GetName().Contains(TEXT("Enemy_MachineGun")))
    {
        ExecuteNativeMachineGunFire();
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
