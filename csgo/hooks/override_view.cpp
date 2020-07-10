#include "override_view.hpp"

#include "../menu/menu.hpp"
#include "../globals.hpp"

decltype( &hooks::override_view ) hooks::old::override_view = nullptr;

void __fastcall hooks::override_view ( REG, void* setup ) {
	OPTION ( bool, thirdperson, "Sesame->E->Effects->Main->Third Person", oxui::object_checkbox );
	OPTION ( double, thirdperson_range, "Sesame->E->Effects->Main->Third Person Range", oxui::object_slider );
	KEYBIND ( thirdperson_key, "Sesame->E->Effects->Main->Third Person Key" );
	KEYBIND ( fd_key, "Sesame->B->Other->Other->Fakeduck Key" );
	OPTION ( bool, no_zoom, "Sesame->C->Other->Removals->No Zoom", oxui::object_checkbox );
	OPTION ( double, custom_fov, "Sesame->C->Other->Removals->Custom FOV", oxui::object_slider );

	if ( g::local && ( no_zoom ? true : !g::local->scoped ( ) ) )
		*reinterpret_cast< float* > ( uintptr_t ( setup ) + 176 ) = static_cast < float > ( custom_fov );

	auto get_ideal_dist = [ & ] ( float ideal_distance ) {
		vec3_t inverse;
		csgo::i::engine->get_viewangles ( inverse );

		inverse.x *= -1.0f, inverse.y += 180.0f;

		vec3_t direction = csgo::angle_vec ( inverse );

		ray_t ray;
		trace_t trace;
		trace_filter_t filter ( g::local );

		csgo::util_traceline ( g::local->eyes ( ), g::local->eyes ( ) + ( direction * ideal_distance ), 0x600400B, g::local, &trace );

		return ( ideal_distance * trace.m_fraction ) - 10.0f;
	};

	if ( thirdperson && thirdperson_key && g::local ) {
		if ( g::local->alive ( ) ) {
			vec3_t ang;
			csgo::i::engine->get_viewangles ( ang );
			csgo::i::input->m_camera_in_thirdperson = true;
			csgo::i::input->m_camera_offset = vec3_t ( ang.x, ang.y, get_ideal_dist ( thirdperson_range ) );
		}
		else {
			csgo::i::input->m_camera_in_thirdperson = false;
			g::local->observer_mode ( ) = 5;
		}
	}
	else {
		csgo::i::input->m_camera_in_thirdperson = false;
	}

	if ( fd_key && g::local && g::local->alive ( ) )
		*reinterpret_cast< float* >( uintptr_t ( setup ) + 0xc0 ) = g::local->abs_origin ( ).z + 64.0f;

	old::override_view ( REG_OUT, setup );
}