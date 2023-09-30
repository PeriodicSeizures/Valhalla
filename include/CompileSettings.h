#pragma once

//#include <chrono>

//using namespace std::chrono_literals;

#define RUN_TESTS

#define VH_VERSION "v1.0.5"

// ELPP log file name
#define VH_LOGFILE_PATH "logs/log.txt"

#define VH_LUA_PATH "lua"
#define VH_LUA_CPATH "bin"
#define VH_MOD_PATH "mods"

#define VH_RAW_IS_ON(OP_SYMBOL) ((3 OP_SYMBOL 3) != 0)
#define VH_RAW_IS_OFF(OP_SYMBOL) ((3 OP_SYMBOL 3) == 0)
#define VH_RAW_IS_DEFAULT_ON(OP_SYMBOL) ((3 OP_SYMBOL 3) > 3)
#define VH_RAW_IS_DEFAULT_OFF(OP_SYMBOL) ((3 OP_SYMBOL 3 OP_SYMBOL 3) < 0)

#define VH_IS_ON(OP_SYMBOL) VH_RAW_IS_ON(OP_SYMBOL ## _I_)
#define VH_IS_OFF(OP_SYMBOL) VH_RAW_IS_OFF(OP_SYMBOL ## _I_)
#define VH_IS_DEFAULT_ON(OP_SYMBOL) VH_RAW_IS_DEFAULT_ON(OP_SYMBOL ## _I_)
#define VH_IS_DEFAULT_OFF(OP_SYMBOL) VH_RAW_IS_DEFAULT_OFF(OP_SYMBOL ## _I_)

#define VH_ON          |
#define VH_OFF         ^
#define VH_DEFAULT_ON  +
#define VH_DEFAULT_OFF -

#if SIZE_MAX <= 0xFFFFULL
	#define VH_PLATFORM_16BIT_I_ VH_ON
	#define VH_PLATFORM_32BIT_I_ VH_OFF
	#define VH_PLATFORM_64BIT_I_ VH_OFF
#elif SIZE_MAX <= 0xFFFFFFFFULL
    #define VH_PLATFORM_16BIT_I_ VH_OFF
    #define VH_PLATFORM_32BIT_I_ VH_ON
    #define VH_PLATFORM_64BIT_I_ VH_OFF
#else
    #define VH_PLATFORM_16BIT_I_ VH_OFF
    #define VH_PLATFORM_32BIT_I_ VH_OFF
    #define VH_PLATFORM_64BIT_I_ VH_ON
#endif

#if defined(VH_PLATFORM_WINDOWS)
    #if VH_PLATFORM_WINDOWS != 0
        #define VH_PLATFORM_WINDOWS_I_ VH_ON
    #else
        #define VH_PLATFORM_WINDOWS_I_ VH_OFF
    #endif
#elif defined(_WIN32)
    #define VH_PLATFORM_WINDOWS_I_ VH_DEFAULT_ON
#else
    #define VH_PLATFORM_WINDOWS_I_ VH_DEFAULT_OFF
#endif

#if defined(VH_PLATFORM_ESP32)
    #if VH_PLATFORM_ESP32 != 0
        #define VH_PLATFORM_ESP32_I_ VH_ON
    #else
        #define VH_PLATFORM_ESP32_I_ VH_OFF
    #endif
#elif defined(ESP_PLATFORM)
    #define VH_PLATFORM_ESP32_I_ VH_DEFAULT_ON
#else
    #define VH_PLATFORM_ESP32_I_ VH_DEFAULT_OFF
#endif

#if VH_IS_ON(VH_PLATFORM_WINDOWS) && VH_IS_ON(VH_PLATFORM_ESP32)
    #error "Platform must be exclusively Windows or ESP32, not both"
#endif

// Whether to use 4 byte zdoid or 2 byte zdoid
//  4 byte: capacity for a recommended 64 owners 
//  2 byte: capacity for a recommended 4 owners
#if defined(VH_USE_SMALL_ZDOID)
    #if VH_USE_SMALL_ZDOID != 0
        #define VH_USE_SMALL_ZDOID_I_ VH_ON
    #else
        #define VH_USE_SMALL_ZDOID_I_ VH_OFF
    #endif
#else
    #if VH_IS_ON(VH_PLATFORM_ESP32)
        #define VH_SMALL_ZDOID_I_ VH_DEFAULT_ON
    #else
        #define VH_SMALL_ZDOID_I_ VH_DEFAULT_OFF
    #endif
#endif

