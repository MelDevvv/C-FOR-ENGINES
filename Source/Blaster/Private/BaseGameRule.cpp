
#include "BaseGameRule.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

UBaseGameRule::UBaseGameRule()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UBaseGameRule::BeginPlay()
{
    Super::BeginPlay();

    // keep bpencounterrule parented to bpcbasegamerule bind its manager in c
    if (IsEncounterRuleBlueprint())
    {
        TryBindEncounterManager();

        if (!bEncounterCompletionBound)
        {
            SetComponentTickEnabled(true);
        }
    }
}

void UBaseGameRule::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (IsEncounterRuleBlueprint() && !bEncounterCompletionBound)
    {
        TryBindEncounterManager();

        if (bEncounterCompletionBound)
        {
            SetComponentTickEnabled(false);
        }
    }
}

void UBaseGameRule::InitRule_Implementation(int32 InRuleID)
{
    RuleID = InRuleID;
    Completed = false;

    // timer setup is independent of the encounter rule binding
    ClearNativeTimerHandles();
    bNativeTimerRule = false;
    bNativeTimerCompleted = false;

    if (HasBlueprintTimerContract())
    {
        bNativeTimerRule = true;

        const float InitialSeconds =
            FMath::Max(0.0f, ReadBlueprintTimerLength());

        // main oneshot timer
        if (InitialSeconds > 0.0f)
        {
            StartNativeTimer(InitialSeconds);
        }

        // onesecond repeating timer check
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                NativeRuleTickHandle,
                this,
                &UBaseGameRule::NativeTimerCheck,
                1.0f,
                true
            );
        }

        BroadcastBlueprintTimerDispatcher(TEXT("ETimerStarted"));

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Timer rule started: RuleID=%d Length=%.2f seconds; timer active=%s"),
            RuleID,
            InitialSeconds,
            (GetWorld() &&
             GetWorld()->GetTimerManager().IsTimerActive(NativeRuleTimerHandle))
                ? TEXT("true")
                : TEXT("false")
        );

        if (InitialSeconds <= 0.0f)
        {
            NativeTimerExpired();
        }
    }

    if (IsEncounterRuleBlueprint() && !bEncounterCompletionBound)
    {
        TryBindEncounterManager();
    }
}

void UBaseGameRule::CompleteRule_Implementation(bool bSuccessful)
{
    if (Completed)
    {
        return;
    }

    Completed = true;

    OnRuleUpdated.Broadcast(RuleID, Completed);
    ERuleUpdated.Broadcast(RuleID, Completed);

    OnRuleComplete.Broadcast(RuleID, bSuccessful);
    ERuleComplete.Broadcast(RuleID, bSuccessful);

    if (bNativeTimerRule)
    {
        bNativeTimerCompleted = true;
        ClearNativeTimerHandles();
    }
}

void UBaseGameRule::UpdateRuleConditions_Implementation()
{
    OnRuleUpdated.Broadcast(RuleID, Completed);
    ERuleUpdated.Broadcast(RuleID, Completed);
}

bool UBaseGameRule::HasBlueprintTimerContract() const
{
    const FFloatProperty* LengthProperty =
        FindFProperty<FFloatProperty>(
            GetClass(),
            TEXT("TimerLength")
        );

    const FMulticastDelegateProperty* StartedProperty =
        FindFProperty<FMulticastDelegateProperty>(
            GetClass(),
            TEXT("ETimerStarted")
        );

    const FMulticastDelegateProperty* TickedProperty =
        FindFProperty<FMulticastDelegateProperty>(
            GetClass(),
            TEXT("ETimerTicked")
        );

    const FMulticastDelegateProperty* CompletedProperty =
        FindFProperty<FMulticastDelegateProperty>(
            GetClass(),
            TEXT("ETimerCompleted")
        );

    if (!LengthProperty ||
        !StartedProperty ||
        !TickedProperty ||
        !CompletedProperty ||
        !TickedProperty->SignatureFunction)
    {
        return false;
    }

    // wbptimer uses a double parameter for etimerticked
    UFunction* Signature = TickedProperty->SignatureFunction.Get();

    int32 InputCount = 0;
    bool bHasDoubleInput = false;

    for (TFieldIterator<FProperty> It(Signature); It; ++It)
    {
        const FProperty* Property = *It;

        if (!Property->HasAnyPropertyFlags(CPF_Parm) ||
            Property->HasAnyPropertyFlags(CPF_ReturnParm))
        {
            continue;
        }

        ++InputCount;
        bHasDoubleInput =
            CastField<FDoubleProperty>(Property) != nullptr;
    }

    return InputCount == 1 && bHasDoubleInput;
}

float UBaseGameRule::ReadBlueprintTimerLength() const
{
    const FFloatProperty* Property =
        FindFProperty<FFloatProperty>(
            GetClass(),
            TEXT("TimerLength")
        );

    return Property
        ? Property->GetPropertyValue_InContainer(this)
        : 0.0f;
}

