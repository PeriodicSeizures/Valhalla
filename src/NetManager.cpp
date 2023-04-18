#include <optick.h>
#include <openssl/md5.h>
#include <openssl/rand.h>
#include <isteamgameserver.h>

#include "NetManager.h"
#include "ValhallaServer.h"
#include "WorldManager.h"
#include "VUtilsRandom.h"
#include "Hashes.h"
#include "ZDOManager.h"
#include "RouteManager.h"
#include "ZoneManager.h"
#include "VUtilsResource.h"

using namespace std::chrono;

// TODO use netmanager instance instead

auto NET_MANAGER(std::make_unique<INetManager>());
INetManager* NetManager() {
    return NET_MANAGER.get();
}



bool INetManager::Kick(std::string user) {
    auto&& peer = GetPeerByName(user);
    try {
        if (!peer) peer = GetPeer(std::stoll(user));
    }
    catch (std::exception&) {}

    if (peer) {
        user = peer->m_socket->GetHostName();
        peer->Kick();
        return true;
    }
    else {
        auto peers = GetPeers(user);

        for (auto&& peer : peers) {
            user = peer->m_socket->GetHostName();
            peer->Kick();
        }

        if (!peers.empty())
            return true;
    }

    return false;
}

bool INetManager::Ban(std::string user) {
    {
        auto&& peer = GetPeerByName(user);
        try {
            if (!peer) peer = GetPeer(std::stoll(user));
        }
        catch (const std::exception&) {}

        if (peer) {
            user = peer->m_socket->GetHostName();
            peer->Kick();
            Valhalla()->m_blacklist.insert(user);
            return true;
        }
    }

    auto peers = GetPeers(user);

    for (auto&& peer : peers) {
        user = peer->m_socket->GetHostName();
        peer->Kick();
    }

    if (!peers.empty())
        return true;
    
    Valhalla()->m_blacklist.insert(user);
    return false;
}

bool INetManager::Unban(const std::string& user) {
    return Valhalla()->m_blacklist.erase(user);
}



void INetManager::SendDisconnect() {
    LOG(INFO) << "Sending disconnect msg";

    for (auto&& peer : m_connectedPeers) {
        peer->SendDisconnect();
    }
}



void INetManager::SendPlayerList() {
    if (!m_onlinePeers.empty()) {
        if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::PlayerList))
            return;

        static BYTES_t bytes; bytes.clear();
        DataWriter writer(bytes);
        writer.Write((int)m_onlinePeers.size());

        for (auto&& peer : m_onlinePeers) {
            writer.Write(std::string_view(peer->m_name));
            writer.Write(std::string_view(peer->m_socket->GetHostName()));
            writer.Write(peer->m_characterID);
            //writer.Write(peer->m_visibleOnMap || VH_SETTINGS.playerForceVisible);
            //if (peer->m_visibleOnMap || VH_SETTINGS.playerForceVisible) {
            //    writer.Write(peer->m_pos);
            //}
            writer.Write(peer->m_visibleOnMap);
            if (peer->m_visibleOnMap)
                writer.Write(peer->m_pos);
        }

        for (auto&& peer : m_onlinePeers) {
            // this is the problem
            peer->Invoke(Hashes::Rpc::S2C_UpdatePlayerList, bytes);
        }
    }
}

void INetManager::SendNetTime() {
    for (auto&& peer : m_onlinePeers) {
        peer->Invoke(Hashes::Rpc::S2C_UpdateTime, Valhalla()->GetWorldTime());
    }
}



void INetManager::SendPeerInfo(Peer& peer) {
    BYTES_t bytes;
    DataWriter writer(bytes);

    writer.Write(Valhalla()->ID());
    writer.Write(VConstants::GAME);
    writer.Write(VConstants::NETWORK);
    writer.Write(Vector3f::Zero()); // dummy
    writer.Write(""); // dummy

    auto world = WorldManager()->GetWorld();

    writer.Write(std::string_view(world->m_name));
    writer.Write(world->m_seed);
    writer.Write(std::string_view(world->m_seedName)); // Peer does not seem to use
    writer.Write(world->m_uid);
    writer.Write(world->m_worldGenVersion);
    writer.Write(Valhalla()->GetWorldTime());

    peer.Invoke(Hashes::Rpc::PeerInfo, bytes);
}



