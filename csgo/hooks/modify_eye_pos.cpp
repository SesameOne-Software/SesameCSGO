#include "modify_eye_pos.hpp"
#include "../globals.hpp"

#include "../animations/anims.hpp"
#include "../menu/options.hpp"

#undef min
#undef max

extern bool in_cm;
extern bool ducking;

decltype( &hooks::modify_eye_pos ) hooks::old::modify_eye_pos = nullptr;

__forceinline float spline_remap_val ( float val, float a, float b, float c, float d ) {
	if ( a == b )
		return val >= b ? d : c;

	const auto clamped_value = std::clamp ( ( val - a ) / ( b - a ), 0.0f, 1.0f );
	const auto sqr = clamped_value * clamped_value;

	return c + ( d - c ) * ( 3.0f * sqr - 2.0f * sqr * val );
}

void __fastcall hooks::modify_eye_pos( REG, vec3_t& pos ) {
	return;
	const auto anim_state = reinterpret_cast < animstate_t* > ( ecx );

	if ( !anim_state )
		return;

	const auto player = anim_state->m_entity;

	//static auto& removals = options::vars [ _ ( "visuals.other.removals" ) ].val.l;
	//
	//if ( removals [ 9 ] && pl && !in_cm )
	//	return;

	using bone_lookup_fn = int( __thiscall* )( player_t*, const char* );
	static auto lookup_bone = pattern::search( _( "client.dll" ), _( "E8 ? ? ? ? 66 89 47 04" ) ).resolve_rip().get<bone_lookup_fn>( );

	if ( !player || player != g::local || !player->bone_cache ( ) || !in_cm )
		return;
	//
	//*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x328 ) = false;
	//
	//return old::modify_eye_pos ( REG_OUT, pos );

	if ( !anim_state->m_hit_ground || anim_state->m_duck_amount == 0.0f /*|| !cs::i::ent_list->get<void*> ( player->ground_entity_handle ( ) )*/ )
		return;

	int bone_index = lookup_bone ( player, _ ( "head_0" ));

	if ( bone_index >= anims::real_matrix.size ( ) )
		return;

	auto bone_pos = anims::real_matrix [ lookup_bone ( player, _("head_0") ) ].origin ( );

	if ( bone_pos.z < pos.z )
		pos.z = std::lerp ( pos.z, bone_pos.z, spline_remap_val ( abs ( pos.z - bone_pos.z ), 4.0f, 10.0f, 0.0f, 1.0f ) );
}