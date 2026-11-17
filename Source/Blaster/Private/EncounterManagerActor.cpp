#include "EncounterManagerActor.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

AEncounterManagerActor::AEncounterManagerActor()
{
    PrimaryActorTick.bCanEverTick = false;
    TargetEncounter.EncounterID = INDEX_NONE;
}

void AEncounterManagerActor::BeginPlay()
{
    Super::BeginPlay();
    BindTriggers();
    UE_LOG(LogTemp, Log, TEXT("[EncounterManager] %s ready: %d encounter entries, %d triggers"),
        *GetName(), LevelEncounters.Num(), EncounterTriggers.Num());
}

void AEncounterManagerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (AActor* Trigger : BoundTriggers)
    {
        if (IsValid(Trigger)) Trigger->OnActorBeginOverlap.RemoveDynamic(this, &AEncounterManagerActor::HandleTriggerOverlap);
    }
    for (AActor* Target : BoundTargets)
    {
        if (IsValid(Target)) Target->OnDestroyed.RemoveDynamic(this, &AEncounterManagerActor::HandleTargetDestroyed);
    }
    Super::EndPlay(EndPlayReason);
}

void AEncounterManagerActor::BindTriggers()
{
    for (const TPair<TObjectPtr<AActor>, int32>& Pair : EncounterTriggers)
    {
        AActor* Trigger = Pair.Key.Get();
        if (!IsValid(Trigger))
        {
            UE_LOG(LogTemp, Warning, TEXT("[EncounterManager] Null trigger for encounter %d"), Pair.Value);
            continue;
        }
        Trigger->OnActorBeginOverlap.AddUniqueDynamic(this, &AEncounterManagerActor::HandleTriggerOverlap);
        BoundTriggers.Add(Trigger);
    }
}

bool AEncounterManagerActor::IsPlayerActor(const AActor* Actor) const
{
    const APawn* Pawn = Cast<APawn>(Actor);
    if (!Pawn) return false;
    const AController* Controller = Pawn->GetController();
    return (Controller && Controller->IsPlayerController()) || Actor->ActorHasTag(TEXT("Player"));
}

APawn* AEncounterManagerActor::GetPlayerPawn() const
{
    if (!GetWorld()) return nullptr;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (APlayerController* PC = It->Get()) return PC->GetPawn();
    return nullptr;
}

void AEncounterManagerActor::HandleTriggerOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
    if (!IsValid(OverlappedActor) || !IsPlayerActor(OtherActor) || bEncounterActive) return;
    const int32* EncounterID = EncounterTriggers.Find(OverlappedActor);
    if (!EncounterID || CompletedEncounters.Contains(*EncounterID)) return;
    ActivateEncounter(*EncounterID);
    // Match the original trigger behaviour: prevent repeated activation.
    OverlappedActor->SetActorEnableCollision(false);
}

FEncounter* AEncounterManagerActor::FindEncounter(int32 EncounterID)
{
    return LevelEncounters.FindByPredicate([EncounterID](const FEncounter& Entry)
    {
        return Entry.EncounterID == EncounterID;
    });
}

void AEncounterManagerActor::ActivateEncounter(int32 EncounterID)
{
    if (bEncounterActive || CompletedEncounters.Contains(EncounterID)) return;
    FEncounter* Entry = FindEncounter(EncounterID);
    if (!Entry)
    {
        UE_LOG(LogTemp, Error, TEXT("[EncounterManager] No LevelEncounters entry for ID %d"), EncounterID);
        return;
    }

    TargetEncounter = *Entry;
    CurrentTargetCount = 0;
    bEncounterActive = true;
    bFinishingEncounter = false;
    BoundTargets.Reset();
    CountedTargets.Reset();

    APawn* PlayerPawn = GetPlayerPawn();
    for (AActor* Interactor : TargetEncounter.StartInteractors)
        CallBlueprintInteract(Interactor, PlayerPawn);

    for (AActor* Target : TargetEncounter.EncounterActors)
    {
        if (!IsValid(Target)) continue;
        BoundTargets.Add(Target);
        CountedTargets.Add(Target);
        Target->OnDestroyed.AddUniqueDynamic(this, &AEncounterManagerActor::HandleTargetDestroyed);
        EnableTarget(Target);
        // Preserve the original BPC_TargetComponent contract if it is still present.
        TArray<UActorComponent*> Components;
        Target->GetComponents(Components);
        for (UActorComponent* Component : Components)
        {
            if (Component && Component->GetClass()->GetName().Contains(TEXT("TargetComponent")))
                InvokeNoArgumentFunction(Component, TEXT("ActivateTarget"));
        }
        ++CurrentTargetCount;
    }

    NotifyTargetCount();
    EEncounterStarted.Broadcast(EncounterID);
    UE_LOG(LogTemp, Log, TEXT("[EncounterManager] Encounter %d started with %d targets"), EncounterID, CurrentTargetCount);
    if (CurrentTargetCount == 0) FinishEncounter(true);
}

