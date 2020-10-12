#include "setup_bones.hpp"

#include "../features/ragebot.hpp"

bool hooks::bone_setup::allow = false;

decltype( &hooks::setup_bones ) hooks::old::setup_bones = nullptr;

bool __fastcall hooks::setup_bones( REG, matrix3x4_t* out, int max_bones, int mask, float curtime ) {
	static auto setup_bones_ret = pattern::search( _( "client.dll" ), _( "E8 ? ? ? ? 5E 5D C2 10 00 32 C0" ) ).add( 5 ).get< int* >( );

	const auto pl = reinterpret_cast< player_t* > ( uintptr_t( ecx ) - 4 );

	if ( pl && pl->is_player( ) /*&& pl->bone_cache( )*/ ) {
		/*if ( !hooks::bone_setup::allow && pl != g::local ) {
			memcpy( out ? out : pl->bone_cache( ), &animations::data::bones [ pl->idx( ) ], sizeof( matrix3x4_t ) * pl->bone_count( ) );
			return true;
		}
		else {*/
		const auto backup_mask_1 = *reinterpret_cast< int* >( uintptr_t( pl ) + 0x269C );
		const auto backup_mask_2 = *reinterpret_cast< int* >( uintptr_t( pl ) + 0x26B0 );
		const auto backup_flags = *reinterpret_cast< int* >( uintptr_t( pl ) + 0xe8 );
		const auto backup_effects = *reinterpret_cast< int* >( uintptr_t( pl ) + 0xf0 );
		const auto backup_use_pred_time = *reinterpret_cast< int* >( uintptr_t( pl ) + 0x2ee );

		auto backup_abs_origin = pl->abs_origin( );
		auto backup_abs_angle = pl->abs_angles( );

		*reinterpret_cast< int* >( uintptr_t( pl ) + 0xA68 ) = 0;
		//*reinterpret_cast< int* >( uintptr_t( pl ) + 0x26AC ) = 0;
		*reinterpret_cast< int* >( uintptr_t( pl ) + 0xe8 ) |= 8;
		//*reinterpret_cast< int* >( uintptr_t( pl ) + 0xf0 ) |= 8;

		//pl->inval_bone_cache( );

		//const auto backup_curtime = csgo::i::globals->m_curtime;
		const auto backup_frametime = csgo::i::globals->m_frametime;
		const auto backup_framecount = csgo::i::globals->m_framecount;

		csgo::i::globals->m_framecount = INT_MAX;
		//csgo::i::globals->m_curtime = csgo::i::globals->m_curtime;
		//csgo::i::globals->m_frametime = 666.0f;

		const auto ret = old::setup_bones( REG_OUT, out, max_bones, mask, curtime );

		csgo::i::globals->m_framecount = backup_framecount;
		//csgo::i::globals->m_curtime = backup_curtime;
		//csgo::i::globals->m_frametime = backup_frametime;

		*reinterpret_cast< int* >( uintptr_t( pl ) + 0xe8 ) = backup_flags;
		//*reinterpret_cast< int* >( uintptr_t( pl ) + 0xf0 ) = backup_effects;

		return ret;
		/*}*/
	}

	return old::setup_bones( REG_OUT, out, max_bones, mask, curtime );
}