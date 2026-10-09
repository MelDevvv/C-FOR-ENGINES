#include "HealthComponent.h"
UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
}
float UHealthComponent::GetCurrentHealth_Implementation()
{
	return CurrentHealth;
}
void UHealthComponent::CheckDeath_Implementation(AController* instigator, AActor* causer)
{
	if (CurrentHealth <= 0.0f)
	{
		bIsDead = true;
	}
	// todo Event Dispatcher Call here!
}

