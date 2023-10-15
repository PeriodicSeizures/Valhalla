#include <thread>

#include "NetManager.h"
#include "ZoneManager.h"
#include "GeoManager.h"
#include "HeightmapBuilder.h"
#include "PrefabManager.h"
#include "RouteManager.h"
#include "VUtilsResource.h"
#include "Hashes.h"
#include "DungeonManager.h"
#include "DungeonGenerator.h"
#include "ZDOManager.h"

auto ZONE_MANAGER(std::make_unique<IZoneManager>()); // TODO stop constructing in global
IZoneManager* ZoneManager() {
    return ZONE_MANAGER.get();
}



// private
void IZoneManager::PostPrefabInit() {
    LOG_INFO(LOGGER, "Initializing ZoneManager");

    {
#if VH_IS_ON(VH_ZONE_GENERATION)
        // load ZoneLocations:
        auto opt = VUtils::Resource::ReadFile<BYTES_t>("features.pkg");
        if (!opt)
            throw std::runtime_error("features.pkg missing");

        DataReader pkg(*opt);

        pkg.Read<std::string_view>(); // comment
        auto ver = pkg.Read<std::string_view>();
        if (ver != VConstants::GAME)
            LOG_WARNING(LOGGER, "features.pkg uses different game version than server ({})", ver);

        auto count = pkg.Read<int32_t>();
        for (int i=0; i < count; i++) {
            // TODO read zoneLocations from file
            auto loc = std::make_unique<Feature>();
            loc->m_name = pkg.Read<std::string>();
            loc->m_hash = VUtils::String::GetStableHashCode(loc->m_name);

            loc->m_biome = (Biome)pkg.Read<int32_t>();
            loc->m_biomeArea = (BiomeArea)pkg.Read<int32_t>();
            loc->m_applyRandomDamage = pkg.Read<bool>();
            loc->m_centerFirst = pkg.Read<bool>();
            loc->m_clearArea = pkg.Read<bool>();
            //loc->m_useCustomInteriorTransform = pkg.Read<bool>();
            loc->m_exteriorRadius = pkg.Read<float>();
            loc->m_interiorRadius = pkg.Read<float>();
            loc->m_forestTresholdMin = pkg.Read<float>();
            loc->m_forestTresholdMax = pkg.Read<float>();
            //loc->m_interiorPosition = pkg.Read<Vector3f>();
            //loc->m_generatorPosition = pkg.Read<Vector3f>();
            loc->m_group = pkg.Read<std::string>();
            loc->m_iconAlways = pkg.Read<bool>();
            loc->m_iconPlaced = pkg.Read<bool>();
            loc->m_inForest = pkg.Read<bool>();
            loc->m_minAltitude = pkg.Read<float>();
            loc->m_maxAltitude = pkg.Read<float>();
            loc->m_minDistance = pkg.Read<float>();
            loc->m_maxDistance = pkg.Read<float>();
            loc->m_minTerrainDelta = pkg.Read<float>();
            loc->m_maxTerrainDelta = pkg.Read<float>();
            loc->m_minDistanceFromSimilar = pkg.Read<float>();
            loc->m_spawnAttempts = pkg.Read<int32_t>();
            loc->m_quantity = pkg.Read<int32_t>();
            loc->m_randomRotation = pkg.Read<bool>();

            // randomspawns
            auto spawns = pkg.Read<int32_t>();
            for (int s = 0; s < spawns; s++) {
                pkg.Read<float>(); // chance
                
                auto views = pkg.Read<int32_t>();
                for (int v = 0; v < views; v++) {
                    pkg.Read<HASH_t>(); // prefab hash
                    pkg.Read<Vector3f>(); // position
                    pkg.Read<Quaternion>(); // rotation
                }
            }

            loc->m_slopeRotation = pkg.Read<bool>();
            loc->m_snapToWater = pkg.Read<bool>();
            loc->m_unique = pkg.Read<bool>();

            auto views = pkg.Read<int32_t>();
            for (int j=0; j < views; j++) {
                Prefab::Instance piece;

                auto hash = pkg.Read<HASH_t>();

                piece.m_prefab = &PrefabManager()->RequirePrefabByHash(hash);

                piece.m_pos = pkg.Read<Vector3f>();
                piece.m_rot = pkg.Read<Quaternion>();
                loc->m_pieces.push_back(piece);
            }

            //m_featuresByHash.insert({ loc->m_hash, *loc.get() });
            m_features.push_back(std::move(loc));
        }

        LOG_INFO(LOGGER, "Loaded {} features", count);
#endif
    }    

    {
#if VH_IS_ON(VH_ZONE_GENERATION)
        // load Foliage:
        auto opt = VUtils::Resource::ReadFile<BYTES_t>("vegetation.pkg");
        if (!opt)
            throw std::runtime_error("vegetation.pkg missing");

        DataReader pkg(*opt);

        pkg.Read<std::string_view>(); // comment
        auto ver = pkg.Read<std::string_view>();
        if (ver != VConstants::GAME) {
            LOG_WARNING(LOGGER, "vegetation.pkg uses different game version than server ({})", ver);
        }

        auto count = pkg.Read<int32_t>();
        for (int i=0; i < count; i++) {
            auto veg = std::make_unique<Foliage>();

            auto prefabName = pkg.Read<std::string>();

            veg->m_prefab = &PrefabManager()->RequirePrefabByName(prefabName);

            veg->m_biome = (Biome) pkg.Read<int32_t>();
            veg->m_biomeArea = (BiomeArea) pkg.Read<int32_t>();
            veg->m_radius = pkg.Read<float>();
            veg->m_min = pkg.Read<float>();
            veg->m_max = pkg.Read<float>();
            veg->m_minTilt = pkg.Read<float>();
            veg->m_maxTilt = pkg.Read<float>();
            veg->m_groupRadius = pkg.Read<float>();
            veg->m_forcePlacement = pkg.Read<bool>();
            veg->m_groupSizeMin = pkg.Read<int32_t>();
            veg->m_groupSizeMax = pkg.Read<int32_t>();
            veg->m_scaleMin = pkg.Read<float>();
            veg->m_scaleMax = pkg.Read<float>();
            veg->m_randTilt = pkg.Read<float>();
            veg->m_blockCheck = pkg.Read<bool>();
            veg->m_minAltitude = pkg.Read<float>();
            veg->m_maxAltitude = pkg.Read<float>();
            veg->m_minOceanDepth = pkg.Read<float>();
            veg->m_maxOceanDepth = pkg.Read<float>();
            veg->m_terrainDeltaRadius = pkg.Read<float>();
            veg->m_minTerrainDelta = pkg.Read<float>();
            veg->m_maxTerrainDelta = pkg.Read<float>();
            veg->m_inForest = pkg.Read<bool>();
            veg->m_forestTresholdMin = pkg.Read<float>();
            veg->m_forestTresholdMax = pkg.Read<float>();
            veg->m_snapToWater = pkg.Read<bool>();
            veg->m_snapToStaticSolid = pkg.Read<bool>();
            veg->m_groundOffset = pkg.Read<float>();
            veg->m_chanceToUseGroundTilt = pkg.Read<float>();
            veg->m_minVegetation = pkg.Read<float>();
            veg->m_maxVegetation = pkg.Read<float>();

            m_foliage.push_back(std::move(veg));
        }

        LOG_INFO(LOGGER, "Loaded {} vegetation", count);
#endif
    }

#if VH_IS_ON(VH_ZONE_GENERATION)
    ZONE_CTRL_PREFAB = PrefabManager()->GetPrefab(Hashes::Object::_ZoneCtrl);
    LOCATION_PROXY_PREFAB = PrefabManager()->GetPrefab(Hashes::Object::LocationProxy);

    if (!ZONE_CTRL_PREFAB || !LOCATION_PROXY_PREFAB)
        throw std::runtime_error("prefabs missing");
#endif

    RouteManager()->Register(Hashes::Routed::C2S_SetGlobalKey, [this](Peer* peer, std::string_view name) {
        // TODO limit keys based on peer and the creature killed
        //  have this as a compiler macro
        if (m_globalKeys.insert(name).second)
            SendGlobalKeys(); // Notify clients
    });

    RouteManager()->Register(Hashes::Routed::C2S_RemoveGlobalKey, [this](Peer* peer, std::string_view name) {
        // TODO limit keys based on peer and the creature killed
        if (m_globalKeys.erase(name))
            SendGlobalKeys(); // Notify clients
    });

    RouteManager()->Register(Hashes::Routed::C2S_RequestIcon, [this](Peer* peer, std::string_view locationName, Vector3f point, std::string_view pinName, int pinType, bool showMap) {
#if VH_IS_ON(VH_ZONE_GENERATION)        
        if (auto&& instance = GetNearestFeature(locationName, point)) {
            LOG_INFO(LOGGER, "Found location: '{}'", locationName);
            RouteManager()->Invoke(peer->m_characterID.GetOwner(),
                Hashes::Routed::S2C_ResponseIcon, 
                pinName, 
                pinType, 
                instance->m_pos, 
                showMap
            );
        }
        else {
            LOG_INFO(LOGGER, "Failed to find location: '{}'", locationName);
        }
#else
        Vector3f out;
        if (GetNearestFeature(locationName, point, out)) {
            LOG_INFO(LOGGER, "Found location: '{}'", locationName);
            RouteManager()->Invoke(peer->m_uuid,
                Hashes::Routed::S2C_ResponseIcon,
                pinName,
                pinType,
                out,
                showMap
            );
        }
        else {
            LOG_INFO(LOGGER, "Failed to find location: '{}'", locationName);
        }
#endif
    });

    RouteManager()->Register(Hashes::Routed::S2C_ResponsePing, [](Peer* peer, float time) {
        peer->Route(Hashes::Routed::Pong, time);
    });
}

