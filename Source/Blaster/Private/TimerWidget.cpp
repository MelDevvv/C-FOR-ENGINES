#include "TimerWidget.h"

#include "Components/ActorComponent.h"
#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"

namespace
{
    const TCHAR* TimerRuleBlueprintClassPath =
        TEXT("/Game/Scripts/Gamemodes/Rules/BPC_TimerGameRule.BPC_TimerGameRule_C");
}

void UTimerWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // The original EventGraph explicitly sets:
    // Not Hit Testable (Self & All Children).
    // This must happen even if timer-rule lookup fails.
    SetVisibility(ESlateVisibility::HitTestInvisible);

    UE_LOG(LogTemp, Warning, TEXT("WBP_Timer native construct. TextBlock=%s"),
        T_Timer ? TEXT("valid") : TEXT("MISSING"));

    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Timer: GetWorld returned null."));
        return;
    }

    if (!T_Timer)
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Timer: BindWidget T_Timer was not found. Check the Designer widget name."));
        return;
    }

    // Match Blueprint Get Game Mode -> Get Component by Class.
    AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
    if (!GameMode)
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Timer: GetGameMode returned null."));
        return;
    }

    UClass* TimerRuleClass = LoadClass<UActorComponent>(nullptr, TimerRuleBlueprintClassPath);
    if (!TimerRuleClass)
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Timer: Could not load %s"), TimerRuleBlueprintClassPath);
        return;
    }

    TimerRuleComponent = GameMode->GetComponentByClass(TimerRuleClass);
    if (!IsValid(TimerRuleComponent))
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Timer: GameMode %s does not contain BPC_TimerGameRule."),
            *GameMode->GetClass()->GetName());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("WBP_Timer: Found timer component %s"),
        *TimerRuleComponent->GetClass()->GetName());

    // Match the three Blueprint Bind Event nodes, in their original order.
    BindTimerDispatcher(TEXT("ETimerStarted"), GET_FUNCTION_NAME_CHECKED(UTimerWidget, HandleTimerStarted));
    BindTimerDispatcher(TEXT("ETimerTicked"), GET_FUNCTION_NAME_CHECKED(UTimerWidget, HandleTimerTicked));
    BindTimerDispatcher(TEXT("ETimerCompleted"), GET_FUNCTION_NAME_CHECKED(UTimerWidget, HandleTimerCompleted));

    // Match the original EventGraph's initial UpdateText(TimerLength).
    UpdateText(ReadTimerLength());
}

void UTimerWidget::NativeDestruct()
{
    UnbindTimerDispatchers();
    TimerRuleComponent = nullptr;
    Super::NativeDestruct();
}

void UTimerWidget::HandleTimerStarted()
{
    UpdateText(ReadTimerLength());
}

void UTimerWidget::HandleTimerTicked(double TimeRemaining)
{
    // Original graph rounds TimeRemaining before calling UpdateText.
    UpdateText(static_cast<double>(FMath::RoundToInt(TimeRemaining)));
}

void UTimerWidget::HandleTimerCompleted()
{
    UpdateText(0.0);
}

void UTimerWidget::BindTimerDispatcher(FName DispatcherName, FName HandlerName)
{
    if (!IsValid(TimerRuleComponent))
    {
        return;
    }

    FMulticastDelegateProperty* Property =
        FindFProperty<FMulticastDelegateProperty>(TimerRuleComponent->GetClass(), DispatcherName);

    if (!Property)
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Timer: Dispatcher %s not found on %s."),
            *DispatcherName.ToString(), *TimerRuleComponent->GetClass()->GetName());
        return;
    }

    FMulticastScriptDelegate* Delegate =
        Property->ContainerPtrToValuePtr<FMulticastScriptDelegate>(TimerRuleComponent);

    if (!Delegate)
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Timer: Could not access dispatcher %s."), *DispatcherName.ToString());
        return;
    }

    FScriptDelegate Binding;
    Binding.BindUFunction(this, HandlerName);
    Delegate->AddUnique(Binding);

    if (DispatcherName == TEXT("ETimerStarted"))
    {
        StartedDelegate = Binding;
    }
    else if (DispatcherName == TEXT("ETimerTicked"))
    {
        TickedDelegate = Binding;
    }
    else if (DispatcherName == TEXT("ETimerCompleted"))
    {
        CompletedDelegate = Binding;
    }
}

void UTimerWidget::UnbindTimerDispatchers()
{
    if (!IsValid(TimerRuleComponent))
    {
        return;
    }

    const auto RemoveBinding = [this](FName DispatcherName, const FScriptDelegate& Binding)
    {
        if (FMulticastDelegateProperty* Property =
            FindFProperty<FMulticastDelegateProperty>(TimerRuleComponent->GetClass(), DispatcherName))
        {
            FMulticastScriptDelegate* Delegate =
                Property->ContainerPtrToValuePtr<FMulticastScriptDelegate>(TimerRuleComponent);

            if (Delegate)
            {
                Delegate->Remove(Binding);
            }
        }
    };

    RemoveBinding(TEXT("ETimerStarted"), StartedDelegate);
    RemoveBinding(TEXT("ETimerTicked"), TickedDelegate);
    RemoveBinding(TEXT("ETimerCompleted"), CompletedDelegate);
}

double UTimerWidget::ReadTimerLength() const
{
    if (!IsValid(TimerRuleComponent))
    {
        return 0.0;
    }

    if (const FFloatProperty* FloatProperty =
        FindFProperty<FFloatProperty>(TimerRuleComponent->GetClass(), TEXT("TimerLength")))
    {
        return static_cast<double>(FloatProperty->GetPropertyValue_InContainer(TimerRuleComponent));
    }

    UE_LOG(LogTemp, Error, TEXT("WBP_Timer: TimerLength float property not found on %s."),
        *TimerRuleComponent->GetClass()->GetName());
    return 0.0;
}

void UTimerWidget::UpdateText(double Seconds)
{
    if (!T_Timer)
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Timer::UpdateText called but T_Timer is null."));
        return;
    }

    // Exact Blueprint conversion: From Seconds -> As Timespan -> SetText(T_Timer).
    const FTimespan TimeSpan = FTimespan::FromSeconds(Seconds);
    T_Timer->SetText(FText::AsTimespan(TimeSpan));
    T_Timer->SetColorAndOpacity(FSlateColor(Seconds <= 30.0 ? FLinearColor(1.0f,0.28f,0.2f,1.0f) : FLinearColor(0.78f,0.91f,1.0f,1.0f)));
    FSlateFontInfo TimerFont = T_Timer->GetFont(); TimerFont.Size = 30; T_Timer->SetFont(TimerFont);
    T_Timer->SetShadowOffset(FVector2D(2.0f,2.0f)); T_Timer->SetShadowColorAndOpacity(FLinearColor(0.0f,0.0f,0.0f,0.8f));
}
