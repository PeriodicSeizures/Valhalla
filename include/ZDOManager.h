#pragma once

#include <vector>
#include <functional>

#include "Vector.h"
#include "ZDO.h"
#include "Peer.h"
#include "PrefabManager.h"
#include "ZoneManager.h"

class IZDOManager {
	friend class INetManager;
	friend class IValhalla;
	friend class ZDO;
		
	//static constexpr int WIDTH_IN_ZONES = 512; // The width of world in zones (the actual world is smaller than this at 315)
	static constexpr int MAX_DEAD_OBJECTS = 100000;

	static bool PREFAB_CHECK_FUNCTION(const ZDO& zdo, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		auto&& prefab = zdo.GetPrefab();

		return prefab.AllFlagsAbsent(flagsAbsent)
			&& (prefabHash == 0 || prefab.m_hash == prefabHash)
			&& prefab.AllFlagsPresent(flagsPresent);
	}

private:
	// Increments over the course of the game as ZDOs are created
	uint32_t m_nextUid = 1;

	// Responsible for managing ZDOs lifetimes
	UNORDERED_MAP_t<ZDOID, std::unique_ptr<ZDO>> m_objectsByID;

	// Contains ZDOs according to Zone
	std::array<UNORDERED_SET_t<ZDO*>,
		(IZoneManager::WORLD_RADIUS_IN_ZONES * IZoneManager::WORLD_RADIUS_IN_ZONES * 2 * 2)> m_objectsBySector; // takes up around 5MB; could be around 72 bytes with map

	// Contains ZDOs according to prefab
	UNORDERED_MAP_t<HASH_t, UNORDERED_SET_t<ZDO*>> m_objectsByPrefab;

	// Primarily used in RPC_ZDOData
	//robin_hood::unordered_map<ZDOID, TICKS_t> m_erasedZDOs;
	UNORDERED_SET_t<ZDOID> m_erasedZDOs;

	// Contains recently destroyed ZDOs to be sent
	std::vector<ZDOID> m_destroySendList;

	BYTES_t m_temp;

	//static const std::function<bool(const ZDO&, HASH_t, Prefab::FLAG_t, Prefab::FLAG_t)> PREFAB_FUNCTION;

private:
	// Called when an authenticated peer joins (internal)
	void OnNewPeer(Peer& peer);
	// Called when an authenticated peer leaves (internal)
	void OnPeerQuit(Peer& peer);

	// Insert a ZDO into zone (internal)
	bool AddZDOToZone(ZDO& zdo);
	// Remove a zdo from a zone (internal)
	void RemoveFromSector(ZDO& zdo);
	// Relay a ZDO sector change to clients (internal)
	void InvalidateZDOZone(ZDO& zdo);

	void AssignOrReleaseZDOs(Peer& peer);
	//void SmartAssignZDOs();

	decltype(m_objectsByID)::iterator DestroyZDO(decltype(m_objectsByID)::iterator itr);

	// Performs an unchecked erasure
	//	Does not check whether the iterator is at end of container
	decltype(m_objectsByID)::iterator EraseZDO(decltype(m_objectsByID)::iterator itr);
	void EraseZDO(ZDOID uid);
	void SendAllZDOs(Peer& peer);
	bool SendZDOs(Peer& peer, bool flush);
	std::list<std::pair<std::reference_wrapper<ZDO>, float>> CreateSyncList(Peer& peer);

	ZDO& Instantiate(Vector3f position);
	ZDO& Instantiate(ZDOID uid, Vector3f position);
		
	// Get a ZDO by id
	//	The ZDO will be created if its ID does not exist
	//	Returns the ZDO and a bool if newly created
	std::pair<decltype(IZDOManager::m_objectsByID)::iterator, bool> GetOrInstantiate(ZDOID id, Vector3f def);

	// Performs a coordinate to pitch conversion
	int SectorToIndex(ZoneID zone) const;

public:
	void Init();
	void Update();

	// Used when saving the world from disk
	void Save(DataWriter& writer);

	// Used when loading the world from disk
	void Load(DataReader& reader, int version);

