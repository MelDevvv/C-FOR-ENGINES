#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FHealthModifiedSignature, bool, bPositive, float, NewHealth, float, HealthDelta, float, HealthRatio);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDeathSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDeathContextSignature, AController*, InstigatedBy, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTargetDestroyedSignature);

/** Native implementation of the project's health and target-destruction component behaviour. */
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHealthComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health") bool bIsDead = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health") float CurrentHealth = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health") float MaxHealth = 50.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health") bool bIsInvulnerable = false;

    // Original Blueprint dispatcher name and signature.
    UPROPERTY(BlueprintAssignable, Category="Health") FHealthModifiedSignature EHealthModified;
    // Compatibility dispatcher used by existing native Tank code.
    UPROPERTY(BlueprintAssignable, Category="Health") FHealthModifiedSignature OnHealthModified;
    // Kept parameterless for existing native consumers such as ATank.
    UPROPERTY(BlueprintAssignable, Category="Health") FDeathSignature OnDeath;
    // Matches the original Blueprint EDeath(Controller, Actor) dispatcher.
    UPROPERTY(BlueprintAssignable, Category="Health") FDeathContextSignature EDeath;
    UPROPERTY(BlueprintAssignable, Category="Target") FTargetDestroyedSignature ETargetDestroyed;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Health") float GetCurrentHealth();
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Health") void CheckDeath(AController* InstigatedBy, AActor* DamageCauser);
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Health") void ApplyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
    UFUNCTION(BlueprintCallable, Category="Health") void ResetHealth();
    UFUNCTION(BlueprintPure, Category="Health") bool IsDead() const { return bIsDead; }

    /** Target component equivalents: bind owner destruction and activate the target. */
    UFUNCTION(BlueprintCallable, Category="Target") void ActivateTarget();

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION() void HandleOwnerDestroyed(AActor* DestroyedActor);
};
