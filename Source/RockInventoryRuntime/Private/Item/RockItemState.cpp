// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Item/RockItemState.h"

#include "Item/RockItemInstance.h"

void FRockItemState::OnStateAdded(URockItemInstance* OwnerInstance)
{
}

void FRockItemState::OnStateRemoved(URockItemInstance* OwnerInstance)
{
}

void FRockItemState::NotifyChanged()
{
	if (URockItemInstance* Instance = OwningInstance.Get())
	{
		Instance->NotifyStateChanged();
	}
}