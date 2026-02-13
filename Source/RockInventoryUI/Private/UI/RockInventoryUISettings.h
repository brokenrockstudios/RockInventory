// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RockInventoryUISettings.generated.h"

/**
 * 
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Rock Inventory: UI"))
class ROCKINVENTORYUI_API URockInventoryUISettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	// Project Settings → “Rock Inventory: Tooltip”
	UPROPERTY(config, EditDefaultsOnly, Category="UI")
	TSoftClassPtr<UUserWidget> TooltipLayerClass;

	UPROPERTY(config, EditDefaultsOnly, Category="UI")
	TSoftClassPtr<UUserWidget> TooltipDefaultContentClass;
	
	// Optional future knobs:
	UPROPERTY(config, EditDefaultsOnly, Category="Behavior")
	float DefaultDelaySeconds = 0.05f;
	
};
