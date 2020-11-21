#include "modify_eye_pos.hpp"
#include "../globals.hpp"

#include "../animations/animation_system.hpp"

extern bool in_cm;
extern bool ducking;

decltype( &hooks::modify_eye_pos ) hooks::old::modify_eye_pos = nullptr;

void __fastcall hooks::modify_eye_pos( REG, vec3_t& pos ) {
	const auto state = reinterpret_cast < animstate_t* > ( ecx );
	const auto pl = state->m_entity;

	using bone_lookup_fn = int( __thiscall* )( void*, const char* );
	static auto lookup_bone = pattern::search( _( "client.dll" ), _( "55 8B EC 53 56 8B F1 57 83 BE ? ? ? ? ? 75" ) ).get<bone_lookup_fn>( );

	if ( !g::local->valid( ) || !pl->valid( ) || pl != g::local ) {
		old::modify_eye_pos( REG_OUT, pos );
		return;
	}

	if ( !in_cm && !ducking ) {
		old::modify_eye_pos( REG_OUT, pos );
		return;
	}

	if ( !state->m_hit_ground && state->m_duck_amount == 0.0f && cs::i::ent_list->get_by_handle< entity_t* >( pl->ground_entity_handle( ) ) )
		return;

	auto bone_idx = lookup_bone( pl, _( "head_0" ) );
	vec3_t bone_pos = anims::aim_matrix [ bone_idx ].origin( );
	bone_pos.z += 1.7f;

	if ( bone_pos.z < pos.z ) {
		auto lerp = std::clamp< float >( ( std::fabsf( pos.z - bone_pos.z ) - 4.0f ) / 6.0f, 0.0f, 1.0f );
		pos.z += ( ( bone_pos.z - pos.z ) * ( ( ( lerp * lerp ) * 3.0f ) - ( ( ( lerp * lerp ) * 2.0f ) * lerp ) ) );
	}
}