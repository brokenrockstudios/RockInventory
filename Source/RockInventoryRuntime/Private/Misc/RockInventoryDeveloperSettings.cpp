// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Misc/RockInventoryDeveloperSettings.h"
#include "Misc/DataValidation.h"
#include "World/RockInventoryWorldItem.h"

#define LOCTEXT_NAMESPACE "RockInventory"

URockInventoryDeveloperSettings::URockInventoryDeveloperSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void URockInventoryDeveloperSettings::PostInitProperties()
{
	Super::PostInitProperties();
	FallbackWorldItemMesh.LoadSynchronous();
}

#if WITH_EDITOR
EDataValidationResult URockInventoryDeveloperSettings::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (DefaultWorldItemClass == nullptr)
	{
		Result = EDataValidationResult::Invalid;
		Context.AddError(FText(LOCTEXT("DefaultWorldItemClass", "DefaultWorldItemClass is not set.")));
	}

	if (FallbackWorldItemMesh != nullptr)
	{
		Result = EDataValidationResult::Invalid;
		Context.AddError(FText(LOCTEXT("FallbackWorldItemMesh", "FallbackWorldItemMesh is not set.")));
	}

	return Result;
}
#endif


#undef LOCTEXT_NAMESPACE
