#pragma once
#include "extractiveResources.hpp"
#include "Cell.hpp"
#include "GlobalWorldObjects.hpp"
#include "util.hpp"
#include <map>

std::string resourceTypeToString(ResourceType type) {
	switch (type) {
	case ResourceType::Copper: return "Copper";
	case ResourceType::Iron: return "Iron";
	case ResourceType::Tin: return "Tin";
	case ResourceType::Gold: return "Gold";
	case ResourceType::Silver: return "Silver";
	case ResourceType::Lead: return "Lead";
	case ResourceType::Lumber: return "Lumber";
	case ResourceType::Stone: return "Stone";
	case ResourceType::Clay: return "Clay";
	case ResourceType::Coal: return "Coal";
	case ResourceType::Grain: return "Grain";
	case ResourceType::Fruit: return "Fruit";
	case ResourceType::Vegetables: return "Vegetables";
	case ResourceType::Cotton: return "Cotton";
	case ResourceType::Livestock: return "Livestock";
	case ResourceType::Sheep: return "Sheep";
	case ResourceType::Furs: return "Furs";
	case ResourceType::Fish: return "Fish";
	case ResourceType::Whales: return "Whales";
	case ResourceType::Spices: return "Spices";
	case ResourceType::Gems: return "Gems";
	case ResourceType::Dyes: return "Dyes";
	default: return "Unknown";
	}
}

void ExtractiveResource::setResourceAmount(ResourceType type, float amount) {
	resources[type] = amount;
}

float ExtractiveResource::getResourceAmount(ResourceType type) const{
	auto it = resources.find(type);
	if (it != resources.end()) {
		return it->second;
	}
	return 0.0f;
}
bool ExtractiveResource::hasResource(ResourceType type) {
	return resources.find(type) != resources.end();
}
std::map<ResourceType, float> ExtractiveResource::getAllResources() const {
	return resources;
}

