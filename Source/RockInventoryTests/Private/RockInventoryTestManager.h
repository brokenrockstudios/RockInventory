// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/RockInventoryManagerComponent.h"

#include "RockInventoryTestManager.generated.h"

/**
 * A manager component for prediction and undo tests (T-78, T-80): it predicts although the test world has authority, keeps its own
 * clock, and holds every command it would send so the test decides when (and whether) the server sees it. The test then calls
 * Server_MoveItem_Implementation with the held command, as the RPC would. Drops and loots are held and never run.
 */
UCLASS(NotBlueprintable, NotPlaceable)
class URockInventoryTestManager : public URockInventoryManagerComponent
{
	GENERATED_BODY()

public:
	bool bPredict = true;
	double Now = 0.0;
	TArray<FRockMoveItemTransaction> Sent;
	TArray<FRockDropItemTransaction> SentDrops;
	TArray<FRockLootWorldItemTransaction> SentLoots;

protected:
	virtual bool ShouldPredict() const override { return bPredict; }
	virtual double GetPredictionTime() const override { return Now; }
	virtual void SendMove(const FRockMoveItemTransaction& Command) override { Sent.Add(Command); }
	virtual void SendDrop(const FRockDropItemTransaction& Command) override { SentDrops.Add(Command); }
	virtual void SendLoot(const FRockLootWorldItemTransaction& Command) override { SentLoots.Add(Command); }
};
