#include "EndReachedWidget.h"

#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"

namespace
{
    const FName EndActivatedName(TEXT("EEndActivated"));
    const FName EncountersCompletedName(TEXT("EEncountersCompleted"));
}

void UEndReachedWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (T_Escape)
    {
        T_Escape->SetText(FText::FromString(TEXT("FIND THE EXIT  >")));
        T_Escape->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f,1.0f,0.55f,1.0f)));
        FSlateFontInfo ExitFont = T_Escape->GetFont(); ExitFont.Size = 22; T_Escape->SetFont(ExitFont);
        T_Escape->SetShadowOffset(FVector2D(2.0f,2.0f)); T_Escape->SetShadowColorAndOpacity(FLinearColor(0.0f,0.0f,0.0f,0.8f));
    }

    // It is hidden until the encounter manager confirms the final encounter is complete.
    SetVisibility(ESlateVisibility::Collapsed);

    CachedEndPointActor = FindEndPointActor();
    if (IsValid(CachedEndPointActor))
    {
        if (ReadEndPointAutoActivate())
        {
            ShowFindExit();
        }
        else
        {
            BindEndActivated();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("WBP_EndReached: BP_EndPoint actor was not found. The widget will still listen for encounter completion."));
    }

    CachedEncounterManager = FindEncounterManagerActor();
    if (IsValid(CachedEncounterManager))
    {
        BindEncountersCompleted();

        // Covers the case where this widget is created after the final encounter event.
        if (HasManagerAlreadyCompletedAllEncounters(CachedEncounterManager))
        {
            ShowFindExit();
        }
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("WBP_EndReached: Could not find an actor exposing EEncountersCompleted. Find the Exit cannot be triggered by encounter completion."));
    }
}

void UEndReachedWidget::NativeDestruct()
{
    UnbindEndActivated();
    UnbindEncountersCompleted();
    CachedEndPointActor = nullptr;
    CachedEncounterManager = nullptr;
    Super::NativeDestruct();
}

AActor* UEndReachedWidget::FindEndPointActor() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Candidate = *It;
        if (!IsValid(Candidate) || !Candidate->GetClass())
        {
            continue;
        }

        const FString ClassPath = Candidate->GetClass()->GetPathName();
        const FString ClassName = Candidate->GetClass()->GetName();
        if (ClassPath.Contains(TEXT("/BP_EndPoint.")) ||
            ClassName.StartsWith(TEXT("BP_EndPoint_C")))
        {
            return Candidate;
        }
    }

    return nullptr;
}

AActor* UEndReachedWidget::FindEncounterManagerActor() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Candidate = *It;
        if (!IsValid(Candidate) || !Candidate->GetClass())
        {
            continue;
        }

        // Match the native manager and Blueprint manager by the dispatcher they expose.
        if (FindFProperty<FMulticastDelegateProperty>(Candidate->GetClass(), EncountersCompletedName))
        {
            return Candidate;
        }
    }

    return nullptr;
}

bool UEndReachedWidget::ReadEndPointAutoActivate() const
{
    if (!IsValid(CachedEndPointActor))
    {
        return false;
    }

    // The actual Blueprint variable is named bAutoActivate.
    const FBoolProperty* Property = FindFProperty<FBoolProperty>(
        CachedEndPointActor->GetClass(), TEXT("bAutoActivate"));

    return Property && Property->GetPropertyValue_InContainer(CachedEndPointActor);
}

bool UEndReachedWidget::HasManagerAlreadyCompletedAllEncounters(AActor* Manager) const
{
    if (!IsValid(Manager))
    {
        return false;
    }

    const FArrayProperty* CompletedProperty = FindFProperty<FArrayProperty>(
        Manager->GetClass(), TEXT("CompletedEncounters"));
    const FArrayProperty* TotalProperty = FindFProperty<FArrayProperty>(
        Manager->GetClass(), TEXT("LevelEncounters"));

    if (!CompletedProperty || !TotalProperty)
    {
        return false;
    }

    const void* CompletedAddress = CompletedProperty->ContainerPtrToValuePtr<void>(Manager);
    const void* TotalAddress = TotalProperty->ContainerPtrToValuePtr<void>(Manager);
    const FScriptArrayHelper CompletedHelper(CompletedProperty, CompletedAddress);
    const FScriptArrayHelper TotalHelper(TotalProperty, TotalAddress);

    return TotalHelper.Num() > 0 && CompletedHelper.Num() >= TotalHelper.Num();
}

bool UEndReachedWidget::BindEndActivated()
{
    if (!IsValid(CachedEndPointActor))
    {
        return false;
    }

    FMulticastDelegateProperty* Property = FindFProperty<FMulticastDelegateProperty>(
        CachedEndPointActor->GetClass(), EndActivatedName);
    if (!Property)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_EndReached: BP_EndPoint has no EEndActivated dispatcher."));
        return false;
    }

    BoundEndActivatedDelegate.BindUFunction(
        this, GET_FUNCTION_NAME_CHECKED(UEndReachedWidget, HandleEndActivated));

    void* Address = Property->ContainerPtrToValuePtr<void>(CachedEndPointActor);
    const FMulticastScriptDelegate* Existing = Property->GetMulticastDelegate(Address);
    FMulticastScriptDelegate* Mutable = const_cast<FMulticastScriptDelegate*>(Existing);
    if (!Mutable)
    {
        BoundEndActivatedDelegate.Unbind();
        return false;
    }

    Mutable->AddUnique(BoundEndActivatedDelegate);
    BoundEndActivatedProperty = Property;
    return true;
}

