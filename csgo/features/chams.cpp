#include "chams.hpp"
#include "../menu/menu.hpp"
#include "../hooks.hpp"
#include "../oxui/themes/purple.hpp"
#include "../globals.hpp"
#include "../animations/animations.hpp"

material_t* m_mat = nullptr,
* m_matflat = nullptr,
* m_mat_wireframe = nullptr,
* m_matflat_wireframe = nullptr,
*m_mat_glow;

float m_last_reflectivity = 0.0f;
float m_last_rimlight = 0.0f;
float m_last_luminance = 0.0f;

bool features::chams::create_materials( ) {
	FIND( double, luminance, "Visuals", "Chams", "Luminance", oxui::object_slider );
	FIND( double, reflectivity, "Visuals", "Chams", "Reflectivity", oxui::object_slider );
	static auto& chams_glow_clr = oxui::theme.main;

	auto ikv = [ ] ( void* kv, const char* name ) {
		static auto ikv_fn = pattern::search( _( "client_panorama.dll"), _( "55 8B EC 51 33 C0 C7 45" )).get< void( __thiscall* )( void*, const char* ) >( );
		ikv_fn( kv, name );
	};

	auto lfb = [ ] ( void* kv, const char* name, const char* buf ) {
		using lfb_fn = void( __thiscall* )( void*, const char*, const char*, void*, const char*, void*, void* );
		static auto lfb = pattern::search( _( "client_panorama.dll"), _( "55 8B EC 83 E4 F8 83 EC 34 53 8B 5D 0C 89" )).get< lfb_fn >( );
		lfb( kv, name, buf, nullptr, nullptr, nullptr, nullptr );
	};

	auto find_key = [ ] ( void* kv, const char* name, bool create ) {
		using find_key_fn = void* ( __thiscall* )( void*, const char*, bool );
		static auto findkey = pattern::search( _( "client_panorama.dll"), _( "55 8B EC 83 EC 1C 53 8B D9 85 DB") ).get< find_key_fn >( );
		return findkey( kv, name, create );
	};

	auto set_int = [ find_key ] ( void* kv, const char* name, int val, bool create = true ) {
		auto k = find_key( kv, name, create );

		if ( k ) {
			*( int* ) ( ( uintptr_t ) k + 0xC ) = val;
			*( char* ) ( ( uintptr_t ) k + 0x10 ) = 2;
		}
	};

	auto set_string = [ find_key ] ( void* kv, const char* name, const char* val, bool create = true ) {
		auto k = find_key( kv, name, create );

		if ( k ) {
			using setstring_fn = void( __thiscall* )( void*, const char* );
			static auto setstring = pattern::search( _( "client_panorama.dll"), _( "55 8B EC A1 ? ? ? ? 53 56 57 8B F9 8B 08 8B 01") ).get< setstring_fn >( );
			setstring( k, val );
		}
	};

	// XREF: Function DrawSpriteModel client_panorama.dll
	auto set_vec = [ ] ( void* var, float x, float y, float z ) {
		vfunc< void( __thiscall* )( void*, float, float, float ) >( var, 11 )( var, x, y, z );
	};

	auto create_mat = [ & ] ( bool xqz, bool flat, bool wireframe, bool glow ) {
		static auto created = 0;
		std::string type = flat ? _( "UnlitGeneric" ): _( "VertexLitGeneric");

		if ( glow )
			type = _( "VertexLitGeneric");

		// allocating space for key values
		auto kv = malloc( 36 );

		ikv( kv, type.c_str( ) );

		if ( glow ) {
			auto found = false;

			set_string( kv, _( "$envmap"), _( "models/effects/cube_white") );
			set_int( kv, _("$additive"), 1 );
			set_int( kv, _( "$envmapfresnel"), 1 );

			auto glow_clr_str_r = std::to_string( chams_glow_clr.r / 255.0f );
			auto glow_clr_str_g = std::to_string( chams_glow_clr.g / 255.0f );
			auto glow_clr_str_b = std::to_string( chams_glow_clr.b / 255.0f );
			auto glow_clr = _( "[")
				+ glow_clr_str_r + _(" ")
				+ glow_clr_str_g + _(" ")
				+ glow_clr_str_b + _("]");

			set_string( kv, _( "$envmapint"), glow_clr.data( ) );
			set_string( kv, _( "$envmapfresnelminmaxexp"), _( "[ 0 1 2 ]") );
			set_string( kv, _( "$alpha"), _( "1") );
		}
		else {
			auto reflectivity_str = std::to_string( reflectivity );
			auto luminance_str = std::to_string( luminance );

			auto sreflectivity = _( "[")
				+ reflectivity_str + _(" ")
				+ reflectivity_str + _(" ")
				+ reflectivity_str + _("]");

			auto sluminance = _( "[")
				+ luminance_str + _(" ")
				+ luminance_str + _(" ")
				+ luminance_str + _("]");

			// set_string( kv, "$basetexture", "vgui/white_additive" );
			set_string( kv, _( "$basetexture"), _( "vgui/white_additive"));
			set_string( kv, _( "$envmaptint"), sreflectivity.data( ) );
			set_string( kv, _( "$envmap"), _( "env_cubemap") );

			set_int( kv, _("$phong"), 1 );
			set_int( kv, _("$phongexponent"), 15 );
			set_int( kv, _("$normalmapalphaenvmask"), 1 );
			set_string( kv, _( "$phongboost"), sluminance.data( ) );
			//set_string( kv, "$phongfresnelranges", "[.5 .5 1]" );
			set_int( kv, _( "$BasemapAlphaPhongMask"), 1 );

			set_int( kv, _("$model"), 1 );
			set_int( kv, _("$flat"), 1 );
			set_int( kv, _("$selfillum"), 1 );
			set_int( kv, _("$halflambert"), 1 );
			set_int( kv, _("$ignorez"), 1 );
		}

		auto matname = _( "mat_") + std::to_string( created );

		// lfb( kv, matname.c_str( ), matdata.c_str( ) );

		// creating material
		auto mat = csgo::i::mat_sys->createmat( matname.c_str( ), kv );

		// incrementing reference count
		mat->increment_reference_count( );

		// we want a different material allocated every time
		created++;

		return mat;
	};

	m_mat = create_mat( false, false, false, false );
	m_matflat = create_mat( false, true, false, false );
	m_mat_wireframe = create_mat( false, false, true, false );
	m_matflat_wireframe = create_mat( false, true, true, false );
	m_mat_glow = create_mat( false, true, true, true );

	return true;
}

