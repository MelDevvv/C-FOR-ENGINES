
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "TankController.generated.h"

class UInputAction;
struct FInputActionInstance;

UCLASS(Blueprintable)
class BLASTER_API ATankController : public APlayerController
{
	GENERATED_BODY()

public:
	ATankController();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> MappingContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> ForwardAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> TurnAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> FireAction = nullptr;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

private:
	void OnForward(const FInputActionInstance& Instance);
	void OnTurn(const FInputActionInstance& Instance);
	void OnFire(const FInputActionInstance& Instance);
};
