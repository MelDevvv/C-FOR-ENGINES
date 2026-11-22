#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Inputable.generated.h"
// this class does not need to be modified
UINTERFACE(Blueprintable)
class UInputable : public UInterface
{
	GENERATED_BODY()
};
class BLASTER_API IInputable
{
	GENERATED_BODY()
	// add interface functions to this class this is the class that will be inherited to implement this interface
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inputable")
	void IMove(float inputValue);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inputable")
	void ITurn(float inputValue);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inputable")
	void IPrimaryAction(bool bIsPressed);
};
