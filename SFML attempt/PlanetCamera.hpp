#pragma once
#include <glm/glm.hpp>
#include "MapProjection.hpp"

enum class ViewMode { Globe, Flat };
// Auto picks the mode from the zoom, the others keep one mode at every zoom
enum class ViewLock { Auto, Globe, Flat };

// One camera shared by every view of the planet: the point at the screen centre and how many
// pixels one radian of arc covers there. North is always up.
//
// The globe is drawn in perspective with the eye straight above the centre point. Its distance
// follows from the zoom, so zoomed out the globe looks almost orthographic and zoomed in it
// looks like flying low over the surface.
//
// The flat view is the projected map scaled so one radian of north-south arc at the centre is
// zoom pixels, so both views agree at the screen centre and switching between them is seamless.
// Panning the flat view keeps the map scale and recomputes the zoom from it, so on Mercator the
// zoom grows towards the poles while the map itself doesn't change size.
//
// In Auto the wheel switches to the flat view once less than SWITCH_LAT_SPAN_DEG of latitude
// fits the window height, and back once more does. Only the wheel switches (zooming in only to
// flat, zooming out only to the globe), so panning never flips the view.
struct PlanetCamera {
    float lat = 20.f;   // Degrees, kept short of the poles so north-up stays defined
    float lon = 0.f;    // Degrees, (-180, 180]
    float zoom = 500.f; // Pixels per radian at the screen centre
    ViewMode mode = ViewMode::Globe;
    ViewLock lock = ViewLock::Auto;
    Projection projection = Projection::Mercator; // Of the flat view
    // Flat view repeats east-west. Off, it is one map from 180W to 180E: the centre stops at
    // the edges instead of wrapping, like it stops at the top and bottom
    bool wrap = true;

    static constexpr float MAX_LAT = 89.f;
    static constexpr float FOV_Y = 30.f; // Degrees, vertical field of view of the globe
    static constexpr float SWITCH_LAT_SPAN_DEG = 65.f;

    // Zoom where the whole globe fits the viewport with a margin
    static float minZoom(const glm::vec2& viewport);

    // Keeps lat, lon and zoom in range, maxZoom comes from the cell size
    void clampTo(const glm::vec2& viewport, float maxZoom);
    float maxLat() const; // Of the centre in the current mode
    bool flatWraps() const; // Whether the flat view repeats east-west

    // Degrees of latitude the window height covers at the screen centre
    float visibleLatSpan(const glm::vec2& viewport) const;
    // Changes the lock, and the mode to match it (Auto picks by the visible span)
    void setLock(ViewLock newLock, const glm::vec2& viewport, float maxZoom);

    // Flat view, see MapProjection.hpp for map coordinates
    float flatScale() const;      // Pixels per map unit
    void setFlatScale(float scale); // Sets the zoom that gives this scale at the current lat
    // Smallest scale of a flat view locked on: the map's full height fills the window
    float minFlatScale(const glm::vec2& viewport) const;
    glm::vec2 flatCentre() const; // Map point at the screen centre
    void setFlatCentre(const glm::vec2& map); // Moves the centre there and keeps the scale
    glm::vec2 flatPixelToMap(const glm::vec2& pixel, const glm::vec2& viewport) const; // x not wrapped
    // Map to clip space for the copy of the map around the centre; x is not wrapped, the
    // renderer draws the copies either side by shifting whole world widths
    glm::mat4 flatViewProj(const glm::vec2& viewport) const;

    // Globe view
    float globeDistance(const glm::vec2& viewport) const; // Eye distance from the planet centre
    glm::vec3 globeEye(const glm::vec2& viewport) const;
    glm::mat4 globeViewProj(const glm::vec2& viewport) const;

    // Point on the unit sphere under the pixel (y down from the top), false if it is space
    bool globePick(const glm::vec2& pixel, const glm::vec2& viewport, glm::vec3& hit) const;
    // Same for the flat view, false past the ends of the map
    bool flatPick(const glm::vec2& pixel, const glm::vec2& viewport, glm::vec3& hit) const;
    // The pick of the current mode
    bool pick(const glm::vec2& pixel, const glm::vec2& viewport, glm::vec3& hit) const;

    // Drag the surface along with the cursor
    void drag(const glm::vec2& deltaPixels);

    // Multiply the zoom and keep the surface point under the pixel where it is. In Auto this
    // is where the mode switches
    void zoomAt(float factor, const glm::vec2& pixel, const glm::vec2& viewport, float maxZoom);

private:
    float fitLon(float l) const; // Wrapped, or clamped to the edges of a static flat map
    void zoomGlobeAt(float factor, const glm::vec2& pixel, const glm::vec2& viewport, float maxZoom);
    void zoomFlatAt(float factor, const glm::vec2& pixel, const glm::vec2& viewport, float maxZoom);
};
