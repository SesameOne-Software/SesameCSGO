#include <deque>
#include "legitbot.hpp"
#include "../menu/menu.hpp"
#include "../menu/options.hpp"
#include "prediction.hpp"
#include "autowall.hpp"

__forceinline void run_triggerbot( ucmd_t* ucmd ) {
    static auto& triggerbot_enabled = options::vars [ _( "legitbot.triggerbot" ) ].val.b;
    static auto& triggerbot_key = options::vars [ _( "legitbot.triggerbot_key" ) ].val.i;
    static auto& triggerbot_key_mode = options::vars [ _( "legitbot.triggerbot_key_mode" ) ].val.i;
    static auto& triggerbot_hitboxes = options::vars [ _( "legitbot.triggerbot_hitboxes" ) ].val.l;

    if ( !triggerbot_enabled || !utils::keybind_active( triggerbot_key, triggerbot_key_mode ) )
        return;

    if ( !g::local->weapon( ) || !g::local->weapon( )->data( ) )
        return;

    const auto eyes = g::local->eyes( );

    vec3_t ang;
    csgo::i::engine->get_viewangles( ang );

    trace_t tr;
    ray_t ray;
    trace_filter_t filter;
    filter.m_skip = g::local;

    ray.init( eyes, eyes + csgo::angle_vec( ang ).normalized( ) * g::local->weapon( )->data( )->m_range );

    csgo::i::trace->trace_ray( ray, mask_shot, &filter, &tr );

    auto can_shoot = [ & ] ( ) {
        if ( !g::local->weapon( ) || !g::local->weapon( )->ammo( ) )
            return false;

        if ( g::local->weapon( )->item_definition_index( ) == 64 && !( g::can_fire_revolver || csgo::time2ticks( csgo::i::globals->m_curtime ) > g::cock_ticks ) )
            return false;

        return csgo::i::globals->m_curtime >= g::local->next_attack( ) && csgo::i::globals->m_curtime >= g::local->weapon( )->next_primary_attack( );
    };

    const auto hit_pl = reinterpret_cast< player_t* >( tr.m_hit_entity );

    std::deque< int > hitboxes { };

    if ( triggerbot_hitboxes [ 3 ] )
        hitboxes.push_back( autowall::hitbox_to_hitgroup( 2 ) ); // pelvis

    if ( triggerbot_hitboxes [ 0 ] )
        hitboxes.push_back( autowall::hitbox_to_hitgroup( 0 ) ); // head

    if ( triggerbot_hitboxes [ 1 ] )
        hitboxes.push_back( autowall::hitbox_to_hitgroup( 1 ) ); // neck

    if ( triggerbot_hitboxes [ 6 ] ) {
        hitboxes.push_back( autowall::hitbox_to_hitgroup( 11 ) ); // right foot
        hitboxes.push_back( autowall::hitbox_to_hitgroup( 12 ) ); // left foot
    }

    if ( triggerbot_hitboxes [ 2 ] ) {
        hitboxes.push_back( autowall::hitbox_to_hitgroup( 6 ) ); // chest
    }

    if ( triggerbot_hitboxes [ 5 ] ) {
        hitboxes.push_back( autowall::hitbox_to_hitgroup( 7 ) ); // right thigh
        hitboxes.push_back( autowall::hitbox_to_hitgroup( 8 ) ); // left thigh
    }

    if ( triggerbot_hitboxes [ 4 ] ) {
        hitboxes.push_back( autowall::hitbox_to_hitgroup( 18 ) ); // right forearm
        hitboxes.push_back( autowall::hitbox_to_hitgroup( 16 ) ); // left forearm
    }

    bool hitbox_target = !hitboxes.empty( )
        ? std::find( hitboxes.begin( ), hitboxes.end( ), tr.m_hitgroup ) != hitboxes.end( )
        : false;

    if ( can_shoot( )
        && hit_pl->valid( )
        && hit_pl->team( ) != g::local->team( )
        && hitbox_target ) {
        ucmd->m_buttons |= 1;
    }
    else {
        ucmd->m_buttons &= ~1;
    }
}

void features::legitbot::run( ucmd_t* ucmd ) {
    static auto& main_switch = options::vars [ _( "global.assistance_type" ) ].val.i;

    if ( main_switch != 1 || !csgo::i::engine->is_in_game( ) || !csgo::i::engine->is_connected( ) || !g::local )
        return;

    run_triggerbot( ucmd );
}