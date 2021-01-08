#include "override_view.hpp"

#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../menu/options.hpp"

decltype( &hooks::override_view ) hooks::old::override_view = nullptr;

void __fastcall hooks::override_view( REG, void* setup ) {
	if ( !cs::i::engine->is_in_game( ) || !cs::i::engine->is_connected( ) )
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

	static float ideal_range = 0.0f;

	{
		static auto& viewmodel_offset_x = options::vars [ _ ( "visuals.other.viewmodel_offset_x" ) ].val.f;
		static auto& viewmodel_offset_y = options::vars [ _ ( "visuals.other.viewmodel_offset_y" ) ].val.f;
		static auto& viewmodel_offset_z = options::vars [ _ ( "visuals.other.viewmodel_offset_z" ) ].val.f;

		g::cvars::viewmodel_offset_x->no_callback ( );
		g::cvars::viewmodel_offset_y->no_callback ( );
		g::cvars::viewmodel_offset_z->no_callback ( );

		g::cvars::viewmodel_offset_x->set_value ( viewmodel_offset_x );
		g::cvars::viewmodel_offset_y->set_value ( viewmodel_offset_y );
		g::cvars::viewmodel_offset_z->set_value ( viewmodel_offset_z );
	}

	if ( g::local && ( removals [ 5 ] ? ( !g::local->weapon ( ) || ( g::local->weapon ( ) && ( !g::local->scoped ( ) || g::local->weapon ( )->zoom_level ( ) <= 1 ) ) ) : !g::local->scoped ( ) ) )
		*reinterpret_cast< float* > ( uintptr_t( setup ) + 176 ) = static_cast < float > ( fov );

	auto get_ideal_dist = [ & ] ( float ideal_distance ) {
		vec3_t inverse;
		cs::i::engine->get_viewangles( inverse );

		inverse.x *= -1.0f, inverse.y += 180.0f;

		vec3_t direction = cs::angle_vec( inverse );

		ray_t ray;
		trace_t trace;
		trace_filter_t filter( g::local );

		auto start = g::local->eyes ( );
		auto end = start + direction * ideal_distance;
		
		cs::util_traceline( start, end, mask_solid & ~contents_monster, g::local, &trace );

		return ( ideal_distance * trace.m_fraction ) - 10.0f;
	};

	if ( g::local ) {
		auto target_range = 0.0f;
		auto collided_range = get_ideal_dist ( third_person_range );

		if ( third_person && utils::keybind_active ( third_person_key, third_person_key_mode ) && g::local ) {
			target_range = collided_range;
		}

		const auto val_before = ideal_range;

		if( target_range - ideal_range != 0.0f)
			ideal_range += copysign ( third_person_range * ( cs::i::globals->m_frametime * 5.0f ), target_range - ideal_range );

		/* clamp range if we pass over limit within the frame */
		if ( (val_before > target_range && ideal_range < target_range)
			|| ( val_before < target_range && ideal_range > target_range ))
			ideal_range = target_range;

		ideal_range = std::clamp ( ideal_range, 0.0f, collided_range );

		if ( ideal_range ) {
			if ( g::local->alive ( ) ) {
				vec3_t ang;
				cs::i::engine->get_viewangles ( ang );
				cs::i::input->m_camera_in_thirdperson = true;
				cs::i::input->m_camera_offset = vec3_t ( ang.x, ang.y, ideal_range );
			}
			else {
				cs::i::input->m_camera_in_thirdperson = false;
				g::local->observer_mode ( ) = 5;
			}
		}
		else {
			cs::i::input->m_camera_in_thirdperson = false;
			
			if( !g::local->alive() )
				g::local->observer_mode ( ) = 4;
		}
	}

	if ( fd_enabled && utils::keybind_active( fd_key, fd_key_mode ) && g::local && g::local->alive( ) )
		*reinterpret_cast< float* >( uintptr_t( setup ) + 192 ) = g::local->abs_origin( ).z + 64.0f;

	old::override_view( REG_OUT, setup );
}