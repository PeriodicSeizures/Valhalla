#include "DungeonGenerator.h"

#include "ZoneManager.h"
#include "WorldManager.h"
#include "GeoManager.h"
#include "VUtilsMathf.h"
#include "VUtilsPhysics.h"
#include "ZDOManager.h"
#include "VUtilsMath2.h"

DungeonGenerator::DungeonGenerator(const Dungeon& dungeon, ZDO& zdo) :
	m_dungeon(dungeon), m_zdo(zdo), m_pos(zdo.Position()), m_rot(zdo.Rotation()) {

	auto zone = IZoneManager::WorldToZonePos(m_pos);
	auto seed = GeoManager()->GetSeed();
	//this->m_generatedSeed = seed + zone.x * 4271 + zone.y * -7187 + (int)m_pos.x * -4271 + (int)m_pos.y * 9187 + (int)m_pos.z * -2134;
	//this->m_generatedSeed = seed + (int)m_pos.x * -4271 + (int)m_pos.y * 9187 + (int)m_pos.z * -2134;

	this->m_zoneCenter = IZoneManager::ZoneToWorldPos(zone);
	this->m_zoneCenter.y = m_pos.y; // -this->m_dungeon.m_originalPosition.y;
}

// TODO generate seed during start
HASH_t DungeonGenerator::GetSeed() {
	if (VH_SETTINGS.dungeonsSeeded) {
		auto seed = GeoManager()->GetSeed();
		auto zone = IZoneManager::WorldToZonePos(m_pos);
		return seed + (int)m_pos.x * -4271 + (int)m_pos.y * 9187 + (int)m_pos.z * -2134;
	}
	else {
		return VUtils::Random::State().Range(INT_MIN, INT_MAX);
	}
}

void DungeonGenerator::Generate() {
	this->Generate(GetSeed());
}

void DungeonGenerator::DungeonGenerator::Generate(HASH_t seed) {
	VUtils::Random::State state(seed);

	this->GenerateRooms(state);
	this->Save();

	//this->m_generatedTime = steady_clock::now();

	LOG_INFO(LOGGER, "Finished generating dungeon: '{}', pos: {}, seed: {}, rooms: {}/{}", 
		m_dungeon.m_prefab->m_name, m_pos, seed, m_placedRooms.size(), m_dungeon.m_maxRooms);
}

//
//void DungeonGenerator::Regenerate(const ZoneID& zone) {
//	// Find the dungeon in that zone
//	//ZDOManager()->AnyZDO(zone).
//	//zdo
//}

//void DungeonGenerator::Regenerate(const ZDO& zdo) {
//	// Find the dungeon in that zone
//	//ZDOManager()->AnyZDO(zone).
//	if (!zdo.GetPrefab()->FlagsPresent(Prefab::FLAG_t::Dungeon))
//		throw std::runtime_error("not a dungeon");
//
//
//}





void DungeonGenerator::GenerateRooms(VUtils::Random::State& state) {
	switch (this->m_dungeon.m_algorithm) {
	case Dungeon::Algorithm::Dungeon:
		this->GenerateDungeon(state);
		break;
	case Dungeon::Algorithm::CampGrid:
		this->GenerateCampGrid(state);
		break;
	case Dungeon::Algorithm::CampRadial:
		this->GenerateCampRadial(state);
		break;
	}
}

void DungeonGenerator::GenerateDungeon(VUtils::Random::State& state) {
	this->PlaceStartRoom(state);
	this->PlaceRooms(state);

	if (VH_SETTINGS.dungeonsEndcapsEnabled)
		this->PlaceEndCaps(state);

	if (VH_SETTINGS.dungeonsDoors)
		this->PlaceDoors(state);
}

