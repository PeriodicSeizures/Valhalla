#pragma once

#include "HashUtils.h"

#include <robin_hood.h>
#include <type_traits>
#include <any>

#include "Quaternion.h"
#include "Vector.h"
#include "VUtils.h"
#include "VUtilsString.h"
#include "NetPackage.h"

template<typename T>
concept TrivialSyncType = std::same_as<T, float>
	|| std::same_as<T, Vector3>
	|| std::same_as<T, Quaternion>
	|| std::same_as<T, int32_t>
	|| std::same_as<T, int64_t>	
	|| std::same_as<T, std::string>
	|| std::same_as<T, BYTES_t>;

// NetSync is 500+ bytes (with all 7 maps)
// NetSync is 168 bytes (with 1 map only)
// NetSync is 144 bytes (combined member map, reduced members)
// Currently 160 bytes
class NetSync {
public:
	enum class ObjectType : int8_t {
		Default,
		Prioritized,
		Solid,
		Terrain
	};

	struct Rev {
		uint32_t m_dataRev = 0;
		uint32_t m_ownerRev = 0;
		int64_t m_time = 0;
	};

	static std::pair<HASH_t, HASH_t> ToHashPair(const std::string& key);

private:
	// https://stackoverflow.com/a/1122109
	enum class Ordinal : uint8_t {
		FLOAT = 1, // position 1 (subtract 1 to get the shift; 1 - 1 = 0 shifts)
		VECTOR3,
		QUATERNION,
		INT,
		STRING,
		LONG,
		ARRAY, // shift by 7 - 1 = 6 bits
	};

	template<TrivialSyncType T>
	constexpr Ordinal GetOrdinal() {
		if constexpr (std::same_as<T, float>) {
			return Ordinal::FLOAT;
		}
		else if constexpr (std::same_as<T, Vector3>) {
			return Ordinal::VECTOR3;
		}
		else if constexpr (std::same_as<T, Quaternion>) {
			return Ordinal::QUATERNION;
		}
		else if constexpr (std::same_as<T, int32_t>) {
			return Ordinal::INT;
		}
		else if constexpr (std::same_as<T, int64_t>) {
			return Ordinal::LONG;
		}
		else if constexpr (std::same_as<T, std::string>) {
			return Ordinal::STRING;
		}
		else { //if constexpr (std::same_as<T, BYTES_t>) {
			return Ordinal::ARRAY;
		}
	}

	template<TrivialSyncType T>
	constexpr Ordinal GetOrdinalShift() {
		return static_cast<std::underlying_type_t>(GetOrdinal<T>()) - 1;
	}

	template<TrivialSyncType T>
	constexpr HASH_t ToShiftHash(HASH_t hash) {
		auto shift = GetOrdinalShift<T>();

		return (hash
			+ (shift * shift)
			^ shift)
			^ (shift << shift);
	}

	template<TrivialSyncType T>
	constexpr HASH_t FromShiftHash(HASH_t hash) {
		auto shift = GetOrdinalShift<T>();

		return
			((hash
				^ (shift << shift))
				^ shift)
			- (shift * shift);
	}

	// Get the object by hash
	// No copies are made (even for primitives)
	// Returns null if not found 
	// Throws on type mismatch
	template<TrivialSyncType T>
	const T* _Get(HASH_t key) {
		auto ordinal = GetOrdinal<T>();
		if (m_ordinalMask(ordinal)) {
			key = ToShiftHash<T>(key);
			auto&& find = m_members.find(key);
			if (find != m_members.end()) {
				if (find->second.first != ordinal)
					throw std::invalid_argument("type mismatch");
				return (T*)find->second.second;
			}
		}
		return nullptr;
	}

