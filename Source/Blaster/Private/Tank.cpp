#include "Tank.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "HealthComponent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

ATank::ATank()
{
    PrimaryActorTick.bCanEverTick = true;
    bUseControllerRotationYaw = false;

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = 600.0f;
        Movement->bOrientRotationToMovement = false;
        Movement->bUseControllerDesiredRotation = false;
    }

    static ConstructorHelpers::FClassFinder<UUserWidget> HUDWidgetClass(
        TEXT("/Game/Scripts/UI/WBP_MainUI"));
    if (HUDWidgetClass.Succeeded())
    {
        TankHUDClass = HUDWidgetClass.Class;
    }

    static ConstructorHelpers::FObjectFinder<UParticleSystem> DeathEffectAsset(
        TEXT("/Game/Assets/Effects/PS_Dead.PS_Dead"));
    if (DeathEffectAsset.Succeeded())
    {
        DeathEffect = DeathEffectAsset.Object;
    }

    RotTracks = CreateDefaultSubobject<UArrowComponent>(TEXT("RotTracks"));
    RotTracks->SetupAttachment(RootComponent);

    Plane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plane"));
    Plane->SetupAttachment(RotTracks);

    Track01 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Track01"));
    Track01->SetupAttachment(RotTracks);

    Treads = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Treads"));
    Treads->SetupAttachment(Track01);

    Track02 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Track02"));
    Track02->SetupAttachment(RotTracks);

    Treads1 = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Treads1"));
    Treads1->SetupAttachment(Track02);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->SetRelativeRotation(FRotator(-70.0f, 0.0f, 0.0f));
    SpringArm->TargetArmLength = 500.0f;
    SpringArm->bDoCollisionTest = false;
    SpringArm->bEnableCameraLag = true;
    SpringArm->SetUsingAbsoluteRotation(true);

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);

    TCore01 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TCore01"));
    TCore01->SetupAttachment(RootComponent);

    TGun01 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TGun01"));
    TGun01->SetupAttachment(TCore01);

    W1 = CreateDefaultSubobject<UArrowComponent>(TEXT("W1"));
    W1->SetupAttachment(TGun01);

    W2 = CreateDefaultSubobject<UArrowComponent>(TEXT("W2"));
    W2->SetupAttachment(TGun01);

    TGun02 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TGun02"));
    TGun02->SetupAttachment(TCore01);

    W3 = CreateDefaultSubobject<UArrowComponent>(TEXT("W3"));
    W3->SetupAttachment(TGun02);

    W4 = CreateDefaultSubobject<UArrowComponent>(TEXT("W4"));
    W4->SetupAttachment(TGun02);

    HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
}

void ATank::BeginPlay()
{
    Super::BeginPlay();

    RespawnTransform = GetActorTransform();

    if (HealthComp)
    {
        HealthComp->ResetHealth();
        HealthComp->OnDeath.AddUniqueDynamic(this, &ATank::HandleTankDeath);
    }

    CreateTankHUD();

    if (TGun01)
    {
        Gun1RestLocation = TGun01->GetRelativeLocation();
    }

    if (TGun02)
    {
        Gun2RestLocation = TGun02->GetRelativeLocation();
    }
}

void ATank::CreateTankHUD()
{
    if (TankHUD || !TankHUDClass || !IsPlayerControlled())
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    }

    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("Tank HUD not created: no PlayerController."));
        return;
    }

    TankHUD = CreateWidget<UUserWidget>(PC, TankHUDClass);
    if (TankHUD)
    {
        TankHUD->AddToViewport();
    }
}

void ATank::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (TCore01 && !bIsRespawning)
    {
        APlayerController* PC = GetLocalViewingPlayerController();
        if (PC)
        {
            const ETraceTypeQuery TraceType =
                UEngineTypes::ConvertToTraceType(ECC_Visibility);

            FHitResult HitResult;
            if (PC->GetHitResultUnderCursorByChannel(TraceType, true, HitResult))
            {
                const FVector TargetLocation(
                    HitResult.Location.X,
                    HitResult.Location.Y,
                    TCore01->GetComponentLocation().Z);

                const FRotator NewRotation =
                    UKismetMathLibrary::FindLookAtRotation(
                        TCore01->GetComponentLocation(), TargetLocation);

                TCore01->SetWorldRotation(NewRotation);
            }
        }
    }

    UpdateGunShudders(DeltaTime);
}

void ATank::IMove_Implementation(float InputValue)
{
    if (bIsRespawning || FMath::IsNearlyZero(InputValue) || !RotTracks)
    {
        return;
    }

    AddMovementInput(RotTracks->GetForwardVector(), InputValue);
}

void ATank::ITurn_Implementation(float InputValue)
{
    if (bIsRespawning || FMath::IsNearlyZero(InputValue) || !RotTracks)
    {
        return;
    }

    const float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
    RotTracks->AddLocalRotation(
        FRotator(0.0f, InputValue * TurnSpeed * DeltaTime, 0.0f));
}