void DungeonGenerator::GenerateCampGrid(VUtils::Random::State& state) {
	float num = std::cos(0.017453292f * this->m_dungeon.m_maxTilt);
	Vector3f a = this->m_pos + Vector3f((float)(-this->m_dungeon.m_gridSize) * this->m_dungeon.m_tileWidth * 0.5f,
		0.f,
		(float)(-this->m_dungeon.m_gridSize) * this->m_dungeon.m_tileWidth * 0.5f);

	for (int i = 0; i < this->m_dungeon.m_gridSize; i++)
	{
		for (int j = 0; j < this->m_dungeon.m_gridSize; j++)
		{
			if (state.Value() <= this->m_dungeon.m_spawnChance) {
				Vector3f pos = a + Vector3f((float)j * this->m_dungeon.m_tileWidth, 0.f, (float)i * this->m_dungeon.m_tileWidth);
				auto randomWeightedRoom = this->GetRandomWeightedRoom(state, false);
				if (randomWeightedRoom)
				{
					Vector3f vector;
					Biome biome;
					BiomeArea biomeArea;
					ZoneManager()->GetGroundData(pos, vector, biome, biomeArea);
					if (vector.y < num)
						continue;

					Quaternion rot = Quaternion::Euler(0, 22.5f * state.Range(0, 16), 0.f);
					this->PlaceRoom(*randomWeightedRoom, pos, rot);
				}
			}
		}
	}
}

void DungeonGenerator::GenerateCampRadial(VUtils::Random::State& state) {
	float num = state.Range(this->m_dungeon.m_campRadiusMin, this->m_dungeon.m_campRadiusMax);
	float num2 = std::cos(0.017453292f * this->m_dungeon.m_maxTilt);
	int num3 = state.Range(this->m_dungeon.m_minRooms, this->m_dungeon.m_maxRooms);
	int num4 = num3 * 20;
	int num5 = 0;
	for (int i = 0; i < num4; i++) {
		Vector3f vector = this->m_pos
			+ Quaternion::Euler(0.f, state.Range(0, 360), 0.f)
			* Vector3f::Forward() * state.Range(0.f, num - this->m_dungeon.m_perimeterBuffer);

		auto randomWeightedRoom = this->GetRandomWeightedRoom(state, false);
		if (randomWeightedRoom) {
			Vector3f vector2;
			Biome biome;
			BiomeArea biomeArea;
			ZoneManager()->GetGroundData(vector, vector2, biome, biomeArea);
			if (vector2.y < num2 || vector.y - IZoneManager::WATER_LEVEL < this->m_dungeon.m_minAltitude)
				continue;

			Quaternion campRoomRotation = this->GetCampRoomRotation(state, *randomWeightedRoom, vector);
			if (!this->TestCollision(*randomWeightedRoom, vector, campRoomRotation))
			{
				this->PlaceRoom(*randomWeightedRoom, vector, campRoomRotation);
				num5++;
				if (num5 >= num3)
					break;
			}
		}
	}

	if (this->m_dungeon.m_perimeterSections > 0)
		this->PlaceWall(state, num, this->m_dungeon.m_perimeterSections);
}

Quaternion DungeonGenerator::GetCampRoomRotation(VUtils::Random::State& state, const Room& room, Vector3f pos) {
	if (room.m_faceCenter) {
		Vector3f vector = m_pos - pos;
		vector.y = 0;
		if (vector == Vector3f::Zero())
			vector = Vector3f::Forward();

		vector.Normal();
		float y = VUtils::Mathf::Round(VUtils::Math::YawFromDirection(vector) / 22.5f) * 22.5f;
		return Quaternion::Euler(0, y, 0);
	}

	return Quaternion::Euler(0, 22.5f * state.Range(0, 16), 0);
}

void DungeonGenerator::PlaceWall(VUtils::Random::State& state, float radius, int sections) {
	float num = std::cos(0.017453292f * this->m_dungeon.m_maxTilt);
	int num2 = 0;
	int num3 = sections * 20;
	for (int i = 0; i < num3; i++)
	{
		auto&& randomWeightedRoom = this->GetRandomWeightedRoom(state, true);
		if (randomWeightedRoom)
		{
			Vector3f vector = this->m_pos
				+ Quaternion::Euler(0, state.Range(0, 360), 0) * Vector3f::Forward() * radius;

			Quaternion campRoomRotation = this->GetCampRoomRotation(state, *randomWeightedRoom, vector);

			Vector3f vector2;
			Biome biome;
			BiomeArea biomeArea;
			ZoneManager()->GetGroundData(vector, vector2, biome, biomeArea);
			if (vector2.y < num || vector.y - IZoneManager::WATER_LEVEL < this->m_dungeon.m_minAltitude)
				continue;

			if (!this->TestCollision(*randomWeightedRoom, vector, campRoomRotation))
			{
				this->PlaceRoom(*randomWeightedRoom, vector, campRoomRotation);
				num2++;
				if (num2 >= sections) {
					break;
				}
			}
		}
	}
}



