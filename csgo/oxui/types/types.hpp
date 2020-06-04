#ifndef OXUI_TYPES_HPP
#define OXUI_TYPES_HPP

#include <memory>
#include <string_view>
#include <algorithm>
#include <vector>
#include "../../security/xorstr.hpp"

namespace oxui {
#define	ENC_OSTR( x ) _( x )

#ifdef OXUI_WIDESTR
	using str = std::basic_string< wchar_t >;
#define OSTR( str ) ENC_OSTR( L##str )
#else
	using str = std::basic_string< char >;
#define OSTR( str ) ENC_OSTR( str )
#endif // OXUI_WIDESTR

	class pos {
	public:
		int x, y;

		pos( ) : x( 0 ), y( 0 ) { }
		pos( int x, int y ) : x( x ), y( y ) { }
		~pos( ) { }
	};

	class rect {
	public:
		int x, y, w, h;

		rect( ) : x( 0 ), y( 0 ), w( 0 ), h( 0 ) { }
		rect( int x, int y, int w, int h ) : x( x ), y( y ), w( w ), h( h ) { }
		~rect( ) { }
	};

	class color {
	public:
		int r, g, b, a;

		color( ) : r( 255 ), g( 255 ), b( 255 ), a( 255 ) { }
		color( int r, int g, int b, int a ) : r( r ), g( g ), b( b ), a( a ) { }
		~color( ) { }
	};

	class dividers {
	public:
		std::vector< int > columns_per_row;
	};

	using font = std::uintptr_t;
}

#endif // OXUI_TYPES_HPP