// private
void IZoneManager::OnNewPeer(Peer& peer) {
    SendGlobalKeys(peer);
    SendLocationIcons(peer);
}

bool IZoneManager::ZonesOverlap(ZoneID zone, Vector3f refPoint) {
    return ZonesOverlap(zone,
        WorldToZonePos(refPoint));
}

bool IZoneManager::ZonesOverlap(ZoneID zone, ZoneID refCenterZone) {
    int num = NEAR_ACTIVE_AREA - 1;
    return zone.x >= refCenterZone.x - num
        && zone.x <= refCenterZone.x + num
        && zone.y <= refCenterZone.y + num
        && zone.y >= refCenterZone.y - num;
}

bool IZoneManager::IsPeerNearby(ZoneID zone, OWNER_t uid) {
    auto&& peer = NetManager()->GetPeerByUUID(uid);
    //assert((peer && uid) || (!peer && uid)); // makes sure no peer is ever found with 0 uid
    if (peer) return ZonesOverlap(zone, peer->m_pos);
    return false;
}

// private
void IZoneManager::SendGlobalKeys() {
    RouteManager()->InvokeAll(Hashes::Routed::S2C_UpdateKeys, m_globalKeys);
}

void IZoneManager::SendGlobalKeys(Peer& peer) {
    //RouteManager()->Invoke(peer, Hashes::Routed::S2C_UpdateKeys, m_globalKeys);
    peer.Route(Hashes::Routed::S2C_UpdateKeys, m_globalKeys);
}

#if VH_IS_ON(VH_ZONE_GENERATION)
// private
void IZoneManager::SendLocationIcons() {
    BYTES_t bytes;
    DataWriter writer(bytes);

    auto&& icons = GetFeatureIcons();

    writer.Write<int32_t>(icons.size());
    for (auto&& instance : icons) {
        writer.Write(instance.get().m_pos);
        writer.Write(std::string_view(instance.get().m_feature.get().m_name));
    }

    RouteManager()->InvokeAll(Hashes::Routed::S2C_UpdateIcons, bytes);
}
#endif