void UBaseGameRule::StartNativeTimer(float DurationSeconds)
{
    UWorld* World = GetWorld();

    if (!World ||
        !bNativeTimerRule ||
        bNativeTimerCompleted ||
        Completed)
    {
        return;
    }

    World->GetTimerManager().ClearTimer(NativeRuleTimerHandle);

    if (DurationSeconds <= 0.0f)
    {
        NativeTimerExpired();
        return;
    }

    World->GetTimerManager().SetTimer(
        NativeRuleTimerHandle,
        this,
        &UBaseGameRule::NativeTimerExpired,
        DurationSeconds,
        false
    );
}

void UBaseGameRule::NativeTimerCheck()
{
    if (!bNativeTimerRule ||
        bNativeTimerCompleted ||
        Completed)
    {
        return;
    }

    const float Remaining = GetTimerSecondsRemaining();

    if (Remaining > 0.0f)
    {
        // send remaining seconds as a double to wbptimer
        BroadcastBlueprintTimerDispatcher(
            TEXT("ETimerTicked"),
            true,
            static_cast<double>(Remaining)
        );

        return;
    }

    if (UWorld* World = GetWorld())
    {
        if (!World->GetTimerManager().IsTimerActive(NativeRuleTimerHandle))
        {
            NativeTimerExpired();
        }
    }
}

void UBaseGameRule::NativeTimerExpired()
{
    if (!bNativeTimerRule ||
        bNativeTimerCompleted ||
        Completed)
    {
        return;
    }

    bNativeTimerCompleted = true;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(NativeRuleTickHandle);
        World->GetTimerManager().ClearTimer(NativeRuleTimerHandle);
    }

    // wbptimer updates its display to zero
    BroadcastBlueprintTimerDispatcher(TEXT("ETimerCompleted"));

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Timer expired. Completing timer rule %d unsuccessfully."),
        RuleID
    );

    // gamemode failure path restarts the level
    CompleteRule(false);
}

void UBaseGameRule::AddToTimer(float SecondsToAdd)
{
    if (!bNativeTimerRule ||
        bNativeTimerCompleted ||
        Completed)
    {
        return;
    }

    const float Remaining = GetTimerSecondsRemaining();

    if (Remaining > 0.0f)
    {
        StartNativeTimer(Remaining + SecondsToAdd);
    }
}

void UBaseGameRule::DeductFromTimer(float SecondsToRemove)
{
    if (!bNativeTimerRule ||
        bNativeTimerCompleted ||
        Completed)
    {
        return;
    }

    const float Remaining = GetTimerSecondsRemaining();

    if (Remaining > 0.0f)
    {
        const float NewRemaining = Remaining - SecondsToRemove;

        if (NewRemaining <= 0.0f)
        {
            NativeTimerExpired();
        }
        else
        {
            StartNativeTimer(NewRemaining);
        }
    }
}

float UBaseGameRule::GetTimerSecondsRemaining() const
{
    const UWorld* World = GetWorld();

    if (!bNativeTimerRule ||
        bNativeTimerCompleted ||
        !World ||
        !World->GetTimerManager().IsTimerActive(NativeRuleTimerHandle))
    {
        return 0.0f;
    }

    return FMath::Max(
        0.0f,
        World->GetTimerManager().GetTimerRemaining(NativeRuleTimerHandle)
    );
}

bool UBaseGameRule::BroadcastBlueprintTimerDispatcher(
    FName DispatcherName,
    bool bHasDoubleParameter,
    double DoubleParameter)
{
    FMulticastDelegateProperty* DelegateProperty =
        FindFProperty<FMulticastDelegateProperty>(
            GetClass(),
            DispatcherName
        );

    if (!DelegateProperty || !DelegateProperty->SignatureFunction)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Timer dispatcher '%s' not found on %s."),
            *DispatcherName.ToString(),
            *GetClass()->GetName()
        );

        return false;
    }

    UFunction* Signature = DelegateProperty->SignatureFunction.Get();
    FStructOnScope Parameters(Signature);

    if (bHasDoubleParameter)
    {
        bool bAssigned = false;

        for (TFieldIterator<FProperty> It(Signature); It; ++It)
        {
            FProperty* Parameter = *It;

            if (!Parameter->HasAnyPropertyFlags(CPF_Parm) ||
                Parameter->HasAnyPropertyFlags(CPF_ReturnParm))
            {
                continue;
            }

            if (FDoubleProperty* DoubleProperty =
                CastField<FDoubleProperty>(Parameter))
            {
                DoubleProperty->SetPropertyValue_InContainer(
                    Parameters.GetStructMemory(),
                    DoubleParameter
                );

                bAssigned = true;
                break;
            }
        }

        if (!bAssigned)
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT("ETimerTicked on %s does not expose the expected double parameter."),
                *GetClass()->GetName()
            );

            return false;
        }
    }

    const void* ValueAddress =
        DelegateProperty->ContainerPtrToValuePtr<void>(this);

    const FMulticastScriptDelegate* Delegate =
        DelegateProperty->GetMulticastDelegate(ValueAddress);

    if (!Delegate)
    {
        return false;
    }

    // ue 58 invoke the multicast script delegate through processdelegate
    Delegate->ProcessDelegate<UObject>(
        Parameters.GetStructMemory()
    );

    return true;
}

