#pragma once
#include <vector>
#include <cstdint>

// ==================
// PopHandle - Safe index-based refrences to a pop
// ==================
struct PopHandle {
	uint32_t index = 0;			// Index in the SoA arrays
	uint32_t generation = 0;	// Generation for validity checking

	bool operator==(const PopHandle& other) const {
		return index == other.index && generation == other.generation;
	}
	bool operator!=(const PopHandle& other) const {
		return !(*this == other);
	}
	bool isValid() const {
		return generation != 0;
	}
	static PopHandle Invalid() {
		return PopHandle{ 0, 0 };
	}
};

// ==================
// PopData - Structure of Arrays for pop attributes
// ==================

class PopData
{
public:
	// ==== Hot Data (Accessed every tick) ====
	// Memory aligned for SIMD operations (AVX2/AVX-512)
	alignas(32) std::vector<int32_t> count;			// Nr. of people in this pop
	alignas(32) std::vector<float> wealth; 			// Wealth level of this pop
	alignas(32) std::vector<float> concious;		// Consciousness level of this pop
	alignas(32) std::vector<float> happy;			// Happiness level of this pop
	alignas(32) std::vector<float> militancy;		// Militancy level of this pop

	// ==== Warm Data (Accessed frequently) ====
	std::vector<float> literacy;		// Literacy level
	std::vector<int32_t> job_id; 		// Job link
	std::vector<size_t> cell_id;	// Province link

	// ==== Cold Data (Accessed rarely) ====
	std::vector<uint16_t> culture_id;	// Culture link
	std::vector<uint16_t> religion_id;	// Religion link
	std::vector<uint16_t> language_id;	// Language link

	// ==== Acumulator Data (For calculations) ====
	std::vector<float> migration_accum; // Migration accumulation
	std::vector<float> growth_accum;    // Growth accumulation

	// ==== Spatial Indexing (intrusive linked list) ====
	std::vector<int32_t> next_pop_indices; // Next pop in the same province

	// ==== Metadata ====
	std::vector<uint32_t> generations; // Generation for validity checking

	// ==== Capacity Management ====
	size_t size() const { return count.size(); }
	size_t capacity() const { return count.capacity(); }

	// reserve memory for all arrays
	void reserve(size_t n);

	// clear all data
	void clear();

	// resize all arrays
	void resize(size_t n);

private:
	friend class PopManager;
};


// ==================
// ArchetypeSignature - Bitmask representing pop archetype
// ==================
struct ArchetypeSignature {
	uint16_t culture_id; 	// Culture link
	uint16_t religion_id;	// Religion link
	uint16_t language_id;	// Language link
	uint16_t profession_id;	// Derived from job_id

	uint64_t Hash() const {
				return (static_cast<uint64_t>(culture_id) << 48) |
			   (static_cast<uint64_t>(religion_id) << 32) |
			   (static_cast<uint64_t>(language_id) << 16) |
					(static_cast<uint64_t>(profession_id));
	}

	bool operator==(const ArchetypeSignature& other) const {
		return culture_id == other.culture_id &&
			   religion_id == other.religion_id &&
			   language_id == other.language_id &&
			   profession_id == other.profession_id;
	}
};
// ==================
// ArchetypeRange - Range of pops belonging to the same archetype (contigous in PopData arrays)
// ==================
struct ArchetypeRange {
	size_t start_index;
	size_t count;

	size_t end_index() const {
		return start_index + count;
	}
};

