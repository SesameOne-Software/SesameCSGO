#include "blockbot.hpp"
#include "../menu/options.hpp"

void features::blockbot::run( ucmd_t* ucmd, vec3_t& move_ang ) {
    constexpr auto max_movement_speed = 450.0f;

    static auto& blockbot = options::vars [ _( "misc.movement.block_bot" ) ].val.b;
    static auto& blockbot_key = options::vars [ _( "misc.movement.block_bot_key" ) ].val.i;
    static auto& blockbot_key_mode = options::vars [ _( "misc.movement.block_bot_key_mode" ) ].val.i;

    if ( !g::local || !blockbot || !utils::keybind_active( blockbot_key, blockbot_key_mode ) )
        return;

    vec3_t nearest_pos;
    player_t* nearest_ent;
    float nearest_dist = FLT_MAX;

    for ( auto i = 1; i < csgo::i::globals->m_max_clients; i++ ) {
        const auto ent = csgo::i::ent_list->get< player_t*>( i );

        if ( !ent->valid( ) || ent == g::local )
            continue;

        const auto mag = ent->origin( ).dist_to( g::local->abs_origin( ) );

        if ( mag < nearest_dist ) {
            nearest_ent = ent;
            nearest_pos = ent->origin( );
            nearest_dist = mag;
        }
    }

    if ( nearest_ent ) {
        const auto extrap_pos = nearest_pos + nearest_ent->vel( ) * csgo::ticks2time( 1 );

        if ( std::fabsf( extrap_pos.z - g::local->abs_origin( ).z ) > 40.0f ) {
            if ( ( extrap_pos - g::local->abs_origin( ) ).length_2d( ) < 4.0f )
                return;

            move_ang.y = csgo::calc_angle( g::local->abs_origin( ), extrap_pos ).y;
            ucmd->m_fmove = max_movement_speed;
        }
        else {
            vec3_t client_ang;
            csgo::i::engine->get_viewangles( client_ang );

            const auto ang_to = csgo::calc_angle( g::local->abs_origin( ), nearest_pos ).y;
            const auto yaw_delta = csgo::normalize( ang_to - client_ang.y );

            if ( fabsf( yaw_delta ) < 5.0f )
                return;

            ucmd->m_smove = yaw_delta < 0.0f ? max_movement_speed : -max_movement_speed;
        }
    }
}