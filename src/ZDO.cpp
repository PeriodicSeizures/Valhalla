#include <functional>

#include "ZDO.h"
#include "ZDOManager.h"
#include "ValhallaServer.h"
#include "ZDOID.h"
#include "PrefabManager.h"
#include "Prefab.h"
#include "ZoneManager.h"
#include "NetManager.h"
#include "VUtilsResource.h"

std::pair<HASH_t, HASH_t> ZDO::ToHashPair(const std::string& key) {
    return {
        VUtils::String::GetStableHashCode(key + "_u"),
        VUtils::String::GetStableHashCode(key + "_i") 
    };
}



ZDO::ZDO() 
    : m_prefab(Prefab::NONE) {

}

ZDO::ZDO(const ZDOID& id, const Vector3f& pos)
    : m_pos(pos), m_prefab(Prefab::NONE)
{
    this->_SetID(id);
    //m_rev.m_ticksCreated = Valhalla()->GetWorldTicks();
}

//ZDO::ZDO(const ZDOID& id, const Vector3f& pos, HASH_t prefab)
//    : m_id(id), m_pos(pos), m_prefab(PrefabManager()->RequirePrefab(prefab))
//{
//    m_rev.m_ticksCreated = Valhalla()->GetWorldTicks();
//}

void ZDO::Save(DataWriter& pkg) const {
    auto&& prefab = GetPrefab();

    pkg.Write(this->m_rev.m_ownerRev);
    pkg.Write(this->m_rev.m_dataRev);

    pkg.Write(prefab.AnyFlagsAbsent(Prefab::Flag::SESSIONED));

    pkg.Write<OWNER_t>(0); //pkg.Write(this->m_owner);
    //pkg.Write(this->m_rev.m_ticksCreated.count());
    pkg.Write(GetTimeCreated().count());
    pkg.Write(VConstants::PGW);

    pkg.Write(prefab.m_type);
    pkg.Write(prefab.AllFlagsPresent(Prefab::Flag::DISTANT));
    pkg.Write(prefab.m_hash);

    pkg.Write(this->GetZone());              //pkg.Write(IZoneManager::WorldToZonePos(this->m_pos));
    pkg.Write(this->m_pos);
    pkg.Write(this->m_rotation);
    
    // Save uses 2 bytes for counts (char in c# is 2 bytes..)
    _TryWriteType<float,            char16_t>(pkg);
    _TryWriteType<Vector3f,         char16_t>(pkg);
    _TryWriteType<Quaternion,       char16_t>(pkg);
    _TryWriteType<int32_t,          char16_t>(pkg);
    _TryWriteType<int64_t,          char16_t>(pkg);
    _TryWriteType<std::string,      char16_t>(pkg);
    _TryWriteType<BYTES_t,          char16_t>(pkg);
}

bool ZDO::Load(DataReader& pkg, int32_t worldVersion) {
    this->m_rev.m_ownerRev = pkg.Read<uint32_t>();  // TODO redundant?
    this->m_rev.m_dataRev = pkg.Read<uint32_t>();   // redundant?
    pkg.Read<bool>(); //this->m_persistent
    //this->m_owner = pkg.Read<OWNER_t>();
    pkg.Read<OWNER_t>(); // unused owner
    //this->m_rev.m_ticksCreated = TICKS_t(pkg.Read<int64_t>()); // necessary for TerrainComp (order)
    auto timeCreated = TICKS_t(pkg.Read<int64_t>());
    bool modern = pkg.Read<int32_t>() == VConstants::PGW;

    if (worldVersion >= 16 && worldVersion < 24)
        pkg.Read<int32_t>();

    if (worldVersion >= 23)
        pkg.Read<Prefab::Type>(); // m_type

    if (worldVersion >= 22)
        pkg.Read<bool>(); // m_distant

    if (worldVersion < 13) {
        pkg.Read<char16_t>();
        pkg.Read<char16_t>();
    }

    //sol::resolve<bool(const fs::path&, const BYTES_t&)>(VUtils::Resource::WriteFile);

    //auto fn = static_cast<std::optional<BYTES_t>()(const fs::path&)>(VUtils::Resource::ReadFile);
    
    if (worldVersion >= 17)
        this->m_prefab = PrefabManager()->RequirePrefab(pkg.Read<HASH_t>());

    pkg.Read<ZoneID>(); // m_sector
    this->m_pos = pkg.Read<Vector3f>();
    this->m_rotation = pkg.Read<Quaternion>();

    _TryReadType<float,         char16_t>(pkg);
    _TryReadType<Vector3f,      char16_t>(pkg);
    _TryReadType<Quaternion,    char16_t>(pkg);
    _TryReadType<int32_t,       char16_t>(pkg);
    _TryReadType<int64_t,       char16_t>(pkg);
    _TryReadType<std::string,   char16_t>(pkg);
    
    if (worldVersion >= 27)
        _TryReadType<BYTES_t,   char16_t>(pkg);

    if (worldVersion < 17)
        this->m_prefab = PrefabManager()->RequirePrefab(GetInt("prefab"));

    SetTimeCreated(timeCreated);

    return modern;
}




