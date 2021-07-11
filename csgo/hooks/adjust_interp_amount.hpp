#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
    float __fastcall adjust_interp_amount ( REG, int type );

    namespace old {
        extern decltype( &hooks::adjust_interp_amount ) adjust_interp_amount;
    }
}