void features::chams::update_mats( ) {
	FIND( double, luminance, "Visuals", "Chams", "Luminance", oxui::object_slider );
	FIND( double, reflectivity, "Visuals", "Chams", "Reflectivity", oxui::object_slider );
	static auto& chams_glow_clr = oxui::theme.main;

	// XREF: Function DrawSpriteModel client_panorama.dll
	auto set_vec = [ ] ( void* var, float x, float y, float z ) {
		vfunc< void( __thiscall* )( void*, float, float, float ) >( var, 11 )( var, x, y, z );
	};

	auto set_int = [ ] ( void* var, int val ) {
		if ( var ) {
			*( int* ) ( ( uintptr_t ) var + 0xC ) = val;
			*( char* ) ( ( uintptr_t ) var + 0x10 ) = 2;
		}
	};

	auto found = false;

	if ( m_mat
		&& m_matflat
		&& m_matflat_wireframe
		&& m_mat_wireframe
		&& m_mat_glow
		&& ( reflectivity != m_last_reflectivity
			|| luminance != m_last_luminance ) ) {
		m_last_reflectivity = reflectivity;
		m_last_luminance = luminance;

		auto reflectivity_str = std::to_string( reflectivity );
		auto luminance_str = std::to_string( luminance );

		auto envmaptint = m_mat->find_var( _( "$envmaptint"), &found );
		auto envmaptint1 = m_matflat->find_var( _( "$envmaptint"), &found );
		auto envmaptint2 = m_matflat_wireframe->find_var( _( "$envmaptint"), &found );
		auto envmaptint3 = m_mat_wireframe->find_var( _( "$envmaptint"), &found );

		set_vec( envmaptint, reflectivity, reflectivity, reflectivity );
		set_vec( envmaptint1, reflectivity, reflectivity, reflectivity );
		set_vec( envmaptint2, reflectivity, reflectivity, reflectivity );
		set_vec( envmaptint3, reflectivity, reflectivity, reflectivity );

		auto phongboost = m_mat->find_var( _( "$phongboost"), &found );
		auto phongboost1 = m_matflat->find_var( _( "$phongboost"), &found );
		auto phongboost2 = m_matflat_wireframe->find_var( _( "$phongboost"), &found );
		auto phongboost3 = m_mat_wireframe->find_var( _( "$phongboost"), &found );

		set_vec( phongboost, luminance, luminance, luminance );
		set_vec( phongboost1, luminance, luminance, luminance );
		set_vec( phongboost2, luminance, luminance, luminance );
		set_vec( phongboost3, luminance, luminance, luminance );
	}

	if ( m_mat_glow ) {
		auto envmap = m_mat_glow->find_var( _( "$envmapint"), &found );
		set_vec( envmap, chams_glow_clr.r / 255.0f, chams_glow_clr.g / 255.0f, chams_glow_clr.b / 255.0f );
	}
}

