#include "PlanetaryObjects.hpp"
#include <vector>

void PlanetaryParameters::initializeEarthLike() {
	rotationSpeed = 1.0f;
	progradeRotation = true;
	axialTilt = 23.5f;
	equatorToPoleTemp = 20.0f;
	greenhouseEffectFactor = 1.0f;
	atmosphereHeight = 100.0f;
	atmosphere.nitrogenPercentage = 78.0f;
	atmosphere.oxygenPercentage = 21.0f;
	atmosphere.carbonDioxidePercentage = 0.04f;
	atmosphere.otherGasesPercentage = 0.96f;
	atmosphere.totalPressure = 1.0f;
}
void PlanetaryParameters::initializeMarsLike() {
	rotationSpeed = 0.97f;
	progradeRotation = true;
	axialTilt = 25.0f;
	equatorToPoleTemp = 40.0f;
	greenhouseEffectFactor = 0.5f;
	atmosphereHeight = 11.1f;
	atmosphere.nitrogenPercentage = 95.0f;
	atmosphere.oxygenPercentage = 0.13f;
	atmosphere.carbonDioxidePercentage = 2.7f;
	atmosphere.otherGasesPercentage = 2.17f;
	atmosphere.totalPressure = 0.006f;
}

void PlanetaryParameters::initialize() {
	rotationSpeed = RandomBetween(0.5f, 2.0f);
	progradeRotation = (rand() % 2 == 0);
	axialTilt = RandomBetween(0.0f, 45.0f);
	equatorToPoleTemp = calculateTemperatureGradient();

	// Generate atmospheric composition
	atmosphere.totalPressure = RandomBetween(0.1f, 3.0f); // in atm
	atmosphere.nitrogenPercentage = RandomBetween(50.0f, 90.0f);
	atmosphere.oxygenPercentage = RandomBetween(10.0f, 30.0f);
	atmosphere.carbonDioxidePercentage = RandomBetween(0.01f, 5.0f);
	atmosphere.otherGasesPercentage = 100.0f - (atmosphere.nitrogenPercentage + atmosphere.oxygenPercentage + atmosphere.carbonDioxidePercentage);

	greenhouseEffectFactor = greenhouseEffect();
	atmosphereHeight = calculateAtmosphereHeight();
}

float PlanetaryParameters::greenhouseEffect()
{
	// Simplified greenhouse effect calculation
	float ghEffect = (atmosphere.carbonDioxidePercentage * 0.02f +
		atmosphere.otherGasesPercentage * 0.01f) * greenhouseEffectFactor;
	return ghEffect;
}
float PlanetaryParameters::calculateAtmosphereHeight()
{
	// Simplified atmosphere height calculation based on composition and pressure
	float height = (atmosphere.totalPressure * 100.0f) / 
		(atmosphere.nitrogenPercentage * 0.78f + atmosphere.oxygenPercentage * 0.21f);
	return height;
}
float PlanetaryParameters::calculateTemperatureGradient()
{
	// Simplified temperature gradient calculation
	float gradient = equatorToPoleTemp / (90.0f - axialTilt);
	return gradient;
}
float PlanetaryParameters::getRotationPeriod()
{
	// Calculate rotation period in hours
	float period = 24.0f / rotationSpeed;
	return period;
}
float PlanetaryParameters::calculateAxialTiltEffect()
{
	// Simplified axial tilt effect on seasons
	float tiltEffect = axialTilt / 23.5f;
	return tiltEffect;
}

float PlanetaryParameters::getCoriolisStrenght() const {
	// Coriolis force strength is proportional to rotation speed
	// Returns 0.5-2.0 range (Earth = 1.0)
	return rotationSpeed;
}

