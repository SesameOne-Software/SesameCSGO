#include "setup_bones.hpp"

#include "../features/ragebot.hpp"

bool hooks::bone_setup::allow = false;

decltype( &hooks::setup_bones ) hooks::old::setup_bones = nullptr;

bool __fastcall hooks::setup_bones( REG, matrix3x4_t* out, int max_bones, int mask, float curtime ) {
	static auto setup_bones_ret = pattern::search( _( "client.dll" ), _( "E8 ? ? ? ? 5E 5D C2 10 00 32 C0" ) ).add( 5 ).get< int* >( );

	const auto pl = reinterpret_cast< player_t* > ( uintptr_t ( ecx ) - 4 );

	auto call_original = [ & ] ( ) -> bool {
		const auto backup_flags2 = *reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xF0 );

		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xA30 ) = cs::i::globals->m_framecount;
		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xA28 ) = 0;
		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xA68 ) = cs::i::globals->m_framecount;
		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xF0 ) |= 8;

		const auto backup_frametime = cs::i::globals->m_frametime;

		cs::i::globals->m_frametime = 666.0f;

		const auto ret = old::setup_bones ( REG_OUT, out, max_bones, mask, curtime );

		cs::i::globals->m_frametime = backup_frametime;

		return ret;
	};
	//return call_original ( );
	if ( pl && pl->is_player ( ) && pl->idx ( ) > 0 && pl->idx ( ) <= cs::i::globals->m_max_clients ) {
		//return false;
		//auto ret = call_original ( );

		if ( pl == g::local ) {
			if ( out ) {
				memcpy ( out, anims::local::real_matrix.data ( ), sizeof ( matrix3x4_t ) * max_bones );
			}
		}
		else {
			if ( out ) {
				const auto idx = pl->idx ( );
				const auto misses = features::ragebot::get_misses ( idx ).bad_resolve % 3;

				memcpy ( out, anims::players::matricies [ idx ][ misses ].data ( ), sizeof ( matrix3x4_t ) * max_bones );
			}
		}

		return true;
	}

	return old::setup_bones( REG_OUT, out, max_bones, mask, curtime );
}