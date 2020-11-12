#pragma once
#include "../sdk/sdk.hpp"

namespace features {
	namespace prediction {
		float curtime( );
		
		int shift_tickbase ( );
		void update( int stage );
		void run( const std::function< void( ) >& fn );
	}
}