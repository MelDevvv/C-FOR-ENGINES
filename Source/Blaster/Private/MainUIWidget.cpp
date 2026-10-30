#include "MainUIWidget.h"

#include "Components/ProgressBar.h"

void UMainUIWidget::NativeConstruct()

{

	Super::NativeConstruct();

	if (HealthBar)

	{

		HealthBar->SetPercent(1.0f);

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
 