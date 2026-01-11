#pragma once
#include <future>
#include <limits>

// Remember to add PopColormap() after the plot
void pushTempColormap(const char* plot_id, const char* colormap_name, ImU32* colors, int size, bool colorsHaveChanged)
{
    if (colorsHaveChanged)
    {
        ImPlot::RemoveColormap(colormap_name);
    }
    if (ImPlot::GetColormapIndex(colormap_name) == -1)
    {
        ImPlotColormap colormap = ImPlot::AddColormap(colormap_name, colors, size, true);
    }
    ImPlot::PushColormap(colormap_name);
    if (colorsHaveChanged)
    {
        ImPlot::BustColorCache(plot_id);
    }
}
// Remember to add PopColormap() after the plot
void pushTempColormap(const char* plot_id, const char* colormap_name, ImVec4* colors, int size, bool colorsHaveChanged)
{
    ImU32* colorsU32 = new ImU32[size];
    for (int i = 0; i < size; i++)
    {
        colorsU32[i] = ImColor(colors[i]);
    }
    if (colorsHaveChanged)
    {
        ImPlot::RemoveColormap(colormap_name);
    }
    if (ImPlot::GetColormapIndex(colormap_name) == -1)
    {
        ImPlotColormap colormap = ImPlot::AddColormap(colormap_name, colors, size, true);
    }
    ImPlot::PushColormap(colormap_name);
    if (colorsHaveChanged)
    {
        ImPlot::BustColorCache(plot_id);
    }
}

static sf::VertexArray drawHighlightCell(vor::Voronoi& map, std::size_t cell)
{
    sf::VertexArray highlight(sf::LinesStrip, map.cells[cell].vertex.size() + 1);
    for (size_t i = 0; i < map.cells[cell].vertex.size(); i++) {
        highlight[i].position = sf::Vector2f(map.voronoi_points[map.cells[cell].vertex[i]].x, map.voronoi_points[map.cells[cell].vertex[i]].y);
        highlight[i].color = sf::Color::Red;
    }
    highlight[map.cells[cell].vertex.size()].position = sf::Vector2f(map.voronoi_points[map.cells[cell].vertex[0]].x, map.voronoi_points[map.cells[cell].vertex[0]].y);
    highlight[map.cells[cell].vertex.size()].color = sf::Color::Red;
    return highlight;
}


static void drawTempMap(vor::Voronoi& map, VertexMap& vertexMap) {
    for (size_t i = 0; i < map.cells.size(); i++)
    {
        sf::Color color(255, 255 / 2 + clamp(5 * map.cells[i].temp, 255 / 2, -255), 0, 255);

        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = color;
        }
    }
    vertexMap.update(map);
}

// Seasonal temperature map - uses SeasonalCalculator to show temperature at a specific day of year
static void drawSeasonalTempMap(vor::Voronoi& map, VertexMap& vertexMap, 
    const SeasonalCalculator& seasonCalc, float dayOfYear) 
{
    for (size_t i = 0; i < map.cells.size(); i++)
    {
        // Get cell position (approximate from first vertex or use center)
        sf::Vector2f position(0.f, 0.f);
        if (!map.cells[i].vertex.empty()) {
            for (int v : map.cells[i].vertex) {
                position.x += map.voronoi_points[v].x;
                position.y += map.voronoi_points[v].y;
            }
            position.x /= map.cells[i].vertex.size();
            position.y /= map.cells[i].vertex.size();
        }
        
        // Get seasonal temperature
        float seasonalTemp = seasonCalc.getTemperature(map.cells[i], position, dayOfYear);
        
        sf::Color color(255, 255 / 2 + clamp(10 * seasonalTemp, 255 / 2, -255), 0, 255); // Increased multiplier for more visible color change

        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = color;
        }
    }
    vertexMap.update(map);
}

static void drawBiomeMap(vor::Voronoi& map, const GlobalWorldObjects& globals, VertexMap& vertexMap)
{
    for (size_t i = 0; i < map.cells.size(); i++)
    {

        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = globals.biomes[map.cells[i].biome].color;
        }
    }
    vertexMap.update(map);
}

static void drawPercepitationMap(vor::Voronoi& map, VertexMap& vertexMap)
{
    for (size_t i = 0; i < map.cells.size(); i++)
    {
        sf::Color color(0, clamp(255 * map.cells[i].percepitation / 100, 255, 0), 0, 255);

        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = color;
        }
    }
    vertexMap.update(map);
}

// Seasonal precipitation map
static void drawSeasonalPercepitationMap(vor::Voronoi& map, VertexMap& vertexMap, 
    const SeasonalCalculator& seasonCalc, float dayOfYear)
{
    for (size_t i = 0; i < map.cells.size(); i++)
    {
        sf::Vector2f position(0.f, 0.f);
        if (!map.cells[i].vertex.empty()) {
            for (int v : map.cells[i].vertex) {
                position.x += map.voronoi_points[v].x;
                position.y += map.voronoi_points[v].y;
            }
            position.x /= map.cells[i].vertex.size();
            position.y /= map.cells[i].vertex.size();
        }
        
        float seasonalPrecip = seasonCalc.getPercepitationDistribution(map.cells[i], position, dayOfYear).mean;
        
        sf::Color color(0, clamp(255 * seasonalPrecip / 100, 255, 0), 0, 255);

        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = color;
        }
    }
    vertexMap.update(map);
}

static void drawHumidityMap(vor::Voronoi& map, VertexMap& vertexMap)
{
    for (size_t i = 0; i < map.cells.size(); i++)
    {
        sf::Color color(0, 0, clamp(255 * map.cells[i].humidity, 255, 0), 255);
        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = color;
        }
    }
    vertexMap.update(map);
}

// Seasonal humidity map
static void drawSeasonalHumidityMap(vor::Voronoi& map, VertexMap& vertexMap, 
    const SeasonalCalculator& seasonCalc, float dayOfYear)
{
    for (size_t i = 0; i < map.cells.size(); i++)
    {
        sf::Vector2f position(0.f, 0.f);
        if (!map.cells[i].vertex.empty()) {
            for (int v : map.cells[i].vertex) {
                position.x += map.voronoi_points[v].x;
                position.y += map.voronoi_points[v].y;
            }
            position.x /= map.cells[i].vertex.size();
            position.y /= map.cells[i].vertex.size();
        }
        
        float seasonalHumidity = seasonCalc.getHumidityDistribution(map.cells[i], position, dayOfYear).mean;
        
        sf::Color color(0, 0, clamp(255 * seasonalHumidity, 255, 0), 255);
        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = color;
        }
    }
    vertexMap.update(map);
}

static void drawHeightMap(vor::Voronoi& map, VertexMap& vertexMap)
{
    if (true) {
        for (std::size_t i = 0; i < map.cells.size(); i++) {
            sf::Color color(
                (128 * (1 - map.cells[i].oceanBool)), 
                (255 * (1 - map.cells[i].oceanBool)), 
                255 / 3 * (map.cells[i].oceanBool + 1.75), 
                55 + (sf::Uint8)std::abs(std::ceil(200 * map.cells[i].height)));

            for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++) {
                map.vertices[j].color = color;
            }
        }
        vertexMap.update(map);
        return;
    }
    // Find min/max height for normalization (heights are now uncapped)
    float minHeight = std::numeric_limits<float>::max();
    float maxHeight = std::numeric_limits<float>::lowest();
    for (std::size_t i = 0; i < map.cells.size(); i++) {
        if (map.cells[i].height < minHeight) minHeight = map.cells[i].height;
        if (map.cells[i].height > maxHeight) maxHeight = map.cells[i].height;
    }
    float heightRange = maxHeight - minHeight;
    if (heightRange < 0.001f) heightRange = 1.0f; // Prevent division by zero

    for (std::size_t i = 0; i < map.cells.size(); i++) {
        // Normalize height to [0, 1] for display purposes
        float normalizedHeight = (map.cells[i].height - minHeight) / heightRange;
        sf::Uint8 alpha = static_cast<sf::Uint8>(55 + 200 * normalizedHeight);
        sf::Color color(
            static_cast<sf::Uint8>(128 * (1 - map.cells[i].oceanBool)),
            static_cast<sf::Uint8>(255 * (1 - map.cells[i].oceanBool)),
            static_cast<sf::Uint8>(255 / 3 * (map.cells[i].oceanBool + 1.75)),
            alpha
        );
        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++) {
            map.vertices[j].color = color;
        }
    }
    vertexMap.update(map);
}

static void drawContinentMap(vor::Voronoi& map, const GlobalWorldObjects& globals, VertexMap& vertexMap)
{
    // Get random color per continent
    ColorTable colorTable;

    std::vector<sf::Color> continentColors = colorTable.getRandomColors(globals.continents.size());

    for (size_t i = 0; i < map.cells.size(); i++)
    {
		// Get the continent id of the cell
		int continentId = map.cells[i].continent;

		// Get the color for the continent
		sf::Color color = continentColors[continentId];

		// Set the color for the vertices of the cell
		for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
		{
			map.vertices[j].color = color;
		}
    }
	vertexMap.update(map);
}

