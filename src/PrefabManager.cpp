#include "PrefabManager.h"

#include "ZDOManager.h"
#include "VUtilsResource.h"

auto PREFAB_MANAGER(std::make_unique<IPrefabManager>());
IPrefabManager* PrefabManager() {
    return PREFAB_MANAGER.get();
}



void IPrefabManager::Init() {
    LOG_INFO(LOGGER, "Initializing PrefabManager");

    auto opt = VUtils::Resource::ReadFile<BYTES_t>("prefabs.pkg");

    if (!opt) {
        throw std::runtime_error("prefabs.pkg missing");
    }

    DataReader pkg(opt.value());

    pkg.Read<std::string_view>(); // comment
    auto ver = pkg.Read<std::string_view>();
    if (ver != VConstants::GAME)
        LOG_WARNING(LOGGER, "prefabs.pkg uses different game version than server ({})", ver);

    auto count = pkg.Read<int32_t>();

    for (int i=0; i < count; i++) {
        Register(pkg);
    }

    LOG_INFO(LOGGER, "Loaded {} prefabs", count);
}