#pragma once

#include "CoreMinimal.h"
#include "BaseGameRule.h"
#include "EncounterGameRule.generated.h"

class AActor;
class FMulticastDelegateProperty;

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UEncounterGameRule : public UBaseGameRule
{
    GENERATED_BODY()

public:
    virtual void InitRule_Implementation(int32 InRuleID) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
    // Matches the EncounterManager object variable from BPC_EncounterRule.
    UPROPERTY(Transient, BlueprintReadOnly, Category="Encounter Rule")
    TObjectPtr<AActor> EncounterManager;

    UFUNCTION()
    void HandleAllEncountersCompleted(int32 CompletedRuleID, bool bSuccessful);

private:
    FScriptDelegate BoundCompletionDelegate;
    FMulticastDelegateProperty* BoundCompletionProperty = nullptr;
    bool bCompletionDelegateBound = false;

    AActor* FindBlueprintEncounterManager() const;
    bool BindToManagerCompletion(AActor* Manager);
    void UnbindManagerCompletion();
};
