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

std::size_t cultureChangeTick(std::vector<Cell>& map, GlobalWorldObjects& globals, int cultureIndex, std::vector<Culture>& newCultures)
{
	// Add a new cell from any of the neighbors in cells
	// 1. Find a valid random cell with neighbors not in cells
	// 2. calculate the overExtension cost of the cell
	// 3. If the cost is too high, break off a child culture
	// 4. If the cost is low, add the cell to the culture

	bool found = false;
	std::size_t foundCell = vor::INVALID_INDEX; // Cell that was found

	int maxAttempts = 100; // Prevent infinite loops
	int attempts = 0;

	while (!found)
	{
		attempts++;

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

				if (overExtensionCost > 10.f)
				{
					sf::Color newColor = closeRandomColorChange(globals.cultures[cultureIndex].color);
					Culture childCulture = Culture(globals.cultures.size(), globals.cultures[cultureIndex].name + " Child", newColor); // Create a child culture
					childCulture.cells.push_back(neighbor.id); // Add the cell to the child culture
					childCulture.overExtension.push_back(0.f); // Add the overExtension cost to the child culture
					
					newCultures.push_back(childCulture); // Add the child culture to the cultures
					
					neighbor.culture = childCulture.id; // Set the culture of the neighbor cell
					globals.cultures[cultureIndex].children.push_back(childCulture.id); // Add the child culture to the children
					foundCell = neighbor.id; // Set the found cell
				}
				else
				{
					globals.cultures[cultureIndex].cells.push_back(neighbor.id); // Add the cell to the culture
					globals.cultures[cultureIndex].overExtension.push_back(overExtensionCost); // Add the overExtension cost to the culture
					globals.cultures[cultureIndex].populatedCells++; // Increment the number of populated cells
					foundCell = neighbor.id; // Set the found cell
					neighbor.culture = globals.cultures[cultureIndex].id; // Set the culture of the neighbor cell
				}
				found = true;
				break;
			}
		}
		// At the end, for safety
		if (attempts >= maxAttempts) {
			std::cout << "Max attempts reached for culture " << cultureIndex << std::endl;
			return vor::INVALID_INDEX; // Return invalid index to indicate failure
		}

	}
	return foundCell; // Return the found cell
}


std::vector<std::size_t> cultureTicker(std::vector<Cell>& cells, GlobalWorldObjects& globals)
{
	std::vector<std::size_t> changed_cells;
	changed_cells.reserve(globals.cultures.size());

	std::vector<Culture> newCultures; // Vector of new cultures

	for (int i = 0; i < globals.cultures.size(); i++)
	{
		std::size_t changed = cultureChangeTick(cells, globals, i, newCultures);
		if (changed != vor::INVALID_INDEX)
		{
			changed_cells.push_back(changed);
		}
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

	std::cout << "Changed cells: ";
	for (std::size_t cellId : change) {
		std::cout << cellId << " ";
	}
	std::cout << std::endl;

	// Update color of map given changed cells
	for (std::size_t i = 0; i < change.size(); i++)
	{
		std::size_t cellId = change[i];
		Cell& cell = map.cells[cellId];
		if (cell.culture == -1) throw std::runtime_error("Cell has no culture");
		Culture& culture = globals.cultures[cell.culture];

		// Debug: Log the culture color being applied
		std::cout << "Updating cell " << cellId << " with culture color: "
			<< static_cast<int>(culture.color.r) << ", "
			<< static_cast<int>(culture.color.g) << ", "
			<< static_cast<int>(culture.color.b) << std::endl;

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
				std::cout << "Waiting for running signal..." << std::endl;
				cv.wait(lock, [this]() { return running || stopFlag; });
				if (stopFlag) {
					std::cout << "Stop flag detected, exiting thread..." << std::endl;
					break;
				}
				std::cout << "Running signal received, continuing..." << std::endl;
			}

			// Debug: Log each tick
			std::cout << "About to acquire globalsMutex for tick..." << std::endl;

			// Perform the tick
			tick(map, globals, config, vertexMap, speed, globalsMutex);

			std::cout << "Tick completed, sleeping..." << std::endl;
			// Wait based on the simulation speed
			std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(1000 / speed)));
			std::cout << "Sleep completed, starting next iteration..." << std::endl;
		}
	}
};

