#include "animation_system.hpp"
#include "../menu/d3d9_render.hpp"
#include "resolver.hpp"
#include "../features/ragebot.hpp"

std::array< matrix3x4_t, 128 > anims::fake_matrix { };
std::array< matrix3x4_t, 128 > anims::aim_matrix { };
std::array< std::deque< anims::animation_frame_t >, 65 > anims::frames { {{}} };

void anims::animation_frame_t::store( player_t* ent, bool anim_update ) {
    memcpy( m_animlayers.data( ), ent->layers( ), sizeof( m_animlayers ) );
    m_poses = ent->poses( );
    m_simtime = ent->simtime( );
    m_old_simtime = ent->simtime( );
    m_animstate = *ent->animstate( );
    m_flags = ent->flags( );

    m_velocity = ent->vel( );
    m_origin = ent->origin( );

    m_anim_update = anim_update;
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

void anims::calc_local_exclusive( float& ground_fraction_out, float& air_time_out ) {
    constexpr auto CS_PLAYER_SPEED_DUCK_MODIFIER = 0.340f;
    constexpr auto CS_PLAYER_SPEED_WALK_MODIFIER = 0.520f;
    constexpr auto CS_PLAYER_SPEED_RUN = 260.0f;

    static float ground_fractions = 0.0f;
    static bool old_not_walking = false;

    static float old_ground_time = 0.0f;

    auto& abs_vel = *reinterpret_cast< vec3_t* >( uintptr_t( g::local ) + 0x94 );

    const auto old_ground_fraction = ground_fractions;

    if ( old_ground_fraction > 0.0f && old_ground_fraction < 1.0f ) {
        const auto twotickstime = g::local->animstate( )->m_last_clientside_anim_update_time_delta * 2.0f;

        if ( old_not_walking )
            ground_fractions = old_ground_fraction - twotickstime;
        else
            ground_fractions = twotickstime + old_ground_fraction;

        ground_fractions = std::clamp< float >( ground_fractions, 0.0f, 1.0f );
    }

    if ( abs_vel.length_2d( ) > ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && old_not_walking ) {
        old_not_walking = false;
        ground_fractions = std::max< float >( ground_fractions, 0.01f );
    }
    else if ( abs_vel.length_2d( ) < ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && !old_not_walking ) {
        old_not_walking = true;
        ground_fractions = std::min< float >( ground_fractions, 0.99f );
    }

    ground_fraction_out = ground_fractions;

    if ( g::local->flags( ) & 1 ) {
        old_ground_time = csgo::i::globals->m_curtime;
        air_time_out = 0.0f;
    }
    else {
        air_time_out = csgo::i::globals->m_curtime - old_ground_time;
    }
}

void anims::calc_animlayers( player_t* ent ) {
    auto state = ent->animstate( );
    auto layers = ent->layers( );

    if ( !state || !layers )
        return;

    if ( ent == g::local )
        calc_local_exclusive( layers [ 6 ].m_weight, layers [ 4 ].m_cycle );

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

    state->m_move_blend_walk_pose.set_value( ent, ( 1.0f - layers [ 6 ].m_weight ) * ( 1.0f - ent->crouch_amount( ) ) );
    state->m_move_blend_run_pose.set_value( ent, ( 1.0f - ent->crouch_amount( ) ) * layers [ 6 ].m_weight );
    state->m_move_blend_crouch_pose.set_value( ent, ent->crouch_amount( ) );

    state->m_body_pitch_pose.set_value( ent, ( csgo::normalize( state->m_pitch ) + 90.0f ) / 180.0f );
    //state->m_lean_yaw_pose.set_value( ent, csgo::normalize( state->m_eye_yaw ) / 360.0f + 0.5f );
    state->m_body_yaw_pose.set_value( ent, std::clamp( csgo::normalize( csgo::normalize( state->m_eye_yaw ) - csgo::normalize( state->m_abs_yaw ) ), -60.0f, 60.0f ) / 120.0f + 0.5f );
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

    if ( ent != g::local )
        csgo::i::globals->m_curtime = ent->old_simtime( ) + csgo::ticks2time( 1 );

    csgo::i::globals->m_frametime = csgo::ticks2time( 1 );

    /* fix other players' velocity */
    if ( ent != g::local ) {
        /* skip call to C_BaseEntity::CalcAbsoluteVelocity */
        *reinterpret_cast< uint32_t* >( uintptr_t( ent ) + 0xe8 ) &= ~0x1000;

        /* replace abs velocity (to be recalculated) */
        abs_vel = ent->vel( );
    }

    /* fix abs velocity rounding errors */
    if ( fabsf( abs_vel.x ) < 0.001f )
        abs_vel.x = 0.0f;

    if ( fabsf( abs_vel.y ) < 0.001f )
        abs_vel.y = 0.0f;

    if ( fabsf( abs_vel.z ) < 0.001f )
        abs_vel.z = 0.0f;

    /* fix foot spin */
    state->m_feet_yaw_rate = 0.0f;

    /* force animation update */
    state->m_force_update = true;

    /* remove anim and bone interpolation */
    *reinterpret_cast< uint32_t* >( uintptr_t( ent ) + 0xf0 ) |= 8;

    state->m_last_clientside_anim_framecount = 0;

    /* fix pitch */
    if ( ent != g::local )
        state->m_pitch = ent->angles( ).x;

    /* recalculate animlayers */
    calc_animlayers( ent );

    /* update animations */
    ent->animate( ) = true;
    ent->update( );

    /* rebuild poses */
    calc_poses( ent );

    /* stop animating */
    ent->animate( ) = false;

    /* restore original information */
    *reinterpret_cast< uint32_t* >( uintptr_t( ent ) + 0xe8 ) = backup_eflags;
    abs_vel = backup_abs_vel;
    *reinterpret_cast< uint32_t* >( uintptr_t( ent ) + 0xf0 ) = backup_effects;

    csgo::i::globals->m_curtime = backup_curtime;
    csgo::i::globals->m_frametime = backup_frametime;
}

void anims::store_frame( player_t* ent, bool anim_update ) {
    const auto state = ent->animstate( );
    const auto animlayers = ent->layers( );

    if ( !state || !animlayers )
        return;

    animation_frame_t animation_frame;

    animation_frame.store( ent, anim_update );

    float abs_yaw1 = 0.0f, abs_yaw2 = 0.0f, abs_yaw3 = 0.0f;
    animations::resolver::resolve( ent, abs_yaw1, abs_yaw2, abs_yaw3 );

    state->m_feet_yaw = state->m_abs_yaw = abs_yaw1;
    calc_poses( ent );
    build_bones( ent, animation_frame.m_matrix1.data( ), 256, ent->abs_angles( ), ent->origin( ), ent->simtime( ) );

    state->m_feet_yaw = state->m_abs_yaw = abs_yaw2;
    calc_poses( ent );
    build_bones( ent, animation_frame.m_matrix2.data( ), 256, ent->abs_angles( ), ent->origin( ), ent->simtime( ) );

    state->m_feet_yaw = state->m_abs_yaw = abs_yaw3;
    calc_poses( ent );
    build_bones( ent, animation_frame.m_matrix3.data( ), 256, ent->abs_angles( ), ent->origin( ), ent->simtime( ) );

    frames [ ent->idx( ) ].push_front( animation_frame );

    /* if recieved information on entity from server, store record */
    //
    //if ( anim_update ) {
    features::lagcomp::cache( ent );
    //}
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

    if ( ent->simtime( ) > last_update [ ent->idx( ) ] ) {
        old_origin [ ent->idx( ) ] = last_origin [ ent->idx( ) ];
        old_velocity [ ent->idx( ) ] = last_velocity [ ent->idx( ) ];
    }

    last_origin [ ent->idx( ) ] = ent->origin( );
    last_velocity [ ent->idx( ) ] = ent->vel( );

    /* update animations if new data was sent */
    if ( ent->simtime( ) > last_update [ ent->idx( ) ] ) {
        /* clear animation frames once we recieve one that matches server... */
        /* we can use this one from now on */
        frames [ ent->idx( ) ].clear( );

        update( ent );

        float abs_yaw1 = 0.0f, abs_yaw2 = 0.0f, abs_yaw3 = 0.0f;
        animations::resolver::resolve( ent, abs_yaw1, abs_yaw2, abs_yaw3 );

        if ( features::ragebot::get_misses( ent->idx( ) ).bad_resolve % 3 == 0 )
            state->m_feet_yaw = state->m_abs_yaw = abs_yaw1;
        else if ( features::ragebot::get_misses( ent->idx( ) ).bad_resolve % 3 == 0 )
            state->m_feet_yaw = state->m_abs_yaw = abs_yaw2;
        else
            state->m_feet_yaw = state->m_abs_yaw = abs_yaw3;

        calc_poses( ent );

        store_frame( ent, true );

        last_update [ ent->idx( ) ] = ent->simtime( );

        return;
    }

    const auto choked = std::clamp< int >( csgo::time2ticks( ent->simtime( ) - ent->old_simtime( ) ), 0, 62 ) - 1;

    if ( choked > 0 /* only if entity choking packets */
        && !frames [ ent->idx( ) ].empty( ) /* only if we have recieved animation frame(s) */
        && frames [ ent->idx( ) ].front( ).m_anim_update ) /* only if entity needs resimulation */ {
        /* backup animation data */
        std::array< animlayer_t, 13 > backup_animlayers { {} };
        const auto backup_poses = ent->poses( );
        const auto backup_simtime = ent->simtime( );
        const auto backup_old_simtime = ent->simtime( );
        const auto backup_animstate = *ent->animstate( );
        const auto backup_flags = ent->flags( );
        const auto backup_velocity = ent->vel( );
        const auto backup_origin = ent->origin( );
        auto backup_abs_origin = ent->abs_origin( );
        memcpy( backup_animlayers.data( ), ent->layers( ), sizeof( backup_animlayers ) );

        /* estimate player velocity */
        ent->vel( ) = ( ent->origin( ) - old_origin [ ent->idx( ) ] ) / ( ent->simtime( ) - ent->old_simtime( ) );

        /* apply acceleration */
        const auto accel = ( ent->vel( ) - old_velocity [ ent->idx( ) ] ) / ( ent->simtime( ) - ent->old_simtime( ) );

        static auto predict = [ & ] ( player_t* player, vec3_t& origin, vec3_t& velocity, uint32_t& flags ) {
            constexpr const auto sv_gravity = 800.0f;
            constexpr const auto sv_jump_impulse = 301.993377f;

            if ( !( flags & 1 ) )
                velocity.z -= csgo::ticks2time( sv_gravity );

            const auto src = origin;
            auto end = src + velocity * csgo::ticks2time( 1 );

            ray_t r;
            r.init( src, end, player->mins( ), player->maxs( ) );

            trace_t t { };
            trace_filter_t filter;
            filter.m_skip = player;

            csgo::util_tracehull( src, end, player->mins( ), player->maxs( ), 0x201400B, player, &t );

            if ( t.m_fraction != 1.0f ) {
                for ( auto i = 0; i < 2; i++ ) {
                    velocity -= t.m_plane.m_normal * velocity.dot_product( t.m_plane.m_normal );

                    const auto dot = velocity.dot_product( t.m_plane.m_normal );

                    if ( dot < 0.0f )
                        velocity -= vec3_t( dot * t.m_plane.m_normal.x, dot * t.m_plane.m_normal.y, dot * t.m_plane.m_normal.z );

                    end = t.m_endpos + velocity * ( csgo::ticks2time( 1 ) * ( 1.0f - t.m_fraction ) );

                    r.init( t.m_endpos, end, player->mins( ), player->maxs( ) );

                    csgo::util_tracehull( t.m_endpos, end, player->mins( ), player->maxs( ), 0x201400B, player, &t );

                    if ( t.m_fraction == 1.0f )
                        break;
                }
            }

            origin = end = t.m_endpos;

            end.z -= 2.0f;

            csgo::util_tracehull( origin, end, player->mins( ), player->maxs( ), 0x201400B, player, &t );

            flags &= 1;

            if ( t.did_hit( ) && t.m_plane.m_normal.z > 0.7f )
                flags |= 1;
        };

        for ( auto tick = 0; tick < choked; tick++ ) {
            /* accelerate player */
            ent->vel( ) += accel * csgo::ticks2time( 1 );

            /* set current time to animate player at */
            ent->old_simtime( ) = ent->simtime( );
            ent->simtime( ) += csgo::ticks2time( 1 );

            /* set previous update to previous tick */
            state->m_last_clientside_anim_update = ent->old_simtime( );

            /* update time one tick */
            state->m_last_clientside_anim_update_time_delta = csgo::ticks2time( 1 );

            /* predict player data */
            predict( ent, ent->origin( ), ent->vel( ), ent->flags( ) );

            ent->set_abs_origin( ent->origin( ) );

            update( ent );

            store_frame( ent, false );
        }

        /* restore animation data */
        ent->simtime( ) = backup_simtime;
        ent->old_simtime( ) = backup_old_simtime;
        ent->poses( ) = backup_poses;
        *ent->animstate( ) = backup_animstate;
        ent->flags( ) = backup_flags;
        ent->vel( ) = backup_velocity;
        ent->origin( ) = backup_origin;
        ent->set_abs_origin( backup_abs_origin );
        memcpy( ent->layers( ), backup_animlayers.data( ), sizeof( backup_animlayers ) );
    }
}

void anims::animate_fake( ) {
    static int curtick = 0;
    static bool should_reset = true;
    static float spawn_time = 0.0f;
    static animstate_t fake_state;

    if ( !g::local || !g::local->alive( ) || !g::local->layers( ) || !g::local->renderable( ) ) {
        should_reset = true;
        curtick = 0;
        return;
    }

    if ( should_reset || spawn_time != g::local->spawn_time( ) ) {
        spawn_time = g::local->spawn_time( );
        g::local->create_animstate( &fake_state );
        should_reset = false;
        curtick = 0;
    }

    if ( g::send_packet && csgo::i::globals->m_tickcount != curtick ) {
        curtick = csgo::i::globals->m_tickcount;

        std::array< animlayer_t, 13 > backup_animlayers {};

        const float backup_frametime = csgo::i::globals->m_frametime;
        const auto backup_poses = g::local->poses( );
        auto backup_abs_angles = g::local->abs_angles( );

        memcpy( backup_animlayers.data( ), g::local->layers( ), sizeof( backup_animlayers ) );

        fake_state.m_last_clientside_anim_update_time_delta = std::max< float >( 0.0f, csgo::i::globals->m_curtime - fake_state.m_last_clientside_anim_update_time_delta );

        fake_state.m_feet_yaw_rate = 0.0f;
        fake_state.m_unk_feet_speed_ratio = 0.0f;
        fake_state.m_last_clientside_anim_framecount = 0;
        csgo::i::globals->m_frametime = csgo::i::globals->m_ipt;
        fake_state.update( g::sent_cmd.m_angs );
        csgo::i::globals->m_frametime = backup_frametime;

        g::local->setup_bones( fake_matrix.data( ), 128, 0x7ff00, csgo::i::globals->m_curtime );

        const auto render_origin = g::local->render_origin( );

        for ( auto i = 0; i < 128; i++ )
            fake_matrix [ i ].set_origin( fake_matrix [ i ].origin( ) - render_origin );

        memcpy( g::local->layers( ), backup_animlayers.data( ), sizeof( backup_animlayers ) );
        g::local->abs_angles( ) = backup_abs_angles;
        g::local->poses( ) = backup_poses;
    }
}

void anims::animate_local( ) {
    const auto state = g::local->animstate( );
    const auto animlayers = g::local->layers( );

    if ( !state || !animlayers || !g::ucmd )
        return;

    static int last_update_tick = 0;
    static float latest_abs_yaw = 0.0f;
    static std::array< float, 24 > latest_poses { 0.0f };
    static std::array< animlayer_t, 13 > latest_animlayers { {} };
    static float old_simtime = 0.0f;

    /* recieved new animlayers, try to use these.. .else, use old ones */
    if ( g::local->simtime( ) != old_simtime ) {
        memcpy( latest_animlayers.data( ), animlayers, sizeof( latest_animlayers ) );
        old_simtime = g::local->simtime( );
    }
    else {
        memcpy( animlayers, latest_animlayers.data( ), sizeof( latest_animlayers ) );
    }

    if ( last_update_tick != csgo::i::globals->m_tickcount ) {
        /* recreate what holdaim var does */
        /* TODO: check if holdaim cvar is enaled */
        if ( g::ucmd->m_buttons & 1 ) {
            g::angles = g::ucmd->m_angs;
            g::hold_aim = true;
        }

        if ( !g::hold_aim ) {
            g::angles = g::ucmd->m_angs;
        }

        state->m_last_clientside_anim_update_time_delta = std::max< float >( 0.0f, csgo::i::globals->m_curtime - state->m_last_clientside_anim_update );

        update( g::local );

        if ( g::send_packet ) {
            memcpy( latest_poses.data( ), g::local->poses( ).data( ), sizeof( latest_poses ) );
            latest_abs_yaw = state->m_abs_yaw;
            g::hold_aim = false;
        }

        build_bones( g::local, aim_matrix.data( ), 0x7ff00, vec3_t( 0.0f, latest_abs_yaw, 0.0f ), g::local->origin( ), csgo::i::globals->m_curtime );

        last_update_tick = csgo::i::globals->m_tickcount;
    }

    memcpy( animlayers, latest_animlayers.data( ), sizeof( latest_animlayers ) );

    g::local->poses( ) = latest_poses;
    g::local->set_abs_angles( vec3_t( 0.0f, latest_abs_yaw, 0.0f ) );
}

void anims::copy_data( player_t* ent ) {
    const auto state = ent->animstate( );
    const auto animlayers = ent->layers( );

    if ( !state || !animlayers || frames [ ent->idx( ) ].empty( ) )
        return;

    /* get animation update, use this record to render */
    const auto target_frame = std::find_if( frames [ ent->idx( ) ].begin( ), frames [ ent->idx( ) ].end( ), [ & ] ( const animation_frame_t& frame ) { return frame.m_anim_update; } );

    if ( target_frame == frames [ ent->idx( ) ].end( ) )
        return;

    memcpy( ent->layers( ), target_frame->m_animlayers.data( ), sizeof( target_frame->m_animlayers ) );
    memcpy( ent->poses( ).data( ), target_frame->m_poses.data( ), sizeof( target_frame->m_poses ) );
}

void anims::run( int stage ) {
    if ( stage == 5 )
        sesui::binds::frame_time = csgo::i::globals->m_frametime;

    if ( !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) )
        return;

    switch ( stage ) {
        case 5: { /* FRAME_RENDER_START */
            for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
                animations::resolver::process_event_buffer( i );

                const auto ent = csgo::i::ent_list->get< player_t* >( i );

                if ( ent && ent->alive( ) && !ent->dormant( ) )
                    copy_data( ent );
            }

            animate_fake( );
            animate_local( );
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