static void drawCulturesMap(vor::Voronoi& map, GlobalWorldObjects& globals, VertexMap& vertexMap)
{
    
	vertexMap.update(map);
}

static void drawResourceMap(vor::Voronoi& map, VertexMap& vertexMap, ResourceType resourceType)
{
    // Find the maximum resource amount for normalization
    float maxAmount = 0.f;
    for (const auto& cell : map.cells) {
        float amount = cell.resources.getResourceAmount(resourceType);
        if (amount > maxAmount) {
            maxAmount = amount;
        }
    }

    // If no resources found, show grey map
    if (maxAmount < 0.001f) {
        for (size_t i = 0; i < map.cells.size(); i++) {
            sf::Color color(80, 80, 80, 255);
            for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++) {
                map.vertices[j].color = color;
            }
        }
        vertexMap.update(map);
        return;
    }

    // Color cells based on resource abundance
    for (size_t i = 0; i < map.cells.size(); i++)
    {
        float amount = map.cells[i].resources.getResourceAmount(resourceType);
        float normalizedAmount = amount / maxAmount;

        sf::Color color;
        if (normalizedAmount < 0.001f) {
            // No resource - grey
            color = sf::Color(80, 80, 80, 255);
        }
        else {
            // Yellow to red gradient based on abundance
            // Yellow (255, 255, 0) -> Orange (255, 128, 0) -> Red (255, 0, 0)
            sf::Uint8 red = 255;
            sf::Uint8 green = static_cast<sf::Uint8>(255 * (1.0f - normalizedAmount));
            color = sf::Color(red, green, 0, 255);
        }

        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = color;
        }
    }
    vertexMap.update(map);
}

static void drawRivers(GlobalWorldObjects& globals, sf::RenderWindow& window)
{
    for (size_t i = 0; i < globals.rivers.size(); i++)
    {
        sf::VertexArray river = globals.rivers[i].drawRiver();
        window.draw(river);
    }
}

static void drawLakes(GlobalWorldObjects& globals, sf::RenderWindow& window)
{
    for (std::size_t i = 0; i < globals.lakes.size(); i++)
    {
        /// TODO: Fix the lake drawing
        //sf::VertexArray lake = globals.lakes[i].drawLake();
        //window.draw(lake);
    }
}

static void drawContinents(GlobalWorldObjects& globals, sf::RenderWindow& window)
{
    for (std::size_t i = 0; i < globals.continents.size(); i++)
    {
        sf::VertexArray continent = globals.continents[i].drawBoundryLine();
        window.draw(continent);
    }
}


void eventloop(sf::RenderWindow& window,
    sf::Event& event,
    int& mapType,
    bool& moving,
    sf::Vector2f& oldPos,
    bool& drawHighlightBool,
    sf::VertexArray& highlight,
    vor::Voronoi& map,
    std::size_t& highlightedCell,
    sf::View& view,
    float& globalZoom,
    unsigned int windowWidth,
    unsigned int windowHeight,
    const bool showLoadConfig,
    const bool showSaveConfig,
	sf::VertexArray& windArrows,
	bool& drawWindArrowsBool
    )
{
    if (showLoadConfig || showSaveConfig) { return; }

	static sf::Vector2f lastViewCenter = view.getCenter();
	static float lastViewUpdateDistance = 0.f;
	const float VIEW_UPDATE_THRESHOLD = 50.f * globalZoom;

    switch (event.type) {
    case sf::Event::Closed:
    {
        window.close();
        break;
    }

    // Mouse buttons
    case sf::Event::MouseButtonPressed:
        // Mouse button is pressed, get the position and set moving as active
        if (event.mouseButton.button == 0) {
            moving = true;
            oldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
			lastViewCenter = view.getCenter();
        }
        if (event.mouseButton.button == 1) {
            // This will need to be a values changer.
        }
        break;

    case sf::Event::MouseButtonReleased:

        // Mouse button is released, no longer move
        if (event.mouseButton.button == 0) {
            moving = false;

			// Update wind arrows if view has moved significantly
            if (drawWindArrowsBool) {
                sf::Vector2f currentViewCenter = view.getCenter();
                float distanceMoved = std::sqrt(std::pow(currentViewCenter.x - lastViewCenter.x, 2) + std::pow(currentViewCenter.y - lastViewCenter.y, 2));
                // If moved more than threshold, update wind arrows
                if (distanceMoved >= VIEW_UPDATE_THRESHOLD) {
                    windArrows = generateWindArrows(map, globalZoom, view, window.getSize());
                    lastViewCenter = currentViewCenter;
                }
            }
        }
        break;

    case sf::Event::KeyPressed:

        // Close Program
        if (event.key.code == sf::Keyboard::Escape) { window.close(); break; }

        // Temperature map
        else if (event.key.code == sf::Keyboard::T) {
            if (mapType == 1) { mapType = 0; }
            else { mapType = 1; }
        }

        // Biome Map
        else if (event.key.code == sf::Keyboard::B) {
            if (mapType == 2) { mapType = 0; }
            else { mapType = 2; }
        }

        // Percepitation map
        else if (event.key.code == sf::Keyboard::P) {
            if (mapType == 3) { mapType = 0; }
            else { mapType = 3; }
        }


        // Activate Cell Highlighting
        else if (event.key.code == sf::Keyboard::H) { drawHighlightBool = !drawHighlightBool; highlight.clear(); }

        break;

        // Mouse Movement
    case sf::Event::MouseMoved:
    {
        // If no button is down, we are not moving the view
        if (!moving) {
            // Highlight Cells if active 
            if (drawHighlightBool)
            {
                // Get the position of the mouse in window coordinates
                const sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
                // Convert the position to world coordinates
                const sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos);
                // Find the cell that contains the mouse
                const std::size_t cellIndex = map.getCellIndex(worldPos);
                // If the mouse is over a cell, highlight it
                if (cellIndex != vor::INVALID_INDEX)
                {
                    // Highlight the cell
                    highlightedCell = cellIndex;
                    highlight = drawHighlightCell(map, highlightedCell);
                }
                else
                {
                    highlightedCell = vor::INVALID_INDEX;
                    highlight.clear();
                }
            }
            break;
        }

        // Determine the new position in world coordinates
        const sf::Vector2f newPos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
        // Determine how the cursor has moved
        // Swap these to invert the movement direction
        const sf::Vector2f deltaPos = oldPos - newPos;

        sf::Vector2f view_center = view.getCenter() + deltaPos;

        // Make sure that the view is contained inside the map
        if ((view.getCenter().x + deltaPos.x) + globalZoom * windowWidth / 2 > windowWidth || (view.getCenter().x + deltaPos.x) - globalZoom * windowWidth / 2 < 0) {
            view_center.x = view.getCenter().x;
        }
        if ((view.getCenter().y + deltaPos.y) + globalZoom * windowHeight / 2 > windowHeight || (view.getCenter().y + deltaPos.y) - globalZoom * windowHeight / 2 < 0) {
            view_center.y = view.getCenter().y;
        }

        view.setCenter(view_center);
        window.setView(view);

        if (drawWindArrowsBool && moving) {
			float distanceMoved = std::sqrt(std::pow(deltaPos.x, 2) + std::pow(deltaPos.y, 2));
			lastViewUpdateDistance += distanceMoved;
			// Update wind arrows if moved more than threshold
            if (lastViewUpdateDistance >= VIEW_UPDATE_THRESHOLD) {
                windArrows = generateWindArrows(map, globalZoom, view, { windowWidth,windowHeight });
                lastViewUpdateDistance = 0.f;
                lastViewCenter = view.getCenter();
            }
        }

        // Save the new position as the old one
        oldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
        break;
    }

    // Zoom
    case sf::Event::MouseWheelScrolled: {
        // Ignore the mouse wheel unless we're not moving
        if (moving) { break; }

		float oldZoom = globalZoom;

        if (event.mouseWheelScroll.delta <= -1)
        {
            globalZoom = std::min(1.f, globalZoom + .07f);
        }
        else if (event.mouseWheelScroll.delta >= 1)
        {
            globalZoom = std::max(.1f, globalZoom - .07f);
        }

        // Update our view
        view.setSize(window.getDefaultView().getSize());
        view.zoom(globalZoom);

        // Make sure that the view is contained inside the map
        sf::Vector2f view_center = view.getCenter();
        if ((view.getCenter().x) + globalZoom * windowWidth / 2 > windowWidth) {
            view_center.x = windowWidth - globalZoom * windowWidth / 2;
        }
        if ((view.getCenter().x) - globalZoom * windowWidth / 2 < 0) {
            view_center.x = globalZoom * windowWidth / 2;
        }
        if ((view.getCenter().y) + globalZoom * windowHeight / 2 > windowHeight) {
            view_center.y = windowHeight - globalZoom * windowHeight / 2;
        }
        if (view.getCenter().y - globalZoom * windowHeight / 2 < 0) {
            view_center.y = globalZoom * windowHeight / 2;
        }
        view.setCenter(view_center);

        if (drawWindArrowsBool && std::abs(oldZoom - globalZoom) > 0.03f) {
            // Update wind arrows
			windArrows = generateWindArrows(map, globalZoom, view, { windowWidth,windowHeight });
			lastViewCenter = view.getCenter();
        }

        window.setView(view);
        break;
    }
    }
}


