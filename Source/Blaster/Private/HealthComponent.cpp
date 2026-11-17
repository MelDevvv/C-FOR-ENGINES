#include "HealthComponent.h"
#include "GameFramework/Actor.h"

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();
    if (AActor* Owner = GetOwner())
    {
        Owner->OnTakeAnyDamage.AddUniqueDynamic(this, &UHealthComponent::ApplyDamage);
        Owner->OnDestroyed.AddUniqueDynamic(this, &UHealthComponent::HandleOwnerDestroyed);
    }
    if (CurrentHealth <= 0.0f && !bIsDead)
    {
        ResetHealth();
    }
}

float UHealthComponent::GetCurrentHealth_Implementation()
{
    return CurrentHealth;
}

void UHealthComponent::ApplyDamage_Implementation(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    // Exact Blueprint flow: return only when dead or invulnerable.
    // Negative damage is allowed and therefore restores health through Clamp.
    if (bIsDead || bIsInvulnerable)
    {
        return;
    }

    CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
    const bool bPositive = Damage > 0.0f;
    const float HealthRatio = MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;

    // Broadcast the original Blueprint dispatcher and the native compatibility
    // dispatcher so existing widgets and Tank.cpp listeners both continue working.
    EHealthModified.Broadcast(bPositive, CurrentHealth, Damage, HealthRatio);
    OnHealthModified.Broadcast(bPositive, CurrentHealth, Damage, HealthRatio);
    CheckDeath(InstigatedBy, DamageCauser);
}

void UHealthComponent::CheckDeath_Implementation(AController* InstigatedBy, AActor* DamageCauser)
{
    if (!bIsDead && CurrentHealth <= 0.0f)
    {
        bIsDead = true;
        EDeath.Broadcast(InstigatedBy, DamageCauser);
        OnDeath.Broadcast();
    }
}

void UHealthComponent::ResetHealth()
{
    MaxHealth = FMath::Max(0.0f, MaxHealth);
    CurrentHealth = MaxHealth;
    bIsDead = false;
    bIsInvulnerable = false;
    const float HealthRatio = MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
    EHealthModified.Broadcast(false, CurrentHealth, 0.0f, HealthRatio);
    OnHealthModified.Broadcast(false, CurrentHealth, 0.0f, HealthRatio);
}

void UHealthComponent::ActivateTarget()
{
    // The original BPC_TargetComponent's ActivateTarget graph has no actions;
    // the event is intentionally retained for Blueprint/API compatibility.
}

void UHealthComponent::HandleOwnerDestroyed(AActor* DestroyedActor)
{
    ETargetDestroyed.Broadcast();
}
