// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FRockInventoryRuntimeModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
