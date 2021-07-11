#include "modify_eye_pos.hpp"
#include "../globals.hpp"

#include "../animations/anims.hpp"
#include "../menu/options.hpp"

#undef min
#undef max

extern bool in_cm;
extern bool ducking;

decltype( &hooks::modify_eye_pos ) hooks::old::modify_eye_pos = nullptr;

void __fastcall hooks::modify_eye_pos( REG, vec3_t& pos ) {
	const auto anim_state = reinterpret_cast < animstate_t* > ( ecx );

	if ( !anim_state )
		return old::modify_eye_pos( REG_OUT , pos );

	const auto pl = anim_state->m_entity;

	static auto& removals = options::vars [ _ ( "visuals.other.removals" ) ].val.l;

	if ( removals [ 9 ] && pl && !in_cm )
		return;

	using bone_lookup_fn = int( __thiscall* )( void*, const char* );
	static auto lookup_bone = pattern::search( _( "client.dll" ), _( "55 8B EC 53 56 8B F1 57 83 BE ? ? ? ? ? 75" ) ).get<bone_lookup_fn>( );

	if ( !pl || pl != g::local || !pl->bone_cache ( ) || !in_cm )
		return old::modify_eye_pos( REG_OUT , pos );

	*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x328 ) = false;

	return old::modify_eye_pos ( REG_OUT, pos );
}