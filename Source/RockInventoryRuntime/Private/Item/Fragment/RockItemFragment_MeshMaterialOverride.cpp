// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Item/Fragment/RockItemFragment_MeshMaterialOverride.h"

#include "Item/RockItemDefinition.h"

void FRockItemFragment_MeshMaterialOverride::ApplyTo(UMeshComponent* Mesh) const
{
	if (!Mesh) { return; }

	for (int32 i = 0; i < SlotOverrides.Num(); i++)
	{
		const int32 MaterialIndex = SlotOverrides[i].SlotIndex;
		// Consider alternative if you need async
		UMaterialInterface* MaterialOverride = SlotOverrides[i].Material.Get();
		if (MaterialOverride)
		{
			Mesh->SetMaterial(MaterialIndex, MaterialOverride);
		}
	}
}

void FRockItemFragment_MeshMaterialOverride::AppendSoftPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	for (const FRockMaterialSlotOverride& Override : SlotOverrides)
	{
		if (!Override.Material.IsNull())
		{
			OutPaths.Add(Override.Material.ToSoftObjectPath());
		}
	}
}

const FRockItemFragment_MeshMaterialOverride* FRockItemFragment_MeshMaterialOverride::Find(const URockItemDefinition* Definition)
{
	return Definition ? Definition->FindFragment<FRockItemFragment_MeshMaterialOverride>() : nullptr;
}
