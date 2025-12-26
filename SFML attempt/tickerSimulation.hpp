#pragma once
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>

float deltaOverExtension(Cell cell)
{
	// Calculate the overExtension cost of the cell

	// rise + river + lake + ocean + biome + snow

	float overExtension = cell.rise + cell.riverBool / 3.f + cell.lakeBool / 3.f + cell.oceanBool / 3.f + cell.biome_prob[cell.biome] + cell.snowBool / 3.f;
	return overExtension;
}

size_t cultureChangeTick(std::vector<Cell>& map, GlobalWorldObjects& globals, int cultureIndex, std::vector<Culture>& newCultures)
{
	// Add a new cell from any of the neighbors in cells
	// 1. Find a valid random cell with neighbors not in cells
	// 2. calculate the overExtension cost of the cell
	// 3. If the cost is too high, break off a child culture
	// 4. If the cost is low, add the cell to the culture
	if (globals.cultures[cultureIndex].cells.size() == 0) return vor::INVALID_INDEX; // If the culture has no cells, return empty vector

	bool found = false;
	size_t foundCells = vor::INVALID_INDEX; // Vector of found cells

	for (std::size_t j = 0; j < globals.cultures[cultureIndex].cells.size(); j++) // Iterate over the cells of the culture
	{

		std::size_t cellId = rand() % globals.cultures[cultureIndex].cells.size(); // Random cell id

		std::size_t cellIndex = globals.cultures[cultureIndex].cells[cellId]; // Get the cell

		if (map[cellIndex].culture == -1 )
		{
			map[cellIndex].culture = globals.cultures[cultureIndex].id; // Set the culture of the cell
		}

		// check neighbors
		for (std::size_t i = 0; i < map[cellIndex].neighbors.size(); i++)
		{
			Cell& neighbor = map[map[cellIndex].neighbors[i]]; // Get the neighbor cell
			if (neighbor.culture == -1) // If the neighbor is not in any culture
			{
				// calculate the overExtension cost of the cell
				float overExtensionCost = deltaOverExtension(neighbor) + globals.cultures[cultureIndex].overExtension[cellId]; // Calculate the overExtension cost of the cell

				if (overExtensionCost > 20.f && !found)
				{
					sf::Color newColor = closeRandomColorChange(globals.cultures[cultureIndex].color);
					Culture childCulture = Culture(globals.cultures.size(), globals.cultures[cultureIndex].name + " Child " + std::to_string(globals.cultures[cultureIndex].children.size()), newColor); // Create a child culture
					childCulture.cells.push_back(neighbor.id); // Add the cell to the child culture
					childCulture.overExtension.push_back(0.f); // Add the overExtension cost to the child culture
					
					newCultures.push_back(childCulture); // Add the child culture to the cultures
					
					neighbor.culture = childCulture.id; // Set the culture of the neighbor cell
					globals.cultures[cultureIndex].children.push_back(childCulture.id); // Add the child culture to the children
					foundCells = neighbor.id; // Set the found cell
				}
				else if (!found)
				{
					globals.cultures[cultureIndex].cells.push_back(neighbor.id); // Add the cell to the culture
					globals.cultures[cultureIndex].overExtension.push_back(overExtensionCost); // Add the overExtension cost to the culture
					globals.cultures[cultureIndex].populatedCells++; // Increment the number of populated cells
					foundCells = neighbor.id; // Set the found cell
					neighbor.culture = globals.cultures[cultureIndex].id; // Set the culture of the neighbor cell
				}
				found = true;
			}
		}
		// At the end, for safety
		if (!found) {
			// delete the cell from cells in culture
			globals.cultures[cultureIndex].cells.erase(globals.cultures[cultureIndex].cells.begin() + cellId); // Remove the cell from the culture
			globals.cultures[cultureIndex].overExtension.erase(globals.cultures[cultureIndex].overExtension.begin() + cellId); // Remove the overExtension cost from the culture 
		}

	}
	return foundCells; // Return the found cell
}


std::vector<std::size_t> cultureTicker(std::vector<Cell>& cells, GlobalWorldObjects& globals)
{
	std::vector<std::size_t> changed_cells;
	changed_cells.reserve(globals.cultures.size());

	std::vector<Culture> newCultures; // Vector of new cultures

	for (int i = 0; i < globals.cultures.size(); i++)
	{
		std::size_t changed = cultureChangeTick(cells, globals, i, newCultures);
		
		if (changed == vor::INVALID_INDEX) continue; // If no cell was changed, continue

		changed_cells.push_back(changed); // Add the changed cell to the vector
	}

	// After iterating, add all new cultures to globals.cultures
	for (const auto& culture : newCultures)
	{
		globals.cultures.push_back(culture);
	}

	return changed_cells;
}