// private
void IZoneManager::SendLocationIcons(Peer& peer) {
    LOG_INFO(LOGGER, "Sending location icons to {}", peer.m_name);

#if VH_IS_ON(VH_ZONE_GENERATION)
    BYTES_t bytes;
    DataWriter writer(bytes);

    auto&& icons = GetFeatureIcons();

    writer.Write<int32_t>(icons.size());
    for (auto&& instance : icons) {
        writer.Write(instance.get().m_pos);
        writer.Write(std::string_view(instance.get().m_feature.get().m_name));
    }

    peer.Route(Hashes::Routed::S2C_UpdateIcons, bytes);
#else
    peer.SubRoute(Hashes::Routed::S2C_UpdateIcons, [this](DataWriter& writer) {
        writer.Write<int32_t>(1); // dummy count
        for (auto&& pair : m_generatedFeatures) {
            // We only care about StartTemple
            //if (m_features[pair.second.first] == std::string_view(m_features[0])) {
            if (pair.second.first == 0) {
                writer.Write(pair.second.second); // pos
                writer.Write(m_features[pair.second.first]); // name
                return;
            }
        }
        assert(false);
    });
#endif
}

// public
void IZoneManager::Save(DataWriter& pkg) {
#if VH_IS_ON(VH_ZONE_GENERATION)
    pkg.Write<int32_t>(m_generatedZones.size());
    for (auto&& zone : m_generatedZones) {
        pkg.Write<int32_t>(zone.x);
        pkg.Write<int32_t>(zone.y);
    }
#else
    LOG_WARNING(LOGGER, "Saving while VH_ZONE_GENERATION:0 is not fully portable");

    pkg.Write<int32_t>(0); // 0 count
#endif
    pkg.Write<int32_t>(0); // PGW
    pkg.Write(VConstants::LOCATION);
    pkg.Write(m_globalKeys);
    pkg.Write(true);
    pkg.Write<int32_t>(m_generatedFeatures.size());
    for (auto&& pair : m_generatedFeatures) {
#if VH_IS_ON(VH_ZONE_GENERATION)
        auto&& inst = pair.second;
        auto&& location = inst->m_feature.get();

        pkg.Write(std::string_view(location.m_name));
        pkg.Write(inst->m_pos);
        pkg.Write(m_generatedZones.contains(WorldToZonePos(inst->m_pos)));
#else
        static_assert(std::is_same_v<Vector3f, decltype(decltype(std::remove_cvref_t<decltype(pair)>::second)::second)>);

        pkg.Write(m_features[pair.second.first]);
        pkg.Write(pair.second.second);
        pkg.Write(true);
#endif
    }
}

// public
void IZoneManager::Load(DataReader& reader, int32_t version) {
    {
        auto count = reader.Read<uint32_t>();
        for (decltype(count) i = 0; i < count; i++) {
            auto x = reader.Read<int32_t>();
            auto y = reader.Read<int32_t>();
#if VH_IS_ON(VH_ZONE_GENERATION)
            m_generatedZones.insert(ZoneID(static_cast<int16_t>(x), int16_t(y)));
#endif // VH_ZONE_GENERATION
        }
    }

#if VH_IS_ON(VH_LEGACY_WORLD_LOADING)
    if (version >= 13) 
#endif // VH_LEGACY_WORLD_LOADING
    {
        reader.Read<int32_t>(); // PGW
        const auto locationVersion = (version >= 21) ? reader.Read<int32_t>() : 0; // 26

#if VH_IS_ON(VH_LEGACY_WORLD_LOADING)
        if (version >= 14)
#endif // VH_LEGACY_WORLD_LOADING
        {
            m_globalKeys = reader.Read<decltype(m_globalKeys)>();

#if VH_IS_ON(VH_LEGACY_WORLD_LOADING)
            if (version >= 18) 
#endif // VH_LEGACY_WORLD_LOADING
            {
#if VH_IS_ON(VH_LEGACY_WORLD_LOADING)
                if (version >= 20) 
#endif // VH_LEGACY_WORLD_LOADING
                {
                    reader.Read<bool>(); // locationsGenerated
                }

                auto count = reader.Read<int32_t>();
                for (decltype(count) i = 0; i < count; i++) {
                    auto text = reader.Read<std::string_view>();
                    auto pos = reader.Read<Vector3f>();

#if VH_IS_ON(VH_LEGACY_WORLD_LOADING)
                    bool generated = (version >= 19) ? reader.Read<bool>() : false;
#else // !VH_LEGACY_WORLD_LOADING
                    bool generated = reader.Read<bool>();
#endif // VH_LEGACY_WORLD_LOADING

#if VH_IS_ON(VH_ZONE_GENERATION)
                    auto&& location = GetFeature(text);
                    if (location) {
                        m_generatedFeatures[WorldToZonePos(pos)] = 
                            std::make_unique<Feature::Instance>(*location, pos);
                    }
                    else {
                        LOG_ERROR(LOGGER, "Unknown feature '{}'", text);
                    }
#else // !VH_ZONE_GENERATION
                    static_assert(std::numeric_limits<uint8_t>::max() > m_features.size());
                    for (uint8_t i = 0; i < m_features.size(); i++) {
                        if (text == std::string_view(m_features[i])) {
                            // register
                            m_generatedFeatures[WorldToZonePos(pos)] 
                                = std::pair<uint8_t, Vector3f>(i, pos);
                            break;
                        }
                    }
#endif // VH_ZONE_GENERATION
                }

                LOG_INFO(LOGGER, "Loaded {}/{} feature instances ", m_generatedFeatures.size(), count);

                if (locationVersion != VConstants::LOCATION) {
                    // regenerate features?
                    //m_generatedFeatures.clear();
                }
            }
        }
    }
}

// private
void IZoneManager::Update() {
    ZoneScoped;

#if VH_IS_ON(VH_ZONE_GENERATION)
    // TODO 100ms seems a tad too frequent
    //  peers dont even move that fast
    if (VUtils::run_periodic<struct periodic_zone_generation>(100ms)) {
        for (auto&& peer : NetManager()->GetPeers()) {
            if (!peer->IsGated())
            {
                // It turns out that zdos generated by whatever ids are different from packets... 
                //  although almost everything is the same...
                //  So the best ACTUAL way to capture the world would be to pre-generate the entire world
                //  then disable the world generation during playback
                TryGenerateNearbyZones(peer->m_pos);
            }
        }
    }
#endif
}

