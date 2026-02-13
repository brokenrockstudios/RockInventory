// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Utils/RockInventoryWidgetUtilLibrary.h"

int32 URockInventoryWidgetUtilLibrary::GetIndexFromPosition(const FIntPoint& Position, const int32 Columns)
{
	return Position.X + Position.Y * Columns;
}

FIntPoint URockInventoryWidgetUtilLibrary::GetPositionFromIndex(const int32 Index, const int32 Columns)
{
	return FIntPoint(Index % Columns, Index / Columns);
}