void tick(vor::Voronoi& map, GlobalWorldObjects& globals, MapConfig& config, VertexMap& vertexMap, float deltaTime, std::mutex& globalsMutex)
{
	//// Try to acquire the lock with a timeout
	//std::unique_lock<std::mutex> lock(globalsMutex, std::defer_lock);
	//if (!lock.try_lock_for(std::chrono::seconds(5))) {
	//	std::cout << "Failed to acquire globalsMutex after 5 seconds. Possible deadlock." << std::endl;
	//	return;
	//}

	// Lock mutex before modifications
	std::lock_guard<std::mutex> lock(globalsMutex);

	// Culture Ticker
	std::vector<std::size_t> change = cultureTicker(map.cells, globals);

	// Update color of map given changed cells
	for (std::size_t i = 0; i < change.size(); i++)
	{
		std::size_t cellId = change[i];
		Cell& cell = map.cells[cellId];
		if (cell.culture == -1) throw std::runtime_error("Cell has no culture");
		Culture& culture = globals.cultures[cell.culture];

		for (size_t j = map.cells[cellId].vertex_offset; j < map.cells[cellId].vertex_offset + map.cells[cellId].vertex.size() * 3; j++)
		{
			map.vertices[j].color = culture.color;
		}
	}
	// Update the vertex buffer
	vertexMap.update(map);

}


class TickerSimulation {
public:
	TickerSimulation()
		: running(false), speed(1.0f), stopFlag(false) {}

	void start(vor::Voronoi& map, GlobalWorldObjects& globals, MapConfig& config, VertexMap& vertexMap) {
		if (running) return; // Already running
		stopFlag = false;
		running = true;
		tickerThread = std::thread(&TickerSimulation::run, this, std::ref(map), std::ref(globals), std::ref(config), std::ref(vertexMap));
	}

	void stop()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			stopFlag = true;
			running = false;
		}
		cv.notify_all();
		if (tickerThread.joinable()) {
			tickerThread.join();
		}
	}
	void setSpeed(float newSpeed) {
		std::lock_guard<std::mutex> lock(mutex);
		speed = newSpeed;
		cv.notify_all();
	}
	void pause() {
		std::lock_guard<std::mutex> lock(mutex);
		running = false;
	}
	void resume() {
		{
			std::lock_guard<std::mutex> lock(mutex);
			running = true;
		}
		cv.notify_all();
	}
	~TickerSimulation() {
		stop();
	}


private:
	std::thread tickerThread;
	std::atomic<bool> running;
	std::atomic<bool> stopFlag;
	std::mutex mutex;
	std::mutex globalsMutex;
	std::condition_variable cv;
	float speed;
	void run(vor::Voronoi& map, GlobalWorldObjects& globals, MapConfig& config, VertexMap& vertexMap) {
		while (!stopFlag) {
			{
				std::unique_lock<std::mutex> lock(mutex);
				cv.wait(lock, [this]() { return running || stopFlag; });
				if (stopFlag) {
					break;
				}
			}

			// Perform the tick
			tick(map, globals, config, vertexMap, speed, globalsMutex);

			// Wait based on the simulation speed
			std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(500 / speed)));
		}
	}
};


void cultureCountPieChart(GlobalWorldObjects& globals, bool colorChange = false)
{
	// Biome distribution pie chart
	std::vector<const char*> labels;
	std::vector<float> data;
	ImU32 colors[1000];

	for (int i = 0; i < globals.cultures.size(); i++) {
		Culture& culture = globals.cultures[i];

		labels.push_back(culture.name.c_str());
		data.push_back(static_cast<float>(culture.populatedCells));

		float color[4];
		color[0] = culture.color.r / 255.0f;
		color[1] = culture.color.g / 255.0f;
		color[2] = culture.color.b / 255.0f;
		color[3] = 1.0f;
		// Convert color to ImU32
		colors[i] = ImColor(color[0], color[1], color[2], color[3]);
	}

	pushTempColormap("Culture Distribution", "CultureColormap", colors, globals.cultures.size(), colorChange);
	if (ImPlot::BeginPlot("Culture Distribution", ImVec2(-1, 0), ImPlotFlags_Equal)) {
		if (labels.size() == data.size()) {
			ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations);
			ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoDecorations);
			ImPlot::PlotPieChart(labels.data(), data.data(), static_cast<int>(labels.size()), 0.5, 0.5, 0.4, "%.0f", 90, ImPlotFlags_NoInputs);
		}
		ImPlot::EndPlot();
	}
	ImPlot::PopColormap();
}

