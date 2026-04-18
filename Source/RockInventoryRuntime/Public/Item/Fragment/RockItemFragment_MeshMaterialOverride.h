// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemFragment.h"
#include "UObject/Object.h"
#include "RockItemFragment_MeshMaterialOverride.generated.h"

USTRUCT()
struct FRockMaterialSlotOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 SlotIndex = 0;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UMaterialInterface> Material;
};

USTRUCT()
struct ROCKINVENTORYRUNTIME_API FRockItemFragment_MeshMaterialOverride : public FRockItemFragment
{
	GENERATED_BODY()
public:
	// Sparse - null entries mean "leave slot alone"
	UPROPERTY(EditAnywhere)
	TArray<FRockMaterialSlotOverride> SlotOverrides;

	void ApplyTo(UMeshComponent* Mesh) const;
	void AppendSoftPaths(TArray<FSoftObjectPath>& OutPaths) const;
	
	static const FRockItemFragment_MeshMaterialOverride* Find(const URockItemDefinition* Definition);
};