void DungeonGenerator::Save() {
	m_zdo.Set("rooms", (int32_t)m_placedRooms.size());
	for (int i = 0; i < m_placedRooms.size(); i++) {
		auto&& instance = m_placedRooms[i];
		auto&& room = instance->m_room.get();

		std::string text = "room" + std::to_string(i);
		m_zdo.Set(text, room.GetHash());

		Vector3f pos = instance->m_pos;
		Quaternion rot = instance->m_rot;

		if (m_dungeon.m_algorithm == Dungeon::Algorithm::Dungeon)
			std::tie(pos, rot) = VUtils::Physics::LocalToGlobal(instance->m_pos, instance->m_rot, this->m_pos, this->m_rot);

		m_zdo.Set(text + "_pos", pos);
		m_zdo.Set(text + "_rot", rot);
		m_zdo.Set(text + "_seed", instance->m_seed); // TODO seed useless?
	}
}



const Dungeon::DoorDef* DungeonGenerator::FindDoorType(VUtils::Random::State& state, std::string_view type) {
	std::vector<std::reference_wrapper<const Dungeon::DoorDef>> list;
	for (auto&& doorDef : this->m_dungeon.m_doorTypes) {
		if (doorDef.m_connectionType == type) {
			list.push_back(doorDef);
		}
	}

	// This case is possible with 'dvergropen' (Mistlands)
	if (list.empty())
		return nullptr;

	return &list[state.Range(0, list.size())].get();
}

void DungeonGenerator::PlaceDoors(VUtils::Random::State& state) {
	int num = 0;
	for (auto&& roomConnection : m_doorConnections) {
		auto&& doorDef = this->FindDoorType(state, roomConnection.get().m_connection.get().m_type);
		if (!doorDef)
		{
			LOG_INFO(LOGGER, "No door type for connection: {}", roomConnection.get().m_connection.get().m_type);
		}
		else if ((doorDef->m_chance <= 0 || state.Value() <= doorDef->m_chance)
			&& (doorDef->m_chance > 0 || state.Value() <= this->m_dungeon.m_doorChance))
		{
			auto global = VUtils::Physics::LocalToGlobal(roomConnection.get().m_pos, roomConnection.get().m_rot,
				this->m_pos, this->m_rot);

			ZDOManager()->Instantiate(*doorDef->m_prefab, global.first, global.second);
			num++;
		}
	}

	LOG_INFO(LOGGER, "Placed {} doors", num);
}

