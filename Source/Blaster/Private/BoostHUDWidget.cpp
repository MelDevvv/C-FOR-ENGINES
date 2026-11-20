#include "BoostHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UBoostHUDWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (!WidgetTree)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("BoostCanvas"));
    WidgetTree->RootWidget = Canvas;

    UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("BoostLayout"));
    UCanvasPanelSlot* LayoutSlot = Canvas->AddChildToCanvas(Layout);
    LayoutSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
    LayoutSlot->SetAlignment(FVector2D(0.5f, 1.0f));
    LayoutSlot->SetPosition(FVector2D(0.0f, -72.0f));
    LayoutSlot->SetSize(FVector2D(340.0f, 62.0f));

    UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("BoostHeading"));
    Heading->SetText(FText::FromString(TEXT("BOOST")));
    Heading->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 0.85f, 1.0f, 1.0f)));
    FSlateFontInfo HeadingFont = Heading->GetFont();
    HeadingFont.Size = 13;
    Heading->SetFont(HeadingFont);
    Layout->AddChildToVerticalBox(Heading);

    EnergyBar = WidgetTree->ConstructWidget<UProgressBar>(
        UProgressBar::StaticClass(), TEXT("BoostEnergyBar"));
    EnergyBar->SetPercent(1.0f);
    EnergyBar->SetFillColorAndOpacity(FLinearColor(0.12f, 0.72f, 1.0f, 1.0f));
    // UProgressBar has no SetBackgroundColor function. Keep the engine default background.
    UVerticalBoxSlot* BarSlot = Layout->AddChildToVerticalBox(EnergyBar);
    BarSlot->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 4.0f));

    StatusText = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("BoostStatus"));
    StatusText->SetJustification(ETextJustify::Center);
    StatusText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    FSlateFontInfo StatusFont = StatusText->GetFont();
    StatusFont.Size = 11;
    StatusText->SetFont(StatusFont);
    Layout->AddChildToVerticalBox(StatusText);

    UpdateBoostDisplay(100.0f, 100.0f, false);
}

void UBoostHUDWidget::UpdateBoostDisplay(
    float CurrentEnergy,
    float MaximumEnergy,
    bool bBoosting)
{
    const float SafeMaximum = FMath::Max(1.0f, MaximumEnergy);
    const float Percent = FMath::Clamp(CurrentEnergy / SafeMaximum, 0.0f, 1.0f);

    if (EnergyBar)
    {
        EnergyBar->SetPercent(Percent);
        EnergyBar->SetFillColorAndOpacity(
            bBoosting
                ? FLinearColor(1.0f, 0.48f, 0.08f, 1.0f)
                : FLinearColor(0.12f, 0.72f, 1.0f, 1.0f));
    }

    if (StatusText)
    {
        const int32 RoundedPercent = FMath::RoundToInt(Percent * 100.0f);
        const TCHAR* StateText = bBoosting
            ? TEXT("BOOST ACTIVE")
            : (Percent >= 0.999f ? TEXT("READY  -  HOLD LEFT SHIFT") : TEXT("RECHARGING"));
        StatusText->SetText(FText::FromString(FString::Printf(
            TEXT("%d%%   |   %s"), RoundedPercent, StateText)));
    }
}
