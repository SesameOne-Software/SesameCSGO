#include "build_transformations.hpp"

#include "../animations/resolver.hpp"

decltype( &hooks::build_transformations ) hooks::old::build_transformations = nullptr;

void __fastcall hooks::build_transformations( REG, studiohdr_t* hdr, const vec3_t& pos, void* quaternion, matrix3x4a_t const& matrix, uint32_t mask, bool* computed ) {
    const auto ent = reinterpret_cast< player_t* > ( ecx );

    if ( !ent || !ent->is_player( ) )
        return old::build_transformations( REG_OUT, hdr, pos, quaternion, matrix, mask, computed );

    const auto backup_jiggle_bones = *reinterpret_cast< int* > ( uintptr_t( ent ) + 0x292C );

    *reinterpret_cast< int* > ( uintptr_t( ent ) + 0x292C ) = 0;

    old::build_transformations( REG_OUT, hdr, pos, quaternion, matrix, mask, computed );

    *reinterpret_cast< int* > ( uintptr_t( ent ) + 0x292C ) = backup_jiggle_bones;
}