#include "chams.hpp"
#include "../menu/menu.hpp"
#include "../hooks/draw_model_execute.hpp"
#include "../globals.hpp"
#include "../animations/anims.hpp"
#include "glow.hpp"
#include "../animations/resolver.hpp"
#include "other_visuals.hpp"
#include "ragebot.hpp"
#include "../menu/options.hpp"

material_t* m_mat = nullptr ,
* m_matflat = nullptr ,
* m_mat_wireframe = nullptr ,
* m_matflat_wireframe = nullptr ,
* m_mat_glow;

vec3_t features::chams::old_origin;
bool features::chams::in_model = false;

bool create_materials( ) {
	auto ikv = [ ] ( void* kv , const char* name ) {
		static auto ikv_fn = pattern::search( _( "client.dll" ) , _( "55 8B EC 51 33 C0 C7 45" ) ).get< void ( __thiscall* )( void*, const char*, void*, void* ) >( );
		ikv_fn( kv , name, nullptr, nullptr );
	};

	auto lfb = [ ] ( void* kv , const char* name , const char* buf ) {
		using lfb_fn = void( __thiscall* )( void* , const char* , const char* , void* , const char* , void* , void* );
		static auto lfb = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 34 53 8B 5D 0C 89" ) ).get< lfb_fn >( );
		lfb( kv , name , buf , nullptr , nullptr , nullptr , nullptr );
	};

	auto find_key = [ ] ( void* kv , const char* name , bool create ) {
		using find_key_fn = void* ( __thiscall* )( void* , const char* , bool );
		static auto findkey = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 EC 1C 53 8B D9 85 DB" ) ).get< find_key_fn >( );
		return findkey( kv , name , create );
	};

	auto set_int = [ find_key ] ( void* kv , const char* name , int val , bool create = true ) {
		auto k = find_key( kv , name , create );

		if ( k ) {
			*( int* ) ( ( uintptr_t ) k + 0xC ) = val;
			*( char* ) ( ( uintptr_t ) k + 0x10 ) = 2;
		}
	};

	auto set_string = [ find_key ] ( void* kv , const char* name , const char* val , bool create = true ) {
		auto k = find_key( kv , name , create );

		if ( k ) {
			using setstring_fn = void( __thiscall* )( void* , const char* );
			static auto setstring = pattern::search( _( "client.dll" ) , _( "55 8B EC A1 ? ? ? ? 53 56 57 8B F9 8B 08 8B 01" ) ).get< setstring_fn >( );
			setstring( k , val );
		}
	};

	// XREF: Function DrawSpriteModel client.dll
	auto set_vec = [ ] ( void* var , float x , float y , float z ) {
		vfunc< void( __thiscall* )( void* , float , float , float ) >( var , 11 )( var , x , y , z );
	};

	auto create_mat = [ & ] ( bool xqz , bool flat , bool wireframe , bool glow ) {
		static auto created = 0;
		std::string type = flat ? _( "UnlitGeneric" ) : _( "VertexLitGeneric" );

		if ( glow )
			type = _( "VertexLitGeneric" );

		// allocating space for key values
		auto kv = malloc( 36 );

		ikv( kv , type.c_str( ) );

		if ( glow ) {
			auto found = false;

			set_string( kv , _( "$envmap" ) , _( "models/effects/cube_white" ) );
			set_int( kv , _( "$additive" ) , 1 );
			set_int( kv , _( "$envmapfresnel" ) , 1 );

			set_string( kv , _( "$envmaptint" ) , _( "[ 0 0 0 ]" ) );
			set_string( kv , _( "$envmapfresnelminmaxexp" ) , _( "[ 0 1 2 ]" ) );
			set_string( kv , _( "$alpha" ) , _( "1" ) );
			set_int( kv , _( "$znearer" ) , 1 );
		}
		else {
			set_string( kv , _( "$basetexture" ) , _( "vgui/white_additive" ) );
			set_string( kv , _( "$envmaptint" ) , _( "[ 0 0 0 ]" ) );
			set_string( kv , _( "$envmap" ) , _( "env_cubemap" ) );

			set_int( kv , _( "$phong" ) , 1 );
			set_int( kv , _( "$phongexponent" ) , 15 );
			set_int( kv , _( "$normalmapalphaenvmask" ) , 1 );
			set_string( kv , _( "$phongboost" ) , _( "[ 0 0 0 ]" ) );
			//set_string( kv, "$phongfresnelranges", "[.5 .5 1]" );
			set_int( kv , _( "$BasemapAlphaPhongMask" ) , 1 );

			set_int( kv , _( "$model" ) , 1 );
			set_int( kv , _( "$flat" ) , 1 );
			set_int( kv , _( "$selfillum" ) , 1 );
			set_int( kv , _( "$halflambert" ) , 1 );
			set_int( kv , _( "$ignorez" ) , 1 );
			set_int( kv , _( "$znearer" ) , 1 );
		}

		auto matname = _( "mat_" ) + std::to_string( created );

		// lfb( kv, matname.c_str( ), matdata.c_str( ) );

		// creating material
		auto mat = cs::i::mat_sys->createmat( matname.c_str( ) , kv );

		// incrementing reference count
		mat->increment_reference_count( );

		// we want a different material allocated every time
		created++;

		return mat;
	};

	//CLEAR_START
	m_mat = create_mat( false , false , false , false );
	m_matflat = create_mat( false , true , false , false );
	m_mat_wireframe = create_mat( false , false , true , false );
	m_matflat_wireframe = create_mat( false , true , true , false );
	m_mat_glow = create_mat( false , false , true , true );
	//CLEAR_END

	return true;
}

