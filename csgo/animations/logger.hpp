#pragma once
#include "../sdk/sdk.hpp"

//#define ANIMATION_LOGGER

#ifdef ANIMATION_LOGGER

namespace anims {
    /*
    *   --ACCOUNT FOR--
    *   1. angle changer per tick
    *   2. angle dir
    *   3. last playback rate to current playback rate change
    */

    /*
    *   !!DATA STRUCTURE AND INFO!!
    *   --SEARCH SEQUENCE--
    *   ->  choke_ticks
    *   ->  ang_delta(vec_angle(vel).y,angle.y)
    *   ->  ang_delta(vec_angle(vel).y, vec_angle(old_vel).y)
    *   ->  playback_rate
    *   ->  sign
    */

    namespace logger {
        constexpr auto choke_limit = 16;
        constexpr auto log_step = 12.0f; /* 12 deg before new animation data is recorded */

        std::string dump ( );
        void init ( );
        void log ( player_t* pl, float desync_amount, int choke_amount );
    }
}

#endif