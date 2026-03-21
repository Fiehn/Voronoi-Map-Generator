#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include "flecs.h"

enum class ActorType : uint8_t {
	Entity, // flecs::entity (religion, language, culture, deity, etc)
	Cell,  // size_t (cell id)
	Pop, // int64_t (population id) (SoA index)
	None
};

struct HistoricalActor {
	ActorType type = ActorType::None;
	uint64_t id = 0; // flecs::entity id, cell id, or pop id

	// Consturctors for different actor types
	HistoricalActor() : type(ActorType::None), id(0) {}
	HistoricalActor(flecs::entity entity) : type(ActorType::Entity), id(static_cast<uint64_t>(entity.id())) {}
	HistoricalActor(size_t cell_id) : type(ActorType::Cell), id(static_cast<uint64_t>(cell_id)) {}
	HistoricalActor(int64_t pop_id) : type(ActorType::Pop), id(static_cast<uint64_t>(pop_id)) {}

	bool operator==(const HistoricalActor& other) const {
		return type == other.type && id == other.id;
	}
	bool operator!=(const HistoricalActor& other) const {
		return !(*this == other);
	}
};

// Expand when more event types are needed
enum class EventType : uint16_t {
	Famine,
	War,
	DeityCreation,
	DeityInteraction,
	DivineConflict,
	Syncretism,
	Schism,
	Reformation,
	None
};

struct EventSnapshot {
	std::string subject_name;
	std::string object_name;
	std::string location_name;

	std::string reason; // eg. "Axiom: Artificial is an affront to the natural order"
	EventSnapshot() : subject_name(""), object_name(""), location_name(""), reason("") {}
	EventSnapshot(const std::string& subject, const std::string& object,
		const std::string& location, const std::string& reason)
		: subject_name(subject), object_name(object), location_name(location), reason(reason) {
	}

};

struct HistoricalEvent {
	// === Identity ===
	uint64_t event_id;
	uint32_t tick;
	EventType type;

	// === The Actors ===
	HistoricalActor subject; // Who did it?
	HistoricalActor object;  // To whom/what?

	// === Context snapshot ===
	EventSnapshot snapshot;

	// === Causality (optional) ===
	// Points to the event that caused this event to trigger
	// eg. Famine (Event A) -> Scapegoating (Event B) -> Schism (Event C)
	std::optional<uint64_t> caused_by_event_id;

	HistoricalEvent() = default;

	HistoricalEvent(uint64_t event_id, uint32_t tick, EventType type,
		HistoricalActor subject, HistoricalActor object,
		EventSnapshot snapshot, std::optional<uint64_t> caused_by_event_id = std::nullopt)
		: event_id(event_id), tick(tick), type(type),
		subject(subject), object(object),
		snapshot(snapshot), caused_by_event_id(caused_by_event_id) {
	}
};


struct History {
	std::vector<HistoricalEvent> events;

	History() {
		events.reserve(10000); // Preallocate space for 10000 events
	}

	void log(const HistoricalEvent& event) {
		events.push_back(event);
	}

	// Accept by rvalue ref for move semantics when caller has a temporary
	void log(HistoricalEvent&& event) {
		events.push_back(std::move(event));
	}

	// Construct in-place for best performance, will automatically assign event_id.
	uint64_t log(uint32_t tick, EventType type,
		HistoricalActor subject, HistoricalActor object,
		EventSnapshot snapshot,
		std::optional<uint64_t> caused_by_event_id = std::nullopt)
	{
		uint64_t current_id = static_cast<uint64_t>(events.size());
		events.emplace_back(current_id, tick, type, subject, object, std::move(snapshot), caused_by_event_id);
		return current_id;
	}

	// Get event by index
	HistoricalEvent& get_event(size_t index) {
		return events[index];
	}
	const HistoricalEvent& get_event(size_t index) const {
		return events[index];
	}
	// Get event by actor
	std::vector<HistoricalEvent> get_events_by_actor(const HistoricalActor& actor) const {
		std::vector<HistoricalEvent> result;
		for (const auto& event : events) {
			if (event.subject == actor || event.object == actor) {
				result.push_back(event);
			}
		}
		return result;
	}
	std::vector<HistoricalEvent> get_events_by_type(EventType type) const {
		std::vector<HistoricalEvent> result;
		for (const auto& event : events) {
			if (event.type == type) {
				result.push_back(event);
			}
		}
		return result;
	}
	std::vector<HistoricalEvent> get_events_by_tick(uint32_t tick) const {
		std::vector<HistoricalEvent> result;
		for (const auto& event : events) {
			if (event.tick == tick) {
				result.push_back(event);
			}
		}
		return result;
	}
	std::vector<HistoricalEvent> get_events_involving_actor_of_type(ActorType type) const {
		std::vector<HistoricalEvent> result;
		for (const auto& event : events) {
			if (event.subject.type == type || event.object.type == type) {
				result.push_back(event);
			}
		}
		return result;
	}


	size_t size() const {
		return events.size();
	}
	void clear() {
		events.clear();
	}
	void resize(size_t new_size) {
		events.resize(new_size);
	}
	void reserve(size_t new_cap) {
		events.reserve(new_cap);
	}
	void initialize(size_t new_size) {
		events.clear();
		events.resize(new_size);
	}
};