	ZDO& Instantiate(const Prefab& prefab, Vector3f pos, Quaternion rot);
	ZDO& Instantiate(const Prefab& prefab, Vector3f pos) { return Instantiate(prefab, pos, Quaternion::IDENTITY); }
	
	ZDO& Instantiate(HASH_t hash, Vector3f pos, Quaternion rot, const Prefab** outPrefab);
	ZDO& Instantiate(HASH_t hash, Vector3f pos, Quaternion rot) { return Instantiate(hash, pos, rot, nullptr); }
	ZDO& Instantiate(HASH_t hash, Vector3f pos) { return Instantiate(hash, pos, Quaternion::IDENTITY, nullptr);  }
	// Clones a ZDO
	ZDO& Instantiate(const ZDO& zdo);

	// Get a ZDO by id
	//	TODO use optional<reference>
	ZDO* GetZDO(ZDOID id);

	// Get all ZDOs strictly within a zone
	void GetZDOs_Zone(ZoneID zone, std::list<std::reference_wrapper<ZDO>>& out);
	// Get all ZDOs strictly within neighboring zones
	void GetZDOs_NeighborZones(ZoneID zone, std::list<std::reference_wrapper<ZDO>>& out);
	// Get all ZDOs strictly within distant zones
	void GetZDOs_DistantZones(ZoneID zone, std::list<std::reference_wrapper<ZDO>>& out);
	// Get all ZDOs strictly within a zone, its neighboring zones, and its distant zones
	void GetZDOs_ActiveZones(ZoneID zone, std::list<std::reference_wrapper<ZDO>>& out, std::list<std::reference_wrapper<ZDO>>& outDistant);
	// Get all ZDOs strictly within a zone that are distant flagged
	void GetZDOs_Distant(ZoneID sector, std::list<std::reference_wrapper<ZDO>>& objects);
	

	// Get a capped number of ZDOs within a radius matching an optional predicate
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(Vector3f pos, float radius, size_t max, const std::function<bool(const ZDO&)>& pred);
	// Get a capped number of ZDOs within a radius
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(Vector3f pos, float radius, size_t max) {
		return SomeZDOs(pos, radius, max, nullptr);
	}
	// Get a capped number of ZDOs with prefab and/or flag
	//	*Note: Prefab or Flag must be non-zero for anything to be returned
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(Vector3f pos, float radius, size_t max, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(pos, radius, max, 
			[&](const ZDO& zdo) {
				return PREFAB_CHECK_FUNCTION(zdo, prefab, flagsPresent, flagsAbsent);
			}
		);
	}


	// Get a capped number of ZDOs within a zone matching an optional predicate
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(ZoneID zone, size_t max, const std::function<bool(const ZDO&)>& pred);
	// Get a capped number of ZDOs within a zone
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(ZoneID zone, size_t max) {
		return SomeZDOs(zone, max, nullptr);
	}
	// Get a capped number of ZDOs within a radius in zone with prefab and/or flag
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(ZoneID zone, size_t max, Vector3f pos, float radius, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		const auto sqRadius = radius * radius;
		return SomeZDOs(zone, max, [&](const ZDO& zdo) {
			return zdo.Position().SqDistance(pos) <= sqRadius
				&& PREFAB_CHECK_FUNCTION(zdo, prefab, flagsPresent, flagsAbsent);
		});
	}
	// Get a capped number of ZDOs within a zone with prefab and/or flag
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(ZoneID zone, size_t max, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(zone, max, Vector3f::Zero(), std::numeric_limits<float>::max(), prefab, flagsPresent, flagsAbsent);
	}
	// Get a capped number of ZDOs within a zone with prefab and/or flag
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(ZoneID zone, size_t max, Vector3f pos, float radius) {
		return SomeZDOs(zone, max, pos, radius, 0, Prefab::Flag::NONE, Prefab::Flag::NONE);
	}


	// Get all ZDOs with prefab
	std::list<std::reference_wrapper<ZDO>> GetZDOs(HASH_t prefab);


