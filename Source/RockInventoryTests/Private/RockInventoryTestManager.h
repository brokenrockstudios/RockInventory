// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/RockInventoryManagerComponent.h"

#include "RockInventoryTestManager.generated.h"

/**
 * A manager component for prediction tests (T-78): it predicts although the test world has authority, keeps its own clock, and holds
 * every move it would send so the test decides when (and whether) the server sees it. The test then calls
 * Server_MoveItem_Implementation with the held command, as the RPC would.
 */
UCLASS(NotBlueprintable, NotPlaceable)
class URockInventoryTestManager : public URockInventoryManagerComponent
{
	GENERATED_BODY()

public:
	bool bPredict = true;
	double Now = 0.0;
	TArray<FRockMoveItemTransaction> Sent;

protected:
	virtual bool ShouldPredict() const override { return bPredict; }
	virtual double GetPredictionTime() const override { return Now; }
	virtual void SendMove(const FRockMoveItemTransaction& Command) override { Sent.Add(Command); }
};
