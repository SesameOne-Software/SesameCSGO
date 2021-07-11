#include "socket.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>

#include "../security/security_handler.hpp"

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

namespace client {
	std::mutex tcp_mutex;
	uintptr_t tcp_socket = INVALID_SOCKET;

	bool create_socket(const char* url, const char* port) {
		OBF_BEGIN;
		WSADATA wsaData;
		DWORD ipv6only = 0;
		int iResult;
		BOOL bSuccess;
		SOCKADDR_STORAGE LocalAddr = { 0 };
		SOCKADDR_STORAGE RemoteAddr = { 0 };
		DWORD dwLocalAddr = sizeof(LocalAddr);
		DWORD dwRemoteAddr = sizeof(RemoteAddr);

		iResult = LI_FN( WSAStartup )(N(MAKEWORD(2, 2)), &wsaData);
		
		IF ( iResult != 0 ) {
			RETURN( false );
		} ENDIF;

		tcp_mutex.lock();

		tcp_socket = LI_FN( socket )(N( AF_INET6 ), N( SOCK_STREAM ), N(0));

		IF (tcp_socket == N(INVALID_SOCKET)) {
			tcp_mutex.unlock();
			RETURN ( false );
		} ENDIF

		iResult = LI_FN( setsockopt )(tcp_socket, IPPROTO_IPV6,
			N(IPV6_V6ONLY), (char*)&ipv6only, N(sizeof(ipv6only)));

		IF ( iResult == SOCKET_ERROR ) {
			LI_FN( closesocket ) ( tcp_socket );
			tcp_mutex.unlock ( );
			RETURN ( false );
		} ENDIF;

		BOOL no_delay = N(TRUE);

		iResult = LI_FN( setsockopt )(tcp_socket, IPPROTO_TCP,
			N(TCP_NODELAY), (char*)&no_delay, N(sizeof(no_delay)));

		IF ( iResult == SOCKET_ERROR ) {
			LI_FN( closesocket ) ( tcp_socket );
			tcp_mutex.unlock ( );
			RETURN ( false );
		} ENDIF;

		bSuccess = LI_FN ( WSAConnectByNameA )( tcp_socket, url,
			port, &dwLocalAddr,
			( SOCKADDR* ) &LocalAddr,
			&dwRemoteAddr,
			( SOCKADDR* ) &RemoteAddr,
			( timeval* ) NULL,
			( LPWSAOVERLAPPED ) NULL );

		tcp_mutex.unlock();

		IF (bSuccess)
			RETURN ( true ); ENDIF;

		RETURN ( false );
		OBF_END;
	}

	bool send_buffer(const char* buffer, int bufflen) {
		OBF_BEGIN;
		tcp_mutex.lock();

		IF ( client::tcp_socket == N(INVALID_SOCKET) ) {
			tcp_mutex.unlock ( );
			RETURN ( false );
		} ENDIF;

		int bytes_sent = 0;
		bytes_sent = LI_FN( send )(tcp_socket, buffer, bufflen, 0);

		IF ( bytes_sent == SOCKET_ERROR ) {
			tcp_mutex.unlock ( );
			close_connection ( );
			RETURN ( false );
		} ENDIF;

		tcp_mutex.unlock();

		RETURN ( true );
		OBF_END;
	}

	const char* read_buffer() {
		OBF_BEGIN;
		tcp_mutex.lock();
		static char buffer[4096] = { 0 };
		int iResult = N( 1 );

		memset ( buffer, N(0), N( sizeof( buffer )) );

		REPEAT {
			iResult = LI_FN ( recv )( tcp_socket, ( char* ) buffer, N ( sizeof ( buffer ) ), N ( 0 ) );
			
			IF ( iResult > 0 ) {
				tcp_mutex.unlock ( );
				RETURN ( strdup ( buffer ) );
			} ENDIF;
		} AS_LONG_AS ( iResult > 0 );

		IF ( iResult == SOCKET_ERROR ) {
			tcp_mutex.unlock ( );
			close_connection ( );
		} ENDIF;

		tcp_mutex.unlock();

		RETURN( nullptr );
		OBF_END;
	}

	void close_connection() {
		tcp_mutex.lock();
		LI_FN( closesocket )(tcp_socket);
		tcp_mutex.unlock();
		LI_FN( WSACleanup )();
	}
}