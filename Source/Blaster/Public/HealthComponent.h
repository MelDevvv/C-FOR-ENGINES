#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UHealthComponent();
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDead = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentHealth = 0.0f;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	float GetCurrentHealth();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void CheckDeath(AController* instigator, AActor* causer);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 50.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bIsInvulnerable = false;
	
	
protected:
	virtual void BeginPlay() override;
};