/*
void IZoneManager::RegenerateZone(const ZoneID& zone) {
    for (auto&& zdo : ZDOManager()->GetZDOs(zone, 0, Prefab::Flag::NONE, Prefab::Flag::Player))
        ZDOManager()->DestroyZDO(zdo);

    //m_generatedZones.erase(zone);
    PopulateZone(HeightmapManager()->GetHeightmap(zone));
    //m_generatedZones.insert(zone);
}*/
#if VH_IS_ON(VH_ZONE_GENERATION)
// Rename?
void IZoneManager::TryGenerateNearbyZones(Vector3f refPoint) {
    auto zone = WorldToZonePos(refPoint);

    // Prioritize center zone
    if (!TryGenerateZone(zone)) {

        // If spawning fails, spawn other neighboring zones
        auto num = NEAR_ACTIVE_AREA + DISTANT_ACTIVE_AREA;
        for (int z = zone.y - num; z <= zone.y + num; z++) {
            for (int x = zone.x - num; x <= zone.x + num; x++) {

                // Skip center zone
                if (x == zone.x && z == zone.y)
                    continue;

                TryGenerateZone(ZoneID( x, z ));
            }
        }
    }
}

bool IZoneManager::GenerateZone(ZoneID zone) {
    if ((zone.x > -WORLD_RADIUS_IN_ZONES && zone.y > -WORLD_RADIUS_IN_ZONES
        && zone.x < WORLD_RADIUS_IN_ZONES && zone.y < WORLD_RADIUS_IN_ZONES)) 
    {
        auto&& pair = m_generatedZones.insert(zone);
        if (pair.second) {
            PopulateZone(HeightmapManager()->GetHeightmap(zone));
            return true;
        }
    }
    return false;
}

bool IZoneManager::TryGenerateZone(ZoneID zone) {
    if ((zone.x >= -WORLD_RADIUS_IN_ZONES && zone.y >= -WORLD_RADIUS_IN_ZONES
        && zone.x <= WORLD_RADIUS_IN_ZONES && zone.y <= WORLD_RADIUS_IN_ZONES)
        && !IsZoneGenerated(zone)) {
        if (auto heightmap = HeightmapManager()->PollHeightmap(zone)) {
            m_generatedZones.insert(zone);

            PopulateZone(*heightmap);

            return true;
        }
    }
    return false;
}

void IZoneManager::PopulateZone(Heightmap &heightmap) {
    ZoneScoped;

//#if VH_IS_ON(VH_ZONE_GENERATION)
    std::vector<ClearArea> m_tempClearAreas;

    if (VH_SETTINGS.worldFeatures)
        m_tempClearAreas = TryGenerateFeature(heightmap.GetZone());

    if (VH_SETTINGS.worldVegetation)
        PopulateFoliage(heightmap, m_tempClearAreas);

    if (VH_SETTINGS.worldCreatures) {
        ZDOManager()->Instantiate(*ZONE_CTRL_PREFAB, 
            ZoneToWorldPos(heightmap.GetZone()));
    }
//#endif // VH_OPTION_ENABLE_ZONE_GENERATION
}

void IZoneManager::PopulateZone(ZoneID zone) {
    this->PopulateZone(HeightmapManager()->GetHeightmap(zone));
}



// private
Vector3f IZoneManager::GetRandomPointInRadius(VUtils::Random::State& state, Vector3f center, float radius) {
    float f = state.NextFloat() * PI * 2.f;
    float num = state.Range(0.f, radius);
    return center + Vector3f(std::sin(f) * num, 0.f, std::cos(f) * num);
}

