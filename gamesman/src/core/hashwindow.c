/************************************************************************
**
** NAME:	hashwindow.c
**
** DESCRIPTION:	Hash Window Support for Tier-Gamesman Modules
**
** AUTHOR:	Max Delgadillo
**		GamesCrafters Research Group, UC Berkeley
**		Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
**
** DATE:	2006-08-01
**
** LICENSE:	This file is part of GAMESMAN,
**		The Finite, Two-person Perfect-Information Game Generator
**		Released under the GPL:
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program, in COPYING; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
**************************************************************************/

#include "gamesman.h"

/* VARIABLES */
BOOLEAN	gHashWindowInitialized = FALSE;
BOOLEAN gGlobalHashMode = FALSE;
BOOLEAN gCurrentTierIsLoopy = FALSE;
TIER* gTierInHashWindow;
TIERPOSITION* gMaxPosOffset;
int gNumTiersInHashWindow;

/*
Hash Windows are set up like this:
Say we are setting a window for Tier 0. Calling TierChildren on it returns
(1, 2, 3, 0). First, Tier 0 is placed in the front of the list, and if the
list already contains 0, it is removed. So the result is (0, 1, 2, 3).
Thus, TierInHashWindow becomes = { -1(kBadTier), 0, 1, 2, 3 }.
Now, say the MaxPositions of the tiers are as follows:
0: 20 Positions (0 through 19)
1: 30 Positions (0 through 29)
2: 15 Positions (0 through 14)
3: 35 Positions (0 through 34)
Then, MaxPosOffset becomes = { 0, 20, 50, 65, 100 }.
gNumberOfPositions is set to 100.

Now in this window, we know 0-19 is Tier 0, 20-49 is Tier 1, 50-64 is Tier 2,
and 65-99 is Tier 3. Subtract the appropriate offsets and you get the original
MaxPositions for the tiers.

Booyakasha.
-Max
*/

/* FOR MODULES TO CALL */

// Called by "Unhash".
void gUnhashToTierPosition(POSITION position, TIERPOSITION* tierposition,
												TIER* tier) {
	if (!gHashWindowInitialized) {
		printf("ERROR: Hash Window is not initialized!\n");
		ExitStageRight();
	} if (position < 0 || position >= gNumberOfPositions) {
		printf("ERROR: Hash Window function \"gUnhashToTierPosition\" called with illegal POSITION.\n");
		ExitStageRight();
	}
	// since we know position is legal, this works:
	int i;
	for(i = 1; i < gNumTiersInHashWindow; i++) {
		if (position < gMaxPosOffset[i]) {//in THIS tier
			(*tierposition) = position;
			(*tier) = gTierInHashWindow[i];
			return;
		}
		position -= gMaxPosOffset[i];
	}
}

// Call by "Hash".
POSITION gHashToWindowPosition(TIERPOSITION tierposition, TIER tier) {
	if (!gHashWindowInitialized) {
		printf("ERROR: Hash Window is not initialized!\n");
		ExitStageRight();
	}
	int i;
	for(i = 1; i < gNumTiersInHashWindow; i++) {
		if (gTierInHashWindow[i] == tier) {
			POSITION position = tierposition += gMaxPosOffset[i-1];
			if (position < 0 || position > gMaxPosOffset[i]) {
				printf("ERROR: Hash Window function \"gHashToWindowPosition\" called with illegal TIERPOSITION.\n");
				ExitStageRight();
			}
			return position;
		}
	}
	// shouldn't be reached. So, error:
	printf("ERROR: Hash Window function \"gHashToWindowPosition\" called with illegal TIER.\n");
	ExitStageRight();
	return 0;
}

/* FOR THE CORE (SOLVER/GAMESMAN) TO CALL */
void gInitializeHashWindow(TIER tier) {
	gHashWindowInitialized = TRUE;
	TIERPOSITION tierpos; TIER tierOfTierpos;
	POSITION position = kBadPosition; // get rid of
	if (position != kBadPosition)
		gUnhashToTierPosition(position, &tierpos, &tierOfTierpos);
	// Free old stuff
	if (gMaxPosOffset != NULL) SafeFree(gMaxPosOffset);
	if (gTierInHashWindow != NULL) SafeFree(gTierInHashWindow);

	// Start by seeing what children go here
	TIERLIST *children, *ptr, *back;
	ptr = gTierChildrenFunPtr(tier);

	// First, we find the list's size for malloc'ing, and insert TIER in its place.
	gCurrentTierIsLoopy = FALSE;
	int childrenCount = 1;
	ptr = CreateTierlistNode(tier, ptr);
	back = ptr; children = ptr->next;
	for (; children != NULL; children = children->next, back = back->next) {
		if (children->tier == tier) {
			back->next = children->next;
			gCurrentTierIsLoopy = TRUE;
		} else childrenCount++;
	}
	gNumTiersInHashWindow = childrenCount+1;
	gTierInHashWindow = (TIER*) SafeMalloc (gNumTiersInHashWindow * sizeof(TIER));
	gMaxPosOffset = (TIERPOSITION*) SafeMalloc (gNumTiersInHashWindow * sizeof(TIERPOSITION));

	// Now we go through the list a second time to set the arrays.
	children = ptr;
	gTierInHashWindow[0] = kBadTier;
	gMaxPosOffset[0] = 0;
	int i;
	for (i = 1; i < gNumTiersInHashWindow; children = children->next, i++) {
		gTierInHashWindow[i] = children->tier;
		gMaxPosOffset[i] = gNumberOfTierPositionsFunPtr(children->tier)+gMaxPosOffset[i-1];
	}
	// set gNumberOfPositions
	gNumberOfPositions = gMaxPosOffset[gNumTiersInHashWindow-1];
	FreeTierList(ptr);
	if (position != kBadPosition)
		position = gHashToWindowPosition(tierpos, tierOfTierpos);
	position = kBadPosition;
}