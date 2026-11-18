#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaseGameRule.generated.h"

class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRuleCompleteSignature, int32, RuleID, bool, bSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRuleUpdatedSignature, int32, RuleID, bool, bCompleted);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UBaseGameRule : public UActorComponent
{
    GENERATED_BODY()

public:
    UBaseGameRule();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rule")
    int32 RuleID = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rule")
    bool MajorRule = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rule")
    bool Completed = false;

    // Retains the property exposed by the original BPC_BaseGameRule Blueprint.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rule")
    TSubclassOf<UUserWidget> RuleUI;

    // Native equivalents of the original Blueprint multicast dispatchers.
    UPROPERTY(BlueprintAssignable, Category="Rule")
    FRuleCompleteSignature ERuleComplete;

    UPROPERTY(BlueprintAssignable, Category="Rule")
    FRuleUpdatedSignature ERuleUpdated;

    // Kept as compatibility aliases for existing native code.
    UPROPERTY(BlueprintAssignable, Category="Rule")
    FRuleCompleteSignature OnRuleComplete;

    UPROPERTY(BlueprintAssignable, Category="Rule")
    FRuleUpdatedSignature OnRuleUpdated;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Rule")
    void InitRule(int32 InRuleID);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Rule")
    void CompleteRule(bool bSuccessful);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Rule")
    void UpdateRuleConditions();
};