	// Get all ZDOs within a radius matching an optional predicate
	std::list<std::reference_wrapper<ZDO>> GetZDOs(Vector3f pos, float radius, const std::function<bool(const ZDO&)>& pred) {
		return SomeZDOs(pos, radius, -1, pred);
	}
	// Get all ZDOs within a radius
	std::list<std::reference_wrapper<ZDO>> GetZDOs(Vector3f pos, float radius) {
		return SomeZDOs(pos, radius, -1, nullptr);
	}
	// Get all ZDOs within a radius with prefab and/or flag
	std::list<std::reference_wrapper<ZDO>> GetZDOs(Vector3f pos, float radius, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(pos, radius, -1, prefab, flagsPresent, flagsAbsent);
	}


	// Get all ZDOs within a zone matching an optional predicate
	std::list<std::reference_wrapper<ZDO>> GetZDOs(ZoneID zone, const std::function<bool(const ZDO&)>& pred) {
		return SomeZDOs(zone, -1, pred);
	}
	// Get all ZDOs within a zone
	std::list<std::reference_wrapper<ZDO>> GetZDOs(ZoneID zone) {
		return SomeZDOs(zone, -1, nullptr);
	}
	// Get all ZDOs within a zone of prefab and/or flag
	std::list<std::reference_wrapper<ZDO>> GetZDOs(ZoneID zone, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(zone, -1, [&](const ZDO& zdo) {
			return PREFAB_CHECK_FUNCTION(zdo, prefab, flagsPresent, flagsAbsent);
		});
	}
	// Get all ZDOs within a radius in zone
	std::list<std::reference_wrapper<ZDO>> GetZDOs(ZoneID zone, Vector3f pos, float radius, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		const auto sqRadius = radius * radius;
		return SomeZDOs(zone, -1, [&](const ZDO& zdo) {
			return zdo.Position().SqDistance(pos) <= sqRadius
				&& PREFAB_CHECK_FUNCTION(zdo, prefab, flagsPresent, flagsAbsent);
		});
	}
	// Get all ZDOs within a radius in zone
	std::list<std::reference_wrapper<ZDO>> GetZDOs(ZoneID zone, Vector3f pos, float radius) {
		return GetZDOs(zone, pos, radius, 0, Prefab::Flag::NONE, Prefab::Flag::NONE);
	}


	// Get any ZDO within a radius with prefab and/or flag
	ZDO* AnyZDO(Vector3f pos, float radius, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		auto&& zdos = SomeZDOs(pos, radius, 1, prefabHash, flagsPresent, flagsAbsent);
		if (zdos.empty())
			return nullptr;
		return &zdos.front().get();
	}
	// Get any ZDO within a zone with prefab and/or flag
	ZDO* AnyZDO(ZoneID zone, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		auto&& zdos = SomeZDOs(zone, 1, prefabHash, flagsPresent, flagsAbsent);
		if (zdos.empty())
			return nullptr;
		return &zdos.front().get();
	}


	// Get the nearest ZDO within a radius matching an optional predicate
	ZDO* NearestZDO(Vector3f pos, float radius, const std::function<bool(const ZDO&)>& pred);
	// Get the nearest ZDO within a radius with prefab and/or flag
	ZDO* NearestZDO(Vector3f pos, float radius, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return NearestZDO(pos, radius, [&](const ZDO& zdo) {
			return PREFAB_CHECK_FUNCTION(zdo, prefabHash, flagsPresent, flagsAbsent);
		});
	}



	void ForceSendZDO(ZDOID id);

	// Erases a ZDO on clients and server
	//	Warning: the target ZDO is freed from memory and will become no longer accessible
	//void DestroyZDO(ZDO& zdo);
	void DestroyZDO(ZDOID zdoid);
	void DestroyZDO(const ZDO& zdo) {
		DestroyZDO(zdo.ID());
	}

	size_t GetSumZDOMembers();
	float GetMeanZDOMembers();
	float GetStDevZDOMembers();
	size_t GetTotalZDOAlloc();

	size_t GetCountEmptyZDOs();
};

// Manager class for everything related to networked object synchronization
IZDOManager* ZDOManager();
