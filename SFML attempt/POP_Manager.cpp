#include "POP_Manager.hpp"
#include <algorithm>
#include <numeric>
#include <cassert>

// ==================
// PopData Implementation
// ==================
void PopData::reserve(size_t n) {
	count.reserve(n);
	wealth.reserve(n);
	concious.reserve(n);
	happy.reserve(n);
	militancy.reserve(n);
	literacy.reserve(n);
	job_id.reserve(n);
	cell_id.reserve(n);
	culture_id.reserve(n);
	religion_id.reserve(n);
	language_id.reserve(n);
	migration_accum.reserve(n);
	growth_accum.reserve(n);
	next_pop_indices.reserve(n);
	generations.reserve(n);
}
void PopData::clear() {
	count.clear();
	wealth.clear();
	concious.clear();
	happy.clear();
	militancy.clear();
	literacy.clear();
	job_id.clear();
	cell_id.clear();
	culture_id.clear();
	religion_id.clear();
	language_id.clear();
	migration_accum.clear();
	growth_accum.clear();
	next_pop_indices.clear();
	generations.clear();
}
void PopData::resize(size_t n) {
	count.resize(n);
	wealth.resize(n);
	concious.resize(n);
	happy.resize(n);
	militancy.resize(n);
	literacy.resize(n);
	job_id.resize(n);
	cell_id.resize(n);
	culture_id.resize(n);
	religion_id.resize(n);
	language_id.resize(n);
	migration_accum.resize(n);
	growth_accum.resize(n);
	next_pop_indices.resize(n);
	generations.resize(n);
}
// ==================
// PopManager Implementation
// ==================
PopManager::PopManager() {
	// Initialize with 1024 pops capacity
	Reserve(1024);
}

PopHandle PopManager::CreatePop(const PopCreateInfo& info) {
	// Allocate a new slot (reuse from free list if possible)
	size_t index = AllocateSlot();

	// if we grow ensure all data arrays are resized
	if (index >= m_data.size()) {
		m_data.resize(index + 1);
	}

	// Assign generation for handle validation
	m_data.generations[index] = m_next_generation++;

	// Initialize hot data	
	m_data.count[index] = info.initial_count;
	m_data.wealth[index] = info.initial_wealth;
	m_data.concious[index] = info.initial_concious;
	m_data.happy[index] = info.initial_happy;
	m_data.militancy[index] = info.initial_militancy;
	// Initialize warm data
	m_data.literacy[index] = info.initial_literacy;
	m_data.job_id[index] = info.job_id;
	m_data.cell_id[index] = info.province_id;
	// Initialize cold data
	m_data.culture_id[index] = info.culture_id;
	m_data.religion_id[index] = info.religion_id;
	m_data.language_id[index] = info.language_id;

	// Initialize accumulators
	m_data.migration_accum[index] = 0.0f;
	m_data.growth_accum[index] = 0.0f;

	// Link pop to province
	LinkPopToProvince(index, info.province_id);

	// Create handle
	PopHandle handle;
	handle.index = static_cast<uint32_t>(index);
	handle.generation = m_data.generations[index];
	return handle;
}
void PopManager::DestroyPop(PopHandle handle) {
	if (!ValidateHandle(handle)) {
		return; // Invalid handle
	}
	size_t index = handle.index;
	// Unlink from province
	UnlinkPopFromProvince(index);
	// Invalidate generation
	m_data.generations[index] = 0;
	// Free the slot
	FreeSlot(index);
}

