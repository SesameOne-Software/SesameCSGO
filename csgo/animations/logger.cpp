#include "logger.hpp"

#include <unordered_map>
#include <fstream>

#include "animation_system.hpp"

#ifdef ANIMATION_LOGGER

std::unordered_map<
    int/*choke_ticks*/,
    std::unordered_map<
    float/*angle_delta*/,
    std::unordered_map<
    float/*vel_delta*/,
    std::unordered_map<
    float/*playback_rate*/,
    float/*sign*/
    >>>> log_data {

};

std::string anims::logger::dump ( ) {
    std::string buf;

    buf.append (
        "#pragma once\n"
        "\n"
        "#include <unordered_map>\n"
        "\n"
        "namespace anims {\n"
        "namespace logger {\n"
        "\n"
        "inline std::unordered_map<"
        "\tint/*choke_ticks*/,\n"
        "\tstd::unordered_map<\n"
        "\t\tfloat/*angle_delta*/,\n"
        "\t\tstd::unordered_map<\n"
        "\t\t\tfloat/*vel_delta*/,\n"
        "\t\t\tstd::unordered_map<\n"
        "\t\t\t\tfloat/*playback_rate*/,\n"
        "\t\t\t\tfloat/*sign*/\n"
        "\t\t\t>\n"
        "\t\t>\n"
        "\t>\n"
        "> data {\n"
    );

    /*
    *   {-12.0, {0.00512, -0.7f3}},\n
    */

    for ( auto& choked : log_data ) {

    }

    for ( auto& choked : log_data ) {
        buf.append ( std::string ( "\t{" ).append ( std::to_string ( choked.first ) ).append ( ",{\n" ) );

        for ( auto& angle_delta : choked.second ) {
            buf.append ( std::string ( "\t\t{" ).append ( std::to_string ( angle_delta.first ) ).append ( ",{" ) );

            for ( auto& vel_delta : angle_delta.second ) {
                buf.append ( std::string ( "{" ).append ( std::to_string ( vel_delta.first ) ).append ( ",{" ) );

                for ( auto& playback_rate : vel_delta.second ) {
                    buf.append ( std::string ( "{" ).append ( std::to_string ( playback_rate.first ) ) );
                    buf.append ( std::string ( "," ).append ( std::to_string ( playback_rate.second ) ).append ( "}," ) );
                }

                buf.append ( "}}," );
            }

            buf.append ( "}},\n" );
        }

        buf.append ( "\t}},\n" );
    }

    buf.append ( "};\n" );
    buf.append (
        "\n"
        "}\n"
        "}\n"
        "\n"
    );

    return buf;
}

void anims::logger::init ( ) {

}

void anims::logger::log ( player_t* pl, float desync_amount, int choke_amount ) {
    static bool _init = false;

    if ( !_init ) {
        init ( );
        _init = true;
    }

    if ( !pl->valid ( ) || !( pl->flags()&flags_t::on_ground) || pl->vel ( ).length_2d()<=0.1f || !utils::key_state(VK_DELETE))
        return;

    char dest [ 64 ] { '\0' };
    sprintf_s ( dest, "move_%s", pl->animstate ( )->get_weapon_move_animation ( ) );

    auto seq = pl->lookup_sequence ( dest );

    if ( seq == -1 )
        seq = pl->lookup_sequence ( _ ( "move" ) );

    const auto seqcyclerate = pl->get_sequence_cycle_rate_server ( seq );

    if ( !seqcyclerate || seqcyclerate > 0.33333333f )
        return;

    const auto ratio = std::clamp<float> ( pl->vel ( ).length_2d ( ) / ( 260.0f * seqcyclerate ), 0.0f, 1.0f );

    if ( !ratio )
        return;
    
    const auto rate_adjusted = pl->layers()[6].m_playback_rate / ratio;

    static float last_vel_dir = cs::normalize ( cs::vec_angle ( pl->vel ( ) ).y );
    
    const auto vel_dir = cs::normalize ( cs::vec_angle ( pl->vel ( ) ).y );
    const auto vel_ang_delta = anims::angle_diff ( vel_dir, cs::normalize ( last_vel_dir ) );
    const auto relative_dir = anims::angle_diff ( cs::normalize ( pl->angles ( ).y ), vel_dir );

    last_vel_dir = vel_dir;

    auto to_nearest = [ ] ( int to_round, int multiple ) -> float {
        if ( !multiple )
            return to_round;

        const auto remainder = abs ( to_round ) % multiple;

        if ( !remainder )
            return to_round;

        if ( to_round < 0 )
            return -( abs ( to_round ) - remainder );
        else
            return to_round + multiple - remainder;
    };

    auto nearest_relative_dir = to_nearest ( relative_dir, log_step );
    auto nearest_ang_delta = to_nearest ( vel_ang_delta, log_step );

    auto& entries = log_data [ choke_amount ][ nearest_relative_dir ][ nearest_ang_delta ];

    /* check if theres one we can already replace */
    float* replaceable = nullptr;

    for ( auto& entry : entries ) {
        if ( abs ( entry.first - rate_adjusted ) < 0.000042 ) {
            replaceable = &entry.second;
            break;
        }
    }

    const auto desync_clamped = std::clamp<float> ( desync_amount, -58.0f, 58.0f ) / 58.0f;

    if ( replaceable )
        *replaceable = desync_clamped;
    else
        entries [ rate_adjusted ] = desync_clamped;

    /* output status message w/ debug info */
    static int last_debug_print = time ( nullptr );

    if ( abs ( last_debug_print - time ( nullptr ) ) >= 1 ) {
        cs::i::engine->client_cmd_unrestricted (_("clear") );

        dbg_print ( _ ( "percent filled: %d\n" ), static_cast< int >( static_cast< float >( log_data [ choke_amount ].size ( ) ) / ( 360.0f / log_step ) * 100.0f ) );

        auto angle_entry = 0;

        for ( auto& angle_entries : log_data[ choke_amount ] ) {
            dbg_print ( _ ( "[%d] percent filled: %d\n" ), angle_entry, static_cast< int >( static_cast< float >( angle_entries.second.size ( ) ) / ( 360.0f / log_step ) * 100.0f ) );
            angle_entry++;
        }

        if ( log_data [ choke_amount ].size ( ) == static_cast< int >( 360.0f / log_step ) )
            dbg_print ( _ ( "FILLING COMPLETE\n" ) );

        last_debug_print = time ( nullptr );
    }
}

#endif