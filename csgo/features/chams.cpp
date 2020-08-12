#include "chams.hpp"
#include "../menu/menu.hpp"
#include "../hooks/draw_model_execute.hpp"
#include "../globals.hpp"
#include "../animations/animations.hpp"
#include "lagcomp.hpp"
#include "glow.hpp"
#include "../animations/resolver.hpp"
#include "other_visuals.hpp"
#include "ragebot.hpp"
#include "../menu/options.hpp"

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
	m_mat_glow = create_mat( false, false, true, true );

	return true;
}

void update_mats( const features::visual_config_t& visuals, const sesui::color& clr ) {
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
		auto envmaptint_coeff = visuals.reflectivity / 100.0f;
		
		set_vec( envmaptint, envmaptint_coeff, envmaptint_coeff, envmaptint_coeff );
		set_vec( envmaptint1, envmaptint_coeff, envmaptint_coeff, envmaptint_coeff );
		set_vec( envmaptint2, envmaptint_coeff, envmaptint_coeff, envmaptint_coeff );
		set_vec( envmaptint3, envmaptint_coeff, envmaptint_coeff, envmaptint_coeff );

		auto phongboost = m_mat->find_var( _( "$phongboost" ), &found );
		auto phongboost1 = m_matflat->find_var( _( "$phongboost" ), &found );
		auto phongboost2 = m_matflat_wireframe->find_var( _( "$phongboost" ), &found );
		auto phongboost3 = m_mat_wireframe->find_var( _( "$phongboost" ), &found );
		auto phong_coeff = visuals.phong / 100.0;

		set_vec( phongboost, phong_coeff, phong_coeff, phong_coeff );
		set_vec( phongboost1, phong_coeff, phong_coeff, phong_coeff );
		set_vec( phongboost2, phong_coeff, phong_coeff, phong_coeff );
		set_vec( phongboost3, phong_coeff, phong_coeff, phong_coeff );
	}

	if ( m_mat_glow ) {
		auto rimlight_exponent = m_mat_glow->find_var ( _ ( "$envmapfresnelminmaxexp" ), &found );
		auto envmap = m_mat_glow->find_var( _( "$envmaptint" ), &found );
		set_vec ( rimlight_exponent, 0.0f, 1.0f, 1.0f + ( 14.0f - clr.a * 14.0f ) );
		set_vec( envmap, clr.r, clr.g, clr.b );
	}
}

std::array < mdlrender_info_t, 65 > mdl_render_info;
long long refresh_time = 0;

