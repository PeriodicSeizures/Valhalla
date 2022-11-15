#pragma once

#include "NetPeer.h"
#include "Method.h"
#include "VServer.h"

namespace NetRouteManager {
	static constexpr OWNER_t EVERYBODY = 0;

	struct Data {
		OWNER_t m_msgID;
		OWNER_t m_senderPeerID;
		OWNER_t m_targetPeerID;
		NetID m_targetSync;
		HASH_t m_methodHash;
		NetPackage m_parameters;

		Data() : m_msgID(0), m_senderPeerID(0), m_targetPeerID(0), m_methodHash(0) {}

		// Will unpack the package
		explicit Data(NetPackage& pkg)
			: m_msgID(pkg.Read<OWNER_t>()),
              m_senderPeerID(pkg.Read<OWNER_t>()),
              m_targetPeerID(pkg.Read<OWNER_t>()),
              m_targetSync(pkg.Read<NetID>()),
              m_methodHash(pkg.Read<HASH_t>()),
              m_parameters(pkg.Read<NetPackage>())
		{}

		void Serialize(NetPackage &pkg) const {
			pkg.Write(m_msgID);
			pkg.Write(m_senderPeerID);
			pkg.Write(m_targetPeerID);
			pkg.Write(m_targetSync);
			pkg.Write(m_methodHash);
			pkg.Write(m_parameters);
		}
	};

	// Called from NetManager
	void OnNewPeer(NetPeer *peer);
	void OnPeerQuit(NetPeer *peer);

	// Internal use only by NetRouteManager
	void _Invoke(OWNER_t target, const NetID& targetNetSync, HASH_t hash, const NetPackage& pkg);
	void _HandleRoutedRPC(Data data);

	// Registers a RoutedRpc function
	// Must pass an IMethod created using new
	void _Register(HASH_t hash, IMethod<OWNER_t>* method);



	/**
		* @brief Register a static method for routed remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<class ...Args>
	auto Register(HASH_t hash, void(*f)(OWNER_t, Args...)) {
		return _Register(hash, new MethodImpl(f, EVENT_HASH_RouteIn ^ hash));
	}

	template<class ...Args>
	auto Register(Routed_Hash hash, void(*f)(OWNER_t, Args...)) {
		return Register(static_cast<HASH_t>(hash), f);
	}

	template<class ...Args>
	auto Register(const char* name, void(*f)(OWNER_t, Args...)) {
		return Register(VUtils::String::GetStableHashCode(name), f);
	}

	template<class ...Args>
	auto Register(std::string& name, void(*f)(OWNER_t, Args...)) {
		return Register(name.c_str(), f);
	}



	/**
		* @brief Register an instance method for routed remote invocation
		* @param name function name to register
		* @param object the object containing the member function
		* @param method ptr to a member function
	*/
	template<class C, class ...Args>
	auto Register(HASH_t hash, C* object, void(C::* f)(OWNER_t, Args...)) {
		return _Register(hash, object, new MethodImpl(object, f, EVENT_HASH_RouteIn ^ hash));
	}

	template<class C, class ...Args>
	auto Register(Routed_Hash hash, C* object, void(C::* f)(OWNER_t, Args...)) {
		return Register(static_cast<HASH_t>(hash), object, f);
	}

	template<class C, class ...Args>
	auto Register(const char* name, C* object, void(C::* f)(OWNER_t, Args...)) {
		return Register(VUtils::String::GetStableHashCode(name), object, f);
	}

	template<class C, class ...Args>
	auto Register(std::string& name, C* object, void(C::* f)(OWNER_t, Args...)) {
		return Register(name.c_str(), object, f);
	}



	/**
		* @brief Invoke a routed function bound to a peer with sub zdo
		* @param name function name
		* @param ...types function parameters
	*/
	template <typename... Args>
	void Invoke(OWNER_t target, const NetID& targetNetSync, HASH_t hash, const Args&... params) {
		_Invoke(target, targetNetSync, hash, NetPackage::Serialize(params...));
	}

	template <typename... Args>
	void Invoke(OWNER_t target, const NetID& targetNetSync, Routed_Hash hash, const Args&... params) {
		Invoke(target, targetNetSync, static_cast<HASH_t>(hash), params...);
	}

	template <typename... Args>
	void Invoke(OWNER_t target, const NetID& targetNetSync, const char* name, const Args&... params) {
		Invoke(target, targetNetSync, VUtils::String::GetStableHashCode(name), params...);
	}

	template <typename... Args>
	void Invoke(OWNER_t target, const NetID& targetNetSync, std::string& name, const Args&... params) {
		Invoke(target, targetNetSync, name.c_str(), params...);
	}



	/**
		* @brief Invoke a routed function bound to a peer
		* @param name function name
		* @param ...types function parameters
	*/
	template <typename... Args>
	void Invoke(OWNER_t target, HASH_t hash, const Args&... params) {
		Invoke(target, NetID::NONE, hash, params...);
	}

	template <typename... Args>
	void Invoke(OWNER_t target, Routed_Hash hash, const Args&... params) {
		Invoke(target, NetID::NONE, static_cast<HASH_t>(hash), params...);
	}

	template <typename... Args>
	void Invoke(OWNER_t target, const char* name, const Args&... params) {
		Invoke(target, NetID::NONE, VUtils::String::GetStableHashCode(name), params...);
	}

	template <typename... Args>
	void Invoke(OWNER_t target, std::string& name, const Args&... params) {
		Invoke(target, NetID::NONE, name.c_str(), params...);
	}
};