#pragma once
#include <string>
#include "SphereVoronoi.hpp"
#include "GlobalWorldObjects.hpp"
#include "mapconfig.hpp"

// What the hydrology step found, for the log and the console harness
struct HydrologyStats {
	int nodes = 0;             // Depressions in the merge tree (every local minimum and every merge of them)
	int leaves = 0;            // The local minima
	int viableNodes = 0;       // Deep and wide enough to hold a lake
	int lakes = 0;             // Lakes that got water
	int closedLakes = 0;       // Of those, the ones that do not reach their spill level
	int passThroughRegions = 0; // Depressions too small to be lakes, the water is carried across them
	double landRunoff = 0.0;   // m3/s
	double lakeEvaporation = 0.0;
	double toOcean = 0.0;      // What reaches the ocean cells
	double lost = 0.0;         // Overflow of a planet without an ocean
	double deadEnd = 0.0;      // Discharge that ends in a land cell that is not under a lake, should be 0
	int cyclicCells = 0;       // Cells whose downstream links loop, should be 0
	std::string report;
};

// Steady state water balance of the land, in one pass over the cells sorted by height:
// 1. Runoff per cell from precipitation and temperature (Budyko, Fu's form), see Cell::runoff
// 2. A merge tree of the depressions: cells joined lowest first, a cell that joins two basins is the pass
//    between them, the pass of a basin and the ocean is where it spills
// 3. Lakes: a basin fills to its pass when it receives more than its surface evaporates, the rest leaves over
//    the pass. Basins sharing a pass that together receive more than they evaporate merge into one lake
// 4. Discharge: every cell's runoff is summed down the downstream links, lake outflows enter at their pass
// Fills Cell::pet, aet, runoff, discharge, downstream, waterBody and globals.waterBodies. Does not touch
// height, lakeBool, riverBool or the old river code
HydrologyStats planetCalcHydrology(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, const MapConfig& config);
