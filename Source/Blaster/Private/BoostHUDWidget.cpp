#include "BoostHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"

namespace
{
    const FLinearColor PanelColor(0.025f, 0.045f, 0.075f, 0.88f);
    const FLinearColor TextColor(0.82f, 0.91f, 1.0f, 1.0f);
    const FLinearColor MutedColor(0.48f, 0.62f, 0.72f, 1.0f);
    void StyleText(UTextBlock* Text, int32 Size, FLinearColor Color, bool bBold = false)
    {
        if (!Text) return;
        Text->SetColorAndOpacity(FSlateColor(Color));
        FSlateFontInfo Font = Text->GetFont(); Font.Size = Size; Text->SetFont(Font);
        Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
        Text->SetShadowColorAndOpacity(FLinearColor(0,0,0,0.65f));
    }
}
void UBoostHUDWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree) return;
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CombatCanvas"));
    WidgetTree->RootWidget = Canvas;
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CombatPanel"));
    Panel->SetBrushColor(PanelColor);
    Panel->SetPadding(FMargin(12.0f, 10.0f));
    UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
    PanelSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
    PanelSlot->SetAlignment(FVector2D(0.5f, 1.0f));
    PanelSlot->SetPosition(FVector2D(0.0f, -30.0f));
    PanelSlot->SetSize(FVector2D(410.0f, 225.0f));
    UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CombatLayout"));
    Panel->SetContent(Layout);
    auto MakeLabel = [this, Layout](const TCHAR* Name, const TCHAR* Text, int32 FontSize, const FLinearColor& Colour, bool bBold=false)
    {
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
        Label->SetText(FText::FromString(Text)); StyleText(Label, FontSize, Colour, bBold);
        UVerticalBoxSlot* Slot = Layout->AddChildToVerticalBox(Label); Slot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 1.0f));
        return Label;
    };
    auto MakeBar = [this, Layout](const TCHAR* Name, FLinearColor Fill)
    {
        UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName(Name));
        Bar->SetPercent(1.0f); Bar->SetFillColorAndOpacity(Fill);
        Bar->SetIsMarquee(false); UVerticalBoxSlot* Slot = Layout->AddChildToVerticalBox(Bar); Slot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 4.0f)); Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        return Bar;
    };
    MakeLabel(TEXT("CombatHeader"), TEXT("COMBAT SYSTEMS"), 10, FLinearColor(0.45f,0.8f,1.0f,1.0f), true);
    MakeLabel(TEXT("BoostHeading"), TEXT("BOOST  /  SHIFT"), 11, FLinearColor(0.25f,0.85f,1.0f,1.0f), true);
    EnergyBar = MakeBar(TEXT("BoostEnergyBar"), FLinearColor(0.1f,0.68f,1.0f,1.0f));
    StatusText = MakeLabel(TEXT("BoostStatus"), TEXT("100%  |  READY"), 9, TextColor);
    MakeLabel(TEXT("ShieldHeading"), TEXT("ENERGY SHIELD  /  Q"), 11, FLinearColor(0.3f,1.0f,0.55f,1.0f), true);
    ShieldBar = MakeBar(TEXT("ShieldEnergyBar"), FLinearColor(0.15f,0.9f,0.45f,1.0f));
    ShieldStatusText = MakeLabel(TEXT("ShieldStatus"), TEXT("100%  |  READY"), 9, TextColor);
    MakeLabel(TEXT("LightningHeading"), TEXT("LIGHTNING BLAST  /  F"), 11, FLinearColor(0.65f,0.42f,1.0f,1.0f), true);
    LightningBlastBar = MakeBar(TEXT("LightningBlastBar"), FLinearColor(0.62f,0.38f,1.0f,1.0f));
    LightningBlastStatusText = MakeLabel(TEXT("LightningBlastStatus"), TEXT("READY  |  50 DAMAGE  |  30 SEC"), 9, TextColor);
    RocketStatusText = MakeLabel(TEXT("RocketStatus"), TEXT("ROCKET  /  RIGHT CLICK  -  READY"), 9, FLinearColor(1.0f,0.65f,0.25f,1.0f), true);
}
void UBoostHUDWidget::UpdateBoostDisplay(float CurrentEnergy, float MaximumEnergy, bool bBoosting)
{
    const float Percent = FMath::Clamp(CurrentEnergy / FMath::Max(1.0f, MaximumEnergy), 0.0f, 1.0f);
    if (EnergyBar) { EnergyBar->SetPercent(Percent); EnergyBar->SetFillColorAndOpacity(bBoosting ? FLinearColor(1.0f,0.48f,0.08f,1.0f) : FLinearColor(0.12f,0.72f,1.0f,1.0f)); }
    if (StatusText) StatusText->SetText(FText::FromString(FString::Printf(TEXT("%d%%  |  %s"), FMath::RoundToInt(Percent*100.0f), bBoosting ? TEXT("BOOST ACTIVE") : (Percent >= 0.999f ? TEXT("READY - HOLD SHIFT") : TEXT("RECHARGING")))));
}
void UBoostHUDWidget::UpdateShieldDisplay(float CurrentEnergy, float MaximumEnergy, bool bActive, float CooldownRemaining)
{
    const float Percent = FMath::Clamp(CurrentEnergy / FMath::Max(1.0f, MaximumEnergy), 0.0f, 1.0f);
    if (ShieldBar) { ShieldBar->SetPercent(Percent); ShieldBar->SetFillColorAndOpacity(bActive ? FLinearColor(0.2f,1.0f,0.55f,1.0f) : FLinearColor(0.1f,0.65f,0.3f,1.0f)); }
    if (ShieldStatusText)
    {
        FString State = bActive ? TEXT("ACTIVE - ABSORBING DAMAGE") : (CooldownRemaining > 0.0f ? FString::Printf(TEXT("COOLDOWN %.1fs"), CooldownRemaining) : (Percent >= 0.999f ? TEXT("READY - PRESS Q") : TEXT("RECHARGING")));
        ShieldStatusText->SetText(FText::FromString(FString::Printf(TEXT("%d%%  |  %s"), FMath::RoundToInt(Percent*100.0f), *State)));
    }
}
void UBoostHUDWidget::UpdateRocketDisplay(float CooldownRemaining)
{
    if (RocketStatusText) RocketStatusText->SetText(CooldownRemaining > 0.0f
        ? FText::FromString(FString::Printf(TEXT("ROCKET  /  RIGHT CLICK  -  %.1fs"), CooldownRemaining))
        : FText::FromString(TEXT("ROCKET  /  RIGHT CLICK  -  READY")));
}
void UBoostHUDWidget::UpdateLightningBlastDisplay(float CooldownRemaining, float CooldownDuration)
{
    const float Remaining = FMath::Max(0.0f, CooldownRemaining);
    const float Duration = FMath::Max(0.1f, CooldownDuration);
    const float Percent = 1.0f - FMath::Clamp(Remaining / Duration, 0.0f, 1.0f);
    if (LightningBlastBar) LightningBlastBar->SetPercent(Percent);
    if (LightningBlastStatusText) LightningBlastStatusText->SetText(Remaining > 0.0f
        ? FText::FromString(FString::Printf(TEXT("RECHARGING  |  %.1f SEC"), Remaining))
        : FText::FromString(TEXT("READY  |  50 DAMAGE  |  30 SEC")));
}
