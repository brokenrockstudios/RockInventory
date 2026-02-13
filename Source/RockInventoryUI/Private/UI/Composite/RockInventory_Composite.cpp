// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "UI/Composite/RockInventory_Composite.h"

#include "Blueprint/WidgetTree.h"

void URockInventory_Composite::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		URockInventory_CompositeBase* Composite = Cast<URockInventory_CompositeBase>(Widget);
		if (IsValid(Composite))
		{
			Children.Add(Composite);
			Composite->Collapse();
		}
	});
}

void URockInventory_Composite::ApplyFunction(FuncType Function)
{
	for (URockInventory_CompositeBase* Child : Children)
	{
		if (IsValid(Child))
		{
			Child->ApplyFunction(Function);
		}
	}
}

void URockInventory_Composite::Collapse()
{
	for (URockInventory_CompositeBase* Child : Children)
	{
		if (IsValid(Child))
		{
			Child->Collapse();
		}
	}
}