// Encapsulate all biome-related UI state in a struct
struct BiomeUI {
    // Async state
    std::future<void> biomeGenFuture;

    // Window state
    struct WindowState {
        bool needsReset = true;
        float lastUpdateTime = 0.0f;
        ImVec2 initialSize{ 300, 400 };  // Fixed typo from 4000
    } windowState;

    // Generation function with proper captures
    void generateBiomesAsync(vor::Voronoi& map,
        GlobalWorldObjects& globals,
        MapConfig& config) {
        biomeGenFuture = std::async(std::launch::async,
            [this, &map, &globals, &config] {  // Explicit captures
                // Biome generation logic
                globals.biomes.clear();
                auto biomeColors = randomColors(config.n_biomes);

                for (int i = 0; i < config.n_biomes; i++) {
                    globals.addBiome("Biome" + std::to_string(i), biomeColors[i]);
                }

                calcBiome(map.cells, globals,
                    config.kmeans_max_iter,
                    config.biome_method,
                    config.prob_smoothing);
            });
    }

    void biomePopUp(vor::Voronoi& map, 
        GlobalWorldObjects& globals, 
        MapConfig& config, 
        bool& showBiomeGenBool, 
        int& mapType,
        bool& changeBiomeColorBool)
    {
		if (!showBiomeGenBool) {
			return;
		}
        if (windowState.needsReset) {
            ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
            windowState.needsReset = false;
        }

        // Only update complex elements every 0.5 seconds
        //if (ImGui::GetTime() - biomeState.lastUpdateTime > 0.5f) {
        //    UpdateBiomePreview(); // Heavy visualization code
        //    biomeState.lastUpdateTime = ImGui::GetTime();
        //}

        ImGui::Begin("Biome Generation Controls");

        if (ImGui::Button("Generate Biomes", { 200,50 })) {
            // Ensure at least 1 biome exists before generation
            config.n_biomes = (config.n_biomes < 1) ? 1 : config.n_biomes;
            generateBiomesAsync(map, globals, config);
            // Update the biome colors
			changeBiomeColorBool = true;
            windowState.lastUpdateTime = ImGui::GetTime();
        }
        // Show loading indicator
        if (biomeGenFuture.valid() &&
            biomeGenFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
            ImGui::Text("Generating... %c", "|/-\\"[(int)(ImGui::GetTime() / 0.05f) & 3]);
            // ensure the biome map is not drawn while generating
			if (mapType == 2) {
                ImGui::Text("Wait a moment before viewing the biome map!");
				mapType = 0;
            }
            }
		else {
			// Show completion message
			ImGui::Text("Biomes Generated!");
		}

        ImGui::PushItemWidth(150.f);

        // KMeans iterations with minimum value enforcement
        ImGui::InputUInt("KMeans Max Iterations", &config.kmeans_max_iter);
        if (config.kmeans_max_iter < 10) {
            config.kmeans_max_iter = 10;
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Min 10");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Amount of maximum iterations for clustering (Minimum: 10)");
            ImGui::EndTooltip();
        }

        // Probability smoothing slider
        ImGui::SliderFloat("Probability Smoothing", &config.prob_smoothing, 0.0f, 5.0f, "%.1f");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Factor of weight neighbors have on biome probability");
            ImGui::EndTooltip();
        }

        // Biome method selection using radio buttons
        ImGui::Separator();
        ImGui::Text("Generation Method:");
        ImGui::RadioButton("1: GMM with Smoothing", (int*)&config.biome_method, 1);
        ImGui::RadioButton("2: K-means Clustering", (int*)&config.biome_method, 2);
        ImGui::RadioButton("3: GMM no Smoothing", (int*)&config.biome_method, 3);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Choose between different biome generation algorithms");
            ImGui::EndTooltip();
        }

        // Biome count input with minimum enforcement
        ImGui::InputUInt("Amount of Biomes", &config.n_biomes);
        if (config.n_biomes < 1) {
            config.n_biomes = 1;
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Min 1");
        }
        else if (config.n_biomes > 99)
        {
			config.n_biomes = 99;
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Max 99");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Increase if generated biomes lack diversity.\n Colors will repeat at 41 biomes.");
            ImGui::EndTooltip();
        }

        ImGui::End();
    }
};

/// This needs to be fixed and thread safe :)
void showNewMap(vor::Voronoi& map,
    GlobalWorldObjects& globals,
    sf::RenderWindow& window,
    VertexMap& vertexMap,
    sf::VertexArray& windArrows,
    sf::VertexArray& lines,
	SeasonalCalculator& seasonalCalc,
    const unsigned int windowWidth,
    const unsigned int windowHeight,
    const sf::Font& font,
    MapConfig& config,
    unsigned int& seed,
    PopManager& popManager,
    bool& showNewMapBool)
{
	if (!showNewMapBool) {
		return;
	}
    ImGui::Begin("New Map Controls");
    // push a color on the generate new map button

    if (ImGui::Button("Generate New Map", { 200,50 })) {
        genWorld(map, globals, window, vertexMap,
            lines, seasonalCalc,
            windowWidth, windowHeight,
            font, config, seed, popManager);
        showNewMapBool = false;
    }

    ImGui::Text("The map will have around %.d cells", config.ncellx * config.ncelly);

    ImGui::PushItemWidth(150.f);

    ImGui::InputUInt("Cells x", &config.ncellx);
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Number of cells in the x direction. \nKeep at least above 100 or it starts to break down.");
        ImGui::EndTooltip();
    }

    ImGui::InputUInt("Cells y", &config.ncelly);
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Number of cells in the y direction. \nKeep at least above 100 or it starts to break down.");
        ImGui::EndTooltip();
    }

    ImGui::DragFloat("Jitter of the cells", &config.point_jitter, 0.5f, 0.0f, 10.f);
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Amount each cell is jittered from their grid position");
        ImGui::EndTooltip();
    }

    ImGui::DragFloat("Sea Level", &config.sealevel, 0.01f, 0.0f, 1.0f);
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("The height of the sealevel across map, between 0 and 1.");
        ImGui::EndTooltip();
    }

    ImGui::DragFloat("Global Temp Avg", &config.global_temp_avg, 0.5f, -30.0f, 100.0f);
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("This is an initializer for global temperature modifier, \naround 30-50 will give realistic values (to earth) depending on the size of the map.");
        ImGui::EndTooltip();
    }

    ImGui::InputUInt("Number of Peaks", &config.npeaks);
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Number of peaks to generate on the map.");
        ImGui::EndTooltip();
    }


    ImGui::InputUInt("Amount of Biomes", &config.n_biomes);
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Amount of biomes to generate initially. \nLarge maps will often have a lot of ocean biomes.");
        ImGui::EndTooltip();
    }

    ImGui::InputUInt("Height Smooths", &config.height_smooth_repeats);
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Amount of times the heihgt map is smoothed. \nShould be above 2 ideally more. \nThis is relatively intensive.");
        ImGui::EndTooltip();
    }

    ImGui::InputUInt("Height Noisers", &config.height_noise_repeats);
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("This is the amount of times the height values are renoised after the smoothing, \nthis gives a more realistic height map. \nShould not be used more times than smoothing.");
        ImGui::EndTooltip();
    }

    ImGui::InputUInt("Temp Smooths", &config.temp_smooth_repeats);
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("This is the amount of times temperature is smoothed out.");
        ImGui::EndTooltip();
    }

    ImGui::InputUInt("Percepitations", &config.percepitation_repeats);
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("This is the amount of times percepitation is calculated. \nShould not be more than 1 unless you want high contrast.");
        ImGui::EndTooltip();
    }
    
    //ImGui::Checkbox("Advanced Settings", &advancedSettings);

    if (ImGui::CollapsingHeader("Advanced Settings")) {
        ImGui::InputUInt("Seed", &seed);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Seed for the random number generator. \nKeep at 0 for random seed.");
            ImGui::EndTooltip();
        }
        ImGui::DragFloat("Delta Max Neg", &config.delta_max_neg, 0.01f, 0.0f, 1.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("The maximum amount of random height added in the negative direction. \nIncreasing will make the map more steepely falling");
            ImGui::EndTooltip();
        }
        ImGui::DragFloat("Delta Max Pos", &config.delta_max_pos, 0.01f, 0.0f, 1.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("The maximum amount of random height added in the positive direction. \nIncreasing will make the map more steepely falling");
            ImGui::EndTooltip();
        }
        ImGui::DragFloat("Islands", &config.prob_of_island, 0.01f, 0.0f, 1.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("small probability of random height increase when away from mainland. \nIncrease to make more islands at a short distance from shore.");
            ImGui::EndTooltip();
        }
        ImGui::DragFloat("Distance from land", &config.dist_from_mainland, 0.04f, 0.0f, 5.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("The distance from the mainland where the probability of random height increase begins,\n Represented by the sum of height of all neighbors");
            ImGui::EndTooltip();
        }
        ImGui::DragFloat("Rise Threshold", &config.rise_threshold, 0.01f, 0.0f, 1.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("The 'rise' that is the maximal allowed under height smoothening.");
            ImGui::EndTooltip();
        }
        ImGui::DragFloat("Coast Line", &config.delta_coast_line, 0.01f, 0.0f, 1.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("The amount above or below sealine that defines a coast.");
            ImGui::EndTooltip();
        }
        ImGui::InputUInt("Method of Biomes", &config.biome_method); //TODO, fix this input
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Methods of biome generation. 1: GMM and probability smoothing, 2: KMeans (should be faster, is not)");
            ImGui::EndTooltip();
        }
        ImGui::InputInt("Method of Height", &config.height_method); //TODO, fix this input
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Methods of height generation. 1: k-peaks, 2: k-peaks with continents, 3: K-Continents");
            ImGui::EndTooltip();
        }
        ImGui::InputInt("Method of Height noise", &config.heigth_noise_method); //TODO, fix this input
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Methods of height noising. 1: Uniform random, 2: Simplex noise");
            ImGui::EndTooltip();
        }
    }

    ImGui::PopItemWidth();

    ImGui::End();
}

