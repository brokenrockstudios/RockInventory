// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"
#include "UObject/GCObject.h"

class FRockDataRegistry;
class URockItemDefinition;

/**
 * Builds one URockItemDefinition per concrete `Item` record of a RockData registry and offers them to the item registry.
 *
 * Objects are created in the item registry (outer) with the name `Item_<ID>` and rewritten in place by Sync(), so a pointer to a
 * definition survives a reload. A record that goes away drops its definition from the provider. Stacks are not touched: when a
 * rewrite lowers MaxStackCount or grows GridSize, one warning names the item, because stacks that are already placed keep their
 * count and footprint.
 *
 * EnumerateItemDefinitions() syncs first, so the item registry always reads the records as they are when it refreshes.
 */
class ROCKINVENTORYDATA_API FRockItemRecordProvider : public IRockItemDefinitionProvider, public FGCObject
{
public:
	/** Source is read on every Sync() and must outlive the provider. Outer owns the created objects. */
	FRockItemRecordProvider(const FRockDataRegistry& InSource, UObject* InOuter);

	/** Creates, rewrites and drops definitions to match the source. Returns true when a definition was created or dropped. */
	bool Sync();

	/** The definition built for a record ID, or null. Does not sync. */
	URockItemDefinition* FindDefinition(FName ItemId) const;

	//~ IRockItemDefinitionProvider
	virtual void EnumerateItemDefinitions(TArray<URockItemDefinition*>& OutDefinitions) const override;
	virtual FString DescribeItemSource(const URockItemDefinition* Definition) const override;

	//~ FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FRockItemRecordProvider"); }

private:
	const FRockDataRegistry& Source;
	TWeakObjectPtr<UObject> Outer;
	TMap<FName, TObjectPtr<URockItemDefinition>> Definitions;
};
