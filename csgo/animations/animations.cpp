#include <array>
#include "animations.hpp"
#include "../globals.hpp"
#include "../hooks.hpp"

namespace local_data {
	int old_tick = 0;
	float abs = 0.0f;
	std::array< float, 24 > poses { };
	std::array< animlayer_t, 15 > overlays { };

	namespace fake {
		animstate_t* fake_state = nullptr;
		bool should_reset = true;
		float spawn_time = 0.0f;
		std::array< matrix3x4_t, 128 > simulated_mat { };
	}
}

float animations::rebuilt::poses::jump_fall( player_t* pl, float air_time ) {
	auto airtime = ( air_time - 0.72f ) * 1.25f;
	auto clamped = airtime >= 0.0f ? std::min< float >( airtime, 1.0f ) : 0.0f;
	auto jump_fall = ( 3.0f - ( clamped + clamped ) ) * ( clamped * clamped );

	if ( jump_fall >= 0.0f )
		jump_fall = std::min< float >( jump_fall, 1.0f );

	return jump_fall;
}

float animations::rebuilt::poses::body_pitch( player_t* pl, float pitch ) {
	return ( csgo::normalize( pitch ) + 90.0f ) / 180.0f;
}

float animations::rebuilt::poses::body_yaw( player_t* pl, float yaw ) {
	return csgo::normalize( yaw );
}

std::array< matrix3x4_t, 128 >& animations::fake::matrix( ) {
	return local_data::fake::simulated_mat;
}

void animations::fake::simulate( ) {
	if ( !g::local || !g::local->alive( ) ) {
		local_data::fake::should_reset = true;
		return;
	}

	if ( local_data::fake::should_reset || local_data::fake::spawn_time != g::local->spawn_time( ) || !local_data::fake::fake_state ) {
		if ( local_data::fake::fake_state ) {
			local_data::fake::fake_state->reset( );
		}
		else {
			local_data::fake::fake_state = reinterpret_cast< animstate_t* >( csgo::i::mem_alloc->alloc( sizeof animstate_t ) );
			g::local->create_animstate( local_data::fake::fake_state );
		}

		local_data::fake::should_reset = false;
	}

	if ( local_data::fake::fake_state && g::local->simtime( ) != g::local->old_simtime( ) ) {
		const auto backup_overlays = g::local->overlays( );
		const auto backup_poses = g::local->poses( );
		const auto backup_abs_angles = g::local->abs_angles( );

		local_data::fake::fake_state->update( g::ucmd->m_angs );
		g::local->setup_bones( local_data::fake::simulated_mat, 128, 0x7ff00, csgo::i::globals->m_curtime );

		const auto render_origin = g::local->render_origin( );

		for ( auto& mat : local_data::fake::simulated_mat )
			mat.set_origin( mat.origin( ) - render_origin );

		g::local->set_abs_angles( backup_abs_angles );
		g::local->poses( ) = backup_poses;
		g::local->overlays( ) = backup_overlays;
	}
}

void animations::fix_matrix_construction( struct animating* a ) {
	/* skip call to AccumulateLayers */
	*reinterpret_cast< int* >( uintptr_t( a ) + 0xa20 ) = 0xa;
	/* previous bone mask && current bone mask (make sure attatchment helper gets called) */
	*reinterpret_cast< int* >( uintptr_t( a ) + 0x2698 ) = 0;
	*reinterpret_cast< int* >( uintptr_t( a ) + 0x26ac ) = 0;

	/* always run bone setup */
	*reinterpret_cast< int* >( uintptr_t( a ) + 0xe4 ) |= 8;
	/* disable matrix interpolation (if not local) */
	*reinterpret_cast< int* >( uintptr_t( a ) + 0xe8 ) |= 8;

	/* don't use prediction seed as seed for SetupBones */
	*reinterpret_cast< bool* >( uintptr_t( a ) + 0x2ea ) = false;
}

/* make sure to backup player index & frametime! */
void animations::force_animation_skip( struct animating* a, bool skip_anim_frame ) {
	const auto pl = reinterpret_cast< player_t* >( uintptr_t( a ) - 4 );

	if ( skip_anim_frame ) {
		/* force C_CSPlayer::ShouldSkipAnimationFrame to return false, forcing game to rebuild bone matrix every time
		*
		*	C_BaseAnimating::SetupBone
		*
		*	if ( C_CSPlayer::ShouldSkipAnimationFrame( pl ) )
		*		; copy old matricies
		*	else
		*		; build new bone matrix
		*
		*	C_CSPlayer::ShouldSkipAnimationFrame
		*
		*	v3 = *( _DWORD* )( v1 + 0xA68 );
		*	v4 = globals->m_framecount;
		*	if ( v3 && abs( v4 - v3 ) < 2 ) <--- pass this check by setting animating + 0xa64 to current framecount
		*/
		*reinterpret_cast< int* >( uintptr_t( pl ) + 0xa68 ) = csgo::i::globals->m_framecount;

		/*
		*	v5 = globals->m_frametime;
		*	if ( v5 < 0.0033333334 ) <--- pass this check by setting frametime lower (if needed)
		*/
		if ( csgo::i::globals->m_frametime >= 0.0033333334f )
			csgo::i::globals->m_frametime = 0.0033333f;

		/*
		*	if ( ( ( pl->get_index( ) % 3 + globals->m_framecount ) % 3 )
		*		return 1;
		*
		*	goto LABEL_12;
		*	...
		*	LABEL_12:
		*	return 0;
		*
		*	credits to Fetus#2465
		*/
		*reinterpret_cast< int* >( uintptr_t( pl ) + 0x64 ) = 2 - ( csgo::i::globals->m_framecount % 3 );

		return;
	}

	/*
	*	never skip animation frames when last_animation_framecount == 0; C_CSPlayer::ShouldSkipAnimationFrame will return false forcing client to rebuild matrix
	*	if ( pl->last_animation_framecount( ) && abs( globals->m_framecount - pl->last_animation_framecount( ) ) < 2 )
	*/
	*reinterpret_cast< int* >( uintptr_t( pl ) + 0xa68 ) = 0;
}

