#include "EncounterManagerActor.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/ActorComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/TriggerBox.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"

AEncounterManagerActor::AEncounterManagerActor()
{
    PrimaryActorTick.bCanEverTick = false;
    TargetEncounter.EncounterID = INDEX_NONE;
}

void AEncounterManagerActor::BeginPlay()
{
    Super::BeginPlay();

    // the exported blueprint had three trigger mappings ids 0 1 2 while
    // the migrated struct entries can retain duplicate default ids normalize
    // entries by their encounter index only when duplicate ids are detected
    TSet<int32> SeenIDs;
    bool bDuplicateIDs = false;
    for (const FEncounter& Entry : LevelEncounters)
    {
        if (SeenIDs.Contains(Entry.EncounterID)) { bDuplicateIDs = true; break; }
        SeenIDs.Add(Entry.EncounterID);
    }
    if (bDuplicateIDs && LevelEncounters.Num() == EncounterTriggers.Num())
    {
        for (int32 Index = 0; Index < LevelEncounters.Num(); ++Index)
            LevelEncounters[Index].EncounterID = Index;
        UE_LOG(LogTemp, Warning, TEXT("EncounterManager: normalized duplicate encounter IDs to array indices 0..%d"), LevelEncounters.Num()-1);
    }

    DiscoverAndBuildEncounterActors();
    AutoAssignDoorEndInteractors();

    for (const TPair<TObjectPtr<AActor>, int32>& Pair : EncounterTriggers)
    {
        if (AActor* Trigger = Pair.Key.Get())
            Trigger->OnActorBeginOverlap.AddUniqueDynamic(this, &AEncounterManagerActor::HandleTriggerOverlap);
        else
            UE_LOG(LogTemp, Error, TEXT("EncounterManager: null trigger reference for ID %d"), Pair.Value);
    }

    UE_LOG(LogTemp, Log, TEXT("EncounterManager ready: %d encounter definitions, %d trigger mappings"), LevelEncounters.Num(), EncounterTriggers.Num());
    for (const FEncounter& E : LevelEncounters)
        UE_LOG(LogTemp, Log, TEXT("Encounter %d has %d discovered targets"), E.EncounterID, E.EncounterActors.Num());
}

bool AEncounterManagerActor::HasTargetComponent(const AActor* Actor) const
{
    if (!IsValid(Actor) || Actor == this) return false;
    TArray<UActorComponent*> Components;
    Actor->GetComponents<UActorComponent>(Components);
    for (const UActorComponent* Component : Components)
    {
        if (!IsValid(Component)) continue;
        const FString ComponentName = Component->GetName();
        const FString ClassName = Component->GetClass()->GetName();
        if (ComponentName.Contains(TEXT("TargetComponent"), ESearchCase::IgnoreCase) ||
            ClassName.Contains(TEXT("BPC_TargetComponent"), ESearchCase::IgnoreCase) ||
            ClassName.Contains(TEXT("TargetComponent"), ESearchCase::IgnoreCase))
            return true;
    }
    return false;
}

bool AEncounterManagerActor::IsInsideTriggerVolume(const AActor* Candidate, const AActor* Trigger) const
{
    if (!IsValid(Candidate) || !IsValid(Trigger)) return false;

    const UBoxComponent* Box = nullptr;
    if (const ATriggerBox* TriggerBox = Cast<ATriggerBox>(Trigger))
        Box = Cast<UBoxComponent>(TriggerBox->GetCollisionComponent());
    if (!Box)
    {
        TArray<UBoxComponent*> Boxes;
        Trigger->GetComponents<UBoxComponent>(Boxes);
        if (Boxes.Num() > 0) Box = Boxes[0];
    }
    if (!Box) return false;
    const FVector Local = Box->GetComponentTransform().InverseTransformPosition(Candidate->GetActorLocation());
    const FVector Extent = Box->GetUnscaledBoxExtent();
    return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}

