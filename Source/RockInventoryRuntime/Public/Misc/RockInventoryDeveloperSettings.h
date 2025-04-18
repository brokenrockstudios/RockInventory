// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RockInventoryDeveloperSettings.generated.h"

class ARockInventoryWorldItem;
/**
 * 
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "RockInventory"))
class ROCKINVENTORYRUNTIME_API URockInventoryDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	URockInventoryDeveloperSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, Config, Category = "RockInventory")
	TSubclassOf<ARockInventoryWorldItem> DefaultWorldItemClass;


#if WITH_EDITOR
	// data validator
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
