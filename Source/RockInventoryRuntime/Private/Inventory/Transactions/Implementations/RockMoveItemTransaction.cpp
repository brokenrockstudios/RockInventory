


#include "Inventory/Transactions/Implementations/RockMoveItemTransaction.h"

URockMoveItemTransaction* URockMoveItemTransaction::CreateMoveItemTransaction(
	URockInventory* InSourceInventory, const FRockInventorySlotHandle& InSourceSlotHandle,
	URockInventory* InTargetInventory, const FRockInventorySlotHandle& InTargetSlotHandle)
{
	URockMoveItemTransaction* Transaction = NewObject<URockMoveItemTransaction>();
	Transaction->SourceInventory = InSourceInventory;
	Transaction->SourceSlotHandle = InSourceSlotHandle;
	Transaction->TargetInventory = InTargetInventory;
	Transaction->TargetSlotHandle = InTargetSlotHandle;

	return Transaction;
}