// ZDO specific-methods

void ZDO::SetPosition(const Vector3f& pos) {
    if (m_pos != pos) {
        ZDOManager()->InvalidateZDOZone(*this);
        this->m_pos = pos;
        ZDOManager()->AddZDOToZone(*this);

        if (IsLocal())
            Revise();
    }
}

ZoneID ZDO::GetZone() const {
    return IZoneManager::WorldToZonePos(m_pos);
}

void ZDO::Serialize(DataWriter& pkg) const {
    static_assert(sizeof(VConstants::PGW) == 4);

    auto&& prefab = GetPrefab();
    
    pkg.Write(prefab.AnyFlagsAbsent(Prefab::Flag::SESSIONED));
    pkg.Write(prefab.AllFlagsPresent(Prefab::Flag::DISTANT));

    pkg.Write(GetTimeCreated().count());
    //pkg.Write(m_rev.m_ticksCreated.count());
    pkg.Write(VConstants::PGW);

    pkg.Write(prefab.m_type); // sbyte
    pkg.Write(prefab.m_hash);

    pkg.Write(m_rotation);

    // sections organized like this:
    //    32 bit mask: F V Q I S L A
    //  for each present type in order...
    
    // Writing a signed/unsigned mask doesnt matter
    //  same when positive (for both)
    pkg.Write(static_cast<int32_t>(GetOrdinalMask()));

    _TryWriteType<float,            BYTE_t>(pkg);
    _TryWriteType<Vector3f,         BYTE_t>(pkg);
    _TryWriteType<Quaternion,       BYTE_t>(pkg);
    _TryWriteType<int32_t,          BYTE_t>(pkg);
    _TryWriteType<int64_t,          BYTE_t>(pkg);
    _TryWriteType<std::string,      BYTE_t>(pkg);
    _TryWriteType<BYTES_t,          BYTE_t>(pkg);
}

void ZDO::Deserialize(DataReader& pkg) {
    static_assert(sizeof(Prefab::Type) == 1);
    
    pkg.Read<bool>();       // m_persistent
    pkg.Read<bool>();       // m_distant
    //this->m_rev.m_ticksCreated = TICKS_t(pkg.Read<int64_t>());
    auto timeCreated = TICKS_t(pkg.Read<int64_t>());
    pkg.Read<int32_t>();    // m_pgwVersion
    pkg.Read<Prefab::Type>(); // this->m_type
    auto prefabHash = pkg.Read<HASH_t>();
    if (!m_prefab.get()) {
        this->m_prefab = PrefabManager()->RequirePrefab(prefabHash);
        SetTimeCreated(timeCreated);
    }
    
    this->m_rotation = pkg.Read<Quaternion>();

    auto ordinalMask = static_cast<Ordinal>(pkg.Read<int32_t>());
    this->SetOrdinalMask(ordinalMask);

    // double check this; 
    if (ordinalMask & GetOrdinalMask<float>())
        _TryReadType<float,         BYTE_t>(pkg);
    if (ordinalMask & GetOrdinalMask<Vector3f>())
        _TryReadType<Vector3f,      BYTE_t>(pkg);
    if (ordinalMask & GetOrdinalMask<Quaternion>())
        _TryReadType<Quaternion,    BYTE_t>(pkg);
    if (ordinalMask & GetOrdinalMask<int32_t>())
        _TryReadType<int32_t,       BYTE_t>(pkg);
    if (ordinalMask & GetOrdinalMask<int64_t>())
        _TryReadType<int64_t,       BYTE_t>(pkg);
    if (ordinalMask & GetOrdinalMask<std::string>())
        _TryReadType<std::string,   BYTE_t>(pkg);
    if (ordinalMask & GetOrdinalMask<BYTES_t>())
        _TryReadType<BYTES_t,       BYTE_t>(pkg);
}