void AEncounterManagerActor::HandleTargetDestroyed(AActor* DestroyedActor)
{
    if (!bEncounterActive || !CountedTargets.Remove(DestroyedActor)) return;
    DisableTarget(DestroyedActor);
    CurrentTargetCount = FMath::Max(0, CurrentTargetCount - 1);
    NotifyTargetCount();
    if (CurrentTargetCount == 0) FinishEncounter(true);
}

void AEncounterManagerActor::NotifyTargetCount()
{
    ETargetCountUpdated.Broadcast(CurrentTargetCount);
}

void AEncounterManagerActor::EncounterCompleted()
{
    FinishEncounter(true);
}

void AEncounterManagerActor::FinishEncounter(bool bSuccessful)
{
    if (!bEncounterActive || bFinishingEncounter) return;
    bFinishingEncounter = true;
    const int32 FinishedID = TargetEncounter.EncounterID;
    if (FinishedID != INDEX_NONE && !CompletedEncounters.Contains(FinishedID))
        CompletedEncounters.Add(FinishedID);

    CurrentTargetCount = 0;
    NotifyTargetCount();
    APawn* PlayerPawn = GetPlayerPawn();
    for (AActor* Interactor : TargetEncounter.EndInteractors)
        CallBlueprintInteract(Interactor, PlayerPawn);

    bEncounterActive = false;
    EEncounterEnded.Broadcast(FinishedID, bSuccessful);
    if (CompletedEncounters.Num() >= LevelEncounters.Num())
        EEncountersCompleted.Broadcast(FinishedID, bSuccessful);

    UE_LOG(LogTemp, Log, TEXT("[EncounterManager] Encounter %d completed (%d/%d)"),
        FinishedID, CompletedEncounters.Num(), LevelEncounters.Num());
    bFinishingEncounter = false;
}

void AEncounterManagerActor::EnableTarget(AActor* Target)
{
    if (!IsValid(Target)) return;
    Target->SetActorTickEnabled(true);
    Target->SetActorHiddenInGame(false);
    Target->SetActorEnableCollision(true);
}

void AEncounterManagerActor::DisableTarget(AActor* Target)
{
    if (!IsValid(Target)) return;
    Target->SetActorTickEnabled(false);
    Target->SetActorHiddenInGame(true);
    Target->SetActorEnableCollision(false);
}

void AEncounterManagerActor::CallBlueprintInteract(AActor* Target, APawn* InteractorPawn) const
{
    if (!IsValid(Target)) return;
    UFunction* Function = Target->FindFunction(FName(TEXT("Interact")));
    if (!Function)
    {
        // Blueprint interface implementations are commonly exposed as Execute_Interact.
        Function = Target->FindFunction(FName(TEXT("Execute_Interact")));
    }
    if (!Function) return;
    FStructOnScope Params(Function);
    if (FObjectProperty* PawnProperty = FindFProperty<FObjectProperty>(Function, TEXT("InteractorPawn")))
        PawnProperty->SetObjectPropertyValue_InContainer(Params.GetStructMemory(), InteractorPawn);
    Target->ProcessEvent(Function, Params.GetStructMemory());
}

bool AEncounterManagerActor::InvokeNoArgumentFunction(UObject* Object, FName FunctionName)
{
    if (!IsValid(Object)) return false;
    UFunction* Function = Object->FindFunction(FunctionName);
    if (!Function || Function->NumParms != 0) return false;
    Object->ProcessEvent(Function, nullptr);
    return true;
}

void AEncounterManagerActor::ResetEncounterProgress()
{
    for (AActor* Target : BoundTargets)
        if (IsValid(Target)) Target->OnDestroyed.RemoveDynamic(this, &AEncounterManagerActor::HandleTargetDestroyed);
    BoundTargets.Reset();
    CountedTargets.Reset();
    CompletedEncounters.Reset();
    CurrentTargetCount = 0;
    TargetEncounter = FEncounter();
    TargetEncounter.EncounterID = INDEX_NONE;
    bEncounterActive = false;
    bFinishingEncounter = false;
    NotifyTargetCount();
}
