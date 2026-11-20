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
    if (!WidgetTree) return;

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CombatCanvas"));
    WidgetTree->RootWidget = Canvas;
    UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CombatLayout"));
    UCanvasPanelSlot* LayoutSlot = Canvas->AddChildToCanvas(Layout);
    LayoutSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
    LayoutSlot->SetAlignment(FVector2D(0.5f, 1.0f));
    LayoutSlot->SetPosition(FVector2D(0.0f, -82.0f));
    LayoutSlot->SetSize(FVector2D(360.0f, 118.0f));

    auto MakeLabel = [this, Layout](const TCHAR* Name, const TCHAR* Text, int32 FontSize, const FLinearColor& Colour)
    {
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
        Label->SetText(FText::FromString(Text));
        Label->SetColorAndOpacity(FSlateColor(Colour));
        FSlateFontInfo Font = Label->GetFont(); Font.Size = FontSize; Label->SetFont(Font);
        Layout->AddChildToVerticalBox(Label);
        return Label;
    };

    MakeLabel(TEXT("BoostHeading"), TEXT("BOOST"), 12, FLinearColor(0.25f,0.85f,1.0f,1.0f));
    EnergyBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("BoostEnergyBar"));
    EnergyBar->SetPercent(1.0f); EnergyBar->SetFillColorAndOpacity(FLinearColor(0.12f,0.72f,1.0f,1.0f));
    UVerticalBoxSlot* BoostSlot = Layout->AddChildToVerticalBox(EnergyBar); BoostSlot->SetPadding(FMargin(0,2,0,3));
    StatusText = MakeLabel(TEXT("BoostStatus"), TEXT("100% | READY - HOLD LEFT SHIFT"), 10, FLinearColor::White);

    MakeLabel(TEXT("ShieldHeading"), TEXT("ENERGY SHIELD  [Q]"), 12, FLinearColor(0.3f,1.0f,0.55f,1.0f));
    ShieldBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ShieldEnergyBar"));
    ShieldBar->SetPercent(1.0f); ShieldBar->SetFillColorAndOpacity(FLinearColor(0.15f,0.9f,0.45f,1.0f));
    UVerticalBoxSlot* ShieldSlot = Layout->AddChildToVerticalBox(ShieldBar); ShieldSlot->SetPadding(FMargin(0,2,0,3));
    ShieldStatusText = MakeLabel(TEXT("ShieldStatus"), TEXT("100% | READY"), 10, FLinearColor::White);
    RocketStatusText = MakeLabel(TEXT("RocketStatus"), TEXT("ROCKETS [RIGHT CLICK] - READY"), 10, FLinearColor(1.0f,0.65f,0.25f,1.0f));
}

void UBoostHUDWidget::UpdateBoostDisplay(float CurrentEnergy, float MaximumEnergy, bool bBoosting)
{
    const float Percent = FMath::Clamp(CurrentEnergy / FMath::Max(1.0f, MaximumEnergy), 0.0f, 1.0f);
    if (EnergyBar) { EnergyBar->SetPercent(Percent); EnergyBar->SetFillColorAndOpacity(bBoosting ? FLinearColor(1.0f,0.48f,0.08f,1.0f) : FLinearColor(0.12f,0.72f,1.0f,1.0f)); }
    if (StatusText) StatusText->SetText(FText::FromString(FString::Printf(TEXT("%d%% | %s"), FMath::RoundToInt(Percent*100.0f), bBoosting ? TEXT("BOOST ACTIVE") : (Percent >= 0.999f ? TEXT("READY - HOLD LEFT SHIFT") : TEXT("RECHARGING")))));
}

void UBoostHUDWidget::UpdateShieldDisplay(float CurrentEnergy, float MaximumEnergy, bool bActive, float CooldownRemaining)
{
    const float Percent = FMath::Clamp(CurrentEnergy / FMath::Max(1.0f, MaximumEnergy), 0.0f, 1.0f);
    if (ShieldBar) { ShieldBar->SetPercent(Percent); ShieldBar->SetFillColorAndOpacity(bActive ? FLinearColor(0.2f,1.0f,0.55f,1.0f) : FLinearColor(0.1f,0.65f,0.3f,1.0f)); }
    if (ShieldStatusText)
    {
        FString State = bActive ? TEXT("ACTIVE - ABSORBING DAMAGE") : (CooldownRemaining > 0.0f ? FString::Printf(TEXT("COOLDOWN %.1fs"), CooldownRemaining) : (Percent >= 0.999f ? TEXT("READY - PRESS Q") : TEXT("RECHARGING")));
        ShieldStatusText->SetText(FText::FromString(FString::Printf(TEXT("%d%% | %s"), FMath::RoundToInt(Percent*100.0f), *State)));
    }
}

void UBoostHUDWidget::UpdateRocketDisplay(float CooldownRemaining)
{
    if (RocketStatusText)
    {
        RocketStatusText->SetText(CooldownRemaining > 0.0f
            ? FText::FromString(FString::Printf(TEXT("ROCKETS [RIGHT CLICK] - %.1fs"), CooldownRemaining))
            : FText::FromString(TEXT("ROCKETS [RIGHT CLICK] - READY")));
    }
}
