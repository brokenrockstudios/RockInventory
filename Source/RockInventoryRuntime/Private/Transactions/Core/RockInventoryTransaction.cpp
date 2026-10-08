// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Transactions/Core/RockInventoryTransaction.h"

FRockItemTransactionBase::FRockItemTransactionBase()
{
}

FRockItemTransactionBase::FRockItemTransactionBase(AController* controller)
	: Instigator(controller)
{
}
