#include "PlanetaryObjects.hpp"
#include <vector>


float AtmosphericComposition::getTotalPercentage() const {
	return nitrogenPercentage + oxygenPercentage + carbonDioxidePercentage +
		methanePercentage + sulfurDioxidePercentage + ammoniaPercentage +
		waterVaporPercentage + nitrogenDioxidePercentage + otherGasesPercentage;
}
void AtmosphericComposition::normalize() {
	float total = getTotalPercentage();
	if (total == 0.0f) return; // Avoid division by zero
	nitrogenPercentage = (nitrogenPercentage / total) * 100.0f;
	oxygenPercentage = (oxygenPercentage / total) * 100.0f;
	carbonDioxidePercentage = (carbonDioxidePercentage / total) * 100.0f;
	methanePercentage = (methanePercentage / total) * 100.0f;
	sulfurDioxidePercentage = (sulfurDioxidePercentage / total) * 100.0f;
	ammoniaPercentage = (ammoniaPercentage / total) * 100.0f;
	waterVaporPercentage = (waterVaporPercentage / total) * 100.0f;
	nitrogenDioxidePercentage = (nitrogenDioxidePercentage / total) * 100.0f;
	otherGasesPercentage = (otherGasesPercentage / total) * 100.0f;
}

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
	atmosphere.methanePercentage = 0.0002f;
	atmosphere.sulfurDioxidePercentage = 0.0f;
	atmosphere.ammoniaPercentage = 0.0f;
	atmosphere.waterVaporPercentage = 0.4f;
	atmosphere.nitrogenDioxidePercentage = 0.0f;
	atmosphere.otherGasesPercentage = 0.5598f;
	atmosphere.totalPressure = 1.0f;
	atmosphere.normalize();
}

void PlanetaryParameters::initializeMarsLike() {
	rotationSpeed = 0.97f;
	progradeRotation = true;
	axialTilt = 25.0f;
	equatorToPoleTemp = 40.0f;
	greenhouseEffectFactor = 0.5f;
	atmosphereHeight = 11.1f;
	atmosphere.nitrogenPercentage = 2.7f;
	atmosphere.oxygenPercentage = 0.13f;
	atmosphere.carbonDioxidePercentage = 95.0f;
	atmosphere.methanePercentage = 0.0f;
	atmosphere.sulfurDioxidePercentage = 0.0f;
	atmosphere.ammoniaPercentage = 0.0f;
	atmosphere.waterVaporPercentage = 0.03f;
	atmosphere.nitrogenDioxidePercentage = 0.0f;
	atmosphere.otherGasesPercentage = 2.14f;
	atmosphere.totalPressure = 0.006f;
	atmosphere.normalize();
}