void AEncounterManagerActor::DiscoverAndBuildEncounterActors()
{
    if (!GetWorld() || EncounterTriggers.Num() == 0 || LevelEncounters.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("EncounterManager discovery skipped: world=%s triggers=%d definitions=%d"),
            GetWorld() ? TEXT("valid") : TEXT("null"), EncounterTriggers.Num(), LevelEncounters.Num());
        return;
    }

    TArray<TPair<AActor*, int32>> TriggerList;
    for (const TPair<TObjectPtr<AActor>, int32>& Pair : EncounterTriggers)
        if (IsValid(Pair.Key.Get())) TriggerList.Emplace(Pair.Key.Get(), Pair.Value);

    int32 Added = 0;
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Candidate = *It;
        if (!HasTargetComponent(Candidate)) continue;

        bool bAlreadyAssigned = false;
        for (const FEncounter& Existing : LevelEncounters)
            if (Existing.EncounterActors.Contains(Candidate)) { bAlreadyAssigned = true; break; }
        if (bAlreadyAssigned) continue;

        // assign by the enemy actor origins distance to each encounter trigger actor
        // do not use meshcomponent bounds or volume overlap large turrets can extend
        // into neighbouring trigger boxes even though their actor pivot belongs to one area
        AActor* ChosenTrigger = nullptr;
        int32 ChosenID = INDEX_NONE;
        double BestDistanceSq = TNumericLimits<double>::Max();
        for (const TPair<AActor*, int32>& TriggerPair : TriggerList)
        {
            const double DistanceSq = FVector::DistSquared(
                Candidate->GetActorLocation(), TriggerPair.Key->GetActorLocation());
            if (DistanceSq < BestDistanceSq)
            {
                BestDistanceSq = DistanceSq;
                ChosenTrigger = TriggerPair.Key;
                ChosenID = TriggerPair.Value;
            }
        }

        if (FEncounter* Encounter = FindEncounter(ChosenID))
        {
            Encounter->EncounterActors.Add(Candidate);
            ++Added;
            UE_LOG(LogTemp, Log, TEXT("EncounterManager assigned target %s to encounter %d; distance to trigger=%0.1f"),
                *Candidate->GetName(), ChosenID, FMath::Sqrt(BestDistanceSq));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("EncounterManager could not find encounter %d for target %s"),
                ChosenID, *Candidate->GetName());
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("EncounterManager discovery added %d targets using nearest trigger actor origin."), Added);
}

void AEncounterManagerActor::AutoAssignDoorEndInteractors()
{
    if (!GetWorld()) return;

    // preserve all explicitly configured endinteractors exactly as authored
    // for entries left empty pair the two nearest bpdoor actors with that
    // encounter this reproduces the common paireddoor setup and unlike the
    // previous implementation does not stop after assigning only one door
    TSet<TObjectPtr<AActor>> AlreadyAssigned;
    for (const FEncounter& Encounter : LevelEncounters)
        for (AActor* Interactor : Encounter.EndInteractors)
            if (IsValid(Interactor)) AlreadyAssigned.Add(Interactor);

    int32 AssignedDoors = 0;
    constexpr int32 MaxAutoLinkedDoorsPerEncounter = 2;
    for (FEncounter& Encounter : LevelEncounters)
    {
        if (Encounter.EndInteractors.Num() >= MaxAutoLinkedDoorsPerEncounter) continue;

        AActor* TriggerForEncounter = nullptr;
        for (const TPair<TObjectPtr<AActor>, int32>& Pair : EncounterTriggers)
            if (Pair.Value == Encounter.EncounterID) { TriggerForEncounter = Pair.Key.Get(); break; }
        if (!IsValid(TriggerForEncounter)) continue;

        TArray<TPair<double, AActor*>> DoorCandidates;
        for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        {
            AActor* Candidate = *It;
            if (!IsValid(Candidate) || AlreadyAssigned.Contains(Candidate)) continue;
            const FString ClassName = Candidate->GetClass()->GetName();
            const FString ActorName = Candidate->GetName();
            if (!ClassName.Contains(TEXT("Door"), ESearchCase::IgnoreCase) &&
                !ActorName.Contains(TEXT("Door"), ESearchCase::IgnoreCase)) continue;
            if (!Candidate->FindFunction(FName(TEXT("Interact")))) continue;
            const double DistanceSq = FVector::DistSquared(Candidate->GetActorLocation(), TriggerForEncounter->GetActorLocation());
            DoorCandidates.Emplace(DistanceSq, Candidate);
        }
        DoorCandidates.Sort([](const TPair<double, AActor*>& A, const TPair<double, AActor*>& B)
        {
            return A.Key < B.Key;
        });

        const int32 RemainingSlots = MaxAutoLinkedDoorsPerEncounter - Encounter.EndInteractors.Num();
        const int32 CountToAssign = FMath::Min(RemainingSlots, DoorCandidates.Num());
        for (int32 Index = 0; Index < CountToAssign; ++Index)
        {
            AActor* Door = DoorCandidates[Index].Value;
            Encounter.EndInteractors.Add(Door);
            AlreadyAssigned.Add(Door);
            ++AssignedDoors;
            UE_LOG(LogTemp, Warning, TEXT("EncounterManager paired door %s with encounter %d (pair %d/%d)."),
                *Door->GetName(), Encounter.EncounterID, Index + 1, CountToAssign);
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("EncounterManager auto-linked %d doors across empty EndInteractors lists."), AssignedDoors);
}

FEncounter* AEncounterManagerActor::FindEncounter(int32 ID)
{
    return LevelEncounters.FindByPredicate([ID](const FEncounter& E) { return E.EncounterID == ID; });
}

void AEncounterManagerActor::HandleTriggerOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (!Pawn || !Pawn->GetController() || !Pawn->GetController()->IsPlayerController()) return;
    const int32* ID = EncounterTriggers.Find(OverlappedActor);
    if (!ID) return;
    CurrentInteractor = Pawn;
    ActivateEncounter(*ID);
    OverlappedActor->SetActorEnableCollision(false);
}

