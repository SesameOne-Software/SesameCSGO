#pragma once
#include <windows.h>
#include <string>
#include "../security/security_handler.hpp"
#include "utils.hpp"

namespace networking {
	static std::string get ( const std::string& url ) {
		std::string data;

		const auto slash = url.find ( _ ( "/" ) );

		std::string split_url = url;
		std::string split_path = _ ( "/" );

		if ( slash != std::string::npos ) {
			split_url = url.substr ( N ( 0 ), slash );
			split_path = url.substr ( slash );
		}

		std::string request = _ ( "GET /" ) + split_path + _ ( " HTTP/1.1\r\n" );
		request += _ ( "Host: " ) + split_url + _ ( "\r\n" );
		//request += _ ( "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36\r\n" );
		request += _ ( "\r\n" );

		WSADATA wsa_data;
		if ( WSAStartup ( MAKEWORD ( N ( 2 ), N ( 2 ) ), &wsa_data ) != N ( 0 ) ) {
			dbg_print ( _ ( "Connection to sesame servers failed.\n" ) );
			return _ ( "ERROR" );
		}

		const auto sock = socket ( N ( AF_INET ), N ( SOCK_STREAM ), N ( IPPROTO_TCP ) );
		const auto host = gethostbyname ( split_url.c_str ( ) );

		SOCKADDR_IN sock_addr;
		sock_addr.sin_port = htons ( N ( 80 ) );
		sock_addr.sin_family = N ( AF_INET );
		sock_addr.sin_addr.s_addr = *( ( unsigned long* ) host->h_addr );

		if ( connect ( sock, ( sockaddr* ) ( &sock_addr ), sizeof ( sock_addr ) ) != N ( 0 ) ) {
			dbg_print ( _ ( "Connection to sesame servers failed.\n" ) );
			return _ ( "ERROR" );
		}

		send ( sock, request.c_str ( ), strlen ( request.c_str ( ) ), N ( 0 ) );

		auto buffer = new char [ N ( 10000 ) ];
		size_t data_len = N ( 0 );

		while ( true ) {
			memset ( buffer, 0, N ( 10000 ) );

			data_len = recv ( sock, buffer, N ( 10000 ), N( 0 ) );

			if ( data_len <= 0 )
				break;

			data += std::string ( buffer, data_len );
		}

		delete [ ] buffer;

		closesocket ( sock );
		WSACleanup ( );

		/* remove header */
		const auto delim = data.find ( _ ( "\r\n\r\n" ) );

		if ( delim == std::string::npos )
			return _ ( "ERROR" );

		data = data.substr ( delim + 4 );

		return data;
	}
}