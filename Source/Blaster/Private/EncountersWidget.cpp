#include "EncountersWidget.h"

#include "EncounterManagerActor.h"
#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

void UEncountersWidget::NativeConstruct()
{
    Super::NativeConstruct();

    CacheEncounterTextBlock();

    // The original Event Construct gets the BP_EncounterManager actor and binds
    // all three dispatchers. Retry until the manager exists in the world.
    FindAndBindEncounterManager();

    if (!IsValid(EncounterManager) && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            ManagerLookupTimer,
            this,
            &UEncountersWidget::FindAndBindEncounterManager,
            0.25f,
            true);
    }
}

void UEncountersWidget::NativeDestruct()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ManagerLookupTimer);
        GetWorld()->GetTimerManager().ClearTimer(EncounterEndedTimer);
    }

    if (IsValid(EncounterManager))
    {
        EncounterManager->EEncounterStarted.RemoveDynamic(this, &UEncountersWidget::HandleEncounterStarted);
        EncounterManager->ETargetCountUpdated.RemoveDynamic(this, &UEncountersWidget::HandleTargetCountUpdated);
        EncounterManager->EEncounterEnded.RemoveDynamic(this, &UEncountersWidget::HandleEncounterEnded);
    }

    EncounterManager = nullptr;
    EncounterTextBlock = nullptr;

    Super::NativeDestruct();
}

void UEncountersWidget::CacheEncounterTextBlock()
{
    // T_Encounter is nested under ScaleBox_23 in the supplied Designer screenshot.
    // GetWidgetFromName resolves the named child in the widget tree.
    EncounterTextBlock = Cast<UTextBlock>(GetWidgetFromName(TEXT("T_Encounter")));

    if (!IsValid(EncounterTextBlock))
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Encounters: Could not find TextBlock 'T_Encounter'."));
    }
}

void UEncountersWidget::FindAndBindEncounterManager()
{
    if (!GetWorld() || IsValid(EncounterManager))
    {
        if (IsValid(EncounterManager) && GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(ManagerLookupTimer);
        }
        return;
    }

    for (TActorIterator<AEncounterManagerActor> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It))
        {
            EncounterManager = *It;
            break;
        }
    }

    if (!IsValid(EncounterManager))
    {
        UE_LOG(LogTemp, Verbose, TEXT("WBP_Encounters: AEncounterManagerActor not found yet; retrying."));
        return;
    }

    EncounterManager->EEncounterStarted.AddUniqueDynamic(this, &UEncountersWidget::HandleEncounterStarted);
    EncounterManager->ETargetCountUpdated.AddUniqueDynamic(this, &UEncountersWidget::HandleTargetCountUpdated);
    EncounterManager->EEncounterEnded.AddUniqueDynamic(this, &UEncountersWidget::HandleEncounterEnded);

    GetWorld()->GetTimerManager().ClearTimer(ManagerLookupTimer);

    UE_LOG(LogTemp, Warning, TEXT("WBP_Encounters: Bound to manager %s (%s)."),
        *EncounterManager->GetName(), *EncounterManager->GetClass()->GetName());
}

void UEncountersWidget::HandleEncounterStarted(int32 InTotalTargets)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(EncounterEndedTimer);
    }

    TotalTargets = InTotalTargets;
    CurrentTargets = InTotalTargets;

    CacheEncounterTextBlock();
    UpdateEncounterText();

    // IMPORTANT: The original Blueprint sets visibility on SELF (the whole
    // WBP_Encounters widget), not only on the TextBlock.
    SetVisibility(ESlateVisibility::Visible);

    UE_LOG(LogTemp, Warning, TEXT("WBP_Encounters: Encounter started; showing %d/%d."),
        CurrentTargets, TotalTargets);
}

void UEncountersWidget::HandleTargetCountUpdated(int32 InTargetCount)
{
    CurrentTargets = InTargetCount;
    UpdateEncounterText();

    // Keep the whole widget visible while the encounter is active.
    SetVisibility(ESlateVisibility::Visible);
}

void UEncountersWidget::HandleEncounterEnded()
{
    CacheEncounterTextBlock();

    if (IsValid(EncounterTextBlock))
    {
        EncounterTextBlock->SetText(FText::FromString(TEXT("Encounter Completed!")));
    }

    // Match the original graph: completion message, Delay 1.0, then
    // SetVisibility(Collapsed) on SELF.
    SetVisibility(ESlateVisibility::Visible);

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(EncounterEndedTimer);
        GetWorld()->GetTimerManager().SetTimer(
            EncounterEndedTimer,
            this,
            &UEncountersWidget::HideEncounterWidget,
            1.0f,
            false);
    }
}

void UEncountersWidget::HideEncounterWidget()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

void UEncountersWidget::UpdateEncounterText()
{
    if (!IsValid(EncounterTextBlock))
    {
        CacheEncounterTextBlock();
    }

    if (!IsValid(EncounterTextBlock))
    {
        return;
    }

    const FText FormattedText = FText::Format(
        NSLOCTEXT("Blaster", "EncounterProgress", "Encounter: {0}/{1}"),
        FText::AsNumber(CurrentTargets),
        FText::AsNumber(TotalTargets));

    EncounterTextBlock->SetText(FormattedText);
    EncounterTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f,0.82f,1.0f,1.0f)));
    FSlateFontInfo EncounterFont = EncounterTextBlock->GetFont(); EncounterFont.Size = 18; EncounterTextBlock->SetFont(EncounterFont);
    EncounterTextBlock->SetShadowOffset(FVector2D(1.5f,1.5f)); EncounterTextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f,0.0f,0.0f,0.8f));
}
