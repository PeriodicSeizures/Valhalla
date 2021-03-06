#include "ZSocket.hpp"
#include <string>
#include <iostream>

ZSocket2::ZSocket2(asio::io_context& ctx)
	: m_socket(ctx) {}

ZSocket2::~ZSocket2() {
	//while (!m_sendQueue.empty()) delete m_sendQueue.pop_front();
	//while (!m_recvQueue.empty()) delete m_recvQueue.pop_front();
	Close();
}



void ZSocket2::Accept() {
	LOG(INFO) << "NetSocket2 Accept";

	m_hostname = m_socket.remote_endpoint().address().to_string();
	m_port = m_socket.remote_endpoint().port();
	ReadPkgSize();
}


/**
	* In ZSocket2, the packet is sent with a single dispatch
	* the size and data is written to the same buffer
	* Im not doing for readability
*/
void ZSocket2::Send(ZPackage pkg) {
	if (pkg.GetStream().Length() == 0)
		return;

	if (pkg.GetStream().Length() + 4 > 10485760)
		LOG(ERROR) << "Too big package";

	if (m_online) {
		const bool was_empty = m_sendQueue.empty();
		m_sendQueue.push_back(std::move(pkg));
		if (was_empty) {
			WritePkgSize();
		}
	}
}

bool ZSocket2::HasNewData() {
	return !m_recvQueue.empty();
}

ZPackage ZSocket2::Recv() {
	return m_recvQueue.pop_front();
}



bool ZSocket2::Close() {
	if (m_online) {
		LOG(INFO) << "NetSocket2::Close()";

		m_online = false;

		m_socket.close();

		return true;
	}

	return false;
}



std::string& ZSocket2::GetHostName() {
	return m_hostname;
}

uint_least16_t ZSocket2::GetHostPort() {
	return m_port;
}

bool ZSocket2::IsOnline() {
	return m_online;
}

tcp::socket& ZSocket2::GetSocket() {
	return m_socket;
}



void ZSocket2::ReadPkgSize() {
	auto self(shared_from_this());
	asio::async_read(m_socket,
		asio::buffer(&m_tempReadOffset, 4),
		[this, self](const std::error_code& e, size_t) {
		if (!e) {
			ReadPkg();
		}
		else {
			LOG(DEBUG) << "read header error: " << e.message() << " (" << e.value() << ")";
			Close();
		}
	}
	);
}

void ZSocket2::ReadPkg() {
	if (m_tempReadOffset == 0 || m_tempReadOffset > 10485760) {
		LOG(ERROR) << "Invalid pkg size received " << m_tempReadOffset;
		Close();
	}
	else {
		//m_recvQueue.push_back(ZPackage(m_tempReadOffset));
		//auto &front = m_recvQueue.front();
		m_recv.GetStream().Reserve(m_tempReadOffset);
		//auto pkg = new ZPackage(); //new Package(m_tempReadOffset);
		// the initialization is pointless
		//pkg->Buffer().resize(m_tempReadOffset);

		auto self(shared_from_this());
		asio::async_read(m_socket,
			asio::buffer(m_recv.GetStream().Bytes(), m_tempReadOffset), // whether vec needs to be reserved or resized
			[this, self](const std::error_code& e, size_t) {
			if (!e) {
				m_recv.GetStream().SetLength(m_tempReadOffset);
				m_recv.GetStream().ResetPos();
				m_recvQueue.push_back(std::move(m_recv));
				ReadPkgSize();
			}
			else {
				LOG(DEBUG) << "read body error: " << e.message() << " (" << e.value() << ")";
				Close();
			}
		});
	}
}

void ZSocket2::WritePkgSize() {
	auto &pkg = m_sendQueue.front();

	m_tempWriteOffset = pkg.GetStream().Length();

	auto self(shared_from_this());
	asio::async_write(m_socket,
		asio::buffer(&m_tempWriteOffset, 4),
		[this, self, &pkg](const std::error_code& e, size_t) {
		if (!e) {
			WritePkg(pkg);
		}
		else {
			LOG(DEBUG) << "write header error: " << e.message() << " (" << e.value() << ")";
			Close();
		}
	});
}

void ZSocket2::WritePkg(ZPackage &pkg) {
	auto self(shared_from_this());
	asio::async_write(m_socket,
		asio::buffer(pkg.GetStream().Bytes(), m_tempWriteOffset),
		[this, self](const std::error_code& e, size_t) {
		if (!e) {
			m_sendQueue.pop_front();
			if (!m_sendQueue.empty()) {
				WritePkgSize();
			}
		}
		else {
			LOG(DEBUG) << "write body error: " << e.message() << " (" << e.value() << ")";
			Close();
		}
	});
}