//void INetManager::OnNewClient(ISocket::Ptr socket, OWNER_t uuid, const std::string &name, const Vector3f &pos) {
void INetManager::OnPeerConnect(Peer& peer) {
    peer.m_admin = Valhalla()->m_admin.contains(peer.m_socket->GetHostName());

    if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::Join, peer)) {
        return peer.Disconnect();
    }

    // Important
    peer.Register(Hashes::Rpc::C2S_UpdatePos, [this](Peer* peer, Vector3f pos, bool publicRefPos) {
        peer->m_pos = pos;
        peer->m_visibleOnMap = publicRefPos; // stupid name
        });

    // Important
    peer.Register(Hashes::Rpc::C2S_UpdateID, [this](Peer* peer, ZDOID characterID) {
        peer->m_characterID = characterID;
        // Peer does send 0,0 on death
        //if (!characterID)
            //throw std::runtime_error("charac")

        LOG(INFO) << "Got CharacterID from " << peer->m_name << " ( " << characterID.m_uuid << ":" << characterID.m_id << ")";
        });

    peer.Register(Hashes::Rpc::C2S_RequestKick, [this](Peer* peer, std::string user) {
        // TODO maybe permissions tree in future?
        //  lua? ...
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        auto split = VUtils::String::Split(user, " ");

        if (Kick(std::string(split[0]))) {
            peer->ConsoleMessage("Kicked '" + user + "'");
        }
        else {
            peer->ConsoleMessage("Player not found");
        }
        });

    peer.Register(Hashes::Rpc::C2S_RequestBan, [this](Peer* peer, std::string user) {
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        auto split = VUtils::String::Split(user, " ");

        if (Ban(std::string(split[0]))) {
            peer->ConsoleMessage("Banned '" + user + "'");
        }
        else {
            peer->ConsoleMessage("Player not found");
        }
        });

    peer.Register(Hashes::Rpc::C2S_RequestUnban, [this](Peer* peer, std::string user) {
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        // devcommands requires an exact format...
        Unban(user);
        peer->ConsoleMessage("Unbanning user " + user);

        //if (Unban(user)) {
        //    peer->ConsoleMessage("Unbanning user " + user);
        //}
        //else {
        //    peer->ConsoleMessage("Player is not banned");
        //}
        });

    peer.Register(Hashes::Rpc::C2S_RequestSave, [](Peer* peer) {
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        //WorldManager()->WriteFileWorldDB(true);

        WorldManager()->GetWorld()->WriteFiles();

        peer->ConsoleMessage("Saved the world");
        });

    peer.Register(Hashes::Rpc::C2S_RequestBanList, [this](Peer* peer) {
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        if (Valhalla()->m_blacklist.empty())
            peer->ConsoleMessage("Banned users: (none)");
        else {
            peer->ConsoleMessage("Banned users:");
            for (auto&& banned : Valhalla()->m_blacklist) {
                peer->ConsoleMessage(banned);
            }
        }

        if (!VH_SETTINGS.playerWhitelist)
            peer->ConsoleMessage("Whitelist is disabled");
        else {
            if (Valhalla()->m_whitelist.empty())
                peer->ConsoleMessage("Whitelisted users: (none)");
            else {
                peer->ConsoleMessage("Whitelisted users:");
                for (auto&& banned : Valhalla()->m_whitelist) {
                    peer->ConsoleMessage(banned);
                }
            }
        }
        });

    SendPeerInfo(peer);

    ZDOManager()->OnNewPeer(peer);
    RouteManager()->OnNewPeer(peer);
    ZoneManager()->OnNewPeer(peer);

    m_onlinePeers.push_back(&peer);
}


// Return the peer or nullptr
Peer* INetManager::GetPeerByName(const std::string& name) {
    for (auto&& peer : m_onlinePeers) {
        if (peer->m_name == name)
            return peer;
    }
    return nullptr;
}

// Return the peer or nullptr
Peer* INetManager::GetPeer(OWNER_t uuid) {
    for (auto&& peer : m_onlinePeers) {
        if (peer->m_uuid == uuid)
            return peer;
    }
    return nullptr;
}

Peer* INetManager::GetPeerByHost(const std::string& host) {
    for (auto&& peer : m_onlinePeers) {
        if (peer->m_socket->GetHostName() == host)
            return peer;
    }
    return nullptr;
}

std::vector<Peer*> INetManager::GetPeers(const std::string& addr) {
    auto addrSplit = VUtils::String::Split(addr, ".");

    std::vector<Peer*> peers;

    for (auto&& peer : m_onlinePeers) {

        // use wildcards too
        std::string p = peer->m_socket->GetAddress();
        auto peerSplit = VUtils::String::Split(p, ".");

        bool match = true;

        for (auto&& sub1 = peerSplit.begin(), sub2 = addrSplit.begin(); 
            sub1 != peerSplit.end() && sub2 != addrSplit.end();
            sub1++, sub2++) 
        {
            if (*sub2 == "*") continue;
            if (*sub2 != *sub1) {
                match = false;
                break;
            }
        }

        if (match)
            peers.push_back(peer);

        //if (peer->m_socket->GetAddress() == buf)
            //return peer.get();
    }
    return peers;
}