void resourceMapController(vor::Voronoi& map,
    GlobalWorldObjects& globals,
    VertexMap& vertexMap,
    MapConfig& config,
    bool& showResourceMapBool,
    int& mapType,
    ResourceType& selectedResource)
{
    if (!showResourceMapBool) {
        return;
    }

    ImGui::Begin("Resource Map Controls", &showResourceMapBool);
    ImGui::Text("Select a resource to display on the map:");
    ImGui::Separator();

    static ResourceType previousResource = selectedResource;

    // Metals section
    if (ImGui::CollapsingHeader("Metals", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::RadioButton("Copper", selectedResource == ResourceType::Copper)) {
            selectedResource = ResourceType::Copper;
        }
        if (ImGui::RadioButton("Iron", selectedResource == ResourceType::Iron)) {
            selectedResource = ResourceType::Iron;
        }
        if (ImGui::RadioButton("Tin", selectedResource == ResourceType::Tin)) {
            selectedResource = ResourceType::Tin;
        }
        if (ImGui::RadioButton("Gold", selectedResource == ResourceType::Gold)) {
            selectedResource = ResourceType::Gold;
        }
        if (ImGui::RadioButton("Silver", selectedResource == ResourceType::Silver)) {
            selectedResource = ResourceType::Silver;
        }
        if (ImGui::RadioButton("Lead", selectedResource == ResourceType::Lead)) {
            selectedResource = ResourceType::Lead;
        }
    }

    // Materials section
    if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::RadioButton("Lumber", selectedResource == ResourceType::Lumber)) {
            selectedResource = ResourceType::Lumber;
        }
        if (ImGui::RadioButton("Stone", selectedResource == ResourceType::Stone)) {
            selectedResource = ResourceType::Stone;
        }
        if (ImGui::RadioButton("Clay", selectedResource == ResourceType::Clay)) {
            selectedResource = ResourceType::Clay;
        }
        if (ImGui::RadioButton("Coal", selectedResource == ResourceType::Coal)) {
            selectedResource = ResourceType::Coal;
        }
    }

    // Agriculture section
    if (ImGui::CollapsingHeader("Agriculture", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::RadioButton("Grain", selectedResource == ResourceType::Grain)) {
            selectedResource = ResourceType::Grain;
        }
        if (ImGui::RadioButton("Fruit", selectedResource == ResourceType::Fruit)) {
            selectedResource = ResourceType::Fruit;
        }
        if (ImGui::RadioButton("Vegetables", selectedResource == ResourceType::Vegetables)) {
            selectedResource = ResourceType::Vegetables;
        }
        if (ImGui::RadioButton("Cotton", selectedResource == ResourceType::Cotton)) {
            selectedResource = ResourceType::Cotton;
        }
    }

    // Livestock section
    if (ImGui::CollapsingHeader("Livestock & Animal Products", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::RadioButton("Livestock", selectedResource == ResourceType::Livestock)) {
            selectedResource = ResourceType::Livestock;
        }
        if (ImGui::RadioButton("Sheep", selectedResource == ResourceType::Sheep)) {
            selectedResource = ResourceType::Sheep;
        }
        if (ImGui::RadioButton("Furs", selectedResource == ResourceType::Furs)) {
            selectedResource = ResourceType::Furs;
        }
        if (ImGui::RadioButton("Fish", selectedResource == ResourceType::Fish)) {
            selectedResource = ResourceType::Fish;
        }
        if (ImGui::RadioButton("Whales", selectedResource == ResourceType::Whales)) {
            selectedResource = ResourceType::Whales;
        }
    }

    // Luxury section
    if (ImGui::CollapsingHeader("Luxury Goods", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::RadioButton("Spices", selectedResource == ResourceType::Spices)) {
            selectedResource = ResourceType::Spices;
        }
        if (ImGui::RadioButton("Gems", selectedResource == ResourceType::Gems)) {
            selectedResource = ResourceType::Gems;
        }
        if (ImGui::RadioButton("Dyes", selectedResource == ResourceType::Dyes)) {
            selectedResource = ResourceType::Dyes;
        }
    }

    ImGui::Separator();
    ImGui::Text("Currently displaying: %s", resourceTypeToString(selectedResource).c_str());

    // Color legend
    ImGui::Separator();
    ImGui::Text("Legend:");
    ImGui::ColorButton("##grey", ImVec4(0.31f, 0.31f, 0.31f, 1.0f), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip);
    ImGui::SameLine();
    ImGui::Text("No resource");

    ImGui::ColorButton("##yellow", ImVec4(1.0f, 1.0f, 0.0f, 1.0f), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip);
    ImGui::SameLine();
    ImGui::Text("Low abundance");

    ImGui::ColorButton("##orange", ImVec4(1.0f, 0.5f, 0.0f, 1.0f), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip);
    ImGui::SameLine();
    ImGui::Text("Medium abundance");

    ImGui::ColorButton("##red", ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip);
    ImGui::SameLine();
    ImGui::Text("High abundance");

    // Update map when selection changes
    if (previousResource != selectedResource) {
        drawResourceMap(map, vertexMap, selectedResource);
        previousResource = selectedResource;
        mapType = 7; // Set to resource map mode
    }

    ImGui::End();
}