void features::chams::drawmodelexecute( void* ctx, void* state, const mdlrender_info_t& info, matrix3x4_t* bone_to_world ) {
	FIND( bool, glow, "Visuals", "Glow", "Glow", oxui::object_checkbox );
	FIND( bool, rim, "Visuals", "Glow", "Rim", oxui::object_checkbox );
	FIND( bool, chams, "Visuals", "Chams", "Chams", oxui::object_checkbox );
	FIND( bool, team, "Visuals", "Targets", "Team", oxui::object_checkbox );
	FIND( bool, enemy, "Visuals", "Targets", "Enemy", oxui::object_checkbox );
	FIND( bool, local, "Visuals", "Targets", "Local", oxui::object_checkbox );
	FIND( bool, weapon, "Visuals", "Targets", "Weapon", oxui::object_checkbox );
	FIND( bool, flat, "Visuals", "Chams", "Flat", oxui::object_checkbox );
	FIND( bool, xqz, "Visuals", "Chams", "XQZ", oxui::object_checkbox );
	static auto& chams_clr = oxui::theme.main;
	static auto& chams_clr_xqz = oxui::theme.main;

	update_mats( );

	if ( !g::local || !info.m_model || !chams || !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) ) {
		auto e = csgo::i::ent_list->get< player_t* >( info.m_entity_index );

		if ( e ) {
			csgo::i::render_view->set_color( 255, 255, 255 );
			csgo::i::render_view->set_alpha( 255 );
			hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
		}

		return;
	}

	auto mdl_name = csgo::i::mdl_info->mdl_name( info.m_model );

	auto e = csgo::i::ent_list->get< player_t* >( info.m_entity_index );

	static auto generated_mats = create_materials( );

	if ( !generated_mats ) {
		csgo::i::render_view->set_alpha( 255 );
		csgo::i::render_view->set_color( 255, 255, 255 );
		hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
		return;
	}

	auto is_weapon = std::strstr( mdl_name, _( "arms") ) || std::strstr( mdl_name, _( "v_models") );

	if ( is_weapon || e ) {
		auto is_player = e->valid( ) && e->is_player( );

		if ( ( is_player && enemy && e->team( ) != g::local->team( ) )
			|| ( is_player && team && e->team( ) == g::local->team( ) && e != g::local )
			|| ( is_player && local && e == g::local )
			|| ( weapon && is_weapon ) ) {
			/* fake chams */
			/*
			if ( e == g::local ) {
				const auto backup_matrix = animations::fake::matrix( );

				for ( auto& i : animations::fake::matrix( ) ) {
					i [ 0 ][ 3 ] += info.m_origin.x;
					i [ 1 ][ 3 ] += info.m_origin.y;
					i [ 2 ][ 3 ] += info.m_origin.z;
				}

				csgo::i::render_view->set_alpha( 100 );
				csgo::i::render_view->set_color( 120, 20, 255 );
				auto mat = flat ? m_matflat : m_mat;
				mat->set_material_var_flag( 0x8000, false );
				mat->set_material_var_flag( 0x1000, flat );
				csgo::i::mdl_render->force_mat( mat );
				hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, ( matrix3x4_t* ) &animations::fake::matrix( ) );
				csgo::i::mdl_render->force_mat( nullptr );

				animations::fake::matrix( ) = backup_matrix;
			}
			*/

			/* occluded */
			if ( e == g::local ? false : xqz ) {
				csgo::i::render_view->set_alpha( 100 );
				csgo::i::render_view->set_color( chams_clr_xqz.r, chams_clr_xqz.g, chams_clr_xqz.b );
				auto mat = flat ? m_matflat : m_mat;
				// mat->set_material_var_flag( 268435456, false );
				mat->set_material_var_flag( 0x8000, true );
				mat->set_material_var_flag( 0x1000, flat );
				csgo::i::mdl_render->force_mat( mat );
				hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
				csgo::i::mdl_render->force_mat( nullptr );
			}

			/* visible */
			if ( local && e == g::local )
				csgo::i::render_view->set_alpha( g::local->scoped( ) ? 50 : 100 );
			else
				csgo::i::render_view->set_alpha( 100 );

			if ( ( local && e == g::local ) ? !g::local->scoped( ) : true ) {
				csgo::i::render_view->set_color( chams_clr.r, chams_clr.g, chams_clr.b );
				auto mat = flat ? m_matflat : m_mat;
				// mat->set_material_var_flag( 268435456, false );
				mat->set_material_var_flag( 0x8000, false );
				mat->set_material_var_flag( 0x1000, flat );
				csgo::i::mdl_render->force_mat( mat );
				hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
				csgo::i::mdl_render->force_mat( nullptr );
			}

			/* glow overlay */
			if ( glow && rim ) {
				csgo::i::render_view->set_alpha( 100 );
				//csgo::i::render_view->set_color( oxui::vars::items [ "chams_glow_clr" ].val.c.r, oxui::vars::items [ "chams_glow_clr" ].val.c.g, oxui::vars::items [ "chams_glow_clr" ].val.c.b );
				// mat->set_material_var_flag( 268435456, false );
				m_mat_glow->set_material_var_flag( 0x8000, false );
				m_mat_glow->set_material_var_flag( 0x1000, flat );
				csgo::i::mdl_render->force_mat( m_mat_glow );
				hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
				csgo::i::mdl_render->force_mat( nullptr );
			}
		}
		else {
			if ( e == g::local ) {
				csgo::i::render_view->set_alpha( 255 );
				csgo::i::render_view->set_color( 255, 255, 255 );
				hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
			}
			else {
				csgo::i::render_view->set_alpha( 255 );
				csgo::i::render_view->set_color( 255, 255, 255 );
				hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
			}
		}
	}
	else {
		csgo::i::render_view->set_alpha( 255 );
		csgo::i::render_view->set_color( 255, 255, 255 );
		hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
	}

	csgo::i::render_view->set_alpha( 255 );
	csgo::i::render_view->set_color( 255, 255, 255 );
}