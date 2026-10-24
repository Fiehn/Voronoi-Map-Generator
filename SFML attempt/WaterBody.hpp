#pragma once
#include <vector>

// A lake from the hydrology step. Steady state only: the level is where the water the basin receives
// balances what its surface evaporates, or the spill level when it receives more and the rest leaves
// over the outlet. Flows are in m3/s
struct WaterBody {
	std::vector<int> cells;     // Cells under the surface, lowest first
	float level = 0.f;          // Surface height (map height units)
	float minHeight = 0.f;      // Height of the lowest cell
	float depthM = 0.f;         // Level above the lowest cell, in metres
	float areaKm2 = 0.f;
	float inflow = 0.f;         // Everything the basin receives, including spills from sibling basins
	float evaporation = 0.f;
	float outflow = 0.f;        // What leaves over the outlet, 0 for a closed basin
	int outlet = -1;            // The pass the water leaves over, -1 when it does not
	int outletTarget = -1;      // The cell on the far side of the pass
	std::vector<int> inflowRivers; // Rivers (globals.rivers) that end in this lake
	int outflowRiver = -1;      // The river that starts at the outlet, -1 if there is none
	bool full = false;          // At its spill level, otherwise a closed basin (endorheic)
};