void AEncounterManagerActor::ActivateEncounter(int32 EncounterID)
{
    if (bEncounterActive || CompletedEncounters.Contains(EncounterID)) return;
    FEncounter* Found = FindEncounter(EncounterID);
    if (!Found)
    {
        UE_LOG(LogTemp, Error, TEXT("EncounterManager: no LevelEncounters entry for ID %d"), EncounterID);
        return;
    }

    TargetEncounter = *Found;
    bEncounterActive = true;
    CurrentTargetCount = 0;
    BoundTargets.Reset();

    for (AActor* Actor : TargetEncounter.StartInteractors) InvokeInteract(Actor, CurrentInteractor.Get());

    for (AActor* Target : TargetEncounter.EncounterActors)
    {
        if (!IsValid(Target) || BoundTargets.Contains(Target)) continue;
        Target->OnDestroyed.AddUniqueDynamic(this, &AEncounterManagerActor::HandleTargetDestroyed);
        BoundTargets.Add(Target);
        EnableTarget(Target);

        // match the original activatetarget call on bpctargetcomponent
        TArray<UActorComponent*> Components;
        Target->GetComponents<UActorComponent>(Components);
        for (UActorComponent* Component : Components)
        {
            if (!IsValid(Component)) continue;
            if (Component->GetName().Contains(TEXT("TargetComponent")) || Component->GetClass()->GetName().Contains(TEXT("TargetComponent")))
            {
                if (UFunction* ActivateFn = Component->FindFunction(FName(TEXT("ActivateTarget"))))
                    Component->ProcessEvent(ActivateFn, nullptr);
                break;
            }
        }
        ++CurrentTargetCount;
    }

    UpdateCount();
    EEncounterStarted.Broadcast(CurrentTargetCount);
    UE_LOG(LogTemp, Log, TEXT("Encounter %d started with %d targets"), EncounterID, CurrentTargetCount);
    // do not complete an empty encounter the original blueprint only completes
    // through its targetdestroyed eventcount path
}

void AEncounterManagerActor::HandleTargetDestroyed(AActor* DestroyedActor)
{
    if (!bEncounterActive || !BoundTargets.Remove(DestroyedActor)) return;
    CurrentTargetCount = FMath::Max(0, CurrentTargetCount - 1);
    UpdateCount();
    if (CurrentTargetCount == 0) EncounterCompleted();
}

void AEncounterManagerActor::EncounterCompleted()
{
    if (!bEncounterActive) return;
    const int32 FinishedID = TargetEncounter.EncounterID;
    if (FinishedID == INDEX_NONE) { bEncounterActive = false; return; }
    if (!CompletedEncounters.Contains(FinishedID)) CompletedEncounters.Add(FinishedID);
    for (AActor* Actor : TargetEncounter.EndInteractors) InvokeInteract(Actor, CurrentInteractor.Get());
    CurrentTargetCount = 0;
    UpdateCount();
    bEncounterActive = false;
    EEncounterEnded.Broadcast();

    if (LevelEncounters.Num() > 0 && CompletedEncounters.Num() == LevelEncounters.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("EncounterManager: all encounters complete; activating final endpoint."));
        ActivateFinalEndpoint();
        EEncountersCompleted.Broadcast(-1, true);
    }
    UE_LOG(LogTemp, Log, TEXT("Encounter %d completed: %d/%d"), FinishedID, CompletedEncounters.Num(), LevelEncounters.Num());
}

