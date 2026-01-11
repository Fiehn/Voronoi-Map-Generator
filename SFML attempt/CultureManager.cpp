#pragma once
#include "CultureManager.hpp"

// =================
// CultureData Implementation
// =================

void CultureData::reserve(size_t n) {
	names.reserve(n);
	colors.reserve(n);
	primary_language_id.reserve(n);
	religion_id.reserve(n);
	parent_culture_id.reserve(n);
	founding_dates.reserve(n);
	origin_cell_ids.reserve(n);
	divergence_rates.reserve(n);
	inovation_rate.reserve(n);
	military_tradition.reserve(n);
	individualism.reserve(n);
	materialism.reserve(n);
	patriarchy.reserve(n);
	temperance.reserve(n);
	closenessToNature.reserve(n);
	religiosity.reserve(n);
	artFocus.reserve(n);
	socialStratification.reserve(n);
	expansionism.reserve(n);
	tradeFocus.reserve(n);
	multiculturalism.reserve(n);
	technology_bits.reserve(n);
}
void CultureData::clear() {
	names.clear();
	colors.clear();
	primary_language_id.clear();
	religion_id.clear();
	parent_culture_id.clear();
	founding_dates.clear();
	origin_cell_ids.clear();
	divergence_rates.clear();
	inovation_rate.clear();
	military_tradition.clear();
	individualism.clear();
	materialism.clear();
	patriarchy.clear();
	temperance.clear();
	closenessToNature.clear();
	religiosity.clear();
	artFocus.clear();
	socialStratification.clear();
	expansionism.clear();
	tradeFocus.clear();
	multiculturalism.clear();
	technology_bits.clear();
}
void CultureData::resize(size_t n) {
	names.resize(n);
	colors.resize(n);
	primary_language_id.resize(n);
	religion_id.resize(n);
	parent_culture_id.resize(n);
	founding_dates.resize(n);
	origin_cell_ids.resize(n);
	divergence_rates.resize(n);
	inovation_rate.resize(n);
	military_tradition.resize(n);
	individualism.resize(n);
	materialism.resize(n);
	patriarchy.resize(n);
	temperance.resize(n);
	closenessToNature.resize(n);
	religiosity.resize(n);
	artFocus.resize(n);
	socialStratification.resize(n);
	expansionism.resize(n);
	tradeFocus.resize(n);
	multiculturalism.resize(n);
	technology_bits.resize(n);
}
// =================
// CultureManager Implementation
// =================
CultureManager::CultureManager() {
	// Initialize with capacity for 256 cultures
	m_data.reserve(256);
}

CultureHandle CultureManager::CreateCulture(const CultureCreateInfo& info) {
	// Allocate new slot
	uint16_t index = AllocateSlot();
	// If we grew, resize all data arrays
	if (index >= m_data.size()) {
		m_data.resize(index + 1);
	}
	// Initialize culture data
	m_data.names[index] = info.name;
	m_data.colors[index] = info.color;
	m_data.primary_language_id[index] = info.primary_language;
	m_data.religion_id[index] = info.primary_religion;
	m_data.parent_culture_id[index] = 0; // Proto-culture
	m_data.founding_dates[index] = info.founding_date;
	m_data.origin_cell_ids[index] = info.origin_cell_id;
	m_data.divergence_rates[index] = 0.0f; // No divergence for proto-culture
	// Initialize cultural traits
	m_data.inovation_rate[index] = info.inovation_rate;
	m_data.military_tradition[index] = info.military_tradition;
	m_data.individualism[index] = info.individualism;
	m_data.materialism[index] = info.materialism;
	m_data.patriarchy[index] = info.patriarchy;
	m_data.temperance[index] = info.temperance;
	m_data.closenessToNature[index] = info.closenessToNature;
	m_data.religiosity[index] = info.religiosity;
	m_data.artFocus[index] = info.artFocus;
	m_data.socialStratification[index] = info.socialStratification;
	m_data.expansionism[index] = info.expansionism;
	m_data.tradeFocus[index] = info.tradeFocus;
	m_data.multiculturalism[index] = info.multiculturalism;
	// Create handle
	CultureHandle handle;
	handle.index = index;
	handle.generation = m_next_generation++;
	return handle;
}

