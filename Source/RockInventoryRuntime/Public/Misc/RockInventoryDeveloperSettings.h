// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RockInventoryDeveloperSettings.generated.h"


UENUM(BlueprintType)
enum class ERockThumbnailMode : uint8
{
	Default, // Standard DataAsset icon
	Mesh, // Render the primary static mesh
	Icon, // Use explicit icon texture
	Auto, // Icon > Mesh > Default (first available)
};

/**
 * 
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "RockInventory"))
class ROCKINVENTORYRUNTIME_API URockInventoryDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	URockInventoryDeveloperSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void PostInitProperties() override;

	UPROPERTY(EditDefaultsOnly, Config, Category = "RockInventory")
	TSubclassOf<AActor> DefaultWorldItemClass;

	UPROPERTY(EditDefaultsOnly, Config, Category = "RockInventory|Visuals")
	TSoftObjectPtr<UStaticMesh> FallbackWorldItemMesh;

	UPROPERTY(EditAnywhere, Config, Category = "Thumbnail")
	ERockThumbnailMode ItemDefinitionThumbnailMode = ERockThumbnailMode::Default;

#if WITH_EDITOR
	// data validator
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