namespace ResourceGen {
	void generateCellResources(std::vector<Cell>& map, Cell& cell, const GlobalWorldObjects& globals)
	{
		cell.resources.clear();

		// Ocean specifics
		if (cell.oceanBool) {
			float fishAmount = calculateFishAbundance(cell);
			if (fishAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Fish, fishAmount);
			}
			float whaleAmount = calculateWhaleAbundance(cell);
			if (whaleAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Whales, whaleAmount);
			}
			return; // No other resources in ocean cells
		}
		// Land specifics (metals first)
		if (cell.height > 0.5f && cell.rise > 0.02f)
		{
			float copperAmount = calculateMetalAbundance(cell, ResourceType::Copper);
			if (copperAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Copper, copperAmount);
				propagateMetalResource(map,cell, ResourceType::Copper);
			}
			float ironAmount = calculateMetalAbundance(cell, ResourceType::Iron);
			if (ironAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Iron, ironAmount);
				propagateMetalResource(map, cell, ResourceType::Iron);
			}
			float tinAmount = calculateMetalAbundance(cell, ResourceType::Tin);
			if (tinAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Tin, tinAmount);
				propagateMetalResource(map, cell, ResourceType::Tin);
			}
			float goldAmount = calculateMetalAbundance(cell, ResourceType::Gold);
			if (goldAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Gold, goldAmount);
				propagateMetalResource(map,cell, ResourceType::Gold);
			}
			float silverAmount = calculateMetalAbundance(cell, ResourceType::Silver);
			if (silverAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Silver, silverAmount);
				propagateMetalResource(map,cell, ResourceType::Silver);
			}
			float leadAmount = calculateMetalAbundance(cell, ResourceType::Lead);
			if (leadAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Lead, leadAmount);
				propagateMetalResource(map,cell, ResourceType::Lead);
			}
			if (cell.height > 0.8f && RandomBetween(0.0f, 1.0f) < 0.05f)
			{
				cell.resources.setResourceAmount(ResourceType::Gems, RandomBetween(0.1f, 0.6f));
			}
		}
		// Materials
		if (cell.treeBool)
		{
			float lumberAmount = calculateLumberAbundance(cell);
			if (lumberAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Lumber, lumberAmount);
			}
		}
		float stoneAmount = calculateMaterialAbundance(map, cell, ResourceType::Stone);
		if (stoneAmount > 0.f) {
			cell.resources.setResourceAmount(ResourceType::Stone, stoneAmount);
		}
		float clayAmount = calculateMaterialAbundance(map, cell, ResourceType::Clay);
		if (clayAmount > 0.f) {
			cell.resources.setResourceAmount(ResourceType::Clay, clayAmount);
		}
		float coalAmount = calculateMaterialAbundance(map, cell, ResourceType::Coal);
		if (coalAmount > 0.f) {
			cell.resources.setResourceAmount(ResourceType::Coal, coalAmount);
		}

		// Agriculture
		if (cell.height < 0.8f && cell.humidity > 0.1f && cell.temp > 0.f) {
			// Suitable for agriculture
			float grainAmount = calculateAgricultureAbundance(cell, ResourceType::Grain);
			if (grainAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Grain, grainAmount);
			}
			float fruitAmount = calculateAgricultureAbundance(cell, ResourceType::Fruit);
			if (fruitAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Fruit, fruitAmount);
			}
			float vegetableAmount = calculateAgricultureAbundance(cell, ResourceType::Vegetables);
			if (vegetableAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Vegetables, vegetableAmount);
			}
		}
		
		if (cell.temp > 15.f) {
			// Cotton needs warm climate
			float cottonAmount = calculateAgricultureAbundance(cell, ResourceType::Cotton);
			if (cottonAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Cotton, cottonAmount);
			}
		}
		// Livestock
		float livestockAmount = calculateLivestockAbundance(cell, ResourceType::Livestock);
		if (livestockAmount > 0.f) {
			cell.resources.setResourceAmount(ResourceType::Livestock, livestockAmount);
		}
		float sheepAmount = calculateLivestockAbundance(cell, ResourceType::Sheep);
		if (sheepAmount > 0.f) {
			cell.resources.setResourceAmount(ResourceType::Sheep, sheepAmount);
		}
		
		// Luxury resources
		if (cell.height < 0.7f && cell.humidity > 0.2f) {
			float spicesAmount = calculateLuxuryAbundance(cell, ResourceType::Spices);
			if (spicesAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Spices, spicesAmount);
			}
			float dyesAmount = calculateLuxuryAbundance(cell, ResourceType::Dyes);
			if (dyesAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Dyes, dyesAmount);
			}
		}

		// furs from colder climates and forests
		if (cell.temp < 10.f && cell.treeBool) {
			float fursAmount = calculateLivestockAbundance(cell, ResourceType::Furs);
			if (fursAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Furs, fursAmount);
			}
		}

		// Whales in coastal cells
		if (cell.coastBool) {
			float whaleAmount = calculateWhaleAbundance(cell);
			if (whaleAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Whales, whaleAmount);
			}
		}
		
		// fish near rivers, lakes and coasts
		if (cell.riverBool || cell.lakeBool || cell.coastBool) {
			float fishAmount = calculateFishAbundance(cell);
			if (fishAmount > 0.f) {
				cell.resources.setResourceAmount(ResourceType::Fish, fishAmount);
			}
		}
	}
	void generateMapResources(std::vector<Cell>& map, const GlobalWorldObjects& globals)
	{
		for (auto& cell : map) {
			generateCellResources(map, cell, globals);
		}
		applyResourceSmoothing(map);
	}

	void applyResourceSmoothing(std::vector<Cell>& map, int smoothingPasses)
	{
		for (int pass = 0; pass < smoothingPasses; ++pass) {
			std::vector<Cell> tempMap = map; // Copy of the map to read from
			std::vector<ResourceType> smoothableResources = {
				ResourceType::Copper,
				ResourceType::Iron,
				ResourceType::Tin,
				ResourceType::Gold,
				ResourceType::Silver,
				ResourceType::Lead,
				ResourceType::Lumber,
				ResourceType::Coal,
				ResourceType::Clay
			};
			for (std::size_t i = 0; i < map.size(); i++)
			{
				if (map[i].oceanBool) continue; // Skip ocean cells
				for (ResourceType resType : smoothableResources) {
					float currentAmount = tempMap[i].resources.getResourceAmount(resType);
					if (currentAmount > 0.f)
					{
						// Get neighbor average
						float neighborAvg = getNeighborAverage(tempMap, i, resType);
						// blend current amount with neighbor average (70% current, 30% neighbors)
						float smoothedAmount = currentAmount * 0.7f + neighborAvg * 0.3f;
						map[i].resources.setResourceAmount(resType, smoothedAmount);
					}
					else {
						// Small chance of propagation from neighbors
						float neighborAvg = getNeighborAverage(tempMap, i, resType);
						if (neighborAvg > 0.4f && RandomBetween(0.f, 1.f) < 0.2f) {
							float propagatedAmount = neighborAvg * RandomBetween(0.1f, 0.5f);
							map[i].resources.setResourceAmount(resType, propagatedAmount);
						}
					}
				}
			}
		}
	}

	float getNeighborAverage(std::vector<Cell>& map, std::size_t cellIndex, ResourceType type) {
		float sum = 0.f;
		int count = 0;
		for (std::size_t neighborIdx : map[cellIndex].neighbors) {
			float neighborAmount = map[neighborIdx].resources.getResourceAmount(type);
			if (neighborAmount > 0.f) {
				sum += neighborAmount;
				count++;
			}
		}
		if (count == 0) return 0.f;
		return sum / static_cast<float>(count);
	}

	float calculateFishAbundance(const Cell& cell) {
		float abundance = 0.f;

		if (cell.oceanBool) {
			abundance += 0.5f + RandomBetween(-0.1f,0.3f);
		}
		else if (cell.coastBool) {
			abundance += 0.7f + RandomBetween(-0.1f, 0.2f);
		} else if (cell.riverBool) {
			abundance += 0.4f + cell.riverStr*0.4f + RandomBetween(-0.1f, 0.2f);
		}
		else if (cell.lakeBool) {
			abundance += 0.6f + RandomBetween(-0.1f, 0.2f);
		}
		return clamp(abundance, 0.f, 1.f);
	}

	float calculateLumberAbundance(const Cell& cell) {
		if (!cell.treeBool) {
			return 0.f;
		}
		// Higher in moderate climates with good percipitation
		float climateFactor = 1.f - std::abs(cell.temp - 15.f) / 30.f; // Peak at 15C
		float humidityFactor = clamp(cell.humidity, 1.f, 0.3f);
		float precepitationFactor = clamp(cell.percepitation / 30.f, 1.f, 0.f);

		return std::max(0.0f, climateFactor * humidityFactor * precepitationFactor * RandomBetween(0.75f, 1.0f));
	}
	float calculateWhaleAbundance(const Cell& cell) {
		if (!cell.oceanBool && !cell.coastBool) {
			return 0.f;
		}
		// Whales are found in cold and deep waters
		float tempFactor = 1.f - clamp((cell.temp + 2.f) / 20.f, 1.f, 0.f); // Peak at -2C
		float heightFactor = clamp(0.3f - cell.height, 1.f, 0.f); // Deeper waters
		return std::max(0.0f, tempFactor * heightFactor * RandomBetween(0.5f, 1.0f));
	}
	float calculateMetalAbundance(const Cell& cell, ResourceType type) {
		float baseChance = 0.f;
		float rarityModifier = 1.f;
		float minAmount = 0.2f;
		float maxAmount = 0.9f;

		// Different metals have different base chances and rarity
		switch (type) {
			case ResourceType::Copper:
				baseChance = 0.0025f;
				rarityModifier = 1.2f;
				break;
			case ResourceType::Iron:
				baseChance = 0.0015f;
				rarityModifier = 1.0f;
				minAmount = 0.3f;
				break;
			case ResourceType::Tin:
				baseChance = 0.0012f;
				rarityModifier = 0.8f;
				break;
			case ResourceType::Gold:
				baseChance = 0.00005f;
				rarityModifier = 0.5f;
				maxAmount = 0.5f;
				break;
			case ResourceType::Silver:
				baseChance = 0.0001f;
				rarityModifier = 0.6f;
				maxAmount = 0.6f;
				break;
			case ResourceType::Lead:
				baseChance = 0.0012f;
				rarityModifier = 0.9f;
				break;
			default:
				return 0.f;
		}
		// Height increase metal probability (more in mountains)
		float heightBonus = clamp((cell.height - 0.7f), 1.f, 0.f); 
		// Rise increase metal probability (more in rugged terrain)
		float riseBonus = clamp(cell.rise, 1.f, 0.f);
		// Volcanic activity boost
		float volcanicBonus = cell.volcanicActivity ? 0.01f : 0.f;

		// Probability calculation
		float probability = baseChance * 1.0f + (heightBonus + riseBonus) * 0.01f + volcanicBonus;
		
		if (RandomBetween(0.f, 1.f) < probability) {
			return RandomBetween(minAmount, maxAmount) * rarityModifier;
		}
		return 0.f;
	}

	float calculateMaterialAbundance(std::vector<Cell>& map, Cell& cell, ResourceType type) {
		float abundance = 0.f;

		switch (type) {
		case ResourceType::Stone:
			// Stone is more abundant in higher elevations
			abundance = 0.3f + cell.height * 0.6f + RandomBetween(-0.1f, 0.2f);
			break;
		case ResourceType::Clay:
			// Clay is more abundant near water and lower elevations (also wet)
			if (cell.riverBool || cell.lakeBool || cell.coastBool) {
				abundance += 0.4f * RandomBetween(0.7f,1.5f);
			}
			abundance += - cell.height * 0.4f + RandomBetween(-0.2f, 0.2f) + clamp(cell.percepitation * 0.01f,0.2f,0.0f);
			break;
		case ResourceType::Coal:
			// coal is in forested areas with specific geological conditions
			if (cell.height > 0.5f && cell.height < 0.75f) {
				float forestBonus = cell.treeBool ? 0.05f : 0.f;
				float probability = 0.05 + forestBonus;
				if (RandomBetween(0.f, 1.f) < probability) {
					abundance = RandomBetween(0.2f, 0.7f);
					propagateMetalResource(map, cell, ResourceType::Coal);
				}
			}
			break;
		default:
			break;
		}
		return clamp(abundance, 0.f, 1.f);
	}

	float calculateAgricultureAbundance(const Cell& cell, ResourceType type) {
		if (cell.height > 0.8f || cell.snowBool || cell.iceBool || cell.temp < 0.f)
		{
			return 0.f;
		}
		float baseYield = 0.f;
		float optimalTemp = 18.f;
		float tempRange = 25.f;

		switch (type) {
		case ResourceType::Grain:
			// Grain thrives in temperate climates
			optimalTemp = 15.f;
			tempRange = 20.f;
			baseYield = 0.5f;
			break;
		case ResourceType::Fruit:
			// Fruit needs warmer climates
			optimalTemp = 20.f;
			tempRange = 15.f;
			break;
		case ResourceType::Vegetables:
			// Vegetables are versatile but prefer moderate warmth
			optimalTemp = 18.f;
			tempRange = 20.f;
			break;
		case ResourceType::Cotton:
			// Cotton needs hot humid climates
			optimalTemp = 25.f;
			tempRange = 10.f;
			if (cell.humidity < 0.2f || cell.tempVariance > 5.0f) {
				return 0.f;
			}
			break;
		default:
			return 0.f;
		}
		// Temperature factor
		float tempFactor = 1.f - std::abs(cell.temp - optimalTemp) / tempRange;
		tempFactor = clamp(tempFactor, 0.8f, 2.f);

		// Humidity and precepitation factors
		float waterFactor = (cell.humidity + clamp(cell.percepitation / 30.f, 1.f, 0.f)) / 2.f;
		waterFactor = clamp(waterFactor, 0.8f, 2.f);

		// height penalty
		float heightPenalty = 1.f - clamp((cell.height - 0.5f) * 2.f, 1.f, 0.f);
		
		// river bonus
		float riverBonus = cell.riverBool ? 0.2f : 0.f;

		baseYield += tempFactor * waterFactor * heightPenalty + riverBonus;
		return clamp(baseYield * RandomBetween(0.7f, 1.1f), 0.f, 1.f);
	}

	float calculateLivestockAbundance(const Cell& cell, ResourceType type) {
		float abundance = 0.f;
		switch (type) {
		case ResourceType::Livestock:
			// Cattle/general livestock prefers moderate climates and open land
			abundance = 0.4f + (1.f - std::abs(cell.temp - 15.f) / 30.f) * 0.4f;
			if (cell.treeBool) {
				abundance -= 0.1f; // less in forests
			}
			break;
		case ResourceType::Sheep:
			// Sheep are more tolerant of colder climates
			abundance = 0.3f + (1.f - std::abs(cell.temp - 10.f) / 25.f) * 0.5f;
			if (cell.treeBool) {
				abundance -= 0.1f; // less in forests
			}
			break;
		case ResourceType::Furs:
			// Furs come from colder climates with forests
			if (cell.temp < 10.f && cell.treeBool) {
				abundance = 0.2f + (1.f - std::abs(cell.temp - 5.f) / 20.f) * 0.6f;
			}
			break;
		default:
			return 0.f;
		}
		return clamp(abundance * RandomBetween(0.7f, 1.f), 1.f, 0.f);
	}
	float calculateLuxuryAbundance(const Cell& cell, ResourceType type) {
		float abundacen = 0.f;
		switch (type) {
		case ResourceType::Spices:
			// Spices need warm and humid climates
			if (cell.temp > 18.f && cell.humidity > 0.30f && cell.tempVariance < 5.0f) {
				float tropicalFactor = clamp((cell.temp - 20.f) / 15.f, 1.f, 0.f);
				float probability = 0.5f * tropicalFactor + 0.6f - (5.0f - cell.tempVariance) * 1.0f;

				if (RandomBetween(0.f, 1.f) < probability) {
					abundacen = RandomBetween(0.2f, 0.8f);
				}
			}
			break;
		case ResourceType::Dyes:
			// Dyes are diverse but often found in warm climates
			if (cell.humidity > 0.3f && cell.temp > 10.f) {
				float probability = 0.12f;
				if (RandomBetween(0.f, 1.f) < probability) {
					abundacen = RandomBetween(0.2f, 0.7f);
				}
			}
			break;
		default:
			return 0.f;
		}
		return abundacen;
	}

	void propagateMetalResource(std::vector<Cell>& map, Cell& cell, ResourceType type)
	{
		// Propagate to neighbors to create clusters (simulating ore veins)
		// 1. Start queue with current cell
		// 2. Set random Ore Vein Strength (random amount of resource to propagate)
		// 3. While queue not empty and strength > threshold 
		//   a. Pop cell from queue
		//   b. For each neighbor
		//    i. If neighbor does not have resource and is above height, add portion of strength to it
		//   ii. Decrease strength
		//  c. If strength > threshold, add neighbors to queue

		float veinStrength = cell.resources.getResourceAmount(type);
		float minHeight = 0.4f; // Minimum height for propagation

		switch (type) {
		case ResourceType::Copper:
			veinStrength *= RandomBetween(2.0f, 3.0f);
			minHeight = 0.4f;
			break;
		case ResourceType::Iron:
			veinStrength *= RandomBetween(1.5f, 2.5f);
			minHeight = 0.5f;
			break;
		case ResourceType::Tin:
			veinStrength *= RandomBetween(1.2f, 2.0f);
			break;
		case ResourceType::Gold:
			veinStrength *= RandomBetween(0.5f, 1.5f);
			minHeight = 0.7f;
			break;
		case ResourceType::Silver:
			veinStrength *= RandomBetween(0.7f, 1.8f);
			minHeight = 0.6f;
			break;
		case ResourceType::Lead:
			veinStrength *= RandomBetween(1.0f, 2.0f);
			break;
		default:
			veinStrength *= 1.0f;
			break;
		}

		std::vector<std::size_t> toProcess;
		toProcess.push_back(cell.id);

		while (!toProcess.empty() && veinStrength > 0.1f) {
			std::size_t currentCellIdx = toProcess.back();
			toProcess.pop_back();
			Cell& currentCell = map[currentCellIdx];
			for (std::size_t neighborIdx : currentCell.neighbors) {
				Cell& neighborCell = map[neighborIdx];
				if (!neighborCell.oceanBool && neighborCell.height >= minHeight && !neighborCell.resources.hasResource(type)) {
					// Add portion of vein strength to neighbor
					float propagatedAmount = veinStrength * RandomBetween(0.2f, 0.7f);
					neighborCell.resources.setResourceAmount(type, propagatedAmount);
					veinStrength -= propagatedAmount * 0.3f; // Decrease vein strength
					// Add neighbor to process list
					toProcess.push_back(neighborIdx);
				}
			}
		}
	}
}