// Ensure that the bits
#if defined(VH_USER_BITS)
    // 8 is the current max (might change stuff to be more adaptable and flexible with compile settings)
    
    // ensure esp32 is always 32 bit
    // 
    #if VH_USER_BITS >= 2 && VH_USER_BITS <= 7
        #define VH_USER_BITS_I_ VH_USER_BITS
    #else
        #error "User bits must be between 2 and 7 (inclusive)"
    #endif
#else
    #if VH_IS_ON(VH_PLATFORM_ESP32)
        #define VH_USER_BITS_I_ 2
    #else
        #define VH_USER_BITS_I_ 6
    #endif
#endif

/*
// The default are the remaining bits from VH_USER_BITS
#if defined(VH_ID_BITS)
    #error "Setting custom id bits not yet supported"

    // 8 is the current max (might change stuff to be more adaptable and flexible with compile settings)
    #if VH_ID_BITS > 1 && VH_ID_BITS < 8
        #define VH_ID_BITS_I_ VH_ID_BITS
    #else
        #error "ID bits must be between 1 and 8 (inclusive)"
    #endif
#else
    //#if defined(VH_USER_BITS)
    //    #if VH_IS_ON(VH_PLATFORM_ESP32)
    //        #define VH_ID_BITS_I_ 1
    //    #else
    //        #define VH_ID_BITS_I_ 
    //    #endif
    //#else

        #define VH_ID_BITS_I_ (32 - VH_USER_BITS_I_)
    //#endif
#endif
*/

/*
// Whether to include only core Valheim functionality
// Included:
//  - World loading/saving
//  - Zone generation
//  - Dungeon generation
//  - Random events
//  - Core features
// Excluded:
//  - Mods
//  - Extra features
//  - Dungeon regeneration
//  - Discord integration
//  - Player packet capture
#if defined(VH_TOTALLY_VANILLA)
    #if VH_TOTALLY_VANILLA != 0
        #define VH_TOTALLY_VANILLA_I_ VH_ON
    #else
        #define VH_TOTALLY_VANILLA_I_ VH_OFF
    #endif
#else
    #define VH_TOTALLY_VANILLA_I_ VH_DEFAULT_OFF
#endif*/


// Whether to disallow potentially malicious modded players
//  'Malicious' is defined as non-conforming or non-standard 
//  behaviour which can and will break server functionality,
//  such as errors, undefined behaviour, and general
//  strict assumptions about the game that require these standards
//  to be met
// TODO rename UNEXPECTED_PACKET_METADATA...
#if defined(VH_DISALLOW_MALICIOUS_PLAYERS)
    #if VH_DISALLOW_MALICIOUS_PLAYERS != 0
        #define VH_DISALLOW_MALICIOUS_PLAYERS_I_ VH_ON
    #else
        #define VH_DISALLOW_MALICIOUS_PLAYERS_I_ VH_OFF
    #endif
#else
    #define VH_DISALLOW_MALICIOUS_PLAYERS_I_ VH_DEFAULT_ON
#endif

// whether to allow non-standard behaviour that does not cause
//  conflict with the server or other players
//  might be an extended name 
#if defined(VH_DISALLOW_NON_CONFORMING_PLAYERS)
    #if VH_DISALLOW_NON_CONFORMING_PLAYERS != 0
        #define VH_DISALLOW_NON_CONFORMING_PLAYERS_I_ VH_ON
    #else
        #define VH_DISALLOW_NON_CONFORMING_PLAYERS_I_ VH_OFF
    #endif
#else
    #define VH_DISALLOW_NON_CONFORMING_PLAYERS_I_ VH_DEFAULT_OFF
#endif

/*
// Controls whether to scan player sent data for
//  obscure patterns of data and/or impossibilities
//  that would normally affect gameplay of server performance
// Minor overhead but also experimental if enabled
#if defined(VH_SANITIZE_EXTERNAL_DATA)

#else

#endif

// Same as VH_SANITIZE_EXTERNAL_DATA but for methods that are 
//  inherently internal and not likely to be used by an external
#if defined(VH_SANITIZE_INTERNAL_DATA)

#else

#endif
*/

#if defined(VH_USE_MODS)
    #if VH_USE_MODS != 0
        #define VH_USE_MODS_I_ VH_ON
    #else
        #define VH_USE_MODS_I_ VH_OFF
    #endif
#else
    #define VH_USE_MODS_I_ VH_DEFAULT_ON
#endif

// Whether events/rpcs/... called by Lua will self trigger other callbacks
//  Disabled by default
//  Enabling will lower performance slightly depending on mods
//  This name isn't the best
#if defined(VH_REFLECTIVE_MOD_EVENTS)
    #if VH_REFLECTIVE_MOD_EVENTS != 0
        #if VH_IS_OFF(VH_USE_MODS)
            #error "Mod visible events must have mods enabled"
        #else
            #define VH_REFLECTIVE_MOD_EVENTS_I_ VH_ON
        #endif
    #else
        #define VH_REFLECTIVE_MOD_EVENTS_I_ VH_OFF
    #endif
