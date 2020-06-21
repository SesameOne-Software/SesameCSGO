#include "chams.hpp"
#include "../menu/menu.hpp"
#include "../hooks.hpp"
#include "../oxui/themes/purple.hpp"
#include "../globals.hpp"
#include "../animations/animations.hpp"
#include "lagcomp.hpp"
#include "glow.hpp"
#include "../animations/resolver.hpp"

material_t* m_mat = nullptr,
* m_matflat = nullptr,
* m_mat_wireframe = nullptr,
* m_matflat_wireframe = nullptr,
* m_mat_glow;

animations::resolver::hit_matrix_rec_t cur_hit_matrix_rec;
bool features::chams::in_model = false;

bool features::chams::create_materials( ) {
	OPTION ( oxui::color, phong_clr, "Sesame->C->Chams->Colors->Phong", oxui::object_colorpicker );
	OPTION ( oxui::color, chams_glow_clr, "Sesame->C->Glow->Colors->Rimlight", oxui::object_colorpicker );
	OPTION ( oxui::color, reflected_clr, "Sesame->C->Chams->Colors->Reflected", oxui::object_colorpicker );

	auto ikv = [ ] ( void* kv, const char* name ) {
		static auto ikv_fn = pattern::search( _( "client.dll" ), _( "55 8B EC 51 33 C0 C7 45" ) ).get< void( __thiscall* )( void*, const char* ) >( );
		ikv_fn( kv, name );
	};

	auto lfb = [ ] ( void* kv, const char* name, const char* buf ) {
		using lfb_fn = void( __thiscall* )( void*, const char*, const char*, void*, const char*, void*, void* );
		static auto lfb = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 83 EC 34 53 8B 5D 0C 89" ) ).get< lfb_fn >( );
		lfb( kv, name, buf, nullptr, nullptr, nullptr, nullptr );
	};

	auto find_key = [ ] ( void* kv, const char* name, bool create ) {
		using find_key_fn = void* ( __thiscall* )( void*, const char*, bool );
		static auto findkey = pattern::search( _( "client.dll" ), _( "55 8B EC 83 EC 1C 53 8B D9 85 DB" ) ).get< find_key_fn >( );
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
			static auto setstring = pattern::search( _( "client.dll" ), _( "55 8B EC A1 ? ? ? ? 53 56 57 8B F9 8B 08 8B 01" ) ).get< setstring_fn >( );
			setstring( k, val );
		}
	};

	// XREF: Function DrawSpriteModel client.dll
	auto set_vec = [ ] ( void* var, float x, float y, float z ) {
		vfunc< void( __thiscall* )( void*, float, float, float ) >( var, 11 )( var, x, y, z );
	};

	auto create_mat = [ & ] ( bool xqz, bool flat, bool wireframe, bool glow ) {
		static auto created = 0;
		std::string type = flat ? _( "UnlitGeneric" ) : _( "VertexLitGeneric" );

		if ( glow )
			type = _( "VertexLitGeneric" );

		// allocating space for key values
		auto kv = malloc( 36 );

		ikv( kv, type.c_str( ) );

		if ( glow ) {
			auto found = false;

			set_string( kv, _( "$envmap" ), _( "models/effects/cube_white" ) );
			set_int( kv, _( "$additive" ), 1 );
			set_int( kv, _( "$envmapfresnel" ), 1 );

			auto chams_glow_coeff = static_cast< float >( chams_glow_clr.a ) / 255.0f;
			auto glow_clr_str_r = std::to_string( static_cast< float >( chams_glow_clr.r ) / 255.0f * chams_glow_coeff );
			auto glow_clr_str_g = std::to_string( static_cast< float >( chams_glow_clr.g ) / 255.0f * chams_glow_coeff );
			auto glow_clr_str_b = std::to_string( static_cast< float >( chams_glow_clr.b ) / 255.0f * chams_glow_coeff );
			auto glow_clr = _( "[" )
				+ glow_clr_str_r + _( " " )
				+ glow_clr_str_g + _( " " )
				+ glow_clr_str_b + _( "]" );

			set_string( kv, _( "$envmaptint" ), glow_clr.data( ) );
			set_string( kv, _( "$envmapfresnelminmaxexp" ), _( "[ 0 1 2 ]" ) );
			set_string( kv, _( "$alpha" ), _( "1" ) );
		}
		else {
			auto envmaptint_coeff = static_cast< float >( reflected_clr.a ) / 255.0f;
			auto reflected_clr_r = std::to_string ( static_cast< float >( reflected_clr.r ) / 255.0f * envmaptint_coeff );
			auto reflected_clr_g = std::to_string ( static_cast< float >( reflected_clr.g ) / 255.0f * envmaptint_coeff );
			auto reflected_clr_b = std::to_string ( static_cast< float >( reflected_clr.b ) / 255.0f * envmaptint_coeff );

			auto phong_coeff = static_cast< float >( phong_clr.a ) / 255.0f;
			auto phong_clr_r = std::to_string ( static_cast< float >( phong_clr.r ) / 255.0f * phong_coeff );
			auto phong_clr_g = std::to_string ( static_cast< float >( phong_clr.g ) / 255.0f * phong_coeff );
			auto phong_clr_b = std::to_string ( static_cast< float >( phong_clr.b ) / 255.0f * phong_coeff );

			auto sreflectivity = _( "[" )
				+ reflected_clr_r + _( " " )
				+ reflected_clr_g + _( " " )
				+ reflected_clr_b + _( "]" );

			auto sluminance = _( "[" )
				+ phong_clr_r + _( " " )
				+ phong_clr_g + _( " " )
				+ phong_clr_b + _( "]" );

			// set_string( kv, "$basetexture", "vgui/white_additive" );
			set_string( kv, _( "$basetexture" ), _( "vgui/white_additive" ) );
			set_string( kv, _( "$envmaptint" ), sreflectivity.data( ) );
			set_string( kv, _( "$envmap" ), _( "env_cubemap" ) );

			set_int( kv, _( "$phong" ), 1 );
			set_int( kv, _( "$phongexponent" ), 15 );
			set_int( kv, _( "$normalmapalphaenvmask" ), 1 );
			set_string( kv, _( "$phongboost" ), sluminance.data( ) );
			//set_string( kv, "$phongfresnelranges", "[.5 .5 1]" );
			set_int( kv, _( "$BasemapAlphaPhongMask" ), 1 );

			set_int( kv, _( "$model" ), 1 );
			set_int( kv, _( "$flat" ), 1 );
			set_int( kv, _( "$selfillum" ), 1 );
			set_int( kv, _( "$halflambert" ), 1 );
			set_int( kv, _( "$ignorez" ), 1 );
		}

		auto matname = _( "mat_" ) + std::to_string( created );

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
	OPTION ( oxui::color, phong_clr, "Sesame->C->Chams->Colors->Phong", oxui::object_colorpicker );
	OPTION ( oxui::color, chams_glow_clr, "Sesame->C->Glow->Colors->Rimlight", oxui::object_colorpicker );
	OPTION ( oxui::color, reflected_clr, "Sesame->C->Chams->Colors->Reflected", oxui::object_colorpicker );

	// XREF: Function DrawSpriteModel client.dll
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
		&& m_mat_glow ) {
		auto envmaptint = m_mat->find_var( _( "$envmaptint" ), &found );
		auto envmaptint1 = m_matflat->find_var( _( "$envmaptint" ), &found );
		auto envmaptint2 = m_matflat_wireframe->find_var( _( "$envmaptint" ), &found );
		auto envmaptint3 = m_mat_wireframe->find_var( _( "$envmaptint" ), &found );
		auto envmaptint_coeff = static_cast< float >( reflected_clr.a ) / 255.0f;
		
		set_vec( envmaptint, static_cast< float >( reflected_clr.r ) / 255.0f * envmaptint_coeff, static_cast< float >( reflected_clr.g ) / 255.0f * envmaptint_coeff, static_cast< float >( reflected_clr.b ) / 255.0f * envmaptint_coeff );
		set_vec( envmaptint1, static_cast< float >( reflected_clr.r ) / 255.0f * envmaptint_coeff, static_cast< float >( reflected_clr.g ) / 255.0f * envmaptint_coeff, static_cast< float >( reflected_clr.b ) / 255.0f * envmaptint_coeff );
		set_vec( envmaptint2, static_cast< float >( reflected_clr.r ) / 255.0f * envmaptint_coeff, static_cast< float >( reflected_clr.g ) / 255.0f * envmaptint_coeff, static_cast< float >( reflected_clr.b ) / 255.0f * envmaptint_coeff );
		set_vec( envmaptint3, static_cast< float >( reflected_clr.r ) / 255.0f * envmaptint_coeff, static_cast< float >( reflected_clr.g ) / 255.0f * envmaptint_coeff, static_cast< float >( reflected_clr.b ) / 255.0f * envmaptint_coeff );

		auto phongboost = m_mat->find_var( _( "$phongboost" ), &found );
		auto phongboost1 = m_matflat->find_var( _( "$phongboost" ), &found );
		auto phongboost2 = m_matflat_wireframe->find_var( _( "$phongboost" ), &found );
		auto phongboost3 = m_mat_wireframe->find_var( _( "$phongboost" ), &found );
		auto phong_coeff = static_cast< float >( phong_clr.a ) / 255.0f;

		set_vec( phongboost, static_cast< float >( phong_clr.r ) / 255.0f * phong_coeff, static_cast< float >( phong_clr.g ) / 255.0f * phong_coeff, static_cast< float >( phong_clr.b ) / 255.0f * phong_coeff );
		set_vec( phongboost1, static_cast< float >( phong_clr.r ) / 255.0f * phong_coeff, static_cast< float >( phong_clr.g ) / 255.0f * phong_coeff, static_cast< float >( phong_clr.b ) / 255.0f * phong_coeff );
		set_vec( phongboost2, static_cast< float >( phong_clr.r ) / 255.0f * phong_coeff, static_cast< float >( phong_clr.g ) / 255.0f * phong_coeff, static_cast< float >( phong_clr.b ) / 255.0f * phong_coeff );
		set_vec( phongboost3, static_cast< float >( phong_clr.r ) / 255.0f * phong_coeff, static_cast< float >( phong_clr.g ) / 255.0f * phong_coeff, static_cast< float >( phong_clr.b ) / 255.0f * phong_coeff );
	}

	if ( m_mat_glow ) {
		auto envmap = m_mat_glow->find_var( _( "$envmaptint" ), &found );
		auto chams_glow_coeff = static_cast< float >( chams_glow_clr.a ) / 255.0f;
		set_vec( envmap, static_cast< float >( chams_glow_clr.r ) / 255.0f * chams_glow_coeff, static_cast< float >( chams_glow_clr.g ) / 255.0f * chams_glow_coeff, static_cast< float >( chams_glow_clr.b ) / 255.0f * chams_glow_coeff );
	}
}

