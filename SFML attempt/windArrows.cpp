#include "windArrows.hpp"

// Color coding for wind strength
static sf::Color getWindColor(float windStrength) {
    // Weak wind: blue -> Strong wind: red
    float normalized = clamp(windStrength, 1.0f, 0.0f);

    if (normalized < 0.33f) {
        // Blue to cyan
        return sf::Color(0,
            static_cast<sf::Uint8>(255 * (normalized / 0.33f)),
            255, 200);
    }
    else if (normalized < 0.66f) {
        // Cyan to yellow
        float t = (normalized - 0.33f) / 0.33f;
        return sf::Color(static_cast<sf::Uint8>(255 * t),
            255,
            static_cast<sf::Uint8>(255 * (1.0f - t)), 200);
    }
    else {
        // Yellow to red
        float t = (normalized - 0.66f) / 0.34f;
        return sf::Color(255,
            static_cast<sf::Uint8>(255 * (1.0f - t)),
            0, 200);
    }
}

sf::VertexArray windArrowsGrid(vor::Voronoi& map)
{
    sf::VertexArray windArrows(sf::Triangles, 3 * map.grid_cells.m_width * map.grid_cells.m_height);
    float arrowLengthBase = 25.f; // Adjust this value as needed
    float baseAngleOffset = PI / 8;

    for (int x_gridCell = 0; x_gridCell < map.grid_cells.m_width; x_gridCell++)
    {
        for (int y_gridCell = 0; y_gridCell < map.grid_cells.m_height; y_gridCell++)
        {
            // get all the cells inside the gridcell
            std::vector<std::size_t> choice_cells = map.grid_cells(x_gridCell, y_gridCell);

            // get the average wind direction and wind strength for all cells inside the gridcell
            double sumX = 0.0;
            double sumY = 0.0;
            double sumStr = 0.0;
            for (std::size_t i = 0; i < choice_cells.size(); i++) {
                float radian = radians(map.cells[choice_cells[i]].windDir);
                sumX += std::cos(radian);
                sumY += std::sin(radian);

                sumStr += map.cells[choice_cells[i]].windStr;
            }
            double averageRadians = std::atan2(sumY, sumX);
            double averageWindStr = sumStr / choice_cells.size();
            // Get positions for vertices of arrow based on direction and located in the center of the gridcell
            float x_center = x_gridCell * map.cell_size + map.cell_size / 2;
            float y_center = y_gridCell * map.cell_size + map.cell_size / 2;
            sf::Vector2f center = sf::Vector2f(x_center, y_center);
            // Create the arrow as a triangle with the tip pointing in the direction of the wind

            float arrowLength = arrowLengthBase * averageWindStr;

            float x_tip = x_center + arrowLength * std::cos(averageRadians);
            float y_tip = y_center + arrowLength * std::sin(averageRadians);


            float x_base1 = x_center + (arrowLength * 0.7f) * std::cos(averageRadians + PI - baseAngleOffset); // Adding 120 degrees to the angle
            float y_base1 = y_center + (arrowLength * 0.7f) * std::sin(averageRadians + PI - baseAngleOffset);

            float x_base2 = x_center + (arrowLength * 0.7f) * std::cos(averageRadians + PI + baseAngleOffset); // Subtracting 120 degrees from the angle
            float y_base2 = y_center + (arrowLength * 0.7f) * std::sin(averageRadians + PI + baseAngleOffset);

            windArrows.append(sf::Vertex(sf::Vector2f(x_tip, y_tip), sf::Color::Black));
            windArrows.append(sf::Vertex(sf::Vector2f(x_base1, y_base1), sf::Color::Black));
            windArrows.append(sf::Vertex(sf::Vector2f(x_base2, y_base2), sf::Color::Black));
        }
    }
    return windArrows;
}


sf::VertexArray generateWindArrows(vor::Voronoi& map, float globalZoom, const sf::View& view, const sf::Vector2u& windowSize)
{ // create a vertex array drawing arrows by the wind direction
    // 1. Seperate the map and get all the cells inside each gridcell
    // 2. take the average wind direction and wind strength for all cells inside the gridcell
    // 3. Get positions for vertices of arrow based on direction and located in the center of the gridcell
    // 4. Scale the arrow based on strength
    // 5. Draw the arrow
    // 6. Repeat for all gridcells / cellls

    WindArrows::DetailLevel lod = WindArrows::getLODFromZoom(globalZoom);

    if (lod == WindArrows::DetailLevel::Grid) {
        return windArrowsGrid(map);
    }

    // Cell based for closer zoom levels
    int cellSkip = WindArrows::getCellSkip(lod);
    bool drawStreamlines = (lod == WindArrows::DetailLevel::VeryDense);

    // Calculate visible area in world coordinates
    sf::Vector2f viewCenter = view.getCenter();
    sf::Vector2f viewSize = view.getSize();
    sf::FloatRect visibleArea(
        viewCenter.x - viewSize.x / 2.f,
        viewCenter.y - viewSize.y / 2.f,
        viewSize.x,
        viewSize.y
    );

    // Estimate the number of arrows needed
    std::size_t estimatedArrows = map.cells.size() / (cellSkip * cellSkip);
    sf::VertexArray arrows(sf::Triangles, 3 * estimatedArrows);

    // Arrow sizing based on zoom
    float baseArrowLength = clamp(33000.f / map.cells.size(), 1.0f, 0.01f);
    float arrowBaseLength = 15.0f * baseArrowLength * (1.0f + globalZoom * 2.0f);
    float vaseAngleOffset = PI / 8.0f;

    // Draw arrows for individual cells
    for (std::size_t i = 0; i < map.cells.size(); i += cellSkip)
    {
        const Cell& cell = map.cells[i];
        sf::Vector2f cellPos = map.points[cell.id];

        // Frustum culling: Skip cells outside the visible area
        if (!visibleArea.contains(cellPos)) {
            continue;
        }

        // Calculate arrow geometry
        float windRad = radians(cell.windDir);
        float arrowLength = arrowBaseLength * cell.windStr;

        // Arrow color based on wind strength
        sf::Color arrowColor = getWindColor(cell.windStr);

        // Tip of the arrow
        float x_tip = cellPos.x + arrowLength * std::cos(windRad);
        float y_tip = cellPos.y + arrowLength * std::sin(windRad);

        // Base corners of the arrow
        float x_base1 = cellPos.x + (arrowLength * 0.7f) * std::cos(windRad + PI - vaseAngleOffset);
        float y_base1 = cellPos.y + (arrowLength * 0.7f) * std::sin(windRad + PI - vaseAngleOffset);
        float x_base2 = cellPos.x + (arrowLength * 0.7f) * std::cos(windRad + PI + vaseAngleOffset);
        float y_base2 = cellPos.y + (arrowLength * 0.7f) * std::sin(windRad + PI + vaseAngleOffset);
        // Append vertices to the vertex array
        arrows.append(sf::Vertex(sf::Vector2f(x_tip, y_tip), arrowColor));
        arrows.append(sf::Vertex(sf::Vector2f(x_base1, y_base1), arrowColor));
        arrows.append(sf::Vertex(sf::Vector2f(x_base2, y_base2), arrowColor));
    }
	
    return arrows;
}