void DungeonGenerator::PlaceEndCaps(VUtils::Random::State& state) {
	//for (int i = 0; i < m_openConnections.size(); i++) {
	for (auto&& itr1 = m_openConnections.begin(); itr1 != m_openConnections.end(); ) {
		//auto&& roomConnection = m_openConnections[i];
		auto&& roomConnection = *itr1;
		const RoomConnectionInstance* roomConnection2 = nullptr;

		// Find the other matching connection
		//for (int j = 0; j < m_openConnections.size(); j++) {
		for (auto&& itr2 = m_openConnections.begin(); itr2 != m_openConnections.end(); ++itr2) {
			//if (j != i && roomConnection.get().TestContact(m_openConnections[j])) {
			if (itr2 != itr1 && roomConnection.get().TestContact(*itr2)) {
				roomConnection2 = &itr2->get();
				break;
			}
		}

		if (roomConnection2) {
			if (roomConnection.get().m_connection.get().m_type != roomConnection2->m_connection.get().m_type)
			{
				auto&& tempRooms = this->FindDividers(state);
				if (!tempRooms.empty())
				{
					auto&& weightedRoom = this->GetWeightedRoom(state, tempRooms);
					auto&& connections = weightedRoom.GetConnections();

					Vector3f vector;
					Quaternion rot;
					this->CalculateRoomPosRot(*connections[0], roomConnection.get().m_pos,
						roomConnection.get().m_rot, vector, rot);

					bool flag = false;
					for (auto&& room : m_placedRooms) {
						if (room->m_room.get().m_divider && room->m_pos.SqDistance(vector) < 0.5f * 0.5f) {
							flag = true;
							break;
						}
					}

					if (!flag) {
						LOG_WARNING(LOGGER, "Cyclic detected: Door mismatch for cyclic room");
					}
				}
				else {
					LOG_WARNING(LOGGER, "Cyclic detected: Door mismatch for cyclic room");
				}
			}
			else {
				LOG_INFO(LOGGER, "Cyclic detected: Door types successfully match");
			}

			++itr1;
		}
		else
		{
			auto&& tempRooms = this->FindEndCaps(state, roomConnection.get().m_connection);
			bool flag2 = false;

			bool erased = false;

			if (!flag2 && this->m_dungeon.m_alternativeFunctionality) {
				for (int k = 0; k < 5; k++)
				{
					auto&& weightedRoom2 = this->GetWeightedRoom(state, tempRooms);
					if (this->PlaceRoom(state, itr1, weightedRoom2, &erased))
					{
						flag2 = true;
						break;
					}
				}
			}

			if (!flag2) {
				// std::stable_sort is used because equal element value order are maintained
				std::stable_sort(tempRooms.begin(), tempRooms.end(), [](const std::reference_wrapper<const Room>& a, const std::reference_wrapper<const Room>& b) {
					return a.get().m_endCapPrio > b.get().m_endCapPrio;
				});

				for (auto&& roomData : tempRooms) {
					if (this->PlaceRoom(state, itr1, roomData, &erased)) {
						flag2 = true;
						break;
					}
				}
			}

			if (!flag2) {
				LOG_WARNING(LOGGER, "Failed to place end cap");
			}

			if (!erased) {
				++itr1;
			}
		}
	}
}

std::vector<std::reference_wrapper<const Room>> DungeonGenerator::FindDividers(VUtils::Random::State& state) {
	std::vector<std::reference_wrapper<const Room>> rooms;

	for (auto&& roomData : m_dungeon.m_availableRooms) {
		if (roomData->m_divider)
			rooms.push_back(*roomData);
	}

	auto i = rooms.size();
	while (i > 1) {
		i--;
		int index = state.Range(0, i);
		auto&& value = rooms[index];
		rooms[index] = rooms[i];
		rooms[i] = value;
	}

	return rooms;
}

std::vector<std::reference_wrapper<const Room>> DungeonGenerator::FindEndCaps(VUtils::Random::State& state, const RoomConnection& connection) {
	std::vector<std::reference_wrapper<const Room>> rooms;

	for (auto&& roomData : m_dungeon.m_availableRooms) {
		if (roomData->m_endCap && roomData->HaveConnection(connection))
			rooms.push_back(*roomData);
	}

	// Inlined .Shuffle
	auto i = rooms.size();
	while (i > 1) {
		i--;
		int index = state.Range(0, i);
		auto&& value = rooms[index];
		rooms[index] = rooms[i];
		rooms[i] = value;
	}

	return rooms;
}

void DungeonGenerator::PlaceRooms(VUtils::Random::State& state) {
	for (int i = 0; i < this->m_dungeon.m_maxRooms; i++)
	{
		this->PlaceOneRoom(state);
		if (this->CheckRequiredRooms()
			&& m_placedRooms.size() > this->m_dungeon.m_minRooms)
		{
			LOG_INFO(LOGGER, "All required rooms have been placed, stopping generation");
			return;
		}
	}
}

void DungeonGenerator::PlaceStartRoom(VUtils::Random::State& state) {
	auto&& roomData = this->FindStartRoom(state);
	auto&& entrance = roomData.GetEntrance();

	Vector3f pos;
	Quaternion rot;
	this->CalculateRoomPosRot(entrance,
		Vector3f::Zero(), Quaternion::IDENTITY,
		pos, rot
	);

	// TODO room.POS and room.ROT are ultimately redundant
	auto global = VUtils::Physics::LocalToGlobal(entrance.m_localPos, entrance.m_localRot,
		roomData.m_pos, roomData.m_rot);

	RoomConnectionInstance dummy = RoomConnectionInstance(entrance, global.first, global.second, 0);
	
	{
		Vector3f size = rot * roomData.m_size;

		size.x = std::abs(size.x);
		size.z = std::abs(size.z);

		size *= .5f;

		//LOG(INFO) << "start: " 
		//	<< "polygon(("
		//	<< pos.x - size.x << "," << pos.z - size.z << "),("
		//	<< pos.x - size.x << "," << pos.z + size.z << "),("
		//	<< pos.x + size.x << "," << pos.z + size.z << "),("
		//	<< pos.x + size.x << "," << pos.z - size.z << "))";
	}

	this->PlaceRoom(roomData, pos, rot, dummy);
}

