#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BoostHUDWidget.generated.h"
class UProgressBar;
class UTextBlock;

/** Native HUD for boost, shield and secondary rocket launcher. */
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
protected:
    virtual void NativeOnInitialized() override;
private:
    UPROPERTY(Transient) TObjectPtr<UProgressBar> EnergyBar;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(Transient) TObjectPtr<UProgressBar> ShieldBar;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ShieldStatusText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RocketStatusText;
};