CultureHandle CultureManager::DeriveCulture(CultureHandle parent_handle, const CultureCreateInfo& info) {
	// Validate parent handle
	if (!ValidateHandle(parent_handle)) {
		throw std::runtime_error("Invalid parent culture handle in DeriveCulture.");
	}
	uint16_t parent_index = parent_handle.index;
	// Allocate new slot
	uint16_t index = AllocateSlot();
	// If we grew, resize all data arrays
	if (index >= m_data.size()) {
		m_data.resize(index + 1);
	}
	// Initialize culture data
	m_data.names[index] = info.name;
	m_data.colors[index] = info.color;
	m_data.primary_language_id[index] = info.primary_language;
	m_data.religion_id[index] = info.primary_religion;
	m_data.parent_culture_id[index] = parent_index;
	m_data.founding_dates[index] = info.founding_date;
	m_data.origin_cell_ids[index] = info.origin_cell_id;
	m_data.divergence_rates[index] = 0.0f; // Start with no divergence
	// Inherit traits from parent culture
	InheritTraitsFromParent(index, parent_index, 0.7f); // 70% inheritance factor
	// Apply any specified trait overrides
	m_data.inovation_rate[index] = info.inovation_rate;
	m_data.military_tradition[index] = info.military_tradition;
	m_data.individualism[index] = info.individualism;
	m_data.materialism[index] = info.materialism;
	m_data.patriarchy[index] = info.patriarchy;
	m_data.temperance[index] = info.temperance;
	m_data.closenessToNature[index] = info.closenessToNature;
	m_data.religiosity[index] = info.religiosity;
	m_data.artFocus[index] = info.artFocus;
	m_data.socialStratification[index] = info.socialStratification;
	m_data.expansionism[index] = info.expansionism;
	m_data.tradeFocus[index] = info.tradeFocus;
	m_data.multiculturalism[index] = info.multiculturalism;
	// Create handle
	CultureHandle handle;
	handle.index = index;
	handle.generation = m_next_generation++;
	return handle;
}

void CultureManager::MarkExtinct(CultureHandle handle) {
	if (!ValidateHandle(handle)) {
		return; // Invalid handle
	}
	uint16_t index = handle.index;
	// Free the slot for reuse
	FreeSlot(index);
}

bool CultureManager::IsValid(CultureHandle handle) const {
	return ValidateHandle(handle);
}

bool CultureManager::ValidateHandle(CultureHandle handle) const {
	if (handle.index >= m_data.size()) {
		return false;
	}
	// In this simple implementation, we don't track generations, so just check index
	return true;
}

std::string CultureManager::GetCultureName(CultureHandle handle) const {
	if (!ValidateHandle(handle)) {
		return "Invalid Culture";
	}
	return m_data.names[handle.index];
}

uint16_t CultureManager::GetPrimaryLanguage(CultureHandle handle) const {
	if (!ValidateHandle(handle)) {
		return 0; // Invalid culture
	}
	return m_data.primary_language_id[handle.index];
}

uint16_t CultureManager::GetPrimaryReligion(CultureHandle handle) const {
	if (!ValidateHandle(handle)) {
		return 0; // Invalid culture
	}
	return m_data.religion_id[handle.index];
}

std::vector<CultureHandle> CultureManager::GetDerivedCultures(CultureHandle handle) const {
	std::vector<CultureHandle> derived;
	if (!ValidateHandle(handle)) {
		return derived; // Invalid handle
	}
	uint16_t index = handle.index;
	for (uint16_t i = 0; i < m_data.size(); ++i) {
		if (m_data.parent_culture_id[i] == index) {
			CultureHandle child_handle;
			child_handle.index = i;
			child_handle.generation = m_next_generation; // Placeholder generation
			derived.push_back(child_handle);
		}
	}
	return derived;
}

CultureHandle CultureManager::GetRootAncestor(CultureHandle handle) const {
	if (!ValidateHandle(handle)) {
		return CultureHandle::Invalid();
	}
	uint16_t index = handle.index;
	while (m_data.parent_culture_id[index] != 0) {
		index = m_data.parent_culture_id[index];
	}
	CultureHandle root_handle;
	root_handle.index = index;
	root_handle.generation = m_next_generation; // Placeholder generation
	return root_handle;
}

