#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
    void __fastcall build_transformations( REG, studiohdr_t* hdr, const vec3_t& pos, void* quaternion, matrix3x4a_t const& matrix, uint32_t mask, bool* computed );

    namespace old {
        extern decltype( &hooks::build_transformations ) build_transformations;
    }
}