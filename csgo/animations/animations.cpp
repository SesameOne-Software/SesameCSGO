#include <array>
#include "animations.hpp"
#include "../globals.hpp"
#include "../hooks.hpp"
#include "../security/security_handler.hpp"

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
	const float airtime = ( air_time - 0.72f ) * 1.25f;
	const float clamped = airtime >= float( N( 0 ) ) ? std::min< float >( airtime, float( N( 1 ) ) ) : float( N( 0 ) );
	float jump_fall = ( float( N( 3 ) ) - ( clamped + clamped ) ) * ( clamped * clamped );

	if ( jump_fall >= float( N( 0 ) ) )
		jump_fall = std::min< float >( jump_fall, float( N( 1 ) ) );

	return jump_fall;
}

float animations::rebuilt::poses::body_pitch( player_t* pl, float pitch ) {
	return ( csgo::normalize( pitch ) + float( N( 90 ) ) ) / float( N( 180 ) );
}

float animations::rebuilt::poses::body_yaw( player_t* pl, float yaw ) {
	return csgo::normalize( yaw );
}

std::array< matrix3x4_t, 128 >& animations::fake::matrix( ) {
	return local_data::fake::simulated_mat;
}

int animations::fake::simulate( ) {
	OBF_BEGIN

	IF ( !g::local || !g::local->alive( ) )
		local_data::fake::should_reset = true;
		RETURN( N( 0 ) )
	ENDIF

	IF ( local_data::fake::should_reset || local_data::fake::spawn_time != g::local->spawn_time( ) || !local_data::fake::fake_state )
		IF ( local_data::fake::fake_state )
			local_data::fake::fake_state->reset( );
		ELSE
			local_data::fake::fake_state = reinterpret_cast< animstate_t* >( csgo::i::mem_alloc->alloc( N( sizeof animstate_t ) ) );
			g::local->create_animstate( local_data::fake::fake_state );
		ENDIF

		local_data::fake::should_reset = false;
	ENDIF

	IF ( local_data::fake::fake_state && g::local->simtime( ) != g::local->old_simtime( ) )
		const auto backup_overlays = g::local->overlays( );
		const auto backup_poses = g::local->poses( );
		const auto backup_abs_angles = g::local->abs_angles( );

		local_data::fake::fake_state->update( g::ucmd->m_angs );
		g::local->setup_bones( local_data::fake::simulated_mat, N( 128 ), N( 0x7ff00 ), csgo::i::globals->m_curtime );

		const auto render_origin = g::local->render_origin( );

		auto i = N( 0 );

		FOR( i = N( 0 ), i < N( 128 ), i++ )
			local_data::fake::simulated_mat [ i ].set_origin( local_data::fake::simulated_mat [ i ].origin( ) - render_origin );
		ENDFOR

		g::local->set_abs_angles( backup_abs_angles );
		g::local->poses( ) = backup_poses;
		g::local->overlays( ) = backup_overlays;
	ENDIF

	OBF_END
}

int animations::fix_matrix_construction( uintptr_t a ) {
	OBF_BEGIN

	/* skip call to AccumulateLayers */
	*reinterpret_cast< int* >( V( a ) + N( 0xa20 ) ) = N( 0xa );
	/* previous bone mask && current bone mask (make sure attatchment helper gets called) */
	*reinterpret_cast< int* >( V( a ) + N( 0x2698 ) ) = N( 0 );
	*reinterpret_cast< int* >( V( a ) + N( 0x26ac ) ) = N( 0 );

	/* always run bone setup */
	*reinterpret_cast< int* >( V( a ) + N( 0xe4 ) ) |= N( 8 );
	/* disable matrix interpolation (if not local) */
	*reinterpret_cast< int* >( V( a ) + N( 0xe8 ) ) |= N( 8 );

	/* don't use prediction seed as seed for SetupBones */
	*reinterpret_cast< bool* >( V( a ) + N( 0x2ea ) ) = false;

	OBF_END
}

/* make sure to backup player index & frametime! */
int animations::force_animation_skip( uintptr_t a, bool skip_anim_frame ) {
	const uintptr_t pl = V( a ) - N( 4 );

	OBF_BEGIN
	IF ( skip_anim_frame )
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
		*reinterpret_cast< int* >( V( pl ) + N( 0xa68 ) ) = csgo::i::globals->m_framecount;

		/*
		*	v5 = globals->m_frametime;
		*	if ( v5 < 0.0033333334 ) <--- pass this check by setting frametime lower (if needed)
		*/
		IF ( csgo::i::globals->m_frametime >= 0.0033333334f )
			csgo::i::globals->m_frametime = 0.0033333f;
		ENDIF

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
		*reinterpret_cast< int* >( V( pl ) + N( 0x64 ) ) = N( 2 ) - ( csgo::i::globals->m_framecount % N( 3 ) );

		RETURN( 0 )
	ENDIF

	/*
	*	never skip animation frames when last_animation_framecount == 0; C_CSPlayer::ShouldSkipAnimationFrame will return false forcing client to rebuild matrix
	*	if ( pl->last_animation_framecount( ) && abs( globals->m_framecount - pl->last_animation_framecount( ) ) < 2 )
	*/
	*reinterpret_cast< int* >( V( pl ) + N( 0xa68 ) ) = N( 0 );
	OBF_END
}

