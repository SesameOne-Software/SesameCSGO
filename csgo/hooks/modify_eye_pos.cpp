#include "modify_eye_pos.hpp"
#include "../globals.hpp"

#include "../animations/anims.hpp"

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

	using bone_lookup_fn = int( __thiscall* )( void*, const char* );
	static auto lookup_bone = pattern::search( _( "client.dll" ), _( "55 8B EC 53 56 8B F1 57 83 BE ? ? ? ? ? 75" ) ).get<bone_lookup_fn>( );

	if ( !pl || pl != g::local || !pl->bone_cache ( ) )
		return old::modify_eye_pos( REG_OUT , pos );

	if ( !in_cm )
		return;

	if ( anim_state->m_hit_ground || anim_state->m_duck_amount || !cs::i::ent_list->get_by_handle< entity_t* > ( pl->ground_entity_handle ( ) ) ) {
		auto bone_pos = anims::real_matrix[ lookup_bone ( pl, "head_0" ) ].origin();
	
		bone_pos.z += 1.7f;
	
		if ( pos.z > bone_pos.z ) {
			float some_factor = 0.0f;
			float delta = pos.z - bone_pos.z;
			float some_offset = ( delta - 4.0f ) / 6.0f;
	
			if ( some_offset >= 0.0f )
				some_factor = std::min ( some_offset, 1.0f );
	
			pos.z += ( ( bone_pos.z - pos.z ) * ( ( ( some_factor * some_factor ) * 3.0f ) - ( ( ( some_factor * some_factor ) * 2.0f ) * some_factor ) ) );
		}
	}
}