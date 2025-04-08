// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Settings/RockInventoryDeveloperSettings.h"
#include "World/RockInventoryWorldItem.h"
#include "Misc/DataValidation.h"

URockInventoryDeveloperSettings::URockInventoryDeveloperSettings(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
}

#if WITH_EDITOR
EDataValidationResult URockInventoryDeveloperSettings::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (DefaultWorldItemClass == nullptr)
	{
		Result = EDataValidationResult::Invalid;
		Context.AddError(FText::Format(NSLOCTEXT("RockInventory", "DefaultWorldItemClass", "DefaultWorldItemClass is not set.")));
	}

	return Result;
}
#endif
