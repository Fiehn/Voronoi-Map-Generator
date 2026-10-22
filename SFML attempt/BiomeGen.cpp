#include "BiomeGen.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include "clustering.hpp"
#include "util.hpp"

namespace {

    const sf::Color SHALLOW_OCEAN = sf::Color(110, 170, 220);
    const sf::Color DEEP_OCEAN = sf::Color(20, 40, 100);

    // One side of the coast, clustered on its own. Its biomes are the block [offset, offset + count)
    struct Domain
    {
        bool ocean = false;
        std::vector<std::size_t> cells;
        int offset = 0;
        int count = 0;
    };

    // Every ocean cell is 0 cells from the ocean, so distance to ocean is only a land feature
    std::vector<float> biomeFeatures(const Cell& cell)
    {
        std::vector<float> features = { cell.temp, cell.percepitation, cell.humidity, cell.height, cell.windStr };
        if (!cell.oceanBool) { features.push_back(static_cast<float>(cell.distToOcean)); }
        return features;
    }

    std::unique_ptr<ClusteringMethod> makeClustering(int method, int k, int dimensions, int maxIter)
    {
        if (method == 1 || method == 3) { return std::make_unique<GMM>(k, dimensions, maxIter); }
        if (method == 2) { return std::make_unique<KMeans>(k, dimensions, maxIter); }
        return nullptr;
    }

    sf::Color lerpColor(const sf::Color& from, const sf::Color& to, float t)
    {
        auto mix = [t](sf::Uint8 a, sf::Uint8 b) { return static_cast<sf::Uint8>(std::lround(a + (b - a) * t)); };
        return sf::Color(mix(from.r, to.r), mix(from.g, to.g), mix(from.b, to.b));
    }

    // Writes the cluster results of one side into its block of the biomes and cell results
    bool clusterDomain(const std::vector<Cell>& map, const Domain& domain, int maxIter, int method,
        std::vector<Biome>& biomes, std::vector<int>& cellBiome, std::vector<std::vector<float>>& cellProb)
    {
        if (domain.count == 0) { return true; }

        std::vector<std::vector<float>> data;
        data.reserve(domain.cells.size());
        for (std::size_t cell : domain.cells) { data.push_back(biomeFeatures(map[cell])); }

        std::unique_ptr<ClusteringMethod> clustering = makeClustering(method, domain.count, static_cast<int>(data[0].size()), maxIter);
        if (clustering == nullptr) { return false; }

        clustering->setData(data);
        clustering->run();

        for (std::size_t i = 0; i < domain.cells.size(); i++)
        {
            std::size_t cell = domain.cells[i];
            cellBiome[cell] = domain.offset + clustering->getClusterId(static_cast<int>(i));
            std::vector<float> prob = clustering->getBiomeProb(static_cast<int>(i));
            std::copy(prob.begin(), prob.end(), cellProb[cell].begin() + domain.offset);
        }

        // Biome values are the cluster centres, both sides get the same keys so the biome table lines up
        for (int j = 0; j < domain.count; j++)
        {
            std::vector<float> centre = clustering->getCentroid(j);
            std::map<std::string, float> values = {
                { "Ocean", domain.ocean ? 1.f : 0.f },
                { "Temperature", centre[0] },
                { "Percepitation", centre[1] },
                { "Humidity", centre[2] },
                { "Height", centre[3] },
                { "Wind Strength", centre[4] },
                { "Dist to Ocean", domain.ocean ? 0.f : centre[5] }
            };
            biomes[domain.offset + j].setValues(values);
        }
        return true;
    }

    // Ocean biomes run from light to dark blue, ordered from the shallowest to the deepest centre
    void colorOceanByDepth(std::vector<Biome>& biomes, const Domain& ocean)
    {
        std::vector<int> byDepth(ocean.count);
        std::iota(byDepth.begin(), byDepth.end(), ocean.offset);
        std::sort(byDepth.begin(), byDepth.end(), [&biomes](int a, int b) {
            return biomes[a].values.at("Height") > biomes[b].values.at("Height");
            });

        for (int rank = 0; rank < ocean.count; rank++)
        {
            float t = ocean.count == 1 ? 0.5f : static_cast<float>(rank) / (ocean.count - 1);
            biomes[byDepth[rank]].color = lerpColor(SHALLOW_OCEAN, DEEP_OCEAN, t);
        }
    }

