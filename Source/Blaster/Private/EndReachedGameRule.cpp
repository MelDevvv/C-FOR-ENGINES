#include "EndReachedGameRule.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"

void UEndReachedGameRule::InitRule_Implementation(int32 InRuleID)
{
    // Preserve the parent rule initialization and RuleID setup.
    Super::InitRule_Implementation(InRuleID);

    // Avoid duplicate bindings if InitRule is called again.
    UnbindEndPoint();

    AActor* FoundEndPoint = FindBlueprintEndPoint();
    if (!IsValid(FoundEndPoint))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("End-reached rule %d could not find BP_EndPoint in the current level."),
            RuleID
        );
        return;
    }

    if (!BindEndPoint(FoundEndPoint))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("End-reached rule %d found %s but could not bind EEndReached."),
            RuleID,
            *FoundEndPoint->GetName()
        );
    }
}

AActor* UEndReachedGameRule::FindBlueprintEndPoint() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // Matches the original Blueprint GetActorOfClass(BP_EndPoint).
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Candidate = *It;
        if (!IsValid(Candidate) || !Candidate->GetClass())
        {
            continue;
        }

        const FString ClassPath = Candidate->GetClass()->GetPathName();
        const FString ClassName = Candidate->GetClass()->GetName();

        if (ClassPath.Contains(
                TEXT("/Game/Scripts/Gameplay/Objectives/BP_EndPoint")
            ) || ClassName.StartsWith(TEXT("BP_EndPoint_C")))
        {
            return Candidate;
        }
    }

    return nullptr;
}

bool UEndReachedGameRule::BindEndPoint(AActor* EndPointActor)
{
    if (!IsValid(EndPointActor))
    {
        return false;
    }

    FMulticastDelegateProperty* DelegateProperty =
        FindFProperty<FMulticastDelegateProperty>(
            EndPointActor->GetClass(),
            TEXT("EEndReached")
        );

    if (!DelegateProperty || !DelegateProperty->SignatureFunction)
    {
        return false;
    }

    // The Blueprint custom event OnEndReached has no input parameters.
    EndReachedDelegate.BindUFunction(
        this,
        GET_FUNCTION_NAME_CHECKED(UEndReachedGameRule, HandleEndReached)
    );

    void* ValueAddress =
        DelegateProperty->ContainerPtrToValuePtr<void>(EndPointActor);

    const FMulticastScriptDelegate* ExistingDelegate =
        DelegateProperty->GetMulticastDelegate(ValueAddress);

    FMulticastScriptDelegate* MutableDelegate =
        const_cast<FMulticastScriptDelegate*>(ExistingDelegate);

    if (!MutableDelegate)
    {
        EndReachedDelegate.Unbind();
        return false;
    }

    MutableDelegate->AddUnique(EndReachedDelegate);

    CachedEndPoint = EndPointActor;
    EndReachedProperty = DelegateProperty;
    bEndReachedBound = true;

    UE_LOG(
        LogTemp,
        Log,
        TEXT("End-reached rule %d bound to endpoint %s."),
        RuleID,
        *EndPointActor->GetName()
    );

    return true;
}

void UEndReachedGameRule::HandleEndReached()
{
    if (Completed)
    {
        return;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("End point reached. Completing rule %d successfully."),
        RuleID
    );

    // Matches the original Blueprint CompleteRule node with Successful = true.
    CompleteRule(true);
}

void UEndReachedGameRule::UnbindEndPoint()
{
    if (bEndReachedBound && IsValid(CachedEndPoint) && EndReachedProperty)
    {
        void* ValueAddress =
            EndReachedProperty->ContainerPtrToValuePtr<void>(CachedEndPoint);

        const FMulticastScriptDelegate* ExistingDelegate =
            EndReachedProperty->GetMulticastDelegate(ValueAddress);

        FMulticastScriptDelegate* MutableDelegate =
            const_cast<FMulticastScriptDelegate*>(ExistingDelegate);

        if (MutableDelegate)
        {
            MutableDelegate->Remove(EndReachedDelegate);
        }
    }

    bEndReachedBound = false;
    EndReachedProperty = nullptr;
    EndReachedDelegate.Unbind();
    CachedEndPoint = nullptr;
}

void UEndReachedGameRule::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    UnbindEndPoint();
    Super::EndPlay(EndPlayReason);
}
