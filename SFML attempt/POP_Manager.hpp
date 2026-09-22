#pragma once
#include "POP_data.hpp"
#include "Voronoi.hpp"
#include <unordered_map>
#include <vector>
#include <functional>

// ===================
// PopCreateInfo - Information needed to create a pop
// ===================
struct PopCreateInfo {
	size_t province_id;		// Province to place the pop in
	int32_t initial_count;	// Initial population count
	uint16_t culture_id;	// Culture of the pop
	uint16_t religion_id;	// Religion of the pop
	uint16_t language_id;	// Language of the pop
	int32_t job_id = -1;	// Job id of the pop

	// Additional attributes can be added here
	float initial_wealth = 100.0f; // Initial wealth
	float initial_concious = 0.5f; // Initial consciousness
	float initial_happy = 0.5f;   // Initial happiness
	float initial_militancy = 0.0f; // Initial militancy
	float initial_literacy = 0.1f; // Initial literacy
};

// =================
// PopManager - Manages all population entities with SoA structure
// =================
class PopManager {
public:
	// ==== Constructor/Destructor ====
	PopManager();
	~PopManager() = default;

	// ==== Pop Lifecycle Management ====
	PopHandle CreatePop(const PopCreateInfo& info);
	void DestroyPop(PopHandle handle);
	bool IsValid(PopHandle handle) const;

	// ==== Data Access ====
	PopData& GetPopData() { return m_data; };
	const PopData& GetPopData() const { return m_data; };

	// Get individual pop data (slower, for ui)
	int32_t GetCount(PopHandle handle) const;
	float GetWealth(PopHandle handle) const;
	int32_t GetProvinceId(PopHandle handle) const;

	// ==== Spatial Queries ====
	// Iterate over pops in a province
	void ForEachPopInProvince(size_t province_id, std::function<void(PopHandle, size_t index)> callback) const;

	// Get the first pop in a province
	PopHandle GetFirstPopInProvince(size_t province_id) const;

	// ==== Archetype Management ====
	// Get all pops with a specific archetype
	ArchetypeRange GetArchetypeRange(const ArchetypeSignature& signature) const;

	// ==== Optimization ====
	// Merge similar pops to reduce fragmentation
	void MergeSimilarPops(int32_t province_id, float similarityThreshold = 0.99f);

	// Reorganize pops to improve data locality
	void DefragmentPops(); // use sparingly

	// ==== Utility ====
	size_t GetTotalPopCount() const { return m_data.size(); };
	size_t GetTotalPopulation() const;

	void Reserve(size_t n) { m_data.reserve(n); };
	void Clear();

	// ==== Province Integration ====
	void InitializeSpatialIndex(size_t numProvinces);

private:
	// ==== Core Data ====
	PopData m_data; // Structure of Arrays for pop attributes

	// ==== Spatial Indexing ====
	std::vector<int32_t> m_province_heads; // index to first pop in each province

	// ==== Archetype Tracking ====
	std::unordered_map<uint64_t, ArchetypeRange> m_archetype_map; // Map from archetype hash to range

	// ==== Free List for Pop Recycling ====
	std::vector<int32_t> m_free_indices; // Indices of free pop slots

	// ==== Generation Tracking ====
	uint32_t m_next_generation = 1; // never 0, as 0 indicates invalid handle

	// ==== Internal Helpers ====
	size_t AllocateSlot();
	void FreeSlot(size_t index);
	void LinkPopToProvince(size_t popIndex, size_t province_id);
	void UnlinkPopFromProvince(size_t popIndex);
	void RebuildArchetypeMap();

	// ==== Validation ====
	bool ValidateHandle(PopHandle handle) const {
		return handle.index < m_data.size() &&
			m_data.generations[handle.index] == handle.generation &&
			handle.generation != 0;
	}
};