    // Neighbours pull a cell's probabilities toward their own. Only neighbours on the same side of the coast count,
    // and every cell reads the probabilities from before smoothing so the order of the cells does not matter
    void smoothProbabilities(const std::vector<Cell>& map, const Domain& ocean, const Domain& land, float probSmoothing,
        std::vector<int>& cellBiome, std::vector<std::vector<float>>& cellProb)
    {
        const std::vector<std::vector<float>> raw = cellProb;

        for (std::size_t i = 0; i < map.size(); i++)
        {
            const Cell& cell = map[i];
            const Domain& domain = cell.oceanBool ? ocean : land;
            const int begin = domain.offset;
            const int end = domain.offset + domain.count;

            std::vector<float> neighborSum(cellProb[i].size(), 0.f);
            int count = 0;
            for (int neighbor : cell.neighbors)
            {
                if (map[neighbor].oceanBool != cell.oceanBool) { continue; }
                for (int k = begin; k < end; k++) { neighborSum[k] += raw[neighbor][k]; }
                count++;
            }
            if (count == 0) { continue; }

            float sum = 0.f;
            for (int k = begin; k < end; k++)
            {
                cellProb[i][k] += neighborSum[k] * probSmoothing / count;
                sum += cellProb[i][k];
            }
            for (int k = begin; k < end; k++) { cellProb[i][k] /= sum + 1e-8f; }

            cellBiome[i] = static_cast<int>(std::max_element(cellProb[i].begin() + begin, cellProb[i].begin() + end) - cellProb[i].begin());
        }
    }
}

void calcBiome(std::vector<Cell>& map, std::vector<Biome>& biomes, int nOcean, int nLand, int maxIter, int method, float probSmoothing)
{
    Domain ocean;
    Domain land;
    ocean.ocean = true;
    for (std::size_t i = 0; i < map.size(); i++) { (map[i].oceanBool ? ocean : land).cells.push_back(i); }

    // A side with cells gets at least one biome, so every cell has a biome on its own side
    ocean.count = std::min(std::max(nOcean, 1), static_cast<int>(ocean.cells.size()));
    land.count = std::min(std::max(nLand, 1), static_cast<int>(land.cells.size()));
    land.offset = ocean.count;
    const int total = ocean.count + land.count;

    // Everything is built here and only written to the map and the biomes at the end, the UI reads them while this runs async
    std::vector<Biome> newBiomes;
    newBiomes.reserve(total);
    for (int i = 0; i < ocean.count; i++)
    {
        newBiomes.emplace_back("Ocean " + std::to_string(i), ocean.offset + i, SHALLOW_OCEAN);
        newBiomes.back().isOcean = true;
    }
    std::vector<sf::Color> landColors = randomColors(land.count);
    for (int i = 0; i < land.count; i++)
    {
        newBiomes.emplace_back("Land " + std::to_string(i), land.offset + i, landColors[i]);
    }

    std::vector<int> cellBiome(map.size(), 0);
    std::vector<std::vector<float>> cellProb(map.size(), std::vector<float>(total, 0.f));

    if (!clusterDomain(map, ocean, maxIter, method, newBiomes, cellBiome, cellProb) ||
        !clusterDomain(map, land, maxIter, method, newBiomes, cellBiome, cellProb))
    {
        std::cout << "Error: Clustering method not found" << std::endl;
        return;
    }

    colorOceanByDepth(newBiomes, ocean);

    if (method == 1) { smoothProbabilities(map, ocean, land, probSmoothing, cellBiome, cellProb); }

    for (int biome : cellBiome) { newBiomes[biome].numCells++; }

    biomes = std::move(newBiomes);
    for (std::size_t i = 0; i < map.size(); i++)
    {
        map[i].biome = cellBiome[i];
        map[i].biome_prob = std::move(cellProb[i]);
    }
}
