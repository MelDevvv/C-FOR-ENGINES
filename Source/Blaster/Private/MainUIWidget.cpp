#include "MainUIWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UMainUIWidget::NativeConstruct()

{

	Super::NativeConstruct();

	if (HealthBar)

	{

		HealthBar->SetPercent(1.0f);
        HealthBar->SetFillColorAndOpacity(FLinearColor(0.12f, 0.82f, 0.45f, 1.0f));
        HealthBar->SetIsMarquee(false);

	}

}

void UMainUIWidget::UpdateHealthBar(

	bool bPositive,

	float NewHealth,

	float HealthDelta,

	float HealthRatio

)

{

	if (HealthBar)

	{

		HealthBar->SetPercent(

			FMath::Clamp(HealthRatio, 0.0f, 1.0f)

		);

	}

}
 