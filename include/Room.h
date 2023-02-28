#pragma once

#include "RoomConnection.h"

#include "VUtilsRandom.h"
#include "Quaternion.h"
#include "Prefab.h"

class Room {
	friend class IDungeonManager;

public:
	enum class Theme {
		Crypt = 1,
		SunkenCrypt = 2,
		Cave = 4,
		ForestCrypt = 8,
		GoblinCamp = 16,
		MeadowsVillage = 32,
		MeadowsFarm = 64,
		DvergerTown = 128,
		DvergerBoss = 256
	};

private:
	//static std::vector<RoomConnection*> tempConnections;

	std::vector<std::unique_ptr<RoomConnection>> m_roomConnections;

public:
	//Vector3Int m_size = new Vector3Int(8, 4, 8);
	Vector3 m_size = Vector3(8, 4, 8);

	Theme m_theme = Theme::Crypt;

	bool m_entrance;

	bool m_endCap;

	bool m_divider;

	int m_endCapPrio;

	int m_minPlaceOrder;

	float m_weight = 1;

	bool m_faceCenter;

	bool m_perimeter;

	std::string m_name; // custom (unity gameobject name of this Room)

	HASH_t m_hash; // based off name

	Vector3 m_pos;
	Quaternion m_rot;

	std::vector<Prefab::Instance> m_netViews;

	// TODO later...
	//std::vector<RandomSpawn>

	// TODO later...
	//public MusicVolume m_musicPrefab;

public:
	Room(const Room& other) = delete;

	HASH_t GetHash();

	std::vector<std::unique_ptr<RoomConnection>>& GetConnections() {
		return m_roomConnections;
	}

	// Nullable
	RoomConnection &GetConnection(VUtils::Random::State& state, const RoomConnection &other);

	RoomConnection &GetEntrance();

	bool HaveConnection(const RoomConnection &other);
};

struct RoomInstance {
	std::reference_wrapper<Room> m_room;
	Vector3 m_pos;
	Quaternion m_rot;
	int m_placeOrder = 0;
	int m_seed = 0;
	std::vector<RoomConnectionInstance> m_connections;

	RoomInstance(Room& room, Vector3 pos, Quaternion rot, int placeOrder, int seed) 
		: m_room(room), m_pos(pos), m_rot(rot), m_placeOrder(placeOrder),  m_seed(seed) {
		for (auto&& conn : room.GetConnections()) {
			// Find the world position of the connection, 
			//	given parent (Room) position and localPosition (Connection)
			//m_connections.emplace_back(conn, room.m_pos + conn.get()->m_localPos)

			// https://stackoverflow.com/questions/73652767/get-new-child-object-postion-based-on-parent-transform

			//Quaternion childWorldRot = rot * conn->m_localRot;
			//Vector3 pointOnRot = (childWorldRot * Vector3::FORWARD).Normalized() * conn->m_localPos.Magnitude();
			//
			//Vector3 childWorldPos = pointOnRot + room.m_pos;
			
			//m_connections.emplace_back(*conn.get(), childWorldPos, childWorldRot, placeOrder);

			auto global = VUtils::Physics::LocalToGlobal(conn->m_localPos, conn->m_localRot,
				pos, rot);

			m_connections.emplace_back(*conn.get(), global.first, global.second, placeOrder);
		}
	}
};
