// Copyright Epic Games, Inc. All Rights Reserved.

#include "SimulationGameGameMode.h"
#include "SimulationGameCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASimulationGameGameMode::ASimulationGameGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
