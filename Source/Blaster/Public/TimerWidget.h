#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerWidget.generated.h"

class UTextBlock;
class UActorComponent;

UCLASS()
class BLASTER_API UTimerWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // must match the existing designer textblock name
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> T_Timer;

    UPROPERTY(Transient)
    TObjectPtr<UActorComponent> TimerRuleComponent;

    UFUNCTION()
    void HandleTimerStarted();

    UFUNCTION()
    void HandleTimerTicked(double TimeRemaining);

    UFUNCTION()
    void HandleTimerCompleted();

    void BindTimerDispatcher(FName DispatcherName, FName HandlerName);
    void UnbindTimerDispatchers();
    double ReadTimerLength() const;

    FScriptDelegate StartedDelegate;
    FScriptDelegate TickedDelegate;
    FScriptDelegate CompletedDelegate;

public:
    // native equivalent of the blueprint updatetextseconds function
    UFUNCTION(BlueprintCallable, Category = "Timer")
    void UpdateText(double Seconds);
};
