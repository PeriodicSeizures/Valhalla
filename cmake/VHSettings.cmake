#option (VH_USE_MODS "Enable Lua modding subsystem" OFF)
#option (VH_USE_PREFABS "Enable Prefab support" ON)
#option (VH_ZONE_GENERATION "Generate zones/features/vegetation" ON) # requires VH_USE_PREFABS default or ON
#option (VH_DISCORD_INTEGRATION "Enable Discord command bot" ON)
option (VH_USE_ZLIB "Whether to enable zlib library support" OFF)

# Download it here https://partner.steamgames.com/downloads/steamworks_sdk.zip
set	(STEAMWORKS_SDK_LOCATION "C:/Users/Rico/Documents/Visual Studio 2022/Libraries/steam-sdk")