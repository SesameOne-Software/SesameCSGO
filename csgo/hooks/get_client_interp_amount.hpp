#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
    float __fastcall get_client_interp_amount( REG );

    namespace old {
        extern decltype( &hooks::get_client_interp_amount ) get_client_interp_amount;
    }
}