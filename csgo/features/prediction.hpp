#pragma once
#include "../sdk/sdk.hpp"

namespace features {
	namespace prediction {
		float curtime( );
		
		int shift ( const int& cur );
		void update( int stage );
		void run( const std::function< void( ) >& fn );
	}
}