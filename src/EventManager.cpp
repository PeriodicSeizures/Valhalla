#include "EventManager.h"
#include "RouteManager.h"
#include "Hashes.h"
#include "NetManager.h"
#include "ZoneManager.h"
#include "GeoManager.h"
#include "VUtilsResource.h"
#include "ZDOManager.h"
#include "Prefab.h"

auto EVENT_MANAGER(std::make_unique<IEventManager>());
IEventManager *EventManager() {
	return EVENT_MANAGER.get();
}

void IEventManager::Init() {
	// interval: 46
	// chance: 20
	// range: 96

	LOG(INFO) << "Initializing EventManager";

	{
		// load Foliage:
		auto opt = VUtils::Resource::ReadFile<BYTES_t>("randomEvents.pkg");
		if (!opt)
			throw std::runtime_error("randomEvents.pkg missing");

		DataReader pkg(*opt);

		pkg.Read<std::string>(); // comment
		std::string ver = pkg.Read<std::string>();
		if (ver != VConstants::GAME)
			LOG(WARNING) << "randomEvents.pkg uses different game version than server (" << ver << ")";

		auto count = pkg.Read<int32_t>();
		for (int i = 0; i < count; i++) {
			auto e = std::make_unique<Event>();

			e->m_name = pkg.Read<std::string>();
			e->m_duration = pkg.Read<float>();
			e->m_nearBaseOnly = pkg.Read<bool>();
			e->m_pauseIfNoPlayerInArea = pkg.Read<bool>();
			e->m_biome = (Biome) pkg.Read<int32_t>();
			
			e->m_presentGlobalKeys = pkg.Read<robin_hood::unordered_set<std::string>>();
			e->m_absentGlobalKeys = pkg.Read<robin_hood::unordered_set<std::string>>();

			m_events.insert({ VUtils::String::GetStableHashCode(e->m_name), std::move(e) });
		}

		LOG(INFO) << "Loaded " << count << " random events";
	}
}

void IEventManager::Update() {
	m_eventIntervalTimer += Valhalla()->Delta();

	// update event timer if an event is active
	if (m_activeEvent) {
		// Update the timer of the current event
		if (!m_activeEvent->m_pauseIfNoPlayerInArea
			|| ZDOManager()->AnyZDO(this->m_activeEventPos, SERVER_SETTINGS.eventsRange, 0, Prefab::Flag::PLAYER, Prefab::Flag::NONE))
			m_activeEventTimer += Valhalla()->Delta();

		if (m_activeEventTimer > this->m_activeEvent->m_duration) {
			m_activeEvent = nullptr;
			m_activeEventPos = Vector3::ZERO;
		}
	}
	else if (SERVER_SETTINGS.eventsEnabled) {
		// try to set a new current event
		if (m_eventIntervalTimer > SERVER_SETTINGS.eventsInterval.count()) {
			m_eventIntervalTimer = 0;
			if (VUtils::Random::State().NextFloat() <= SERVER_SETTINGS.eventsChance) {

				if (auto opt = GetPossibleRandomEvent()) {
					auto&& e = opt.value().first;
					auto&& pos = opt.value().second;

					this->m_activeEvent = &e.get();
					this->m_activeEventPos = pos;
					this->m_activeEventTimer = 0;

					LOG(INFO) << "Set current random event: " << e.get().m_name;

					// send event
					//SendCurrentRandomEvent();
				}
			}
		}
	}

	PERIODIC_NOW(1s, { SendCurrentRandomEvent(); });
}

std::optional<std::pair<std::reference_wrapper<const IEventManager::Event>, Vector3>> IEventManager::GetPossibleRandomEvent() {
	std::vector<std::pair<std::reference_wrapper<const Event>, Vector3>> result;
	
	for (auto&& pair : this->m_events) {

		auto&& e = pair.second;
		if (CheckGlobalKeys(*e)) {

			std::vector<Vector3> positions;

			// now look for valid spaces
			for (auto&& peer : NetManager()->GetPeers()) {
				auto&& zdo = peer->GetZDO();
				if (!zdo) continue;

				if (
					// Check biome first
					(e->m_biome == Biome::None || (GeoManager()->GetBiome(zdo->Position()) & e->m_biome) != Biome::None)
					// check base next
					&& (!e->m_nearBaseOnly || zdo->GetInt(Hashes::ZDO::Player::BASE_VALUE) >= 3)
					// check that player is not in dungeon
					&& (zdo->Position().y < 3000.f)) 
				{
					//result.push_back({VUtils::Random::State().Range(0, )})
					positions.push_back(zdo->Position());
				}
			}

			if (!positions.empty())
				result.push_back({ *e, positions[VUtils::Random::State().Range(0, positions.size())] });

		}
	}

	if (!result.empty())
		return result[VUtils::Random::State().Range(0, result.size())];

	return std::nullopt;
}

bool IEventManager::CheckGlobalKeys(const Event& e) {
	for (auto&& key : e.m_presentGlobalKeys) {
		if (!ZoneManager()->GlobalKeys().contains(key))
			return false;
	}

	for (auto&& key : e.m_absentGlobalKeys) {
		if (ZoneManager()->GlobalKeys().contains(key))
			return false;
	}

	return true;
}

void IEventManager::Save(DataWriter& writer) {
	writer.Write(m_eventIntervalTimer);
	writer.Write(m_activeEvent ? m_activeEvent->m_name : "");
	writer.Write(m_activeEventTimer);
	writer.Write(m_activeEventPos);
}

void IEventManager::Load(DataReader& reader, int version) {
	m_eventIntervalTimer = reader.Read<float>();
	if (version >= 25) {
		this->m_activeEvent = GetEvent(reader.Read<std::string>());
		this->m_activeEventTimer = reader.Read<float>();
		this->m_activeEventPos = reader.Read<Vector3>();
	}
}

void IEventManager::SendCurrentRandomEvent() {
	if (m_activeEvent) {
		RouteManager()->InvokeAll(Hashes::Routed::SetEvent, 
			m_activeEvent->m_name,
			m_activeEventTimer,
			m_activeEventPos
		);
	}
	else {
		RouteManager()->InvokeAll(Hashes::Routed::SetEvent,
			"",
			0.f,
			Vector3::ZERO
		);
	}
}