bool PopManager::IsValid(PopHandle handle) const {
	return ValidateHandle(handle);
}
int32_t PopManager::GetCount(PopHandle handle) const {
	if (!ValidateHandle(handle)) {
		return 0; // Invalid handle
	}
	return m_data.count[handle.index];
}
float PopManager::GetWealth(PopHandle handle) const {
	if (!ValidateHandle(handle)) {
		return 0.0f; // Invalid handle
	}
	return m_data.wealth[handle.index];
}
int32_t PopManager::GetProvinceId(PopHandle handle) const {
	if (!ValidateHandle(handle)) {
		return vor::INVALID_INDEX; // Invalid handle
	}
	return static_cast<int32_t>(m_data.cell_id[handle.index]);
}
void PopManager::ForEachPopInProvince(size_t province_id, std::function<void(PopHandle, size_t index)> callback) const {
	if (province_id < 0 || province_id >= static_cast<size_t>(m_province_heads.size())) {
		return; // Invalid province
	}
	int32_t currentIndex = m_province_heads[province_id];
	while (currentIndex != -1) {
		// Create handle for this pop
		PopHandle handle;
		handle.index = static_cast<uint32_t>(currentIndex);
		handle.generation = m_data.generations[currentIndex];

		int32_t nextIndex = m_data.next_pop_indices[currentIndex];

		callback(handle, currentIndex);

		currentIndex = nextIndex;
	}
}
PopHandle PopManager::GetFirstPopInProvince(size_t province_id) const {
	if(province_id < 0 || province_id >= static_cast<size_t>(m_province_heads.size())) {
		return PopHandle::Invalid(); // Invalid province
	}
	int32_t firstIndex = m_province_heads[province_id];
	if (firstIndex == -1) {
		return PopHandle::Invalid(); // No pops in this province
	}
	PopHandle handle;
	handle.index = static_cast<uint32_t>(firstIndex);
	handle.generation = m_data.generations[firstIndex];
	return handle;
}

ArchetypeRange PopManager::GetArchetypeRange(const ArchetypeSignature& signature) const {
	auto it = m_archetype_map.find(signature.Hash());
	if (it != m_archetype_map.end()) {
		return it->second;
	}
	return ArchetypeRange{ 0, 0 }; // Not found
}
void PopManager::MergeSimilarPops(int32_t province_id, float similarityThreshold) {
	// TODO: Implementation needed
}
void PopManager::DefragmentPops() {
	// TODO: Implementation needed
}
size_t PopManager::GetTotalPopulation() const {
	return std::accumulate(m_data.count.begin(), m_data.count.end(), static_cast<size_t>(0));
}

void PopManager::Clear() {
	m_data.clear();
	m_province_heads.clear();
	m_archetype_map.clear();
	m_free_indices.clear();
	m_next_generation = 1;
}
void PopManager::InitializeSpatialIndex(size_t numProvinces) {
	m_province_heads.clear();
	m_province_heads.resize(numProvinces, -1);
}

size_t PopManager::AllocateSlot() {
	// Reuse from free list if available
	if (!m_free_indices.empty()) {
		size_t index = m_free_indices.back();
		m_free_indices.pop_back();
		return index;
	}
	// Otherwise, allocate new slot at the end
	return m_data.size();
}
void PopManager::FreeSlot(size_t index) {
	// Add index to free list
	m_free_indices.push_back(static_cast<int32_t>(index));
}
void PopManager::LinkPopToProvince(size_t popIndex, size_t province_id)
{
	assert(province_id >= 0 && province_id < m_province_heads.size());

	// Insert at the head of the linked list for the province
	m_data.next_pop_indices[popIndex] = m_province_heads[province_id];
	m_province_heads[province_id] = static_cast<int32_t>(popIndex);
}
void PopManager::UnlinkPopFromProvince(size_t popIndex)
{
	size_t province_id = m_data.cell_id[popIndex];
	if (province_id < 0 || province_id >= m_province_heads.size()) {
		return; // Invalid province
	}
	// Find and remove from linked list
	int32_t* current = &m_province_heads[province_id];
	
	while (*current != -1) {
		if (static_cast<size_t>(*current) == popIndex) {
			// Found the pop to unlink
			*current = m_data.next_pop_indices[popIndex];
			return;
		}
		current = &m_data.next_pop_indices[*current];
	}
}

void PopManager::RebuildArchetypeMap() {
	// TODO: Implementation needed
}