void features::chams::drawmodelexecute( void* ctx, void* state, const mdlrender_info_t& info, matrix3x4_t* bone_to_world ) {
	if ( !m_mat )
		create_materials ( );

	auto set_vec = [ ] ( void* var, float x, float y, float z ) {
		vfunc< void ( __thiscall* )( void*, float, float, float ) > ( var, 11 )( var, x, y, z );
	};

	if ( !g::local || !info.m_model || !csgo::i::engine->is_connected ( ) || !csgo::i::engine->is_in_game ( ) ) {
		csgo::i::render_view->set_color ( 255, 255, 255 );
		csgo::i::render_view->set_alpha ( 255 );
		hooks::old::draw_model_execute( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );

		return;
	}

	auto e = csgo::i::ent_list->get< player_t* >( info.m_entity_index );

	auto mdl_name = csgo::i::mdl_info->mdl_name ( info.m_model );
	auto is_arms = std::strstr ( mdl_name, _ ( "models/weapons/" ) ) || std::strstr ( mdl_name, _ ( "arms" ) );

	visual_config_t visuals;
	if ( !get_visuals( e, visuals ) ) {
		hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
		return;
	}

	update_mats ( visuals, visuals.rimlight_color );

	const auto recs = lagcomp::get ( e );

	if ( csgo::i::mdl_render->is_forced_mat_override ( ) ) {
		//csgo::i::render_view->set_color ( 255, 255, 255 );
		//csgo::i::render_view->set_alpha ( 255 );
		hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
		return;
	}

	if ( /*is_arms || */e || features::chams::in_model ) {
		auto is_player = e->valid( ) && e->is_player( );

		if ( is_arms ) {
			hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
		}
		else if ( is_player || features::chams::in_model ) {
			/* hit matrix chams */
			if ( features::chams::in_model && visuals.hit_matrix ) {
				bool found = false;
				auto envmap = m_mat_glow->find_var ( _ ( "$envmaptint" ), &found );
				set_vec ( envmap, visuals.hit_matrix_color.r, visuals.hit_matrix_color.g, visuals.hit_matrix_color.b );

				auto rimlight_exponent = m_mat_glow->find_var ( _ ( "$envmapfresnelminmaxexp" ), &found );
				set_vec ( rimlight_exponent, 0.0f, 1.0f, 1.0f + ( 14.0f - visuals.hit_matrix_color.a * 14.0f ) );

				csgo::i::render_view->set_alpha ( 255 );
				csgo::i::render_view->set_color ( 255, 255, 255 );
				// mat->set_material_var_flag( 268435456, false );
				m_mat_glow->set_material_var_flag ( 0x8000, visuals.backtrack_chams );
				m_mat_glow->set_material_var_flag ( 0x1000, visuals.chams_flat );
				csgo::i::mdl_render->force_mat ( m_mat_glow );
				hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, (matrix3x4_t*)&cur_hit_matrix_rec.m_bones );
				csgo::i::mdl_render->force_mat ( nullptr );
			}
			else {

			if ( visuals.backtrack_chams && e->vel().length_2d() > 10.0f && e != g::local && g::local->alive() ) {
				csgo::i::render_view->set_alpha ( visuals.backtrack_chams_color.a * 255.0f );
				csgo::i::render_view->set_color ( visuals.backtrack_chams_color.r * 255.0f, visuals.backtrack_chams_color.g * 255.0f, visuals.backtrack_chams_color.b * 255.0f );
				auto mat = visuals.chams_flat ? m_matflat : m_mat;
				mat->set_material_var_flag ( 0x8000, visuals.backtrack_chams );
				mat->set_material_var_flag ( 0x1000, visuals.chams_flat );
				csgo::i::mdl_render->force_mat ( mat );

				if ( features::ragebot::get_misses ( e->idx ( ) ).bad_resolve % 3 == 0 )
					hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, lagcomp::data::cham_records [ e->idx ( ) ].m_bones1 );
				else if ( features::ragebot::get_misses ( e->idx ( ) ).bad_resolve % 3 == 1 )
					hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, lagcomp::data::cham_records [ e->idx ( ) ].m_bones2 );
				else
					hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, lagcomp::data::cham_records [ e->idx ( ) ].m_bones3 );

				csgo::i::mdl_render->force_mat ( nullptr );
			}

				/* fake chams */
				if ( e == g::local ) {
					auto& ref_matrix = animations::fake::matrix ( );
					const auto backup_matrix = ref_matrix;
				
					for ( auto& i : ref_matrix )
						i.set_origin ( i.origin ( ) + ( visuals.desync_chams_fakelag ? old_origin : info.m_origin ) );
				
					if ( visuals.desync_chams ) {
						csgo::i::render_view->set_alpha ( visuals.desync_chams_color.a * 255.0f );
						csgo::i::render_view->set_color ( visuals.desync_chams_color.r * 255.0f, visuals.desync_chams_color.g * 255.0f, visuals.desync_chams_color.b * 255.0f );
						auto mat = visuals.chams_flat ? m_matflat : m_mat;
						mat->set_material_var_flag ( 0x8000, false );
						mat->set_material_var_flag ( 0x1000, visuals.chams_flat );
						csgo::i::mdl_render->force_mat ( mat );
						hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, ( matrix3x4_t* ) &ref_matrix );
						csgo::i::mdl_render->force_mat ( nullptr );
					}

					if ( visuals.desync_chams_rimlight ) {
						update_mats ( visuals, visuals.desync_rimlight_color );
						csgo::i::render_view->set_alpha ( 255 );
						csgo::i::render_view->set_color ( 255, 255, 255 );
						m_mat_glow->set_material_var_flag ( 0x8000, true );
						m_mat_glow->set_material_var_flag ( 0x1000, visuals.chams_flat );
						csgo::i::mdl_render->force_mat ( m_mat_glow );
						hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, ( matrix3x4_t* ) &ref_matrix );
						csgo::i::mdl_render->force_mat ( nullptr );
					}
				
					ref_matrix = backup_matrix;
				}
			
				if ( visuals.chams ) {
					/* occluded */
					if ( e == g::local ? false : visuals.chams_xqz ) {
						csgo::i::render_view->set_alpha ( visuals.chams_xqz_color.a * 255.0f );
						csgo::i::render_view->set_color ( visuals.chams_xqz_color.r * 255.0f, visuals.chams_xqz_color.g * 255.0f, visuals.chams_xqz_color.b * 255.0f );
						auto mat = visuals.chams_flat ? m_matflat : m_mat;
						// mat->set_material_var_flag( 268435456, false );
						mat->set_material_var_flag ( 0x8000, true );
						mat->set_material_var_flag ( 0x1000, visuals.chams_flat );
						csgo::i::mdl_render->force_mat ( mat );
						hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
						csgo::i::mdl_render->force_mat ( nullptr );
					}

					/* normal chams */
					auto mat = visuals.chams_flat ? m_matflat : m_mat;
					// mat->set_material_var_flag( 268435456, false );
					mat->set_material_var_flag ( 0x8000, false );
					mat->set_material_var_flag ( 0x1000, visuals.chams_flat );

					if ( ( g::local && e == g::local ) && g::local->scoped ( ) ) {
						csgo::i::render_view->set_alpha ( 22 );
						csgo::i::render_view->set_color ( 255, 255, 255 );
						csgo::i::mdl_render->force_mat ( nullptr );
					}
					else {
						csgo::i::render_view->set_alpha ( visuals.chams_color.a * 255.0f );
						csgo::i::render_view->set_color ( visuals.chams_color.r * 255.0f, visuals.chams_color.g * 255.0f, visuals.chams_color.b * 255.0f );
						csgo::i::mdl_render->force_mat ( mat );
					}


					hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
					csgo::i::mdl_render->force_mat ( nullptr );
				}
				else {
					csgo::i::render_view->set_alpha ( 255 );
					csgo::i::render_view->set_color ( 255, 255, 255 );
					csgo::i::mdl_render->force_mat ( nullptr );

					if ( ( g::local && e == g::local ) && g::local->scoped ( ) )
						csgo::i::render_view->set_alpha ( 22 );

					hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
				}

				/* glow overlay */
				if ( visuals.rimlight_overlay ) {
					update_mats ( visuals, visuals.rimlight_color );
					csgo::i::render_view->set_alpha ( 255 );
					csgo::i::render_view->set_color ( 255, 255, 255 );
					// mat->set_material_var_flag( 268435456, false );
					m_mat_glow->set_material_var_flag ( 0x8000, true );
					m_mat_glow->set_material_var_flag ( 0x1000, visuals.chams_flat );
					csgo::i::mdl_render->force_mat ( m_mat_glow );
					hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
					csgo::i::mdl_render->force_mat ( nullptr );
				}
			}
		}
		else {
			if ( e == g::local ) {
				csgo::i::render_view->set_alpha( ( g::local && g::local->scoped( ) ) ? 50 : 255 );
				csgo::i::render_view->set_color( 255, 255, 255 );
				hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
			}
			else {
				csgo::i::render_view->set_alpha( 255 );
				csgo::i::render_view->set_color( 255, 255, 255 );
				hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
			}
		}
	}
	else {
		csgo::i::render_view->set_alpha( 255 );
		csgo::i::render_view->set_color( 255, 255, 255 );
		hooks::old::draw_model_execute ( csgo::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
	}

	csgo::i::render_view->set_alpha( 255 );
	csgo::i::render_view->set_color( 255, 255, 255 );
}