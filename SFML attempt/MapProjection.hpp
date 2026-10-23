#pragma once
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// Flat map projections of the unit sphere. Map coordinates are in radians: x is the longitude
// (unbounded, the map repeats every 2 pi to the east and west) and y grows to the north.
//
// GlobeRenderer's vertex shader repeats project() in GLSL, keep the two in step.
enum class Projection {
	Mercator = 1,        // Conformal, cut at MERCATOR_CUT_DEG since the poles are at infinity
	Equirectangular = 2, // y = lat, the whole sphere fits
};

namespace mapproj {

	constexpr float MERCATOR_CUT_DEG = 85.f;
	// Mercator clamps latitudes to this before projecting, so a point on the pole stays finite.
	// 89.99 lands at y ~ 9.3, far outside the cut
	constexpr float POLE_CLAMP_DEG = 89.99f;

	// Latitude where the map ends, degrees
	inline float maxLat(Projection p)
	{
		return p == Projection::Mercator ? MERCATOR_CUT_DEG : 90.f;
	}

	// Whether the map repeats east-west. Both projections do, the flag is there for future ones
	inline bool wrapsHorizontally(Projection) { return true; }

	// Map point of lat/lon in degrees, lon may be outside (-180, 180] (unwrapped)
	inline glm::vec2 project(Projection p, float lat, float lon)
	{
		if (p != Projection::Mercator) return glm::vec2(glm::radians(lon), glm::radians(lat));
		const float phi = glm::radians(std::clamp(lat, -POLE_CLAMP_DEG, POLE_CLAMP_DEG));
		return glm::vec2(glm::radians(lon), std::log(std::tan(0.25f * glm::pi<float>() + 0.5f * phi)));
	}

	// (lat, lon) in degrees of a map point, lon is not wrapped
	inline glm::vec2 unproject(Projection p, const glm::vec2& map)
	{
		const float phi = p == Projection::Mercator ? std::atan(std::sinh(map.y)) : map.y;
		return glm::vec2(glm::degrees(phi), glm::degrees(map.x));
	}

	// Map units per radian of north-south arc at lat (degrees). Mercator is conformal, so this
	// is its scale in every direction; equirectangular stretches east-west by 1/cos(lat) instead
	inline float northScale(Projection p, float lat)
	{
		if (p != Projection::Mercator) return 1.f;
		return 1.f / std::cos(glm::radians(std::clamp(lat, -POLE_CLAMP_DEG, POLE_CLAMP_DEG)));
	}

	// lon plus the whole turn that puts it nearest to reference, degrees
	inline double unwrapLon(double lon, double reference)
	{
		return lon - 360.0 * std::round((lon - reference) / 360.0);
	}

}