// private
void IZoneManager::PopulateFoliage(Heightmap& heightmap, const std::vector<ClearArea>& clearAreas) {
    auto&& zoneID = heightmap.GetZone();

    const Vector3f center = ZoneToWorldPos(zoneID);

    const auto seed = GeoManager()->GetSeed();

    //Biome biomes = GeoManager()->GetBiomes(center.x, center.z);

    std::vector<ClearArea> placedAreas;

    for (const auto& zoneVegetation : m_foliage) {
        // This ultimately serves as a large precheck, assuming heightmap were being used (which it no longer seems good)
        if (!heightmap.HaveBiome(zoneVegetation->m_biome))
            continue;

        // TODO make unique per vegetation instance
        // this state will be the same for all same vegetation within a given zone, in a given world
        VUtils::Random::State state(
            seed + zoneID.x * 4271 + zoneID.y * 9187 + zoneVegetation->m_prefab->m_hash
        );

        int32_t num3 = 1;
        // max is used for both chance, and quantity in conjunction with min 
        if (zoneVegetation->m_max < 1) {
            if (state.NextFloat() > zoneVegetation->m_max) {
                continue;
            }
        }
        else {
            num3 = state.Range((int32_t) zoneVegetation->m_min, (int32_t) zoneVegetation->m_max + 1);
        }

        // flag should always be true, all vegetation seem to always have a NetView
        //bool flag = zoneVegetation.m_prefab.GetComponent<ZNetView>() != null;
        float maxTilt = std::cosf(zoneVegetation->m_maxTilt * PI / 180.f);
        float minTilt = std::cosf(zoneVegetation->m_minTilt * PI / 180.f);
        float num6 = ZONE_SIZE * .5f - zoneVegetation->m_groupRadius;
        const int spawnAttempts = zoneVegetation->m_forcePlacement ? (num3 * 50) : num3;
        int32_t numSpawned = 0;
        for (int i = 0; i < spawnAttempts; i++) {
            float vx = state.Range(center.x - num6, center.x + num6);
            float vz = state.Range(center.z - num6, center.z + num6);

            Vector3f basePos(vx, 0., vz);

            const auto groupCount = state.Range(zoneVegetation->m_groupSizeMin, zoneVegetation->m_groupSizeMax + 1);
            bool generated = false;
            for (int32_t j = 0; j < groupCount; j++) {

                Vector3f pos = (j == 0) ? basePos
                    : GetRandomPointInRadius(state, basePos, zoneVegetation->m_groupRadius);

                // random rotations
                float rot_y = state.Range(0, 360);
                float scale = state.Range(zoneVegetation->m_scaleMin, zoneVegetation->m_scaleMax);
                float rot_x = state.Range(-zoneVegetation->m_randTilt, zoneVegetation->m_randTilt);
                float rot_z = state.Range(-zoneVegetation->m_randTilt, zoneVegetation->m_randTilt);

                // Use a method similar to clear area with rectangular regions
                //if (!zoneVegetation->m_blockCheck
                    //|| !IsBlocked(vector2)) // no unity   \_(^.^)_/
                {

                    Vector3f normal;
                    Biome biome;
                    BiomeArea biomeArea;
                    Heightmap &otherHeightmap = GetGroundData(pos, normal, biome, biomeArea);

                    if (!((std::to_underlying(zoneVegetation->m_biome) & std::to_underlying(biome))
                        && (std::to_underlying(zoneVegetation->m_biomeArea) & std::to_underlying(biomeArea))))
                        continue;

                    // Mistlands only
                    //  A huge amount of mistlands are angled rock spires (not part of terrain),
                    //  so vegetation spanws on top of these,
                    // I do not have a way to implement this. The client, however does, which updating
                    // objects with static physics every so often while nearby
                    // 
                    //float y2 = 0;
                    //Vector3f vector4;
                    //if (zoneVegetation->m_snapToStaticSolid && GetStaticSolidHeight(vector2, y2, vector4)) {
                    //    vector2.y = y2;
                    //    vector3 = vector4;
                    //}

                    float waterDiff = pos.y - WATER_LEVEL;
                    if (waterDiff < zoneVegetation->m_minAltitude || waterDiff > zoneVegetation->m_maxAltitude)
                        continue;

                    // Mistlands only
                    // TODO might be affecting mist (probably is? just a hunch)
                    if (zoneVegetation->m_minVegetation != zoneVegetation->m_maxVegetation) {
                        float vegetationMask = otherHeightmap.GetVegetationMask(pos);
                        if (vegetationMask > zoneVegetation->m_maxVegetation || vegetationMask < zoneVegetation->m_minVegetation) {
                            continue;
                        }
                    }

                    if (zoneVegetation->m_minOceanDepth != zoneVegetation->m_maxOceanDepth) {
                        float oceanDepth = otherHeightmap.GetOceanDepth(pos);
                        if (oceanDepth < zoneVegetation->m_minOceanDepth || oceanDepth > zoneVegetation->m_maxOceanDepth) {
                            continue;
                        }
                    }

                    if (normal.y >= maxTilt && normal.y <= minTilt) 
                    {

                        if (zoneVegetation->m_terrainDeltaRadius > 0) {
                            float num12;
                            Vector3f vector5;
                            GetTerrainDelta(state, pos, zoneVegetation->m_terrainDeltaRadius, num12, vector5);
                            if (num12 > zoneVegetation->m_maxTerrainDelta || num12 < zoneVegetation->m_minTerrainDelta) {
                                continue;
                            }
                        }

                        if (zoneVegetation->m_inForest) {
                            float forestFactor = GeoManager()->GetForestFactor(pos);
                            if (forestFactor < zoneVegetation->m_forestTresholdMin || forestFactor > zoneVegetation->m_forestTresholdMax) {
                                continue;
                            }
                        }

                        if (!InsideClearArea(clearAreas, pos) 
                            && (zoneVegetation->m_radius == 0 || !OverlapsClearArea(placedAreas, pos, zoneVegetation->m_radius))) // custom
                        {

                            if (zoneVegetation->m_snapToWater)
                                pos.y = WATER_LEVEL;

                            pos.y += zoneVegetation->m_groundOffset;
                            Quaternion rotation;

                            if (zoneVegetation->m_chanceToUseGroundTilt > 0
                                && state.NextFloat() <= zoneVegetation->m_chanceToUseGroundTilt) {
                                auto rotation2 = Quaternion::Euler(0, rot_y, 0);
                                rotation = Quaternion::LookRotation(
                                    normal.Cross(rotation2 * Vector3f::Forward()),
                                    normal
                                );
                            }
                            else {
                                rotation = Quaternion::Euler(rot_x, rot_y, rot_z);
                            }

                            // TODO rotation during generation are not correct
                            //  this is proven because of the correct world loaded zdos, 
                            //  however new generated zone zdos are not correctly rotated

                            auto &&zdo = ZDOManager()->Instantiate(*zoneVegetation->m_prefab, pos);
                            zdo.SetRotation(rotation);

                            // basically any solid objects cannot be overlapped
                            //  the exception to this rule is mist, swamp_beacon, silvervein... basically non-physical vegetation
                            if (zoneVegetation->m_radius > 0)
                                placedAreas.push_back({ pos, zoneVegetation->m_radius });

                            if (scale != zoneVegetation->m_prefab->m_localScale.x) {
                                zdo.SetLocalScale(Vector3f(scale, scale, scale), true);
                            }

                            generated = true;
                        }
                    }
                }
            }

            if (generated) {
                numSpawned++;
            }

            if (numSpawned >= num3) {
                break;
            }
        }   
    }
}


// private
bool IZoneManager::InsideClearArea(const std::vector<ClearArea>& areas, Vector3f point) {
    for (auto&& clearArea : areas) {
        if (point.x > clearArea.m_center.x - clearArea.m_semiWidth
            && point.x < clearArea.m_center.x + clearArea.m_semiWidth
            && point.z > clearArea.m_center.z - clearArea.m_semiWidth
            && point.z < clearArea.m_center.z + clearArea.m_semiWidth) {
            return true;
        }
    }
    return false;
}

bool IZoneManager::OverlapsClearArea(const std::vector<ClearArea>& areas, Vector3f point, float radius) {
    for (auto&& area : areas) {

        float d = VUtils::Math::SqDistance(point.x, point.z, area.m_center.x, area.m_center.z);
        float rd = area.m_semiWidth + radius;

        if (d < rd * rd)
            return true;
    }
    return false;
}

// private
const IZoneManager::Feature* IZoneManager::GetFeature(std::string_view name) {
    for (auto&& feature : m_features) {
        if (feature->m_name == name)
            return feature.get();
    }
    return nullptr;
}

