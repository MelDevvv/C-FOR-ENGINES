#include "HealthComponent.h"
UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	// Bind to the engine's built-in damage event[span_5](start_span)[span_5](end_span)
	GetOwner()->OnTakeAnyDamage.AddUniqueDynamic(this, &UHealthComponent::ApplyDamage);
}
float UHealthComponent::GetCurrentHealth_Implementation()
{
	return CurrentHealth;
}
void UHealthComponent::ApplyDamage_Implementation(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	// C++ logic matching the old Blueprint graph[span_6](start_span)[span_6](end_span)
	if (bIsDead || bIsInvulnerable)
	{
		return;
	}
	else
	{
		CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
		OnHealthModified.Broadcast(CurrentHealth > 0.0f, CurrentHealth, Damage, CurrentHealth / MaxHealth);
		CheckDeath(InstigatedBy, DamageCauser);
	}
}
void UHealthComponent::CheckDeath_Implementation(AController* instigator, AActor* causer)
{
	if (CurrentHealth <= 0.0f)
	{
		bIsDead = true;
		OnDeath.Broadcast(); // Trigger the C++ Death delegate[span_7](start_span)[span_7](end_span)
	}
	
}
void UHealthComponent::ResetHealth()
{
	CurrentHealth = MaxHealth;
	bIsDead = false;
	bIsInvulnerable = false;
	OnHealthModified.Broadcast(true, CurrentHealth, 0.0f, 1.0f);
}