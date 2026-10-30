#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainUIWidget.generated.h"
class UProgressBar;
UCLASS()
class BLASTER_API UMainUIWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Health")
	void UpdateHealthBar(
		bool bPositive,
		float NewHealth,
		float HealthDelta,
		float HealthRatio
	);
protected:
	virtual void NativeConstruct() override;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;
};