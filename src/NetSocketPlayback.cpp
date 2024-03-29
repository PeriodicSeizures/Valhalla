#ifdef VH_OPTION_ENABLE_CAPTURE
#include "NetSocket.h"
#include "VUtilsResource.h"
#include "DataReader.h"
#include "ValhallaServer.h"
#include "WorldManager.h"

ReplaySocket::ReplaySocket(std::string host, int session, nanoseconds disconnectTime) {

    // trial is the connect/disconnect index

    this->m_disconnectTime = disconnectTime;

    const fs::path root = fs::path(VH_CAPTURE_PATH) 
        / WorldManager()->GetWorld()->m_name                    // the world being replayed
        / std::to_string(VH_SETTINGS.packetPlaybackSessionIndex)      // the world session being replayed
        / host                                                  // the player being replayed
        / std::to_string(session);                              // the player session being replayed

    m_thread = std::jthread([this, host, root](std::stop_token token) {
        size_t chunkIndex = 0;

        if (!fs::exists(root) || !fs::is_directory(root)) {
            LOG_WARNING(LOGGER, "Failed to find dir capture {}", root.c_str());
            return;
        }

        //fs::path path = root / (std::to_string(chunkIndex) + ".cap");

        while (!token.stop_requested()) {
            fs::path path = root / (std::to_string(chunkIndex) + ".cap");

            bool flag;
            {
                std::scoped_lock<std::mutex> scoped(m_mux);
                flag = m_ready.size() < 200;
            }

            if (flag) {
                if (auto opt = VUtils::Resource::ReadFile<BYTES_t>(path)) {
                    if (auto decompressed = ZStdDecompressor().Decompress(*opt)) {
                        DataReader reader(*decompressed);

                        auto count = reader.Read<int32_t>();
                        for (int i = 0; i < count; i++) {
                            auto ns = nanoseconds(reader.ReadInt64());
                            auto packet = reader.Read<BYTES_t>();

                            std::scoped_lock<std::mutex> scoped(m_mux);
                            m_ready.push_back({ ns, packet });
                        }

                        chunkIndex++;

                        //fs::path path = root / (std::to_string(++chunkIndex) + ".cap");
                    }
                    else {
                        LOG_WARNING(LOGGER, "Failed to decompress capture chunk {}", path.c_str());
                        break;
                    }

                }
                else {
                    LOG_WARNING(LOGGER, "Finished replay for {}", host);
                    break;
                }
            }
            std::this_thread::sleep_for(1ms);
        }

        LOG_WARNING(LOGGER, "Terminating async capture reader {}", host);
    });

    // Wait for some packets to prepare before returning
    while (true) {
        {
            std::scoped_lock<std::mutex> scoped(m_mux);
            if (!m_ready.empty()) break;
        }
        std::this_thread::sleep_for(1ms);
    }

    this->m_originalHost = "REPLAY_" + host;
}

void ReplaySocket::Close(bool flush) {
    m_thread.request_stop();
}

void ReplaySocket::Update() {
    if (Valhalla()->Nanos() >= m_disconnectTime) {
        this->Close(false);
    }
}

void ReplaySocket::Send(BYTES_t) {}

std::optional<BYTES_t> ReplaySocket::Recv() {
    std::scoped_lock<std::mutex> scoped(m_mux);
    if (m_ready.empty()) {
        LOG_WARNING(LOGGER, "No packets queued for replay");
        return std::nullopt;
    }

    auto&& ref = m_ready.front();

    if (Valhalla()->Nanos() >= ref.first) {
        BYTES_t result = std::move(ref.second);
        m_ready.pop_front();
        return result;
    }

    return std::nullopt;
}

std::string ReplaySocket::GetHostName() const { return m_originalHost; }

std::string ReplaySocket::GetAddress() const { return m_originalAddress; }

bool ReplaySocket::Connected() const { return !m_thread.get_stop_token().stop_requested(); }

unsigned int ReplaySocket::GetSendQueueSize() const { return 0; }

unsigned int ReplaySocket::GetPing() const { return 0; }

#endif 