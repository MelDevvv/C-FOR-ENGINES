#include "ObjectiveUIWidget.h"

#include "BaseGameRule.h"
#include "Blueprint/UserWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"

void UObjectiveUIWidget::InitRules(const TArray<UObject*>& ActiveRules)
{
    AddRuleWidgets(ActiveRules);
}

void UObjectiveUIWidget::InitRulesCPP(const TArray<UObject*>& ActiveRules)
{
    AddRuleWidgets(ActiveRules);
}

void UObjectiveUIWidget::AddRuleWidgets(const TArray<UObject*>& ActiveRules)
{
    if (!IsValid(ObjectivesPanel))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("%s: ObjectivesPanel is not bound. Check the Widget Designer name and type."),
            *GetName());

        return;
    }

    for (UObject* RuleObject : ActiveRules)
    {
        UBaseGameRule* Rule = Cast<UBaseGameRule>(RuleObject);
        if (!IsValid(Rule))
        {
            continue;
        }

        UClass* RuleWidgetClass = Rule->RuleUI.Get();
        if (!IsValid(RuleWidgetClass))
        {
            continue;
        }

        // The original Blueprint leaves OwningPlayer unconnected, so use
        // the world-context CreateWidget overload rather than requiring a PC.
        UUserWidget* RuleWidget = CreateWidget<UUserWidget>(GetWorld(), RuleWidgetClass);
        if (!IsValid(RuleWidget))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("%s: Failed to create rule widget for rule ID %d."),
                *GetName(),
                Rule->RuleID);

            continue;
        }

        ObjectivesPanel->AddChildToVerticalBox(RuleWidget);
    }
}
