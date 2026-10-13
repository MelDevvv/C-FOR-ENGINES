#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"
// Delegate Declarations[span_0](start_span)[span_0](end_span)[span_1](start_span)[span_1](end_span)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FHealthModifiedSignature, bool, bPositive, float, newHealth, float, healthDelta, float, healthRatio);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDeathSignature);
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 50.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bIsInvulnerable = false;
	// Delegate Properties[span_2](start_span)[span_2](end_span)[span_3](start_span)[span_3](end_span)
	UPROPERTY(BlueprintCallable, BlueprintAssignable)
	FHealthModifiedSignature OnHealthModified;
	UPROPERTY(BlueprintCallable, BlueprintAssignable)
	FDeathSignature OnDeath;
	// Functions[span_4](start_span)[span_4](end_span)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	float GetCurrentHealth();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void CheckDeath(AController* instigator, AActor* causer);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void ApplyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();
protected:
	virtual void BeginPlay() override;
};