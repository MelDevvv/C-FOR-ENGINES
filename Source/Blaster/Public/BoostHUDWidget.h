#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BoostHUDWidget.generated.h"
class UProgressBar;
class UTextBlock;
/** Native, presentation-only combat HUD. Existing update functions retain their gameplay-facing signatures. */
UCLASS()
class BLASTER_API UBoostHUDWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Tank|Boost")
    void UpdateBoostDisplay(float CurrentEnergy, float MaximumEnergy, bool bBoosting);
    UFUNCTION(BlueprintCallable, Category="Tank|Shield")
    void UpdateShieldDisplay(float CurrentEnergy, float MaximumEnergy, bool bActive, float CooldownRemaining);
    UFUNCTION(BlueprintCallable, Category="Tank|Weapons")
    void UpdateRocketDisplay(float CooldownRemaining);
    /** UI hook for the lightning-blast ability; gameplay code must call this to drive its cooldown. */
    UFUNCTION(BlueprintCallable, Category="Tank|Abilities")
    void UpdateLightningBlastDisplay(float CooldownRemaining, float CooldownDuration = 30.0f);
protected:
    virtual void NativeOnInitialized() override;
private:
    UPROPERTY(Transient) TObjectPtr<UProgressBar> EnergyBar;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(Transient) TObjectPtr<UProgressBar> ShieldBar;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ShieldStatusText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RocketStatusText;
    UPROPERTY(Transient) TObjectPtr<UProgressBar> LightningBlastBar;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LightningBlastStatusText;
};
