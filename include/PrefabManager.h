#pragma once

#include <robin_hood.h>

#include "VUtils.h"
#include "Prefab.h"
#include "ZDO.h"

class IPrefabManager {

private:
	robin_hood::unordered_map<HASH_t, std::unique_ptr<Prefab>> m_prefabs;

public:
	void Init();

	const Prefab* GetPrefab(HASH_t hash);

	const Prefab* GetPrefab(const std::string& name) {
		return GetPrefab(VUtils::String::GetStableHashCode(name));
	}

	ZDO* Instantiate(HASH_t hash, const Vector3& pos, const Quaternion& rot = Quaternion::IDENTITY, const Prefab** outPrefab = nullptr);
	ZDO* Instantiate(const Prefab* prefab, const Vector3& pos, const Quaternion& rot = Quaternion::IDENTITY);

};

IPrefabManager* PrefabManager();