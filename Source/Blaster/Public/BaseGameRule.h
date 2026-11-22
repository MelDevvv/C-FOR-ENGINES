#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "BaseGameRule.generated.h"

class UUserWidget;
class AActor;
class FMulticastDelegateProperty;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rule")
    TSubclassOf<UUserWidget> RuleUI;

    UPROPERTY(BlueprintAssignable, Category="Rule")
    FRuleCompleteSignature ERuleComplete;

    UPROPERTY(BlueprintAssignable, Category="Rule")
    FRuleUpdatedSignature ERuleUpdated;

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

    // timer controls used by the original bpctimergamerule custom events
    UFUNCTION(BlueprintCallable, Category="Rule|Timer")
    void AddToTimer(float SecondsToAdd);

    UFUNCTION(BlueprintCallable, Category="Rule|Timer")
    void DeductFromTimer(float SecondsToRemove);

    UFUNCTION(BlueprintPure, Category="Rule|Timer")
    float GetTimerSecondsRemaining() const;

protected:
    // shared with uencountergamerule which inherits this manager reference
    UPROPERTY(Transient, BlueprintReadOnly, Category="Encounter Rule")
    TObjectPtr<AActor> EncounterManager = nullptr;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION()
    void HandleEncounterCompletion(int32 CompletedRuleID, bool bSuccessful);

private:
    // existing encounter rule native binding
    bool IsEncounterRuleBlueprint() const;
    AActor* FindBlueprintEncounterManager() const;
    bool BindEncounterManager(AActor* Manager);
    void TryBindEncounterManager();
    void UnbindEncounterManager();

    FScriptDelegate EncounterCompletionDelegate;
    FMulticastDelegateProperty* EncounterCompletionProperty = nullptr;
    bool bEncounterCompletionBound = false;

    // timer is activated only for the existing bpctimergamerule blueprint child
    FTimerHandle NativeRuleTimerHandle;
    FTimerHandle NativeRuleTickHandle;
    bool bNativeTimerRule = false;
    bool bNativeTimerCompleted = false;

    float ReadBlueprintTimerLength() const;
    bool HasBlueprintTimerContract() const;
    void StartNativeTimer(float DurationSeconds);
    void NativeTimerCheck();
    void NativeTimerExpired();
    void ClearNativeTimerHandles();
    bool BroadcastBlueprintTimerDispatcher(FName DispatcherName, bool bHasDoubleParameter = false, double DoubleParameter = 0.0);
};