void INetManager::PostInit() {
    LOG(INFO) << "Initializing NetManager";

    // load session file if replaying
    if (VH_SETTINGS.worldMode == WorldMode::PLAYBACK) {
        auto path = fs::path(VALHALLA_WORLD_RECORDING_PATH) 
            / WorldManager()->GetWorld()->m_name 
            / "sessions.pkg";

        if (auto opt = VUtils::Resource::ReadFile<BYTES_t>(path)) {
            DataReader reader(*opt);

            auto count = reader.Read<int32_t>();
            for (int i = 0; i < count; i++) {
                auto host = reader.Read<std::string>();
                auto nsStart = nanoseconds(reader.Read<int64_t>());
                auto nsEnd = nanoseconds(reader.Read<int64_t>());

                m_sortedSessions.push_back( { host, { nsStart, nsEnd } } );
            }
        }        
    }

    if (VH_SETTINGS.serverDedicated) m_acceptor = std::make_unique<AcceptorSteamDedicated>();
    else m_acceptor = std::make_unique<AcceptorSteamP2P>();

    m_acceptor->Listen();
}

void INetManager::Update() {
    OPTICK_CATEGORY("NetManagerUpdate", Optick::Category::Network);    

    // Accept new connections
    while (auto opt = m_acceptor->Accept()) {
        auto&& ptr = std::make_unique<Peer>(std::move(*opt));
        if (VH_DISPATCH_MOD_EVENT(IModManager::Events::Connect, ptr.get())) {
            Peer* peer = (*m_connectedPeers.insert(m_connectedPeers.end(), std::move(ptr))).get();
            
            if (VH_SETTINGS.worldMode == WorldMode::CAPTURE) {
                // record peer joindata
                m_sortedSessions.push_back({ peer->m_socket->GetHostName(),
                    { Valhalla()->Nanos(), 0ns } });
                peer->m_disconnectCapture = &m_sortedSessions.back().second.second;

                const fs::path root = fs::path(VALHALLA_WORLD_RECORDING_PATH) 
                    / WorldManager()->GetWorld()->m_name 
                    / peer->m_socket->GetHostName() 
                    / std::to_string(m_sessionIndexes[peer->m_socket->GetHostName()]++);

                std::string host = ptr->m_socket->GetHostName();

                peer->m_recordThread = std::jthread([root, peer, host](std::stop_token token) {
                    size_t chunkIndex = 0;

                    fs::create_directories(root);

                    auto&& saveBuffered = [&](int count) {
                        BYTES_t chunk;
                        DataWriter writer(chunk);

                        writer.Write((int32_t)count);
                        for (int i = 0; i < count; i++) {
                            nanoseconds ns;
                            BYTES_t packet;
                            {
                                std::scoped_lock<std::mutex> scoped(peer->m_recordmux);

                                auto&& front = peer->m_recordBuffer.front();
                                ns = front.first;
                                packet = std::move(front.second);
                                peer->m_captureQueueSize -= packet.size();

                                peer->m_recordBuffer.pop_front();
                            }
                            writer.Write(ns.count());
                            writer.Write(packet);
                        }

                        fs::path path = root / (std::to_string(chunkIndex++) + ".cap");

                        if (auto compressed = ZStdCompressor().Compress(chunk)) {
                            if (VUtils::Resource::WriteFile(path, *compressed))
                                LOG(WARNING) << "Saving " << path.c_str();
                            else
                                LOG(ERROR) << "Failed to save " << path.c_str();
                        }
                        else
                            LOG(ERROR) << "Failed to compress packet capture chunk";
                    };

                    // Primary occasional saving of captured packets
                    while (!token.stop_requested()) {
                        size_t size = 0;
                        size_t captureQueueSize = 0;
                        {
                            std::scoped_lock<std::mutex> scoped(peer->m_recordmux);
                            size = peer->m_recordBuffer.size();
                            captureQueueSize = peer->m_captureQueueSize;
                        }

                        // save at ~256Kb increments
                        if (captureQueueSize > VH_SETTINGS.worldCaptureDumpSize) {
                            saveBuffered(size);
                        }

                        std::this_thread::sleep_for(1ms);
                    }

                    LOG(WARNING) << "Terminating async capture writer " << host;

                    // Save any buffered captures before exit
                    int size = 0;
                    {
                        std::scoped_lock<std::mutex> scoped(peer->m_recordmux);
                        size = peer->m_recordBuffer.size();
                    }

                    if (size)
                        saveBuffered(size);

                });

                LOG(WARNING) << "Starting capture for " << peer->m_socket->GetHostName();
            }
        }
    }

    // accept replay peers
    if (VH_SETTINGS.worldMode == WorldMode::PLAYBACK) {
        if (!m_sortedSessions.empty()) {
            auto&& front = m_sortedSessions.front();
            if (Valhalla()->Nanos() >= front.second.first) {
                auto&& peer = std::make_unique<Peer>(
                    std::make_shared<ReplaySocket>(front.first, m_sessionIndexes[front.first]++, front.second.second));

                m_connectedPeers.push_back(std::move(peer));
                m_sortedSessions.pop_front();
            }
            else {
                PERIODIC_NOW(30s, {
                    LOG(INFO) << "Replay peer joining in " << duration_cast<seconds>(front.second.first - Valhalla()->Nanos());
                });
            }
        }
    }



    // Send periodic data (2s)
    PERIODIC_NOW(2s, {
        SendNetTime();
        SendPlayerList();
    });

    // Send periodic pings (1s)
    PERIODIC_NOW(1s, {
        BYTES_t bytes;
        DataWriter writer(bytes);
        writer.Write<HASH_t>(0);
        writer.Write(true);

        for (auto&& peer : m_connectedPeers) {
            peer->m_socket->Send(bytes);
        }
    });

    // Update peers
    for (auto&& peer : m_connectedPeers) {
        try {
            peer->Update();
        }
        catch (const std::runtime_error& e) {
            LOG(WARNING) << "Peer error";
            LOG(WARNING) << e.what();
            peer->m_socket->Close(false);
        }
    }

    // TODO I think this is in the correct location?
    if (VH_SETTINGS.serverDedicated)
        SteamGameServer_RunCallbacks();
    else 
        SteamAPI_RunCallbacks();

    // Cleanup
    {
        for (auto&& itr = m_onlinePeers.begin(); itr != m_onlinePeers.end(); ) {
            Peer& peer = *(*itr);

            if (!peer.m_socket->Connected()) {
                OnPeerQuit(peer);

                itr = m_onlinePeers.erase(itr);
            }
            else {
                ++itr;
            }
        }
    }

    {
        for (auto&& itr = m_connectedPeers.begin(); itr != m_connectedPeers.end(); ) {
            Peer& peer = *(*itr);

            if (!peer.m_socket->Connected()) {
                VH_DISPATCH_MOD_EVENT(IModManager::Events::Disconnect, peer);

                OnPeerDisconnect(peer);

                itr = m_connectedPeers.erase(itr);
            }
            else {
                ++itr;
            }
        }
    }
}

