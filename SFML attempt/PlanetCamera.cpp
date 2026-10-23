#include "PlanetCamera.hpp"
#include "SphereMath.hpp"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace {
    // Focal length in pixels for the vertical field of view
    float focalPixels(const glm::vec2& viewport)
    {
        return 0.5f * viewport.y / std::tan(0.5f * glm::radians(PlanetCamera::FOV_Y));
    }

    float wrapLon(float lon)
    {
        lon = std::fmod(lon + 180.f, 360.f);
        if (lon <= 0.f) lon += 360.f;
        return lon - 180.f;
    }
}

float PlanetCamera::minZoom(const glm::vec2& viewport)
{
    // The globe's silhouette has radius f / sqrt(d^2 - 1) pixels at eye distance d
    const float f = focalPixels(viewport);
    const float radius = 0.4f * std::min(viewport.x, viewport.y);
    const float d = std::sqrt(1.f + (f / radius) * (f / radius));
    return f / (d - 1.f);
}

void PlanetCamera::clampTo(const glm::vec2& viewport, float maxZoom)
{
    lat = glm::clamp(lat, -maxLat(), maxLat());
    lon = wrapLon(lon);
    const float lo = minZoom(viewport);
    zoom = glm::clamp(zoom, lo, std::max(lo, maxZoom));
}

float PlanetCamera::maxLat() const
{
    // The flat view can't centre past the Mercator cut, beyond it there is no map
    return mode == ViewMode::Flat ? std::min(MAX_LAT, mapproj::maxLat(projection)) : MAX_LAT;
}

float PlanetCamera::flatScale() const
{
    return zoom / mapproj::northScale(projection, lat);
}

glm::vec2 PlanetCamera::flatCentre() const
{
    return mapproj::project(projection, lat, lon);
}

glm::mat4 PlanetCamera::flatViewProj(const glm::vec2& viewport) const
{
    // Orthographic: subtract the centre, then scale pixels to the -1..1 of clip space
    const float s = flatScale();
    const glm::vec2 centre = flatCentre();
    glm::mat4 m(1.f);
    m[0][0] = 2.f * s / viewport.x;
    m[1][1] = 2.f * s / viewport.y;
    m[3][0] = -centre.x * m[0][0];
    m[3][1] = -centre.y * m[1][1];
    return m;
}

float PlanetCamera::globeDistance(const glm::vec2& viewport) const
{
    // At distance d - 1 above the surface one radian of arc covers f / (d - 1) pixels
    return 1.f + focalPixels(viewport) / zoom;
}

glm::vec3 PlanetCamera::globeEye(const glm::vec2& viewport) const
{
    return globeDistance(viewport) * latLonToUnit(lat, lon);
}

glm::mat4 PlanetCamera::globeViewProj(const glm::vec2& viewport) const
{
    const float d = globeDistance(viewport);
    const glm::vec3 centre = latLonToUnit(lat, lon);
    glm::vec3 east, north;
    tangentFrame(centre, east, north);

    // Nothing on the sphere is closer than the point straight below, nothing is further than d + 1
    const float nearPlane = 0.5f * (d - 1.f);
    const float farPlane = d + 1.f;
    const glm::mat4 proj = glm::perspective(glm::radians(FOV_Y), viewport.x / viewport.y, nearPlane, farPlane);
    const glm::mat4 view = glm::lookAt(d * centre, glm::vec3(0.f), north);
    return proj * view;
}

bool PlanetCamera::globePick(const glm::vec2& pixel, const glm::vec2& viewport, glm::vec3& hit) const
{
    const glm::mat4 inv = glm::inverse(globeViewProj(viewport));
    const glm::vec2 ndc(2.f * pixel.x / viewport.x - 1.f, 1.f - 2.f * pixel.y / viewport.y);
    glm::vec4 nearPoint = inv * glm::vec4(ndc, -1.f, 1.f);
    glm::vec4 farPoint = inv * glm::vec4(ndc, 1.f, 1.f);
    const glm::vec3 origin = glm::vec3(nearPoint) / nearPoint.w;
    const glm::vec3 dir = glm::normalize(glm::vec3(farPoint) / farPoint.w - origin);

    // |origin + t dir| = 1, nearest root
    const float b = glm::dot(origin, dir);
    const float c = glm::dot(origin, origin) - 1.f;
    const float disc = b * b - c;
    if (disc < 0.f) return false;
    const float t = -b - std::sqrt(disc);
    if (t < 0.f) return false;
    hit = glm::normalize(origin + t * dir);
    return true;
}

void PlanetCamera::drag(const glm::vec2& deltaPixels)
{
    // Dragging right pulls the surface east to west, dragging down shows more of the north.
    // Longitude speeds up with 1 / cos(lat), capped so it stays usable next to the poles
    const float cosLat = std::max(std::cos(glm::radians(lat)), 0.1f);
    lon -= glm::degrees(deltaPixels.x / (zoom * cosLat));
    lat += glm::degrees(deltaPixels.y / zoom);
    lat = glm::clamp(lat, -maxLat(), maxLat());
    lon = wrapLon(lon);
}

void PlanetCamera::zoomAt(float factor, const glm::vec2& pixel, const glm::vec2& viewport, float maxZoom)
{
    glm::vec3 before, after;
    const bool hadBefore = globePick(pixel, viewport, before);
    zoom *= factor;
    clampTo(viewport, maxZoom);
    if (!hadBefore) return;

    // Shift the centre by how far the point under the cursor slid. One shift is only right to
    // first order (a few pixels off for big steps), a few more make it exact
    const glm::vec2 llBefore = unitToLatLon(before);
    for (int i = 0; i < 4; i++) {
        if (!globePick(pixel, viewport, after)) return;
        const glm::vec2 llAfter = unitToLatLon(after);
        const float dLat = llBefore.x - llAfter.x;
        const float dLon = wrapLon(llBefore.y - llAfter.y);
        lat += dLat;
        lon += dLon;
        clampTo(viewport, maxZoom);
        if (std::abs(dLat) < 1e-5f && std::abs(dLon) < 1e-5f) break;
    }
}
