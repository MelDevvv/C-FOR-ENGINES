#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameStructs.h"
#include "EncounterManagerActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEncounterStartedNative, int32, TotalTargets);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTargetCountUpdatedNative, int32, TargetCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEncounterEndedNative);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEncountersCompletedNative, int32, RuleID, bool, Successful);

UCLASS(Blueprintable)
class BLASTER_API AEncounterManagerActor : public AActor
{
    GENERATED_BODY()
public:
    AEncounterManagerActor();
    virtual void BeginPlay() override;

    // keep the original blueprint property namestypes for migration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Encounter")
    TArray<FEncounter> LevelEncounters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Encounter")
    TMap<TObjectPtr<AActor>, int32> EncounterTriggers;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Encounter")
    int32 CurrentTargetCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Encounter")
    TArray<int32> CompletedEncounters;

    UPROPERTY(BlueprintAssignable, Category="Encounter|Events")
    FEncounterStartedNative EEncounterStarted;
    UPROPERTY(BlueprintAssignable, Category="Encounter|Events")
    FTargetCountUpdatedNative ETargetCountUpdated;
    UPROPERTY(BlueprintAssignable, Category="Encounter|Events")
    FEncounterEndedNative EEncounterEnded;
    UPROPERTY(BlueprintAssignable, Category="Encounter|Events")
    FEncountersCompletedNative EEncountersCompleted;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Encounter")
    FEncounter TargetEncounter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Encounter")
    FEncounter TestEncounter;

    UFUNCTION(BlueprintCallable, Category="Encounter") void ActivateEncounter(int32 EncounterID);
    UFUNCTION(BlueprintCallable, Category="Encounter") void EncounterCompleted();
    UFUNCTION(BlueprintCallable, Category="Encounter") void EnableTarget(AActor* TargetActor);
    UFUNCTION(BlueprintCallable, Category="Encounter") void DisableTarget(AActor* TargetActor);

protected:
    UFUNCTION() void HandleTriggerOverlap(AActor* OverlappedActor, AActor* OtherActor);
    UFUNCTION() void HandleTargetDestroyed(AActor* DestroyedActor);

private:
    TSet<TObjectPtr<AActor>> BoundTargets;
    TWeakObjectPtr<AActor> CurrentInteractor;
    bool bEncounterActive = false;
    void DiscoverAndBuildEncounterActors();
    void AutoAssignDoorEndInteractors();
    bool IsInsideTriggerVolume(const AActor* Candidate, const AActor* Trigger) const;
    bool HasTargetComponent(const AActor* Actor) const;
    FEncounter* FindEncounter(int32 ID);
    void InvokeInteract(AActor* Actor, AActor* InteractorPawn);
    void UpdateCount();
    void ActivateFinalEndpoint();
};