std::array < mdlrender_info_t, 65 > mdl_render_info;

void features::chams::drawmodelexecute( void* ctx, void* state, const mdlrender_info_t& info, matrix3x4_t* bone_to_world ) {
	OPTION( bool, glow, "Sesame->C->Glow->Main->Glow", oxui::object_checkbox );
	OPTION( bool, rim, "Sesame->C->Glow->Main->Rimlight", oxui::object_checkbox );

	OPTION( bool, chams, "Sesame->C->Chams->Main->Chams", oxui::object_checkbox );
	OPTION( bool, team, "Sesame->C->Chams->Targets->Team", oxui::object_checkbox );
	OPTION( bool, enemy, "Sesame->C->Chams->Targets->Enemy", oxui::object_checkbox );
	OPTION( bool, local, "Sesame->C->Chams->Targets->Local", oxui::object_checkbox );

	OPTION( bool, flat, "Sesame->C->Chams->Main->Flat", oxui::object_checkbox );
	OPTION( bool, xqz, "Sesame->C->Chams->Main->XQZ", oxui::object_checkbox );
	OPTION ( bool, lcchams, "Sesame->C->Chams->Main->Backtrack", oxui::object_checkbox );
	OPTION ( bool, fake_matrix, "Sesame->C->Chams->Main->Fake Matrix", oxui::object_checkbox );

	OPTION ( oxui::color, chams_clr, "Sesame->C->Chams->Colors->Chams", oxui::object_colorpicker );
	OPTION ( oxui::color, chams_clr_xqz, "Sesame->C->Chams->Colors->XQZ", oxui::object_colorpicker );
	OPTION ( oxui::color, cham_lagcomp, "Sesame->C->Chams->Colors->Backtrack", oxui::object_colorpicker );
	OPTION ( oxui::color, cham_fake, "Sesame->C->Chams->Colors->Fake", oxui::object_colorpicker );

	if ( !m_mat )
		create_materials ( );

	auto set_vec = [ ] ( void* var, float x, float y, float z ) {
		vfunc< void ( __thiscall* )( void*, float, float, float ) > ( var, 11 )( var, x, y, z );
	};

	update_mats( );

	auto e = csgo::i::ent_list->get< player_t* >( info.m_entity_index );

	if ( e->valid ( ) ) {
		mdl_render_info [ e->idx ( ) ] = info;
	}

	if ( !e->valid( ) /*&& !features::chams::in_model*/ ) {
		if ( !csgo::i::mdl_render->is_forced_mat_override( ) ) {
			//csgo::i::render_view->set_alpha( 255 );
			//csgo::i::render_view->set_color( 255, 255, 255 );
		}
		
		hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
		return;
	}

	const auto recs = lagcomp::get ( e );

	if ( csgo::i::mdl_render->is_forced_mat_override ( ) ) {
		//csgo::i::render_view->set_color ( 255, 255, 255 );
		//csgo::i::render_view->set_alpha ( 255 );
		hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first.front ( ).m_bones : bone_to_world );
		return;
	}

	//if ( e == g::local ? false : !recs.second ) {
	//	csgo::i::render_view->set_alpha( 255 );
	//	csgo::i::render_view->set_color( 255, 255, 255 );
	//	hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first.front().m_bones : bone_to_world );
	//	return;
	//}

	if ( !g::local || !info.m_model || !chams || !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) ) {
		if ( e ) {
			csgo::i::render_view->set_color( 255, 255, 255 );
			csgo::i::render_view->set_alpha( 255 );
			hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first.front ( ).m_bones : bone_to_world );
		}

		return;
	}

	auto mdl_name = csgo::i::mdl_info->mdl_name( info.m_model );
	auto is_weapon = std::strstr( mdl_name, _( "arms" ) ) /*|| std::strstr( mdl_name, _( "v_models" ) )*/;

	if ( is_weapon || e || features::chams::in_model ) {
		auto is_player = e->valid( ) && e->is_player( );

		if ( ( ( is_player && enemy && e->team( ) != g::local->team( ) )
			|| ( is_player && team && e->team( ) == g::local->team( ) && e != g::local )
			|| ( is_player && local && e == g::local ) ) || features::chams::in_model || is_weapon ) {
			if ( is_weapon ) {
				csgo::i::render_view->set_alpha ( 255 );
				csgo::i::render_view->set_color ( 255, 255, 255 );
				// mat->set_material_var_flag( 268435456, false );
				m_mat_glow->set_material_var_flag ( 0x8000, true );
				m_mat_glow->set_material_var_flag ( 0x1000, flat );
				csgo::i::mdl_render->force_mat ( m_mat_glow );
				hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first.front ( ).m_bones : bone_to_world );
				csgo::i::mdl_render->force_mat ( nullptr );
			}
			/* hit matrix chams */
			else if ( features::chams::in_model ) {
				bool found = false;
				auto envmap = m_mat_glow->find_var ( _ ( "$envmaptint" ), &found );
				auto chams_glow_coeff = oxui::color ( ( cur_hit_matrix_rec.m_clr >> 16 ) & 0xff, ( cur_hit_matrix_rec.m_clr >> 8 ) & 0xff, ( cur_hit_matrix_rec.m_clr ) & 0xff, ( cur_hit_matrix_rec.m_clr >> 24 ) & 0xff );
				set_vec ( envmap, static_cast< float >( chams_glow_coeff.r ) / 255.0f * ( static_cast < float > ( chams_glow_coeff.a ) / 255.0f ), static_cast< float >( chams_glow_coeff.g ) / 255.0f * ( static_cast < float > ( chams_glow_coeff.a ) / 255.0f ), static_cast< float >( chams_glow_coeff.b ) / 255.0f * ( static_cast < float > ( chams_glow_coeff.a ) / 255.0f ) );

				csgo::i::render_view->set_alpha ( 255 );
				csgo::i::render_view->set_color ( 255, 255, 255 );
				// mat->set_material_var_flag( 268435456, false );
				m_mat_glow->set_material_var_flag ( 0x8000, true );
				m_mat_glow->set_material_var_flag ( 0x1000, flat );
				csgo::i::mdl_render->force_mat ( m_mat_glow );
				hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, (matrix3x4_t*)&cur_hit_matrix_rec.m_bones );
				csgo::i::mdl_render->force_mat ( nullptr );
			}
			else {
				/* extrapolation chams */
				//if ( e != g::local && e->vel ( ).length ( ) > 10.0f && !lagcomp::data::extrapolated_records [ e->idx ( ) ].empty ( ) && g::local->alive ( ) ) {
				//	csgo::i::render_view->set_alpha ( 255 );
				//	csgo::i::render_view->set_color ( 255, 50, 80 );
				//	auto mat = flat ? m_matflat : m_mat;
				//	mat->set_material_var_flag ( 0x8000, true );
				//	mat->set_material_var_flag ( 0x1000, flat );
				//	csgo::i::mdl_render->force_mat ( mat );
				//	hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, lagcomp::data::extrapolated_records [ e->idx ( ) ][ 0 ].m_bones );
				//	csgo::i::mdl_render->force_mat ( nullptr );
				//}
				
				if ( lcchams && e != g::local && e->vel ( ).length ( ) > 10.0f && !lagcomp::data::records [ e->idx ( ) ].empty ( ) && g::local->alive ( ) ) {
					csgo::i::render_view->set_alpha ( cham_lagcomp.a );
					csgo::i::render_view->set_color ( cham_lagcomp.r, cham_lagcomp.g, cham_lagcomp.b );
					auto mat = flat ? m_matflat : m_mat;
					mat->set_material_var_flag ( 0x8000, true );
					mat->set_material_var_flag ( 0x1000, flat );
					csgo::i::mdl_render->force_mat ( mat );
					hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, lagcomp::data::records [ e->idx ( ) ].back( ).m_bones );
					csgo::i::mdl_render->force_mat ( nullptr );
				}

				/* fake chams */
				if ( e == g::local && fake_matrix ) {
					auto& ref_matrix = animations::fake::matrix ( );
					const auto backup_matrix = ref_matrix;
				
					for ( auto& i : ref_matrix ) {
						i [ 0 ][ 3 ] += info.m_origin.x;
						i [ 1 ][ 3 ] += info.m_origin.y;
						i [ 2 ][ 3 ] += info.m_origin.z;
					}
				
					csgo::i::render_view->set_alpha ( cham_fake.a );
					csgo::i::render_view->set_color ( cham_fake.r, cham_fake.g, cham_fake.b );
					auto mat = flat ? m_matflat : m_mat;
					mat->set_material_var_flag ( 0x8000, false );
					mat->set_material_var_flag ( 0x1000, flat );
					csgo::i::mdl_render->force_mat ( mat );
					hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, ( matrix3x4_t* ) &ref_matrix );
					csgo::i::mdl_render->force_mat ( nullptr );
				
					ref_matrix = backup_matrix;
				}

				/* outline */
				//if ( xqz && chams_clr_xqz.a == 255 && chams_clr.a == 255 ) {
				//	csgo::i::render_view->set_alpha( 255 );
				//	csgo::i::render_view->set_color( 25, 25, 25 );
				//	auto mat = flat ? m_matflat : m_mat;
				//	mat->set_material_var_flag( 268435456, true );
				//	mat->set_material_var_flag( 0x8000, true );
				//	mat->set_material_var_flag( 0x1000, flat );
				//	csgo::i::mdl_render->force_mat( mat );
				//	hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first.front().m_bones : bone_to_world );
				//	csgo::i::mdl_render->force_mat( nullptr );
				//	mat->set_material_var_flag( 268435456, false );
				//}

				/* occluded */
				if ( e == g::local ? false : xqz ) {
					csgo::i::render_view->set_alpha ( chams_clr_xqz.a );
					csgo::i::render_view->set_color ( chams_clr_xqz.r, chams_clr_xqz.g, chams_clr_xqz.b );
					auto mat = flat ? m_matflat : m_mat;
					// mat->set_material_var_flag( 268435456, false );
					mat->set_material_var_flag ( 0x8000, true );
					mat->set_material_var_flag ( 0x1000, flat );
					csgo::i::mdl_render->force_mat ( mat );
					hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first.front ( ).m_bones : bone_to_world );
					csgo::i::mdl_render->force_mat ( nullptr );

					///* fake angle chams for enemies */ {
					//	csgo::i::render_view->set_alpha ( chams_clr_xqz.a );
					//	csgo::i::render_view->set_color ( 150, 25, 225 );
					//	auto mat = flat ? m_matflat : m_mat;
					//	// mat->set_material_var_flag( 268435456, false );
					//	mat->set_material_var_flag ( 0x8000, true );
					//	mat->set_material_var_flag ( 0x1000, flat );
					//	csgo::i::mdl_render->force_mat ( mat );
					//	hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, ( matrix3x4_t* ) &animations::data::bones [ e->idx ( ) ] );
					//	csgo::i::mdl_render->force_mat ( nullptr );
					//
					//	csgo::i::render_view->set_alpha ( 255 );
					//	csgo::i::render_view->set_color ( 255, 255, 255 );
					//	// mat->set_material_var_flag( 268435456, false );
					//	m_mat_glow->set_material_var_flag ( 0x8000, true );
					//	m_mat_glow->set_material_var_flag ( 0x1000, flat );
					//	csgo::i::mdl_render->force_mat ( m_mat_glow );
					//	hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, ( matrix3x4_t* ) &animations::data::bones [ e->idx ( ) ] );
					//	csgo::i::mdl_render->force_mat ( nullptr );
					//}
				}

				if ( true ) {
					auto mat = flat ? m_matflat : m_mat;
					// mat->set_material_var_flag( 268435456, false );
					mat->set_material_var_flag ( 0x8000, false );
					mat->set_material_var_flag ( 0x1000, flat );

					if ( ( local && e == g::local ) && g::local->scoped ( ) ) {
						csgo::i::render_view->set_alpha ( 22 );
						csgo::i::render_view->set_color ( 255, 255, 255 );
						csgo::i::mdl_render->force_mat ( nullptr );
					}
					else {
						csgo::i::render_view->set_alpha ( chams_clr.a );
						csgo::i::render_view->set_color ( chams_clr.r, chams_clr.g, chams_clr.b );
						csgo::i::mdl_render->force_mat ( mat );
					}
					
					
					hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first.front ( ).m_bones : bone_to_world );
					csgo::i::mdl_render->force_mat ( nullptr );
				}

				/* glow overlay */
				if ( glow && rim ) {
					csgo::i::render_view->set_alpha ( 255 );
					csgo::i::render_view->set_color ( 255, 255, 255 );
					// mat->set_material_var_flag( 268435456, false );
					m_mat_glow->set_material_var_flag ( 0x8000, true );
					m_mat_glow->set_material_var_flag ( 0x1000, flat );
					csgo::i::mdl_render->force_mat ( m_mat_glow );
					hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first.front ( ).m_bones : bone_to_world );
					csgo::i::mdl_render->force_mat ( nullptr );
				}
			}
		}
		else {
			if ( e == g::local ) {
				csgo::i::render_view->set_alpha( ( g::local && g::local->scoped( ) ) ? 50 : 255 );
				csgo::i::render_view->set_color( 255, 255, 255 );
				hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first.front ( ).m_bones : bone_to_world );
			}
			else {
				csgo::i::render_view->set_alpha( 255 );
				csgo::i::render_view->set_color( 255, 255, 255 );
				hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first.front ( ).m_bones : bone_to_world );
			}
		}
	}
	else {
		csgo::i::render_view->set_alpha( 255 );
		csgo::i::render_view->set_color( 255, 255, 255 );
		hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first.front ( ).m_bones : bone_to_world );
	}

	csgo::i::render_view->set_alpha( 255 );
	csgo::i::render_view->set_color( 255, 255, 255 );
}