void PlanetaryParameters::initialize() {
	rotationSpeed = RandomBetween(0.5f, 2.0f);
	progradeRotation = (rand() % 2 == 0);
	axialTilt = RandomBetween(12.0f, 40.0f);
	equatorToPoleTemp = calculateTemperatureGradient();
	// Generate atmospheric pressure first
	atmosphere.totalPressure = RandomBetween(0.1f, 3.0f); // in atm

	// Decide on atmosphere type randomly
	int atmosphereType = RandomBetween(0.0f, 100.0f);

	if (atmosphereType < 55) {
		// Earth-like atmosphere (N2/O2 dominant)
		atmosphere.nitrogenPercentage = RandomBetween(60.0f, 85.0f);
		atmosphere.oxygenPercentage = RandomBetween(15.0f, 30.0f);
		atmosphere.carbonDioxidePercentage = RandomBetween(0.01f, 2.0f);
		atmosphere.methanePercentage = RandomBetween(0.0f, 0.5f);
		atmosphere.sulfurDioxidePercentage = RandomBetween(0.0f, 0.01f);
		atmosphere.ammoniaPercentage = RandomBetween(0.0f, 0.01f);
		atmosphere.waterVaporPercentage = RandomBetween(0.1f, 3.0f);
		atmosphere.nitrogenDioxidePercentage = RandomBetween(0.0f, 0.01f);
		atmosphere.otherGasesPercentage = 1.0f; // Will be normalized
	}
	else if (atmosphereType < 70) {
		// CO2-rich atmosphere (Mars/Venus-like)
		atmosphere.carbonDioxidePercentage = RandomBetween(70.0f, 98.0f);
		atmosphere.nitrogenPercentage = RandomBetween(2.0f, 20.0f);
		atmosphere.oxygenPercentage = RandomBetween(0.0f, 1.0f);
		atmosphere.methanePercentage = RandomBetween(0.0f, 0.1f);
		atmosphere.sulfurDioxidePercentage = RandomBetween(0.0f, 0.5f);
		atmosphere.ammoniaPercentage = RandomBetween(0.0f, 0.1f);
		atmosphere.waterVaporPercentage = RandomBetween(0.0f, 0.5f);
		atmosphere.nitrogenDioxidePercentage = RandomBetween(0.0f, 0.1f);
		atmosphere.otherGasesPercentage = 1.0f;
	}
	else if (atmosphereType < 85) {
		// Methane-rich atmosphere (Titan-like)
		atmosphere.nitrogenPercentage = RandomBetween(80.0f, 95.0f);
		atmosphere.methanePercentage = RandomBetween(3.0f, 15.0f);
		atmosphere.oxygenPercentage = RandomBetween(0.0f, 0.5f);
		atmosphere.carbonDioxidePercentage = RandomBetween(0.0f, 0.5f);
		atmosphere.sulfurDioxidePercentage = RandomBetween(0.0f, 0.1f);
		atmosphere.ammoniaPercentage = RandomBetween(0.0f, 1.0f);
		atmosphere.waterVaporPercentage = RandomBetween(0.0f, 0.5f);
		atmosphere.nitrogenDioxidePercentage = RandomBetween(0.0f, 0.1f);
		atmosphere.otherGasesPercentage = 1.0f;
	}
	else {
		// Exotic/toxic atmosphere
		atmosphere.nitrogenPercentage = RandomBetween(20.0f, 60.0f);
		atmosphere.oxygenPercentage = RandomBetween(0.0f, 10.0f);
		atmosphere.carbonDioxidePercentage = RandomBetween(5.0f, 30.0f);
		atmosphere.methanePercentage = RandomBetween(0.0f, 10.0f);
		atmosphere.sulfurDioxidePercentage = RandomBetween(0.5f, 5.0f);
		atmosphere.ammoniaPercentage = RandomBetween(0.0f, 10.0f);
		atmosphere.waterVaporPercentage = RandomBetween(0.0f, 5.0f);
		atmosphere.nitrogenDioxidePercentage = RandomBetween(0.5f, 5.0f);
		atmosphere.otherGasesPercentage = RandomBetween(5.0f, 20.0f);
	}

	// Normalize to ensure sum is exactly 100%
	atmosphere.normalize();

	greenhouseEffectFactor = greenhouseEffect();
	atmosphereHeight = calculateAtmosphereHeight();
}


float PlanetaryParameters::greenhouseEffect()
{
	// Greenhouse effect calculation based on gas composition
	float ghEffect = (atmosphere.carbonDioxidePercentage * 0.5f +
		atmosphere.methanePercentage * 0.8f +
		atmosphere.waterVaporPercentage * 0.3f +
		atmosphere.nitrogenDioxidePercentage * 0.4f +
		atmosphere.sulfurDioxidePercentage * 0.2f +
		atmosphere.otherGasesPercentage * 0.1f);

	// Clamp to reasonable range (Earth: ~33°C boost, Venus: ~500°C boost)
	ghEffect = clamp(ghEffect, 0.0f, 100.0f);

	return ghEffect;
}
float PlanetaryParameters::calculateAtmosphereHeight()
{
	// Simplified atmosphere height calculation based on composition and pressure
	float baseHeight = (atmosphere.totalPressure * 100.0f) /
		(atmosphere.nitrogenPercentage * 0.78f + atmosphere.oxygenPercentage * 0.21f + 0.01f);

	// Lighter gases extend atmosphere higher
	float methaneEffect = atmosphere.methanePercentage * 5.0f;
	float ammoniaEffect = atmosphere.ammoniaPercentage * 4.0f;

	return baseHeight + methaneEffect + ammoniaEffect;
}
float PlanetaryParameters::calculateTemperatureGradient()
{
	float baseGradient = 25.0f; // Earth-like base
	// rotation effect
	float rotationEffect = 1.0f / (rotationSpeed + 0.3f);

	float pressureEffect = 1.0f / std::sqrt(atmosphere.totalPressure + 0.1f);
	float gradient = baseGradient * rotationEffect * pressureEffect;
	gradient = clamp(gradient * 1.5, 5.0f, 60.0f);
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
	baseStrength = clamp(baseStrength * 0.5f, 0.1f, 1.0f);
	int numZones = cellBoundaries.size() + 1;

	for (int i = 0; i < numZones; i++)
	{
		float zoneStrength = baseStrength;

		// Equatorial zones (Hadley cells) tend to have stronger winds
		// Mid-latitude zones (Ferrel cells) have moderate winds
		// Polar zones have weaker winds
		if (i == 0 || i == numZones - 1) {
			zoneStrength *= RandomBetween(0.5f, 0.7f); // polar zones
		}
		else if (i == numZones / 2 || i == (numZones / 2) - 1 || i == (numZones / 2) + 1) {
			zoneStrength *= RandomBetween(0.7f, 0.9f); // equatorial zones
		}
		else {
			zoneStrength *= RandomBetween(0.6f, 0.8f); // mid-latitude zones
		}

		// Clamp to 0.0 - 1.0
		zoneStrength = clamp(zoneStrength, 1.0f, 0.0f);
		strengths.push_back(zoneStrength);
	}
	return strengths;
}

