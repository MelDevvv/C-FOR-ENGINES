#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ObjectiveUIWidget.generated.h"

class UVerticalBox;
class UBaseGameRule;

/**
 * Native logic for WBP_ObjectiveUI.
 *
 * Keep the existing Widget Blueprint as a child of this class so its Designer
 * layout and ObjectivesPanel widget binding remain intact.
 */
UCLASS(Blueprintable)
class BLASTER_API UObjectiveUIWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * Replaces the original WBP_ObjectiveUI EventGraph custom event.
     * ActiveRules is intentionally UObject-based to match the exported
     * Blueprint pin type (Object Array).
     */
    UFUNCTION(BlueprintCallable, Category = "Objectives")
    void InitRules(const TArray<UObject*>& ActiveRules);

    /**
     * Native replacement for the existing InitRulesCPP Blueprint function.
     */
    UFUNCTION(BlueprintCallable, Category = "Objectives")
    void InitRulesCPP(const TArray<UObject*>& ActiveRules);

protected:
    /** Must retain the existing VerticalBox named ObjectivesPanel in the Designer. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> ObjectivesPanel;

private:
    void AddRuleWidgets(const TArray<UObject*>& ActiveRules);
};
