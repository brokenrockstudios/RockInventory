// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventory.h"

#define LOCTEXT_NAMESPACE "FRockInventoryModule"

void FRockInventoryModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FRockInventoryModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRockInventoryModule, RockInventory)