#pragma once

#include "Method.h"
#include "ValhallaServer.h"
#include "DataReader.h"
#include "DataWriter.h"
#include "ModManager.h"
#include "Hashes.h"
#include "NetManager.h"

class Peer;

class IRouteManager {
	friend class INetManager;

public:
	static constexpr OWNER_t EVERYBODY = 0;

private:	
	UNORDERED_MAP_t<HASH_t, std::unique_ptr<IMethod<Peer*>>> m_methods;

private:
	// Called from NetManager
	void OnNewPeer(Peer &peer);

public:

	/**
		* @brief Register a static method for routed remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<typename F>
	void Register(HASH_t hash, F func) {
		m_methods[hash] = std::make_unique<MethodImpl<Peer*, F>>(func, IModManager::Events::RouteIn, hash);
	}

	template<typename F>
	decltype(auto) Register(std::string_view name, F func) {
		return Register(VUtils::String::GetStableHashCode(name), func);
	}

	void RegisterLua(const IModManager::MethodSig& sig, sol::function func) {
		//VLOG(1) << "RegisterLua, func: " << sol::state_view(func.lua_state())["tostring"](func).get<std::string>() << ", hash: " << sig.m_hash;

		m_methods[sig.m_hash] = std::make_unique<MethodImplLua<Peer*>>(func, sig.m_types);
	}


	// Forwards raw data to peer(s) with no Lua handlers
	//void InvokeParams(OWNER_t target, const ZDOID& targetZDO, HASH_t hash, BYTES_t params);


	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(OWNER_t target, ZDOID targetZDO, HASH_t hash, Args&&... params) {
		// Prefix
		if (target == EVERYBODY) {
			// targetZDO can have a value apparently
			if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::RouteOutAll ^ hash, targetZDO, params...))
				return;

			auto bytes = Serialize(VH_ID, target, targetZDO, hash, DataWriter::Serialize(params...));

			for (auto&& peer : NetManager()->GetPeers()) {
				peer->Invoke(Hashes::Rpc::RoutedRPC, bytes);
			}
		}
		else {
			if (auto peer = NetManager()->GetPeerByUUID(target)) {
				peer->RouteView(targetZDO, hash, std::forward<Args>(params)...);
			}
		}
	}

	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(OWNER_t target, ZDOID targetZDO, std::string_view name, Args&&... params) {
		InvokeView(target, targetZDO, VUtils::String::GetStableHashCode(name), std::forward<Args>(params)...);
	}

	void InvokeViewLua(Int64Wrapper target, ZDOID targetZDO, const IModManager::MethodSig& repr, const sol::variadic_args& args) {		
		if ((OWNER_t)target == EVERYBODY) {
			if (args.size() != repr.m_types.size())
				throw std::runtime_error("mismatched number of args");

			auto results = sol::variadic_results(args.begin(), args.end());

#ifdef MOD_EVENT_RESPONSE
			if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::RouteOutAll ^ repr.m_hash, sol::as_args(results)))
				return;
#endif

			auto bytes = Serialize(VH_ID, (OWNER_t) target, targetZDO, repr.m_hash, DataWriter::SerializeExtLua(repr.m_types, results));

			for (auto&& peer : NetManager()->GetPeers()) {
				peer->Invoke(Hashes::Rpc::RoutedRPC, bytes);
			}
		}
		else {
			if (auto peer = NetManager()->GetPeerByUUID(target))
				peer->RouteViewLua(targetZDO, repr, args);
		}
		
		//Serialize(VH_ID, target, targetZDO, repr.m_hash,
			//DataWriter::SerializeLua(repr.m_types, sol::variadic_results(args.begin(), args.end())));
		
		//Invoke(target, targetZDO, repr.m_hash, DataWriter::SerializeLua(repr.m_types, sol::variadic_results(args.begin(), args.end())));
	}



	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, HASH_t hash, Args&&... params) {
		InvokeView(target, ZDOID::NONE, hash, std::forward<Args>(params)...);
	}

	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, std::string_view name, Args&&... params) {
		InvokeView(target, ZDOID::NONE, VUtils::String::GetStableHashCode(name), std::forward<Args>(params)...);
	}

	void InvokeLua(Int64Wrapper target, const IModManager::MethodSig& repr, const sol::variadic_args& args) {
		InvokeViewLua(target, ZDOID::NONE, repr, args);
	}



	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(HASH_t hash, Args&&... params) {
		Invoke(EVERYBODY, hash, std::forward<Args>(params)...);
	}

	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(std::string_view name, Args&&... params) {
		Invoke(EVERYBODY, VUtils::String::GetStableHashCode(name), std::forward<Args>(params)...);
	}

	void InvokeAllLua(const IModManager::MethodSig& repr, const sol::variadic_args& args) {
		InvokeLua(EVERYBODY, repr, args);
	}

	BYTES_t Serialize(OWNER_t sender, OWNER_t target, ZDOID targetZDO, HASH_t hash, BYTES_t params) {
		BYTES_t bytes;
		DataWriter writer(bytes);

		writer.Write<int64_t>(0); // msg id
		writer.Write(sender);
		writer.Write(target);
		writer.Write(targetZDO);
		writer.Write(hash);
		writer.Write(params);

		return bytes;
	}

};

// Manager class for everything related to high-level networking for simulated p2p communication
IRouteManager* RouteManager();
