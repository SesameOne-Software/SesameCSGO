#pragma once
#include <windows.h>
#include <string>
#include "../security/security_handler.hpp"
#include "utils.hpp"

static const unsigned char ses_pfp[1227UL + 1] = {
  0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x01, 0x00, 0x60, 0x00, 0x60, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x3B, 0x43, 0x52, 0x45, 0x41, 0x54, 0x4F, 0x52, 0x3A, 0x20, 0x67, 0x64, 0x2D, 0x6A, 0x70, 0x65, 0x67,
  0x20, 0x76, 0x31, 0x2E, 0x30, 0x20, 0x28, 0x75, 0x73, 0x69, 0x6E, 0x67, 0x20, 0x49, 0x4A, 0x47, 0x20, 0x4A, 0x50, 0x45, 0x47, 0x20, 0x76, 0x36, 0x32, 0x29, 0x2C, 0x20, 0x71, 0x75, 0x61, 0x6C, 0x69, 0x74, 0x79, 0x20, 0x3D, 0x20, 0x38, 0x35,
  0x0A, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x05, 0x03, 0x04, 0x04, 0x04, 0x03, 0x05, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x06, 0x07, 0x0C, 0x08, 0x07, 0x07, 0x07, 0x07, 0x0F, 0x0B, 0x0B, 0x09, 0x0C, 0x11, 0x0F, 0x12, 0x12, 0x11, 0x0F, 0x11, 0x11,
  0x13, 0x16, 0x1C, 0x17, 0x13, 0x14, 0x1A, 0x15, 0x11, 0x11, 0x18, 0x21, 0x18, 0x1A, 0x1D, 0x1D, 0x1F, 0x1F, 0x1F, 0x13, 0x17, 0x22, 0x24, 0x22, 0x1E, 0x24, 0x1C, 0x1E, 0x1F, 0x1E, 0xFF, 0xDB, 0x00, 0x43, 0x01, 0x05, 0x05, 0x05, 0x07, 0x06,
  0x07, 0x0E, 0x08, 0x08, 0x0E, 0x1E, 0x14, 0x11, 0x14, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
  0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0xFF, 0xC0, 0x00, 0x11, 0x08, 0x00, 0x30, 0x00, 0x30, 0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, 0xFF, 0xC4,
  0x00, 0x1F, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0xFF, 0xC4, 0x00, 0xB5, 0x10, 0x00, 0x02, 0x01, 0x03,
  0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1,
  0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55,
  0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3,
  0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6,
  0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xC4, 0x00, 0x1F, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
  0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0xFF, 0xC4, 0x00, 0xB5, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41,
  0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A,
  0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83,
  0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
  0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDA, 0x00, 0x0C, 0x03, 0x01, 0x00, 0x02, 0x11, 0x03,
  0x11, 0x00, 0x3F, 0x00, 0xF1, 0xEF, 0xB4, 0x7E, 0x34, 0xAB, 0x73, 0xED, 0x55, 0xE7, 0x78, 0x21, 0x95, 0x97, 0xED, 0x31, 0x48, 0x81, 0x88, 0x59, 0x14, 0x10, 0x1F, 0x1D, 0xC0, 0x20, 0x1F, 0xCC, 0x0A, 0xC8, 0xB2, 0x70, 0x97, 0x72, 0xB7, 0x99,
  0x23, 0x48, 0xDB, 0xB1, 0xCE, 0x47, 0xB5, 0x75, 0xB9, 0x44, 0xC9, 0x73, 0x1A, 0x5A, 0x86, 0xA3, 0x14, 0xD6, 0x32, 0x22, 0xA1, 0x1B, 0xBE, 0x50, 0x49, 0x1E, 0xBE, 0x99, 0xCD, 0x67, 0xE9, 0x4C, 0x13, 0x50, 0x88, 0xE0, 0xF1, 0x9E, 0xFE, 0xD5,
  0x3D, 0xBD, 0xBF, 0xF6, 0x94, 0x8B, 0x0D, 0xA4, 0x76, 0x30, 0x48, 0xA3, 0x73, 0xC9, 0x25, 0xD2, 0xC3, 0xBB, 0xF1, 0x91, 0x80, 0xFC, 0xA9, 0x8B, 0x0B, 0xDA, 0xDF, 0x47, 0x14, 0x86, 0x26, 0x75, 0x90, 0xA9, 0x68, 0xE4, 0x59, 0x14, 0xF1, 0xD9,
  0x94, 0x90, 0x47, 0xD0, 0xD4, 0x73, 0x26, 0xCA, 0xD7, 0x73, 0x7B, 0xED, 0x4B, 0x8F, 0xB8, 0xC4, 0xF7, 0xE6, 0x90, 0x5C, 0xA6, 0x72, 0x63, 0x6F, 0xFB, 0xEA, 0xAA, 0x30, 0xB7, 0x2A, 0x4C, 0xBB, 0xF7, 0x0F, 0xBB, 0x81, 0x44, 0x97, 0x50, 0x5B,
  0x42, 0xF2, 0x5C, 0x45, 0x2C, 0xAB, 0x80, 0x00, 0x47, 0x0A, 0x41, 0xC8, 0xE7, 0x90, 0x7B, 0x66, 0xAE, 0xE9, 0x5C, 0x35, 0xD0, 0xC3, 0x4C, 0x98, 0xCA, 0xB6, 0x4E, 0x3A, 0x66, 0x88, 0x7E, 0x4B, 0xA5, 0x52, 0x30, 0xA7, 0x3F, 0xC8, 0x55, 0xEB,
  0x5F, 0x1E, 0x5A, 0xC7, 0x64, 0xDA, 0x7C, 0x36, 0xF0, 0xDA, 0xC5, 0x2A, 0x08, 0x26, 0x99, 0x2C, 0xA3, 0xF3, 0x1E, 0x3E, 0x01, 0x24, 0xE7, 0x39, 0xE3, 0x3C, 0x75, 0x22, 0xB9, 0x8D, 0x47, 0x5F, 0x69, 0x2E, 0xCB, 0x43, 0x10, 0x31, 0xA9, 0x38,
  0x2D, 0xD5, 0x86, 0x31, 0xF8, 0x57, 0x24, 0x2A, 0x4A, 0x4D, 0xF3, 0x2B, 0x1A, 0x38, 0xA5, 0xB3, 0x3A, 0x0B, 0x32, 0x62, 0xBD, 0x8E, 0xE3, 0x61, 0xDA, 0x23, 0x66, 0x2F, 0xC9, 0x07, 0x82, 0x3F, 0x9F, 0x14, 0x4B, 0x21, 0xFB, 0x42, 0xCA, 0x06,
  0xF0, 0x64, 0xDD, 0xC0, 0xEB, 0x5C, 0xCF, 0xF6, 0xFD, 0xDE, 0x02, 0x6C, 0x8C, 0x26, 0x4F, 0x00, 0xB7, 0x43, 0xDB, 0x93, 0x5B, 0x37, 0xDE, 0x2E, 0x86, 0x5B, 0x28, 0xA2, 0xB4, 0xD2, 0xA0, 0xB7, 0x99, 0x15, 0x55, 0x9D, 0x9B, 0x76, 0xE6, 0x05,
  0xB2, 0xDC, 0x01, 0x8C, 0x86, 0xC6, 0x39, 0xE8, 0x3F, 0x01, 0xCA, 0x49, 0xE8, 0x82, 0x30, 0x83, 0x6D, 0xB6, 0x6D, 0x8B, 0x99, 0x56, 0x41, 0xB4, 0x32, 0x42, 0x72, 0x0B, 0x14, 0xCF, 0x20, 0x72, 0x3A, 0x7B, 0xF6, 0xA0, 0xA3, 0x5E, 0xC9, 0xF6,
  0x5F, 0x29, 0xD1, 0x18, 0x02, 0x64, 0x72, 0x10, 0x0C, 0x8D, 0xC3, 0x93, 0xEA, 0x07, 0x1E, 0xB5, 0xCB, 0x5C, 0x78, 0x9E, 0x56, 0x5C, 0x24, 0x2A, 0x09, 0x03, 0xEF, 0x60, 0xF3, 0x81, 0x9E, 0x00, 0xE8, 0x4E, 0x7F, 0xCF, 0x58, 0x4F, 0x88, 0x2E,
  0x3C, 0x91, 0x1F, 0x91, 0x6A, 0x00, 0x25, 0x86, 0x23, 0x00, 0x82, 0x46, 0x3A, 0xF5, 0x3F, 0x43, 0xC7, 0xE6, 0x6A, 0xB9, 0xE4, 0xD3, 0xF3, 0x0D, 0x0C, 0x02, 0xEE, 0xCB, 0x82, 0xD9, 0x19, 0xCD, 0x1B, 0xCE, 0x07, 0xAD, 0x44, 0x0D, 0x38, 0x1C,
  0x1E, 0x95, 0x22, 0x1C, 0x5C, 0x9E, 0xA0, 0x0A, 0x96, 0x0B, 0x69, 0xE6, 0x6F, 0x93, 0x00, 0x7A, 0xB3, 0x62, 0x96, 0xD6, 0x58, 0xD0, 0xE5, 0xED, 0xC4, 0x8D, 0xDB, 0x39, 0xFE, 0x55, 0x75, 0x35, 0x00, 0xAE, 0xA7, 0xEC, 0x23, 0x68, 0x3C, 0xAA,
  0xFC, 0xB9, 0xFD, 0x29, 0xA4, 0xBA, 0x81, 0x01, 0xB1, 0xB8, 0x52, 0xE4, 0x80, 0x42, 0x0C, 0xB3, 0x67, 0x81, 0xF8, 0xD5, 0x66, 0x0D, 0x8E, 0x3A, 0x56, 0x8C, 0xD3, 0x47, 0x7B, 0x34, 0x48, 0xB6, 0xAF, 0x1E, 0x25, 0x3F, 0x28, 0x7E, 0x5D, 0x49,
  0x1B, 0x54, 0x71, 0xF7, 0xBA, 0xF3, 0xDF, 0x3D, 0x38, 0xE6, 0xAB, 0xAD, 0xC0, 0xDD, 0x11, 0x40, 0xB8, 0x24, 0x1E, 0x06, 0x47, 0xF5, 0xA4, 0x0D, 0x58, 0xFF, 0xD9, 0x00
};

