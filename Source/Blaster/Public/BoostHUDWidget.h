#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BoostHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;

/** Native-only HUD for the tank's rechargeable emergency boost. */
UCLASS()
class BLASTER_API UBoostHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Tank|Boost")
    void UpdateBoostDisplay(float CurrentEnergy, float MaximumEnergy, bool bBoosting);

protected:
    virtual void NativeOnInitialized() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> EnergyBar;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StatusText;
};
