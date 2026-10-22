#pragma once
#include <vector>
#include "cell.hpp"
#include "CellObjects.hpp"

// Biomes are clustered separately for ocean and land cells, so neither side can take the other's clusters
// The biome list holds the ocean block first, ids [0, nOcean), then the land block, ids [nOcean, nOcean + nLand)
// Every cell's biome_prob has one entry per biome and the other side's block is always zero
// A side gets between 1 biome and one per cell (0 if it has no cells), so the blocks can differ from what was asked for
// method 1: GMM with neighbour smoothing, 2: KMeans, 3: GMM without smoothing
void calcBiome(std::vector<Cell>& map, std::vector<Biome>& biomes, int nOcean, int nLand,
    int maxIter = 5, int method = 1, float probSmoothing = 0.5f);
