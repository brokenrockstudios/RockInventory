// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Item/Fragment/RockItemFragment_Sound.h"

const USoundBase* FRockItemFragment_Sound::GetWorldPickup() const
{
	if (!WorldPickup.IsNull()) { return WorldPickup.Get(); }
	if (Preset) { return Preset->WorldPickup.Get(); }
	return nullptr;
}

const USoundBase* FRockItemFragment_Sound::GetWorldDrop() const
{
	if (!WorldDrop.IsNull()) { return WorldDrop.Get(); }
	if (Preset) { return Preset->WorldDrop.Get(); }
	return nullptr;
}

const USoundBase* FRockItemFragment_Sound::GetInventoryPickup() const
{
	if (!InventoryDrop.IsNull()) { return InventoryDrop.Get(); }
	if (Preset) { return Preset->InventoryDrop.Get(); }
	return nullptr;
}

const USoundBase* FRockItemFragment_Sound::GetInventoryMove() const
{
	if (!InventoryMove.IsNull()) { return InventoryMove.Get(); }
	if (Preset) { return Preset->InventoryMove.Get(); }
	return nullptr;
}

const USoundBase* FRockItemFragment_Sound::GetInventoryDrop() const
{
	if (!InventoryDrop.IsNull()) { return InventoryDrop.Get(); }
	if (Preset) { return Preset->InventoryDrop.Get(); }
	return nullptr;
}
