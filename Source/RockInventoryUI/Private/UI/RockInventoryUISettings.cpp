// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "RockInventoryUISettings.h"

#include "InputCoreTypes.h"

URockInventoryUISettings::URockInventoryUISettings()
{
	UndoChords = {FInputChord(EKeys::Z, /*bShift*/ false, /*bCtrl*/ true, /*bAlt*/ false, /*bCmd*/ false)};
	RedoChords = {
		FInputChord(EKeys::Y, /*bShift*/ false, /*bCtrl*/ true, /*bAlt*/ false, /*bCmd*/ false),
		FInputChord(EKeys::Z, /*bShift*/ true, /*bCtrl*/ true, /*bAlt*/ false, /*bCmd*/ false),
	};
}
