#include "ShieldComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UShieldComponent::UShieldComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UShieldComponent::ActivateShield()
{
    if (bShieldActive || bOnCooldown || CurrentEnergy <= KINDA_SMALL_NUMBER || !GetWorld())
    {
        return false;
    }

    bShieldActive = true;
    GetWorld()->GetTimerManager().ClearTimer(ActiveTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        ActiveTimerHandle, this, &UShieldComponent::EndShield,
        FMath::Max(0.1f, ActiveDuration), false);

    UE_LOG(LogTemp, Log, TEXT("Shield activated for %.1f seconds."), ActiveDuration);
    return true;
}

float UShieldComponent::AbsorbDamage(float IncomingDamage)
{
    if (!bShieldActive || IncomingDamage <= 0.0f)
    {
        return IncomingDamage;
    }

    const float AbsorbedDamage = FMath::Min(CurrentEnergy, IncomingDamage);
    CurrentEnergy = FMath::Max(0.0f, CurrentEnergy - AbsorbedDamage);
    const float RemainingDamage = FMath::Max(0.0f, IncomingDamage - AbsorbedDamage);

    if (CurrentEnergy <= KINDA_SMALL_NUMBER)
    {
        EndShield();
    }

    return RemainingDamage;
}

void UShieldComponent::EndShield()
{
    if (!bShieldActive)
    {
        return;
    }

    bShieldActive = false;
    if (!GetWorld())
    {
        return;
    }

    bOnCooldown = true;
    GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        CooldownTimerHandle, this, &UShieldComponent::FinishCooldown,
        FMath::Max(0.0f, CooldownDuration), false);

    UE_LOG(LogTemp, Log, TEXT("Shield ended. Cooldown: %.1f seconds."), CooldownDuration);
}

void UShieldComponent::FinishCooldown()
{
    bOnCooldown = false;
    CurrentEnergy = MaximumEnergy;
    UE_LOG(LogTemp, Log, TEXT("Shield recharged and ready."));
}

void UShieldComponent::ResetShield()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ActiveTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);
    }
    bShieldActive = false;
    bOnCooldown = false;
    CurrentEnergy = MaximumEnergy;
}

float UShieldComponent::GetCooldownRemaining() const
{
    if (!bOnCooldown || !GetWorld())
    {
        return 0.0f;
    }
    return GetWorld()->GetTimerManager().GetTimerRemaining(CooldownTimerHandle);
}
