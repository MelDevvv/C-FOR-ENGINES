
#include "TankController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Inputable.h"
#include "Engine/LocalPlayer.h"
#include "UObject/ConstructorHelpers.h"

ATankController::ATankController()
{
    static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextAsset(
        TEXT("/Game/Assets/Input/IMC_Tank.IMC_Tank"));

    if (MappingContextAsset.Succeeded())
    {
        MappingContext = MappingContextAsset.Object;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load IMC_Tank."));
    }

    static ConstructorHelpers::FObjectFinder<UInputAction> ForwardActionAsset(
        TEXT("/Game/Assets/Input/IA_Forward.IA_Forward"));

    if (ForwardActionAsset.Succeeded())
    {
        ForwardAction = ForwardActionAsset.Object;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load IA_Forward."));
    }

    static ConstructorHelpers::FObjectFinder<UInputAction> TurnActionAsset(
        TEXT("/Game/Assets/Input/IA_Turn.IA_Turn"));

    if (TurnActionAsset.Succeeded())
    {
        TurnAction = TurnActionAsset.Object;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load IA_Turn."));
    }

    static ConstructorHelpers::FObjectFinder<UInputAction> FireActionAsset(
        TEXT("/Game/Assets/Input/IA_Fire.IA_Fire"));

    if (FireActionAsset.Succeeded())
    {
        FireAction = FireActionAsset.Object;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load IA_Fire."));
    }
}

void ATankController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (!IsValid(InPawn))
    {
        UE_LOG(LogTemp, Error, TEXT("No pawn to possess."));
        return;
    }

    ULocalPlayer* LocalPlayer = GetLocalPlayer();

    if (!LocalPlayer)
    {
        UE_LOG(LogTemp, Error, TEXT("No local player found."));
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
            LocalPlayer);

    if (!Subsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("Enhanced Input subsystem not found."));
        return;
    }

    if (MappingContext)
    {
        Subsystem->ClearAllMappings();
        Subsystem->AddMappingContext(MappingContext, 0);
    }

    SetShowMouseCursor(true);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("TankController possessed pawn: %s"),
        *InPawn->GetName());
}

void ATankController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInput =
        Cast<UEnhancedInputComponent>(InputComponent);

    if (!EnhancedInput)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Enhanced Input component not found."));
        return;
    }

    if (ForwardAction)
    {
        EnhancedInput->BindAction(
            ForwardAction,
            ETriggerEvent::Triggered,
            this,
            &ATankController::OnForward);
    }

    if (TurnAction)
    {
        EnhancedInput->BindAction(
            TurnAction,
            ETriggerEvent::Triggered,
            this,
            &ATankController::OnTurn);
    }

    if (FireAction)
    {
        EnhancedInput->BindAction(
            FireAction,
            ETriggerEvent::Triggered,
            this,
            &ATankController::OnFire);
    }

    UE_LOG(LogTemp, Warning, TEXT("TankController input actions bound."));
}

void ATankController::OnForward(const FInputActionInstance& Instance)
{
    APawn* ControlledPawn = GetPawn();

    if (IsValid(ControlledPawn) &&
        ControlledPawn->GetClass()->ImplementsInterface(
            UInputable::StaticClass()))
    {
        IInputable::Execute_IMove(
            ControlledPawn,
            Instance.GetValue().Get<float>());
    }
}

void ATankController::OnTurn(const FInputActionInstance& Instance)
{
    APawn* ControlledPawn = GetPawn();

    if (IsValid(ControlledPawn) &&
        ControlledPawn->GetClass()->ImplementsInterface(
            UInputable::StaticClass()))
    {
        IInputable::Execute_ITurn(
            ControlledPawn,
            Instance.GetValue().Get<float>());
    }
}

void ATankController::OnFire(const FInputActionInstance& Instance)
{
    APawn* ControlledPawn = GetPawn();

    if (IsValid(ControlledPawn) &&
        ControlledPawn->GetClass()->ImplementsInterface(
            UInputable::StaticClass()))
    {
        IInputable::Execute_IPrimaryAction(
            ControlledPawn,
            Instance.GetValue().Get<bool>());
    }
}