bool DungeonGenerator::PlaceOneRoom(VUtils::Random::State& state) {

	// Get a random attachment point for the next room
	auto&& itr = this->GetOpenConnection(state);
	if (itr == m_openConnections.end())
		return false;

	auto&& openConnection = itr->get();

	for (int i = 0; i < 10; i++)
	{
		// Get a new random room to attach to the existing open instanced connect point
		const Room* roomData = this->m_dungeon.m_alternativeFunctionality
			? this->GetRandomWeightedRoom(state, &openConnection)
			: this->GetRandomRoom(state, &openConnection);
		if (!roomData)
			break;

		if (this->PlaceRoom(state, itr, *roomData, nullptr))
			return true;
	}
	return false;
}

void DungeonGenerator::CalculateRoomPosRot(const RoomConnection& roomCon, Vector3f pos, Quaternion rot, Vector3f& outPos, Quaternion& outRot) {
	outRot = rot * Quaternion::Inverse(roomCon.m_localRot);
	outPos = pos - outRot * roomCon.m_localPos;
}

bool DungeonGenerator::PlaceRoom(VUtils::Random::State& state, decltype(m_openConnections)::iterator& itr, const Room& room, bool* outErased) {

	auto&& connection = itr->get();

	auto&& connection2 = room.GetConnection(state, connection.m_connection);

	if (outErased)
		*outErased = false;

	Vector3f pos;
	Quaternion rot;
	this->CalculateRoomPosRot(connection2,
		connection.m_pos, connection.m_rot * (VH_SETTINGS.dungeonsRoomsFlipped ? Quaternion::Euler(0, 180, 0) : Quaternion::IDENTITY),
		pos, rot);

	// this is making me want to rip my hair out
	// https://www.desmos.com/calculator/hykg8ckp3i

	//pos += connection2.m_localPos;

	//pos += connection.m_connection.get().m_localPos;
	//pos += {0, 0, 1};
	//rot = Quaternion::IDENTITY;

	if (room.m_size.x != 0 && room.m_size.z != 0 && this->TestCollision(room, pos, rot)) {
		return false;
	}

	this->PlaceRoom(room, pos, rot, connection);
	if (!room.m_endCap) {
		if (connection.m_connection.get().m_allowDoor
			&& (!connection.m_connection.get().m_doorOnlyIfOtherAlsoAllowsDoor || connection2.m_allowDoor))
		{
			m_doorConnections.push_back(connection);
		}

		itr = m_openConnections.erase(itr);

		if (outErased) *outErased = true;
	}

	return true;
}


void DungeonGenerator::PlaceRoom(const Room& room, Vector3f pos, Quaternion rot) {
	// TODO seed is only useful for RandomSpawn
	int seed = (int)pos.x * 4271 + (int)pos.y * 9187 + (int)pos.z * 2134;

	//VUtils::Random::State state(seed);
	//for (auto&& randomSpawn : room.m_randomSpawns)
	//	randomSpawn.Randomize();

	for (auto&& view : room.m_netViews) {
		Vector3f pos1 = pos + rot * view.m_pos;
		Quaternion rot1 = rot * view.m_rot;

		ZDOManager()->Instantiate(*view.m_prefab, pos1, rot1);
	}

	// TODO this might be redundant for dummy 'dungeons' (plains villages shouldnt be considered dungeons)
	auto component2 = std::make_unique<RoomInstance>(room, pos, rot, 0, seed);
	m_placedRooms.push_back(std::move(component2));
}

