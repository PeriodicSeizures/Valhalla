#pragma once

#include <ranges>

#include "Peer.h"
#include "NetAcceptor.h"
#include "Vector.h"
#include "WorldManager.h"
#include "HashUtils.h"

class INetManager {
    friend class IModManager;

private:
    std::unique_ptr<IAcceptor> m_acceptor;

    //std::list<std::unique_ptr<Peer>> m_rpcs; // used to temporarily connecting peers (until PeerInfo)
    //std::list<std::unique_ptr<Peer>> m_onlinePeers;

    std::vector<std::unique_ptr<Peer>> m_connectedPeers;
    std::vector<Peer*> m_onlinePeers;

    std::list<std::pair<std::string, std::pair<nanoseconds, nanoseconds>>> m_sortedSessions;
    UNORDERED_MAP_t<std::string, int32_t> m_sessionIndexes;

public:
    std::string m_password;
    std::string m_salt;

private:
    // Kick a player by name
    bool Kick(std::string user);

    // Ban a player by name
    bool Ban(std::string user);

    // Unban a player by name
    bool Unban(const std::string& user);

    void SendDisconnect();
    void SendPlayerList();
    void SendNetTime();
    void SendPeerInfo(Peer &peer);

    void OnPeerQuit(Peer& peer);
    void OnPeerDisconnect(Peer& peer);

    //void RPC_PeerInfo(NetRpc* rpc, BYTES_t bytes);

public:
    void PostInit();
    void Update();
    void Uninit();
    
    void OnConfigLoad(bool reloading);

    Peer* GetPeerByName(const std::string& name);
    Peer* GetPeer(OWNER_t uuid);
    Peer* GetPeerByHost(const std::string& host);

    std::vector<Peer*> GetPeers(const std::string &addr);

    //const robin_hood::unordered_map<OWNER_t, std::unique_ptr<Peer>>& GetPeers();

    //void OnPeerConnect(std::unique_ptr<Peer> peer);

    //void OnNewClient(ISocket::Ptr socket, OWNER_t uuid, const std::string& name, const Vector3f &pos);
    void OnPeerConnect(Peer& peer);

    //void OnNewClient(NetRpc* rpc, OWNER_t uuid, const std::string& name, const Vector3f& pos);

    const auto& GetPeers() {
        return m_onlinePeers;
    }
};

// Manager class for everything related to networking at a mildly abstracted level
INetManager* NetManager();
