// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Item/RockItemDefinition.h"

#include "RockInventoryLogging.h"
#include "Engine/AssetManager.h"
#include "Misc/RockInventoryDeveloperSettings.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/AssetRegistryTagsContext.h"

const TArray<FInstancedStruct>& URockItemDefinition::GetAllFragments() const
{
	return Fragments;
}

const FGameplayTagContainer& URockItemDefinition::GetAllTags() const
{
	return CachedAllTags;
}

FPrimaryAssetId URockItemDefinition::GetPrimaryAssetId() const
{
	if (ItemId != NAME_None)
	{
		return FPrimaryAssetId("RockItemDefinition", ItemId);
	}
	return FPrimaryAssetId("RockItemDefinition", GetFName());
}

void URockItemDefinition::RegisterItemDefinition(const URockItemDefinition* NewItem)
{
	checkf(false, TEXT("RegisterItemDefinition is not tested yet!"));

	if (!NewItem)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("RegisterItemDefinition called with null item."));
		return;
	}

	const FPrimaryAssetId AssetId = NewItem->GetPrimaryAssetId();
	if (!AssetId.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("RegisterItemDefinition: Invalid PrimaryAssetId for item %s."), *NewItem->GetName());
		return;
	}

	const FSoftObjectPath ItemPath(NewItem);
	if (!ItemPath.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("RegisterItemDefinition: Invalid SoftObjectPath for item %s."), *NewItem->GetName());
		return;
	}

	// Empty bundle data (optional, unless you're using bundled loading)
	const FAssetBundleData EmptyBundleData;
	UAssetManager::Get().AddDynamicAsset(AssetId, ItemPath, EmptyBundleData);

	// Consider manually registering with RockItemRegistrySubsystem if needed
	// URockItemRegistrySubsystem::GetInstance()->RegisterRuntimeItemDefinition(NewItem);

	UE_LOG(LogRockInventory, Warning, TEXT("Registered item %s with ID %s"), *NewItem->GetName(), *AssetId.ToString());
}

UClass* URockItemDefinition::GetWorldItemClass() const
{
	// If you need different behavior, make a subclass of URockItemDefinition and override this function.
	// Such as adding a fragment lookup or having the data be part of ItemDefinition.
	return GetDefault<URockInventoryDeveloperSettings>()->DefaultWorldItemClass;
}

void URockItemDefinition::RebuildStatTags()
{
	StatTags = FGameplayTagStackContainer();
	for (const FGameplayTagStack& StatTag : StatTagDefaults)
	{
		if (StatTag.GetTag().IsValid())
		{
			StatTags.SetStack(StatTag.GetTag(), StatTag.GetStackCount(), true);
		}
		else
		{
			UE_LOG(LogRockInventory, Warning, TEXT("RebuildStatTags - Invalid tag in StatTagDefaults for item %s"), *GetName());
		}
	}
}

void URockItemDefinition::RebuildCachedTags()
{
	CachedAllTags.Reset();;
	CachedAllTags.AppendTags(ItemType);
	CachedAllTags.AppendTags(ItemTags);
}

void URockItemDefinition::SortFragments()
{
	Fragments.StableSort(
		[](const FInstancedStruct& A, const FInstancedStruct& B)
		{
			const FRockItemFragment* FragA = A.GetPtr<FRockItemFragment>();
			const FRockItemFragment* FragB = B.GetPtr<FRockItemFragment>();
			const int32 PriorityA = FragA ? FragA->GetSortOrder() : 0;
			const int32 PriorityB = FragB ? FragB->GetSortOrder() : 0;
			return PriorityA < PriorityB;
		});
}

static FString StripBeforeFirstUnderscore(FString FragmentName)
{
	int32 UnderscoreIdx;
	if (FragmentName.FindChar(TEXT('_'), UnderscoreIdx))
	{
		FragmentName = FragmentName.RightChop(UnderscoreIdx + 1);
	}
	return FragmentName;
}

void URockItemDefinition::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	Super::GetAssetRegistryTags(Context);

	Context.AddTag(FAssetRegistryTag(FPrimaryAssetId::PrimaryAssetDisplayNameTag, ItemId.ToString(), FAssetRegistryTag::TT_Alphabetical));
	Context.AddTag(FAssetRegistryTag("FragmentCount", FString::FromInt(Fragments.Num()), FAssetRegistryTag::TT_Numerical));
	TStringBuilder<256> FragmentTypes;
	for (const FInstancedStruct& FragmentInstance : GetAllFragments())
	{
		if (const FRockItemFragment* Fragment = FragmentInstance.GetPtr<FRockItemFragment>())
		{
			FragmentTypes.Append(StripBeforeFirstUnderscore(FragmentInstance.GetScriptStruct()->GetName()));
			FragmentTypes.Append(TEXT(","));
			Fragment->GetAssetRegistryTags(Context);
		}
	}
	if (FragmentTypes.Len() > 0)
	{
		// Remove trailing comma
		FragmentTypes.RemoveSuffix(1);
	}
	Context.AddTag(
		FAssetRegistryTag(
			TEXT("ItemFragment"),
			FragmentTypes.ToString(),
			FAssetRegistryTag::TT_Alphabetical
		));
}


void URockItemDefinition::SetDefaultItemId()
{
	if (ItemId == NAME_None)
	{
		ItemId = GetFName();
		UE_LOG(LogRockInventory, Warning, TEXT("SetDefaultItemId - ItemId was not set for item %s, defaulting to %s"), *GetName(), *ItemId.ToString());
	}
}

void URockItemDefinition::PostLoad()
{
	Super::PostLoad();
	for (FInstancedStruct& Fragment : Fragments)
	{
		if (FRockItemFragment* FragmentPtr = Fragment.GetMutablePtr<FRockItemFragment>())
		{
			FragmentPtr->OnPostLoad(this);
		}
	}
	SetDefaultItemId();
	RebuildStatTags();
	RebuildCachedTags();
}

#if WITH_EDITOR

EDataValidationResult URockItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	for (const FInstancedStruct& fragment : Fragments)
	{
		if (const FRockItemFragment* data = fragment.GetPtr<FRockItemFragment>())
		{
			Result = CombineDataValidationResults(Result, data->IsDataValid(Context, this));
		}
	}
	return Result;
}

void URockItemDefinition::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberPropertyName = PropertyChangedEvent.GetMemberPropertyName();

	static const FName FragmentsName = GET_MEMBER_NAME_CHECKED(URockItemDefinition, Fragments);
	if (MemberPropertyName == FragmentsName)
	{
		for (FInstancedStruct& Fragment : Fragments)
		{
			if (FRockItemFragment* Data = Fragment.GetMutablePtr<FRockItemFragment>())
			{
				Data->OnPostEditChangeProperty(this);
			}
		}
	}

	static const FName StatTagDefaultsName = GET_MEMBER_NAME_CHECKED(URockItemDefinition, StatTagDefaults);
	if (PropertyName == StatTagDefaultsName || MemberPropertyName == StatTagDefaultsName)
	{
		RebuildStatTags();
	}

	static const FName ItemTypeName = GET_MEMBER_NAME_CHECKED(URockItemDefinition, ItemType);
	static const FName ItemTagsName = GET_MEMBER_NAME_CHECKED(URockItemDefinition, ItemTags);
	if (PropertyName == ItemTypeName || MemberPropertyName == ItemTypeName ||
		PropertyName == ItemTagsName || MemberPropertyName == ItemTagsName)
	{
		RebuildCachedTags();
	}
}
#endif