void DungeonGenerator::PlaceRoom(const Room& room, Vector3f pos, Quaternion rot, const RoomConnectionInstance& fromConnection) {
	// wtf is the point of this?
	//	only setting a seed? seems really extraneous
	//Vector3f vector = pos;
	//if (this->m_dungeon.m_useCustomInteriorTransform)
		//vector -= this->m_pos;
	//int seed = (int)vector.x * 4271 + (int)vector.y * 9187 + (int)vector.z * 2134;

	// TODO seed is only useful for RandomSpawn
	int seed = (int)pos.x * 4271 + (int)pos.y * 9187 + (int)pos.z * 2134;

	//VUtils::Random::State state(seed);
	//for (auto&& randomSpawn : room.m_randomSpawns)
	//	randomSpawn.Randomize();

	if (VH_SETTINGS.dungeonsRoomsFurnishing) {
		for (auto&& view : room.m_netViews) {
			Vector3f pos1 = pos + rot * view.m_pos;
			Quaternion rot1 = rot * view.m_rot;

			// Prefabs can be instantiated exactly in world space (not local room space)
			auto global = VUtils::Physics::LocalToGlobal(pos1, rot1, this->m_pos, this->m_rot);

			ZDOManager()->Instantiate(*view.m_prefab, global.first, global.second);
		}
	}

	auto component2 = std::make_unique<RoomInstance>(room, pos, rot, fromConnection.m_placeOrder + 1, seed);

	this->AddOpenConnections(*component2, fromConnection);

	m_placedRooms.push_back(std::move(component2));
}


void DungeonGenerator::AddOpenConnections(RoomInstance& newRoom, const RoomConnectionInstance& skipConnection) {
	auto&& connections = newRoom.m_connections;
	for (auto&& roomConnection : connections) {
		if (!roomConnection->m_connection.get().m_entrance
			&& roomConnection->m_pos.SqDistance(skipConnection.m_pos) >= .1f * .1f)
		{
			roomConnection->m_placeOrder = newRoom.m_placeOrder;
			m_openConnections.push_back(*roomConnection.get());
		}
	}
}

// Determine whether a room (with center at origin of room) is completely contained within a zone
// TODO rename something better, wtf is 'IsInsideDungeon'
//	this just makes sure that a rotated rectangle is within the zone
bool DungeonGenerator::IsInsideZone(const Room& room, Vector3f pos, Quaternion rot) {
	if (!VH_SETTINGS.dungeonsRoomsZoneBounded)
		return true;

	Vector3f semiSize = room.m_size * .5f;

	if (room.m_endCap)
		semiSize *= VH_SETTINGS.dungeonsEndcapsInsetFrac;

	if (pos.y + semiSize.y < m_zoneCenter.y - m_zoneSize.y * .5f
		|| pos.y - semiSize.y > m_zoneCenter.y + m_zoneSize.y * .5f)
		return false;

	Vector3f a = pos + rot * Vector3f(-semiSize.x,	0,		-semiSize.z);
	Vector3f b = pos + rot * Vector3f(-semiSize.x,	0,		semiSize.z);
	Vector3f c = pos + rot * Vector3f(semiSize.x,		0,		semiSize.z);
	Vector3f d = pos + rot * Vector3f(semiSize.x,		0,		-semiSize.z);

	//Vector3f e = pos + rot * Vector3f(-semiSize.x,	0,	-semiSize.z);
	//Vector3f f = pos + rot * Vector3f(-semiSize.x,	0,	semiSize.z);
	//Vector3f g = pos + rot * Vector3f(semiSize.x,		0,	semiSize.z);
	//Vector3f h = pos + rot * Vector3f(semiSize.x,		0,	-semiSize.z);

	static auto&& inRectSemi = [](const Vector3f &semiSize, const Vector3f& pos, const Vector3f& point) {
		return point.x >= pos.x - semiSize.x && point.x <= pos.x + semiSize.x
			//&& point.y >= pos.y - semiSize.y && point.y <= pos.y + semiSize.y
			&& point.z >= pos.z - semiSize.z && point.z <= pos.z + semiSize.z;
	};

	Vector3f semiZone = m_zoneSize * .5f;

	return inRectSemi(semiZone, m_zoneCenter, a)
		&& inRectSemi(semiZone, m_zoneCenter, b)
		&& inRectSemi(semiZone, m_zoneCenter, c)
		&& inRectSemi(semiZone, m_zoneCenter, d);
		//&& inRectSemi(semiZone, m_zoneCenter, e)
		//&& inRectSemi(semiZone, m_zoneCenter, f)
		//&& inRectSemi(semiZone, m_zoneCenter, g)
		//&& inRectSemi(semiZone, m_zoneCenter, h);

	//return VUtils::Physics::RectInsideRect(
	//	m_zoneSize, m_zoneCenter, Quaternion::IDENTITY,
	//	room.m_size, pos, rot);
}