	// Set the object by hash
	// If the hash already exists (assuming type matches), type assignment operator is used
	// Otherwise, a copy is made of the value and allocated
	// Throws on type mismatch
	template<TrivialSyncType T>
	void _Set(HASH_t key, const T& value) {
		//auto prefix = GetShift<T>();
		auto ordinal = GetOrdinal<T>();
		key = ToShiftHash<T>(key);
		if (m_ordinalMask(ordinal)) {
			auto&& find = m_members.find(key);
			assert(find != m_members.end());

			if (prefix != find->second.first)
				throw std::invalid_argument("type mismatch; possible hash collision?");

			// reassign if changed
			auto&& v = (T*)find->second.second;
			if (*v == value)
				return;
			*v = value;
		}
		else {
			m_members.insert({ key, std::make_pair(ordinal, new T(value)) });
		}

		m_dataMask |= (0b1 << GetOrdinalShift<T>());
	}

	void _Set(HASH_t key, const void* value, Ordinal prefix) {
		switch (prefix) {
			case Ordinal::FLOAT:		_Set(key, *(float*)			value); break;
			case Ordinal::VECTOR3:		_Set(key, *(Vector3*)		value); break;
			case Ordinal::QUATERNION:	_Set(key, *(Quaternion*)	value); break;
			case Ordinal::INT:			_Set(key, *(int32_t*)		value); break;
			case Ordinal::LONG:			_Set(key, *(int64_t*)		value); break;
			case Ordinal::STRING:		_Set(key, *(std::string*)	value); break;
			case Ordinal::ARRAY:		_Set(key, *(BYTES_t*)		value); break;
			default:
				throw std::invalid_argument("invalid type");
		}
	}

private:
	robin_hood::unordered_map<HASH_t, std::pair<Ordinal, void*>> m_members;

	bool m_persistent = false;	// set by ZNetView
	bool m_distant = false;		// set by ZNetView
	//int64_t m_timeCreated = 0;	// TerrainModifier (10000 ticks / ms) (union)?
	
	ObjectType m_type = ObjectType::Default; // set by ZNetView
	HASH_t m_prefab = 0;
	Quaternion m_rotation = Quaternion::IDENTITY;
	//uint8_t m_dataMask = 0; // internal member used for zdo name map types
	BitMask<Ordinal> m_ordinalMask;

	Vector2i m_sector;
	Vector3 m_position;			// position of the 
	NetID m_id;					// unique identifier; immutable through 'lifetime'
	OWNER_t m_owner = 0;			// local or remote OWNER_t

	//uint32_t m_ownerRev = 0;	// could be rev structure
	//uint32_t m_dataRev = 0;	// could be rev structure

	void Revise() {
		m_rev.m_dataRev++;
	}

    void FreeMembers();

public:
	Rev m_rev;
	int32_t m_pgwVersion = 0;	// 53 is the latest

public:
	// Create ZDO with me (im owner)
	NetSync();

	// Loading ZDO from disk package
	NetSync(NetPackage reader, int version);

	// Save ZDO to the disk package
	void Save(NetPackage &writer);


	NetSync(const NetSync& other); // copy constructor

	~NetSync();

	// Trivial hash getters

	template<TrivialSyncType T>
		requires (!std::same_as<T, BYTES_t>)
	const T& Get(HASH_t key, const T& value) {
		auto&& get = _Get<T>(key);
		if (get) return *get;
		return value;
	}

	float GetFloat(HASH_t key, float value = 0);
	int32_t GetInt(HASH_t key, int32_t value = 0);
	int64_t GetLong(HASH_t key, int64_t value = 0);
	const Quaternion& GetQuaternion(HASH_t key, const Quaternion& value = Quaternion::IDENTITY);
	const Vector3& GetVector3(HASH_t key, const Vector3& value);
	const std::string& GetString(HASH_t key, const std::string& value = "");
	const BYTES_t* GetBytes(HASH_t key /* no default */);

	// Special hash getters

	bool GetBool(HASH_t key, bool value = false);
	NetID GetNetID(const std::pair<HASH_t, HASH_t> &key /* no default */);



	// Trivial string getters

