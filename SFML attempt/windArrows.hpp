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

static sf::Color getWindColor(float windStrength);
sf::VertexArray windArrowsGrid(vor::Voronoi& map);
sf::VertexArray generateWindArrows(vor::Voronoi& map, float globalZoom, const sf::View& view, const sf::Vector2u& windowSize);