static bool RectOverlapRect(Vector3f size1, Vector3f pos1, Vector3f size2, Vector3f pos2) {
	assert(size1.x >= 0 && size1.y >= 0 && size1.z >= 0
		&& size2.x >= 0 && size2.y >= 0 && size2.z >= 0);

	size1 *= .5f;
	size2 *= .5f;

	return !(pos1.x + size1.x < pos2.x - size2.x
		|| pos1.y + size1.y < pos2.y - size2.y
		|| pos1.z + size1.z < pos2.z - size2.z
		|| pos1.x - size1.x > pos2.x + size2.x
		|| pos1.y - size1.y > pos2.y + size2.y
		|| pos1.z - size1.z > pos2.z + size2.z);
}

bool DungeonGenerator::TestCollision(const Room& room, Vector3f pos, Quaternion rot) {

	{
		Vector3f newPos = pos;
		Quaternion newRot = rot;

		if (m_dungeon.m_algorithm == Dungeon::Algorithm::Dungeon)
			std::tie(newPos, newRot) = VUtils::Physics::LocalToGlobal(pos, rot, this->m_pos, this->m_rot);

		// Constrain dungeon within zone
		if (!this->IsInsideZone(room, newPos, newRot))
			return true;
	}

	//if (room.m_endCap)
		//return false;

	Vector3f size;	
	if (m_dungeon.m_algorithm == Dungeon::Algorithm::Dungeon) {
		// Rotate this room by either IDENTITY (0 deg), or 90deg
		// Assumes that dungeon rooms fit together like axis aligned boxes
		size = rot * room.m_size;

		size.x = std::abs(size.x);
		size.z = std::abs(size.z);
	}
	else {
		// Resize the room to its smallest fitting circular region
		size = room.m_size.Normal() * Vector2f(room.m_size.x, room.m_size.z).Magnitude();
	}

	if (room.m_endCap)
		//size *= .5f;
		size *= VH_SETTINGS.dungeonsEndcapsInsetFrac;
	else
		//size -= Vector3f(.1f, .1f, .1f);
		size -= Vector3f(1, 1, 1) * VH_SETTINGS.dungeonsRoomsInsetSize;

	//if (VH_SETTINGS.dungeonsRoomsInsetSize)
		//size -= Vector3f(.1f, .1f, .1f);
	//if (room.m_endCap)
		//size -= Vector3f(.2f, .2f, .2f); // subtract because edge touching rectangles always overlap (so prevent that)
	//else size += Vector3f(.2f, .2f, .2f);

	// determine whether the room collides with any other room
	for (auto&& other : m_placedRooms) {
		auto&& otherRoom = other->m_room.get();

		Vector3f otherSize;
		if (m_dungeon.m_algorithm == Dungeon::Algorithm::Dungeon) {
			otherSize = other->m_rot * otherRoom.m_size;

			otherSize.x = std::abs(otherSize.x);
			otherSize.z = std::abs(otherSize.z);
		}
		else {
			otherSize = otherRoom.m_size.Normal() * Vector2f(otherRoom.m_size.x, otherRoom.m_size.z).Magnitude();
		}

		//otherSize -= Vector3f(.1f, .1f, .1f);

		if (RectOverlapRect(size, pos, otherSize, other->m_pos))
			return true;
	}

	size *= .5f;

	////LOG(INFO) 
	//	<< "polygon((" 
	//	<< pos.x - size.x << "," << pos.z - size.z << "),("
	//	<< pos.x - size.x << "," << pos.z + size.z << "),("
	//	<< pos.x + size.x << "," << pos.z + size.z << "),("
	//	<< pos.x + size.x << "," << pos.z - size.z << "))";

	return false;
}

