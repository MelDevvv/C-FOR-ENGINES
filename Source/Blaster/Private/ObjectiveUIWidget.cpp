#include "ObjectiveUIWidget.h"

#include "BaseGameRule.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Styling/SlateColor.h"
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
        // Presentation-only pass: keep rule widget logic/bindings untouched, improve typography.
        TArray<UWidget*> RuleChildren; if (RuleWidget->WidgetTree) RuleWidget->WidgetTree->GetAllWidgets(RuleChildren);
        for (UWidget* Child : RuleChildren)
        {
            if (UTextBlock* RuleText = Cast<UTextBlock>(Child))
            {
                RuleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f,0.88f,0.96f,1.0f)));
                FSlateFontInfo RuleFont = RuleText->GetFont(); RuleFont.Size = FMath::Max(12, RuleFont.Size); RuleText->SetFont(RuleFont);
                RuleText->SetShadowOffset(FVector2D(1.0f,1.0f)); RuleText->SetShadowColorAndOpacity(FLinearColor(0,0,0,0.65f));
            }
        }
    }
}