void planetaryParamsViewer(GlobalWorldObjects& globals, bool& showPlanetaryParamsBool)
{
    if (!showPlanetaryParamsBool)
    {
        return;
	}
    ImGui::Begin("Planetary Parameters", &showPlanetaryParamsBool);
	const auto& params = globals.planetaryParams;

	// Basic Parameters
    if (ImGui::CollapsingHeader("Basic Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginTable("BasicParams", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("Property");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Rotation Speed");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2fx Earth", params.rotationSpeed);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Earth = 1.0, affects Coriolis strength and circulation cells");
            ImGui::EndTooltip();
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Rotation Period");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.1f hours", 24.0f / params.rotationSpeed);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Rotation Direction");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", params.progradeRotation ? "Prograde" : "Retrograde");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Prograde = same direction as Earth (counterclockwise from north pole)");
            ImGui::EndTooltip();
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Axial Tilt");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.1f", params.axialTilt);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Earth = 23.5, affects seasonal variation");
            ImGui::EndTooltip();
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Equator-Pole Temp Diff");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.1fC", params.equatorToPoleTemp);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Earth = 20C, Mars = 40C");
            ImGui::EndTooltip();
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Greenhouse Effect Factor");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2fx", params.greenhouseEffectFactor);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Atmosphere Height");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.1f km", params.atmosphereHeight);

        ImGui::EndTable();
    }
	// Atmospheric Parameters
    if (ImGui::CollapsingHeader("Atmospheric Composition", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Prepare data for pie chart - only include gases with >0.01% for visibility
        std::vector<const char*> gasLabels;
        std::vector<float> gasData;
        std::vector<ImU32> gasColors;
        
        // Add gases if they have significant percentage
        if (params.atmosphere.nitrogenPercentage > 0.01f) {
            gasLabels.push_back("N2");
            gasData.push_back(params.atmosphere.nitrogenPercentage);
            gasColors.push_back(ImColor(135, 206, 235)); // Sky blue
        }
        if (params.atmosphere.oxygenPercentage > 0.01f) {
            gasLabels.push_back("O2");
            gasData.push_back(params.atmosphere.oxygenPercentage);
            gasColors.push_back(ImColor(100, 255, 100)); // Light green
        }
        if (params.atmosphere.carbonDioxidePercentage > 0.01f) {
            gasLabels.push_back("CO2");
            gasData.push_back(params.atmosphere.carbonDioxidePercentage);
            gasColors.push_back(ImColor(210, 180, 140)); // Tan
        }
        if (params.atmosphere.methanePercentage > 0.01f) {
            gasLabels.push_back("CH4");
            gasData.push_back(params.atmosphere.methanePercentage);
            gasColors.push_back(ImColor(80, 200, 200)); // Cyan
        }
        if (params.atmosphere.sulfurDioxidePercentage > 0.01f) {
            gasLabels.push_back("SO2");
            gasData.push_back(params.atmosphere.sulfurDioxidePercentage);
            gasColors.push_back(ImColor(255, 230, 120)); // Pale yellow
        }
        if (params.atmosphere.ammoniaPercentage > 0.01f) {
            gasLabels.push_back("NH3");
            gasData.push_back(params.atmosphere.ammoniaPercentage);
            gasColors.push_back(ImColor(240, 240, 220)); // Off-white
        }
        if (params.atmosphere.waterVaporPercentage > 0.01f) {
            gasLabels.push_back("H2O");
            gasData.push_back(params.atmosphere.waterVaporPercentage);
            gasColors.push_back(ImColor(220, 220, 220)); // White
        }
        if (params.atmosphere.nitrogenDioxidePercentage > 0.01f) {
            gasLabels.push_back("NO2");
            gasData.push_back(params.atmosphere.nitrogenDioxidePercentage);
            gasColors.push_back(ImColor(180, 100, 60)); // Reddish-brown
        }
        if (params.atmosphere.otherGasesPercentage > 0.01f) {
            gasLabels.push_back("Other");
            gasData.push_back(params.atmosphere.otherGasesPercentage);
            gasColors.push_back(ImColor(150, 150, 150)); // Gray
        }

        // Draw pie chart
        if (!gasLabels.empty()) {
            pushTempColormap("AtmosphereComposition", "AtmosphereColormap", gasColors.data(), gasColors.size(), false);

            if (ImPlot::BeginPlot("Gas Composition", ImVec2(-1, 250), ImPlotFlags_Equal)) {
                ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations);
                ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoDecorations);
                ImPlot::PlotPieChart(gasLabels.data(), gasData.data(), static_cast<int>(gasLabels.size()), 0.5, 0.5, 0.4, "%.2f%%", 90, ImPlotPieChartFlags_Normalize);
                ImPlot::EndPlot();
            }
            ImPlot::PopColormap();
        }

        // Legend with detailed info - show ALL gases
        ImGui::Separator();
        ImGui::Text("Detailed Composition:");
        ImGui::Spacing();

        ImGui::BeginTable("GasComposition", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("Gas");
        ImGui::TableSetupColumn("Percentage");
        ImGui::TableHeadersRow();

        // All gases with their names and colors
        struct GasInfo {
            const char* name;
            float percentage;
            ImU32 color;
        };

        std::vector<GasInfo> allGases = {
            {"Nitrogen (N2)", params.atmosphere.nitrogenPercentage, ImColor(135, 206, 235)},
            {"Oxygen (O2)", params.atmosphere.oxygenPercentage, ImColor(100, 255, 100)},
            {"Carbon Dioxide (CO2)", params.atmosphere.carbonDioxidePercentage, ImColor(210, 180, 140)},
            {"Methane (CH4)", params.atmosphere.methanePercentage, ImColor(80, 200, 200)},
            {"Sulfur Dioxide (SO2)", params.atmosphere.sulfurDioxidePercentage, ImColor(255, 230, 120)},
            {"Ammonia (NH3)", params.atmosphere.ammoniaPercentage, ImColor(240, 240, 220)},
            {"Water Vapor (H2O)", params.atmosphere.waterVaporPercentage, ImColor(220, 220, 220)},
            {"Nitrogen Dioxide (NO2)", params.atmosphere.nitrogenDioxidePercentage, ImColor(180, 100, 60)},
            {"Other Gases", params.atmosphere.otherGasesPercentage, ImColor(150, 150, 150)}
        };

        for (const auto& gas : allGases) {
            if (gas.percentage > 0.001f) { // Only show if percentage > 0.001%
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::ColorButton("##gascolor", ImColor(gas.color), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip);
                ImGui::SameLine();
                ImGui::Text("%s", gas.name);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.3f%%", gas.percentage);
            }
        }

        // Show total pressure
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Total Pressure");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "%.3f atm", params.atmosphere.totalPressure);

        ImGui::EndTable();
    }

    // Skybox Color Visualization
    if (ImGui::CollapsingHeader("Atmospheric Color (Skybox)", ImGuiTreeNodeFlags_DefaultOpen)) {
        static float timeOfDay = 0.5f; // 0.0 = midnight, 0.5 = noon, 1.0 = midnight

        ImGui::Text("Time of Day:");
        ImGui::SliderFloat("##TimeOfDay", &timeOfDay, 0.0f, 1.0f, "%.2f");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("0.0 = Midnight, 0.5 = Noon, 1.0 = Midnight");
            ImGui::EndTooltip();
        }

        // Display time labels
        ImGui::SameLine();
        float hourOfDay = timeOfDay * 24.0f;
        ImGui::Text("(%.1f:%.0f)", std::floor(hourOfDay), std::fmod(hourOfDay * 60.0f, 60.0f));

        // Generate color gradient showing sky color throughout the day
        const int numSamples = 48; // Sample every 30 minutes
        ImGui::Text("Sky Color Throughout Day:");

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImVec2(ImGui::GetContentRegionAvail().x, 40);

        // Draw gradient bar
        for (int i = 0; i < numSamples; i++) {
            float t = (float)i / (numSamples - 1);
            sf::Color skyColor = params.getAtmosphereColor(t);

            ImU32 color = ImColor(skyColor.r, skyColor.g, skyColor.b, skyColor.a);

            ImVec2 p1(canvas_pos.x + (canvas_size.x / numSamples) * i, canvas_pos.y);
            ImVec2 p2(canvas_pos.x + (canvas_size.x / numSamples) * (i + 1), canvas_pos.y + canvas_size.y);

            draw_list->AddRectFilled(p1, p2, color);
        }

        // Draw border
        draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            ImColor(255, 255, 255, 128), 0.0f, 0, 1.0f);

        // Draw current time indicator
        float indicatorX = canvas_pos.x + canvas_size.x * timeOfDay;
        draw_list->AddLine(ImVec2(indicatorX, canvas_pos.y),
            ImVec2(indicatorX, canvas_pos.y + canvas_size.y),
            ImColor(255, 255, 0, 255), 2.0f);

        ImGui::Dummy(canvas_size);

        // Show current sky color
        ImGui::Spacing();
        sf::Color currentSkyColor = params.getAtmosphereColor(timeOfDay);
        ImVec4 currentColor(currentSkyColor.r / 255.0f, currentSkyColor.g / 255.0f,
            currentSkyColor.b / 255.0f, currentSkyColor.a / 255.0f);

        ImGui::Text("Current Sky Color:");
        ImGui::SameLine();
        ImGui::ColorButton("##CurrentSky", currentColor,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoBorder, ImVec2(100, 30));
        ImGui::SameLine();
        ImGui::Text("RGB(%d, %d, %d)", currentSkyColor.r, currentSkyColor.g, currentSkyColor.b);
    }

	// Calculated properties
    if (ImGui::CollapsingHeader("Calculated Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginTable("CalcProps", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("Property");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Coriolis Strength");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f", params.getCoriolisStrenght());
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Affects wind deflection. Higher = more east-west flow");
            ImGui::EndTooltip();
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Atmospheric Circulation");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f", params.getAtmosphericCirculationStrength());
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Strength of convection cells. Depends on temp gradient, pressure, and greenhouse effect");
            ImGui::EndTooltip();
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Circulation Cells");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d (%d per hemisphere)", params.getNumberOfCirculationCells(),
            params.getNumberOfCirculationCells() / 2);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Earth = 6 (Hadley, Ferrel, Polar), Venus = 2");
            ImGui::EndTooltip();
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Convergence Zones");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", globals.convergenceLines.size());

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Wind Zones");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", globals.windDirection.size());

        ImGui::EndTable();
    }
    
    // Wind patterns
    if (ImGui::CollapsingHeader("Wind Patterns by Zone")) {
        ImGui::Text("Convergence lines and wind directions:");
        ImGui::Separator();

        ImGui::BeginTable("WindPatterns", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("Zone");
        ImGui::TableSetupColumn("Latitude");
        ImGui::TableSetupColumn("Direction");
        ImGui::TableSetupColumn("Strength");
        ImGui::TableHeadersRow();

        for (int i = 0; i < globals.windDirection.size(); i++) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            // Determine zone name
            std::string zoneName;
            if (i == 0) {
                zoneName = "North Polar";
            }
            else if (i == globals.windDirection.size() - 1) {
                zoneName = "South Polar";
            }
            else if (std::abs((float)i / globals.windDirection.size() - 0.5f) < 0.15f) {
                zoneName = "Equatorial";
            }
            else {
                zoneName = (i < globals.windDirection.size() / 2) ? "North Mid-Lat" : "South Mid-Lat";
            }
            ImGui::Text("%s", zoneName.c_str());

            ImGui::TableSetColumnIndex(1);
            if (i < globals.convergenceLines.size()) {
                float latPercent = globals.convergenceLines[i] * 100.0f;
                ImGui::Text("%.1f%%", latPercent);
            }
            else {
                ImGui::Text("--");
            }

            ImGui::TableSetColumnIndex(2);
            float dir = globals.windDirection[i];
            std::string dirName;
            if (dir >= 337.5f || dir < 22.5f) dirName = "N";
            else if (dir >= 22.5f && dir < 67.5f) dirName = "NE";
            else if (dir >= 67.5f && dir < 112.5f) dirName = "E";
            else if (dir >= 112.5f && dir < 157.5f) dirName = "SE";
            else if (dir >= 157.5f && dir < 202.5f) dirName = "S";
            else if (dir >= 202.5f && dir < 247.5f) dirName = "SW";
            else if (dir >= 247.5f && dir < 292.5f) dirName = "W";
            else dirName = "NW";
            ImGui::Text("%s (%.0f°)", dirName.c_str(), dir);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.2f", globals.windStrength[i]);
        }

        ImGui::EndTable();
    }

    // Comparison to Known planets
    if (ImGui::CollapsingHeader("Planetary Classification")) {
        ImGui::Text("Classification based on parameters:");
        ImGui::Separator();

        // Calculate similarity scores
        float earthScore = 0.0f;
        float marsScore = 0.0f;

        // Rotation speed similarity
        earthScore += 1.0f - std::abs(params.rotationSpeed - 1.0f);
        marsScore += 1.0f - std::abs(params.rotationSpeed - 0.97f);

        // Pressure similarity (logarithmic scale)
        earthScore += 1.0f - std::abs(std::log10(params.atmosphere.totalPressure) - std::log10(1.0f)) / 3.0f;
        marsScore += 1.0f - std::abs(std::log10(params.atmosphere.totalPressure) - std::log10(0.006f)) / 3.0f;

        // Temperature gradient similarity
        earthScore += 1.0f - std::abs(params.equatorToPoleTemp - 20.0f) / 40.0f;
        marsScore += 1.0f - std::abs(params.equatorToPoleTemp - 40.0f) / 40.0f;

        earthScore /= 3.0f;
        marsScore /= 3.0f;

        ImGui::Text("Similarity to Earth: %.1f%%", earthScore * 100.0f);
        ImGui::ProgressBar(earthScore, ImVec2(-1, 0));

        ImGui::Text("Similarity to Mars: %.1f%%", marsScore * 100.0f);
        ImGui::ProgressBar(marsScore, ImVec2(-1, 0));

        ImGui::Separator();

        // Classification
        ImGui::Text("Classification:");
        if (params.rotationSpeed < 0.3f) {
            ImGui::BulletText("Slow rotator (like Venus)");
        }
        else if (params.rotationSpeed > 1.5f) {
            ImGui::BulletText("Fast rotator (like Jupiter)");
        }
        else {
            ImGui::BulletText("Earth-like rotation");
        }

        if (params.atmosphere.totalPressure < 0.1f) {
            ImGui::BulletText("Thin atmosphere (Mars-like)");
        }
        else if (params.atmosphere.totalPressure > 2.0f) {
            ImGui::BulletText("Dense atmosphere (Venus-like)");
        }
        else {
            ImGui::BulletText("Earth-like atmospheric pressure");
        }

        if (params.equatorToPoleTemp < 15.0f) {
            ImGui::BulletText("Low thermal gradient (uniform temps)");
        }
        else if (params.equatorToPoleTemp > 30.0f) {
            ImGui::BulletText("High thermal gradient (extreme temps)");
        }
        else {
            ImGui::BulletText("Earth-like thermal gradient");
        }
    }

    ImGui::End();
}


