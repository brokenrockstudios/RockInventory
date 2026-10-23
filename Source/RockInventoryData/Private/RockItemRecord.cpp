// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockItemRecord.h"

#include "RockDataRegistry.h"
#include "RockDataTypeRegistrar.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RockItemRecord)

namespace RockItemData
{
	const FName ItemTypeName(TEXT("Item"));

	namespace RockItemRecordPrivate
	{
		void BakeItem(const FRockDataRegistry& /*Registry*/, const FRockDataRecord& /*Record*/, void* Definition, FRockDataBakeErrors& Errors)
		{
			const FRockItemRecord& Item = *static_cast<const FRockItemRecord*>(Definition);
			if (Item.MaxStackCount < 1)
			{
				Errors.Add(FString::Printf(TEXT("MaxStackCount: %d is below 1"), Item.MaxStackCount));
			}
			if (Item.GridSize.X < 1 || Item.GridSize.Y < 1)
			{
				Errors.Add(FString::Printf(TEXT("GridSize: %dx%d needs at least 1x1"), Item.GridSize.X, Item.GridSize.Y));
			}
			if (Item.Weight < 0)
			{
				Errors.Add(FString::Printf(TEXT("Weight: %lld is below 0"), Item.Weight));
			}
		}

		void DescribeItem(const void* Definition, FString& /*OutName*/, FString& OutDetail)
		{
			const FRockItemRecord& Item = *static_cast<const FRockItemRecord*>(Definition);
			OutDetail = FString::Printf(TEXT("Stack %d, %dx%d, %d fragments"), Item.MaxStackCount, Item.GridSize.X, Item.GridSize.Y, Item.Fragments.Num());
		}
	}

	ROCKDATA_DEFINE_TYPE(FRockItemRecord, "Item", "Items", "Name,DisplayName,Description", &RockItemRecordPrivate::BakeItem, &RockItemRecordPrivate::DescribeItem)
}