float PlanetaryParameters::getAtmosphericCirculationStrength() const {
	// Circulation strenght depends on:
	// 1. Temperature differential
	// 2. Atmospheric Preassure
	// 3. Greenhouse effect
	float tempFactor = clamp(equatorToPoleTemp / 30.0f, 2.0f, 0.3f);
	float pressureFactor = clamp(atmosphere.totalPressure, 2.0f, 0.1f);
	float ghFactor = 1.0f + greenhouseEffectFactor * 0.2f;
	return tempFactor * pressureFactor * ghFactor;
}

int PlanetaryParameters::getNumberOfCirculationCells() const {
	// number of circulation cells depend on rotation speed and atmospheric density
	// slow rotation = fewer cells (venus = 1-2)
	// fast rotation = more cells (jupiter = many)
	// earth has 3 cells per hemisphere
	if (rotationSpeed < 0.3f) return 2;
	else if (rotationSpeed < 0.7f) return 4;
	else if (rotationSpeed < 1.5f) return 6;
	else return 8;
}

std::vector<float> PlanetaryParameters::calculateCellBoundaries() const {
	// normalized latitude positions (0.0 = north pole, 1.0 = south pole)
	// convergence zones between circulation cells

	int numCells = getNumberOfCirculationCells();
	std::vector<float> boundaries;

	// ITCZ should be near equator (for earth like planets)
	float itczOffset = (axialTilt / 90.0f) * 0.1f;

	if (numCells == 2)
	{
		// Single cell per hemisphere like venus
		boundaries = { 0.5f + itczOffset };
	}
	else if (numCells == 4)
	{
		// Two cells per hemisphere like mars
		boundaries = { 0.25f + itczOffset, 0.5f + itczOffset, 0.75f + itczOffset };
	}
	else if (numCells == 6)
	{
		// Three cells per hemisphere like earth
		boundaries = { 0.1667f + itczOffset, 0.3333f + itczOffset, 0.5f + itczOffset,
			0.6667f + itczOffset, 0.8333f + itczOffset };
	}
	else
	{
		// More than three cells per hemisphere
		for (int i = 1; i < numCells; i++)
		{
			float boundary = (float)i / (float)numCells + itczOffset;
			boundaries.push_back(boundary);
		}
	}
	return boundaries;
}

