#pragma once

#include "CoreMinimal.h"

#include "GameStructs.generated.h"

USTRUCT(BlueprintType)

struct FEncounter

{

	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)

	int32 EncounterID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)

	TArray<AActor*> EncounterActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)

	bool bStartSpawned = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)

	TArray<AActor*> StartInteractors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)

	TArray<AActor*> EndInteractors;

};
 