void configLoadSave(MapConfig& config, bool& showLoadConfig, bool& showSaveConfig)
{
    if (showLoadConfig)
    {
        ImGuiFD::OpenDialog("Choose Dir", ImGuiFDMode_LoadFile, ".");
        showLoadConfig = false; 
    }
    else if (showSaveConfig)
    {
        ImGuiFD::OpenDialog("Choose Dir", ImGuiFDMode_SaveFile, ".");
		showSaveConfig = false;
    }

    std::string path; // Path to save and load filess
    if (ImGuiFD::BeginDialog("Choose Dir")) {
        if (ImGuiFD::ActionDone()) {
            if (ImGuiFD::SelectionMade()) {
                path = ImGuiFD::GetSelectionPathString(0);
                std::cout << "Selected: " << path << std::endl;
                if (showLoadConfig)
                {
                    config.load_json(path);
                    showLoadConfig = false;
                }
                else if (showSaveConfig)
                {
                    config.save_json(path);
                    showSaveConfig = false;
                }
            }
            if (ImGuiFD::CanceledOperation())
            {
                showSaveConfig = false;
                showLoadConfig = false;
            }
            ImGuiFD::CloseCurrentDialog();
        }
        ImGuiFD::EndDialog();
    }
}

void searchFinder(const vor::Voronoi& map, std::vector<std::size_t>& findingCells, std::size_t& findCell, bool& showFindSearcherBool, const std::size_t maxCellInMap)
{
    if (!showFindSearcherBool)
    {
        return;
    }

    ImGui::Begin("Find Cell");

    unsigned int findCellUInt = findCell;
    ImGui::InputUInt("Cell", &findCellUInt);
    findCell = findCellUInt;
    if (ImGui::Button("Find Cell")) {
        if (findCell < map.cells.size()) {
            // if the cell is found and in the list
            if (std::find(findingCells.begin(), findingCells.end(), findCell) == findingCells.end() && findCell <= maxCellInMap)
            {
                findingCells.push_back(findCell);
            }
        }
    }
    for (int i = 0; i < findingCells.size(); i++)
    {
        ImGui::PushID(i);
        ImGui::Text("Cell %d", findingCells[i]); ImGui::SameLine();
        if (ImGui::Button("Remove")) {
            findingCells.erase(findingCells.begin() + i);
        }
        ImGui::PopID();
    }

    ImGui::End();
}

void biomeCountPieChart(GlobalWorldObjects& globals, bool& colorChange)
{
    // Biome distribution pie chart
    std::vector<const char*> labels;
    std::vector<float> data;
    ImU32 colors[100];

    for (int i = 0; i < globals.biomes.size(); i++) {
        Biome& biome = globals.biomes[i];

        labels.push_back(biome.name.c_str());
        data.push_back(static_cast<float>(biome.numCells));

        float color[4];
        color[0] = biome.color.r / 255.0f;
        color[1] = biome.color.g / 255.0f;
        color[2] = biome.color.b / 255.0f;
        color[3] = 1.0f;
        // Convert color to ImU32
        colors[i] = ImColor(color[0], color[1], color[2], color[3]);
    }

	pushTempColormap("Biome Distribution", "BiomeColormap", colors, globals.biomes.size(), colorChange);
    if (ImPlot::BeginPlot("Biome Distribution", ImVec2(-1, 0), ImPlotFlags_Equal)) {
        if (labels.size() == data.size()) {
            ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations);
            ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoDecorations);
            ImPlot::PlotPieChart(labels.data(), data.data(), static_cast<int>(labels.size()), 0.5, 0.5, 0.4, "%.0f", 90, ImPlotFlags_NoInputs);
        }
        ImPlot::EndPlot();
    }
    ImPlot::PopColormap();
}

