#include "EncounterGameRule.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"

void UEncounterGameRule::InitRule_Implementation(int32 InRuleID)
{
    Super::InitRule_Implementation(InRuleID);
    UnbindManagerCompletion();
    EncounterManager = FindBlueprintEncounterManager();

    if (!IsValid(EncounterManager))
    {
        UE_LOG(LogTemp, Error, TEXT("EncounterGameRule %d: BP_EncounterManager was not found in this world."), InRuleID);
        return;
    }

    if (!BindToManagerCompletion(EncounterManager))
    {
        UE_LOG(LogTemp, Error, TEXT("EncounterGameRule %d: %s does not expose a compatible EEncountersCompleted dispatcher."),
            InRuleID, *EncounterManager->GetName());
        EncounterManager = nullptr;
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("EncounterGameRule %d bound to EEncountersCompleted on %s."),
        InRuleID, *EncounterManager->GetName());
}

AActor* UEncounterGameRule::FindBlueprintEncounterManager() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // Match the original Blueprint GetActorOfClass(BP_EncounterManager).
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Candidate = *It;
        if (!IsValid(Candidate))
        {
            continue;
        }

        const FString ClassPath = Candidate->GetClass()->GetPathName();
        const FString ClassName = Candidate->GetClass()->GetName();
        if (ClassPath.Contains(TEXT("/Game/Scripts/Gameplay/Managers/BP_EncounterManager")) ||
            ClassName.StartsWith(TEXT("BP_EncounterManager_C")))
        {
            return Candidate;
        }
    }

    return nullptr;
}

bool UEncounterGameRule::BindToManagerCompletion(AActor* Manager)
{
    if (!IsValid(Manager))
    {
        return false;
    }

    FMulticastDelegateProperty* DelegateProperty =
        FindFProperty<FMulticastDelegateProperty>(Manager->GetClass(), TEXT("EEncountersCompleted"));

    // Also support the native manager event name if the manager was converted separately.
    if (!DelegateProperty)
    {
        DelegateProperty = FindFProperty<FMulticastDelegateProperty>(Manager->GetClass(), TEXT("OnNativeAllEncountersCompleted"));
    }

    if (!DelegateProperty)
    {
        return false;
    }

    BoundCompletionDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UEncounterGameRule, HandleAllEncountersCompleted));
    void* ValueAddress = DelegateProperty->ContainerPtrToValuePtr<void>(Manager);
    const FMulticastScriptDelegate* Existing = DelegateProperty->GetMulticastDelegate(ValueAddress);
    FMulticastScriptDelegate* Mutable = const_cast<FMulticastScriptDelegate*>(Existing);
    if (!Mutable)
    {
        BoundCompletionDelegate.Unbind();
        return false;
    }

    Mutable->AddUnique(BoundCompletionDelegate);
    BoundCompletionProperty = DelegateProperty;
    bCompletionDelegateBound = true;
    return true;
}

void UEncounterGameRule::UnbindManagerCompletion()
{
    if (bCompletionDelegateBound && IsValid(EncounterManager) && BoundCompletionProperty)
    {
        void* ValueAddress = BoundCompletionProperty->ContainerPtrToValuePtr<void>(EncounterManager);
        const FMulticastScriptDelegate* Existing = BoundCompletionProperty->GetMulticastDelegate(ValueAddress);
        FMulticastScriptDelegate* Mutable = const_cast<FMulticastScriptDelegate*>(Existing);
        if (Mutable)
        {
            Mutable->Remove(BoundCompletionDelegate);
        }
    }

    bCompletionDelegateBound = false;
    BoundCompletionProperty = nullptr;
    BoundCompletionDelegate.Unbind();
}

void UEncounterGameRule::HandleAllEncountersCompleted(int32 CompletedRuleID, bool bSuccessful)
{
    // Exact equivalent of the Blueprint's ERuleComplete.Broadcast(RuleID, Successful).
    ERuleComplete.Broadcast(CompletedRuleID, bSuccessful);
    UE_LOG(LogTemp, Log, TEXT("EncounterGameRule: all encounters completed. ID=%d Success=%s"),
        CompletedRuleID, bSuccessful ? TEXT("true") : TEXT("false"));
}

void UEncounterGameRule::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindManagerCompletion();
    EncounterManager = nullptr;
    Super::EndPlay(EndPlayReason);
}
