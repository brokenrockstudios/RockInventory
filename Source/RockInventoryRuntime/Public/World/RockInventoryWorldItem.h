// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockWorldItemInterface.h"
#include "GameFramework/Actor.h"
#include "Item/RockItemStack.h"
#include "RockInventoryWorldItem.generated.h"

// Note: This is only an example of a WorldItem.
// You can use any actor with the added interface or optionally also replace the Transactions with your own
UCLASS(Abstract)
class ROCKINVENTORYRUNTIME_API ARockInventoryWorldItemBase : public AActor, public IRockWorldItemInterface
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	// Sets default values for this actor's properties
	ARockInventoryWorldItemBase(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

	// IRockWorldItemInterface
	virtual FRockItemStack GetItemStack(AActor *InstigatorPawn) const override;
	virtual void SetItemStack(const FRockItemStack &InItemStack) override;
	virtual void OnPickedUp(AActor *InInstigator) override;
	virtual void OnLooted(AActor *InstigatorPawn, const FRockItemStack &LootedItem, int32 Excess) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ItemStack)
	FRockItemStack ItemStack;

	UFUNCTION()
	void OnRep_ItemStack();

#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif
};
