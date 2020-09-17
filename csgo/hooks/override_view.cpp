#include "override_view.hpp"

#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../menu/options.hpp"

decltype( &hooks::override_view ) hooks::old::override_view = nullptr;

void __fastcall hooks::override_view( REG, void* setup ) {
	if ( !csgo::i::engine->is_in_game( ) || !csgo::i::engine->is_connected( ) )
		return old::override_view( REG_OUT, setup );

	static auto& removals = options::vars [ _( "visuals.other.removals" ) ].val.l;
	static auto& fov = options::vars [ _( "visuals.other.fov" ) ].val.f;

	static auto& third_person = options::vars [ _( "misc.effects.third_person" ) ].val.b;
	static auto& third_person_range = options::vars [ _( "misc.effects.third_person_range" ) ].val.f;
	static auto& third_person_key = options::vars [ _( "misc.effects.third_person_key" ) ].val.i;
	static auto& third_person_key_mode = options::vars [ _( "misc.effects.third_person_key_mode" ) ].val.i;

	static auto& fd_enabled = options::vars [ _( "antiaim.fakeduck" ) ].val.b;
	static auto& fd_mode = options::vars [ _( "antiaim.fakeduck_mode" ) ].val.i;
	static auto& fd_key = options::vars [ _( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _( "antiaim.fd_key_mode" ) ].val.i;

	if ( g::local && ( removals [ 5 ] ? true : !g::local->scoped( ) ) )
		*reinterpret_cast< float* > ( uintptr_t( setup ) + 176 ) = static_cast < float > ( fov );

	auto get_ideal_dist = [ & ] ( float ideal_distance ) {
		vec3_t inverse;
		csgo::i::engine->get_viewangles( inverse );

		inverse.x *= -1.0f, inverse.y += 180.0f;

		vec3_t direction = csgo::angle_vec( inverse );

		ray_t ray;
		trace_t trace;
		trace_filter_t filter( g::local );

		csgo::util_traceline( g::local->eyes( ), g::local->eyes( ) + ( direction * ideal_distance ), mask_playersolid, g::local, &trace );

		return ( ideal_distance * trace.m_fraction ) - 10.0f;
	};

	if ( third_person && utils::keybind_active( third_person_key, third_person_key_mode ) && g::local ) {
		if ( g::local->alive( ) ) {
			vec3_t ang;
			csgo::i::engine->get_viewangles( ang );
			csgo::i::input->m_camera_in_thirdperson = true;
			csgo::i::input->m_camera_offset = vec3_t( ang.x, ang.y, get_ideal_dist( third_person_range ) );
		}
		else {
			csgo::i::input->m_camera_in_thirdperson = false;
			g::local->observer_mode( ) = 5;
		}
	}
	else {
		csgo::i::input->m_camera_in_thirdperson = false;
	}

	if ( fd_enabled && utils::keybind_active( fd_key, fd_key_mode ) && g::local && g::local->alive( ) )
		*reinterpret_cast< float* >( uintptr_t( setup ) + 0xc0 ) = g::local->abs_origin( ).z + 64.0f;

	old::override_view( REG_OUT, setup );
}