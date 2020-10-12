#pragma once
#include <deque>
#include <array>
#include "../sdk/sdk.hpp"

namespace anims {
    struct animation_frame_t {
        std::array< animlayer_t, 13 > m_animlayers { {} };
        std::array< float, 24 > m_poses1;
        std::array< float, 24 > m_poses2;
        std::array< float, 24 > m_poses3;
        float m_simtime;
        float m_old_simtime;
        animstate_t m_animstate;
        uint32_t m_flags;
        vec3_t m_velocity;
        vec3_t m_origin;

        bool m_anim_update;

        std::array< matrix3x4_t, 128 > m_matrix1 { };
        std::array< matrix3x4_t, 128 > m_matrix2 { };
        std::array< matrix3x4_t, 128 > m_matrix3 { };

        void store( player_t* ent, bool anim_update );
    };

	struct player_data_t {
		float spawn_times { 0.0f };
		float last_update { 0.0f };
		vec3_t last_origin { vec3_t ( ) };
		vec3_t old_origin { vec3_t ( ) };
		vec3_t last_velocity { vec3_t ( ) };
		vec3_t old_velocity { vec3_t ( ) };
	};

	extern std::array< int, 65 > choked_commands;
	extern std::array< float, 65 > desync_sign;
	extern std::array< float, 65 > client_feet_playback_rate;
	extern std::array< float, 65 > feet_playback_rate;
	extern std::array< std::deque<std::array< animlayer_t, 13 >>, 65 > old_animlayers;
    extern std::array< matrix3x4_t, 128 > fake_matrix;
    extern std::array< matrix3x4_t, 128 > aim_matrix;
    extern std::array< std::deque< animation_frame_t >, 65 > frames;
    //extern std::map< std::string, std::array< std::deque< int >, 65 > > choke_sequences;
	extern std::array< player_data_t, 65 > player_data;
    extern bool new_tick;

    float angle_mod( float a );
    float approach_angle( float target, float value, float speed );
    float angle_diff( float dst, float src );
    bool build_bones( player_t* target, matrix3x4_t* mat, int mask, vec3_t rotation, vec3_t origin, float time );
    void interpolate( player_t* ent, bool should_interp );
	float calc_feet_cycle ( player_t* ent );
    void calc_animlayers( player_t* ent );
	void predict_animlayers ( player_t* ent );
    void calc_local_exclusive( float& ground_fraction_out, float& air_time_out );
	void predict_animlayers ( player_t* ent );
    void calc_poses( player_t* ent );
    int predict_choke_sequence( player_t* ent );
    void update( player_t* ent );
    void store_frame( player_t* ent, bool anim_update );
    void animate_player( player_t* ent );
    void animate_local( bool copy = false );
    void animate_fake( );
    void copy_data( player_t* ent );
    void run( int stage );
}