void ATank::IPrimaryAction_Implementation(bool bPressed)
{
    if (!bPressed || !bCanFire || bIsRespawning)
    {
        return;
    }

    UArrowComponent* SpawnPoint = nullptr;
    bool bFirstGun = false;

    switch (NextGunIndex)
    {
        case 0: SpawnPoint = W1; bFirstGun = true; break;
        case 1: SpawnPoint = W3; bFirstGun = false; break;
        case 2: SpawnPoint = W2; bFirstGun = true; break;
        case 3: SpawnPoint = W4; bFirstGun = false; break;
        default: break;
    }

    if (!SpawnPoint)
    {
        return;
    }

    bCanFire = false;
    FireProjectile(SpawnPoint, bFirstGun);
    StartGunShudder(bFirstGun);
    NextGunIndex = (NextGunIndex + 1) % 4;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            FireRateTimerHandle,
            this,
            &ATank::ResetFireCooldown,
            FMath::Max(0.05f, FireRateSeconds),
            false);
    }
}

void ATank::FireProjectile(UArrowComponent* SpawnPoint, bool bFirstGun)
{
    if (!ProjectileClass || !SpawnPoint || !GetWorld())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Cannot fire: check ProjectileClass and spawn point."));
        return;
    }

    const FTransform SpawnTransform = SpawnPoint->GetComponentTransform();

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = this;

    GetWorld()->SpawnActor<AActor>(
        ProjectileClass, SpawnTransform, SpawnParameters);

    if (ShotCaseEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            ShotCaseEffect,
            SpawnTransform.GetLocation(),
            SpawnTransform.Rotator(),
            FVector(1.0f),
            true);
    }
}

void ATank::StartGunShudder(bool bFirstGun)
{
    if (bFirstGun)
    {
        Gun1ShudderTime = 0.0f;
        bGun1ShudderActive = true;
    }
    else
    {
        Gun2ShudderTime = 0.0f;
        bGun2ShudderActive = true;
    }
}

void ATank::UpdateGunShudders(float DeltaTime)
{
    constexpr float ShudderDuration = 0.25f;
    constexpr float ShudderDistance = -2.0f;

    if (bGun1ShudderActive && TGun01)
    {
        Gun1ShudderTime += DeltaTime;
        const float Alpha = FMath::Clamp(
            Gun1ShudderTime / ShudderDuration, 0.0f, 1.0f);

        const float CurveValue = Alpha <= 0.4f
            ? FMath::Lerp(0.0f, ShudderDistance, Alpha / 0.4f)
            : FMath::Lerp(ShudderDistance, 0.0f, (Alpha - 0.4f) / 0.6f);

        FVector NewLocation = Gun1RestLocation;
        NewLocation.X += CurveValue;
        TGun01->SetRelativeLocation(NewLocation);

        if (Alpha >= 1.0f)
        {
            bGun1ShudderActive = false;
            TGun01->SetRelativeLocation(Gun1RestLocation);
        }
    }

    if (bGun2ShudderActive && TGun02)
    {
        Gun2ShudderTime += DeltaTime;
        const float Alpha = FMath::Clamp(
            Gun2ShudderTime / ShudderDuration, 0.0f, 1.0f);

        const float CurveValue = Alpha <= 0.4f
            ? FMath::Lerp(0.0f, ShudderDistance, Alpha / 0.4f)
            : FMath::Lerp(ShudderDistance, 0.0f, (Alpha - 0.4f) / 0.6f);

        FVector NewLocation = Gun2RestLocation;
        NewLocation.X += CurveValue;
        TGun02->SetRelativeLocation(NewLocation);

        if (Alpha >= 1.0f)
        {
            bGun2ShudderActive = false;
            TGun02->SetRelativeLocation(Gun2RestLocation);
        }
    }
}

void ATank::ResetFireCooldown()
{
    bCanFire = true;
}

void ATank::HandleTankDeath()
{
    if (bIsRespawning || !GetWorld())
    {
        return;
    }

    bIsRespawning = true;

    GetWorld()->GetTimerManager().ClearTimer(RespawnTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(FireRateTimerHandle);

    if (DeathEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(), DeathEffect, GetActorLocation(), GetActorRotation());
    }

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->DisableMovement();
    }

    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);

    GetWorld()->GetTimerManager().SetTimer(
        RespawnTimerHandle,
        this,
        &ATank::RespawnTank,
        FMath::Max(0.1f, RespawnTime),
        false);
}

void ATank::RespawnTank()
{
    if (!GetWorld())
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(RespawnTimerHandle);

    SetActorLocationAndRotation(
        RespawnTransform.GetLocation(),
        RespawnTransform.Rotator(),
        false, nullptr, ETeleportType::TeleportPhysics);

    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->SetMovementMode(MOVE_Walking);
    }

    if (HealthComp)
    {
        HealthComp->ResetHealth();
    }

    bCanFire = true;
    bIsRespawning = false;
}