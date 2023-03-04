#pragma once

#include <vector>

#include "Vector.h"
#include "ZDO.h"
#include "Peer.h"
#include "PrefabManager.h"
#include "ZoneManager.h"

class IZDOManager {
	friend class IPrefabManager;
	friend class INetManager;
	friend class ZDO;

	//friend void AddToSector(ZDO* zdo);
	//friend void RemoveFromSector(ZDO* zdo);
	
	//static constexpr int WIDTH_IN_ZONES = 512; // The width of world in zones (the actual world is smaller than this at 315)
	static constexpr int MAX_DEAD_OBJECTS = 100000;

private:
	// Increments over the course of the game as ZDOs are created
	uint32_t m_nextUid = 1;

	// Responsible for managing ZDOs lifetimes
	robin_hood::unordered_map<ZDOID, std::unique_ptr<ZDO>> m_objectsByID;

	// Contains ZDOs according to Zone
	std::array<robin_hood::unordered_set<ZDO*>, 
		(IZoneManager::WORLD_RADIUS_IN_ZONES * IZoneManager::WORLD_RADIUS_IN_ZONES * 2 * 2)> m_objectsBySector; // takes up around 5MB; could be around 72 bytes with map

	// Contains ZDOs according to prefab
	robin_hood::unordered_map<HASH_t, robin_hood::unordered_set<ZDO*>> m_objectsByPrefab;

	// Primarily used in RPC_ZDOData
	robin_hood::unordered_map<ZDOID, TICKS_t> m_deadZDOs;

	// Contains recently destroyed ZDOs to be sent
	std::vector<ZDOID> m_destroySendList;

private:
	// Called when an authenticated peer joins (internal)
	void OnNewPeer(Peer& peer);
	// Called when an authenticated peer leaves (internal)
	void OnPeerQuit(Peer& peer);

	// Insert a ZDO into zone (internal)
	bool AddToSector(ZDO& zdo);
	// Remove a zdo from a zone (internal)
	void RemoveFromSector(ZDO& zdo);
	// Relay a ZDO sector change to clients (internal)
	void InvalidateSector(ZDO& zdo);

	void AssignOrReleaseZDOs(Peer& peer);
	//void SmartAssignZDOs();

	void EraseZDO(const ZDOID& uid);
	void SendAllZDOs(Peer& peer);
	bool SendZDOs(Peer& peer, bool flush);
	std::list<std::reference_wrapper<ZDO>> CreateSyncList(Peer& peer);

	ZDO& AddZDO(const Vector3& position);
	ZDO& AddZDO(const ZDOID& uid, const Vector3& position);
		
	// Performs a coordinate to pitch conversion
	int SectorToIndex(const ZoneID& zone) const;

public:
	void Init();
	void Update();

	// Used when saving the world from disk
	void Save(DataWriter& writer);

	// Used when loading the world from disk
	void Load(DataReader& reader, int version);

	// Get a ZDO by id
	//	The ZDO will be created if its ID does not exist
	//	Returns the ZDO and a bool if newly created
	std::pair<ZDO&, bool> GetOrCreateZDO(const ZDOID& id, const Vector3& def);

	// Get a ZDO by id
	//	TODO use optional<reference>
	ZDO* GetZDO(const ZDOID& id);

	// Get all ZDOs strictly within a zone
	void GetZDOs_Zone(const ZoneID& zone, std::list<std::reference_wrapper<ZDO>>& out);
	// Get all ZDOs strictly within neighboring zones
	void GetZDOs_NeighborZones(const ZoneID& zone, std::list<std::reference_wrapper<ZDO>>& out);
	// Get all ZDOs strictly within distant zones
	void GetZDOs_DistantZones(const ZoneID& zone, std::list<std::reference_wrapper<ZDO>>& out);
	// Get all ZDOs strictly within a zone, its neighboring zones, and its distant zones
	void GetZDOs_ActiveZones(const ZoneID& zone, std::list<std::reference_wrapper<ZDO>>& out, std::list<std::reference_wrapper<ZDO>>& outDistant);

	// Get all ZDOs strictly within a zone that are distant flagged
	void GetZDOs_Distant(const ZoneID& sector, std::list<std::reference_wrapper<ZDO>>& objects);

	// Get all ZDOs strictly by prefab
	std::list<std::reference_wrapper<ZDO>> GetZDOs_Prefab(HASH_t prefabHash);



	// Get all ZDOs strictly within a radius based on a condition
	std::list<std::reference_wrapper<ZDO>> GetZDOs(const Vector3& pos, float radius, std::function<bool(const ZDO&)> cond);

	// Get all ZDOs strictly within a radius
	std::list<std::reference_wrapper<ZDO>> GetZDOs(const Vector3& pos, float radius) {
		return GetZDOs(pos, radius, std::function<bool(const ZDO&)>());
	}
		
	// Get all ZDOs strictly within a radius by prefab
	std::list<std::reference_wrapper<ZDO>> GetZDOs(const Vector3& pos, float radius, const Prefab& prefab) {
		return GetZDOs(pos, radius, std::function<bool(const ZDO&)>([=](const ZDO& zdo) {
			return *zdo.m_prefab == prefab;
		}));
	}

	std::list<std::reference_wrapper<ZDO>> GetZDOs(const Vector3& pos, float radius, Prefab::Flag flag) {
		return GetZDOs(pos, radius, std::function<bool(const ZDO&)>([=](const ZDO& zdo) {
			return zdo.m_prefab->HasFlag(flag);
		}));
	}



	ZDO* AnyZDO_PrefabRadius(const Vector3& pos, float radius, HASH_t prefabHash);



	void ForceSendZDO(const ZDOID& id);
	void DestroyZDO(ZDO& zdo, bool immediate);
};

IZDOManager* ZDOManager();