void INetManager::OnPeerQuit(Peer& peer) {
    LOG(INFO) << "Cleaning up peer";
    VH_DISPATCH_MOD_EVENT(IModManager::Events::Quit, peer);
    ZDOManager()->OnPeerQuit(peer);

    if (peer.m_admin)
        Valhalla()->m_admin.insert(peer.m_socket->GetHostName());
    else
        Valhalla()->m_admin.erase(peer.m_socket->GetHostName());
}

void INetManager::OnPeerDisconnect(Peer& peer) {
    ModManager()->CallEvent(IModManager::Events::Disconnect, peer);

    if (VH_SETTINGS.worldMode == WorldMode::CAPTURE) {
        *(peer.m_disconnectCapture) = Valhalla()->Nanos();
    }

    peer.SendDisconnect();

    LOG(INFO) << peer.m_socket->GetHostName() << " disconnected";
}

void INetManager::Uninit() {
    SendDisconnect();

    for (auto&& peer : m_onlinePeers) {
        OnPeerQuit(*peer);
    }

    for (auto&& peer : m_connectedPeers) {
        OnPeerDisconnect(*peer);
    }

    {
        // save sessions
        BYTES_t bytes;
        DataWriter writer(bytes);
        writer.Write((int)m_sortedSessions.size());
        for (auto&& session : m_sortedSessions) {
            writer.Write(std::string_view(session.first));
            writer.Write(session.second.first.count());
            writer.Write(session.second.second.count());
        }

        auto path = fs::path(VALHALLA_WORLD_RECORDING_PATH)
            / WorldManager()->GetWorld()->m_name
            / "sessions.pkg";

        VUtils::Resource::WriteFile(path, bytes);
    }

    m_acceptor.reset();
}
