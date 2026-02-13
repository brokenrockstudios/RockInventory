// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemDefinition.h"
#include "RockInventoryDescriptor.generated.h"

// // We will build this out, and the widget will be data-driven based on this descriptor
USTRUCT(BlueprintType)
struct ROCKINVENTORYUI_API FRockItemDescriptor
{
	GENERATED_BODY()

	// Should we be using a copy of the item stack, or a handle and the inventory?
	// This is a read-only descriptor, so a copy is fine for now.
	// Later on we may want to support live updates, in which case a handle would be better.
	// We do our best to avoid actually using this directly, but it is here if needed.
	UPROPERTY()
	FRockItemStack ItemStackCopy;

	// Identity
	UPROPERTY()
	FName ItemId;
	UPROPERTY()
	FText DisplayName;
	UPROPERTY()
	FText Description;

	// Visuals (soft refs only)
	UPROPERTY()
	FRockItemUIData Icon;
	UPROPERTY()
	FVector2D IconSize = FVector2D(64, 64);
	UPROPERTY()
	FGameplayTag Rarity = FGameplayTag::EmptyTag;
	UPROPERTY()
	FLinearColor RarityColor = FLinearColor::White;
	UPROPERTY()
	FText RarityName;

	// Categorization
	UPROPERTY()
	FGameplayTagContainer ItemTags;
	UPROPERTY()
	int32 StackCount = 1;

	// Unused for now, we just push into FloatStats
	// e.g., Stats.Attack=12
	// UPROPERTY()
	// TMap<FGameplayTag, int32> IntStats;

	// Arbitrary facts / stats (flat, UI-agnostic)
	// e.g., Fuel.BurnSecs=30
	UPROPERTY()
	TMap<FGameplayTag, float> FloatStats;

	// e.g., Lore.Flavor="..."
	UPROPERTY()
	TMap<FGameplayTag, FText> Notes;

	//UPROPERTY()
	//TMap<FGameplayTag, FName> Tags;

	// 	// Helpers
	// 	bool TryGetFloatStat(FGameplayTag Tag, float& Out) const
	// 	{
	// 		if (const float* V = FloatStats.Find(Tag))
	// 		{
	// 			Out = *V;
	// 			return true;
	// 		}
	// 		return false;
	// 	}
};

// // Core — fragment extractor interface (no UMG types)
// class ROCKINVENTORYUI_API IRockFragmentExtractor
// {
// public:
// 	virtual ~IRockFragmentExtractor() = default;
// 	virtual UScriptStruct* ForFragmentType() const = 0; // identifies the fragment type
// 	virtual void Extract(const FRockItemFragment& Frag, FRockItemDescriptor& OutDesc) const = 0;
// };


USTRUCT()
struct ROCKINVENTORYUI_API FRockItemDescriptorBuilder
{
	GENERATED_BODY()

public:
	// Singleton-style access, optional (or inject into a subsystem)
	static FRockItemDescriptorBuilder& Get();

	/**
	 * Builds a presentation descriptor from the given item.
	 * @param Item   Item stack to build from (definition + fragments).
	 * @param OutDesc [out] Filled descriptor with typed fields + Extras.
	 * @return true if built successfully, false if item was invalid.
	 */
	bool Build(const FRockItemStack& Item, FRockItemDescriptor& OutDesc) const;

	/** 
	 * Register a fragment extractor. 
	 * Called at startup by modules that define new fragment→UI mappings.
	 */
	//void RegisterExtractor(TUniquePtr<class IRockFragmentExtractor> Extractor);

private:
	//TMap<UScriptStruct*, TArray<TUniquePtr<IRockFragmentExtractor>>> ExtractorMap;
};

// // Registry (core)
// class ROCKINVENTORYUI_API FRockExtractorRegistry
// {
// public:
// 	static FRockExtractorRegistry& Get();
//
// 	void Register(TUniquePtr<IRockFragmentExtractor> Extractor);
// 	const TArray<const IRockFragmentExtractor*>& Find(UScriptStruct* FragType) const;
//
// private:
// 	TMap<UScriptStruct*, TArray<TUniquePtr<IRockFragmentExtractor>>> Map;
// };
//


//
// void FRockInventoryRuntimeModule::StartupModule()
// {
// 	auto& R = FRockExtractorRegistry::Get();
// 	R.Register(MakeUnique<FSetStatsExtractor>());
// 	R.Register(MakeUnique<FFuelExtractor>());
// 	// ...register more extractors
// }
//
// // SetStats -> IntStats
// class FSetStatsExtractor : public IRockFragmentExtractor
// {
// public:
// 	UScriptStruct* ForFragmentType() const override { return FRockItemFragment_SetStats::StaticStruct(); }
// 	void Extract(const FRockItemFragment& Frag, FRockItemDescriptor& Out) const override
// 	{
// 		auto& S = static_cast<const FRockItemFragment_SetStats&>(Frag);
// 		for (const auto& KVP : S.InitialItemStats)
// 		{
// 			Out.IntStats.Add(KVP.Key, KVP.Value);
// 		}
// 	}
// };
//
// // FuelData -> FloatStats/TextFacts
// class FFuelExtractor : public IRockFragmentExtractor
// {
// public:
// 	UScriptStruct* ForFragmentType() const override { return FRockItemFragment_FuelData::StaticStruct(); }
// 	void Extract(const FRockItemFragment& Frag, FRockItemDescriptor& Out) const override
// 	{
// 		auto& F = static_cast<const FRockItemFragment_FuelData&>(Frag);
// 		Out.FloatStats.Add(FGameplayTag::RequestGameplayTag(TEXT("Fuel.Energy")),      F.EnergyContent);
// 		Out.FloatStats.Add(FGameplayTag::RequestGameplayTag(TEXT("Fuel.BurnSeconds")), F.BurnDuration);
// 	}
// };
//
// inline FRockItemDescriptor BuildItemDescriptor(const FRockItemStack& Item)
// {
// 	// FRockItemDescriptor D;
// 	// const URockItemDefinition* Def = Item.GetDefinition();
// 	// D.InternalId      = Def->GetFName();
// 	// D.DisplayName     = Def->DisplayName;
// 	// D.ShortDescription= Def->Description;
// 	// D.Icon            = Def->IconData;          // soft ref only
// 	// D.ItemTags        = Def->ItemTags;
// 	// D.Rarity    = Def->ItemRarity;
// 	//
// 	// D.StackCount      = Item.GetStackCount();
// 	//
// 	// for (const FRockItemFragmentInstance& Inst : Def->GetAllFragments())
// 	// {
// 	// 	if (const FRockItemFragment* Frag = Inst.Fragment.GetPtr<FRockItemFragment>())
// 	// 	{
// 	// 		
// 	// 		UScriptStruct* Type = Inst.Fragment.GetScriptStruct();
// 	// 		for (const IRockFragmentExtractor* X : FRockExtractorRegistry::Get().Find(Type))
// 	// 		{
// 	// 			X->Extract(*Frag, D);
// 	// 		}
// 	// 	}
// 	// }
// 	// return D;
// }
