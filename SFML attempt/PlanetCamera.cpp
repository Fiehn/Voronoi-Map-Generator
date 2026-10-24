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
    lon = fitLon(lon);
    float lo = minZoom(viewport);
    float hi = maxZoom;
    if (mode == ViewMode::Flat) {
        // The flat limits are on the map scale, so panning never hits them. Zoomed out, a flat
        // view in Auto needs no limit of its own, the globe takes over long before
        const float north = mapproj::northScale(projection, lat);
        if (lock == ViewLock::Flat) lo = minFlatScale(viewport) * north;
        hi = maxZoom * north;
    }
    zoom = glm::clamp(zoom, lo, std::max(lo, hi));
}

float PlanetCamera::visibleLatSpan(const glm::vec2& viewport) const
{
    return glm::degrees(viewport.y / zoom);
}

void PlanetCamera::setLock(ViewLock newLock, const glm::vec2& viewport, float maxZoom)
{
    lock = newLock;
    if (lock == ViewLock::Globe) mode = ViewMode::Globe;
    else if (lock == ViewLock::Flat) mode = ViewMode::Flat;
    else mode = visibleLatSpan(viewport) < SWITCH_LAT_SPAN_DEG ? ViewMode::Flat : ViewMode::Globe;
    clampTo(viewport, maxZoom);
}

float PlanetCamera::maxLat() const
{
    // The flat view can't centre past the Mercator cut, beyond it there is no map
    return mode == ViewMode::Flat ? std::min(MAX_LAT, mapproj::maxLat(projection)) : MAX_LAT;
}

bool PlanetCamera::flatWraps() const
{
    return wrap && mapproj::wrapsHorizontally(projection);
}

float PlanetCamera::fitLon(float l) const
{
    // Wrapping 180 would be -180 and jump the centre from one edge of a static map to the other
    if (mode == ViewMode::Flat && !flatWraps()) return glm::clamp(l, -180.f, 180.f);
    return wrapLon(l);
}

float PlanetCamera::flatScale() const
{
    return zoom / mapproj::northScale(projection, lat);
}

void PlanetCamera::setFlatScale(float scale)
{
    zoom = scale * mapproj::northScale(projection, lat);
}

float PlanetCamera::minFlatScale(const glm::vec2& viewport) const
{
    const float maxY = mapproj::project(projection, mapproj::maxLat(projection), 0.f).y;
    return viewport.y / (2.f * maxY);
}

glm::vec2 PlanetCamera::flatCentre() const
{
    return mapproj::project(projection, lat, lon);
}

void PlanetCamera::setFlatCentre(const glm::vec2& map)
{
    const float scale = flatScale();
    const glm::vec2 ll = mapproj::unproject(projection, map);
    lat = glm::clamp(ll.x, -maxLat(), maxLat());
    lon = fitLon(ll.y);
    setFlatScale(scale);
}

glm::vec2 PlanetCamera::flatPixelToMap(const glm::vec2& pixel, const glm::vec2& viewport) const
{
    const float s = flatScale();
    return flatCentre() + glm::vec2((pixel.x - 0.5f * viewport.x) / s, (0.5f * viewport.y - pixel.y) / s);
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

bool PlanetCamera::flatPick(const glm::vec2& pixel, const glm::vec2& viewport, glm::vec3& hit) const
{
    const glm::vec2 map = flatPixelToMap(pixel, viewport);
    const float maxY = mapproj::project(projection, mapproj::maxLat(projection), 0.f).y;
    if (std::abs(map.y) > maxY) return false;
    if (!flatWraps() && std::abs(map.x) > glm::pi<float>()) return false;
    const glm::vec2 ll = mapproj::unproject(projection, map);
    hit = latLonToUnit(ll.x, ll.y);
    return true;
}

bool PlanetCamera::pick(const glm::vec2& pixel, const glm::vec2& viewport, glm::vec3& hit) const
{
    return mode == ViewMode::Flat ? flatPick(pixel, viewport, hit) : globePick(pixel, viewport, hit);
}

void PlanetCamera::drag(const glm::vec2& deltaPixels)
{
    if (mode == ViewMode::Flat) {
        // The map point under the cursor follows it exactly, pixel y runs down and map y up
        const float s = flatScale();
        setFlatCentre(flatCentre() + glm::vec2(-deltaPixels.x / s, deltaPixels.y / s));
        return;
    }

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
    if (mode == ViewMode::Flat) zoomFlatAt(factor, pixel, viewport, maxZoom);
    else zoomGlobeAt(factor, pixel, viewport, maxZoom);
    if (lock != ViewLock::Auto) return;

    // Both views agree at the screen centre, so switching only needs the new mode
    const float span = visibleLatSpan(viewport);
    if (factor > 1.f && mode == ViewMode::Globe && span < SWITCH_LAT_SPAN_DEG) mode = ViewMode::Flat;
    else if (factor < 1.f && mode == ViewMode::Flat && span > SWITCH_LAT_SPAN_DEG) mode = ViewMode::Globe;
    else return;
    clampTo(viewport, maxZoom);
}

void PlanetCamera::zoomFlatAt(float factor, const glm::vec2& pixel, const glm::vec2& viewport, float maxZoom)
{
    // Scale the map about the point under the cursor. The limits apply after, so at a limit
    // the point may slide a little
    const glm::vec2 anchor = flatPixelToMap(pixel, viewport);
    const float s = flatScale() * factor;
    setFlatScale(s);
    setFlatCentre(anchor - glm::vec2((pixel.x - 0.5f * viewport.x) / s, (0.5f * viewport.y - pixel.y) / s));
    clampTo(viewport, maxZoom);
}

void PlanetCamera::zoomGlobeAt(float factor, const glm::vec2& pixel, const glm::vec2& viewport, float maxZoom)
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