#else
    #define VH_REFLECTIVE_MOD_EVENTS_I_ VH_DEFAULT_OFF
#endif

//#define VH_STANDARD_PREFABS 0

// Whether to use the smallest form prefabs possible
//  Each prefab generally takes up min 72 bytes
//  Each prefab minimally needs only flags to work well (not extra redundant hashes)
#if defined(VH_STANDARD_PREFABS)
    #if VH_STANDARD_PREFABS != 0
        #define VH_STANDARD_PREFABS_I_ VH_ON
    #else
        #define VH_STANDARD_PREFABS_I_ VH_OFF
    #endif
#else
    #define VH_STANDARD_PREFABS_I_ VH_DEFAULT_ON
#endif

// Whether unknown prefabs are allowed
//  useful with mods
#if defined(VH_REQUIRE_RECOGNIZED_PREFABS)
    #if VH_REQUIRE_RECOGNIZED_PREFABS != 0
        #define VH_REQUIRE_RECOGNIZED_PREFABS_I_ VH_ON
    #else
        #define VH_REQUIRE_RECOGNIZED_PREFABS_I_ VH_OFF
    #endif
#else
    #define VH_REQUIRE_RECOGNIZED_PREFABS_I_ VH_DEFAULT_OFF
#endif

// Whether prefabs should be utilized for world operations
//  or use from-scratch operations
//  (using prefab members VS checking zdo prefab hashes)
#if defined(VH_WORLD_UTILIZE_PREFABS)
    #if VH_WORLD_UTILIZE_PREFABS != 0
        #define VH_WORLD_UTILIZE_PREFABS_I_ VH_ON
    #else
        #define VH_WORLD_UTILIZE_PREFABS_I_ VH_OFF
    #endif
#else
    #define VH_WORLD_UTILIZE_PREFABS_I_ VH_DEFAULT_OFF
#endif

//#define VH_ZONE_GENERATION 0

#if defined(VH_ZONE_GENERATION)
    #if VH_ZONE_GENERATION != 0
        #if VH_IS_OFF(VH_STANDARD_PREFABS)
            #error "Zone generation VH_ZONE_GENERATION requires VH_STANDARD_PREFABS to be on"
        #endif

        #define VH_ZONE_GENERATION_I_ VH_ON
    #else
        #define VH_ZONE_GENERATION_I_ VH_OFF
    #endif
#else
    #define VH_ZONE_GENERATION_I_ VH_STANDARD_PREFABS_I_
#endif

#if defined(VH_RANDOM_EVENTS)
    #if VH_RANDOM_EVENTS != 0
        #if VH_IS_OFF(VH_STANDARD_PREFABS)
            #error "VH_RANDOM_EVENTS requires VH_STANDARD_PREFABS"
        #endif

        #define VH_RANDOM_EVENTS_I_ VH_ON
    #else
        #define VH_RANDOM_EVENTS_I_ VH_OFF
    #endif
#else
    #define VH_RANDOM_EVENTS_I_ VH_STANDARD_PREFABS_I_
#endif

#if defined(VH_DUNGEON_GENERATION)
    #if VH_DUNGEON_GENERATION != 0
        #if VH_IS_OFF(VH_STANDARD_PREFABS)
            #error("VH_DUNGEON_GENERATION requires VH_STANDARD_PREFABS")
        #endif

        #define VH_DUNGEON_GENERATION_I_ VH_ON
    #else
        #define VH_DUNGEON_GENERATION_I_ VH_OFF
    #endif
#else
    #define VH_DUNGEON_GENERATION_I_ VH_STANDARD_PREFABS_I_
#endif

#if defined(VH_DISCORD_INTEGRATION)
    #if VH_DISCORD_INTEGRATION != 0
        #define VH_DISCORD_INTEGRATION_I_ VH_ON
    #else
        #define VH_DISCORD_INTEGRATION_I_ VH_OFF
    #endif
#else
    #define VH_DISCORD_INTEGRATION_I_ VH_DEFAULT_ON
#endif

#if defined(VH_PLAYER_CAPTURE)
    #if VH_DISCORD_INTEGRATION != 0
        #define VH_PLAYER_CAPTURE_I_ VH_ON
    #else
        #define VH_PLAYER_CAPTURE_I_ VH_OFF
    #endif
