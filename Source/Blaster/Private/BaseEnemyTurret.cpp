#include "BaseEnemyTurret.h"

#include "Tank.h"

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

    // This is the effect referenced by the original BP_BaseEnemyTurret asset.
    static ConstructorHelpers::FObjectFinder<UParticleSystem> DeathEffectAsset(
        TEXT("/Game/Assets/Effects/PS_EnemyDeathExplosion.PS_EnemyDeathExplosion")
    );

    if (DeathEffectAsset.Succeeded())
    {
        NativeDeathEffect = DeathEffectAsset.Object;
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
    InitialiseBlueprintHealthComponent();
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
}

void ABaseEnemyTurret::CacheBlueprintComponents()
{
    TArray<UActorComponent*> Components;
    GetComponents<UActorComponent>(Components);

    CachedTurretSpin = nullptr;
    CachedDangerZone = nullptr;
    CachedHealthActorComponent = nullptr;

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

        // The uploaded project has a Blueprint-only BPC_HealthComponent whose
        // native parent is ActorComponent. Do not cast it to UHealthComponent.
        if (Component->GetFName() == FName(TEXT("HealthComponent")) ||
            ComponentName.Contains(TEXT("HealthComponent")) ||
            ComponentClassName.Contains(TEXT("BPC_HealthComponent")))
        {
            CachedHealthActorComponent = Component;
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

    if (!CachedHealthActorComponent)
    {
        UE_LOG(LogTemp, Error,
            TEXT("%s: Could not find the Blueprint HealthComponent."),
            *GetName());
    }
}

void ABaseEnemyTurret::InitialiseBlueprintHealthComponent()
{
    if (!CachedHealthActorComponent)
    {
        return;
    }

    // Preserve a configured component MaxHealth. If it is unset (<= 0),
    // fall back to the parent Blueprint's MaxHP variable.
    const float MaxHP = ReadBlueprintFloat(FName(TEXT("MaxHP")), 50.0f);
    float EffectiveMaxHealth = MaxHP;

    if (FFloatProperty* MaxHealthProperty = FindFProperty<FFloatProperty>(
        CachedHealthActorComponent->GetClass(), FName(TEXT("MaxHealth"))))
    {
        const float ConfiguredMaxHealth =
            MaxHealthProperty->GetPropertyValue_InContainer(
                CachedHealthActorComponent
            );

        if (ConfiguredMaxHealth > 0.0f)
        {
            EffectiveMaxHealth = ConfiguredMaxHealth;
        }
        else if (MaxHP > 0.0f)
        {
            MaxHealthProperty->SetPropertyValue_InContainer(
                CachedHealthActorComponent,
                MaxHP
            );
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("%s: HealthComponent has no MaxHealth float property."),
            *GetName());
    }

    // The original Blueprint component exposes a ResetHealth function.
    if (UFunction* ResetFunction = CachedHealthActorComponent->FindFunction(
        FName(TEXT("ResetHealth"))))
    {
        CachedHealthActorComponent->ProcessEvent(ResetFunction, nullptr);
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("%s: HealthComponent has no ResetHealth function."),
            *GetName());
    }

    // The supplied BPC_HealthComponent exposes the EDeath dispatcher.
    // Support OnDeath as well in case the component is later replaced by the
    // native UHealthComponent or its Blueprint child.
    FMulticastDelegateProperty* DeathDelegateProperty =
        FindFProperty<FMulticastDelegateProperty>(
            CachedHealthActorComponent->GetClass(),
            FName(TEXT("EDeath"))
        );

    if (!DeathDelegateProperty)
    {
        DeathDelegateProperty = FindFProperty<FMulticastDelegateProperty>(
            CachedHealthActorComponent->GetClass(),
            FName(TEXT("OnDeath"))
        );
    }

    if (DeathDelegateProperty)
    {
        // The original BPC_HealthComponent.EDeath signature contains
        // (AController*, AActor*). The native UHealthComponent.OnDeath
        // dispatcher has no parameters, so select a matching handler.
        int32 SignatureParameterCount = 0;
        if (DeathDelegateProperty->SignatureFunction)
        {
            for (TFieldIterator<FProperty> It(DeathDelegateProperty->SignatureFunction); It; ++It)
            {
                if (It->HasAnyPropertyFlags(CPF_Parm) &&
                    !It->HasAnyPropertyFlags(CPF_ReturnParm))
                {
                    ++SignatureParameterCount;
                }
            }
        }

        const FName HandlerName = SignatureParameterCount >= 2
            ? FName(TEXT("HandleHealthDeathWithContext"))
            : FName(TEXT("HandleHealthDeath"));

        FScriptDelegate DeathBinding;
        DeathBinding.BindUFunction(this, HandlerName);

        void* DelegateValue =
            DeathDelegateProperty->ContainerPtrToValuePtr<void>(
                CachedHealthActorComponent
            );

        DeathDelegateProperty->AddDelegate(
            DeathBinding,
            CachedHealthActorComponent,
            DelegateValue
        );
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("%s: HealthComponent has neither EDeath nor OnDeath dispatcher."),
            *GetName());
    }

    UE_LOG(LogTemp, Warning,
        TEXT("%s: Health initialised. MaxHP = %.1f. Health component = %s."),
        *GetName(),
        EffectiveMaxHealth,
        *CachedHealthActorComponent->GetClass()->GetName());
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

void ABaseEnemyTurret::ExecuteChildTurretFire()
{
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
