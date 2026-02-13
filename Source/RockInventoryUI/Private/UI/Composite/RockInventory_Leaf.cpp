// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "UI/Composite/RockInventory_Leaf.h"

void URockInventory_Leaf::ApplyFunction(FuncType Function)
{
	Function(this);
}
