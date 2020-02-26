#pragma once
#include <sdk.hpp>

namespace features {
	namespace prediction {
		void run( const std::function< void( ) >& fn );
	}
}