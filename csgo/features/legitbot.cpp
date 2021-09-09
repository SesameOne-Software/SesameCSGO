#include <deque>
#include "legitbot.hpp"
#include "../menu/menu.hpp"
#include "../menu/options.hpp"
#include "prediction.hpp"
#include "autowall.hpp"
#include "exploits.hpp"

__forceinline void run_triggerbot( ucmd_t* ucmd ) {
    static auto& triggerbot_enabled = options::vars [ _( "legitbot.triggerbot" ) ].val.b;
    static auto& triggerbot_key = options::vars [ _( "legitbot.triggerbot_key" ) ].val.i;
    static auto& triggerbot_key_mode = options::vars [ _( "legitbot.triggerbot_key_mode" ) ].val.i;
    static auto& triggerbot_hitboxes = options::vars [ _( "legitbot.triggerbot_hitboxes" ) ].val.l;

    if ( !triggerbot_enabled || !utils::keybind_active ( triggerbot_key, triggerbot_key_mode ) )
            return;

    if ( !g::local->weapon ( ) || !g::local->weapon ( )->data ( ) )
            return;

    const auto eyes = g::local->eyes( );

    vec3_t ang;
    cs::i::engine->get_viewangles( ang );

    trace_t tr;
    ray_t ray;
    trace_filter_t filter;
    filter.m_skip = g::local;

    ray.init( eyes, eyes + cs::angle_vec( ang ).normalized( ) * g::local->weapon( )->data( )->m_range );

    cs::i::trace->trace_ray( ray, mask_shot, &filter, &tr );

    const auto hit_pl = reinterpret_cast< player_t* >( tr.m_hit_entity );

    std::deque< int > hitboxes { };

    if ( triggerbot_hitboxes [ 3 ] )
        hitboxes.push_back( autowall::hitbox_to_hitgroup( hitbox_t::pelvis ) );

    if ( triggerbot_hitboxes [ 0 ] )
        hitboxes.push_back( autowall::hitbox_to_hitgroup( hitbox_t::head ) );

    if ( triggerbot_hitboxes [ 1 ] )
        hitboxes.push_back( autowall::hitbox_to_hitgroup( hitbox_t::neck ) );

    if ( triggerbot_hitboxes [ 6 ] ) {
        hitboxes.push_back( autowall::hitbox_to_hitgroup( hitbox_t::r_foot ) );
        hitboxes.push_back( autowall::hitbox_to_hitgroup( hitbox_t::l_foot ) );
    }

    if ( triggerbot_hitboxes [ 2 ] ) {
        hitboxes.push_back( autowall::hitbox_to_hitgroup( hitbox_t::u_chest ) );
    }

    if ( triggerbot_hitboxes [ 5 ] ) {
        hitboxes.push_back( autowall::hitbox_to_hitgroup( hitbox_t::r_thigh ) );
        hitboxes.push_back( autowall::hitbox_to_hitgroup( hitbox_t::l_thigh ) );
    }

    if ( triggerbot_hitboxes [ 4 ] ) {
        hitboxes.push_back( autowall::hitbox_to_hitgroup( hitbox_t::r_forearm ) );
        hitboxes.push_back( autowall::hitbox_to_hitgroup( hitbox_t::l_forearm ) );
    }

    bool hitbox_target = !hitboxes.empty( )
        ? std::find( hitboxes.begin( ), hitboxes.end( ), tr.m_hitgroup ) != hitboxes.end( )
        : false;

    if ( exploits::can_shoot( )
        && hit_pl->valid( )
        && g::local->is_enemy ( hit_pl )
        && hitbox_target ) {
        ucmd->m_buttons |= buttons_t::attack;
    }
    else {
        ucmd->m_buttons &= ~buttons_t::attack;
    }
}

void features::legitbot::run( ucmd_t* ucmd ) {
    VMP_BEGINMUTATION ( );
    static auto& main_switch = options::vars [ _( "global.assistance_type" ) ].val.i;

    if ( main_switch != 1 || !cs::i::engine->is_in_game( ) || !cs::i::engine->is_connected( ) || !g::local )
        return;

    run_triggerbot( ucmd );
    VMP_END ( );
}