void AEncounterManagerActor::EnableTarget(AActor* TargetActor)
{
    if (!IsValid(TargetActor)) return;
    TargetActor->SetActorTickEnabled(true);
    TargetActor->SetActorHiddenInGame(false);
}

void AEncounterManagerActor::DisableTarget(AActor* TargetActor)
{
    if (!IsValid(TargetActor)) return;
    TargetActor->SetActorTickEnabled(false);
    TargetActor->SetActorHiddenInGame(true);
}

void AEncounterManagerActor::InvokeInteract(AActor* Actor, AActor* InteractorPawn)
{
    if (!IsValid(Actor)) return;
    UFunction* Function = Actor->FindFunction(FName(TEXT("Interact")));
    if (!Function) return;
    FStructOnScope Parameters(Function);
    for (TFieldIterator<FProperty> It(Function); It; ++It)
    {
        FProperty* Property = *It;
        if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
        {
            if (Property->HasAnyPropertyFlags(CPF_Parm) && !Property->HasAnyPropertyFlags(CPF_ReturnParm))
            {
                ObjectProperty->SetObjectPropertyValue_InContainer(Parameters.GetStructMemory(), InteractorPawn);
                break;
            }
        }
    }
    Actor->ProcessEvent(Function, Parameters.GetStructMemory());
}

void AEncounterManagerActor::UpdateCount()
{
    ETargetCountUpdated.Broadcast(CurrentTargetCount);
}

void AEncounterManagerActor::ActivateFinalEndpoint()
{
    if (!GetWorld()) return;

    int32 ActivatedCount = 0;
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Candidate = *It;
        if (!IsValid(Candidate) || Candidate == this) continue;

        const FString ClassName = Candidate->GetClass()->GetName();
        const FString ActorName = Candidate->GetName();
        if (!ClassName.Contains(TEXT("EndPoint"), ESearchCase::IgnoreCase) &&
            !ActorName.Contains(TEXT("EndPoint"), ESearchCase::IgnoreCase)) continue;

        // prefer an explicit blueprint activation function if the endpoint provides one
        static const FName ActivationFunctions[] = {
            FName(TEXT("ActivateEndPoint")), FName(TEXT("ActivateEndpoint")),
            FName(TEXT("EnableEndPoint")), FName(TEXT("EnableEndpoint")),
            FName(TEXT("ShowEndPoint")), FName(TEXT("ShowEndpoint"))
        };
        bool bCalledActivationFunction = false;
        for (const FName FunctionName : ActivationFunctions)
        {
            if (UFunction* Function = Candidate->FindFunction(FunctionName))
            {
                Candidate->ProcessEvent(Function, nullptr);
                bCalledActivationFunction = true;
                UE_LOG(LogTemp, Warning, TEXT("EncounterManager: called %s on %s"), *FunctionName.ToString(), *Candidate->GetName());
                break;
            }
        }

        // bpendpoints original beginplay hidesdisables it when bautoactivate is false
        // at final encounter completion explicitly reveal it and restore overlap collision
        Candidate->SetActorHiddenInGame(false);
        Candidate->SetActorEnableCollision(true);
        Candidate->SetActorTickEnabled(true);
        TArray<UActorComponent*> Components;
        Candidate->GetComponents<UActorComponent>(Components);
        for (UActorComponent* Component : Components)
        {
            if (!IsValid(Component)) continue;
            if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
            {
                Primitive->SetHiddenInGame(false, true);
                Primitive->SetVisibility(true, true);
                const FString ComponentName = Primitive->GetName();
                if (ComponentName.Contains(TEXT("Sphere"), ESearchCase::IgnoreCase) ||
                    ComponentName.Contains(TEXT("Trigger"), ESearchCase::IgnoreCase))
                {
                    Primitive->SetGenerateOverlapEvents(true);
                    Primitive->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                }
            }
        }
        ++ActivatedCount;
        UE_LOG(LogTemp, Warning, TEXT("EncounterManager: endpoint %s revealed; activation function called=%s"),
            *Candidate->GetName(), bCalledActivationFunction ? TEXT("true") : TEXT("false"));
    }

    if (ActivatedCount == 0)
        UE_LOG(LogTemp, Error, TEXT("EncounterManager: no BP_EndPoint actor found. Check the endpoint actor name/class in BaseMap."));
}
