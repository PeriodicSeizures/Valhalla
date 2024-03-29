#pragma once

#include <future>
#include <thread>

#include "VUtils.h"
#include "DataReader.h"
#include "DataWriter.h"

class IWorldManager;

class World {
    friend class IWorldManager;

public:
    std::string m_name;
    std::string m_seedName;
    HASH_t m_seed;
    OWNER_t m_uid;
    int32_t m_worldGenVersion;

public:
    World(std::string name, std::string seedName);
    World(DataReader reader);

public:
    BYTES_t SaveMeta();
    //BYTES_t SaveDB();

    void WriteFileMeta(const fs::path& root);
    void WriteFileDB(const fs::path& root);
    void LoadFileDB(const fs::path& root);
    void CopyCompressDB(const fs::path& root);
    void WriteFiles(const fs::path& root);

    void WriteFileMeta();
    void WriteFileDB();
    void LoadFileDB();
    void WriteFiles();
};

class IWorldManager {
private:
    std::unique_ptr<World> m_world;
    //std::jthread m_saveThread;

public:
    World* GetWorld();

    // Get root path of worlds
    //  threadsafe
    fs::path GetWorldsPath() const;
    // Get meta path of world
    //  threadsafe
    //fs::path GetWorldMetaPath(const std::string& name) const;
    // Get db path of world
    //  threadsafe
    //fs::path GetWorldDBPath(const std::string& name) const;

    bool LoadWorldMeta(const fs::path &root);

    std::unique_ptr<World> RetrieveWorld(std::string_view name, std::string_view fallbackSeedName) const;

    BYTES_t SaveWorldDB() const;
    //void LoadFileWorldDB(const fs::path& path) const;

    // Create a copy of a world by name
    //  threadsafe
    //void BackupFileWorldDB() const;

    // Write the db of the current world to disk
    //  The world is only saved
    //  Threadsafe
    //void WriteFileWorldDB(const fs::path& path, bool sync);

    // Write the db of the current world to disk
    //  This is the go-to method to save the world to disk
    //  The world is backed up then saved
    //  Threadsafe
    //void WriteFileWorldDB(bool sync);

    void PostZoneInit();

    void PostInit();
};

// Manager class for everything related to world file loading and file saving
IWorldManager* WorldManager();
