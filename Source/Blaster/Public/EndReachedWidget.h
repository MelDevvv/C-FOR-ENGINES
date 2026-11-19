#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EndReachedWidget.generated.h"

class AActor;
class UTextBlock;
class FMulticastDelegateProperty;

/** Native behaviour for WBP_EndReached. Keep the existing Widget Blueprint Designer layout. */
UCLASS(Blueprintable)
class BLASTER_API UEndReachedWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

protected:
    /** Existing Designer Text Block; its name must remain T_Escape. */
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> T_Escape;

    UFUNCTION()
    void HandleEndActivated();

    /** Matches AEncounterManagerActor::EEncountersCompleted(int32, bool). */
    UFUNCTION()
    void HandleEncountersCompleted(int32 CompletedEncounterID, bool bSuccessful);

private:
    UPROPERTY(Transient)
    TObjectPtr<AActor> CachedEndPointActor = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<AActor> CachedEncounterManager = nullptr;

    FScriptDelegate BoundEndActivatedDelegate;
    FMulticastDelegateProperty* BoundEndActivatedProperty = nullptr;

    FScriptDelegate BoundEncountersCompletedDelegate;
    FMulticastDelegateProperty* BoundEncountersCompletedProperty = nullptr;

    AActor* FindEndPointActor() const;
    AActor* FindEncounterManagerActor() const;

    bool ReadEndPointAutoActivate() const;
    bool HasManagerAlreadyCompletedAllEncounters(AActor* Manager) const;

    bool BindEndActivated();
    bool BindEncountersCompleted();
    void UnbindEndActivated();
    void UnbindEncountersCompleted();

    void ShowFindExit();
};
