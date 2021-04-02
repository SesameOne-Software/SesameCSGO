#include "socket.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

namespace client {
	std::mutex tcp_mutex;
	uintptr_t tcp_socket = INVALID_SOCKET;

	bool create_socket(const char* url, const char* port) {
		WSADATA wsaData;
		DWORD ipv6only = 0;
		int iResult;
		BOOL bSuccess;
		SOCKADDR_STORAGE LocalAddr = { 0 };
		SOCKADDR_STORAGE RemoteAddr = { 0 };
		DWORD dwLocalAddr = sizeof(LocalAddr);
		DWORD dwRemoteAddr = sizeof(RemoteAddr);

		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		
		if (iResult != 0) {
			return false;
		}

		tcp_mutex.lock();

		tcp_socket = socket(AF_INET6, SOCK_STREAM, 0);
		if (tcp_socket == INVALID_SOCKET) {
			tcp_mutex.unlock();
			return false;
		}

		iResult = setsockopt(tcp_socket, IPPROTO_IPV6,
			IPV6_V6ONLY, (char*)&ipv6only, sizeof(ipv6only));
		if (iResult == SOCKET_ERROR) {
			closesocket(tcp_socket);
			tcp_mutex.unlock();
			return false;
		}

		BOOL no_delay = TRUE;

		iResult = setsockopt(tcp_socket, IPPROTO_TCP,
			TCP_NODELAY, (char*)&no_delay, sizeof(no_delay));
		if (iResult == SOCKET_ERROR) {
			closesocket(tcp_socket);
			tcp_mutex.unlock();
			return false;
		}

		bSuccess = WSAConnectByNameA(tcp_socket, url,
			port, &dwLocalAddr,
			(SOCKADDR*)&LocalAddr,
			&dwRemoteAddr,
			(SOCKADDR*)&RemoteAddr,
			NULL,
			NULL);

		tcp_mutex.unlock();

		if (bSuccess)
			return true;

		return false;
	}

	bool send_buffer(const char* buffer, int bufflen) {
		tcp_mutex.lock();

		if (client::tcp_socket == INVALID_SOCKET) {
			tcp_mutex.unlock();
			return false;
		}

		int bytes_sent = 0;
		bytes_sent = send(tcp_socket, buffer, bufflen, 0);

		if (bytes_sent == SOCKET_ERROR) {
			tcp_mutex.unlock();
			close_connection();
			return false;
		}

		tcp_mutex.unlock();

		return true;
	}

	std::string read_buffer() {
		tcp_mutex.lock();
		char buffer[4096] = { 0 };
		int iResult = 1;
		do {
			iResult = recv(tcp_socket, (char*)buffer, sizeof(buffer), 0);
			if (iResult > 0) {
				tcp_mutex.unlock();
				return buffer;
			}
		} while (iResult > 0);

		if (iResult == SOCKET_ERROR) {
			tcp_mutex.unlock();
			close_connection();
		}

		tcp_mutex.unlock();

		return {};
	}

	void close_connection() {
		tcp_mutex.lock();
		closesocket(tcp_socket);
		tcp_mutex.unlock();
		WSACleanup();
	}
}