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
		animstate_t fake_state;
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

float animations::rebuilt::poses::lean_yaw( player_t* pl, float yaw ) {
	return csgo::normalize( pl->animstate( )->m_abs_yaw - yaw );
}

void animations::rebuilt::poses::calculate( player_t* pl ) {
	auto state = pl->animstate( );

	if ( !state )
		return;

	state->m_jump_fall_pose.set_value( pl, rebuilt::poses::jump_fall( pl, pl->overlays( ) [ N( 4 ) ].m_cycle ) );
	state->m_body_pitch_pose.set_value( pl, rebuilt::poses::body_pitch( pl, pl->angles( ).x ) );
}

std::array< matrix3x4_t, 128 >& animations::fake::matrix( ) {
	return local_data::fake::simulated_mat;
}

int animations::fake::simulate( ) {
	OBF_BEGIN

	IF( !g::local || !g::local->alive( ) )
		local_data::fake::should_reset = true;
		RETURN( N( 0 ) )
	ENDIF

	IF( local_data::fake::should_reset || local_data::fake::spawn_time != g::local->spawn_time( ) )
		local_data::fake::spawn_time = g::local->spawn_time( );
		g::local->create_animstate( &local_data::fake::fake_state );
		local_data::fake::should_reset = false;
	ENDIF

	IF( g::local->simtime( ) != g::local->old_simtime( ) )
		const auto backup_overlays = g::local->overlays( );
		const auto backup_poses = g::local->poses( );
		auto backup_abs_angles = g::local->abs_angles( );
		auto backup_abs_origin = g::local->abs_origin( );

		local_data::fake::fake_state.update( g::sent_cmd.m_angs );
		build_matrix( g::local, reinterpret_cast< matrix3x4_t* >( &local_data::fake::simulated_mat ), N( 128 ), N( 256 ), csgo::i::globals->m_curtime );

		const auto render_origin = g::local->render_origin( );

		auto i = N( 0 );

		FOR( i = N( 0 ), i < N( 128 ), i++ )
			local_data::fake::simulated_mat [ i ].set_origin( local_data::fake::simulated_mat [ i ].origin( ) - render_origin );
		ENDFOR

		g::local->set_abs_angles( backup_abs_angles );
		g::local->set_abs_origin( backup_abs_origin );
		g::local->poses( ) = backup_poses;
		g::local->overlays( ) = backup_overlays;
	ENDIF

	OBF_END
}

bool animations::build_matrix( player_t* pl, matrix3x4_t* out, int max_bones, int mask, float seed ) {
	OBF_BEGIN

	const auto backup_mask_1 = *reinterpret_cast< int* >( uintptr_t( pl ) + N( 0x269C ) );
	const auto backup_mask_2 = *reinterpret_cast< int* >( uintptr_t( pl ) + N( 0x26B0 ) );
	const auto backup_flags = *reinterpret_cast< int* >( uintptr_t( pl ) + N( 0xe8 ) );
	const auto backup_effects = *reinterpret_cast< int* >( uintptr_t( pl ) + N( 0xf0 ) );
	const auto backup_use_pred_time = *reinterpret_cast< int* >( uintptr_t( pl ) + N( 0x2ee ) );
	auto backup_abs_origin = pl->abs_origin( );

	*reinterpret_cast< int* >( uintptr_t( pl ) + N( 0x269C ) ) = N( 0 );
	*reinterpret_cast< int* >( uintptr_t( pl ) + N( 0x26B0 ) ) = N( 0 );
	*reinterpret_cast< int* >( uintptr_t( pl ) + N( 0xe8 ) ) |= N( 8 );

	/* disable matrix interpolation */
	*reinterpret_cast< int* >( uintptr_t( pl ) + N( 0xf0 ) ) |= N( 8 );

	/* use our setup time */
	*reinterpret_cast< bool* >( uintptr_t( pl ) + N( 0x2ee ) ) = false;

	/* use uninterpolated origin */
	pl->set_abs_origin( pl->origin( ) );

	pl->inval_bone_cache( );
	const auto ret = hooks::setupbones( pl->renderable( ), nullptr, out, max_bones, mask, seed );

	*reinterpret_cast< int* >( uintptr_t( pl ) + N( 0x269C ) ) = backup_mask_1;
	*reinterpret_cast< int* >( uintptr_t( pl ) + N( 0x26B0 ) ) = backup_mask_2;
	*reinterpret_cast< int* >( uintptr_t( pl ) + N( 0xe8 ) ) = backup_flags;
	*reinterpret_cast< int* >( uintptr_t( pl ) + N( 0xf0 ) ) = backup_effects;
	*reinterpret_cast< int* >( uintptr_t( pl ) + N( 0x2ee ) ) = backup_use_pred_time;

	pl->set_abs_origin( backup_abs_origin );

	RETURN( ret )

	OBF_END
}

