#pragma once

#include "HashUtils.h"
#include "VUtils.h"
#include "HeightMap.h"
#include "TerrainModifier.h"
#include "Biome.h"



class IHeightmapManager {
	//UNORDERED_SET_t<ZoneID> m_population;
	UNORDERED_MAP_t<ZoneID, std::unique_ptr<Heightmap>> m_heightmaps;

public:
	//void ForceGenerateAll();
	void ForceQueuedRegeneration();

	float GetOceanDepthAll(Vector3f worldPos);

	bool AtMaxLevelDepth(Vector3f worldPos);

	//Vector3f GetNormal(const Vector3f& pos);

	// Get the heightmap height at position
	//	Will only succeed given the heightmap exists
	// TODO make this return the height, or throw if out of bounds of entire square world
	//bool GetHeight(const Vector3f& worldPos, float& height);
	//bool GetAverageHeight(const Vector3f& worldPos, float radius, float &height);

	

	// Get the heightmap height at position,
	//	The heightmap will be created if it does not exist
	//float GetHeight(const Vector3f& worldPos);

	//static std::vector<Heightmap> GetAllHeightmaps();
	UNORDERED_MAP_t<ZoneID, std::unique_ptr<Heightmap>>& GetAllHeightmaps();

	//Heightmap* GetOrCreateHeightmap(const Vector2i& zoneID);

	Heightmap* PollHeightmap(ZoneID zone);

	Heightmap &GetHeightmap(Vector3f point);
	Heightmap &GetHeightmap(ZoneID zone);
	std::vector<Heightmap*> GetHeightmaps(Vector3f point, float radius);
	//Biome FindBiome(const Vector3f& point);

	bool IsRegenerateQueued(Vector3f point, float radius);

	//Heightmap* CreateHeightmap(const Vector2i& zone);
};

// Manager class for everything related to finely partitioned world heights and biomes during generation
IHeightmapManager* HeightmapManager();
