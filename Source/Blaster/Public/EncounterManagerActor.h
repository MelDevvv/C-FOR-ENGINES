#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameStructs.h"
#include "EncounterManagerActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEncounterStartedSignature, int32, EncounterID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTargetCountUpdatedSignature, int32, TargetCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEncounterEndedSignature, int32, EncounterID, bool, bSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEncountersCompletedSignature, int32, EncounterID, bool, bSuccessful);

/** Native replacement base for BP_EncounterManager. Existing Blueprint doors,
 * buttons and target components are deliberately invoked through their existing
 * reflected functions/events so they can remain Blueprint assets. */
UCLASS(Blueprintable)
class BLASTER_API AEncounterManagerActor : public AActor
{
    GENERATED_BODY()

public:
    AEncounterManagerActor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Keep the original Blueprint variable names and types so existing defaults
    // and level-instance values can be retained when BP_EncounterManager is reparented.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Encounters")
    TArray<FEncounter> LevelEncounters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Encounters")
    TMap<TObjectPtr<AActor>, int32> EncounterTriggers;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Encounters")
    int32 CurrentTargetCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Encounters")
    TArray<int32> CompletedEncounters;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Encounters")
    FEncounter TargetEncounter;

    UPROPERTY(BlueprintAssignable, Category="Encounters|Events")
    FEncounterStartedSignature EEncounterStarted;

    UPROPERTY(BlueprintAssignable, Category="Encounters|Events")
    FTargetCountUpdatedSignature ETargetCountUpdated;

    UPROPERTY(BlueprintAssignable, Category="Encounters|Events")
    FEncounterEndedSignature EEncounterEnded;

    UPROPERTY(BlueprintAssignable, Category="Encounters|Events")
    FEncountersCompletedSignature EEncountersCompleted;

    UFUNCTION(BlueprintCallable, Category="Encounters")
    void ActivateEncounter(int32 EncounterID);

    UFUNCTION(BlueprintCallable, Category="Encounters")
    void EncounterCompleted();

    UFUNCTION(BlueprintCallable, Category="Encounters")
    void EnableTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category="Encounters")
    void DisableTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category="Encounters")
    void ResetEncounterProgress();

protected:
    UFUNCTION()
    void HandleTriggerOverlap(AActor* OverlappedActor, AActor* OtherActor);

    UFUNCTION()
    void HandleTargetDestroyed(AActor* DestroyedActor);

private:
    TSet<TObjectPtr<AActor>> BoundTargets;
    TSet<TObjectPtr<AActor>> CountedTargets;
    TSet<TObjectPtr<AActor>> BoundTriggers;
    bool bEncounterActive = false;
    bool bFinishingEncounter = false;

    void BindTriggers();
    void BindEncounterTargets();
    void NotifyTargetCount();
    void FinishEncounter(bool bSuccessful);
    FEncounter* FindEncounter(int32 EncounterID);
    APawn* GetPlayerPawn() const;
    bool IsPlayerActor(const AActor* Actor) const;
    void CallBlueprintInteract(AActor* Target, APawn* InteractorPawn) const;
    static bool InvokeNoArgumentFunction(UObject* Object, FName FunctionName);
};
