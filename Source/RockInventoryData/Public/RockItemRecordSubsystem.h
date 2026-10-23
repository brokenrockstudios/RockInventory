// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"

#include "RockItemRecordSubsystem.generated.h"

class FRockItemRecordProvider;
struct FRockDataKey;

/** Registers the game-wide RockData registry's `Item` records with the item registry and follows RockData reloads. */
UCLASS()
class ROCKINVENTORYDATA_API URockItemRecordSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void OnRecordsReloaded(const TArray<FRockDataKey>& Changed);

	TUniquePtr<FRockItemRecordProvider> Provider;
	FDelegateHandle ReloadedHandle;
	FDelegateHandle PostEngineInitHandle;
};