bool animations::build_matrix( player_t* pl ) {
	/* backup necessary information */
	const auto backup_frametime = csgo::i::globals->m_frametime;
	const auto backup_index = *reinterpret_cast< int* >( uintptr_t( pl ) + 0x64 );

	/* most of the stuff is located in animating class */
	const auto animating = reinterpret_cast< struct animating* >( uintptr_t( pl ) + 4 );

	/* never skip updating bone matrix */
	force_animation_skip( animating, false );

	/* fix matrix construction */
	fix_matrix_construction( animating );

	/* overwrite old matrix */
	pl->inval_bone_cache( );

	/* build bones */
	const auto ret = hooks::setupbones( animating, nullptr, nullptr, 128, 0x7ff00, csgo::i::globals->m_curtime );

	/* restore original information */
	csgo::i::globals->m_frametime = backup_frametime;
	*reinterpret_cast< int* >( uintptr_t( pl ) + 0x64 ) = backup_index;

	return ret;
}

void animations::fix_local( ) {
	if ( !g::local->valid( ) || !g::local->animstate( ) )
		return;

	const auto state = g::local->animstate( );

	if ( local_data::old_tick != csgo::i::globals->m_tickcount ) {
		local_data::old_tick = csgo::i::globals->m_tickcount;

		local_data::overlays = g::local->overlays( );

		state->m_force_update = true;
		state->m_last_clientside_anim_framecount = 0;

		g::local->animate( ) = true;
		g::local->update( );

		state->m_jump_fall_pose.set_value( g::local, rebuilt::poses::jump_fall( g::local, g::local->overlays( ) [ 4 ].m_cycle ) );
		state->m_body_pitch_pose.set_value( g::local, rebuilt::poses::body_pitch( g::local, g::ucmd->m_angs.x ) );

		local_data::poses [ 12 ] = g::local->poses( ) [ 12 ];

		if ( g::send_packet ) {
			local_data::abs = state->m_abs_yaw;
			local_data::poses = g::local->poses( );
		}
	}

	g::local->animate( ) = false;

	// *reinterpret_cast< float* >( uintptr_t( state ) + 0x124 ) = 0.0f;

	g::local->set_abs_angles( vec3_t( 0.0f, local_data::abs, 0.0f ) );

	g::local->overlays( ) = local_data::overlays;
	g::local->poses( ) = local_data::poses;
}

void animations::fix_pl( player_t* pl ) {
	if ( !pl->valid( ) || !pl->animstate( ) )
		return;

	const auto state = pl->animstate( );

	state->m_jump_fall_pose.set_value( pl, rebuilt::poses::jump_fall( pl, pl->overlays( ) [ 4 ].m_cycle ) );

	//build_matrix( pl );
}

void animations::run( int stage ) {
	switch ( stage ) {
	case 5: /* fix local anims */ {
		// animations::fake::simulate( );
		// fix_local( );
		
		/*
		auto util_playerbyindex = [ ] ( int idx ) {
			using player_by_index_fn = player_t * ( __fastcall* )( int );
			static auto fn = pattern::search( "server.dll", "85 C9 7E 2A A1" ).get< player_by_index_fn >( );
			return fn( idx );
		};

		static auto draw_hitboxes = pattern::search( "server.dll", "55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE" ).get< std::uintptr_t >( );

		const auto dur = -1.f;

		for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
			auto e_local = csgo::i::ent_list->get< player_t* >( i );

			if ( !e_local )
				continue;

			auto e = util_playerbyindex( e_local->idx( ) );

			if ( !e )
				continue;

			__asm {
				pushad
				movss xmm1, dur
				push 0
				mov ecx, e
				call draw_hitboxes
				popad
			}
		}
		*/
	} break;
	case 4: /* fix other player anims */ {
		csgo::for_each_player( [ ] ( player_t* pl ) {
			if ( pl == g::local ) {
				fix_local( );
				return;
			}

			const auto var_map = reinterpret_cast< std::uintptr_t >( pl ) + 36;

			for ( auto index = 0; index < *reinterpret_cast< int* >( var_map + 20 ); index++ )
				*reinterpret_cast< std::uintptr_t* >( *reinterpret_cast< std::uintptr_t* >( var_map ) + index * 12 ) = 0;

			fix_pl( pl );
		} );
	} break;
	case 6: /* store bone matricies */ {
	} break;
	}
}