void update_mats( const features::visual_config_t& visuals , const options::option::colorf& clr ) {
		// XREF: Function DrawSpriteModel client.dll
		auto set_vec = [ ] ( void* var , float x , float y , float z ) {
		vfunc< void( __thiscall* )( void* , float , float , float ) >( var , 11 )( var , x , y , z );
	};

	auto set_int = [ ] ( void* var , int val ) {
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
		auto envmaptint = m_mat->find_var( _( "$envmaptint" ) , &found );
		auto envmaptint1 = m_matflat->find_var( _( "$envmaptint" ) , &found );
		auto envmaptint2 = m_matflat_wireframe->find_var( _( "$envmaptint" ) , &found );
		auto envmaptint3 = m_mat_wireframe->find_var( _( "$envmaptint" ) , &found );
		auto envmaptint_coeff = visuals.reflectivity / 100.0f;

		set_vec( envmaptint , envmaptint_coeff , envmaptint_coeff , envmaptint_coeff );
		set_vec( envmaptint1 , envmaptint_coeff , envmaptint_coeff , envmaptint_coeff );
		set_vec( envmaptint2 , envmaptint_coeff , envmaptint_coeff , envmaptint_coeff );
		set_vec( envmaptint3 , envmaptint_coeff , envmaptint_coeff , envmaptint_coeff );

		auto phongboost = m_mat->find_var( _( "$phongboost" ) , &found );
		auto phongboost1 = m_matflat->find_var( _( "$phongboost" ) , &found );
		auto phongboost2 = m_matflat_wireframe->find_var( _( "$phongboost" ) , &found );
		auto phongboost3 = m_mat_wireframe->find_var( _( "$phongboost" ) , &found );
		auto phong_coeff = visuals.phong / 100.0;

		set_vec( phongboost , phong_coeff , phong_coeff , phong_coeff );
		set_vec( phongboost1 , phong_coeff , phong_coeff , phong_coeff );
		set_vec( phongboost2 , phong_coeff , phong_coeff , phong_coeff );
		set_vec( phongboost3 , phong_coeff , phong_coeff , phong_coeff );
	}

	if ( m_mat_glow ) {
		auto rimlight_exponent = m_mat_glow->find_var( _( "$envmapfresnelminmaxexp" ) , &found );
		auto envmap = m_mat_glow->find_var( _( "$envmaptint" ) , &found );
		set_vec( rimlight_exponent , 0.0f , 1.0f , 1.0f + ( 14.0f - clr.a * 14.0f ) );
		set_vec( envmap , clr.r , clr.g , clr.b );
	}
}

std::array < mdlrender_info_t , 65 > mdl_render_info;
long long refresh_time = 0;