#else
    #define VH_PLAYER_CAPTURE_I_ VH_DEFAULT_OFF
#endif

#define VH_CORE_FEATURES 1

// Whether to enable:
//  - portal linking
//  - sleeping/time skip
// Disabled by default if mods are enabled
#if defined(VH_CORE_FEATURES)
    #if VH_CORE_FEATURES != 0
        #define VH_CORE_FEATURES_I_ VH_ON
    #else
        #define VH_CORE_FEATURES_I_ VH_OFF
    #endif
#else
    #if VH_IS_ON(VH_USE_MODS)
        #define VH_CORE_FEATURES_I_ VH_DEFAULT_OFF
    #else
        #define VH_CORE_FEATURES_I_ VH_DEFAULT_ON
    #endif
#endif

#if defined(VH_PORTAL_LINKING)
    #if VH_PORTAL_LINKING != 0
        #define VH_PORTAL_LINKING_I_ VH_ON
    #else
        #define VH_PORTAL_LINKING_I_ VH_OFF
    #endif
#else
    #if VH_IS_ON(VH_CORE_FEATURES)
        #define VH_PORTAL_LINKING_I_ VH_DEFAULT_ON
    #else
        #define VH_PORTAL_LINKING_I_ VH_DEFAULT_OFF
    #endif
#endif

#if defined(VH_PLAYER_SLEEP)
    #if VH_PLAYER_SLEEP != 0
        #define VH_PLAYER_SLEEP_I_ VH_ON
    #else
        #define VH_PLAYER_SLEEP_I_ VH_OFF
    #endif
#else
    #if VH_IS_ON(VH_CORE_FEATURES)
        #define VH_PLAYER_SLEEP_I_ VH_DEFAULT_ON
    #else
        #define VH_PLAYER_SLEEP_I_ VH_DEFAULT_OFF
    #endif
#endif

// Whether to include extra useful builtins
//  - Built-in BetterNetworking mod (in C++)
//  - Dungeon regeneration
//  - experimental ZDO assignment algorithm
//  - 
#if defined(VH_EXTRA_FEATURES)
    #if VH_EXTRA_FEATURES != 0
        #define VH_EXTRA_FEATURES_I_ VH_ON
    #else
        #define VH_EXTRA_FEATURES_I_ VH_OFF
    #endif
#else
    #define VH_EXTRA_FEATURES_I_ VH_DEFAULT_OFF
#endif

// https://github.com/T3kla/ValMods/blob/master/~DungeonReset/Scripts/Extensions.cs
#if defined(VH_DUNGEON_REGENERATION)
    #if VH_DUNGEON_REGENERATION != 0
        #if VH_IS_OFF(VH_DUNGEON_GENERATION)
            #error "VH_DUNGEON_REGENERATION requires VH_DUNGEON_GENERATION"
        #endif

        #define VH_DUNGEON_REGENERATION_I_ VH_ON
    #else
        #define VH_DUNGEON_REGENERATION_I_ VH_OFF
    #endif
#else
    #if VH_IS_ON(VH_EXTRA_FEATURES)
        #define VH_DUNGEON_REGENERATION_I_ VH_DUNGEON_GENERATION_I_
    #else
        #define VH_DUNGEON_REGENERATION_I_ VH_DEFAULT_OFF
    #endif
#endif

// Whether to support loading worlds older than the latest version
//  Can be disabled to very slightly reduce executable size
#if defined(VH_LEGACY_WORLD_COMPATABILITY)
    #if VH_LEGACY_WORLD_COMPATABILITY != 0
        #define VH_LEGACY_WORLD_COMPATABILITY_I_ VH_ON
    #else
        #define VH_LEGACY_WORLD_COMPATABILITY_I_ VH_OFF
    #endif
#else
    #define VH_LEGACY_WORLD_COMPATABILITY_I_ VH_DEFAULT_ON
#endif

// Packet capture path
#define VH_CAPTURE_PATH "captures"

// Valheim latest versionings
//    Includes game, worldgen, zdo, zonelocation, ...
namespace VConstants {
    // Valheim Steam app id
    static constexpr int32_t APP_ID = 892970;

    // Valheim game version
    //  Located in Version.cs
    static const char* GAME = "0.217.20";

    static constexpr uint32_t NETWORK = 15;

    // Used while loading world from file (ZNet/ZoneSystem/ZDOMan/RandEventSystem)
    // 32: Hildir beta
    static constexpr int32_t WORLD = 32;

    // Used in WorldGenerator terrain
    static constexpr int32_t WORLDGEN = 2;

    // Used in ZoneSystem Feature-Prefabs
    static constexpr int32_t LOCATION = 26;
}