#pragma once
#include "util.hpp"

struct AtmosphericComposition {
	float nitrogenPercentage = 78.0f; // Percentage of Nitrogen
	float oxygenPercentage = 21.0f;   // Percentage of Oxygen
	float carbonDioxidePercentage = 0.04f; // Percentage of Carbon Dioxide
	float methanePercentage = 0.0f; // Methane (CH4) - cyan/blue-green skies
	float sulfurDioxidePercentage = 0.0f; // Sulfur Dioxide (SO2) - yellow/amber tones
	float ammoniaPercentage = 0.0f; // Ammonia (NH3) - white/pale yellow
	float waterVaporPercentage = 0.0f; // Water vapor (H2O) - white haze/clouds
	float nitrogenDioxidePercentage = 0.0f; // Nitrogen Dioxide (NO2) - reddish-brown
	float otherGasesPercentage = 0.96f; // Percentage of other gases
	float totalPressure = 1.0f; // Total atmospheric pressure in atm

	float getTotalPercentage() const;
	void normalize();
};

struct SeasonalModifiers {
	float temperatureAmplitude = 0.0f; // Seasonal temperature amplitude
	float preceipitationMultiplier = 1.0f;
	float windStrengthMultiplier = 1.0f;
	float humidityMultiplier = 1.0f;
	float daylightHoursVariation = 0.0f; // Variation in daylight hours
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
	
	float orbitalPeriod = 365.25f; // Orbital period in days
	float orbitalEccentricity = 0.0167f; // Orbital eccentricity

	void initializeEarthLike();
	void initializeMarsLike();
	void initialize();

	float getCoriolisStrenght() const;
	float getAtmosphericCirculationStrength() const;
	int getNumberOfCirculationCells() const;
	std::vector<float> calculateCellBoundaries() const;
	std::vector<float> calculateWindDirections(const std::vector<float>& cellBoundaries) const;
	std::vector<float> calculateWindStrengths(const std::vector<float>& cellBoundaries) const;
	sf::Color getAtmosphereColor(float timeOfDay) const;

	float getDayLength() const { return 24.0f / rotationSpeed; }
	float getYearLength() const { return orbitalPeriod; }

	SeasonalModifiers getSeasonalModifiers(float dayOfYear, float normalizedLatitude, bool isNorthernHemisphere) const;
	float getSolarIntensity(float dayOfYear, float normalizedLatitude, bool isNorthernHemisphere) const;
	float getSeasonalTemperatureOffset(float dayOfYear, float normalizedLatitude, bool isNorthernHemisphere) const;
private:
	float greenhouseEffect();
	float calculateAtmosphereHeight();
	float calculateTemperatureGradient();
	float getRotationPeriod();
	float calculateAxialTiltEffect();
};