void features::chams::add_shot ( player_t* player, const anims::anim_info_t& anim_info ) {
	visual_config_t visuals;
	if ( !get_visuals ( player, visuals ) || !visuals.hit_matrix )
		return;

	auto renderable = player->renderable( );

	if ( !renderable )
		return;

	auto model = player->mdl( );

	if ( !model )
		return;

	auto hdr = cs::i::mdl_info->studio_mdl ( model );

	if ( !hdr )
		return;

	anims::resolver::hit_matrix_rec_t rec {};

	rec.m_bones = anim_info.m_aim_bones [ anim_info.m_side ];

	rec.m_render_info.m_origin = anim_info.m_origin;
	rec.m_render_info.m_angles = anim_info.m_abs_angles [ anim_info.m_side ];
	rec.m_render_info.m_renderable = renderable;
	rec.m_render_info.m_model = model;
	rec.m_render_info.m_lighting_offset = nullptr;
	rec.m_render_info.m_lighting_origin = nullptr;
	rec.m_render_info.m_hitbox_set = player->hitbox_set ( );
	rec.m_render_info.m_skin = player->get_skin ( );
	rec.m_render_info.m_body = player->body ( );
	rec.m_render_info.m_entity_index = player->idx ( );
	rec.m_render_info.m_instance = vfunc<uint16_t ( __thiscall* )( void* )> ( renderable, 30 )( renderable );
	rec.m_render_info.m_flags = 0x1;
	rec.m_render_info.m_model_to_world = rec.m_bones.data ( );

	rec.m_mdl_state.decals = 0;
	rec.m_mdl_state.lod = 0;
	rec.m_mdl_state.draw_flags = 0;
	rec.m_mdl_state.studiohdr = hdr;
	rec.m_mdl_state.studio_hw_data = vfunc<void* ( __thiscall* )( void*, uint16_t )> ( cs::i::mdl_cache, 15 )( cs::i::mdl_cache, cs::i::mdl_info->cache_handle( model ) );
	rec.m_mdl_state.renderable = renderable;
	rec.m_mdl_state.mdl_to_world = rec.m_bones.data ( );
	
	//for ( auto& mat : rec.m_bones )
	//	cs::angle_matrix ( rec.m_render_info.m_angles, rec.m_render_info.m_origin, mat );

	rec.m_time = cs::i::globals->m_curtime;
	rec.m_pl = player->idx ( );
	rec.m_color = { visuals.hit_matrix_color.r, visuals.hit_matrix_color.g, visuals.hit_matrix_color.b, visuals.hit_matrix_color.a };

	anims::resolver::hit_matrix_rec.push_back ( rec );

	anims::resolver::hit_matrix_rec.back ( ).m_render_info.m_model_to_world = rec.m_bones.data ( );
	anims::resolver::hit_matrix_rec.back ( ).m_mdl_state.mdl_to_world = rec.m_bones.data ( );
}

void features::chams::cull_shots ( ) {
	if ( !g::local || !g::local->alive( ) ) {
		if ( !anims::resolver::hit_matrix_rec.empty ( ) )
			anims::resolver::hit_matrix_rec.clear ( );

		return;
	}

	while ( !anims::resolver::hit_matrix_rec.empty ( ) && anims::resolver::hit_matrix_rec.back ( ).m_time + 4.0f < cs::i::globals->m_curtime )
		anims::resolver::hit_matrix_rec.pop_back ( );
}

anims::resolver::hit_matrix_rec_t* cur_hit_rec = nullptr;
void features::chams::render_shots ( ) {
	cull_shots ( );

	auto ctx = cs::i::mat_sys->get_context ( );

	for ( auto& rec : anims::resolver::hit_matrix_rec ) {
		cur_hit_rec = &rec;

		in_model = true;
		drawmodelexecute ( ctx, &rec.m_mdl_state, rec.m_render_info, rec.m_bones.data ( ) );
		in_model = false;

		cur_hit_rec = nullptr;

		rec.m_color [ 0 ] -= 0.25f * cs::i::globals->m_frametime;
		rec.m_color [ 1 ] -= 0.25f * cs::i::globals->m_frametime;
		rec.m_color [ 2 ] -= 0.25f * cs::i::globals->m_frametime;
		rec.m_color [ 3 ] -= 0.25f * cs::i::globals->m_frametime;
	}
}