bool UEndReachedWidget::BindEncountersCompleted()
{
    if (!IsValid(CachedEncounterManager))
    {
        return false;
    }

    FMulticastDelegateProperty* Property = FindFProperty<FMulticastDelegateProperty>(
        CachedEncounterManager->GetClass(), EncountersCompletedName);
    if (!Property || !Property->SignatureFunction)
    {
        return false;
    }

    // The converted manager dispatcher is EEncountersCompleted(int32, bool).
    int32 InputCount = 0;
    bool bHasInt = false;
    bool bHasBool = false;
    for (TFieldIterator<FProperty> It(Property->SignatureFunction); It; ++It)
    {
        FProperty* Parameter = *It;
        if (!Parameter->HasAnyPropertyFlags(CPF_Parm) ||
            Parameter->HasAnyPropertyFlags(CPF_ReturnParm))
        {
            continue;
        }
        ++InputCount;
        bHasInt |= CastField<FIntProperty>(Parameter) != nullptr;
        bHasBool |= CastField<FBoolProperty>(Parameter) != nullptr;
    }

    if (InputCount != 2 || !bHasInt || !bHasBool)
    {
        UE_LOG(LogTemp, Error,
            TEXT("WBP_EndReached: EEncountersCompleted has an unexpected signature on %s."),
            *GetNameSafe(CachedEncounterManager));
        return false;
    }

    BoundEncountersCompletedDelegate.BindUFunction(
        this, GET_FUNCTION_NAME_CHECKED(UEndReachedWidget, HandleEncountersCompleted));

    void* Address = Property->ContainerPtrToValuePtr<void>(CachedEncounterManager);
    const FMulticastScriptDelegate* Existing = Property->GetMulticastDelegate(Address);
    FMulticastScriptDelegate* Mutable = const_cast<FMulticastScriptDelegate*>(Existing);
    if (!Mutable)
    {
        BoundEncountersCompletedDelegate.Unbind();
        return false;
    }

    Mutable->AddUnique(BoundEncountersCompletedDelegate);
    BoundEncountersCompletedProperty = Property;
    UE_LOG(LogTemp, Log,
        TEXT("WBP_EndReached: bound to EEncountersCompleted on %s."),
        *GetNameSafe(CachedEncounterManager));
    return true;
}

void UEndReachedWidget::UnbindEndActivated()
{
    if (IsValid(CachedEndPointActor) && BoundEndActivatedProperty &&
        BoundEndActivatedDelegate.IsBound())
    {
        void* Address = BoundEndActivatedProperty->ContainerPtrToValuePtr<void>(CachedEndPointActor);
        const FMulticastScriptDelegate* Existing =
            BoundEndActivatedProperty->GetMulticastDelegate(Address);
        FMulticastScriptDelegate* Mutable = const_cast<FMulticastScriptDelegate*>(Existing);
        if (Mutable)
        {
            Mutable->Remove(BoundEndActivatedDelegate);
        }
    }

    BoundEndActivatedDelegate.Unbind();
    BoundEndActivatedProperty = nullptr;
}

void UEndReachedWidget::UnbindEncountersCompleted()
{
    if (IsValid(CachedEncounterManager) && BoundEncountersCompletedProperty &&
        BoundEncountersCompletedDelegate.IsBound())
    {
        void* Address = BoundEncountersCompletedProperty->ContainerPtrToValuePtr<void>(CachedEncounterManager);
        const FMulticastScriptDelegate* Existing =
            BoundEncountersCompletedProperty->GetMulticastDelegate(Address);
        FMulticastScriptDelegate* Mutable = const_cast<FMulticastScriptDelegate*>(Existing);
        if (Mutable)
        {
            Mutable->Remove(BoundEncountersCompletedDelegate);
        }
    }

    BoundEncountersCompletedDelegate.Unbind();
    BoundEncountersCompletedProperty = nullptr;
}

void UEndReachedWidget::HandleEndActivated()
{
    ShowFindExit();
}

void UEndReachedWidget::HandleEncountersCompleted(int32 CompletedEncounterID, bool bSuccessful)
{
    if (bSuccessful)
    {
        UE_LOG(LogTemp, Log,
            TEXT("WBP_EndReached: all encounters completed; showing Find the Exit."));
        ShowFindExit();
    }
}

void UEndReachedWidget::ShowFindExit()
{
    if (T_Escape)
    {
        T_Escape->SetText(FText::FromString(TEXT("FIND THE EXIT  >")));
        T_Escape->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f,1.0f,0.55f,1.0f)));
        FSlateFontInfo ExitFont = T_Escape->GetFont(); ExitFont.Size = 22; T_Escape->SetFont(ExitFont);
        T_Escape->SetShadowOffset(FVector2D(2.0f,2.0f)); T_Escape->SetShadowColorAndOpacity(FLinearColor(0.0f,0.0f,0.0f,0.8f));
    }
    SetVisibility(ESlateVisibility::HitTestInvisible);
}
