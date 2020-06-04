#pragma once
#include <sdk.hpp>

namespace features {
	namespace prediction {
		extern float predicted_curtime;

		void update_curtime( );
		void run( const std::function< void( ) >& fn );
	}
}