/// There is a problem with indexes being 16 bit and it running out..
void cultureLineChart(GlobalWorldObjects& globals, bool colorChange = false)
{
	// Store historical data for each culture across ticks
	static std::map<int, std::vector<float>> cultureHistory; // culture_id -> historical data
	static int tickCount = 0;
	static std::vector<float> tickLabels;

	// Update history with current data
	tickCount++;

	// Line chart counting ticks ahead
	std::vector<const char*> labels;
	std::vector<ImU32> colors;
	labels.reserve(globals.cultures.size());
	colors.reserve(globals.cultures.size());

	// Update history for each culture
	for (int i = 0; i < globals.cultures.size(); i++) {
		Culture& culture = globals.cultures[i];
		labels.push_back(culture.name.c_str());

		// Create or update history for this culture
		if (cultureHistory.find(culture.id) == cultureHistory.end()) {
			// Culture encountered for the first time, backfill with zeros
			cultureHistory[culture.id] = std::vector<float>(tickCount - 1, 0.0f);
		}

		// Add current data point
		cultureHistory[culture.id].push_back(static_cast<float>(culture.populatedCells));

		// Create color
		ImU32 color = ImColor(
			culture.color.r / 255.0f,
			culture.color.g / 255.0f,
			culture.color.b / 255.0f,
			1.0f
		);
		colors.push_back(color);
	}

	// Create x-axis values (as float)
	tickLabels.push_back(static_cast<float>(tickCount));

	// Set up and draw the plot
	if (ImPlot::BeginPlot("Culture Growth Over Time", ImVec2(-1, 400))) {
		// Configure axes
		ImPlot::SetupAxes("Ticks", "Population", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

		// Plot each culture's data
		for (int i = 0; i < globals.cultures.size(); i++) {
			Culture& culture = globals.cultures[i];
			const std::vector<float>& history = cultureHistory[culture.id];

			// Convert color to ImVec4
			ImVec4 lineColor = ImVec4(
				culture.color.r / 255.0f,
				culture.color.g / 255.0f,
				culture.color.b / 255.0f,
				1.0f
			);

			// Set line style and color
			ImPlot::SetNextLineStyle(lineColor);

			// Create x-axis data as float matching the history size
			std::vector<float> xData;
			xData.reserve(history.size());
			for (int j = 0; j < history.size(); j++) {
				xData.push_back(static_cast<float>(tickCount - history.size() + 1 + j));
			}

			// Try the most common ImPlot::PlotLine signature
			ImPlot::PlotLine(culture.name.c_str(), xData.data(), history.data(), history.size());
		}

		ImPlot::EndPlot();
	}

	// If color change is requested, update the colormap
	if (colorChange) {
		ImU32* colorArray = new ImU32[globals.cultures.size()];
		for (int i = 0; i < globals.cultures.size(); i++) {
			colorArray[i] = colors[i];
		}
		pushTempColormap("Culture Distribution", "CultureColormap", colorArray, globals.cultures.size(), true);
		delete[] colorArray;
	}
}

void tickerControls(TickerSimulation& ticker, 
	float& simulationSpeed, vor::Voronoi& map, 
	GlobalWorldObjects& globals, MapConfig& config, 
	VertexMap& vertexMap)
{
	//// TICKER CONTRLS
	ImGui::Begin("Simulation Controls");
	ImGui::SliderFloat("Simulation Speed", &simulationSpeed, 0.1f, 10.0f);
	if (ImGui::Button("Start")) {
		ticker.start(map, globals, config, vertexMap);
	}
	if (ImGui::Button("Pause")) {
		ticker.pause();
	}
	if (ImGui::Button("Resume")) {
		ticker.resume();
	}
	if (ImGui::Button("Stop")) {
		ticker.stop();
	}
	ticker.setSpeed(simulationSpeed);

	// Add the piechart Ui from the biomes
	cultureCountPieChart(globals, false);

	// if ticker.running or something 
	//cultureLineChart(globals, false);

	ImGui::End();
}