// NEEDS MORE WORK
std::vector<float> PlanetaryParameters::calculateWindDirections(const std::vector<float>& cellBoundaries) const {
	// returns wind directions in degrees for each cell
	// determined by coriolis effect and pressure gradients

	std::vector<float> directions;
	int rotationSign = progradeRotation ? 1 : -1;
	float coriolisStrength = getCoriolisStrenght();

	// Find the actual equator position (where ITCZ is located)
	// This is the middle boundary in symmetric setups, or calculated from boundaries
	float equatorPosition = 0.5f;
	if (!cellBoundaries.empty()) {
		// The equator is typically at the central convergence line (ITCZ)
		int middleIndex = cellBoundaries.size() / 2;
		equatorPosition = cellBoundaries[middleIndex];
	}

	// For each zone between boundaries
	int numZones = cellBoundaries.size() + 1;
	for (int i = 0; i < numZones; i++)
	{
		float zoneCenterLat = 0.f;
		if (i == 0) {
			// north polar zone 
			zoneCenterLat = (cellBoundaries.empty() ? 0.25f : cellBoundaries[0] * 0.5f);
		}
		else if (i == numZones - 1) {
			// south polar zone
			zoneCenterLat = cellBoundaries.back() + (1.0f - cellBoundaries.back()) * 0.5f;
		}
		else {
			// mid zones
			zoneCenterLat = (cellBoundaries[i - 1] + cellBoundaries[i]) * 0.5f;
		}

		// distance from ACTUAL equator (not hardcoded 0.5)
		float latitudeFromEquator = std::abs(zoneCenterLat - equatorPosition) / (0.5f); // normalize to 0-1
		latitudeFromEquator = std::min(latitudeFromEquator, 1.0f); // clamp

		// Determine hemisphere based on actual equator position
		bool isNorthernHemisphere = (zoneCenterLat < equatorPosition);

		// Determine if this is a rising (low pressure) or sinking (high pressure) zone
		// Alternates from pole: Polar cell (sinking) -> Ferrel (rising) -> Hadley (sinking) -> ITCZ (rising)
		bool isSinkingZone = (i % 2 == 0);

		// Base pressure gradient direction (before Coriolis deflection)
		// 0° = North, 180° = South
		float pressureGradientDirection = 0.0f;
		if (isSinkingZone) {
			// High pressure zone: air flows AWAY from this zone toward equator
			if (isNorthernHemisphere) {
				// Northern hemisphere: flows southward (toward equator)
				pressureGradientDirection = 180.0f;
			}
			else {
				// Southern hemisphere: flows northward (toward equator)
				pressureGradientDirection = 0.0f;
			}
		}
		else {
			// Low pressure zone: air flows TOWARD this zone (poleward)
			if (isNorthernHemisphere) {
				// Northern hemisphere: flows northward (toward pole)
				pressureGradientDirection = 0.0f;
			}
			else {
				// Southern hemisphere: flows southward (toward pole)
				pressureGradientDirection = 180.0f;
			}
		}

		// Calculate Coriolis deflection
		// Deflection increases with:
		// 1. Distance from equator (latitude)
		// 2. Rotation speed
		// Maximum deflection is ~90° (perpendicular to pressure gradient)
		float maxDeflection = 90.0f;
		float coriolisDeflection = maxDeflection * coriolisStrength * latitudeFromEquator * rotationSign;

		// Apply deflection: right in Northern Hemisphere (prograde), left in Southern
		if (!isNorthernHemisphere) {
			coriolisDeflection = -coriolisDeflection; // Reverse for southern hemisphere
		}

		// Combine pressure gradient and Coriolis deflection
		float windDirection = pressureGradientDirection + coriolisDeflection;

		// Special case: Equatorial zone (weak Coriolis, variable winds)
		if (std::abs(zoneCenterLat - equatorPosition) < 0.15f) {
			// ITCZ-like behavior: very weak deflection, mostly easterly
			windDirection = 90.0f * rotationSign + RandomBetween(-30.0f, 30.0f);
		}

		// Add some randomness based on atmospheric turbulence
		// Less turbulent atmospheres (thin, low pressure) have more variation
		float turbulenceFactor = 1.0f / (atmosphere.totalPressure + 0.1f);
		float randomVariation = RandomBetween(-20.0f, 20.0f) * turbulenceFactor;
		windDirection += randomVariation;

		// Normalize to 0-360°
		while (windDirection < 0.0f) windDirection += 360.0f;
		while (windDirection >= 360.0f) windDirection -= 360.0f;

		directions.push_back(windDirection);
	}
	return directions;
}

std::vector<float> PlanetaryParameters::calculateWindStrengths(const std::vector<float>& cellBoundaries) const {
	// returns wind strengths (0.0 - 1.0) for each cell
	// Strenght depends on circulation strength and positions

	std::vector<float> strengths;
	float baseStrength = getAtmosphericCirculationStrength();
	int numZones = cellBoundaries.size() + 1;

	for (int i = 0; i < numZones; i++)
	{
		float zoneStrength = baseStrength;

		// Equatorial zones (Hadley cells) tend to have stronger winds
		// Mid-latitude zones (Ferrel cells) have moderate winds
		// Polar zones have weaker winds
		if (i == 0 || i == numZones - 1) {
			zoneStrength *= RandomBetween(0.6f, 0.8f); // polar zones
		}
		else if (i == numZones / 2 || i == (numZones / 2) - 1 || i == (numZones / 2) + 1) {
			zoneStrength *= RandomBetween(0.9f, 1.1f); // equatorial zones
		}
		else {
			zoneStrength *= RandomBetween(0.7f, 0.9f); // mid-latitude zones
		}

		// Clamp to 0.0 - 1.0
		zoneStrength = clamp(zoneStrength, 0.0f, 1.0f);
		strengths.push_back(zoneStrength);
	}
	return strengths;
}

