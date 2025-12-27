#pragma once
#include <SFML/Graphics.hpp>
#include "util.hpp"
#include "Voronoi.hpp"

struct WindArrows {
    enum class DetailLevel {
        Grid,       // One arrow per grid cell
        Sparse,     // Every 10th cell
        Medium,     // Every 5th cell
        Dense,      // Every cell
        VeryDense   // Every cell + streamlines
    };
    static DetailLevel getLODFromZoom(float zoom) {
        if (zoom < 0.3f) return DetailLevel::VeryDense;
        else if (zoom < 0.45f) return DetailLevel::Dense;
        else if (zoom < 0.7f) return DetailLevel::Medium;
        else if (zoom < 0.90f) return DetailLevel::Sparse;
        else return DetailLevel::Grid;
    }
    static int getCellSkip(DetailLevel level) {
        switch (level) {
        case DetailLevel::Grid: return -1; // use the grid
        case DetailLevel::Sparse: return 10;
        case DetailLevel::Medium: return 5;
        case DetailLevel::Dense: return 1;
        case DetailLevel::VeryDense: return 1;
        default: return 5;
        }
    }
};

class WindStreamlines {
public:
    struct StreamlineConfig {
		int numStreamlines = 50; // Number of streamlines to generate
		int maxSteps = 100; // Maximum number of steps per streamline
		float stepSize = 5.f; // Step size for each iteration
		float separationDistance = 20.f; // Minimum distance between streamlines
		float lineWidth = 2.0f; // Width of the streamline
		bool fadeWithStrength = true; // Whether to fade color based on wind strength
    };
    sf::VertexArray generateStreamlines(
        vor::Voronoi& map,
        const sf::View& view,
        const StreamlineConfig& config = StreamlineConfig());
private:
    std::vector<sf::Vector2f> traceStreamline(
        vor::Voronoi& map,
        sf::Vector2f seedPoint,
        const sf::FloatRect& visibleArea,
        const StreamlineConfig& config);

    sf::Vector2f getWindVectorAt(vor::Voronoi& map, sf::Vector2f position);

    void addStreamlineToVertexArray(
        sf::VertexArray& vertexArray,
        const std::vector<sf::Vector2f>& streamline,
        vor::Voronoi& map,
        const StreamlineConfig& config);

    bool isSeparated(
        const sf::Vector2f& point,
        const std::vector<std::vector<sf::Vector2f>>& existingStreamlines,
        float minDistance);
};


static sf::Color getWindColor(float windStrength);
sf::VertexArray windArrowsGrid(vor::Voronoi& map);
sf::VertexArray generateWindArrows(vor::Voronoi& map, float globalZoom, const sf::View& view, const sf::Vector2u& windowSize);
