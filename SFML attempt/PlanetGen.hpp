#pragma once
#include <vector>
#include "cell.hpp"
#include "CellObjects.hpp"
#include "GlobalWorldObjects.hpp"
#include "mapconfig.hpp"
#include "SphereVoronoi.hpp"

// World generation on the sphere.
//
// The first group only walks the cell graph (cells and their neighbours), so it works on any map.
// It moved here from Map.hpp, which still uses it for the old flat world until that is removed.

void rise(std::vector<Cell>& map);
void smooth_height(std::vector<Cell>& map, float rise_threshold = 0.1, int repeats = 1, int method = 1);
void assignPlateTypes(std::vector<Continent>& continents, const MapConfig& config);
void setBaseElevations(std::vector<Cell>& map, GlobalWorldObjects& globals);
float calculateBoundaryHeightModifier(BoundaryType boundaryType, float dotProduct, float continent1Age, float continent2Age);
void simulateErosion(std::vector<Cell>& map, GlobalWorldObjects& globals, const MapConfig& config);
void propagateTerrainFeatures(std::vector<Cell>& map, GlobalWorldObjects& globals, std::size_t startCellIndex,
    float heightIncrease, bool volcanicActivity, BoundaryType boundaryType, int propagation_depth);
void cleanUpContinents(std::vector<Cell>& map, GlobalWorldObjects& globals);
void generateOceans(std::vector<Cell>& map, GlobalWorldObjects& globals);
void generateSnowline(std::vector<Cell>& map, GlobalWorldObjects& globals);
void generateTreeline(std::vector<Cell>& map, GlobalWorldObjects& globals);
void calcHeightValues(std::vector<Cell>& map, GlobalWorldObjects& globals, float deltaCoastLine);
void closeOceanCell(std::vector<Cell>& map);
void smoothTemps(std::vector<Cell>& map, int smoothTimes);
void calcHumid(std::vector<Cell>& map);
void riverIteration(std::vector<Cell>& map, GlobalWorldObjects& globals, std::vector<std::size_t>& stack, std::size_t start, std::size_t river_id);
// Rivers from the snow line down. The flat map's version also takes the positions for its river and lake drawing
void calcRiverStart(std::vector<Cell>& map, GlobalWorldObjects& globals);

// The second group needs positions, these are the sphere versions of the Map.hpp functions.

// Plate speeds, drawn uniformly per plate. The range keeps the mean squared speed at 1, which gives the
// relative plate velocities the same spread as the flat map's random direction vectors, so the
// boundary thresholds (0.2) sort boundaries into the same mix of types
constexpr float PLATE_SPEED_MIN = 0.5f;
constexpr float PLATE_SPEED_MAX = 1.43f;

// Simplex noise is sampled at p * NOISE_FREQUENCY. The flat map spanned about 11 noise units each way,
// this gives the sphere the same noise area (4 pi k^2 = 11^2), so features keep their size relative to the world
constexpr float NOISE_FREQUENCY = 3.13f;

void setRandomEulerPole(Continent& continent);

// random_height_gen: height methods 1 and 2 (k peaks, 2 adds plate interaction), 3 and 4 (plates first)
void planetHeightGen(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, const MapConfig& config);
// noise_height: method 1 adds small random noise, method 2 a layer of simplex noise
void planetNoiseHeight(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, const MapConfig& config, int iterations);

void planetContinentGeneration(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, const MapConfig& config);
void planetContinentInteraction(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, int propagation_depth = 1);
void planetSimplexNoise(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, const MapConfig& config);

// Climate. The flat map measured latitude as y over the map height, and its convergence lines are still
// stored that way: a fraction t from 0 at the north pole to 1 at the south pole, lat = 90 - 180 t.
// The formulas stay linear in latitude as before

// Same rule as NOISE_FREQUENCY: the wind noise spanned 1 / 0.15 units each way, the precipitation noise
// 0.02 units per pixel on the 2500 x 1500 window
constexpr float WIND_NOISE_FREQUENCY = 1.88f;
constexpr float PRECIP_NOISE_FREQUENCY = 10.9f;

float latToMapFraction(float lat);
// The middle convergence line (the ITCZ) is the climate equator, as a map fraction
float climateEquatorFraction(const GlobalWorldObjects& globals);
// 0 on the climate equator, 1 at 90 degrees from it or further
float normalizedClimateLatitude(float lat, float equatorFraction);

// windDir is a compass bearing the wind blows toward. The flat map read it as a screen angle
// (0 = east, 90 = down) when moving moisture, so every wind there was turned 90 degrees clockwise
void planetCalcWind(vor::SphereVoronoi& planet, GlobalWorldObjects& globals);
void planetCalcTemp(vor::SphereVoronoi& planet, GlobalWorldObjects& globals);
void planetCalcPercepitation(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, const MapConfig& config);
void planetCalcClimateVariance(vor::SphereVoronoi& planet, GlobalWorldObjects& globals);