void RenderBiomeTable(GlobalWorldObjects& globals, bool& doChange) {
    auto& biomes = globals.biomes;

    static bool showValues = false; // Track the state of the "Read More" button

    // "Read More" button
    if (ImGui::Button(showValues ? "Show Less" : "Show More")) {
        showValues = !showValues; // Toggle the state
    }

    int countColumns = 6 + biomes[0].values.size() * showValues;

    if (ImGui::BeginTable("Biomes", countColumns, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable)) {
        // Set up columns
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 15.f);
        ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 45.f);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Cells");
        ImGui::TableSetupColumn("Vegetation Density");
        ImGui::TableSetupColumn("Animal Density");

        // Add additional columns if showValues is true
        if (showValues) {
            for (const auto& pair : biomes[0].values) {
                ImGui::TableSetupColumn(pair.first.c_str());
            }
        }

        ImGui::TableHeadersRow();

        // Populate table with biome data
        for (auto& biome : biomes) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", biome.id);

            ImGui::TableSetColumnIndex(1);
            float color[4];
            color[0] = biome.color.r / 255.0f;
            color[1] = biome.color.g / 255.0f;
            color[2] = biome.color.b / 255.0f;
            color[3] = 1.0f;
            ImGui::PushID(biome.id);
            ImGui::ColorEdit4("", color, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
            ImGui::PopID();

            if (color[0] != biome.color.r / 255.0f || color[1] != biome.color.g / 255.0f || color[2] != biome.color.b / 255.0f) {
                doChange = true;
                biome.color.r = color[0] * 255;
                biome.color.g = color[1] * 255;
                biome.color.b = color[2] * 255;
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", biome.name.c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%u", biome.numCells);

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%d", biome.vegetationDensity);

            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%d", biome.animalDensity);

            // Add biome values if showValues is true
            if (showValues) {
                int count = 5;
                for (const auto& pair : biome.values) {
                    count++;
                    float intPart;
                    float fractPart = std::modf(pair.second, &intPart);
                    ImGui::TableSetColumnIndex(count);
                    if (fractPart == 0.0) ImGui::Text("%.0f", pair.second);
                    else ImGui::Text("%.2f", pair.second);
                }
            }
        }

        ImGui::EndTable();
    }
}

void biomeObservation(GlobalWorldObjects& globals, bool& doChange)
{
    ImGui::Begin("Biome Showing");

    RenderBiomeTable(globals, doChange);

	biomeCountPieChart(globals, doChange);

    ImGui::End();
}

