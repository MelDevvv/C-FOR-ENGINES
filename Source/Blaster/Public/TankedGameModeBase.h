#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "TankedGameModeBase.generated.h"

class UBaseGameRule;
class UObjectiveUIWidget;

UCLASS(Abstract, Blueprintable)
class BLASTER_API ATankedGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    ATankedGameModeBase();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules")
    TArray<TObjectPtr<UBaseGameRule>> ActiveRules;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
    TSubclassOf<UBaseGameRule> TimerRuleClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
    TSubclassOf<UBaseGameRule> EndReachedRuleClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
    TSubclassOf<UBaseGameRule> EncounterRuleClass;

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rules")
    void OnRuleCompleted(int32 RuleID, bool bSuccessful);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rules")
    void EndGame();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rules")
    void RegisterGameRules();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rules")
    bool CheckRules(int32 RuleID, bool bSuccessful);

    UFUNCTION(BlueprintNativeEvent, Category = "Rules")
    void CreateUIElementsForRules();

    UFUNCTION(BlueprintNativeEvent, Category = "Rules")
    void OnEndTimerComplete();

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UBaseGameRule>> CreatedRules;

    /** Native class reference to /Game/Scripts/UI/WBP_ObjectiveUI. */
    UPROPERTY(Transient)
    TSubclassOf<UObjectiveUIWidget> ObjectiveUIWidgetClass;

    /** Prevents creating a second objective panel if UI creation is requested again. */
    UPROPERTY(Transient)
    TObjectPtr<UObjectiveUIWidget> ObjectiveUIWidgetInstance = nullptr;

    FTimerHandle EndGameTimerHandle;

    UBaseGameRule* CreateRule(TSubclassOf<UBaseGameRule> RuleClass);
};
