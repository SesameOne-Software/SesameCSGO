#pragma once
#include <sdk.hpp>

namespace features {
	namespace prediction {
		extern float predicted_curtime;

		int shift ( const int& cur );

		void update_curtime ( );
		void run( const std::function< void( ) >& fn );
	}
}