void highligtedCellObservation(const vor::Voronoi& map, const GlobalWorldObjects& globals, std::size_t highlightedCell,
    const SeasonalCalculator* seasonCalc = nullptr, const PopManager* popManager = nullptr)
{
    if (highlightedCell == vor::INVALID_INDEX) {
        return;
    }
    ImGui::Begin("Highlighted Cell");

    const Cell& cell = map.cells[highlightedCell];

    const Biome& biome = globals.biomes[cell.biome];
    ImVec4 color = ImVec4(biome.color.r / 255.0f, biome.color.g / 255.0f, biome.color.b / 255.0f, 1.0f);

    ImGui::BeginTable("Highlighted Cell", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
    ImGui::TableSetupColumn("Property");
    ImGui::TableSetupColumn("Value");
    ImGui::TableHeadersRow();

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Cell Id");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%d", highlightedCell);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Temperature");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", cell.temp);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Temperature Variance");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", cell.tempVariance);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Precipitation");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", cell.percepitation);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Precipitation Variance");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", cell.percepitationVariance);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Elevation");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", cell.height);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Rise");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", cell.rise);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Distance to Ocean");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%d", cell.distToOcean);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Humidity");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", cell.humidity);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Humidity Variance");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", cell.humidityVariance);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Biome");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%s", biome.name.c_str());
    ImGui::SameLine();
    ImGui::ColorButton("colorHighlightedCell", color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Coast");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%d", cell.coastBool);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Ocean");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%d", cell.oceanBool);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Tree");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%d", cell.treeBool);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("River");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%d", cell.riverBool);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Lake");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%d", cell.lakeBool);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("River Id");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%d", cell.riverId);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Lake Id");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%d", cell.lakeId);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Wind Direction");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", cell.windDir);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Wind Direction Variance");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", cell.windDirVariance);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Wind Strength");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", cell.windStr);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Wind Strength Variance");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", cell.windStrVariance);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Culture");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%d", cell.culture);

    if (!globals.continents.empty())
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Continent Id");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", cell.continent);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Volcano");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", cell.volcanicActivity);
    }

    ImGui::EndTable();
    

    // Seasonal Climate Curve Visualization
    if (seasonCalc != nullptr) {
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Seasonal Climate Curves", ImGuiTreeNodeFlags_DefaultOpen)) {
            static int selectedClimateVar = 0;
            const char* climateVarNames[] = { "Temperature", "Precipitation", "Humidity", "Wind Strength" };
            ImGui::Combo("Climate Variable", &selectedClimateVar, climateVarNames, IM_ARRAYSIZE(climateVarNames));

            // Get cell position
            sf::Vector2f position(0.f, 0.f);
            if (!cell.vertex.empty()) {
                for (int v : cell.vertex) {
                    position.x += map.voronoi_points[v].x;
                    position.y += map.voronoi_points[v].y;
                }
                position.x /= cell.vertex.size();
                position.y /= cell.vertex.size();
            }

            // Generate data for the whole year (12 months)
            float yearLength = globals.planetaryParams.getYearLength();
            const int numSamples = 365;
            static std::vector<float> xData(numSamples);
            static std::vector<float> yMean(numSamples);
            static std::vector<float> yMin(numSamples);
            static std::vector<float> yMax(numSamples);

            for (int i = 0; i < numSamples; i++) {
                float dayOfYear = (float)i;
                xData[i] = dayOfYear;

                ClimateDistribution dist;
                switch (selectedClimateVar) {
                case 0: // Temperature
                    dist = seasonCalc->getTemperatureDistribution(cell, position, dayOfYear);
                    break;
                case 1: // Precipitation
                    dist = seasonCalc->getPercepitationDistribution(cell, position, dayOfYear);
                    break;
                case 2: // Humidity
                    dist = seasonCalc->getHumidityDistribution(cell, position, dayOfYear);
                    break;
                case 3: // Wind Strength
                    dist = seasonCalc->getWindStrengthDistribution(cell, position, dayOfYear);
                    break;
                }
                yMean[i] = dist.mean;
                yMin[i] = dist.getMin();
                yMax[i] = dist.getMax();
            }

            // Plot the seasonal curve
            if (ImPlot::BeginPlot("##SeasonalCurve", ImVec2(-1, 200))) {
                ImPlot::SetupAxes("Day of Year", climateVarNames[selectedClimateVar]);
                ImPlot::SetupAxisLimits(ImAxis_X1, 0, yearLength, ImPlotCond_Always);

                // Plot variance band (min to max)
                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                ImPlot::PlotShaded("Range", xData.data(), yMin.data(), yMax.data(), numSamples);
                ImPlot::PopStyleVar();

                // Plot mean line
                ImPlot::SetNextLineStyle(ImVec4(1, 0.5f, 0, 1), 2.0f);
                ImPlot::PlotLine("Mean", xData.data(), yMean.data(), numSamples);

                // Add season markers
                float springStart = 0.0f;
                float summerStart = yearLength * 0.25f;
                float autumnStart = yearLength * 0.5f;
                float winterStart = yearLength * 0.75f;

                ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
                double springLine = springStart;
                double summerLine = summerStart;
                double autumnLine = autumnStart;
                double winterLine = winterStart;
                ImPlot::PlotInfLines("##seasons", &springLine, 1);
                ImPlot::PlotInfLines("##seasons", &summerLine, 1);
                ImPlot::PlotInfLines("##seasons", &autumnLine, 1);
                ImPlot::PlotInfLines("##seasons", &winterLine, 1);
                ImPlot::PopStyleColor();

                ImPlot::EndPlot();
            }

            // Show current season info
            ImGui::Text("Season labels (Northern Hemisphere):");
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Spring: Day 0-%.0f", yearLength * 0.25f);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Summer: Day %.0f-%.0f", yearLength * 0.25f, yearLength * 0.5f);
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "Autumn: Day %.0f-%.0f", yearLength * 0.5f, yearLength * 0.75f);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f), "Winter: Day %.0f-%.0f", yearLength * 0.75f, yearLength);
        }
    }

    ImGui::Text("Biome Probabilities: ");
    for (int i = 0; i < cell.biome_prob.size(); i++)
    {
        const Biome& biome = globals.biomes[i];
        ImVec4 color = ImVec4(biome.color.r / 255.0f, biome.color.g / 255.0f, biome.color.b / 255.0f, 1.0f);
        ImGui::TextColored(color, "%s: %.2f", biome.name.c_str(), cell.biome_prob[i]);
    }

    ImGui::Text("Resources:");

    auto resources = cell.resources.getAllResources();
    if (resources.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No resources in this cell");
    }
    else {
        ImGui::BeginTable("Resources", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("Resource Type");
        ImGui::TableSetupColumn("Amount");
        ImGui::TableHeadersRow();

        for (const auto& [resType, amount] : resources) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", resourceTypeToString(resType).c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", amount);
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void continentViewer(GlobalWorldObjects& globals, bool& showContinentViewer)
{
    if (!showContinentViewer)
    {
		return;
    }
	ImGui::Begin("Continent Viewer", &showContinentViewer);

    if (globals.continents.empty())
    {
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No continents generated.");
        ImGui::End();
		return;
    }

    // Summary section
    if (ImGui::CollapsingHeader("Summary", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int oceanicCount = 0, continentalCount = 0, mixedCount = 0;
        std::size_t totalCells = 0;
        float avgAge = 0.0f;

        for (const auto& continent : globals.continents)
        {
            switch (continent.plateType)
            {
            case PlateType::Oceanic: oceanicCount++; break;
            case PlateType::Continental: continentalCount++; break;
            case PlateType::Mixed: mixedCount++; break;
            }
            totalCells += continent.cells.size();
            avgAge += continent.age;
        }
        avgAge /= globals.continents.size();

        ImGui::BeginTable("SummaryTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("Property");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Total Continents");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%zu", globals.continents.size());

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Oceanic Plates");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(ImVec4(0.3f, 0.5f, 1.0f, 1.0f), "%d", oceanicCount);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Continental Plates");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(ImVec4(0.6f, 0.4f, 0.2f, 1.0f), "%d", continentalCount);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Mixed Plates");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%d", mixedCount);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Total Cells");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%zu", totalCells);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Average Plate Age");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f Ga", avgAge);

        ImGui::EndTable();
    }
    // Plate Type Distribution
    if (ImGui::CollapsingHeader("Plate Type Distribution", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int oceanicCount = 0, continentalCount = 0, mixedCount = 0;
        for (const auto& continent : globals.continents)
        {
            switch (continent.plateType)
            {
            case PlateType::Oceanic: oceanicCount++; break;
            case PlateType::Continental: continentalCount++; break;
            case PlateType::Mixed: mixedCount++; break;
            }
        }

        const char* labels[] = { "Oceanic", "Continental", "Mixed" };
        float data[] = { static_cast<float>(oceanicCount), static_cast<float>(continentalCount), static_cast<float>(mixedCount) };
        ImU32 colors[] = { ImColor(77, 128, 255), ImColor(153, 102, 51), ImColor(128, 128, 128) };

        pushTempColormap("PlateDistribution", "PlateColormap", colors, 3, false);
        if (ImPlot::BeginPlot("Plate Types", ImVec2(-1, 200), ImPlotFlags_Equal))
        {
            ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations);
            ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoDecorations);
            ImPlot::PlotPieChart(labels, data, 3, 0.5, 0.5, 0.4, "%.0f", 90, ImPlotPieChartFlags_Normalize);
            ImPlot::EndPlot();
        }
        ImPlot::PopColormap();
    }
    // Individual Continent Details
    if (ImGui::CollapsingHeader("Continent Details", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static int selectedContinent = 0;

        // Continent selector
        ImGui::Text("Select Continent:");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##ContinentSelector",
            ("Continent " + std::to_string(selectedContinent)).c_str()))
        {
            for (int i = 0; i < globals.continents.size(); i++)
            {
                bool isSelected = (selectedContinent == i);
                std::string label = "Continent " + std::to_string(i);

                // Add plate type indicator
                switch (globals.continents[i].plateType)
                {
                case PlateType::Oceanic: label += " (Oceanic)"; break;
                case PlateType::Continental: label += " (Continental)"; break;
                case PlateType::Mixed: label += " (Mixed)"; break;
                }

                if (ImGui::Selectable(label.c_str(), isSelected))
                {
                    selectedContinent = i;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        // Clamp selected continent to valid range
        if (selectedContinent >= globals.continents.size())
        {
            selectedContinent = 0;
        }

        const Continent& continent = globals.continents[selectedContinent];

        ImGui::Separator();

        // Plate type with color indicator
        ImGui::Text("Plate Type: ");
        ImGui::SameLine();
        switch (continent.plateType)
        {
        case PlateType::Oceanic:
            ImGui::TextColored(ImVec4(0.3f, 0.5f, 1.0f, 1.0f), "Oceanic");
            break;
        case PlateType::Continental:
            ImGui::TextColored(ImVec4(0.6f, 0.4f, 0.2f, 1.0f), "Continental");
            break;
        case PlateType::Mixed:
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Mixed");
            break;
        }
        ImGui::BeginTable("ContinentDetails", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("Property");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("ID");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", continent.id);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Cell Count");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%zu", continent.cells.size());

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Age");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f Ga (billion years)", continent.age);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Older plates tend to be cooler and denser");
            ImGui::EndTooltip();
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Crust Thickness");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.1f km", continent.crustThickness);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Continental: ~35 km, Oceanic: ~7 km");
            ImGui::EndTooltip();
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Base Density");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f g/cm³", continent.baseDensity);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Center Position");
        ImGui::TableSetColumnIndex(1);
        sf::Vector2f center = globals.continents[selectedContinent].getCenter();
        ImGui::Text("(%.1f, %.1f)", center.x, center.y);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Direction Vector");
        ImGui::TableSetColumnIndex(1);
        sf::Vector2f dir = globals.continents[selectedContinent].getDirection();
        ImGui::Text("(%.2f, %.2f)", dir.x, dir.y);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Plate movement direction for tectonic interactions");
            ImGui::EndTooltip();
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Isostatic Height");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.3f", globals.continents[selectedContinent].getIsostaticHeight());
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Height adjustment based on crustal buoyancy");
            ImGui::EndTooltip();
        }

        ImGui::EndTable();

        // Direction visualization
        ImGui::Separator();
        ImGui::Text("Movement Direction:");

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        float canvas_size = 80.0f;
        ImVec2 center_pos(canvas_pos.x + canvas_size / 2, canvas_pos.y + canvas_size / 2);

        // Draw background circle
        draw_list->AddCircleFilled(center_pos, canvas_size / 2 - 5, ImColor(50, 50, 50, 255));
        draw_list->AddCircle(center_pos, canvas_size / 2 - 5, ImColor(100, 100, 100, 255), 32, 2.0f);

        // Draw direction arrow
        sf::Vector2f direction = globals.continents[selectedContinent].getDirection();
        float mag = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (mag > 0.001f)
        {
            float normX = direction.x / mag;
            float normY = direction.y / mag;
            float arrowLen = (canvas_size / 2 - 10) * std::min(mag, 1.5f);

            ImVec2 arrowEnd(center_pos.x + normX * arrowLen, center_pos.y + normY * arrowLen);

            // Arrow color based on plate type
            ImU32 arrowColor;
            switch (continent.plateType)
            {
            case PlateType::Oceanic: arrowColor = ImColor(77, 128, 255, 255); break;
            case PlateType::Continental: arrowColor = ImColor(153, 102, 51, 255); break;
            case PlateType::Mixed: arrowColor = ImColor(128, 128, 128, 255); break;
            }

            draw_list->AddLine(center_pos, arrowEnd, arrowColor, 3.0f);

            // Arrow head
            float angle = std::atan2(normY, normX);
            float headLen = 10.0f;
            ImVec2 head1(arrowEnd.x - headLen * std::cos(angle - 0.4f),
                arrowEnd.y - headLen * std::sin(angle - 0.4f));
            ImVec2 head2(arrowEnd.x - headLen * std::cos(angle + 0.4f),
                arrowEnd.y - headLen * std::sin(angle + 0.4f));
            draw_list->AddTriangleFilled(arrowEnd, head1, head2, arrowColor);
        }
        else
        {
            draw_list->AddCircleFilled(center_pos, 5, ImColor(200, 200, 200, 255));
        }

        ImGui::Dummy(ImVec2(canvas_size, canvas_size));
    }

    // Continent Comparison Table
    if (ImGui::CollapsingHeader("All Continents Comparison"))
    {
        if (ImGui::BeginTable("AllContinents", 7,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY,
            ImVec2(0, 300)))
        {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Cells", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Age (Ga)", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Thickness", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Density", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Isostatic", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (std::size_t i = 0; i < globals.continents.size(); i++)
            {
                const Continent& c = globals.continents[i];
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", c.id);

                ImGui::TableSetColumnIndex(1);
                switch (c.plateType)
                {
                case PlateType::Oceanic:
                    ImGui::TextColored(ImVec4(0.3f, 0.5f, 1.0f, 1.0f), "Oceanic");
                    break;
                case PlateType::Continental:
                    ImGui::TextColored(ImVec4(0.6f, 0.4f, 0.2f, 1.0f), "Continental");
                    break;
                case PlateType::Mixed:
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Mixed");
                    break;
                }

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%zu", c.cells.size());

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.2f", c.age);

                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%.1f km", c.crustThickness);

                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%.2f", c.baseDensity);

                ImGui::TableSetColumnIndex(6);
                ImGui::Text("%.3f", globals.continents[i].getIsostaticHeight());
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();
}

void populationViewer(PopManager& popManager, GlobalWorldObjects& globals,
    bool& showPopulationViewer, const vor::Voronoi& map)
{
    if (!showPopulationViewer)
    {
        return;
    }

}