void features::chams::drawmodelexecute( void* ctx , void* state , const mdlrender_info_t& info , matrix3x4_t* bone_to_world ) {
	if ( !m_mat )
		create_materials( );

	auto set_vec = [ ] ( void* var , float x , float y , float z ) {
		vfunc< void( __thiscall* )( void* , float , float , float ) >( var , 11 )( var , x , y , z );
	};

	if ( cs::i::mdl_render->is_forced_mat_override( ) && !features::chams::in_model ) {
		//csgo::i::render_view->set_color ( 255, 255, 255 );
		//csgo::i::render_view->set_alpha ( 255 );
		hooks::old::draw_model_execute( cs::i::mdl_render , nullptr , ctx , state , info , bone_to_world );
		return;
	}

	if ( ( !g::local || !info.m_model || !cs::i::engine->is_connected ( ) || !cs::i::engine->is_in_game ( ) ) && !features::chams::in_model ) {
		cs::i::render_view->set_color( 255 , 255 , 255 );
		cs::i::render_view->set_alpha( 255 );
		hooks::old::draw_model_execute( cs::i::mdl_render , nullptr , ctx , state , info , bone_to_world );

		return;
	}

	auto e = cs::i::ent_list->get< player_t* >( info.m_entity_index );

	auto mdl_name = cs::i::mdl_info->mdl_name ( info.m_model );
	auto is_player = strstr ( mdl_name, _ ( "models/player" ) );
	auto is_weapon = strstr ( mdl_name, _ ( "weapons/v_" ) ) && !strstr ( mdl_name, _ ( "arms" ) );
	auto is_arms = strstr ( mdl_name, _ ( "arms" ) );
	auto is_sleeve = strstr ( mdl_name, _ ( "sleeve" ) );

	visual_config_t visuals;
	if ( !get_visuals( e , visuals ) && !features::chams::in_model ) {
		cs::i::render_view->set_color( 255 , 255 , 255 );
		cs::i::render_view->set_alpha( 255 );
		hooks::old::draw_model_execute( cs::i::mdl_render , nullptr , ctx , state , info , bone_to_world );
		return;
	}

	static auto& enable_blend = options::vars[ _( "visuals.other.blend" ) ].val.b;
	static auto& blend_opacity = options::vars[ _( "visuals.other.blend_opacity" ) ].val.f;

	update_mats( visuals , visuals.rimlight_color );

	if ( /*is_arms || */e || features::chams::in_model ) {
		if ( is_arms ) {
			hooks::old::draw_model_execute( cs::i::mdl_render , nullptr , ctx , state , info , bone_to_world );
		}
		else if ( ( is_player && e->alive ( ) ) || features::chams::in_model ) {
			/* hit matrix chams */
			if ( features::chams::in_model ) {
				if ( visuals.hit_matrix ) {
					bool found = false;
					auto envmap = m_mat_glow->find_var ( _ ( "$envmaptint" ), &found );
					set_vec ( envmap, cur_hit_rec->m_color [ 0 ], cur_hit_rec->m_color [ 1 ], cur_hit_rec->m_color [ 2 ] );

					auto rimlight_exponent = m_mat_glow->find_var ( _ ( "$envmapfresnelminmaxexp" ), &found );
					set_vec ( rimlight_exponent, 0.0f, 1.0f, 1.0f + ( 14.0f - cur_hit_rec->m_color [ 3 ] * 14.0f ) );

					cs::i::render_view->set_alpha ( 255 );
					cs::i::render_view->set_color ( 255, 255, 255 );
					// mat->set_material_var_flag( 268435456, false );
					m_mat_glow->set_material_var_flag ( 0x8000, visuals.chams_xqz );
					m_mat_glow->set_material_var_flag ( 0x1000, visuals.chams_flat );
					cs::i::mdl_render->force_mat ( m_mat_glow );
					hooks::old::draw_model_execute ( cs::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
				}
			}
			else {
				std::array< matrix3x4_t , 128> matrix {};

				if ( visuals.backtrack_chams && e != g::local && g::local->alive( ) && anims::get_lagcomp_bones( e , matrix ) ) {
					cs::i::render_view->set_alpha( visuals.backtrack_chams_color.a * 255.0f );
					cs::i::render_view->set_color( visuals.backtrack_chams_color.r * 255.0f , visuals.backtrack_chams_color.g * 255.0f , visuals.backtrack_chams_color.b * 255.0f );

					auto mat = visuals.chams_flat ? m_matflat : m_mat;
					mat->set_material_var_flag( 0x8000 , visuals.chams_xqz );
					mat->set_material_var_flag( 0x1000 , visuals.chams_flat );

					cs::i::mdl_render->force_mat( mat );
					hooks::old::draw_model_execute( cs::i::mdl_render , nullptr , ctx , state , info , matrix.data( ) );

					cs::i::mdl_render->force_mat( nullptr );
				}

				/* fake chams */
				if ( e == g::local ) {
					auto backup_matrix = anims::fake_matrix;

					for ( auto& iter : backup_matrix )
						iter.set_origin( iter.origin( ) + ( visuals.desync_chams_fakelag ? old_origin : info.m_origin ) );

					if ( visuals.desync_chams ) {
						cs::i::render_view->set_alpha( visuals.desync_chams_color.a * 255.0f );
						cs::i::render_view->set_color( visuals.desync_chams_color.r * 255.0f , visuals.desync_chams_color.g * 255.0f , visuals.desync_chams_color.b * 255.0f );
						auto mat = visuals.chams_flat ? m_matflat : m_mat;
						mat->set_material_var_flag( 0x8000 , visuals.chams_xqz );
						mat->set_material_var_flag( 0x1000 , visuals.chams_flat );
						cs::i::mdl_render->force_mat( mat );
						hooks::old::draw_model_execute( cs::i::mdl_render , nullptr , ctx , state , info , backup_matrix.data( ) );
						cs::i::mdl_render->force_mat( nullptr );
					}

					if ( visuals.desync_chams_rimlight ) {
						update_mats ( visuals, visuals.desync_rimlight_color );
						cs::i::render_view->set_alpha ( 255 );
						cs::i::render_view->set_color ( 255, 255, 255 );
						m_mat_glow->set_material_var_flag ( 0x8000, visuals.chams_xqz );
						m_mat_glow->set_material_var_flag ( 0x1000, visuals.chams_flat );
						cs::i::mdl_render->force_mat ( m_mat_glow );
						hooks::old::draw_model_execute ( cs::i::mdl_render, nullptr, ctx, state, info, backup_matrix.data ( ) );
						cs::i::mdl_render->force_mat ( nullptr );
					}
				}

				if ( visuals.chams ) {
					/* occluded */
					if ( e == g::local ? false : visuals.chams_xqz ) {
						cs::i::render_view->set_alpha ( visuals.chams_xqz_color.a * 255.0f );
						cs::i::render_view->set_color ( visuals.chams_xqz_color.r * 255.0f, visuals.chams_xqz_color.g * 255.0f, visuals.chams_xqz_color.b * 255.0f );
						auto mat = visuals.chams_flat ? m_matflat : m_mat;
						// mat->set_material_var_flag( 268435456, false );
						mat->set_material_var_flag ( 0x8000, true );
						mat->set_material_var_flag ( 0x1000, visuals.chams_flat );
						cs::i::mdl_render->force_mat ( mat );
						hooks::old::draw_model_execute ( cs::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
						cs::i::mdl_render->force_mat ( nullptr );
					}

					/* normal chams */
					auto mat = visuals.chams_flat ? m_matflat : m_mat;
					// mat->set_material_var_flag( 268435456, false );
					mat->set_material_var_flag( 0x8000 , false );
					mat->set_material_var_flag( 0x1000 , visuals.chams_flat );

					if ( ( g::local && e == g::local ) && g::local->scoped( ) && enable_blend )
						cs::i::render_view->set_alpha( blend_opacity );
					else
						cs::i::render_view->set_alpha( visuals.chams_color.a * 255.0f );

					cs::i::render_view->set_color( visuals.chams_color.r * 255.0f , visuals.chams_color.g * 255.0f , visuals.chams_color.b * 255.0f );
					cs::i::mdl_render->force_mat( mat );

					hooks::old::draw_model_execute( cs::i::mdl_render , nullptr , ctx , state , info , bone_to_world );
					cs::i::mdl_render->force_mat( nullptr );
				}
				else {
					cs::i::render_view->set_alpha( 255 );
					cs::i::render_view->set_color( 255 , 255 , 255 );
					cs::i::mdl_render->force_mat( nullptr );

					if ( ( ( g::local && e == g::local ) && g::local->scoped( ) ) && enable_blend )
						cs::i::render_view->set_alpha( blend_opacity );

					hooks::old::draw_model_execute( cs::i::mdl_render , nullptr , ctx , state , info , bone_to_world );
				}

				/* glow overlay */
				if ( visuals.rimlight_overlay ) {
					update_mats ( visuals, visuals.rimlight_color );
					cs::i::render_view->set_alpha ( 255 );
					cs::i::render_view->set_color ( 255, 255, 255 );
					// mat->set_material_var_flag( 268435456, false );
					m_mat_glow->set_material_var_flag ( 0x8000, visuals.chams_xqz );
					m_mat_glow->set_material_var_flag ( 0x1000, visuals.chams_flat );
					cs::i::mdl_render->force_mat ( m_mat_glow );
					hooks::old::draw_model_execute ( cs::i::mdl_render, nullptr, ctx, state, info, bone_to_world );
					cs::i::mdl_render->force_mat ( nullptr );
				}
			}
		}
		else {
			cs::i::render_view->set_alpha( 255 );
			cs::i::render_view->set_color( 255 , 255 , 255 );
			hooks::old::draw_model_execute( cs::i::mdl_render , nullptr , ctx , state , info , bone_to_world );
		}
	}
	else {
		cs::i::render_view->set_alpha( 255 );
		cs::i::render_view->set_color( 255 , 255 , 255 );
		hooks::old::draw_model_execute( cs::i::mdl_render , nullptr , ctx , state , info , bone_to_world );
	}

	cs::i::render_view->set_alpha( 255 );
	cs::i::render_view->set_color( 255 , 255 , 255 );
	cs::i::mdl_render->force_mat( nullptr );
}