const Room* DungeonGenerator::GetRandomWeightedRoom(VUtils::Random::State& state, bool perimeterRoom) {
	std::vector<const Room*> tempRooms;

	float num = 0;
	for (auto&& roomData : m_dungeon.m_availableRooms)
	{
		if (!roomData->m_entrance && !roomData->m_endCap && !roomData->m_divider && roomData->m_perimeter == perimeterRoom)
		{
			num += roomData->m_weight;
			tempRooms.push_back(roomData.get());
		}
	}

	if (tempRooms.empty())
		return nullptr;

	float num2 = state.Range(0.f, num);
	float num3 = 0;
	for (auto&& roomData2 : tempRooms) {
		num3 += roomData2->m_weight;
		if (num2 <= num3)
			return roomData2;
	}

	// TODO this seems sus
	//	weighted search usually failed if this point is ever reach, signaling bad values or bad algo
	// throw or exit() if this point is reached
	throw std::runtime_error("unexpected");
	//return tempRooms[0];
}

const Room* DungeonGenerator::GetRandomWeightedRoom(VUtils::Random::State& state, const RoomConnectionInstance* connection) {
	std::vector<std::reference_wrapper<const Room>> tempRooms;

	for (auto&& roomData : m_dungeon.m_availableRooms)
	{
		if (!roomData->m_entrance
			&& !roomData->m_endCap
			&& !roomData->m_divider
			&& (!connection || (roomData->HaveConnection(connection->m_connection)
				&& connection->m_placeOrder >= roomData->m_minPlaceOrder)))
		{
			tempRooms.push_back(*roomData);
		}
	}

	// This case is possible with DG_DvergrBoss
	if (tempRooms.empty())
		return nullptr;

	return &this->GetWeightedRoom(state, tempRooms);
}

const Room& DungeonGenerator::GetWeightedRoom(VUtils::Random::State& state, const std::vector<std::reference_wrapper<const Room>>& rooms) {
	float num = 0;
	for (auto&& roomData : rooms)
		num += roomData.get().m_weight;

	float num2 = state.Range(0.f, num);
	float num3 = 0;
	for (auto&& roomData2 : rooms) {
		num3 += roomData2.get().m_weight;
		if (num2 <= num3)
			return roomData2;
	}

	throw std::runtime_error("unexpected");
	//return *m_tempRooms[0];
}

const Room* DungeonGenerator::GetRandomRoom(VUtils::Random::State& state, const RoomConnectionInstance* connection) {
	std::vector<std::reference_wrapper<const Room>> tempRooms;

	for (auto&& roomData : m_dungeon.m_availableRooms) {
		if (!roomData->m_entrance
			&& !roomData->m_endCap
			&& !roomData->m_divider
			&& (!connection
				|| (roomData->HaveConnection(connection->m_connection)
					&& connection->m_placeOrder >= roomData->m_minPlaceOrder)))
		{
			tempRooms.push_back(*roomData.get());
		}
	}

	if (tempRooms.empty())
		return nullptr;

	return &tempRooms[state.Range(0, tempRooms.size())].get();
}

decltype(DungeonGenerator::m_openConnections)::iterator DungeonGenerator::GetOpenConnection(VUtils::Random::State& state) {
	if (m_openConnections.empty())
		return m_openConnections.end();

	return std::next(m_openConnections.begin(), state.Range(0, m_openConnections.size()));
}

const Room& DungeonGenerator::FindStartRoom(VUtils::Random::State& state) {
	std::vector<std::reference_wrapper<const Room>> tempRooms;

	for (auto&& roomData : m_dungeon.m_availableRooms) {
		if (roomData->m_entrance)
			tempRooms.push_back(*roomData.get());
	}

	return tempRooms[state.Range(0, tempRooms.size())];
}

bool DungeonGenerator::CheckRequiredRooms() {
	if (this->m_dungeon.m_minRequiredRooms == 0 || this->m_dungeon.m_requiredRooms.empty())
		return false;

	int num = 0;
	for (auto&& room : m_placedRooms) {
		if (this->m_dungeon.m_requiredRooms.contains(room->m_room.get().m_name))
			num++;
	}

	return num >= this->m_dungeon.m_minRequiredRooms;
}
