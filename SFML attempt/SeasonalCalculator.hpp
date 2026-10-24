#pragma once
#include "cell.hpp"
#include "GlobalWorldObjects.hpp"
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

// Seasons by latitude in degrees, measured from the geographic equator
class SeasonalCalculator {
public:
	explicit SeasonalCalculator(const GlobalWorldObjects& globals) :
		m_globals(globals) {
	}

	// == Distribution Getters ==
	ClimateDistribution getTemperatureDistribution(const Cell& cell, float lat, float dayOfYear) const {
		float normalizedLatitude = getNormalizedLatitude(lat);
		bool isNorthern = isNorthernHemisphere(lat);

		// Seasonal offset for mean
		float seasonalOffset = m_globals.planetaryParams.getSeasonalTemperatureOffset(dayOfYear, normalizedLatitude, isNorthern);
		float mean = cell.temp + seasonalOffset;

		// variance increases in winter
		float solarIntensity = m_globals.planetaryParams.getSolarIntensity(dayOfYear, normalizedLatitude, isNorthern);
		float winterFactor = 1.0f + (1.0f - solarIntensity) * 0.5; // More variance in winter
		float variance = cell.tempVariance * winterFactor;

		return ClimateDistribution(mean, variance, -100.0f, 100.0f);
	}
	ClimateDistribution getPercepitationDistribution(const Cell& cell, float lat, float dayOfYear) const {
		float normalizedLatitude = getNormalizedLatitude(lat);
		bool isNorthern = isNorthernHemisphere(lat);

		SeasonalModifiers mods = m_globals.planetaryParams.getSeasonalModifiers(dayOfYear, normalizedLatitude, isNorthern);
		float mean = cell.percepitation * mods.preceipitationMultiplier;

		float variance = cell.percepitationVariance * mods.preceipitationMultiplier;
		return ClimateDistribution(mean, variance, 0.0f, 500.0f);
	}
	ClimateDistribution getWindStrengthDistribution(const Cell& cell, float lat, float dayOfYear) const {
		float normalizedLatitude = getNormalizedLatitude(lat);
		bool isNorthern = isNorthernHemisphere(lat);
		SeasonalModifiers mods = m_globals.planetaryParams.getSeasonalModifiers(dayOfYear, normalizedLatitude, isNorthern);
		float mean = cell.windStr * mods.windStrengthMultiplier;
		float variance = cell.windStrVariance * mods.windStrengthMultiplier;
		return ClimateDistribution(mean, variance, 0.0f, 1.0f);
	}
	ClimateDistribution getHumidityDistribution(const Cell& cell, float lat, float dayOfYear) const {
		float normalizedLatitude = getNormalizedLatitude(lat);
		bool isNorthern = isNorthernHemisphere(lat);
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
	float getTemperature(const Cell& cell, float lat, float dayOfYear) const {
		float normalizedLatitude = getNormalizedLatitude(lat);
		bool isNorthern = isNorthernHemisphere(lat);
		float seasonalOffset = m_globals.planetaryParams.getSeasonalTemperatureOffset(dayOfYear, normalizedLatitude, isNorthern);
		return cell.temp + seasonalOffset;
	}
	bool hasSnowCover(const Cell& cell, float lat, float dayOfYear) const {
		float temp = getTemperature(cell, lat, dayOfYear);
		return temp < 0.0f && !cell.oceanBool;
	}
	int getSeason(float lat, float dayOfYear) const {
		float yearLength = m_globals.planetaryParams.getYearLength();
		float phase = std::fmod(dayOfYear, yearLength) / yearLength;
		bool isNorthern = isNorthernHemisphere(lat);
		if (!isNorthern) {
			phase = std::fmod(phase + 0.5f, 1.0f);
		}
		if (phase < 0.25f) return 0; // Spring
		else if (phase < 0.5f) return 1; // Summer
		else if (phase < 0.75f) return 2; // Autumn
		else return 3; // Winter
	}
	const char* getSeasonName(float lat, float dayOfYear) const {
		static const char* seasonNames[4] = { "Spring", "Summer", "Autumn", "Winter" };
		return seasonNames[getSeason(lat, dayOfYear)];
	}

	// == Extremes ==

	// Get temperature extremes at a cell
	std::pair<float, float> getTemperatureExtremes(const Cell& cell, float lat) const {
		float minTemp = 999.0f;
		float maxTemp = -999.0f;
		float yearLength = m_globals.planetaryParams.getYearLength();
		for (int month = 0; month < 12; ++month) {
			float dayOfYear = month * (yearLength / 12.0f) + (yearLength / 24.0f); // Mid-month
			ClimateDistribution tempDist = getTemperatureDistribution(cell, lat, dayOfYear);
			minTemp = std::min(minTemp, tempDist.getMin());
			maxTemp = std::max(maxTemp, tempDist.getMax());
		}
		return { minTemp, maxTemp };
	}

private:
	const GlobalWorldObjects& m_globals;

	// 0 at the equator, 1 at the poles. The flat map divided by the whole map height, so it stopped at 0.5
	float getNormalizedLatitude(float lat) const
	{
		return std::min(std::abs(lat) / 90.0f, 1.0f);
	}
	bool isNorthernHemisphere(float lat) const
	{
		return lat > 0.0f;
	}
};
