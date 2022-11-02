#pragma once

#include <memory>
#include <thread>

#include "NetSocket.h"

class IAcceptor {
public:
	virtual ~IAcceptor() = default;

	// Init listening and queueing any accepted connections
	virtual void Listen() = 0;

	// Poll for a ready and newly accepted connection
	virtual ISocket *Accept() = 0;

    // Internally cleans up the socket
    // Expected to be closed with no external references
    virtual void Cleanup(ISocket* socket) = 0;
};

class AcceptorSteam : public IAcceptor {
private:
	uint16_t m_port;
	HSteamListenSocket m_listenSocket;

	robin_hood::unordered_map<HSteamNetConnection, std::unique_ptr<SteamSocket>> m_sockets;	// holds all sockets and manages lifetime
	robin_hood::unordered_map<HSteamNetConnection, SteamSocket*> m_connecting;	            // holds all newly connecting sockets
	robin_hood::unordered_map<HSteamNetConnection, SteamSocket*> m_connected;	            // holds sockets ready for application

public:
	AcceptorSteam();
	~AcceptorSteam() override;

	void Listen() override;

	ISocket *Accept() override;

    void Cleanup(ISocket* socket) override;

private:
	// https://partner.steamgames.com/doc/sdk/api#callbacks
	STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamStatusChanged, SteamNetConnectionStatusChangedCallback_t);

	// status logs
	STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServersConnected, SteamServersConnected_t);
	STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServersDisconnected, SteamServersDisconnected_t);
	STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServerConnectFailure, SteamServerConnectFailure_t);
};

class RCONAcceptor : public IAcceptor {
private:
    asio::io_context m_ctx;
    std::thread m_thread;
    asio::ip::tcp::acceptor m_acceptor;

    std::mutex m_mut;
    robin_hood::unordered_map<RCONSocket*, std::unique_ptr<RCONSocket>> m_sockets;
    robin_hood::unordered_set<RCONSocket*> m_connected;

public:
    RCONAcceptor();
    ~RCONAcceptor() noexcept override;

    // Init listening and queueing any accepted connections
    void Listen() override;

    // Poll for a ready and newly accepted connection
    ISocket *Accept() override;

    // Internally cleans up the socket
    // Expected to be closed with no external references
    void Cleanup(ISocket* socket) override;

private:
    void DoAccept();
};