//namespace networking {
//	static std::string get ( const std::string& url ) {
//		std::string data;
//
//		const auto slash = url.find ( _ ( "/" ) );
//
//		std::string split_url = url;
//		std::string split_path = _ ( "/" );
//
//		if ( slash != std::string::npos ) {
//			split_url = url.substr ( N ( 0 ), slash );
//			split_path = url.substr ( slash );
//		}
//
//		std::string request = _ ( "GET /" ) + split_path + _ ( " HTTP/1.1\r\n" );
//		request += _ ( "Host: " ) + split_url + _ ( "\r\n" );
//		//request += _ ( "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36\r\n" );
//		request += _ ( "\r\n" );
//
//		WSADATA wsa_data;
//		if ( WSAStartup ( MAKEWORD ( N ( 2 ), N ( 2 ) ), &wsa_data ) != N ( 0 ) ) {
//			dbg_print ( _ ( "Connection to sesame servers failed.\n" ) );
//			return _ ( "ERROR" );
//		}
//
//		const auto sock = socket ( N ( AF_INET ), N ( SOCK_STREAM ), N ( IPPROTO_TCP ) );
//		const auto host = gethostbyname ( split_url.c_str ( ) );
//
//		SOCKADDR_IN sock_addr;
//		sock_addr.sin_port = htons ( N ( 80 ) );
//		sock_addr.sin_family = N ( AF_INET );
//		sock_addr.sin_addr.s_addr = *( ( unsigned long* ) host->h_addr );
//
//		if ( connect ( sock, ( sockaddr* ) ( &sock_addr ), sizeof ( sock_addr ) ) != N ( 0 ) ) {
//			dbg_print ( _ ( "Connection to sesame servers failed.\n" ) );
//			return _ ( "ERROR" );
//		}
//
//		send ( sock, request.c_str ( ), strlen ( request.c_str ( ) ), N ( 0 ) );
//
//		auto buffer = new char [ N ( 10000 ) ];
//		size_t data_len = N ( 0 );
//
//		while ( true ) {
//			memset ( buffer, 0, N ( 10000 ) );
//
//			data_len = recv ( sock, buffer, N ( 10000 ), N( 0 ) );
//
//			if ( data_len <= 0 )
//				break;
//
//			data += std::string ( buffer, data_len );
//		}
//
//		delete [ ] buffer;
//
//		closesocket ( sock );
//		WSACleanup ( );
//
//		/* remove header */
//		const auto delim = data.find ( _ ( "\r\n\r\n" ) );
//
//		if ( delim == std::string::npos )
//			return _ ( "ERROR" );
//
//		data = data.substr ( delim + 4 );
//
//		return data;
//	}
//}