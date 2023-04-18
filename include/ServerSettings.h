#pragma once

#include "VUtils.h"

enum class AssignAlgorithm {
    NONE,
    DYNAMIC_RADIUS
};

enum class WorldMode : int32_t {
    NORMAL,
    CAPTURE,
    PLAYBACK,
};

struct ServerSettings {
    std::string     serverName;
    uint16_t        serverPort;
    std::string     serverPassword;
    bool            serverPublic;
    bool            serverDedicated;

    std::string     worldName;
    std::string     worldSeed;
    bool            worldPregenerate;
    seconds         worldSaveInterval;  // set to 0 to disable
    bool            worldModern;        // whether to purge old objects on load
    WorldMode       worldCaptureMode;
    size_t          worldCaptureSize;
    int             worldCaptureSession;
    int             worldPlaybackSession;
    
    //bool            playerAutoPassword;
    bool            playerWhitelist;
    unsigned int    playerMax;
    bool            playerAuth;
    seconds         playerTimeout;
    //bool            playerList;
    //bool            playerArrivePing;
    //bool            playerForceVisible;
    //bool            playerSleep;        // enable time skip when all players sleeping
    //bool            playerSleepSolo;    // whether only 1 player needs to sleep to enable time skip
    
    //milliseconds    socketTimeout;          // ms

    unsigned int    zdoMaxCongestion;    // congestion rate
    unsigned int    zdoMinCongestion;    // congestion rate
    milliseconds    zdoSendInterval;
    seconds         zdoAssignInterval;
    //bool            zdoSmartAssign;     // experimental feature that attempts to reduce lagg
    AssignAlgorithm zdoAssignAlgorithm;

    bool            spawningCreatures;
    bool            spawningLocations;
    bool            spawningVegetation;
    bool            spawningDungeons;

    bool            dungeonEndCaps;
    bool            dungeonDoors;
    bool            dungeonFlipRooms;
    bool            dungeonZoneLimit;
    bool            dungeonRoomShrink;
    bool            dungeonReset;
    seconds         dungeonResetTime;
    //milliseconds    dungeonIncrementalResetTime;
    int             dungeonIncrementalResetCount;
    bool            dungeonSeededRandom;

    bool            eventsEnabled;
    float           eventsChance;
    seconds         eventsInterval;
    float           eventsRange;
    //seconds         eventsSendInterval;

};
