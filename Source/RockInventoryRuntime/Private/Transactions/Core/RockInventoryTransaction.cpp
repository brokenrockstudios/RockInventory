// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Transactions/Core/RockInventoryTransaction.h"

namespace RockInventoryTransaction::Internal
{
static std::atomic<int32> GRockTransactionCount{1};
// This is a static counter to ensure each transaction has a unique ID
// This is not thread safe, but we don't expect to have multiple transactions created at the same time.
// If we do, we should probably use a mutex or some other synchronization method.
// But for now, this is good enough.
}

FRockItemTransactionBase::FRockItemTransactionBase()
{
}

FRockItemTransactionBase::FRockItemTransactionBase(AController* controller)
	: Instigator(controller)
{
}

void FRockItemTransactionBase::GenerateNewHandle()
{
	// Generate a new handle for the transaction
	TransactionID = RockInventoryTransaction::Internal::GRockTransactionCount.fetch_add(1);
}
