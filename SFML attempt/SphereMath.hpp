#pragma once
#include <cmath>
#include <glm/glm.hpp>

// Unit sphere conventions, shared by the simulation and both views:
//  - lat/lon in degrees, lat in [-90, 90], lon in (-180, 180]
//  - +Y is the north pole, lon 0 sits on +Z and east points towards +X,
//    so (east, north, up) is a right handed frame everywhere
//  - bearings are compass bearings in degrees: 0 = north, 90 = east, clockwise

inline glm::vec3 latLonToUnit(float lat, float lon)
{
	float phi = glm::radians(lat);
	float lambda = glm::radians(lon);
	float c = std::cos(phi);
	return glm::vec3(c * std::sin(lambda), std::sin(phi), c * std::cos(lambda));
}

// Returns (lat, lon) in degrees, p does not need to be normalised
inline glm::vec2 unitToLatLon(const glm::vec3& p)
{
	float horizontal = std::sqrt(p.x * p.x + p.z * p.z);
	float lat = glm::degrees(std::atan2(p.y, horizontal));
	float lon = glm::degrees(std::atan2(p.x, p.z));
	return glm::vec2(lat, lon);
}

// East and north unit tangents at p. At the poles east falls back to +X
inline void tangentFrame(const glm::vec3& p, glm::vec3& east, glm::vec3& north)
{
	// cross((0,1,0), p) without the zero terms
	glm::vec3 e(p.z, 0.0f, -p.x);
	float len = glm::length(e);
	east = len > 1e-6f ? e / len : glm::vec3(1.0f, 0.0f, 0.0f);
	north = glm::normalize(glm::cross(p, east));
}

// Unit tangent at p pointing along the compass bearing
inline glm::vec3 bearingToTangent(const glm::vec3& p, float bearing)
{
	glm::vec3 east, north;
	tangentFrame(p, east, north);
	float b = glm::radians(bearing);
	return std::sin(b) * east + std::cos(b) * north;
}

// Compass bearing in [0, 360) of the tangent part of v at p
inline float tangentToBearing(const glm::vec3& p, const glm::vec3& v)
{
	glm::vec3 east, north;
	tangentFrame(p, east, north);
	float bearing = glm::degrees(std::atan2(glm::dot(v, east), glm::dot(v, north)));
	return bearing < 0.0f ? bearing + 360.0f : bearing;
}

// Great circle angle in radians between two unit vectors.
// atan2 form stays accurate for tiny and near antipodal angles, unlike acos(dot)
inline float angleBetween(const glm::vec3& a, const glm::vec3& b)
{
	return std::atan2(glm::length(glm::cross(a, b)), glm::dot(a, b));
}
