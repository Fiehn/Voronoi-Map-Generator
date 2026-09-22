#pragma once
#include "cell.hpp"
#include "GlobalWorldObjects.hpp"
#include <SFML/System/Vector2.hpp>
#include "util.hpp"

struct ClimateDistribution {
	float mean = 0.0f;
	float variance = 0.0f; // My variance is actually standard deviation
	float min = -999.0f;
	float max = 999.0f;

	ClimateDistribution() = default;
	ClimateDistribution(float m, float v, float mi, float ma) :
		mean(m), variance(v), min(mi), max(ma) {
	}

	// Sample a value from the distribution
	float sample() const {
		if (variance < 0.0001f) return mean;
		float value = normalDistPDF(mean, variance);
		return clamp(value, min, max);
	}

	// Get value at specific percentile (0.0 to 1.0)
	float atPercentile(float p) const {
		// Using inverse error function approximation for normal distribution
		float offset = (p - 0.5f) * 2.0f * variance * 2.5f; // Approximation
		return clamp(mean + offset, min, max);
	}

	// Get the range of the distribution
	float getMin() const { return atPercentile(0.02f); }
	float getMax() const { return atPercentile(0.98f); }
};

class SeasonalCalculator {
public:
	SeasonalCalculator(const GlobalWorldObjects& globals, float equatorY) :
		m_globals(globals), m_equatorY(equatorY) {
	}

	// == Distribution Getters ==
	ClimateDistribution getTemperatureDistribution(const Cell& cell, const sf::Vector2f& position, float dayOfYear) const {
		float normalizedLatitude = getNormalizedLatitude(position);
		bool isNorthern = isNorthernHemisphere(position);

		// Seasonal offset for mean
		float seasonalOffset = m_globals.planetaryParams.getSeasonalTemperatureOffset(dayOfYear, normalizedLatitude, isNorthern);
		float mean = cell.temp + seasonalOffset;

		// variance increases in winter
		float solarIntensity = m_globals.planetaryParams.getSolarIntensity(dayOfYear, normalizedLatitude, isNorthern);
		float winterFactor = 1.0f + (1.0f - solarIntensity) * 0.5; // More variance in winter
		float variance = cell.tempVariance * winterFactor;

		return ClimateDistribution(mean, variance, -100.0f, 100.0f);
	}
	ClimateDistribution getPercepitationDistribution(const Cell& cell, const sf::Vector2f& position, float dayOfYear) const {
		float normalizedLatitude = getNormalizedLatitude(position);
		bool isNorthern = isNorthernHemisphere(position);

		SeasonalModifiers mods = m_globals.planetaryParams.getSeasonalModifiers(dayOfYear, normalizedLatitude, isNorthern);
		float mean = cell.percepitation * mods.preceipitationMultiplier;

		float variance = cell.percepitationVariance * mods.preceipitationMultiplier;
		return ClimateDistribution(mean, variance, 0.0f, 500.0f);
	}
	ClimateDistribution getWindStrengthDistribution(const Cell& cell, const sf::Vector2f& position, float dayOfYear) const {
		float normalizedLatitude = getNormalizedLatitude(position);
		bool isNorthern = isNorthernHemisphere(position);
		SeasonalModifiers mods = m_globals.planetaryParams.getSeasonalModifiers(dayOfYear, normalizedLatitude, isNorthern);
		float mean = cell.windStr * mods.windStrengthMultiplier;
		float variance = cell.windStrVariance * mods.windStrengthMultiplier;
		return ClimateDistribution(mean, variance, 0.0f, 1.0f);
	}
	ClimateDistribution getHumidityDistribution(const Cell& cell, const sf::Vector2f& position, float dayOfYear) const {
		float normalizedLatitude = getNormalizedLatitude(position);
		bool isNorthern = isNorthernHemisphere(position);
		SeasonalModifiers mods = m_globals.planetaryParams.getSeasonalModifiers(dayOfYear, normalizedLatitude, isNorthern);
		float mean = cell.humidity * mods.humidityMultiplier;
		float variance = cell.humidityVariance * mods.humidityMultiplier;
		return ClimateDistribution(mean, variance, 0.0f, 1.0f);
	}
	ClimateDistribution getWindDirectionDistribution(const Cell& cell) const {
		// Wind direction does not vary seasonally in this model
		return ClimateDistribution(cell.windDir, cell.windDirVariance, 0.0f, 360.0f);
	}

	// Get the actual temperature at a given cell considering seasonal variation
	float getTemperature(const Cell& cell, const sf::Vector2f& position, float dayOfYear) const {
		float normalizedLatitude = getNormalizedLatitude(position);
		bool isNorthern = isNorthernHemisphere(position);
		float seasonalOffset = m_globals.planetaryParams.getSeasonalTemperatureOffset(normalizedLatitude, isNorthern, dayOfYear);
		return cell.temp + seasonalOffset;
	}
	bool hasSnowCover(const Cell& cell, const sf::Vector2f& position, float dayOfYear) const {
		float temp = getTemperature(cell, position, dayOfYear);
		return temp < 0.0f && !cell.oceanBool;
	}
	int getSeason(const sf::Vector2f& position, float dayOfYear) const {
		float yearLength = m_globals.planetaryParams.getYearLength();
		float phase = std::fmod(dayOfYear, yearLength) / yearLength;
		bool isNorthern = isNorthernHemisphere(position);
		if (!isNorthern) {
			phase = std::fmod(phase + 0.5f, 1.0f);
		}
		if (phase < 0.25f) return 0; // Spring
		else if (phase < 0.5f) return 1; // Summer
		else if (phase < 0.75f) return 2; // Autumn
		else return 3; // Winter
	}
	const char* getSeasonName(const sf::Vector2f& position, float dayOfYear) const {
		static const char* seasonNames[4] = { "Spring", "Summer", "Autumn", "Winter" };
		return seasonNames[getSeason(position, dayOfYear)];
	}

	// == Extremes ==

	// Get temperature extremes at a cell
	std::pair<float, float> getTemperatureExtremes(const Cell& cell, const sf::Vector2f& position) const {
		float minTemp = 999.0f;
		float maxTemp = -999.0f;
		float yearLength = m_globals.planetaryParams.getYearLength();
		for (int month = 0; month < 12; ++month) {
			float dayOfYear = month * (yearLength / 12.0f) + (yearLength / 24.0f); // Mid-month
			ClimateDistribution tempDist = getTemperatureDistribution(cell, position, dayOfYear);
			minTemp = std::min(minTemp, tempDist.getMin());
			maxTemp = std::max(maxTemp, tempDist.getMax());
		}
		return { minTemp, maxTemp };
	}

private:
	const GlobalWorldObjects& m_globals;
	float m_equatorY;

	float getNormalizedLatitude(const sf::Vector2f& position) const
	{
		float distFromEquator = std::abs(position.y - m_equatorY);
		float maxDist = m_equatorY * 2; // map height is 2 * equatorY
		return std::min(distFromEquator / maxDist, 1.0f);
	}
	bool isNorthernHemisphere(const sf::Vector2f& position) const
	{
		return position.y < m_equatorY;
	}
};
