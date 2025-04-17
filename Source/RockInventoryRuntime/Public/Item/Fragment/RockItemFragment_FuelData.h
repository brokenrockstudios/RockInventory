// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemFragment.h"
#include "UObject/Object.h"
#include "RockItemFragment_FuelData.generated.h"

// FuelData fragment
// This fragment is used to define the fuel properties of an item.
// Generally this should be game specific, but for now we will use an example. 

enum class EReactionType
{
	Standard, // Fixed energy output
	Catalytic, // Modifies a burning fuel
	Composite // Combines multiple fuel sources
};

/**
 * Fuel Data
 * This is a fragment that can be attached to an item to define its fuel properties.
 * It can be used to define how much energy it produces when burned, and how long it burns for.
 * 
 * Example Values
 * - 8 MJ | Coal (burns over 16 seconds)
 * - 4 MJ | Wood (burns over 8 second) 
 * - 1 MJ | Plant Fiber (burns over 4 seconds
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemFragment_FuelData : public FRockItemFragment
{
	GENERATED_BODY()

public:
	// total KJ per count
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fuel")
	float EnergyContent;
	// Total burn time
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fuel")
	float BurnDuration;
	// The temperature of the fuel output
	// Fixed 300 C for now if/where it matters

	// PowerOutput kJ/s
	float GetPowerOutputPerSecond() const
	{
		return EnergyContent / FMath::Max(BurnDuration, 0.001f);
	}
};


// Example ReactionData
USTRUCT(BlueprintType)
struct FReactantEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<URockItemDefinition> ItemDef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity;
};

USTRUCT(BlueprintType)
struct FProductEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<URockItemDefinition> ItemDef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity;

	// TODO: Optionally add product-specific properties here
	// For example, if different products can have different temperatures:
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Units = "Celsius"))
	// float ProductTemperature;
};


USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemFragment_ReactionData : public FRockItemFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
	TArray<FReactantEntry> Reactants;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
	TArray<FProductEntry> Products;

	// Total Energy.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
	float EnergyOutput;
	// Internally we always use Celsius
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction", meta=(Units = "Celsius"))
	float OutputTemperature;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
	float BaseReactionTime;


	// Optional temperature range for the reaction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
	float TemperatureMin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
	float TemperatureMax;
};
