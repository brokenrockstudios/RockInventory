// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/Transactions/RockInventoryTransactionManager.h"
#include "RockInventoryTransactionManagerComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ROCKINVENTORYRUNTIME_API URockInventoryTransactionManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URockInventoryTransactionManagerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RockInventory")
	TObjectPtr<URockInventoryTransactionManager> RockInventoryTransactionManager; 

};
