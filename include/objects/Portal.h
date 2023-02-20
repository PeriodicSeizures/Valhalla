#pragma once

#include "../Hashes.h"
#include "../ZDO.h"

class Portal {
public:
	ZDO& m_zdo;
	Portal(ZDO& zdo) : m_zdo(zdo) {}

    std::string GetTag() {
		return m_zdo.GetString(Hashes::ZDO::TeleportWorld::TAG, "");
    }

	void SetTag(const std::string& tag) {
		m_zdo.Set(Hashes::ZDO::TeleportWorld::TAG, tag);
	}

	ZDOID GetTarget() {
		//return m_zdo->GetNetID(Hashes::ZDO::TeleportWorld::TARGET);
		return m_zdo.GetNetID("target");
	}

	void SetTarget(const ZDOID& target) {
		m_zdo.Set("target", target);
	}

	std::string GetAuthor() {
		return m_zdo.GetString(Hashes::ZDO::TeleportWorld::AUTHOR, "");
	}

	void SetAuthor(const std::string& author) {
		m_zdo.Set(Hashes::ZDO::TeleportWorld::AUTHOR, author);
	}
};