float CultureManager::CalculateCulturalDistance(CultureHandle a, CultureHandle b) const {
	if (!ValidateHandle(a) || !ValidateHandle(b)) {
		return 1.0f; // Max distance for invalid cultures
	}
	uint16_t index_a = a.index;
	uint16_t index_b = b.index;
	// Simple Euclidean distance in trait space
	float distance = 0.0f;
	distance += (m_data.inovation_rate[index_a] - m_data.inovation_rate[index_b]) * (m_data.inovation_rate[index_a] - m_data.inovation_rate[index_b]);
	distance += (m_data.military_tradition[index_a] - m_data.military_tradition[index_b]) * (m_data.military_tradition[index_a] - m_data.military_tradition[index_b]);
	distance += (m_data.individualism[index_a] - m_data.individualism[index_b]) * (m_data.individualism[index_a] - m_data.individualism[index_b]);
	distance += (m_data.materialism[index_a] - m_data.materialism[index_b]) * (m_data.materialism[index_a] - m_data.materialism[index_b]);
	distance += (m_data.patriarchy[index_a] - m_data.patriarchy[index_b]) * (m_data.patriarchy[index_a] - m_data.patriarchy[index_b]);
	distance += (m_data.temperance[index_a] - m_data.temperance[index_b]) * (m_data.temperance[index_a] - m_data.temperance[index_b]);
	distance += (m_data.closenessToNature[index_a] - m_data.closenessToNature[index_b]) * (m_data.closenessToNature[index_a] - m_data.closenessToNature[index_b]);
	distance += (m_data.religiosity[index_a] - m_data.religiosity[index_b]) * (m_data.religiosity[index_a] - m_data.religiosity[index_b]);
	distance += (m_data.artFocus[index_a] - m_data.artFocus[index_b]) * (m_data.artFocus[index_a] - m_data.artFocus[index_b]);
	distance += (m_data.socialStratification[index_a] - m_data.socialStratification[index_b]) * (m_data.socialStratification[index_a] - m_data.socialStratification[index_b]);
	distance += (m_data.expansionism[index_a] - m_data.expansionism[index_b]) * (m_data.expansionism[index_a] - m_data.expansionism[index_b]);
	distance += (m_data.tradeFocus[index_a] - m_data.tradeFocus[index_b]) * (m_data.tradeFocus[index_a] - m_data.tradeFocus[index_b]);
	distance += (m_data.multiculturalism[index_a] - m_data.multiculturalism[index_b]) * (m_data.multiculturalism[index_a] - m_data.multiculturalism[index_b]);
	return sqrt(distance);
}

