#include "chams.hpp"
#include "../menu/menu.hpp"
#include "../hooks.hpp"
#include "../oxui/themes/purple.hpp"
#include "../globals.hpp"
#include "../animations/animations.hpp"
#include "lagcomp.hpp"
#include "glow.hpp"
#include "../animations/resolver.hpp"
#include "other_visuals.hpp"

material_t* m_mat = nullptr,
* m_matflat = nullptr,
* m_mat_wireframe = nullptr,
* m_matflat_wireframe = nullptr,
* m_mat_glow;

animations::resolver::hit_matrix_rec_t cur_hit_matrix_rec;
vec3_t features::chams::old_origin;
bool features::chams::in_model = false;

bool create_materials( ) {
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
			set_int ( kv, _ ( "$envmapfresnel" ), 1 );

			set_string( kv, _( "$envmaptint" ), _ ( "[ 0 0 0 ]" ) );
			set_string( kv, _( "$envmapfresnelminmaxexp" ), _( "[ 0 1 2 ]" ) );
			set_string( kv, _( "$alpha" ), _( "1" ) );
		}
		else {
			// set_string( kv, "$basetexture", "vgui/white_additive" );
			set_string( kv, _( "$basetexture" ), _( "vgui/white_additive" ) );
			set_string( kv, _( "$envmaptint" ), _ ( "[ 0 0 0 ]" ) );
			set_string( kv, _( "$envmap" ), _( "env_cubemap" ) );

			set_int( kv, _( "$phong" ), 1 );
			set_int( kv, _( "$phongexponent" ), 15 );
			set_int( kv, _( "$normalmapalphaenvmask" ), 1 );
			set_string( kv, _( "$phongboost" ), _ ( "[ 0 0 0 ]" ) );
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

void update_mats( oxui::visual_editor::settings_t* visuals, const oxui::color& clr ) {
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
		auto envmaptint_coeff = visuals->reflectivity->value / 100.0;
		
		set_vec( envmaptint, envmaptint_coeff, envmaptint_coeff, envmaptint_coeff );
		set_vec( envmaptint1, envmaptint_coeff, envmaptint_coeff, envmaptint_coeff );
		set_vec( envmaptint2, envmaptint_coeff, envmaptint_coeff, envmaptint_coeff );
		set_vec( envmaptint3, envmaptint_coeff, envmaptint_coeff, envmaptint_coeff );

		auto phongboost = m_mat->find_var( _( "$phongboost" ), &found );
		auto phongboost1 = m_matflat->find_var( _( "$phongboost" ), &found );
		auto phongboost2 = m_matflat_wireframe->find_var( _( "$phongboost" ), &found );
		auto phongboost3 = m_mat_wireframe->find_var( _( "$phongboost" ), &found );
		auto phong_coeff = visuals->phong->value / 100.0;

		set_vec( phongboost, phong_coeff, phong_coeff, phong_coeff );
		set_vec( phongboost1, phong_coeff, phong_coeff, phong_coeff );
		set_vec( phongboost2, phong_coeff, phong_coeff, phong_coeff );
		set_vec( phongboost3, phong_coeff, phong_coeff, phong_coeff );
	}

	if ( m_mat_glow ) {
		auto envmap = m_mat_glow->find_var( _( "$envmaptint" ), &found );
		auto chams_glow_coeff = static_cast< float >( clr.a ) / 255.0f;
		set_vec( envmap, static_cast< float >( clr.r ) / 255.0f * chams_glow_coeff, static_cast< float >( clr.g ) / 255.0f * chams_glow_coeff, static_cast< float >( clr.b ) / 255.0f * chams_glow_coeff );
	}
}

std::array < mdlrender_info_t, 65 > mdl_render_info;
long long refresh_time = 0;

