#pragma once

#include "CoreMinimal.h"
#include "BaseGameRule.h"
#include "EndReachedGameRule.generated.h"

class AActor;
class FMulticastDelegateProperty;

/** Native implementation of BPC_EndReachedRule. */
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UEndReachedGameRule : public UBaseGameRule
{
    GENERATED_BODY()

public:
    virtual void InitRule_Implementation(int32 InRuleID) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // runtime reference to the bpendpoint actor found during initrule
    UPROPERTY(Transient)
    TObjectPtr<AActor> CachedEndPoint = nullptr;

    FScriptDelegate EndReachedDelegate;
    FMulticastDelegateProperty* EndReachedProperty = nullptr;
    bool bEndReachedBound = false;

    AActor* FindBlueprintEndPoint() const;
    bool BindEndPoint(AActor* EndPointActor);
    void UnbindEndPoint();

    UFUNCTION()
    void HandleEndReached();
};
