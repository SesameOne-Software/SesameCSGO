#include "animation_system.hpp"
#include "../menu/d3d9_render.hpp"
#include "resolver.hpp"
#include "../features/ragebot.hpp"

std::array< matrix3x4_t, 128 > anims::fake_matrix { };
std::array< matrix3x4_t, 128 > anims::aim_matrix { };
std::array< std::deque< anims::animation_frame_t >, 65 > anims::frames { {{}} };
std::map< std::string, std::array< std::deque< int >, 65 > > anims::choke_sequences {
    {"air", {{}} },
    {"moving_slow", {{}} },
    {"moving", {{}} },
    {"shot", {{}} }
};

bool anims::new_tick = false;

void anims::animation_frame_t::store( player_t* ent, bool anim_update ) {
    memcpy( m_animlayers.data( ), ent->layers( ), sizeof( m_animlayers ) );
    //m_poses = ent->poses( );
    m_simtime = ent->simtime( );
    m_old_simtime = ent->old_simtime( );
    m_animstate = *ent->animstate( );
    m_flags = ent->flags( );

    m_velocity = ent->vel( );
    m_origin = ent->origin( );

    m_anim_update = anim_update;
}

#pragma optimize( "2", on )

/* https://github.com/ValveSoftware/halflife/blob/master/ricochet/pm_shared/pm_math.c#L21 */
float anims::angle_mod( float a ) {
    return ( 360.0 / 65536 ) * ( ( int )( a * ( 65536 / 360.0 ) ) & 65535 );
};

/* https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/src_main/mathlib/mathlib_base.cpp#L3327 */
float anims::approach_angle( float target, float value, float speed ) {
    target = angle_mod( target );
    value = angle_mod( value );

    float delta = target - value;

    if ( speed < 0.0f )
        speed = -speed;

    if ( delta < -180.0f )
        delta += 360.0f;
    else if ( delta > 180.0f )
        delta -= 360.0f;

    if ( delta > speed )
        value += speed;
    else if ( delta < -speed )
        value -= speed;
    else
        value = target;

    return value;
};

float anims::angle_diff( float dst, float src ) {
    auto delta = fmodf( dst - src, 360.0f );

    if ( dst > src ) {
        if ( delta >= 180.0f )
            delta -= 360.0f;
    }
    else {
        if ( delta <= -180.0f )
            delta += 360.0f;
    }

    return delta;
}

