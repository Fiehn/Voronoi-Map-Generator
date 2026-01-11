#pragma once
#include "CultureData.hpp"
#include "Voronoi.hpp"
#include <unordered_map>

// =================
// Culture Create Info - Information needed to create a culture
// =================
struct CultureCreateInfo {
	std::string name;
	sf::Color color;
	size_t origin_cell_id;
	uint32_t founding_date;

	// Optional - Parent culture
	uint16_t parent_culture = 0;

	// Initial Language and Religion
	uint16_t primary_language = 0;
	uint16_t primary_religion = 0;

	// intial values for cultural traits
	float inovation_rate = 0.5f;        // Tech adoption rate
	float military_tradition = 0.5f;   // Military focus
	float individualism = 0.5f;        // Individualism vs collectivism
	float materialism = 0.5f;         // Materialism vs spiritualism
	float patriarchy = 0.5f;          // Patriarchal vs egalitarian
	float temperance = 0.5f;          // Temperance vs indulgence
	float closenessToNature = 0.5f;   // Harmony with nature
	float religiosity = 0.5f;         // Religiousness
	float artFocus = 0.5f;            // Focus on arts and culture
	float socialStratification = 0.5f; // Social hierarchy
	float expansionism = 0.5f;        // Expansionist tendencies
	float tradeFocus = 0.5f;         // Trade and commerce focus
	float multiculturalism = 0.5f;   // Openness to other cultures

	// Geographic Adaptation
	int origin_biome = -1; // Biome where the culture originated
	bool origin_coast = false; // Whether the culture originated on the coast
};

// =================
// Culture Manager - Manages creation and storage of cultures
// =================

class CultureManager
{
public:
	CultureManager();

	// === Culutre Lifecycle ===
	CultureHandle CreateCulture(const CultureCreateInfo& info);
	CultureHandle DeriveCulture(CultureHandle parent_handle, const CultureCreateInfo& info);
	void MarkExtinct(CultureHandle handle);
	bool IsValid(CultureHandle handle) const;

	// === Accessors ===
	CultureData& GetCultureData() { return m_data; }
	const CultureData& GetCultureData() const { return m_data; }

	std::string GetCultureName(CultureHandle handle) const;
	uint16_t GetPrimaryLanguage(CultureHandle handle) const;
	uint16_t GetPrimaryReligion(CultureHandle handle) const;

	// === Phylogenetic Queries ===
	std::vector<CultureHandle> GetDerivedCultures(CultureHandle handle) const;

	CultureHandle GetRootAncestor(CultureHandle handle) const;

	float CalculateCulturalDistance(CultureHandle a, CultureHandle b) const;

	bool ShareRecentCommonAncestor(CultureHandle a, CultureHandle b, uint32_t max_generations) const;

	// === Geographic Integration ===
	size_t GetCultureOriginCell(CultureHandle handle) const;

	float CalculateExpansionSuitability(CultureHandle handle, const Cell& cell) const;

	// === Cultural Evolution ===
	void AccumulateDrift(CultureHandle handle, float amount);
	bool ShouldSpeciate(CultureHandle handle, float drift_threshold = 0.5f) const;
	
	// === Statistics ===
	size_t GetTotalCultures() const { return m_data.size(); }
	size_t GetActiveCultures() const; // Non-extinct cultures

	void Reserve(size_t n) {
		m_data.reserve(n);
	}
	void Clear();
private:
	CultureData m_data;

	// Free list for reusing extinct culture slots (Maybe this is a bad idea)
	std::vector<uint16_t> m_free_indices;
	uint16_t m_next_generation = 1;

	// Helpers
	uint16_t AllocateSlot();
	void FreeSlot(uint16_t index);
	void InheritTraitsFromParent(uint16_t child_index, uint16_t parent_index, float inheritance_factor);
	void AdaptToBiome(uint16_t culture_index, int biome_type);

	bool ValidateHandle(CultureHandle handle) const;
};
