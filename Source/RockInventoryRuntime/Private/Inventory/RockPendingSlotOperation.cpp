#include "Inventory/RockPendingSlotOperation.h"


bool FRockPendingSlotOperation::CanClaimSlot(const FRockPendingSlotOperation& SlotOperation)
{
	switch (SlotOperation.SlotStatus)
	{
	case ERockSlotStatus::Empty:
		//case ERockSlotStatus::Occupied:
		return true;
	case ERockSlotStatus::Pending:
		//case ERockSlotStatus::PendingEmpty:
		//case ERockSlotStatus::PendingIncoming:
		return false;
	default:
		return false;
	}
}

bool FRockPendingSlotOperation::IsClaimedByOther(AController* OtherController) const
{
	return SlotStatus == ERockSlotStatus::Pending && Controller != OtherController;
}