// public
// call from within ZNet.init or earlier...
void IZoneManager::PostGeoInit() {
    // Will be empty if world failed to load
    if (!m_generatedFeatures.empty())
        return;

    // Crucially important Location
    // So check that it exists period
    auto&& spawnLoc = GetFeature("StartTemple");
    if (!spawnLoc)
        throw std::runtime_error("World spawnpoint missing (StartTemple)");

    if (!VH_SETTINGS.worldFeatures) {
        LOG_WARNING(LOGGER, "Location generation is disabled");
        PrepareFeatures(*spawnLoc);
    }
    else {
        auto now(steady_clock::now());

        // Already presorted by priority
        for (auto&& loc : m_features) {
            PrepareFeatures(*loc.get());
        }

        LOG_INFO(LOGGER, "Location generation took {}s", duration_cast<seconds>(steady_clock::now() - now).count());
    }

    if (VH_SETTINGS.worldPregenerate
        && m_generatedZones.empty()) 
    {
        auto now(steady_clock::now());
        int prevCount = 0;

        //LOG_WARNING(LOGGER, "Pregenerating world...");
        LOG_WARNING(LOGGER, "Pregeneration takes up a lot of memory and resources during and after generation!");
        LOG_WARNING(LOGGER, "This setting is experimental and unoptimized! (I dont know why :(");
        LOG_WARNING(LOGGER, "This will take a while!");
        while (m_generatedZones.size() < WORLD_RADIUS_IN_ZONES*2* WORLD_RADIUS_IN_ZONES*2) {
            for (int16_t y = -WORLD_RADIUS_IN_ZONES; y <= WORLD_RADIUS_IN_ZONES; y++) {
                for (int16_t x = -WORLD_RADIUS_IN_ZONES; x <= WORLD_RADIUS_IN_ZONES; x++) {
                    TryGenerateZone(ZoneID(x, y));
                    
                    if (VUtils::run_periodic<struct periodic_pregen_stats>(3s)) {
                        std::string lines;
                        // print a cool grid
                        for (int16_t iy = -WORLD_RADIUS_IN_ZONES; iy <= WORLD_RADIUS_IN_ZONES; iy += 6) {
                            for (int16_t ix = -WORLD_RADIUS_IN_ZONES; ix <= WORLD_RADIUS_IN_ZONES; ix += 6) {
                                if (std::abs(ix - x) < 3 && std::abs(iy - y) < 3) {
                                    lines += COLOR_GOLD;
                                }
                                else if (m_generatedZones.contains({ ix, iy })) {
                                    lines += COLOR_GREEN;
                                }
                                else {
                                    lines += COLOR_GRAY;
                                }
                                lines += "O ";
                            }
                            lines += COLOR_RESET + std::string("\n");
                        }

                        LOG_INFO(LOGGER, "Zone progress: \n{}", lines);
                        LOG_WARNING(LOGGER, "{}/{} zones generated \t({} z/s)",
                            m_generatedZones.size(), (WORLD_RADIUS_IN_ZONES * 2 * WORLD_RADIUS_IN_ZONES * 2),
                            ((m_generatedZones.size() - prevCount) / 3));
                        prevCount = m_generatedZones.size();
                    }
                }
            }
        }

        LOG_WARNING(LOGGER, "Pregeneration took {}s", duration_cast<seconds>(steady_clock::now() - now).count());
    }
}