bool CultureManager::ShareRecentCommonAncestor(CultureHandle a, CultureHandle b, uint32_t max_generations) const {
	if (!ValidateHandle(a) || !ValidateHandle(b)) {
		return false;
	}
	uint16_t index_a = a.index;
	uint16_t index_b = b.index;
	std::vector<uint16_t> ancestors_a;
	std::vector<uint16_t> ancestors_b;
	// Collect ancestors for culture A
	while (index_a != 0) {
		ancestors_a.push_back(index_a);
		index_a = m_data.parent_culture_id[index_a];
	}
	// Collect ancestors for culture B
	while (index_b != 0) {
		ancestors_b.push_back(index_b);
		index_b = m_data.parent_culture_id[index_b];
	}
	// Check for common ancestors within max_generations
	size_t generations = 0;
	for (uint16_t anc_a : ancestors_a) {
		for (uint16_t anc_b : ancestors_b) {
			if (anc_a == anc_b && generations <= max_generations) {
				return true;
			}
		}
		generations++;
		if (generations > max_generations) {
			break;
		}
	}
	return false;
}
size_t CultureManager::GetCultureOriginCell(CultureHandle handle) const {
	if (!ValidateHandle(handle)) {
		return vor::INVALID_INDEX; // Invalid culture
	}
	return m_data.origin_cell_ids[handle.index];
}
float CultureManager::CalculateExpansionSuitability(CultureHandle handle, const Cell& cell) const {
	if (!ValidateHandle(handle)) {
		return 0.0f; // Invalid culture
	}
	uint16_t index = handle.index;
	float suitability = 1.0f;
	// Adjust suitability based on biome adaptations
	// (Placeholder logic, real implementation would consider actual biome types)
	if (cell.coastBool) {
		suitability *= (1.0f + m_data.closenessToNature[index] * 0.1f);
	}
	else {
		suitability *= (1.0f - m_data.closenessToNature[index] * 0.1f);
	}
	return suitability;
}
uint16_t CultureManager::AllocateSlot() {
	if (!m_free_indices.empty()) {
		uint16_t index = m_free_indices.back();
		m_free_indices.pop_back();
		return index;
	}
	return static_cast<uint16_t>(m_data.size());
}
void CultureManager::FreeSlot(uint16_t index) {
	m_free_indices.push_back(index);
}
void CultureManager::AccumulateDrift(CultureHandle handle, float amount) {
	if (!ValidateHandle(handle)) {
		return; // Invalid handle
	}
	uint16_t index = handle.index;
	// Simple accumulation (could be more complex)
	m_data.divergence_rates[index] += amount;
}
bool CultureManager::ShouldSpeciate(CultureHandle handle, float drift_threshold) const {
	if (!ValidateHandle(handle)) {
		return false; // Invalid handle
	}
	uint16_t index = handle.index;
	return m_data.divergence_rates[index] >= drift_threshold;
}
void CultureManager::InheritTraitsFromParent(uint16_t child_index, uint16_t parent_index, float inheritance_factor) {
	m_data.inovation_rate[child_index] = m_data.inovation_rate[parent_index] * inheritance_factor + m_data.inovation_rate[child_index] * (1.0f - inheritance_factor);
	m_data.military_tradition[child_index] = m_data.military_tradition[parent_index] * inheritance_factor + m_data.military_tradition[child_index] * (1.0f - inheritance_factor);
	m_data.individualism[child_index] = m_data.individualism[parent_index] * inheritance_factor + m_data.individualism[child_index] * (1.0f - inheritance_factor);
	m_data.materialism[child_index] = m_data.materialism[parent_index] * inheritance_factor + m_data.materialism[child_index] * (1.0f - inheritance_factor);
	m_data.patriarchy[child_index] = m_data.patriarchy[parent_index] * inheritance_factor + m_data.patriarchy[child_index] * (1.0f - inheritance_factor);
	m_data.temperance[child_index] = m_data.temperance[parent_index] * inheritance_factor + m_data.temperance[child_index] * (1.0f - inheritance_factor);
	m_data.closenessToNature[child_index] = m_data.closenessToNature[parent_index] * inheritance_factor + m_data.closenessToNature[child_index] * (1.0f - inheritance_factor);
	m_data.religiosity[child_index] = m_data.religiosity[parent_index] * inheritance_factor + m_data.religiosity[child_index] * (1.0f - inheritance_factor);
	m_data.artFocus[child_index] = m_data.artFocus[parent_index] * inheritance_factor + m_data.artFocus[child_index] * (1.0f - inheritance_factor);
	m_data.socialStratification[child_index] = m_data.socialStratification[parent_index] * inheritance_factor + m_data.socialStratification[child_index] * (1.0f - inheritance_factor);
	m_data.expansionism[child_index] = m_data.expansionism[parent_index] * inheritance_factor + m_data.expansionism[child_index] * (1.0f - inheritance_factor);
	m_data.tradeFocus[child_index] = m_data.tradeFocus[parent_index] * inheritance_factor + m_data.tradeFocus[child_index] * (1.0f - inheritance_factor);
	m_data.multiculturalism[child_index] = m_data.multiculturalism[parent_index] * inheritance_factor + m_data.multiculturalism[child_index] * (1.0f - inheritance_factor);
}

size_t CultureManager::GetActiveCultures() const {
	return m_data.size() - m_free_indices.size();
}

void CultureManager::Clear() {
	m_data.clear();
	m_free_indices.clear();
	m_next_generation = 1;
}
void CultureManager::AdaptToBiome(uint16_t culture_index, int biome_type) {
	// Placeholder logic for biome adaptation TODO
	// In a real implementation, this would adjust culture traits based on biome
}