void features::chams::drawmodelexecute( void* ctx, void* state, const mdlrender_info_t& info, matrix3x4_t* bone_to_world ) {
	/* desync chams */
	OPTION ( bool, show_fakelag_on_desync_chams, "Sesame->C->Local->Options->Show Fakelag On Desync Chams", oxui::object_checkbox );
	OPTION ( bool, desync_chams, "Sesame->C->Local->Options->Desync Chams", oxui::object_checkbox );
	OPTION ( bool, rimlight_desync_chams, "Sesame->C->Local->Options->Rimlight Desync Chams", oxui::object_checkbox );
	OPTION ( oxui::color, desync_chams_color, "Sesame->C->Local->Options->Desync Chams Color", oxui::object_colorpicker );
	OPTION ( oxui::color, rimlight_desync_chams_color, "Sesame->C->Local->Options->Rimlight Desync Color", oxui::object_colorpicker );

	if ( !m_mat ) create_materials ( );

	auto set_vec = [ ] ( void* var, float x, float y, float z ) {
		vfunc< void ( __thiscall* )( void*, float, float, float ) > ( var, 11 )( var, x, y, z );
	};

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

	oxui::visual_editor::settings_t* visuals;
	if ( !get_visuals( e, &visuals ) ) {
		hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
		return;
	}

	update_mats ( visuals, visuals->rimlight_picker->clr );

	const auto recs = lagcomp::get ( e );

	if ( csgo::i::mdl_render->is_forced_mat_override ( ) ) {
		//csgo::i::render_view->set_color ( 255, 255, 255 );
		//csgo::i::render_view->set_alpha ( 255 );
		hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first [ 0 ].m_bones : bone_to_world );
		return;
	}

	if ( !g::local || !info.m_model || !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) ) {
		if ( e ) {
			csgo::i::render_view->set_color( 255, 255, 255 );
			csgo::i::render_view->set_alpha( 255 );
			hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first [ 0 ].m_bones : bone_to_world );
		}

		return;
	}

	auto mdl_name = csgo::i::mdl_info->mdl_name( info.m_model );
	//auto is_weapon = std::strstr( mdl_name, _( "arms" ) ) /*|| std::strstr( mdl_name, _( "v_models" ) )*/;

	if ( /*is_weapon ||*/ e || features::chams::in_model ) {
		auto is_player = e->valid( ) && e->is_player( );

		if ( is_player || features::chams::in_model /*|| is_weapon*/ ) {
			/*if ( is_weapon ) {
				csgo::i::render_view->set_alpha ( 255 );
				csgo::i::render_view->set_color ( 255, 255, 255 );
				// mat->set_material_var_flag( 268435456, false );
				m_mat_glow->set_material_var_flag ( 0x8000, true );
				m_mat_glow->set_material_var_flag ( 0x1000, visuals->model_type == oxui::model_type_t::model_flat );
				csgo::i::mdl_render->force_mat ( m_mat_glow );
				hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
				csgo::i::mdl_render->force_mat ( nullptr );
			}*/
			/* hit matrix chams */
		/*else*/ if ( features::chams::in_model && visuals->hit_matrix ) {
				bool found = false;
				auto envmap = m_mat_glow->find_var ( _ ( "$envmaptint" ), &found );
				auto chams_glow_coeff = oxui::color ( ( cur_hit_matrix_rec.m_clr >> 16 ) & 0xff, ( cur_hit_matrix_rec.m_clr >> 8 ) & 0xff, ( cur_hit_matrix_rec.m_clr ) & 0xff, ( cur_hit_matrix_rec.m_clr >> 24 ) & 0xff );
				set_vec ( envmap, static_cast< float >( chams_glow_coeff.r ) / 255.0f * ( static_cast < float > ( chams_glow_coeff.a ) / 255.0f ), static_cast< float >( chams_glow_coeff.g ) / 255.0f * ( static_cast < float > ( chams_glow_coeff.a ) / 255.0f ), static_cast< float >( chams_glow_coeff.b ) / 255.0f * ( static_cast < float > ( chams_glow_coeff.a ) / 255.0f ) );

				csgo::i::render_view->set_alpha ( 255 );
				csgo::i::render_view->set_color ( 255, 255, 255 );
				// mat->set_material_var_flag( 268435456, false );
				m_mat_glow->set_material_var_flag ( 0x8000, true );
				m_mat_glow->set_material_var_flag ( 0x1000, visuals->model_type == oxui::model_type_t::model_flat );
				csgo::i::mdl_render->force_mat ( m_mat_glow );
				hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, (matrix3x4_t*)&cur_hit_matrix_rec.m_bones );
				csgo::i::mdl_render->force_mat ( nullptr );
			}
			else {
				if ( visuals->backtrack && e != g::local && e->vel ( ).length ( ) > 10.0f && !lagcomp::data::records [ e->idx ( ) ].empty ( ) && g::local->alive ( ) ) {
					csgo::i::render_view->set_alpha ( visuals->backtrack_picker->clr.a );
					csgo::i::render_view->set_color ( visuals->backtrack_picker->clr.r, visuals->backtrack_picker->clr.g, visuals->backtrack_picker->clr.b );
					auto mat = ( visuals->model_type == oxui::model_type_t::model_flat ) ? m_matflat : m_mat;
					mat->set_material_var_flag ( 0x8000, true );
					mat->set_material_var_flag ( 0x1000, visuals->model_type == oxui::model_type_t::model_flat );
					csgo::i::mdl_render->force_mat ( mat );
					hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, lagcomp::data::records [ e->idx ( ) ].back( ).m_bones );
					csgo::i::mdl_render->force_mat ( nullptr );
				}

				/* fake chams */
				if ( e == g::local ) {
					auto& ref_matrix = animations::fake::matrix ( );
					const auto backup_matrix = ref_matrix;
				
					for ( auto& i : ref_matrix )
						i.set_origin ( i.origin ( ) + ( show_fakelag_on_desync_chams ? old_origin : info.m_origin ) );
				
					if ( desync_chams ) {
						csgo::i::render_view->set_alpha ( desync_chams_color.a );
						csgo::i::render_view->set_color ( desync_chams_color.r, desync_chams_color.g, desync_chams_color.b );
						auto mat = ( visuals->model_type == oxui::model_type_t::model_flat ) ? m_matflat : m_mat;
						mat->set_material_var_flag ( 0x8000, false );
						mat->set_material_var_flag ( 0x1000, visuals->model_type == oxui::model_type_t::model_flat );
						csgo::i::mdl_render->force_mat ( mat );
						hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, ( matrix3x4_t* ) &ref_matrix );
						csgo::i::mdl_render->force_mat ( nullptr );
					}

					if ( rimlight_desync_chams ) {
						update_mats ( visuals, rimlight_desync_chams_color );
						csgo::i::render_view->set_alpha ( 255 );
						csgo::i::render_view->set_color ( 255, 255, 255 );
						m_mat_glow->set_material_var_flag ( 0x8000, true );
						m_mat_glow->set_material_var_flag ( 0x1000, visuals->model_type == oxui::model_type_t::model_flat );
						csgo::i::mdl_render->force_mat ( m_mat_glow );
						hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, ( matrix3x4_t* ) &ref_matrix );
						csgo::i::mdl_render->force_mat ( nullptr );
					}
				
					ref_matrix = backup_matrix;
				}
			
				if ( visuals->model_type != oxui::model_type_t::model_default ) {
					/* occluded */
					if ( e == g::local ? false : visuals->xqz ) {
						csgo::i::render_view->set_alpha ( visuals->xqz_picker->clr.a );
						csgo::i::render_view->set_color ( visuals->xqz_picker->clr.r, visuals->xqz_picker->clr.g, visuals->xqz_picker->clr.b );
						auto mat = ( visuals->model_type == oxui::model_type_t::model_flat ) ? m_matflat : m_mat;
						// mat->set_material_var_flag( 268435456, false );
						mat->set_material_var_flag ( 0x8000, true );
						mat->set_material_var_flag ( 0x1000, visuals->model_type == oxui::model_type_t::model_flat );
						csgo::i::mdl_render->force_mat ( mat );
						hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first [ 0 ].m_bones : bone_to_world );
						csgo::i::mdl_render->force_mat ( nullptr );
					}

					/* normal chams */
					auto mat = ( visuals->model_type == oxui::model_type_t::model_flat ) ? m_matflat : m_mat;
					// mat->set_material_var_flag( 268435456, false );
					mat->set_material_var_flag ( 0x8000, false );
					mat->set_material_var_flag ( 0x1000, visuals->model_type == oxui::model_type_t::model_flat );

					if ( ( g::local && e == g::local ) && g::local->scoped ( ) ) {
						csgo::i::render_view->set_alpha ( 22 );
						csgo::i::render_view->set_color ( 255, 255, 255 );
						csgo::i::mdl_render->force_mat ( nullptr );
					}
					else {
						csgo::i::render_view->set_alpha ( visuals->cham_picker->clr.a );
						csgo::i::render_view->set_color ( visuals->cham_picker->clr.r, visuals->cham_picker->clr.g, visuals->cham_picker->clr.b );
						csgo::i::mdl_render->force_mat ( mat );
					}


					hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first [ 0 ].m_bones : bone_to_world );
					csgo::i::mdl_render->force_mat ( nullptr );
				}
				else {
					csgo::i::render_view->set_alpha ( 255 );
					csgo::i::render_view->set_color ( 255, 255, 255 );
					csgo::i::mdl_render->force_mat ( nullptr );

					if ( ( g::local && e == g::local ) && g::local->scoped ( ) )
						csgo::i::render_view->set_alpha ( 22 );

					hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first [ 0 ].m_bones : bone_to_world );
				}

				/* glow overlay */
				if ( visuals->rimlight ) {
					update_mats ( visuals, visuals->rimlight_picker->clr );
					csgo::i::render_view->set_alpha ( 255 );
					csgo::i::render_view->set_color ( 255, 255, 255 );
					// mat->set_material_var_flag( 268435456, false );
					m_mat_glow->set_material_var_flag ( 0x8000, true );
					m_mat_glow->set_material_var_flag ( 0x1000, visuals->model_type == oxui::model_type_t::model_flat );
					csgo::i::mdl_render->force_mat ( m_mat_glow );
					hooks::drawmodelexecute ( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first [ 0 ].m_bones : bone_to_world );
					csgo::i::mdl_render->force_mat ( nullptr );
				}
			}
		}
		else {
			if ( e == g::local ) {
				csgo::i::render_view->set_alpha( ( g::local && g::local->scoped( ) ) ? 50 : 255 );
				csgo::i::render_view->set_color( 255, 255, 255 );
				hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first [ 0 ].m_bones : bone_to_world );
			}
			else {
				csgo::i::render_view->set_alpha( 255 );
				csgo::i::render_view->set_color( 255, 255, 255 );
				hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first [ 0 ].m_bones : bone_to_world );
			}
		}
	}
	else {
		csgo::i::render_view->set_alpha( 255 );
		csgo::i::render_view->set_color( 255, 255, 255 );
		hooks::drawmodelexecute( csgo::i::mdl_render, nullptr, ctx, state, info, recs.second ? recs.first [ 0 ].m_bones : bone_to_world );
	}

	csgo::i::render_view->set_alpha( 255 );
	csgo::i::render_view->set_color( 255, 255, 255 );
}