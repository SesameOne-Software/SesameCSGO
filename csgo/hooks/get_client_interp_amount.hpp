#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
    float __cdecl get_client_interp_amount ( );

    namespace old {
        extern decltype( &hooks::get_client_interp_amount ) get_client_interp_amount;
    }
}