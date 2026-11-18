#include "TankedGameModeBase.h"

#include "BaseGameRule.h"
#include "ObjectiveUIWidget.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ATankedGameModeBase::ATankedGameModeBase()
{
    static ConstructorHelpers::FClassFinder<UObjectiveUIWidget> ObjectiveWidgetFinder(
        TEXT("/Game/Scripts/UI/WBP_ObjectiveUI"));

    if (ObjectiveWidgetFinder.Succeeded())
    {
        ObjectiveUIWidgetClass = ObjectiveWidgetFinder.Class;
    }
    else
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("TankedGameModeBase: Could not find /Game/Scripts/UI/WBP_ObjectiveUI. "
                 "Check that WBP_ObjectiveUI inherits from ObjectiveUIWidget."));
    }
}

void ATankedGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("TankedGameModeBase BeginPlay"));

    RegisterGameRules();

    for (int32 Index = 0; Index < ActiveRules.Num(); ++Index)
    {
        UBaseGameRule* Rule = ActiveRules[Index];

        if (!IsValid(Rule))
        {
            UE_LOG(LogTemp, Error, TEXT("Rule %d is invalid"), Index);
            continue;
        }

        Rule->OnRuleComplete.AddUniqueDynamic(
            this,
            &ATankedGameModeBase::OnRuleCompleted);

        Rule->InitRule(Index);

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Initialised rule %d: %s"),
            Index,
            *Rule->GetClass()->GetName());
    }

    CreateUIElementsForRules();
}

UBaseGameRule* ATankedGameModeBase::CreateRule(
    TSubclassOf<UBaseGameRule> RuleClass)
{
    if (!RuleClass)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("A rule class is missing in GameMode Class Defaults."));
        return nullptr;
    }

    UBaseGameRule* Rule = NewObject<UBaseGameRule>(this, RuleClass);

    if (!IsValid(Rule))
    {
        return nullptr;
    }

    Rule->RegisterComponent();
    CreatedRules.Add(Rule);

    return Rule;
}

void ATankedGameModeBase::RegisterGameRules_Implementation()
{
    ActiveRules.Empty();
    CreatedRules.Empty();

    if (UBaseGameRule* Rule = CreateRule(TimerRuleClass))
    {
        ActiveRules.Add(Rule);
    }

    if (UBaseGameRule* Rule = CreateRule(EndReachedRuleClass))
    {
        ActiveRules.Add(Rule);
    }

    if (UBaseGameRule* Rule = CreateRule(EncounterRuleClass))
    {
        ActiveRules.Add(Rule);
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Registered %d game rules."),
        ActiveRules.Num());
}

void ATankedGameModeBase::OnRuleCompleted_Implementation(
    int32 RuleID,
    bool bSuccessful)
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("RULE COMPLETED: ID=%d, Success=%s"),
        RuleID,
        bSuccessful ? TEXT("TRUE") : TEXT("FALSE"));

    // Rule 0 is the timer. Failure means time has run out.
    if (RuleID == 0 && !bSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("TIMER EXPIRED - RESTARTING LEVEL"));
        EndGame();
        return;
    }

    // Rule 1 is the exit. Success means the player reached the exit.
    if (RuleID == 1 && bSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("EXIT REACHED - RESTARTING LEVEL"));
        EndGame();
        return;
    }

    // Encounter completion does not restart the level.
    if (RuleID == 2 && bSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("ENCOUNTER RULE COMPLETED"));
    }
}

void ATankedGameModeBase::EndGame_Implementation()
{
    if (!GetWorld())
    {
        return;
    }

    if (GetWorld()->GetTimerManager().IsTimerActive(EndGameTimerHandle))
    {
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(
        EndGameTimerHandle,
        this,
        &ATankedGameModeBase::OnEndTimerComplete,
        1.0f,
        false);
}

bool ATankedGameModeBase::CheckRules_Implementation(
    int32 RuleID,
    bool bSuccessful)
{
    if (!bSuccessful)
    {
        return false;
    }

    for (const TObjectPtr<UBaseGameRule>& Rule : ActiveRules)
    {
        if (!IsValid(Rule.Get()))
        {
            continue;
        }

        if (!Rule->Completed)
        {
            return false;
        }
    }

    return true;
}

void ATankedGameModeBase::CreateUIElementsForRules_Implementation()
{
    if (IsValid(ObjectiveUIWidgetInstance))
    {
        // Already on screen; do not duplicate the objective panel.
        if (!ObjectiveUIWidgetInstance->IsInViewport())
        {
            ObjectiveUIWidgetInstance->AddToViewport(0);
        }
        return;
    }

    if (!ObjectiveUIWidgetClass)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Cannot create objective UI: ObjectiveUIWidgetClass is null. "
                 "Ensure WBP_ObjectiveUI is a child of ObjectiveUIWidget."));
        return;
    }

    UObjectiveUIWidget* ObjectiveWidget =
        CreateWidget<UObjectiveUIWidget>(GetWorld(), ObjectiveUIWidgetClass);

    if (!IsValid(ObjectiveWidget))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create WBP_ObjectiveUI."));
        return;
    }

    // The original Blueprint function pin is a generic Object Array.
    // Convert ActiveRules to that exact input type before calling InitRulesCPP.
    TArray<UObject*> RuleObjects;
    RuleObjects.Reserve(ActiveRules.Num());

    for (const TObjectPtr<UBaseGameRule>& Rule : ActiveRules)
    {
        if (IsValid(Rule.Get()))
        {
            RuleObjects.Add(Rule.Get());
        }
    }

    ObjectiveWidget->InitRulesCPP(RuleObjects);
    ObjectiveWidget->AddToViewport(0);

    ObjectiveUIWidgetInstance = ObjectiveWidget;

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Created WBP_ObjectiveUI and added %d rules to the viewport."),
        RuleObjects.Num());
}

void ATankedGameModeBase::OnEndTimerComplete_Implementation()
{
    const FString CurrentLevel =
        UGameplayStatics::GetCurrentLevelName(this, true);

    if (CurrentLevel.IsEmpty())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Cannot restart: current level name is empty."));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Restarting level: %s"), *CurrentLevel);
    UGameplayStatics::OpenLevel(this, FName(*CurrentLevel));
}
