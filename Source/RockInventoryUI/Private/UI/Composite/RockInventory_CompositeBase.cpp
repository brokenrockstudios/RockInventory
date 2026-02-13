// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "UI/Composite/RockInventory_CompositeBase.h"

void URockInventory_CompositeBase::Collapse()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void URockInventory_CompositeBase::Expand()
{
	SetVisibility(ESlateVisibility::Visible);
}
