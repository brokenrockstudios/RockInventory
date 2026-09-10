// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Item/State/RockItemState_Metadata.h"

#include "Item/RockItemInstance.h"

void FRockItemState_Metadata::OnStateAdded(URockItemInstance* OwnerInstance)
{
	FRockItemState::OnStateAdded(OwnerInstance);
	StatTags.SetListenerObject(OwnerInstance);
}

void FRockItemState_Metadata::OnStateRemoved(URockItemInstance* OwnerInstance)
{
	FRockItemState::OnStateRemoved(OwnerInstance);
	StatTags.SetListenerObject(nullptr);
}

bool FRockItemState_Metadata::HasTag(FGameplayTag Tag) const
{
	return Tags.HasTag(Tag);
}

void FRockItemState_Metadata::AddTag(FGameplayTag Tag)
{
	Tags.AddTag(Tag);
	NotifyChanged();
}

void FRockItemState_Metadata::RemoveTag(FGameplayTag Tag)
{
	Tags.RemoveTag(Tag);
	NotifyChanged();
}

int32 FRockItemState_Metadata::GetStatTagCount(FGameplayTag Tag) const
{ 
	return StatTags.GetStackCount(Tag);
}

void FRockItemState_Metadata::AddStatTagCount(FGameplayTag Tag, int32 StackCount, bool bKeepZeroStacks)
{
	StatTags.AddStack(Tag, StackCount, bKeepZeroStacks);
	// We don't need to explicitly call NotifyChanged because it's already called within the AddStack method of FGameplayTagStackContainer.
}

void FRockItemState_Metadata::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount, bool bKeepZeroStacks)
{
	StatTags.RemoveStack(Tag, StackCount, bKeepZeroStacks);
}

void FRockItemState_Metadata::SetStatTagCount(FGameplayTag Tag, int32 StackCount, bool bKeepZeroStacks)
{
	StatTags.SetStack(Tag, StackCount, bKeepZeroStacks);
}

bool FRockItemState_Metadata::ContainsTag(FGameplayTag Tag) const
{
	return Tags.HasTag(Tag);
}

