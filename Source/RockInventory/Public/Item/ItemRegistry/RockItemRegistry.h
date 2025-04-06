// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RockItemRegistry.generated.h"

class URockItemDefinition;
/**
 * 
 */
UCLASS()
class ROCKINVENTORY_API URockItemRegistry : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface

	/**
	 * Finds an item definition by its unique ItemId.
	 *
	 * @param ItemID The FName identifier of the item definition to find.
	 * @return A pointer to the URockItemDefinition if found, otherwise nullptr.
	 */
	UFUNCTION(BlueprintPure, Category = "Item Registry") // Expose to Blueprint if needed
	URockItemDefinition* FindDefinition(FName ItemID) const;

	/**
	 * Gets all loaded item definitions.
	 * Useful for displaying all available items in UI or debug tools.
	 *
	 * @param OutDefinitions Array to populate with all found definitions.
	 */
	UFUNCTION(BlueprintPure, Category = "Item Registry") // Expose to Blueprint if needed
	void GetAllDefinitions(TArray<URockItemDefinition*>& OutDefinitions) const;


private:
	/** Map storing ItemId -> ItemDefinition associations for quick lookup. */
	UPROPERTY(Transient) // Transient as it's populated at runtime
	TMap<FName, TObjectPtr<URockItemDefinition>> ItemDefinitionMap;

	/** Primary Asset Type for URockItemDefinition as configured in Project Settings. */
	UPROPERTY() // Allow configuration via DefaultGame.ini if needed
	FName ItemDefinitionAssetType = FName("RockItemDefinition"); // Default to "RockItem", matches step 1

	/** Flag to track if the registry has been successfully initialized. */
	bool bIsInitialized = false;

	/** Internal function to scan and load item definitions using the Asset Manager. */
	void BuildRegistry();
	
};