	float GetFloat(const std::string& key, float value = 0);
	int32_t GetInt(const std::string& key, int32_t value = 0);
	int64_t GetLong(const std::string& key, int64_t value = 0);
	const Quaternion& GetQuaternion(const std::string& key, const Quaternion& value = Quaternion::IDENTITY);
	const Vector3& GetVector3(const std::string& key, const Vector3& value);
	const std::string& GetString(const std::string& key, const std::string& value = "");
	const BYTES_t* GetBytes(const std::string& key /* no default */);

	// Special string getters

	bool GetBool(const std::string &key, bool value = false);
	NetID GetNetID(const std::string &key /* no default */);



	// Trivial hash setters

	template<TrivialSyncType T>
	void Set(HASH_t key, const T& value) {
		_Set(key, value);

		Revise();
	}

	// Special hash setters

	void Set(HASH_t key, bool value);
	void Set(const std::pair<HASH_t, HASH_t> &key, const NetID& value);



	// Trivial string setters (+bool)

	template<typename T> requires TrivialSyncType<T> || std::same_as<T, bool>
	void Set(const std::string& key, const T &value) { Set(VUtils::String::GetStableHashCode(key), value); }
	void Set(const std::string& key, const std::string& value) { Set(VUtils::String::GetStableHashCode(key), value); } // String overload
	void Set(const char* key, const std::string &value) { Set(VUtils::String::GetStableHashCode(key), value); } // string constexpr overload

	// Special string setters

	void Set(const std::string& key, const NetID& value) { Set(ToHashPair(key), value); }
	void Set(const char* key, const NetID& value) { Set(ToHashPair(key), value); }




	
	

	// dumb vars
	//float m_tempSortValue = 0; // only used in sending priority
	//bool m_tempHaveRevision = 0; // appears to be unused besides assignments

	//int32_t m_tempRemovedAt = -1; // equal to frame counter at intervals
	//int32_t m_tempCreatedAt = -1; // ^

	[[nodiscard]] bool Persists() const {
		return m_persistent;
	}

	[[nodiscard]] bool Distant() const {
		return m_distant;
	}

	//[[nodiscard]] int32_t Version() const {
	//	return m_pgwVersion;
	//}

	[[nodiscard]] ObjectType Type() const {
		return m_type;
	}

	[[nodiscard]] HASH_t PrefabHash() const {
		return m_prefab;
	}

	[[nodiscard]] const Quaternion& Rotation() const {
		return m_rotation;
	}

	[[nodiscard]] const Vector2i& Sector() const {
		return m_sector;
	}

	[[nodiscard]] const NetID ID() const {
		return m_id;
	}

	//const Rev GetRev() const {
	//	return m_rev;
	//}

	[[nodiscard]] OWNER_t Owner() const {
		return m_owner;
	}

	[[nodiscard]] const Vector3& Position() const {
		return m_position;
	}

	void InvalidateSector() {
		this->SetSector(Vector2i(-100000, -10000));
	}

	void SetSector(const Vector2i& sector);




	void SetPosition(const Vector3& pos);

	//bool Outdated(const Rev& min) const {
	//	return min.m_dataRev > m_rev.m_dataRev
	//		|| min.m_ownerRev > m_rev.m_ownerRev;
	//}



	//friend void NetSyncManager::RPC_NetSyncData(NetRpc* rpc, NetPackage pkg);



	// Return whether the ZDO instance is self hosted or remotely hosted
	bool Local();

	// Whether an owner has been assigned to this ZDO
	bool HasOwner() const {
		return m_owner != 0;
	}

	// Claim ownership over this ZDO
	void SetLocal();

	// set the owner of the ZDO
	void SetOwner(OWNER_t owner) {
		if (m_owner != owner) {
			m_owner = owner;
			m_rev.m_ownerRev++;
		}
	}

	[[nodiscard]] bool Valid() const {
		if (m_id)
			return true;
		return false;
	}

    void Invalidate();

	void Abandon() {
		m_owner = 0;
	}

	// Write ZDO to the network packet
	void Serialize(NetPackage &pkg);

	// Load ZDO from the network packet
	void Deserialize(NetPackage &pkg);
};

using ZDO = NetSync;
