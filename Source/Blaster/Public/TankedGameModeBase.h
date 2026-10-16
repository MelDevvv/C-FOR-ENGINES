
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "TankedGameModeBase.generated.h"

class UBaseGameRule;

UCLASS(Abstract, Blueprintable)
class BLASTER_API ATankedGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
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

	FTimerHandle EndGameTimerHandle;

	UBaseGameRule* CreateRule(
		TSubclassOf<UBaseGameRule> RuleClass);
};