int animations::fix_local( ) {
	OBF_BEGIN

	IF( !g::local->valid( ) || !g::local->animstate( ) || !g::ucmd )
		IF ( g::local )
			data::origin [ g::local->idx( ) ] = vec3_t( FLT_MAX, FLT_MAX, FLT_MAX );
			data::old_origin [ g::local->idx( ) ] = vec3_t( FLT_MAX, FLT_MAX, FLT_MAX );
		ENDIF

		RETURN( N( 0 ) )
	ENDIF

		const auto state = g::local->animstate( );

	IF( local_data::old_tick != csgo::i::globals->m_tickcount )
		local_data::old_tick = csgo::i::globals->m_tickcount;

		local_data::overlays = g::local->overlays( );

		state->m_force_update = true;
		state->m_last_clientside_anim_framecount = csgo::i::globals->m_framecount - N( 1 );

		g::local->animate( ) = true;
		g::local->update( );

		rebuilt::poses::calculate( g::local );

		IF( g::send_packet )
			local_data::abs = state->m_abs_yaw;
			local_data::poses = g::local->poses( );

			/* store old origin for lag-comp break check */
			data::old_origin [ g::local->idx( ) ] = data::origin [ g::local->idx( ) ];
			data::origin [ g::local->idx( ) ] = g::local->origin( );
		ENDIF
	ENDIF

	g::local->animate( ) = false;

	*reinterpret_cast< float* >( uintptr_t( state ) + 0x124 ) = 0.0f;

	g::local->set_abs_angles( vec3_t( N( 0 ), local_data::abs, N( 0 ) ) );

	g::local->overlays( ) = local_data::overlays;
	g::local->poses( ) = local_data::poses;

	OBF_END
}

void animations::simulate_command( player_t* pl ) {
	/* simulate origin */
	pl->origin( ) += pl->vel( ) * csgo::ticks2time( 1 );
}

int animations::fix_pl( player_t* pl ) {
	OBF_BEGIN

	IF( !pl->valid( ) || !pl->animstate( ) )
		IF( pl )
			data::origin [ pl->idx( ) ] = vec3_t( FLT_MAX, FLT_MAX, FLT_MAX );
			data::old_origin [ pl->idx( ) ] = vec3_t( FLT_MAX, FLT_MAX, FLT_MAX );
		ENDIF

		RETURN( N( 0 ) )
	ENDIF

	const auto state = pl->animstate( );

	/* information recieved from client */
	IF( pl->simtime( ) != pl->old_simtime( ) )
		pl->animate( ) = true;
		pl->update( );
		pl->animate( ) = false;

		*reinterpret_cast< uint32_t* >( uintptr_t( pl ) + N( 0xe8 ) ) &= ~0x1000;
		*reinterpret_cast< vec3_t* >( uintptr_t( pl ) + N( 0x94 ) ) = pl->vel( );

		/* re-calculate poses */
		rebuilt::poses::calculate( pl );

		/* store old origin for lag-comp break check */
		data::old_origin [ pl->idx( ) ] = data::origin [ pl->idx( ) ];
		data::origin [ pl->idx( ) ] = pl->origin( );
	ENDIF

	OBF_END
}

int animations::run( int stage ) {
	OBF_BEGIN

		IF( !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) )
		RETURN( N( 0 ) )
		ENDIF

		switch ( stage ) {
		case 5: { /* fix local anims */
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
		} break;
		case 4: {
			csgo::for_each_player( [ ] ( player_t* pl ) -> int {
				if ( pl == g::local )
					return 0;

				const auto var_map = reinterpret_cast< std::uintptr_t >( pl ) + N( 36 );

				for ( int index = N( 0 ); index < *reinterpret_cast< int* >( var_map + N( 20 ) ); index++ )
					*reinterpret_cast< std::uintptr_t* >( *reinterpret_cast< std::uintptr_t* >( var_map ) + index * N( 12 ) ) = N( 0 );

				fix_pl( pl );
				} );
		} break;
		}

	OBF_END
}