sf::Color PlanetaryParameters::getAtmosphereColor(float timeOfDay) const
{
	// Base colors for different gas compositions
	sf::Color nitrogenOxygenColor(135, 206, 235); // Sky blue (Rayleigh scattering)
	sf::Color carbonDioxideColor(210, 180, 140); // Tan/butterscotch
	sf::Color methaneColor(80, 200, 200); // Cyan/turquoise
	sf::Color sulfurDioxideColor(255, 230, 120); // Pale yellow
	sf::Color ammoniaColor(240, 240, 220); // Off-white/cream
	sf::Color waterVaporColor(220, 220, 220); // White haze
	sf::Color nitrogenDioxideColor(180, 100, 60); // Reddish-brown
	sf::Color thinAtmosphereColor(25, 25, 40); // Near-black
	sf::Color denseAtmosphereColor(200, 200, 190); // Pale yellowish-white

	// Calculate atmospheric density factor
	float densityFactor = clamp(atmosphere.totalPressure / 1.0f, 0.0f, 5.0f);

	// Calculate scattering strengths (normalized by total percentage)
	float rayleighStrength = (atmosphere.nitrogenPercentage + atmosphere.oxygenPercentage) / 100.0f;
	rayleighStrength *= densityFactor;
	rayleighStrength = clamp(rayleighStrength, 0.0f, 1.0f);

	float co2Strength = (atmosphere.carbonDioxidePercentage / 100.0f) * densityFactor;
	co2Strength = clamp(co2Strength, 0.0f, 1.0f);

	float methaneStrength = (atmosphere.methanePercentage / 100.0f) * densityFactor * 1.5f; // Methane is visually prominent
	methaneStrength = clamp(methaneStrength, 0.0f, 1.0f);

	float so2Strength = (atmosphere.sulfurDioxidePercentage / 100.0f) * densityFactor * 2.0f; // Very visible
	so2Strength = clamp(so2Strength, 0.0f, 1.0f);

	float nh3Strength = (atmosphere.ammoniaPercentage / 100.0f) * densityFactor;
	nh3Strength = clamp(nh3Strength, 0.0f, 1.0f);

	float h2oStrength = (atmosphere.waterVaporPercentage / 100.0f) * densityFactor;
	h2oStrength = clamp(h2oStrength, 0.0f, 1.0f);

	float no2Strength = (atmosphere.nitrogenDioxidePercentage / 100.0f) * densityFactor * 2.5f; // Very colored
	no2Strength = clamp(no2Strength, 0.0f, 1.0f);

	// Weighted color blending
	float totalWeight = rayleighStrength + co2Strength + methaneStrength +
		so2Strength + nh3Strength + h2oStrength + no2Strength + 0.0001f;

	sf::Color baseColor;
	baseColor.r = static_cast<sf::Uint8>(
		(nitrogenOxygenColor.r * rayleighStrength +
			carbonDioxideColor.r * co2Strength +
			methaneColor.r * methaneStrength +
			sulfurDioxideColor.r * so2Strength +
			ammoniaColor.r * nh3Strength +
			waterVaporColor.r * h2oStrength +
			nitrogenDioxideColor.r * no2Strength) / totalWeight
		);
	baseColor.g = static_cast<sf::Uint8>(
		(nitrogenOxygenColor.g * rayleighStrength +
			carbonDioxideColor.g * co2Strength +
			methaneColor.g * methaneStrength +
			sulfurDioxideColor.g * so2Strength +
			ammoniaColor.g * nh3Strength +
			waterVaporColor.g * h2oStrength +
			nitrogenDioxideColor.g * no2Strength) / totalWeight
		);
	baseColor.b = static_cast<sf::Uint8>(
		(nitrogenOxygenColor.b * rayleighStrength +
			carbonDioxideColor.b * co2Strength +
			methaneColor.b * methaneStrength +
			sulfurDioxideColor.b * so2Strength +
			ammoniaColor.b * nh3Strength +
			waterVaporColor.b * h2oStrength +
			nitrogenDioxideColor.b * no2Strength) / totalWeight
		);

	// Atmospheric density effects
	if (atmosphere.totalPressure < 0.1f) {
		// Very thin atmosphere - blend toward space
		float thinFactor = atmosphere.totalPressure / 0.1f;
		baseColor.r = static_cast<sf::Uint8>(baseColor.r * thinFactor + thinAtmosphereColor.r * (1.0f - thinFactor));
		baseColor.g = static_cast<sf::Uint8>(baseColor.g * thinFactor + thinAtmosphereColor.g * (1.0f - thinFactor));
		baseColor.b = static_cast<sf::Uint8>(baseColor.b * thinFactor + thinAtmosphereColor.b * (1.0f - thinFactor));
	}
	else if (atmosphere.totalPressure > 2.0f) {
		// Very dense atmosphere - blend toward hazy white
		float denseFactor = clamp((atmosphere.totalPressure - 2.0f) / 3.0f, 0.0f, 1.0f);
		baseColor.r = static_cast<sf::Uint8>(baseColor.r * (1.0f - denseFactor) + denseAtmosphereColor.r * denseFactor);
		baseColor.g = static_cast<sf::Uint8>(baseColor.g * (1.0f - denseFactor) + denseAtmosphereColor.g * denseFactor);
		baseColor.b = static_cast<sf::Uint8>(baseColor.b * (1.0f - denseFactor) + denseAtmosphereColor.b * denseFactor);
	}

	// Time of day effects
	// Map timeOfDay (0.0-1.0) to hours (0-24)
	float hourOfDay = timeOfDay * 24.0f;
	
	// Calculate sun elevation angle (simplified - peaks at noon, lowest at midnight)
	// 0° = horizon (sunrise/sunset), 90° = directly overhead, -90° = midnight
	float sunElevation = std::sin((timeOfDay - 0.25f) * 2.0f * 3.14159f) * 90.0f;
	
	// Calculate horizon proximity with sharp peak
	// Use exponential falloff for realistic sunset/sunrise duration (~1-2 hours)
	// Peak occurs when sun is near horizon (elevation near 0°)
	float horizonDistance = std::abs(sunElevation) / 90.0f; // 0.0 at horizon, 1.0 at zenith/nadir
	
	// Sharp Gaussian-like peak centered at horizon
	// This creates a ~1-2 hour window of peak reddening
	float horizonSharpness = 8.0f; // Higher = sharper peak
	float horizonProximity = std::exp(-horizonSharpness * horizonDistance * horizonDistance);
	
	// Only apply horizon effects during day (when sun is above horizon)
	if (sunElevation < 0.0f) {
		horizonProximity = 0.0f; // No sunset colors at night
	}
	
	// Sunset/sunrise reddening (stronger with more scattering gases)
	if (atmosphere.totalPressure > 0.1f && horizonProximity > 0.01f) {
		float scatteringFactor = rayleighStrength + co2Strength * 0.5f + methaneStrength * 0.3f;
		float redShift = horizonProximity * scatteringFactor * 150.0f;
		baseColor.r = static_cast<sf::Uint8>(std::min(255, static_cast<int>(baseColor.r) + static_cast<int>(redShift)));
		baseColor.g = static_cast<sf::Uint8>(std::max(0, static_cast<int>(baseColor.g) - static_cast<int>(redShift * 0.3f)));
		baseColor.b = static_cast<sf::Uint8>(std::max(0, static_cast<int>(baseColor.b) - static_cast<int>(redShift * 0.5f)));
	}

	// Darkness at night (smooth transition)
	// Full brightness when sun > 0°, fade to dark when sun < -18° (astronomical twilight)
	float brightness = 1.0f;
	if (sunElevation < 0.0f) {
		// Transition from full brightness to ~30% over 18° (civil twilight)
		brightness = clamp(1.0f + (sunElevation / 18.0f) * 0.7f, 0.3f, 1.0f);
	}
	
	baseColor.r = static_cast<sf::Uint8>(baseColor.r * brightness);
	baseColor.g = static_cast<sf::Uint8>(baseColor.g * brightness);
	baseColor.b = static_cast<sf::Uint8>(baseColor.b * brightness);

	baseColor.a = 255; // Full opacity

	return baseColor;
}