#include "HealthComponent.h"
#include "ShieldComponent.h"

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* OwnerActor = GetOwner())
    {
        OwnerActor->OnTakeAnyDamage.AddUniqueDynamic(this, &UHealthComponent::ApplyDamage);
    }
}

float UHealthComponent::GetCurrentHealth_Implementation()
{
    return CurrentHealth;
}

void UHealthComponent::ApplyDamage_Implementation(
    AActor* DamagedActor,
    float Damage,
    const UDamageType* DamageType,
    AController* InstigatedBy,
    AActor* DamageCauser)
{
    if (bIsDead || bIsInvulnerable || Damage <= 0.0f)
    {
        return;
    }

    float DamageAfterShield = Damage;
    if (AActor* OwnerActor = GetOwner())
    {
        if (UShieldComponent* Shield = OwnerActor->FindComponentByClass<UShieldComponent>())
        {
            DamageAfterShield = Shield->AbsorbDamage(Damage);
        }
    }

    if (DamageAfterShield <= 0.0f)
    {
        return;
    }

    CurrentHealth = FMath::Clamp(CurrentHealth - DamageAfterShield, 0.0f, MaxHealth);
    const float HealthRatio = MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;

    OnHealthModified.Broadcast(CurrentHealth > 0.0f, CurrentHealth, DamageAfterShield, HealthRatio);
    EHealthModified.Broadcast(CurrentHealth > 0.0f, CurrentHealth, DamageAfterShield, HealthRatio);

    CheckDeath(InstigatedBy, DamageCauser);
}

void UHealthComponent::CheckDeath_Implementation(AController* InstigatedBy, AActor* DamageCauser)
{
    if (CurrentHealth <= 0.0f && !bIsDead)
    {
        bIsDead = true;
        OnDeath.Broadcast();
        EDeath.Broadcast(InstigatedBy, DamageCauser);
    }
}

void UHealthComponent::ResetHealth()
{
    CurrentHealth = MaxHealth;
    bIsDead = false;
    bIsInvulnerable = false;

    OnHealthModified.Broadcast(true, CurrentHealth, 0.0f, 1.0f);
    EHealthModified.Broadcast(true, CurrentHealth, 0.0f, 1.0f);
}

void UHealthComponent::ActivateTarget()
{
    // the original target component listens for its owning actor being destroyed
    // adduniquedynamic makes this safe if the encounter manager activates it again
    if (AActor* OwnerActor = GetOwner())
    {
        OwnerActor->OnDestroyed.AddUniqueDynamic(this, &UHealthComponent::HandleOwnerDestroyed);
    }
}

void UHealthComponent::HandleOwnerDestroyed(AActor* DestroyedActor)
{
    ETargetDestroyed.Broadcast();
}
