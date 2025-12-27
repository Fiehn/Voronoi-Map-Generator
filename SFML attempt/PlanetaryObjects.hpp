#pragma once
#include "util.hpp"

struct AtmosphericComposition {
	float nitrogenPercentage = 78.0f; // Percentage of Nitrogen
	float oxygenPercentage = 21.0f;   // Percentage of Oxygen
	float carbonDioxidePercentage = 0.04f; // Percentage of Carbon Dioxide
	float otherGasesPercentage = 0.96f; // Percentage of other gases
	float totalPressure = 1.0f; // Total atmospheric pressure in atm
};

class PlanetaryParameters {
public:
	float rotationSpeed = 1.0f; // Rotation speed of the planet (relative to Earth)
	bool progradeRotation = true; // True for prograde, false for retrograde
	float axialTilt = 23.5f; // Axial tilt in degrees
	float equatorToPoleTemp = 20.0f; // Temperature difference from equator to pole
	float greenhouseEffectFactor = 1.0f; // Greenhouse effect multiplier
	float atmosphereHeight = 100.0f; // Height of the atmosphere in km
	AtmosphericComposition atmosphere; // Atmospheric composition
	
	void initializeEarthLike();
	void initializeMarsLike();
	void initialize();

	float getCoriolisStrenght() const;
	float getAtmosphericCirculationStrength() const;
	int getNumberOfCirculationCells() const;
	std::vector<float> calculateCellBoundaries() const;
	std::vector<float> calculateWindDirections(const std::vector<float>& cellBoundaries) const;
	std::vector<float> calculateWindStrengths(const std::vector<float>& cellBoundaries) const;
private:
	float greenhouseEffect();
	float calculateAtmosphereHeight();
	float calculateTemperatureGradient();
	float getRotationPeriod();
	float calculateAxialTiltEffect();
};