void UBaseGameRule::ClearNativeTimerHandles()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(NativeRuleTimerHandle);
        World->GetTimerManager().ClearTimer(NativeRuleTickHandle);
    }
}

bool UBaseGameRule::IsEncounterRuleBlueprint() const
{
    if (!GetClass())
    {
        return false;
    }

    const FString Path = GetClass()->GetPathName();
    const FString Name = GetClass()->GetName();

    return Path.Contains(
        TEXT("/Game/Scripts/Gamemodes/Rules/BPC_EncounterRule")
    ) || Name.StartsWith(TEXT("BPC_EncounterRule_C"));
}

AActor* UBaseGameRule::FindBlueprintEncounterManager() const
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Candidate = *It;

        if (!IsValid(Candidate))
        {
            continue;
        }

        const FString Path = Candidate->GetClass()->GetPathName();
        const FString Name = Candidate->GetClass()->GetName();

        if (Path.Contains(
                TEXT("/Game/Scripts/Gameplay/Managers/BP_EncounterManager")
            ) ||
            Name.StartsWith(TEXT("BP_EncounterManager_C")))
        {
            return Candidate;
        }
    }

    return nullptr;
}

bool UBaseGameRule::BindEncounterManager(AActor* Manager)
{
    if (!IsValid(Manager))
    {
        return false;
    }

    FMulticastDelegateProperty* Property =
        FindFProperty<FMulticastDelegateProperty>(
            Manager->GetClass(),
            TEXT("EEncountersCompleted")
        );

    if (!Property)
    {
        Property = FindFProperty<FMulticastDelegateProperty>(
            Manager->GetClass(),
            TEXT("OnNativeAllEncountersCompleted")
        );
    }

    if (!Property)
    {
        return false;
    }

    EncounterCompletionDelegate.BindUFunction(
        this,
        GET_FUNCTION_NAME_CHECKED(
            UBaseGameRule,
            HandleEncounterCompletion
        )
    );

    void* ValueAddress = Property->ContainerPtrToValuePtr<void>(Manager);

    const FMulticastScriptDelegate* Existing =
        Property->GetMulticastDelegate(ValueAddress);

    FMulticastScriptDelegate* Mutable =
        const_cast<FMulticastScriptDelegate*>(Existing);

    if (!Mutable)
    {
        EncounterCompletionDelegate.Unbind();
        return false;
    }

    Mutable->AddUnique(EncounterCompletionDelegate);

    EncounterManager = Manager;
    EncounterCompletionProperty = Property;
    bEncounterCompletionBound = true;

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Encounter rule %d bound to encounter manager %s."),
        RuleID,
        *Manager->GetName()
    );

    return true;
}

void UBaseGameRule::TryBindEncounterManager()
{
    if (bEncounterCompletionBound)
    {
        return;
    }

    if (AActor* Manager = FindBlueprintEncounterManager())
    {
        BindEncounterManager(Manager);
    }
}

void UBaseGameRule::UnbindEncounterManager()
{
    if (bEncounterCompletionBound &&
        IsValid(EncounterManager) &&
        EncounterCompletionProperty)
    {
        void* ValueAddress =
            EncounterCompletionProperty->ContainerPtrToValuePtr<void>(
                EncounterManager
            );

        const FMulticastScriptDelegate* Existing =
            EncounterCompletionProperty->GetMulticastDelegate(ValueAddress);

        FMulticastScriptDelegate* Mutable =
            const_cast<FMulticastScriptDelegate*>(Existing);

        if (Mutable)
        {
            Mutable->Remove(EncounterCompletionDelegate);
        }
    }

    bEncounterCompletionBound = false;
    EncounterCompletionProperty = nullptr;
    EncounterCompletionDelegate.Unbind();
    EncounterManager = nullptr;
}

void UBaseGameRule::HandleEncounterCompletion(
    int32 CompletedRuleID,
    bool bSuccessful)
{
    ERuleComplete.Broadcast(CompletedRuleID, bSuccessful);
    OnRuleComplete.Broadcast(CompletedRuleID, bSuccessful);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Encounter rule completed. ID=%d Success=%s"),
        CompletedRuleID,
        bSuccessful ? TEXT("true") : TEXT("false")
    );
}

void UBaseGameRule::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    ClearNativeTimerHandles();

    bNativeTimerRule = false;
    bNativeTimerCompleted = false;

    UnbindEncounterManager();

    Super::EndPlay(EndPlayReason);
}
