#include "setup_bones.hpp"

#include "../features/ragebot.hpp"

bool hooks::bone_setup::allow = false;

decltype( &hooks::setup_bones ) hooks::old::setup_bones = nullptr;

bool __fastcall hooks::setup_bones( REG, matrix3x4_t* out, int max_bones, int mask, float curtime ) {
	static auto setup_bones_ret = pattern::search( _( "client.dll" ), _( "E8 ? ? ? ? 5E 5D C2 10 00 32 C0" ) ).add( 5 ).get< int* >( );

	const auto pl = reinterpret_cast< player_t* > ( uintptr_t( ecx ) - 4 );

	if ( pl && pl->is_player( ) ) {
		const auto backup_flags = *reinterpret_cast< int* >( uintptr_t( pl ) + 0xe8 );

		auto backup_abs_origin = pl->abs_origin( );
		auto backup_abs_angle = pl->abs_angles( );

		*reinterpret_cast< int* >( uintptr_t( pl ) + 0xA68 ) = 0;
		*reinterpret_cast< int* >( uintptr_t( pl ) + 0xe8 ) |= 8;

		const auto backup_frametime = cs::i::globals->m_frametime;
		const auto backup_framecount = cs::i::globals->m_framecount;

		cs::i::globals->m_framecount = INT_MAX;

		const auto ret = old::setup_bones( REG_OUT, out, max_bones, mask, curtime );

		cs::i::globals->m_framecount = backup_framecount;

		*reinterpret_cast< int* >( uintptr_t( pl ) + 0xe8 ) = backup_flags;

		return ret;
	}

	return old::setup_bones( REG_OUT, out, max_bones, mask, curtime );
}