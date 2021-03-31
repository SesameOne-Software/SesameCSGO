#include "setup_bones.hpp"

#include "../features/ragebot.hpp"

#undef min
#undef max

bool hooks::bone_setup::allow = false;

decltype( &hooks::setup_bones ) hooks::old::setup_bones = nullptr;

std::array<matrix3x4_t , 128> temp_mat;

bool __fastcall hooks::setup_bones( REG, matrix3x4_t* out, int max_bones, int mask, float curtime ) {
	static auto setup_bones_ret = pattern::search( _( "client.dll" ), _( "E8 ? ? ? ? 5E 5D C2 10 00 32 C0" ) ).add( 5 ).get< int* >( );

	const auto pl = reinterpret_cast< player_t* > ( uintptr_t ( ecx ) - 4 );
	
	if ( pl && pl->is_player ( ) && pl->idx ( ) > 0 && pl->idx ( ) <= cs::i::globals->m_max_clients ) {
		const auto backup_flags1 = *reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xF0 );
		const auto backup_framecount = cs::i::globals->m_framecount;

		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xF0 ) |= 8;
		cs::i::globals->m_framecount = std::numeric_limits<int>::max( );

		auto& info = anims::anim_info[ pl->idx( ) ];

		if ( !info.empty( ) && pl != g::local ) {
			auto& first = info.front( );

			pl->set_abs_angles( first.m_abs_angles[ first.m_side ] );
			pl->poses( ) = first.m_poses[ first.m_side ];
		}

		const auto ret = old::setup_bones( REG_OUT , out , max_bones , mask , curtime );

		if ( pl == g::local && g::local ) {
			anims::build_bones( pl , temp_mat.data( ) , mask , pl->abs_angles( ) , pl->abs_origin( ) , cs::i::globals->m_curtime, pl->poses( ) );
			const auto mat_out = out ? out : pl->bone_cache( );
		
			if ( mat_out )
				memcpy( mat_out , temp_mat.data( ) , ( out ? max_bones : pl->bone_count( ) ) * sizeof( matrix3x4_t ) );
		}

		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xF0 ) = backup_flags1;
		cs::i::globals->m_framecount = backup_framecount;

		return ret;
	}

	return old::setup_bones( REG_OUT, out, max_bones, mask, curtime );
}