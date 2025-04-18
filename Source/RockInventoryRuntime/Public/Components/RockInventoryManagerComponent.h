// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RockInventoryManagerComponent.generated.h"


class URockInventoryManager;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ROCKINVENTORYRUNTIME_API URockInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URockInventoryManagerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RockInventory")
	TObjectPtr<URockInventoryManager> TransactionManager;
};
