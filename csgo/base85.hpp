#pragma once
#include <cstdint>
#include <cstring>
#include <memory>

/* thanks to ocornut's IMGUI and nothings' STB libraries */
namespace base85 {
	inline unsigned int decode_byte ( char c ) {
		return c >= '\\' ? c - 36 : c - 35;
	}

	inline size_t uncompressed_size ( const char* data ) {
		return ( ( strlen ( data ) + 4 ) / 5 ) * 4;
	}

	inline void decode ( const char* src, uint8_t* dst ) {
		while ( *src ) {
			unsigned int tmp = decode_byte ( src [ 0 ] ) + 85 * ( decode_byte ( src [ 1 ] ) + 85 * ( decode_byte ( src [ 2 ] ) + 85 * ( decode_byte ( src [ 3 ] ) + 85 * decode_byte ( src [ 4 ] ) ) ) );
			dst [ 0 ] = ( ( tmp >> 0 ) & 0xFF ); dst [ 1 ] = ( ( tmp >> 8 ) & 0xFF ); dst [ 2 ] = ( ( tmp >> 16 ) & 0xFF ); dst [ 3 ] = ( ( tmp >> 24 ) & 0xFF );
			src += 5;
			dst += 4;
		}
	}

	//inline auto decode ( const char* src ) {
	//	const auto decoded = std::make_shared<uint8_t [ ]> ( uncompressed_size (  src ) );
	//	decode ( src, decoded.get() );
	//	return decoded;
	//}
}