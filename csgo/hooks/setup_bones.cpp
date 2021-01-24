#include "setup_bones.hpp"

#include "../features/ragebot.hpp"

#undef min
#undef max

bool hooks::bone_setup::allow = false;

decltype( &hooks::setup_bones ) hooks::old::setup_bones = nullptr;

bool __fastcall hooks::setup_bones( REG, matrix3x4_t* out, int max_bones, int mask, float curtime ) {
	static auto setup_bones_ret = pattern::search( _( "client.dll" ), _( "E8 ? ? ? ? 5E 5D C2 10 00 32 C0" ) ).add( 5 ).get< int* >( );

	const auto pl = reinterpret_cast< player_t* > ( uintptr_t ( ecx ) - 4 );
	
	if ( pl && pl->is_player ( ) && pl->idx ( ) > 0 && pl->idx ( ) <= cs::i::globals->m_max_clients ) {
		const auto backup_flags2 = *reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xF0 );

		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xA30 ) = 0;
		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xA28 ) = 0;
		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xA68 ) = 0;
		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xF0 ) |= 8;

		const auto backup_frametime = cs::i::globals->m_frametime;
		const auto backup_framecount = cs::i::globals->m_framecount;

		cs::i::globals->m_frametime = 666.0f;
		cs::i::globals->m_framecount = std::numeric_limits<int>::max ( );

		//pl->readable_bones ( ) = 0;
		//pl->writeable_bones ( ) = 0;
		//pl->inval_bone_cache ( );

		const auto ret = old::setup_bones ( REG_OUT, out, max_bones, mask, curtime );

		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( pl ) + 0xF0 ) &= ~8;

		cs::i::globals->m_frametime = backup_frametime;
		cs::i::globals->m_framecount = backup_framecount;

		if ( pl == g::local ) {
			return ret;
		}
		else {
			//if(!out && !pl->bone_cache ( ) )
			//	return call_original ( );

			memcpy ( out ? out : pl->bone_cache ( ), anims::aim_matricies [ pl->idx ( ) ][ features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 3 ].data ( ), sizeof ( matrix3x4_t ) * ( out ? max_bones : pl->bone_count ( ) ) );

			return ret;
		}
	
		return ret;
	}

	return old::setup_bones( REG_OUT, out, max_bones, mask, curtime );
}