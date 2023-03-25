#include "ValhallaServer.h"
#include "NetManager.h"
#include "ZDOManager.h"
#include "RouteManager.h"

static const char* STATUS_STRINGS[] = { "None", "Connecting", "Connected",
    "ErrorVersion", "ErrorDisconnected", "ErrorConnectFailed", "ErrorPassword",
    "ErrorAlreadyConnected", "ErrorBanned", "ErrorFull" };

// Static globals initialized once
std::string Peer::PASSWORD;
std::string Peer::SALT;

Peer::Peer(ISocket::Ptr socket)
    : m_socket(std::move(socket)), m_lastPing(steady_clock::now())
{
    this->Register(Hashes::Rpc::Disconnect, [](Peer* self) {
        LOG(INFO) << "RPC_Disconnect";
        self->Disconnect();
    });

    this->Register(Hashes::Rpc::C2S_Handshake, [](Peer* rpc) {
        rpc->Register(Hashes::Rpc::PeerInfo, [](Peer* rpc, BYTES_t bytes) {
            // Forward call to rpc

            DataReader reader(bytes);

            rpc->m_uuid = reader.Read<OWNER_t>();
            if (!rpc->m_uuid)
                throw std::runtime_error("peer provided 0 owner");

            auto version = reader.Read<std::string>();
            LOG(INFO) << "Client " << rpc->m_socket->GetHostName() << " has version " << version;
            if (version != VConstants::GAME)
                return rpc->Close(ConnectionStatus::ErrorVersion);

            rpc->m_pos = reader.Read<Vector3>();
            rpc->m_name = reader.Read<std::string>();
            //if (!(name.length() >= 3 && name.length() <= 15))
                //throw std::runtime_error("peer provided invalid length name");

            auto password = reader.Read<std::string>();
            auto ticket = reader.Read<BYTES_t>(); // read in the dummy ticket

            if (SERVER_SETTINGS.playerAuth) {
                auto steamSocket = std::dynamic_pointer_cast<SteamSocket>(rpc->m_socket);
                if (steamSocket && SteamGameServer()->BeginAuthSession(ticket.data(), ticket.size(), steamSocket->m_steamNetId.GetSteamID()) != k_EBeginAuthSessionResultOK)
                    return rpc->Close(ConnectionStatus::ErrorBanned);
            }

            if (Valhalla()->m_blacklist.contains(rpc->m_socket->GetHostName()))
                return rpc->Close(ConnectionStatus::ErrorBanned);

            // sanitize name (spaces, any ascii letters)
            //for (auto ch : name) {
            //    if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))) {
            //        LOG(INFO) << "Player has unsupported name character: " << (int)ch;
            //        return rpc->Close(ConnectionStatus::ErrorDisconnected);
            //    }
            //}


            if (password != PASSWORD)
                return rpc->Close(ConnectionStatus::ErrorPassword);



            // if peer already connected
            if (NetManager()->GetPeer(rpc->m_uuid))
                return rpc->Close(ConnectionStatus::ErrorAlreadyConnected);

            // if whitelist enabled
            if (SERVER_SETTINGS.playerWhitelist
                && !Valhalla()->m_whitelist.contains(rpc->m_socket->GetHostName())) {
                return rpc->Close(ConnectionStatus::ErrorFull);
            }

            // if too many players online
            if (NetManager()->GetPeers().size() >= SERVER_SETTINGS.playerMax)
                return rpc->Close(ConnectionStatus::ErrorFull);

            //NetManager()->OnNewClient(rpc->m_socket, uuid, name, pos);

            NetManager()->OnNewPeer(*rpc);

            return false;
        });

        bool hasPassword = !SERVER_SETTINGS.serverPassword.empty();

        if (hasPassword) {
            // Init password statically once
            if (PASSWORD.empty()) {
                SALT = VUtils::Random::GenerateAlphaNum(16);

                const auto merge = SERVER_SETTINGS.serverPassword + SALT;

                // Hash a salted password
                PASSWORD.resize(16);
                MD5(reinterpret_cast<const uint8_t*>(merge.c_str()),
                    merge.size(), reinterpret_cast<uint8_t*>(PASSWORD.data()));
                
                VUtils::String::FormatAscii(PASSWORD);
            }
        }

        rpc->Invoke(Hashes::Rpc::S2C_Handshake, hasPassword, SALT);

        return false;
    });
}

void Peer::Update() {
    OPTICK_EVENT();

    auto now(steady_clock::now());

    // Send packet data
    m_socket->Update();

    // Read packets
    while (auto opt = m_socket->Recv()) {

        auto&& bytes = opt.value();
        DataReader reader(bytes);

        auto hash = reader.Read<HASH_t>();
        if (hash == 0) {
            if (reader.Read<bool>()) {
                // Reply to the server with a pong
                bytes.clear();
                DataWriter writer(bytes);
                writer.Write<HASH_t>(0);
                writer.Write<bool>(false);
                m_socket->Send(std::move(bytes));
            }
            else {
                m_lastPing = now;
            }
        }
        else {
            InternalInvoke(hash, reader);
        }
    }

    if (now - m_lastPing > SERVER_SETTINGS.socketTimeout) {
        LOG(INFO) << "Client RPC timeout";
        Disconnect();
    }
}

bool Peer::Close(ConnectionStatus status) {
    LOG(INFO) << "RpcClient error: " << STATUS_STRINGS[(int)status];
    Invoke(Hashes::Rpc::S2C_Error, status);
    Disconnect();
    return false;
}



void Peer::ChatMessage(const std::string& text, ChatMsgType type, const Vector3& pos, const UserProfile& profile, const std::string& senderID) {
    RouteManager()->Invoke(m_uuid, Hashes::Routed::ChatMessage,
        pos, //Vector3(10000, 10000, 10000),
        type,
        profile, //"<color=yellow><b>SERVER</b></color>",
        text,
        senderID //""
    );
}

void Peer::UIMessage(const std::string& text, UIMsgType type) {
    RouteManager()->Invoke(m_uuid, Hashes::Routed::S2C_UIMessage, type, text);
}

ZDO* Peer::GetZDO() {
    return ZDOManager()->GetZDO(m_characterID);
}

void Peer::Teleport(const Vector3& pos, const Quaternion& rot, bool animation) {
    RouteManager()->Invoke(m_uuid, Hashes::Routed::S2C_RequestTeleport,
        pos,
        rot,
        animation
    );
}



void Peer::ZDOSectorInvalidated(ZDO& zdo) {
    if (zdo.IsOwner(this->m_uuid))
        return;

    if (!ZoneManager()->ZonesOverlap(zdo.GetZone(), m_pos)) {
        if (m_zdos.erase(zdo.ID())) {
            m_invalidSector.insert(zdo.ID());
        }
    }
}

bool Peer::IsOutdatedZDO(ZDO& zdo, decltype(m_zdos)::iterator& outItr) {
    auto&& find = m_zdos.find(zdo.ID());

    outItr = find;

    return find == m_zdos.end()
        || zdo.m_rev.m_ownerRev > find->second.m_ownerRev
        || zdo.m_rev.m_dataRev > find->second.m_dataRev;
}