// private
void IZoneManager::PrepareFeatures(const Feature& feature) {
    int spawnedLocations = 0;

    // CountNrOfLocation: inlined
    for (auto&& inst : m_generatedFeatures) {
        if (inst.second->m_feature.get() == feature) // better to compare locations itself rather than name
            spawnedLocations++;
    }

    unsigned int errLocations = 0;
    unsigned int errCenterDistances = 0;
    unsigned int errNoneBiomes = 0;
    unsigned int errBiomeArea = 0;
    unsigned int errAltitude = 0;
    unsigned int errForestFactor = 0;
    unsigned int errSimilarLocation = 0;
    unsigned int errTerrainDelta = 0;

    VUtils::Random::State state(GeoManager()->GetSeed() + feature.m_hash);
    const float locationRadius = std::max(feature.m_exteriorRadius, feature.m_interiorRadius);

    float range = feature.m_centerFirst ? feature.m_minDistance : 10000;

    for (int a = 0; a < feature.m_spawnAttempts && spawnedLocations < feature.m_quantity; a++) {
        auto randomZone = GetRandomZone(state, range);
        if (feature.m_centerFirst)
            range++;

        if (m_generatedFeatures.contains(randomZone))
            errLocations++;
        else {
            auto zonePos = ZoneToWorldPos(randomZone);
            BiomeArea biomeArea = GeoManager()->GetBiomeArea(zonePos);

            if (!(std::to_underlying(feature.m_biomeArea) & std::to_underlying(biomeArea)))
                errBiomeArea++;
            else {
                for (int i = 0; i < 20; i++) {
                    auto randomPointInZone = GetRandomPointInZone(state, randomZone, locationRadius);

                    float magnitude = randomPointInZone.Magnitude();
                    if ((feature.m_minDistance != 0 && magnitude < feature.m_minDistance)
                        || (feature.m_maxDistance != 0 && magnitude > feature.m_maxDistance)) {
                        errCenterDistances++;
                    } 
                    else {
                        Biome biome = GeoManager()->GetBiome(randomPointInZone);

                        if (!(std::to_underlying(biome) & std::to_underlying(feature.m_biome)))
                            errNoneBiomes++;
                        else {
                            randomPointInZone.y = GeoManager()->GetHeight(randomPointInZone.x, randomPointInZone.z);
                            float waterDiff = randomPointInZone.y - WATER_LEVEL;
                            if (waterDiff < feature.m_minAltitude || waterDiff > feature.m_maxAltitude)
                                errAltitude++;
                            else {
                                if (feature.m_inForest) {
                                    float forestFactor = GeoManager()->GetForestFactor(randomPointInZone);
                                    if (forestFactor < feature.m_forestTresholdMin || forestFactor > feature.m_forestTresholdMax) {
                                        errForestFactor++;
                                        continue;
                                    }
                                }

                                float delta = 0;
                                Vector3f vector;
                                GeoManager()->GetTerrainDelta(state, randomPointInZone, feature.m_exteriorRadius, delta, vector);
                                if (delta > feature.m_maxTerrainDelta
                                    || delta < feature.m_minTerrainDelta)
                                    errTerrainDelta++;
                                else {
                                    if (feature.m_minDistanceFromSimilar <= 0
                                        || !HaveLocationInRange(feature, randomPointInZone)) {
                                        auto zone = WorldToZonePos(randomPointInZone);

                                        m_generatedFeatures[zone] = std::make_unique<Feature::Instance>(
                                            feature,
                                            randomPointInZone
                                        );

                                        spawnedLocations++;
                                        break;
                                    }
                                    errSimilarLocation++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (spawnedLocations < feature.m_quantity) {
        LOG_WARNING(LOGGER, "Failed to place all {}, placed {}/{}", feature.m_name, spawnedLocations, feature.m_quantity);

        //LOG(ERROR) << "errLocations " << errLocations;
        //LOG(ERROR) << "errCenterDistances " << errCenterDistances;
        //LOG(ERROR) << "errNoneBiomes " << errNoneBiomes;
        //LOG(ERROR) << "errBiomeArea " << errBiomeArea;
        //LOG(ERROR) << "errAltitude " << errAltitude;
        //LOG(ERROR) << "errForestFactor " << errForestFactor;
        //LOG(ERROR) << "errSimilarLocation " << errSimilarLocation;
        //LOG(ERROR) << "errTerrainDelta " << errTerrainDelta;
    }
}

bool IZoneManager::HaveLocationInRange(const Feature& loc, Vector3f p) {
    for (auto&& pair : m_generatedFeatures) {
        auto&& locationInstance = pair.second;
        auto&& location = locationInstance->m_feature.get();

        if ((location == loc 
            || (!loc.m_group.empty() && loc.m_group == location.m_group)) 
            && locationInstance->m_pos.Distance(p) < loc.m_minDistanceFromSimilar) // TODO use sqdist
        {
            return true;
        }
    }
    return false;
}

Vector3f IZoneManager::GetRandomPointInZone(VUtils::Random::State& state, ZoneID zone, float locationRadius) {
    auto pos = ZoneToWorldPos(zone);
    float num = ZONE_SIZE / 2.f;
    float x = state.Range(-num + locationRadius, num - locationRadius);
    float z = state.Range(-num + locationRadius, num - locationRadius);
    return pos + Vector3f(x, 0.f, z);
}

ZoneID IZoneManager::GetRandomZone(VUtils::Random::State& state, float range) {
    int num = (int32_t)range / (int32_t)ZONE_SIZE;
    ZoneID zone;
    do {
        float x = state.Range(-num, num);
        float y = state.Range(-num, num);
        zone = ZoneID(x, y);
    } while (ZoneToWorldPos(zone).Magnitude() >= 10000);
    return zone;
}

// private
std::vector<IZoneManager::ClearArea> IZoneManager::TryGenerateFeature(ZoneID zoneID)
{
    auto now(steady_clock::now());

    std::vector<ClearArea> clearAreas;

    auto&& find = m_generatedFeatures.find(zoneID);
    if (find != m_generatedFeatures.end()) {
        auto&& locationInstance = find->second;
        auto&& location = locationInstance->m_feature.get();

        Vector3f position = locationInstance->m_pos;
        //Vector3f vector;
        //Biome biome;
        //BiomeArea biomeArea;
        //Heightmap *heightmap = GetGroundData(position, vector, biome, biomeArea);

        // m_snapToWater is Mistlands only
        if (location.m_snapToWater)
            position.y = WATER_LEVEL;

        if (location.m_clearArea)
            clearAreas.push_back({position, location.m_exteriorRadius });

        Quaternion rot;

        // slopeRotation is Mistlands only
        //if (locationInstance.m_feature->m_slopeRotation) {
        //    float num;
        //    Vector3f vector2;
        //    GetTerrainDelta(position, locationInstance.m_feature->m_exteriorRadius, num, vector2);
        //    Vector3f forward(vector2.x, 0.f, vector2.z);
        //    forward.Normalize();
        //    rot = Quaternion::LookRotation(forward);
        //    assert(false);
        //    //Vector3f eulerAngles = rot.eulerAngles;
        //    //eulerAngles.y = round(eulerAngles.y / 22.5f) * 22.5f;
        //    //rot.eulerAngles = eulerAngles;
        //}

        if (location.m_randomRotation) {
            rot = Quaternion::Euler(0, VUtils::Random::State().Range(0, 16) * 22.5f, 0);
        }

        HASH_t seed = GeoManager()->GetSeed() + zoneID.x * 4271 + zoneID.y * 9187;
        GenerateFeature(location, seed, position, rot);

        LOG_INFO(LOGGER, "Placed '{}' in zone {} ({})", location.m_name, zoneID, position);

        // Remove all other Haldor locations, etc...
        if (location.m_unique) {
            RemoveUngeneratedFeatures(location);
        }

        // TODO determine whether this method requires a special Peer* method
        if (location.m_iconPlaced) {
            SendLocationIcons();
        }
    }

    return clearAreas;
}

// private
void IZoneManager::RemoveUngeneratedFeatures(const Feature& feature) {
    int count = 0;
    for (auto&& itr = m_generatedFeatures.begin(); itr != m_generatedFeatures.end();) {
        auto&& instance = itr->second;
        auto&& otherFeature = instance->m_feature.get();
        if (!IsZoneGenerated(WorldToZonePos(instance->m_pos))
            && otherFeature == feature) 
        {
            itr = m_generatedFeatures.erase(itr);
            count++;
        }
        else ++itr;
    }

    LOG_INFO(LOGGER, "Removed {} unplaced '{}'", count, feature.m_name);
}

// private
void IZoneManager::GenerateFeature(const Feature& location, HASH_t seed, Vector3f pos, Quaternion rot) {
    VUtils::Random::State state(seed);

    // TODO is random damage really important?
    //  I don't think it drasically affects much
    //WearNTear.m_randomInitialDamage = location.m_feature.m_applyRandomDamage;
    //for (auto&& znetView2 : location.m_netViews) {

    for (auto&& piece : location.m_pieces) {
        // Dungeon hierarchy:
        //  Location
        //      Interior (InteriorTransform)
        //          DG_(dungeon)

        if (!(VH_SETTINGS.dungeonsEnabled && piece.m_prefab->AllFlagsPresent(Prefab::Flag::DUNGEON))) {
            auto&& zdo = ZDOManager()->Instantiate(*piece.m_prefab, pos + rot * piece.m_pos);
            zdo.SetRotation(rot * piece.m_rot);
        } else {
            auto&& dungeon = DungeonManager()->RequireDungeon(piece.m_prefab->m_hash);

            std::optional<ZDO> zdo;

            if (dungeon.m_interiorPosition != Vector3f::Zero()) {

                ZoneID zone = WorldToZonePos(pos);
                Vector3f zonePos = ZoneToWorldPos(zone);

                Vector3f piecePos = zonePos
                    + dungeon.m_interiorPosition // ( 0, 5000, 0 )
                    + dungeon.m_originalPosition; // minor position change (usually height and a horizontal axis)

                piecePos.y = dungeon.m_interiorPosition.y + pos.y;

                zdo = ZDOManager()->Instantiate(*piece.m_prefab, piecePos);
                zdo->SetRotation(piece.m_rot);
            }
            else {
                zdo = ZDOManager()->Instantiate(*piece.m_prefab, pos + rot * piece.m_pos);
                zdo->SetRotation(rot * piece.m_rot);
            }

            assert(zdo);

            // Only add real sky dungeon
            if (zdo->Position().y > 4000)
                DungeonManager()->m_dungeonInstances.push_back(zdo->ID());

            DungeonManager()->Generate(dungeon, *zdo);
        }
    }
    //WearNTear.m_randomInitialDamage = false;

    // https://www.reddit.com/r/valheim/comments/xns70u/comment/ipv77ca/?utm_source=share&utm_medium=web2x&context=3
    // https://www.reddit.com/r/valheim/comments/r6mv1q/comment/hmutgdl/?utm_source=share&utm_medium=web2x&context=3
    // LocationProxy are client-side generated models
    GenerateLocationProxy(location, seed, pos, rot);
}

// could be inlined...
// private
void IZoneManager::GenerateLocationProxy(const Feature& location, HASH_t seed, Vector3f pos, Quaternion rot) {
    auto &&zdo = ZDOManager()->Instantiate(*LOCATION_PROXY_PREFAB, pos);
    zdo.SetRotation(rot);
    
    zdo.Set(Hashes::ZDO::ZoneManager::LOCATION, location.m_hash);
    zdo.Set(Hashes::ZDO::ZoneManager::SEED, seed);
}

// public
// TODO make this batch update every time a new location is added or whatever
std::list<std::reference_wrapper<IZoneManager::Feature::Instance>> IZoneManager::GetFeatureIcons() {
    std::list<std::reference_wrapper<IZoneManager::Feature::Instance>> result;

    for (auto&& pair : m_generatedFeatures) {
        auto&& instance = pair.second;
        auto&& location = instance->m_feature.get();

        auto zone = WorldToZonePos(instance->m_pos);
        if (location.m_iconAlways
            || (location.m_iconPlaced && m_generatedZones.contains(zone)))
        {
            result.push_back(*instance.get());
        }
    }

    return result;
}

// private
void IZoneManager::GetTerrainDelta(VUtils::Random::State& state, Vector3f center, float radius, float& delta, Vector3f& slopeDirection) {
    float num2 = std::numeric_limits<float>::min();
    float num3 = std::numeric_limits<float>::max();
    Vector3f b = center;
    Vector3f a = center;
    for (int i = 0; i < 10; i++) {
        Vector2f vector = state.InsideUnitCircle() * radius;
        Vector3f vector2 = center + Vector3f(vector.x, 0.f, vector.y);
        float groundHeight = GetGroundHeight(vector2);
        if (groundHeight < num3) {
            num3 = groundHeight;
            a = vector2;
        }
        if (groundHeight > num2) {
            num2 = groundHeight;
            b = vector2;
        }
    }
    delta = num2 - num3;
    slopeDirection = (a - b).Normal();
}

// used importantly for snapping and location/vegetation generation
// public
float IZoneManager::GetGroundHeight(Vector3f p) {
    return GeoManager()->GetHeight(p.x, p.z);
}

// public
// if terrain is just heightmap,
// could easily create a wrapper and poll points where needed
Heightmap& IZoneManager::GetGroundData(Vector3f& p, Vector3f& normal, Biome& biome, BiomeArea& biomeArea) {
    auto &&heightmap = HeightmapManager()->GetHeightmap(WorldToZonePos(p));

    heightmap.GetWorldHeight(p, p.y);

    biome = heightmap.GetBiome(p);
    biomeArea = heightmap.GetBiomeArea();

    heightmap.GetWorldNormal(p, normal);

    return heightmap;
}

// public
IZoneManager::Feature::Instance* IZoneManager::GetNearestFeature(std::string_view name, Vector3f point) {
    float closestDist = std::numeric_limits<float>::max();

    IZoneManager::Feature::Instance* closest = nullptr;

    for (auto&& pair : m_generatedFeatures) {
        auto&& instance = pair.second;
        auto&& location = instance->m_feature.get();

        float dist = instance->m_pos.SqDistance(point);
        if (location.m_name == name && dist < closestDist) {
            closestDist = dist;
            closest = instance.get();
        }
    }

    return closest;
}

#else

// public
bool IZoneManager::GetNearestFeature(std::string_view name, Vector3f in, Vector3f& out) {
    float sqMin = std::numeric_limits<float>::max();
        
    for (auto&& pair : m_generatedFeatures) {
        if (m_features[pair.second.first] == name) {
            float sq = in.SqDistance(pair.second.second);
            if (sq < sqMin) {
                out = pair.second.second;
                sqMin = sq;
            }
        }
    }

    return sqMin < std::numeric_limits<float>::max();
}
#endif

// public
// this is world position to zone position
// formerly GetZone
ZoneID IZoneManager::WorldToZonePos(Vector3f point) {
    auto x = floor((point.x + (float)ZONE_SIZE / 2.f) / (float)ZONE_SIZE);
    auto y = floor((point.z + (float)ZONE_SIZE / 2.f) / (float)ZONE_SIZE);
    return ZoneID(x, y);
}

// public
// zone position to ~world position
// GetZonePos
Vector3f IZoneManager::ZoneToWorldPos(ZoneID id) {
    return Vector3f(id.x * ZONE_SIZE, 0, id.y * ZONE_SIZE);
}

#if VH_IS_ON(VH_ZONE_GENERATION)
// private
bool IZoneManager::IsZoneGenerated(ZoneID zoneID) {
    return m_generatedZones.contains(zoneID);
}
#endif