bool anims::build_bones( player_t* target, matrix3x4_t* mat, int mask, vec3_t rotation, vec3_t origin, float time ) {
    /* vfunc indices */
    static auto standard_blending_rules_vfunc_idx = pattern::search( _( "client.dll" ), _( "FF 90 ? ? ? ? 8B 47 FC" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;
    static auto build_transformations_vfunc_idx = pattern::search( _( "client.dll" ), _( "FF 90 ? ? ? ? A1 ? ? ? ? B9 ? ? ? ? 8B 40 34 FF D0 85 C0 74 41" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;
    static auto update_ik_locks_vfunc_idx = pattern::search( _( "client.dll" ), _( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;
    static auto calculate_ik_locks_vfunc_idx = pattern::search( _( "client.dll" ), _( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50 FF B7 ? ? ? ? 8D 84 24 ? ? ? ? 50 8D 84 24 ? ? ? ? 50 E8 ? ? ? ? 8B 47 FC 8D 4F FC 8B 80" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;

    /* func sigs */
    static auto init_iks = pattern::search( _( "client.dll" ), _( "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D" ) ).get< void( __thiscall* )( void*, studiohdr_t*, vec3_t&, vec3_t&, float, int, int ) >( );
    static auto update_targets = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F0 81 EC 18 01 00 00 33" ) ).get< void( __thiscall* )( void*, vec3_t*, void*, void*, uint8_t* ) >( );
    static auto solve_dependencies = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F0 81 EC 98 05 00 00 8B 81" ) ).get< void( __thiscall* )( void*, vec3_t*, void*, void*, uint8_t* ) >( );

    /* offset sigs */
    static auto iks_off = pattern::search( _( "client.dll" ), _( "8D 47 FC 8B 8F" ) ).add( 5 ).deref( ).add( 4 ).get< uint32_t >( );
    static auto effects_off = pattern::search( _( "client.dll" ), _( "75 0D 8B 87" ) ).add( 4 ).deref( ).add( 4 ).get< uint32_t >( );

    auto cstudio = *reinterpret_cast< studiohdr_t** >( uintptr_t( target ) + 0x294C );

    if ( !cstudio )
        return false;

    //  we need an aligned matrix in the bone accessor, so do this :) bad performance cause memcpy but that's ok
    matrix3x4a_t used [ 128 ];

    //	output shit
    uint8_t bone_computed [ 0x100 ] = { 0 };

    //	needs to be aligned
    matrix3x4a_t base_matrix;

    csgo::angle_matrix( rotation, origin, base_matrix );

    //	store shit
    const auto old_bones = target->bones( );

    auto iks = *reinterpret_cast< void** >( uintptr_t( target ) + iks_off );
    *reinterpret_cast< int* >( uintptr_t( target ) + effects_off ) |= 8;

    //	clear iks & re-create them
    if ( iks ) {
        if ( *( uint32_t* )( uintptr_t( iks ) + 0xFF0 ) > 0 ) {
            int v1 = 0;
            auto v62 = ( uint32_t* )( uintptr_t( iks ) + 0xD0 );

            do {
                *v62 = -9999;
                v62 += 0x55;
                ++v1;
            } while ( v1 < *( uint32_t* )( uintptr_t( iks ) + 0xFF0 ) );
        }

        init_iks( iks, cstudio, rotation, origin, time, csgo::i::globals->m_framecount, 0x1000 );
    }

    vec3_t pos [ 128 ];
    memset( pos, 0, sizeof( vec3_t [ 128 ] ) );

    void* q = malloc( /* sizeof quaternion_t */ 48 * 128 );
    memset( q, 0, /* sizeof quaternion_t */ 48 * 128 );

    //	set flags and bones
    target->bones( ) = used;

    //	build some shit
    // IDX = 205 ( 2020.7.10 )
    vfunc< void( __thiscall* )( player_t*, studiohdr_t*, vec3_t*, void*, float, uint32_t ) >( target, standard_blending_rules_vfunc_idx ) ( target, cstudio, pos, q, time, mask );

    //	set iks
    if ( iks ) {
        vfunc< void( __thiscall* )( player_t*, float ) >( target, update_ik_locks_vfunc_idx ) ( target, time );
        update_targets( iks, pos, q, target->bones( ), bone_computed );
        vfunc< void( __thiscall* )( player_t*, float ) >( target, calculate_ik_locks_vfunc_idx ) ( target, time );
        solve_dependencies( iks, pos, q, target->bones( ), bone_computed );
    }

    //	build the matrix
    // IDX = 189 ( 2020.7.10 )
    vfunc< void( __thiscall* )( player_t*, studiohdr_t*, vec3_t*, void*, matrix3x4a_t const&, uint32_t, void* ) >( target, build_transformations_vfunc_idx ) ( target, cstudio, pos, q, base_matrix, mask, bone_computed );

    free( q );

    //	restore flags and bones
    *reinterpret_cast< int* >( uintptr_t( target ) + effects_off ) &= ~8;
    target->bones( ) = old_bones;

    //  and pop out our new matrix
    memcpy( mat, used, sizeof( matrix3x4_t ) * 128 );

    return true;
}

void anims::interpolate( player_t* ent, bool should_interp ) {
    struct varmapentry_t {
        uint16_t m_type;
        uint16_t m_needstointerpolate;
        void* data;
        void* watcher;
    };

    struct varmapping_t {
        varmapentry_t* m_entries;
        int m_interpolatedentries;
        float m_lastinterptime;
    };

    auto map = reinterpret_cast< varmapping_t* >( uintptr_t( ent ) + 36 );

    if ( map ) {
        for ( auto i = 0; i < map->m_interpolatedentries; i++ )
            map->m_entries [ i ].m_needstointerpolate = should_interp;
    }
}

float anims::calc_feet_cycle ( player_t* ent ) {
	float feet_cycle_rate = 0.0f;

	if ( ent->vel ( ).length_2d ( ) > 0.0f ) {
		//    get the sequence used in the calc
		char dest [ 64 ];
		sprintf_s ( dest, "move_%s", ent->animstate ( )->get_weapon_move_animation ( ) );

		int seq = ent->lookup_sequence ( dest );
		if ( seq == -1 ) {
			char movestr [ 4 ] = { 'm', 'o', 'v', 'e' };
			seq = ent->lookup_sequence ( movestr );
		}

		//    cycle rate 
		float seqcyclerate = ent->get_sequence_cycle_rate_server ( seq );

		float seqmovedist = ent->get_sequence_move_distance ( *reinterpret_cast< studiohdr_t** >( uintptr_t ( ent ) + 0x294C ), seq );
		seqmovedist *= 1.0f / ( 1.0f / seqcyclerate );

		if ( seqmovedist <= 0.001f )
			seqmovedist = 0.001f;

		float speed_multiplier = ent->vel ( ).length_2d ( ) / seqmovedist;
		feet_cycle_rate = ( 1.0f - ( ent->animstate ( )->m_ground_fraction * 0.15f ) ) * ( speed_multiplier * seqcyclerate );
	}

	float feetcycle_playback_rate = ent->animstate()->m_last_clientside_anim_update_time_delta * feet_cycle_rate;
	float accurate_feet_cycle = ent->layers ( ) [ 6 ].m_cycle - feetcycle_playback_rate;

	return accurate_feet_cycle;
}

void anims::calc_local_exclusive( float& ground_fraction_out, float& air_time_out ) {
    constexpr auto CS_PLAYER_SPEED_DUCK_MODIFIER = 0.340f;
    constexpr auto CS_PLAYER_SPEED_WALK_MODIFIER = 0.520f;
    constexpr auto CS_PLAYER_SPEED_RUN = 260.0f;

    static float ground_fractions = 0.0f;
    static bool old_not_walking = false;
    static bool last_ground = false;

    static float old_ground_time = 0.0f;

    auto& abs_vel = *reinterpret_cast< vec3_t* >( uintptr_t( g::local ) + 0x94 );

    const auto twotickstime = g::local->animstate( )->m_last_clientside_anim_update_time_delta * 2.0f;
    const auto speed = std::fminf( abs_vel.length_2d( ), 260.0f );

    /* ground fraction */
    if ( g::local->flags( ) & 1 ) {
        if ( old_not_walking )
            ground_fractions -= twotickstime;
        else
            ground_fractions += twotickstime;

        if ( speed > ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && old_not_walking )
            old_not_walking = false;
        else if ( speed < ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && !old_not_walking )
            old_not_walking = true;
    }

    ground_fractions = std::clamp< float >( ground_fractions, 0.0f, 1.0f );
    ground_fraction_out = ground_fractions;

    /* layer 4 cycle*/
    if ( g::local->flags( ) & 1 ) {
        old_ground_time = features::prediction::curtime( );
        air_time_out = 0.0f;
    }
    else {
        air_time_out = features::prediction::curtime( ) - old_ground_time;
        ground_fraction_out = 0.0f;
    }

    if ( !last_ground && g::local->flags( ) & 1 )
        ground_fraction_out = 0.0f;

    last_ground = g::local->flags( ) & 1;
}

void anims::calc_animlayers( player_t* ent ) {
    auto state = ent->animstate( );
    auto layers = ent->layers( );

    if ( !state || !layers )
        return;

	if ( ent == g::local ) {
		//calc_feet_cycle ( ent );
		//dbg_print ( "feet_cycle: %.3f\n", calc_feet_cycle ( ent ) );
	}

    layers [ 12 ].m_weight = 0.0f;
}

void anims::calc_poses( player_t* ent ) {
    static auto jump_fall = [ ] ( float air_time ) {
        const float recalc_air_time = ( air_time - 0.72f ) * 1.25f;
        const float clamped = recalc_air_time >= 0.0f ? std::min< float >( recalc_air_time, 1.0f ) : 0.0f;
        float out = ( 3.0f - ( clamped + clamped ) ) * ( clamped * clamped );

        if ( out >= 0.0f )
            out = std::min< float >( out, 1.0f );

        return out;
    };

    auto state = ent->animstate( );
    auto layers = ent->layers( );

    if ( !state || !layers )
        return;

    state->m_jump_fall_pose.set_value( ent, jump_fall( layers [ 4 ].m_cycle ) );

    state->m_move_blend_walk_pose.set_value( ent, ( 1.0f - state->m_ground_fraction ) * ( 1.0f - state->m_duck_amount ) );
    state->m_move_blend_run_pose.set_value( ent, ( 1.0f - state->m_duck_amount ) * state->m_ground_fraction );
    state->m_move_blend_crouch_pose.set_value( ent, state->m_duck_amount );
    // TODO: FIX THIS MOVE YAW
    auto move_yaw = csgo::normalize( angle_diff( csgo::normalize( csgo::vec_angle( ent->vel( ) ).y ), csgo::normalize( state->m_abs_yaw ) ) );
    if ( move_yaw < 0.0f )
        move_yaw += 360.0f;
    state->m_move_yaw_pose.set_value( ent, move_yaw / 360.0f );
    state->m_speed_pose.set_value( ent, ent->vel( ).length_2d( ) );
    state->m_stand_pose.set_value( ent, 1.0f - ( state->m_duck_amount * state->m_unk_fraction ) );

    auto lean_yaw = csgo::normalize( state->m_eye_yaw + 180.0f );
    if ( lean_yaw < 0.0f )
        lean_yaw += 360.0f;

    state->m_body_pitch_pose.set_value( ent, ( csgo::normalize( state->m_pitch ) + 90.0f ) / 180.0f );
    //state->m_lean_yaw_pose.set_value( ent, lean_yaw / 360.0f );
    state->m_body_yaw_pose.set_value( ent, std::clamp( csgo::normalize( angle_diff( csgo::normalize( state->m_eye_yaw ), csgo::normalize( state->m_abs_yaw ) ) ), -60.0f, 60.0f ) / 120.0f + 0.5f );
}

/* ghetto last minute decision: associate a certain velocity range with a fakelag factor and apply this fakelag factor when we reach a similar velocity */
int anims::predict_choke_sequence( player_t* ent ) {
    /* predicts amount of commands player will choke next */
    /* constant amounds are easier to predict, but fluctuating or other amounts are much harder to predict */
    /* this breaks our cheat, because we will predict 1 tick while they actually choke 14 */
    /* this is because we predict using the last amount of choked commands */
    /* 14 1 14 1 14 1 */
    /* if we are able to predict the amount of commands the play will choke next... for ex: */
    /* 1 2 4 8 ? */
    /* if we calculate it as 16, and amount they choke is 16 or around there, we have a very high chance to hit the enemy */

    const auto current_choked = std::clamp< int >( csgo::time2ticks( ent->simtime( ) - ent->old_simtime( ) ), 0, g::cvars::sv_maxusrcmdprocessticks->get_int() );

    static auto predict_choke = [ & ] ( const std::deque< int >& sequence ) -> int {
        //const auto entries = sequence.size( );
//
        //if ( entries < 3 )
        //    return current_choked;
//
        //const auto last_entry_1 = sequence [ entries - 1 ];
        //const auto last_entry_2 = sequence [ entries - 2 ];
        //const auto last_entry_3 = sequence [ entries - 3 ];
//
        //for ( auto i = 0; i < entries; i++ ) {
        //    if ( sequence [ i ]] )
        //}
//
        //return current_choked;

        const auto entries = sequence.size( );

        if ( !entries )
            return current_choked;

        return sequence.back( );
    };

    if ( !( ent->flags( ) & 1 ) )
        return predict_choke( choke_sequences [ _( "air" ) ][ ent->idx( ) ] );
    else if ( ent->weapon( ) && ent->weapon( )->last_shot_time( ) > ent->old_simtime( ) )
        return predict_choke( choke_sequences [ _( "shot" ) ][ ent->idx( ) ] );
    else if ( ent->weapon( ) && ent->weapon( )->data( ) && ent->vel( ).length_2d( ) > 5.0f && ent->vel( ).length_2d( ) <= ent->weapon( )->data( )->m_max_speed * 0.34f )
        return predict_choke( choke_sequences [ _( "moving_slow" ) ][ ent->idx( ) ] );
    else if ( ent->weapon( ) && ent->weapon( )->data( ) && ent->vel( ).length_2d( ) > 5.0f && ent->vel( ).length_2d( ) > ent->weapon( )->data( )->m_max_speed * 0.34f )
        return predict_choke( choke_sequences [ _( "moving" ) ][ ent->idx( ) ] );

    return current_choked;
}

void anims::update( player_t* ent ) {
    /* update viewmodel manually tox fix it dissappearing*/
    using update_all_viewmodel_addons_t = int( __fastcall* )( void* );
    static auto update_all_viewmodel_addons = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 ? 83 EC ? 53 8B D9 56 57 8B 03 FF 90 ? ? ? ? 8B F8 89 7C 24 ? 85 FF 0F 84 ? ? ? ? 8B 17 8B CF" ) ).get< update_all_viewmodel_addons_t >( );

    if ( ent->viewmodel_handle( ) != -1 && csgo::i::ent_list->get_by_handle< void* >( ent->viewmodel_handle( ) ) )
        update_all_viewmodel_addons( csgo::i::ent_list->get_by_handle< void* >( ent->viewmodel_handle( ) ) );

    /* update all animation and animation data */
    auto& abs_vel = *reinterpret_cast< vec3_t* >( uintptr_t( ent ) + 0x94 );

    const auto backup_eflags = *reinterpret_cast< uint32_t* >( uintptr_t( ent ) + 0xe8 );
    const auto backup_abs_vel = abs_vel;
    const auto backup_effects = *reinterpret_cast< uint32_t* >( uintptr_t( ent ) + 0xf0 );

    const auto backup_curtime = csgo::i::globals->m_curtime;
    const auto backup_frametime = csgo::i::globals->m_frametime;

    const auto state = ent->animstate( );

    if ( !state )
        return;

    //if ( ent != g::local )
    //    csgo::i::globals->m_curtime = ent->old_simtime( ) + csgo::ticks2time( 1 );

    csgo::i::globals->m_frametime = csgo::ticks2time( 1 );

    /* fix other players' velocity */
    //if ( ent != g::local ) {
        /* skip call to C_BaseEntity::CalcAbsoluteVelocity */
    *reinterpret_cast< uint32_t* >( uintptr_t( ent ) + 0xe8 ) &= ~0x1000;

    /* replace abs velocity (to be recalculated) */
    abs_vel = ent->vel( );
    //}

    /* fix abs velocity rounding errors */
    if ( fabsf( abs_vel.x ) < 0.001f )
        abs_vel.x = 0.0f;

    if ( fabsf( abs_vel.y ) < 0.001f )
        abs_vel.y = 0.0f;

    if ( fabsf( abs_vel.z ) < 0.001f )
        abs_vel.z = 0.0f;

    /* fix foot spin */
    state->m_feet_yaw_rate = 0.0f;
    state->m_feet_yaw = state->m_abs_yaw;

    /* force animation update */
    state->m_force_update = true;

    //state->m_on_ground = ent->flags( ) & 1;

    if ( state->m_hit_ground )
        state->m_time_in_air = 0.0f;

    /* fix on ground */
    //state->m_on_ground = g::local->flags( ) & 1;

    /* remove anim and bone interpolation */
    *reinterpret_cast< uint32_t* >( uintptr_t( ent ) + 0xf0 ) |= 8;

    state->m_last_clientside_anim_framecount = 0;

    /* fix pitch */
    if ( ent != g::local ) {
        state->m_pitch = ent->angles( ).x;
        //state->m_eye_yaw = ent->angles( ).y;
    }

    /* recalculate animlayers */
    calc_animlayers( ent );

    /* update animations */
    ent->animate( ) = true;
    ent->update( );

    /* rebuild poses */
    if ( ent != g::local )
        calc_poses( ent );

    /* stop animating */
    ent->animate( ) = false;

    /* restore original information */
    *reinterpret_cast< uint32_t* >( uintptr_t( ent ) + 0xe8 ) = backup_eflags;
    abs_vel = backup_abs_vel;
    *reinterpret_cast< uint32_t* >( uintptr_t( ent ) + 0xf0 ) = backup_effects;

    //csgo::i::globals->m_curtime = backup_curtime;
    csgo::i::globals->m_frametime = backup_frametime;
}

void anims::store_frame( player_t* ent, bool anim_update ) {
    const auto state = ent->animstate( );
    const auto animlayers = ent->layers( );

    if ( !state || !animlayers )
        return;

    /* make static, allocating too much memory on stack rn, this might be more efficient */
    static animation_frame_t animation_frame;

    animation_frame.store( ent, anim_update );

    float abs_yaw1 = 0.0f, abs_yaw2 = 0.0f, abs_yaw3 = 0.0f;
    animations::resolver::resolve( ent, abs_yaw1, abs_yaw2, abs_yaw3 );

    state->m_feet_yaw = state->m_abs_yaw = abs_yaw1;
    calc_poses( ent );
    memcpy( animation_frame.m_poses1.data( ), ent->poses( ).data( ), sizeof( ent->poses( ) ) );
    build_bones( ent, animation_frame.m_matrix1.data( ), 256, vec3_t( 0.0f, abs_yaw1, 0.0f ), ent->origin( ), ent->simtime( ) );

    state->m_feet_yaw = state->m_abs_yaw = abs_yaw2;
    calc_poses( ent );
    memcpy( animation_frame.m_poses2.data( ), ent->poses( ).data( ), sizeof( ent->poses( ) ) );
    build_bones( ent, animation_frame.m_matrix2.data( ), 256, vec3_t( 0.0f, abs_yaw2, 0.0f ), ent->origin( ), ent->simtime( ) );

    state->m_feet_yaw = state->m_abs_yaw = abs_yaw3;
    calc_poses( ent );
    memcpy( animation_frame.m_poses3.data( ), ent->poses( ).data( ), sizeof( ent->poses( ) ) );
    build_bones( ent, animation_frame.m_matrix3.data( ), 256, vec3_t( 0.0f, abs_yaw3, 0.0f ), ent->origin( ), ent->simtime( ) );

    frames [ ent->idx( ) ].push_back( animation_frame );
}

void anims::animate_player( player_t* ent ) {
    const auto state = ent->animstate( );
    const auto animlayers = ent->layers( );

    static std::array< float, 65 > spawn_times { 0.0f };
    static std::array< float, 65 > last_update { 0.0f };
    static std::array< vec3_t, 65 > last_origin { vec3_t( ) };
    static std::array< vec3_t, 65 > old_origin { vec3_t( ) };
    static std::array< vec3_t, 65 > last_velocity { vec3_t( ) };
    static std::array< vec3_t, 65 > old_velocity { vec3_t( ) };

    if ( !state || !animlayers || fabsf( ent->simtime( ) - last_update [ ent->idx( ) ] ) > 1.0f ) {
        last_update [ ent->idx( ) ] = ent->simtime( );
        last_origin [ ent->idx( ) ] = ent->origin( );
        old_origin [ ent->idx( ) ] = ent->origin( );
        last_velocity [ ent->idx( ) ] = ent->vel( );
        old_velocity [ ent->idx( ) ] = ent->vel( );
        frames [ ent->idx( ) ].clear( );
        //spawn_times [ ent->idx( ) ] = ent->spawn_time( );
        return;
    }

    const auto choked = std::clamp< int >( csgo::time2ticks( ent->simtime( ) - ent->old_simtime( ) ), 0, g::cvars::sv_maxusrcmdprocessticks->get_int() );

    if ( ent->simtime( ) > last_update [ ent->idx( ) ] ) {
        old_origin [ ent->idx( ) ] = last_origin [ ent->idx( ) ];
        old_velocity [ ent->idx( ) ] = last_velocity [ ent->idx( ) ];

        auto& air_choke = choke_sequences [ _( "air" ) ][ ent->idx( ) ];
        auto& shot_choke = choke_sequences [ _( "shot" ) ][ ent->idx( ) ];
        auto& moving_slow_choke = choke_sequences [ _( "moving_slow" ) ][ ent->idx( ) ];
        auto& moving_choke = choke_sequences [ _( "moving" ) ][ ent->idx( ) ];

        if ( !( ent->flags( ) & 1 ) )
            air_choke.push_back( choked );
        else if ( ent->weapon( ) && ent->weapon( )->last_shot_time( ) > ent->old_simtime( ) )
            shot_choke.push_back( choked );
        else if ( ent->weapon( ) && ent->weapon( )->data( ) && ent->vel( ).length_2d( ) > 5.0f && ent->vel( ).length_2d( ) <= ent->weapon( )->data( )->m_max_speed * 0.34f )
            moving_slow_choke.push_back( choked );
        else if ( ent->weapon( ) && ent->weapon( )->data( ) && ent->vel( ).length_2d( ) > 5.0f && ent->vel( ).length_2d( ) > ent->weapon( )->data( )->m_max_speed * 0.34f )
            moving_choke.push_back( choked );

        while ( air_choke.size( ) > 32 )
            air_choke.pop_back( );

        while ( shot_choke.size( ) > 32 )
            shot_choke.pop_back( );

        while ( moving_slow_choke.size( ) > 32 )
            moving_slow_choke.pop_back( );

        while ( moving_choke.size( ) > 32 )
            moving_choke.pop_back( );
    }

    last_origin [ ent->idx( ) ] = ent->origin( );
    last_velocity [ ent->idx( ) ] = ent->vel( );

    /* update animations if new data was sent */
    if ( ent->simtime( ) > last_update [ ent->idx( ) ] ) {
        /* clear animation frames once we recieve one that matches server... */
        /* we can use this one from now on */
        if ( !frames [ ent->idx( ) ].empty( ) )
            frames [ ent->idx( ) ].clear( );

		std::array< animlayer_t, 13> backup_layers;
		memcpy ( backup_layers.data(), ent->layers(), sizeof( backup_layers ) );
        update( ent );
		memcpy ( ent->layers ( ), backup_layers.data ( ), sizeof ( backup_layers ) );

        calc_poses( ent );

        store_frame( ent, true );
        features::lagcomp::cache( ent, false );

        last_update [ ent->idx( ) ] = ent->simtime( );

        const auto predicted_choke = predict_choke_sequence( ent );

        //if ( predicted_choke > 0 ) /* only if entity needs resimulation */ {
        //    /* backup animation data */
        //    std::array< animlayer_t, 13 > backup_animlayers { {} };
        //    const auto backup_curtime = csgo::i::globals->m_curtime;
        //    const auto backup_poses = ent->poses( );
        //    const auto backup_simtime = ent->simtime( );
        //    const auto backup_old_simtime = ent->simtime( );
        //    const auto backup_animstate = *ent->animstate( );
        //    const auto backup_flags = ent->flags( );
        //    const auto backup_velocity = ent->vel( );
        //    const auto backup_origin = ent->origin( );
        //    const auto backup_abs_origin = ent->abs_origin( );
        //    memcpy( backup_animlayers.data( ), ent->layers( ), sizeof( backup_animlayers ) );
//
        //    /* estimate player velocity */
        //    ent->vel( ) = ( ent->origin( ) - old_origin [ ent->idx( ) ] ) / ( ent->simtime( ) - ent->old_simtime( ) );
//
        //    /* apply acceleration */
        //    const auto accel = ( ent->vel( ) - old_velocity [ ent->idx( ) ] ) / ( ent->simtime( ) - ent->old_simtime( ) );
//
        //    static auto predict = [ & ] ( player_t* player, vec3_t& origin, vec3_t& velocity, uint32_t& flags, bool was_in_air ) {
		// const auto sv_gravity = g::cvars::sv_gravity;
		// const auto sv_jump_impulse = g::cvars::sv_jump_impulse;

        //     constexpr const auto sv_gravity = sv_gravity->get_float();
        //     constexpr const auto sv_jump_impulse = sv_jump_impulse->get_float();

        //     if ( !( flags & 1 ) )
        //         velocity.z -= csgo::ticks2time( 1 ) * sv_gravity;
        //     else if ( was_in_air )
        //         velocity.z = sv_jump_impulse;

        //     const auto src = origin;
        //     auto end = src + velocity * csgo::ticks2time( 1 );

        //     ray_t r;
        //     r.init( src, end, player->mins( ), player->maxs( ) );

        //     trace_t t { };
        //     trace_filter_t filter;
        //     filter.m_skip = player;

        //     csgo::util_tracehull( src, end, player->mins( ), player->maxs( ), mask_playersolid, player, &t );

        //     if ( t.m_fraction != 1.0f ) {
        //         for ( auto i = 0; i < 2; i++ ) {
        //             velocity -= t.m_plane.m_normal * velocity.dot_product( t.m_plane.m_normal );

        //             const auto dot = velocity.dot_product( t.m_plane.m_normal );

        //             if ( dot < 0.0f )
        //                 velocity -= vec3_t( dot * t.m_plane.m_normal.x, dot * t.m_plane.m_normal.y, dot * t.m_plane.m_normal.z );

        //             end = t.m_endpos + velocity * ( csgo::ticks2time( 1 ) * ( 1.0f - t.m_fraction ) );

        //             r.init( t.m_endpos, end, player->mins( ), player->maxs( ) );

        //             csgo::util_tracehull( t.m_endpos, end, player->mins( ), player->maxs( ), mask_playersolid, player, &t );

        //             if ( t.m_fraction == 1.0f )
        //                 break;
        //         }
        //     }

        //     origin = end = t.m_endpos;

        //     end.z -= 2.0f;

        //     csgo::util_tracehull( origin, end, player->mins( ), player->maxs( ), mask_playersolid, player, &t );

        //     flags &= ~1;

        //     if ( t.did_hit( ) && t.m_plane.m_normal.z > 0.7f )
        //         flags |= 1;
        // };
//
        //   constexpr auto CS_PLAYER_SPEED_DUCK_MODIFIER = 0.340f;
        //   constexpr auto CS_PLAYER_SPEED_WALK_MODIFIER = 0.520f;
        //   constexpr auto CS_PLAYER_SPEED_RUN = 260.0f;
//
        //   static auto solve_ground_fraction = [ & ] ( player_t* ent, bool& not_walking, float& ground_fraction_out ) {
        //     auto ground_fractions = ground_fraction_out;

        //     if ( ground_fraction_out > 0.0f && ground_fraction_out < 1.0f ) {
        //         const auto twotickstime = state->m_last_clientside_anim_update_time_delta * 2.0f;

        //         if ( not_walking )
        //             ground_fractions = ground_fraction_out - twotickstime;
        //         else
        //             ground_fractions = twotickstime + ground_fraction_out;

        //         ground_fractions = std::clamp< float >( ground_fractions, 0.0f, 1.0f );
        //     }

        //     if ( ent->vel( ).length_2d( ) > ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && not_walking ) {
        //         not_walking = false;
        //         ground_fractions = std::max< float >( ground_fractions, 0.01f );
        //     }
        //     else if ( ent->vel( ).length_2d( ) < ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && !not_walking ) {
        //         not_walking = true;
        //         ground_fractions = std::min< float >( ground_fractions, 0.99f );
        //     }

        //     ground_fraction_out = ground_fractions;
        // };
//
        //    bool not_walking = ent->vel( ).length_2d( ) > ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER );
//
        //    for ( auto tick = 0; tick < predicted_choke - 1; tick++ ) {
                /* accelerate player */
   //    ent->vel( ) += accel * csgo::ticks2time( 1 );

   //    /* set current time to animate player at */
   //    ent->simtime( ) = csgo::i::globals->m_curtime = ent->old_simtime( ) + csgo::ticks2time( tick + 1 );

   //    /* set previous update to previous tick */
   //    state->m_last_clientside_anim_update = csgo::i::globals->m_curtime - csgo::ticks2time( 1 );

   //    /* update time one tick */
   //    state->m_last_clientside_anim_update_time_delta = csgo::ticks2time( 1 );

   //    /* predict player data */
   //    predict( ent, ent->origin( ), ent->vel( ), ent->flags( ), !( backup_flags & 1 ) );

   //    /* fix layer 4 cycle (time in air) */
   //    ent->layers( ) [ 4 ].m_cycle += csgo::ticks2time( 1 );

   //    if ( ent->flags( ) & 1 )
   //        ent->layers( ) [ 4 ].m_cycle = 0.0f;

   //    if ( ent->flags( ) & 1 )
   //        solve_ground_fraction( ent, not_walking, ent->layers( ) [ 6 ].m_weight );

   //    ent->abs_origin( ) = ent->origin( );

   //    update( ent );

   //    store_frame( ent, tick == predicted_choke - 1 );
   //}
    //
            //    //features::lagcomp::cache( ent, true );
    //
            //    /* restore animation data */
            //    csgo::i::globals->m_curtime = backup_curtime;
            //    ent->simtime( ) = backup_simtime;
            //    ent->old_simtime( ) = backup_old_simtime;
            //    ent->poses( ) = backup_poses;
            //    *ent->animstate( ) = backup_animstate;
            //    ent->flags( ) = backup_flags;
            //    ent->vel( ) = backup_velocity;
            //    ent->origin( ) = backup_origin;
            //    ent->abs_origin( ) = backup_abs_origin;
            //    memcpy( ent->layers( ), backup_animlayers.data( ), sizeof( backup_animlayers ) );
            //}
    }
}

namespace lby {
    extern bool in_update;
}

void anims::animate_fake( ) {
    static auto jump_fall = [ ] ( float air_time ) {
        const float recalc_air_time = ( air_time - 0.72f ) * 1.25f;
        const float clamped = recalc_air_time >= 0.0f ? std::min< float >( recalc_air_time, 1.0f ) : 0.0f;
        float out = ( 3.0f - ( clamped + clamped ) ) * ( clamped * clamped );

        if ( out >= 0.0f )
            out = std::min< float >( out, 1.0f );

        return out;
    };

    static float abs_yaw = 0.0f;
    static float lby = 0.0f;
    static int curtick = 0;
    static float spawn_time = 0.0f;
    static float ground_time = 0.0f;

    if ( !g::local || !g::local->alive( ) || !g::local->animstate( ) || !g::local->layers( ) ) {
        abs_yaw = g::angles.y;
        lby = g::angles.y;
        spawn_time = features::prediction::curtime( );
        ground_time = features::prediction::curtime( );
        curtick = 0;
        return;
    }

    const auto state = g::local->animstate( );

    if ( spawn_time != g::local->spawn_time( ) ) {
        abs_yaw = g::angles.y;
        lby = g::angles.y;
        spawn_time = g::local->spawn_time( );
        ground_time = g::local->spawn_time( );
        curtick = 0;
    }

    if ( lby::in_update )
        lby = g::angles.y;

    if ( g::send_packet && new_tick ) {
        /* calculate airtime only on sent info */
        if ( g::local->flags( ) & 1 )
            ground_time = features::prediction::curtime( );

        auto& abs_vel = *reinterpret_cast< vec3_t* >( uintptr_t( g::local ) + 0x94 );

        if ( abs_vel.length_2d( ) > 0.1f || fabsf( abs_vel.z ) > 100.0f ) {
            abs_yaw = approach_angle(
                g::angles.y,
                abs_yaw,
                ( ( state->m_ground_fraction * 20.0f ) + 30.0f )
                * csgo::ticks2time( csgo::time2ticks( features::prediction::curtime( ) ) - curtick ) );
        }
        else {
            abs_yaw = approach_angle(
                lby,
                abs_yaw,
                csgo::ticks2time( csgo::time2ticks( features::prediction::curtime( ) ) - curtick ) * 100.0f );
        }

        /* backup vars */
        std::array< animlayer_t, 13 > backup_animlayers {};
        const auto backup_poses = g::local->poses( );
        memcpy( backup_animlayers.data( ), g::local->layers( ), sizeof( backup_animlayers ) );

        /* remove lean / body sway */
        g::local->layers( ) [ 12 ].m_weight = 0.0f;

        const auto body_yaw = std::clamp( csgo::normalize( csgo::normalize( g::angles.y ) - csgo::normalize( abs_yaw ) ), -60.0f, 60.0f );

        /* set up fake poses */
        state->m_jump_fall_pose.set_value( g::local, jump_fall( features::prediction::curtime( ) - ground_time ) );
        state->m_body_pitch_pose.set_value( g::local, ( csgo::normalize( g::angles.x ) + 90.0f ) / 180.0f );
        state->m_lean_yaw_pose.set_value( g::local, csgo::normalize( g::angles.y ) / 360.0f + 0.5f );
        state->m_body_yaw_pose.set_value( g::local, body_yaw / 120.0f + 0.5f );

        /* build fake bone matrix */
        build_bones( g::local, fake_matrix.data( ), 0x7ff00, vec3_t( 0.0f, csgo::normalize( g::angles.y - body_yaw ), 0.0f ), g::local->render_origin( ), features::prediction::curtime( ) );

        /* remove render origin from matrix so we can change it later */
        const auto render_origin = g::local->render_origin( );

        for ( auto i = 0; i < 128; i++ )
            fake_matrix [ i ].set_origin( fake_matrix [ i ].origin( ) - render_origin );

        /* restore vars */
        memcpy( g::local->layers( ), backup_animlayers.data( ), sizeof( backup_animlayers ) );
        g::local->poses( ) = backup_poses;

        curtick = csgo::time2ticks( features::prediction::curtime( ) );
    }
}

std::array< animlayer_t, 13 > latest_animlayers { {} };
float old_simtime = 0.0f;

void anims::animate_local( bool copy ) {
    const auto state = g::local->animstate( );
    const auto animlayers = g::local->layers( );

    if ( !state || !animlayers || !g::ucmd )
        return;

    static auto jump_fall = [ ] ( float air_time ) {
        const float recalc_air_time = ( air_time - 0.72f ) * 1.25f;
        const float clamped = recalc_air_time >= 0.0f ? std::min< float >( recalc_air_time, 1.0f ) : 0.0f;
        float out = ( 3.0f - ( clamped + clamped ) ) * ( clamped * clamped );

        if ( out >= 0.0f )
            out = std::min< float >( out, 1.0f );

        return out;
    };

    static int last_update_tick = 0;
    static float latest_abs_yaw = 0.0f;
    static std::array< float, 24 > latest_poses { 0.0f };

    static float calc_weight = 0.0f;
    static float calc_cycle = 0.0f;
    static float synced_weight = 0.0f;
    static float synced_cycle = 0.0f;

	if ( !copy ) {
		memcpy ( animlayers, latest_animlayers.data ( ), sizeof ( latest_animlayers ) );

		state->m_last_clientside_anim_update_time_delta = std::max< float > ( 0.0f, features::prediction::curtime ( ) - state->m_last_clientside_anim_update );

		//if ( new_tick ) {
		calc_local_exclusive ( calc_weight, calc_cycle );

		//animlayers [ 6 ].m_weight = synced_weight;
		//animlayers [ 4 ].m_cycle = synced_cycle;
		animlayers [ 12 ].m_weight = 0.0f;

		//if ( g::send_packet ) {
		//    synced_weight = calc_weight;
		//    synced_cycle = calc_cycle;
//
		//    latest_animlayers [ 6 ].m_weight = animlayers [ 6 ].m_weight = synced_weight;
		//    latest_animlayers [ 4 ].m_cycle = animlayers [ 4 ].m_cycle = synced_cycle;
		//    latest_animlayers [ 12 ].m_weight = animlayers [ 12 ].m_weight = 0.0f;
		//}

		/* recreate what holdaim var does */
		/* TODO: check if holdaim cvar is enaled */
		if ( g::cvars::sv_maxusrcmdprocessticks_holdaim->get_bool ( ) ) {
			if ( g::ucmd->m_buttons & 1 ) {
				g::angles = g::ucmd->m_angs;
				g::hold_aim = true;
			}
		}
		else {
			g::hold_aim = false;
		}

		if ( !g::hold_aim ) {
			g::angles = g::ucmd->m_angs;
		}

		update ( g::local );

		if ( g::send_packet ) {
			//state->m_jump_fall_pose.set_value( g::local, jump_fall( synced_cycle ) );

			memcpy ( latest_poses.data ( ), g::local->poses ( ).data ( ), sizeof ( latest_poses ) );
			latest_abs_yaw = state->m_abs_yaw;
			g::hold_aim = false;
		}

		build_bones ( g::local, aim_matrix.data ( ), 0x7ff00, vec3_t ( 0.0f, latest_abs_yaw, 0.0f ), g::local->origin ( ), features::prediction::curtime ( ) );

		last_update_tick = csgo::time2ticks ( features::prediction::curtime ( ) );

		animate_fake ( );

		//	new_tick = false;
		//}
	}
	else {
		memcpy ( animlayers, latest_animlayers.data ( ), sizeof ( latest_animlayers ) );

		g::local->poses ( ) = latest_poses;
		g::local->set_abs_angles ( vec3_t ( 0.0f, latest_abs_yaw, 0.0f ) );
	}
}

void anims::copy_data( player_t* ent ) {
    const auto state = ent->animstate( );
    const auto animlayers = ent->layers( );

    if ( !state || !animlayers || frames [ ent->idx( ) ].empty( ) )
        return;

    //ent->set_abs_angles( vec3_t( 0.0f, state->m_abs_yaw, 0.0f ) );

    /* get animation update, use this record to render */
    const auto target_frame = std::find_if( frames [ ent->idx( ) ].begin( ), frames [ ent->idx( ) ].end( ), [ & ] ( const animation_frame_t& frame ) { return frame.m_anim_update; } );

    if ( target_frame == frames [ ent->idx( ) ].end( ) )
        return;

    memcpy( ent->layers( ), target_frame->m_animlayers.data( ), sizeof( target_frame->m_animlayers ) );

    if ( features::ragebot::get_misses( ent->idx( ) ).bad_resolve % 3 == 0 ) {
        ent->set_abs_angles( vec3_t( 0.0f, csgo::normalize( csgo::normalize( target_frame->m_animstate.m_eye_yaw ) - ( ( target_frame->m_poses1 [ 11 ] - 0.5f ) * 120.0f ) ), 0.0f ) );
        memcpy( ent->poses( ).data( ), target_frame->m_poses1.data( ), sizeof( ent->poses( ) ) );
    }
    else if ( features::ragebot::get_misses( ent->idx( ) ).bad_resolve % 3 == 1 ) {
        ent->set_abs_angles( vec3_t( 0.0f, csgo::normalize( csgo::normalize( target_frame->m_animstate.m_eye_yaw ) - ( ( target_frame->m_poses2 [ 11 ] - 0.5f ) * 120.0f ) ), 0.0f ) );
        memcpy( ent->poses( ).data( ), target_frame->m_poses2.data( ), sizeof( ent->poses( ) ) );
    }
    else {
        ent->set_abs_angles( vec3_t( 0.0f, csgo::normalize( csgo::normalize( target_frame->m_animstate.m_eye_yaw ) - ( ( target_frame->m_poses3 [ 11 ] - 0.5f ) * 120.0f ) ), 0.0f ) );
        memcpy( ent->poses( ).data( ), target_frame->m_poses3.data( ), sizeof( ent->poses( ) ) );
    }
}

void anims::run( int stage ) {
    if ( !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) )
        return;

    switch ( stage ) {
        case 5: { /* FRAME_RENDER_START */
            for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
                animations::resolver::process_event_buffer( i );

                const auto ent = csgo::i::ent_list->get< player_t* >( i );

				if (!ent || !ent->is_player() || !ent->alive())
					features::ragebot::get_misses ( i ).bad_resolve = 0;

                if ( ent && ent->alive( ) && !ent->dormant( ) ) {
                    *reinterpret_cast< int* >( uintptr_t( ent ) + 0xA30 ) = csgo::i::globals->m_framecount;
                    *reinterpret_cast< int* >( uintptr_t( ent ) + 0xA28 ) = 0;
                    copy_data( ent );
                }
            }

            animate_local( true );
        } break;
        case 4: { /* FRAME_NET_UPDATE_END */
            csgo::for_each_player( [ & ] ( player_t* ent ) {
                if ( ent == g::local )
                    return;

                interpolate( ent, false );
                animate_player( ent );
                } );
        } break;
        case 2: { /* FRAME_NET_UPDATE_POSTDATAUPDATE_START */
            csgo::for_each_player( [ & ] ( player_t* ent ) {
                if ( ent == g::local )
                    return;

                interpolate( ent, false );
                } );

			if ( g::local && g::local->simtime ( ) != old_simtime ) {
				if( g::local->layers ( ) )
					memcpy ( latest_animlayers.data ( ), g::local->layers ( ), sizeof ( latest_animlayers ) );

				old_simtime = g::local->simtime ( );
			}
        } break;
        case 3: { /* FRAME_NET_UPDATE_POSTDATAUPDATE_END */
            csgo::for_each_player( [ & ] ( player_t* ent ) {
                if ( ent == g::local )
                    return;

                /* fix pvs */
                *reinterpret_cast< int* >( uintptr_t( ent ) + 0xA30 ) = csgo::i::globals->m_framecount;
                *reinterpret_cast< int* >( uintptr_t( ent ) + 0xA28 ) = 0;
                } );
        } break;
    }
}

#pragma optimize( "2", off )