bool animations::build_matrix( player_t* pl ) {
	OBF_BEGIN

	/* backup necessary information */
	const auto backup_frametime = csgo::i::globals->m_frametime;
	const auto backup_index = *reinterpret_cast< int* >( uintptr_t( pl ) + N( 0x64 ) );

	/* most of the stuff is located in animating class */
	const uintptr_t animating = uintptr_t( pl ) + N( 4 );

	/* never skip updating bone matrix */
	force_animation_skip( animating, false );

	/* fix matrix construction */
	fix_matrix_construction( animating );

	/* overwrite old matrix */
	pl->inval_bone_cache( );

	/* build bones */
	const auto ret = hooks::setupbones( (void*) animating, nullptr, nullptr, N( 128 ), N( 0x7ff00 ), csgo::i::globals->m_curtime );

	/* restore original information */
	csgo::i::globals->m_frametime = backup_frametime;
	*reinterpret_cast< int* >( uintptr_t( pl ) + N( 0x64 ) ) = backup_index;

	RETURN( ret)

	OBF_END
}

int animations::fix_local( ) {
	OBF_BEGIN

	IF ( !g::local->valid( ) || !g::local->animstate( ) )
		RETURN( N( 0 ) )
	ENDIF

	const auto state = g::local->animstate( );

	IF ( local_data::old_tick != csgo::i::globals->m_tickcount )
		local_data::old_tick = csgo::i::globals->m_tickcount;

		local_data::overlays = g::local->overlays( );

		state->m_force_update = true;
		state->m_last_clientside_anim_framecount = N( 0 );

		g::local->animate( ) = true;
		g::local->update( );

		state->m_jump_fall_pose.set_value( g::local, rebuilt::poses::jump_fall( g::local, g::local->overlays( ) [ N( 4 ) ].m_cycle ) );
		state->m_body_pitch_pose.set_value( g::local, rebuilt::poses::body_pitch( g::local, g::ucmd->m_angs.x ) );

		local_data::poses [ N( 12 ) ] = g::local->poses( ) [ N( 12 ) ];

		IF ( g::send_packet )
			local_data::abs = state->m_abs_yaw;
			local_data::poses = g::local->poses( );
		ENDIF
	ENDIF

	g::local->animate( ) = false;

	// *reinterpret_cast< float* >( uintptr_t( state ) + 0x124 ) = 0.0f;

	g::local->set_abs_angles( vec3_t( 0.0f, local_data::abs, 0.0f ) );

	g::local->overlays( ) = local_data::overlays;
	g::local->poses( ) = local_data::poses;

	OBF_END
}

int animations::fix_pl( player_t* pl ) {
	OBF_BEGIN

	IF ( !pl->valid( ) || !pl->animstate( ) )
		RETURN( N( 0 ) )
	ENDIF

	const auto state = pl->animstate( );

	state->m_jump_fall_pose.set_value( pl, rebuilt::poses::jump_fall( pl, pl->overlays( ) [ N( 4 ) ].m_cycle ) );

	//build_matrix( pl );

	OBF_END
}

int animations::run( int stage ) {
	OBF_BEGIN

	CASE ( stage )
	WHEN(N( 5 )) DO /* fix local anims */
		// animations::fake::simulate( );
		fix_local( );
		
		/*
		auto util_playerbyindex = [ ] ( int idx ) {
			using player_by_index_fn = player_t * ( __fastcall* )( int );
			static auto fn = pattern::search( _("server.dll"), _("85 C9 7E 2A A1" )).get< player_by_index_fn >( );
			return fn( idx );
		};

		static auto draw_hitboxes = pattern::search( _("server.dll"), _("55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE" )).get< std::uintptr_t >( );

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
		BREAK;
		DONE
	WHEN( N( 4 ) ) DO
		csgo::for_each_player( [ ] ( player_t* pl ) -> int {
			OBF_BEGIN

			IF ( pl == g::local )
				RETURN(N(0))
			ENDIF

			const auto var_map = reinterpret_cast< std::uintptr_t >( pl ) + N( 36 );
			auto index = N(0);

			FOR ( index = N(0), index < *reinterpret_cast< int* >( var_map + N( 20 ) ), index++ )
				*reinterpret_cast< std::uintptr_t* >( *reinterpret_cast< std::uintptr_t* >( var_map ) + index * N( 12 ) ) = 0;
			ENDFOR

			fix_pl( pl );

			OBF_END
		} );
		BREAK;
		DONE
		WHEN( N( 6 ) ) DO
			BREAK;
		DONE
		DEFAULT
		DONE
	ENDCASE

	OBF_END
}