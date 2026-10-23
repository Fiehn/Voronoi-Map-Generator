#pragma once
#include <glm/glm.hpp>
#include "MapProjection.hpp"

enum class ViewMode { Globe, Flat };

// One camera shared by every view of the planet: the point at the screen centre and how many
// pixels one radian of arc covers there. North is always up.
//
// The globe is drawn in perspective with the eye straight above the centre point. Its distance
// follows from the zoom, so zoomed out the globe looks almost orthographic and zoomed in it
// looks like flying low over the surface.
//
// The flat view is the projected map scaled so one radian of north-south arc at the centre is
// zoom pixels, so both views agree at the screen centre and switching between them is seamless.
struct PlanetCamera {
    float lat = 20.f;   // Degrees, kept short of the poles so north-up stays defined
    float lon = 0.f;    // Degrees, (-180, 180]
    float zoom = 500.f; // Pixels per radian at the screen centre
    ViewMode mode = ViewMode::Globe;
    Projection projection = Projection::Mercator; // Of the flat view

    static constexpr float MAX_LAT = 89.f;
    static constexpr float FOV_Y = 30.f; // Degrees, vertical field of view of the globe

    // Zoom where the whole globe fits the viewport with a margin
    static float minZoom(const glm::vec2& viewport);

    // Keeps lat, lon and zoom in range, maxZoom comes from the cell size
    void clampTo(const glm::vec2& viewport, float maxZoom);
    float maxLat() const; // Of the centre in the current mode

    // Flat view, see MapProjection.hpp for map coordinates
    float flatScale() const;      // Pixels per map unit
    glm::vec2 flatCentre() const; // Map point at the screen centre
    // Map to clip space for the copy of the map around the centre; x is not wrapped, the
    // renderer draws the copies either side by shifting whole world widths
    glm::mat4 flatViewProj(const glm::vec2& viewport) const;

    // Globe view
    float globeDistance(const glm::vec2& viewport) const; // Eye distance from the planet centre
    glm::vec3 globeEye(const glm::vec2& viewport) const;
    glm::mat4 globeViewProj(const glm::vec2& viewport) const;

    // Point on the unit sphere under the pixel (y down from the top), false if it is space
    bool globePick(const glm::vec2& pixel, const glm::vec2& viewport, glm::vec3& hit) const;

    // Drag the surface along with the cursor
    void drag(const glm::vec2& deltaPixels);

    // Multiply the zoom and keep the surface point under the pixel where it is
    void zoomAt(float factor, const glm::vec2& pixel, const glm::vec2& viewport, float maxZoom);
};
