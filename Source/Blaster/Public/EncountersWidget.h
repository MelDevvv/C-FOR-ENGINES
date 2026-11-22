#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EncountersWidget.generated.h"

class AEncounterManagerActor;
class UTextBlock;

UCLASS()
class BLASTER_API UEncountersWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    // finds the existing designer textblock named tencounter
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> EncounterTextBlock = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<AEncounterManagerActor> EncounterManager = nullptr;

    int32 TotalTargets = 0;
    int32 CurrentTargets = 0;

    FTimerHandle ManagerLookupTimer;
    FTimerHandle EncounterEndedTimer;

    void CacheEncounterTextBlock();
    void FindAndBindEncounterManager();
    void UpdateEncounterText();

    UFUNCTION()
    void HandleEncounterStarted(int32 InTotalTargets);

    UFUNCTION()
    void HandleTargetCountUpdated(int32 InTargetCount);

    UFUNCTION()
    void HandleEncounterEnded();

    void HideEncounterWidget();
};
