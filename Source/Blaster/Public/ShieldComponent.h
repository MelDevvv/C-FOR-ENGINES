#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShieldComponent.generated.h"

/** Rechargeable defensive shield: absorbs incoming damage for a short activation window. */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UShieldComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UShieldComponent();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Shield", meta=(ClampMin="1.0"))
    float MaximumEnergy = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Shield", meta=(ClampMin="0.1"))
    float ActiveDuration = 5.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Shield", meta=(ClampMin="0.0"))
    float CooldownDuration = 10.0f;

    UFUNCTION(BlueprintCallable, Category="Shield")
    bool ActivateShield();

    /** Returns damage left over after shield absorption. */
    float AbsorbDamage(float IncomingDamage);

    UFUNCTION(BlueprintCallable, Category="Shield")
    void ResetShield();

    UFUNCTION(BlueprintPure, Category="Shield")
    bool IsShieldActive() const { return bShieldActive; }

    UFUNCTION(BlueprintPure, Category="Shield")
    float GetCurrentEnergy() const { return CurrentEnergy; }

    UFUNCTION(BlueprintPure, Category="Shield")
    float GetMaximumEnergy() const { return MaximumEnergy; }

    UFUNCTION(BlueprintPure, Category="Shield")
    float GetCooldownRemaining() const;

private:
    UPROPERTY(Transient)
    float CurrentEnergy = 100.0f;

    bool bShieldActive = false;
    bool bOnCooldown = false;
    FTimerHandle ActiveTimerHandle;
    FTimerHandle CooldownTimerHandle;

    void EndShield();
    void FinishCooldown();
};
