#include "animation_system.hpp"
#include "../renderer/render.hpp"
#include "resolver.hpp"
#include "../features/ragebot.hpp"

float feet_weight = 0.0f;
float jump_weight = 0.0f;
float jump_cycle = 0.0f;
float jump_playback_rate = 0.0f;
float landing_playback_rate = 0.0f;
float landing_cycle = 0.0f;
float landing_weight = 0.0f;
int feet_sequence = 0;
int jump_sequence = 0;
std::deque<std::pair<float/*time*/, float/*rate*/>> playback_rates_track {};
std::array< animlayer_t, 13 > latest_animlayers { {} };
std::array< animlayer_t, 13 > latest_animlayers1 { {} };
float old_simtime = 0.0f;

std::array< int, 65 > anims::choked_commands { 0 };
std::array< float, 65 > anims::desync_sign { 1.0f };
std::array< float, 65 > anims::client_feet_playback_rate { 0.0f };
std::array< float, 65 > anims::feet_playback_rate {0.0f };

std::array< std::deque<std::array< animlayer_t, 13 >>, 65 > anims::old_animlayers;

std::array< matrix3x4_t, 128 > anims::fake_matrix { };
std::array< matrix3x4_t, 128 > anims::aim_matrix { };
std::array< std::deque< anims::animation_frame_t >, 65 > anims::frames { {{}} };
/*std::map< std::string, std::array< std::deque< int >, 65 > > anims::choke_sequences {
    {"air", {{}} },
    {"moving_slow", {{}} },
    {"moving", {{}} },
    {"shot", {{}} }
};*/

bool anims::new_tick = false;

std::array< anims::player_data_t, 65 > anims::player_data;

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

//#pragma optimize( "2", on )

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
    static auto init_iks = pattern::search( _( "client.dll" ), _( "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D" ) ).get< void( __thiscall* )( void*, void*, vec3_t&, vec3_t&, float, int, int ) >( );
    static auto update_targets = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F0 81 EC 18 01 00 00 33" ) ).get< void( __thiscall* )( void*, vec3_t*, void*, void*, uint8_t* ) >( );
    static auto solve_dependencies = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F0 81 EC 98 05 00 00 8B 81" ) ).get< void( __thiscall* )( void*, vec3_t*, void*, void*, uint8_t* ) >( );

    /* offset sigs */
    static auto iks_off = pattern::search( _( "client.dll" ), _( "8D 47 FC 8B 8F" ) ).add( 5 ).deref( ).add( 4 ).get< uint32_t >( );
    static auto effects_off = pattern::search( _( "client.dll" ), _( "75 0D 8B 87" ) ).add( 4 ).deref( ).add( 4 ).get< uint32_t >( );

    auto cstudio = *reinterpret_cast< void** >( uintptr_t( target ) + 0x294C );

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
    vfunc< void( __thiscall* )( player_t*, void*, vec3_t*, void*, float, uint32_t ) >( target, standard_blending_rules_vfunc_idx ) ( target, cstudio, pos, q, time, mask );

    //	set iks
    if ( iks ) {
        vfunc< void( __thiscall* )( player_t*, float ) >( target, update_ik_locks_vfunc_idx ) ( target, time );
        update_targets( iks, pos, q, target->bones( ), bone_computed );
        vfunc< void( __thiscall* )( player_t*, float ) >( target, calculate_ik_locks_vfunc_idx ) ( target, time );
        solve_dependencies( iks, pos, q, target->bones( ), bone_computed );
    }

    //	build the matrix
    // IDX = 189 ( 2020.7.10 )
    vfunc< void( __thiscall* )( player_t*, void*, vec3_t*, void*, matrix3x4a_t const&, uint32_t, void* ) >( target, build_transformations_vfunc_idx ) ( target, cstudio, pos, q, base_matrix, mask, bone_computed );

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

	static auto truncate_to = [ ] ( float x, int places ) {
		const auto factor = powf ( 10.0f, places );
		const auto value = static_cast<int> ( x * factor + 0.5f );
		return static_cast< float >(value) / factor;
	};
	
	if ( ent->vel ( ).length_2d ( ) <= 0.1f
		|| !(ent->flags() & 1)
		|| !ent->layers ( ) [ 6 ].m_playback_rate
		|| !ent->weapon ( )
		|| !ent->weapon ( )->data ( )
		/*|| !( ent->vel ( ).length_2d ( ) < ( ent->scoped ( ) ? ent->weapon ( )->data ( )->m_max_speed_alt : ent->weapon ( )->data ( )->m_max_speed ) * 0.34f )*/ )
		return desync_sign [ ent->idx ( ) ];
	
	ent->animstate ( )->m_speed2d = ent->vel ( ).length_2d ( );

	//    get the sequence used in the calc
	char dest [ 64 ] { '\0' };
	sprintf_s ( dest, "move_%s", ent->animstate ( )->get_weapon_move_animation ( ) );

	int seq = ent->lookup_sequence ( dest );

	if ( seq == -1 )
		seq = ent->lookup_sequence ( _ ( "move" ) );

	//    cycle rate 
	float seqcyclerate = ent->get_sequence_cycle_rate_server ( seq );
	
	//feet_playback_rate [ ent->idx ( ) ] = seqcyclerate;

	/* don't log playback rate immediately after player is switching direction of movement, will cause jittering and innacuracies */
	/* once they finish switching directions, we can log again */
	static std::array<float, 65> last_vel_dir { 0.0f };
	static std::array<float, 65> last_vel_magnitude { 0.0f };

	const auto vel_dir = csgo::vec_angle ( ent->vel ( ) );
	const auto vel_ang_delta = anims::angle_diff ( csgo::normalize ( vel_dir.y ), csgo::normalize ( last_vel_dir [ ent->idx ( ) ] ) );
	const auto relative_dir = anims::angle_diff ( csgo::normalize ( ent->angles ( ).y ), csgo::normalize ( vel_dir.y ) );
	const auto vel_mag_delta = ent->vel ( ).length_2d ( ) - last_vel_magnitude [ ent->idx ( ) ];

	last_vel_dir [ ent->idx ( ) ] = vel_dir.y;
	last_vel_magnitude [ ent->idx ( ) ] = ent->vel ( ).length_2d ( );
	
	if ( fabsf ( vel_ang_delta ) > 25.0f /*|| fabsf ( vel_mag_delta ) > 30.0f*/ )
		return desync_sign [ ent->idx ( ) ];

	if ( !seqcyclerate || !( seqcyclerate < 0.3334f || seqcyclerate > 0.4545f ) )
		return desync_sign [ ent->idx ( ) ];
	
	/* ratio of movement to seq. cycle */
	const auto ratio = std::clamp<float> ( ent->vel ( ).length_2d ( ) / ( 260.0f * seqcyclerate ), 0.0f, 1.0f );
	
	if ( !ratio )
		return desync_sign [ ent->idx ( ) ];

	if( seqcyclerate < 0.3334f )
		feet_playback_rate [ ent->idx ( ) ] = ent->layers ( ) [ 6 ].m_playback_rate / ratio;
	else
		feet_playback_rate [ ent->idx ( ) ] = ( ent->layers ( ) [ 6 ].m_playback_rate / ( ent->scoped() ? ent->weapon ( )->data ( )->m_max_speed_alt : ent->weapon ( )->data ( )->m_max_speed ) ) * 1000.0f;

	//static std::array<float, 65> last_playback_rate { 0.0f };
	//static std::array<float, 65> last_playback_rate_check { 0.0f };
	//
	//if ( fabsf ( last_playback_rate_check [ ent->idx ( ) ] - features::prediction::curtime ( ) ) > 0.12f && seqcyclerate < 0.3334f ) {
	//	last_playback_rate [ ent->idx ( ) ] = feet_playback_rate [ ent->idx ( ) ];
	//	last_playback_rate_check [ ent->idx ( ) ] = features::prediction::curtime ( );
	//}
	//
	//if ( fabsf ( last_playback_rate [ ent->idx ( ) ] - feet_playback_rate [ ent->idx ( ) ] ) > 0.0001f && seqcyclerate < 0.3334f )
	//	return desync_sign [ ent->idx ( ) ];

	/* ANG - VEL DELTA LOGS */
	/* TODO: ADD LOG FOR 50% LEFT SIDE DESYNC (EVERYONE USES THIS SAME LOW-DESYNC AMOUNT) */
	/* associate a direction to a rate to a desync delta */
	/* TODO: REMOVE THE VALUES THAT ARE TOO CLOSE TOGETHER AND PICK THE BEST ONE TO KEEP IN HERE */
	static std::map<float, std::vector<std::pair<float, float>>> dir_to_rate = {
		/* FORMAT: */
		/* { ANGLE, { { RATE1, DELTA1 }, { RATE2, DELTA2 }, { RATE3, DELTA3 }, ... } } */
		
		/* positive ang-vel delta */
		{ 0.0f, {{0.004954f, -1.0f}, {0.005028f, -0.5f}, /*{0.004561f, 0.5f},*/ {0.004377f, 1.0f} , {0.004530f, 0.0f}} },
		{ 45.0f, {{0.004691f, -1.0f}, {0.004690f, -0.5f}, /*{0.004932f, 0.5f},*/ {0.004888f , 1.0f}, {0.004187f, 0.0f}} },
		{ 90.0f, {{0.004329f, -1.0f}, {0.004660f, -0.5f}, /*{0.005099f, 0.5f},*/ {0.004945f , 1.0f}, {0.004741f, 0.0f}} },
		{ 135.0f, {{0.004851f, -1.0f}, {0.005069f, -0.5f}, /*{0.005450f, 0.5f},*/ {0.005394f, 1.0f} , {0.004758f, 0.0f}} },
		/* -180 and 180 will be the same */
		{ 180.0f, {{0.004900f, -1.0f}, {0.005214f, -0.5f}, {0.005537f, 0.5f}, {0.005280f, 1.0f} , {0.005243f, 0.0f}} },
		{ -180.0f, {{0.004900f, -1.0f}, {0.005214f, -0.5f}, {0.005537f, 0.5f}, {0.005280f, 1.0f} , {0.005243f, 0.0f}} },
		/* negative ang-vel delta */
		{ -135.0f, {{0.005356f, -1.0f}, {0.005565f, -0.5f}, /*{0.005493f, 0.5f},*/ {0.005242f, 1.0f} ,{ 0.005123f, 0.0f}} },
		{ -90.0f, {{0.005291f, -1.0f}, {0.005471f, -0.5f}, /*{0.005236f, 0.5f},*/ {0.004901f , 1.0f},{ 0.005117f, 0.0f}} },
		{ -45.0f, {{0.005276f, -1.0f}, {0.005361f, -0.5f}, {0.004941f, 0.5f}, {0.004616f , 1.0f}, {0.004778f, 0.0f}} }
	};
	
	static std::map<float, std::vector<std::pair<float, float>>> dir_to_rate_run = {
		/* FORMAT: */
		/* { ANGLE, { { RATE1, DELTA1 }, { RATE2, DELTA2 }, { RATE3, DELTA3 }, ... } } */
	
		/* positive ang-vel delta */
		{ 0.0f, {{0.036710f, -1.0f},  {0.037048f , 1.0f}, {0.032411f, 0.0f}} },
		{ 45.0f, {{0.036157f, -1.0f},{0.038077f , 1.0f}, {0.035659f, 0.0f}} },
		{ 90.0f, {{0.038223f, -1.0f}, {0.038349f , 1.0f}, {0.035127f, 0.0f}} },
		{ 135.0f, {{0.03151f, -1.0f}, {0.039675f, 1.0f} , {0.035849f, 0.0f}} },
		/* -180 and 180 will be the same */
		{ 180.0f, {{0.039249f, -1.0f}, {0.039089f, 1.0f} , {0.037402f, 0.0f}} },
		{ -180.0f, {{0.039249f, -1.0f}, {0.039089f, 1.0f} , {0.037402f, 0.0f}} },
		/* negative ang-vel delta */
		{ -135.0f, {{0.039561f, -1.0f}, {0.038250f, 1.0f} ,{ 0.035660f, 0.0f}} },
		{ -90.0f, {{0.038311f, -1.0f}, {0.037973f , 1.0f},{ 0.035405f, 0.0f}} },
		{ -45.0f, {{0.038050f, -1.0f},  {0.035967f , 1.0f}, {0.035122f, 0.0f}} }
	};
	
	/* find closest ang-vel delta to ours */
	/* avoid copying this as we add more... */
	std::vector<std::pair<float, float>>* nearest_rate_log = nullptr;
	float nearest_rate_diff = FLT_MAX;
	
	for ( auto& entry : ( ( seqcyclerate < 0.3334f ) ? dir_to_rate : dir_to_rate_run ) ) {
		const auto delta_dir = fmodf( fabsf ( entry.first - relative_dir ), 360.0f );

		if ( delta_dir < nearest_rate_diff ) {
			nearest_rate_log = &entry.second;
			nearest_rate_diff = delta_dir;
		}
	}
	
	if ( !nearest_rate_log )
		return desync_sign [ ent->idx ( ) ];

	/* find closest rate to ours */
	const auto rate_adjusted = feet_playback_rate [ ent->idx ( ) ];

	float desync_delta = 0.0f;
	float rate_diff = FLT_MAX;

	for ( auto& entry : *nearest_rate_log ) {
		const auto delta_rate = fabsf ( rate_adjusted - entry.first );

		if ( delta_rate < rate_diff ) {
			desync_delta = entry.second;
			rate_diff = delta_rate;
		}
	}

	if ( rate_diff == FLT_MAX )
		return desync_sign [ ent->idx ( ) ];

	desync_sign [ ent->idx ( ) ] = desync_delta;

	return desync_sign [ ent->idx ( ) ];
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

namespace lby {
	extern bool in_update;
}

void anims::calc_animlayers( player_t* ent ) {
    auto state = ent->animstate( );
    auto layers = ent->layers( );

    if ( !state || !layers )
        return;

	if (ent != g::local ) {
		//feet_playback_rate [ ent->idx ( ) ] = ent->layers ( ) [ 6 ].m_playback_rate;
		client_feet_playback_rate [ ent->idx ( ) ] = calc_feet_cycle ( ent );
	}
	else {
		static vec3_t m_vecLastAcceleratingVelocity = vec3_t( 0.0f, 0.0f, 0.0f );
		static auto m_flFeetWeight = 0.0f;
		static float m_flGoalFeetYaw = 0.0f;
		static float m_flCurrentMoveDirGoalFeetDelta = 0.0f;
		static float m_flDuckRate = 0.0f;
		static float m_flGroundFraction = 0.0f;
		static float m_flSpeed = 0.0f;
		static float lby = 0.0f;
		static float last_lby = 0.0f;
		static float m_flMovePlaybackRate = 0.0f;
		static int m_iMoveState = 0;
		static float m_flTotalTimeInAir = 0.0f;
		static bool before_onground = false;
		static bool m_bOnGround = false;
		static bool m_bJust_LeftGround = false;
		static bool m_bInHitGroundAnimation = false;

		constexpr auto CS_PLAYER_SPEED_DUCK_MODIFIER = 0.340f;
		constexpr auto CS_PLAYER_SPEED_WALK_MODIFIER = 0.520f;
		constexpr auto CS_PLAYER_SPEED_RUN = 260.0f;

		static float ground_fractions = 0.0f;
		static bool old_not_walking = false;

		constexpr auto m_flMaxYaw = -58.0f;
		constexpr auto m_flMinYaw = 58.0f;

		if ( lby::in_update )
			lby = g::angles.y;

		if ( last_lby != ent->lby ( ) )
			lby = ent->lby ( );

		last_lby = ent->lby ( );

		/* SetupVelocity */

		vec3_t m_vVelocityNormalized = ent->vel().normalized();

		float speed = std::min<float> ( ent->vel ( ).length ( ), 260.0f );
		m_flSpeed = speed;

		if ( speed > 0.0f )
			m_vecLastAcceleratingVelocity = m_vVelocityNormalized;
		
		float flMaxMovementSpeed = 260.0f;

		if ( ent->weapon() )
			flMaxMovementSpeed = std::fmax ( ent->weapon ( )->max_speed(), 0.001f );

		float m_flRunningSpeed = m_flSpeed / ( flMaxMovementSpeed * 0.520f );
		float m_flDuckingSpeed = m_flSpeed / ( flMaxMovementSpeed * 0.340f );

		m_flGoalFeetYaw = std::clamp<float> ( m_flGoalFeetYaw, -360.0f, 360.0f );

		float eye_feet_delta = angle_diff ( g::angles.y, m_flGoalFeetYaw );

		float flRunningSpeed = std::clamp<float> ( m_flRunningSpeed, 0.0f, 1.0f );

		float flYawModifier = ( ( ( m_flGroundFraction * -0.3f ) - 0.2f ) * flRunningSpeed ) + 1.0f;

		if ( ent->crouch_amount ( ) > 0.0f ) {
			float flDuckingSpeed = std::clamp<float> ( m_flDuckingSpeed, 0.0f, 1.0f );
			flYawModifier = flYawModifier + ( ( ent->crouch_amount() * flDuckingSpeed ) * ( 0.5f - flYawModifier ) );
		}

		float flMaxYawModifier = flYawModifier * m_flMaxYaw;
		float flMinYawModifier = flYawModifier * m_flMinYaw;

		if ( eye_feet_delta <= flMaxYawModifier ) {
			if ( flMinYawModifier > eye_feet_delta )
				m_flGoalFeetYaw = fabsf ( flMinYawModifier ) + g::angles.y;
		}
		else {
			m_flGoalFeetYaw = g::angles.y - fabs ( flMaxYawModifier );
		}

		m_flGoalFeetYaw = csgo::normalize ( m_flGoalFeetYaw );

		if ( ent->vel().length_2d ( ) > 0.1f || fabsf ( ent->vel ( ).z ) > 100.0f ) {
			m_flGoalFeetYaw = approach_angle (
				g::angles.y,
				m_flGoalFeetYaw,
				( ( state->m_ground_fraction * 20.0f ) + 30.0f )
				* /* m_flLastClientSideAnimationUpdateTimeDelta */ csgo::ticks2time ( 1 ) );
		}
		else {
			m_flGoalFeetYaw = approach_angle (
				lby,
				m_flGoalFeetYaw,
				/* m_flLastClientSideAnimationUpdateTimeDelta */ csgo::ticks2time ( 1 ) * 100.0f );
		}

		/* TODO: FIX LATER, REPLACED WITH GHETTO CALCULATION SO I CAN SKIP REBUILDING A BIG PIECE OF THE ANIM CODE... */
		if ( m_flSpeed > 0.0f ) {
			//turns speed into an angle
			float velAngle = atan2f ( -ent->vel ( ).y, -ent->vel ( ).x ) * 180.0f * ( 1.0f / csgo::pi );

			if ( velAngle < 0.0f )
				velAngle += 360.0f;

			m_flCurrentMoveDirGoalFeetDelta = csgo::normalize ( angle_diff ( velAngle, m_flGoalFeetYaw ) );
		}

		/* SetupMovement */
		
		static auto approach = [ & ] ( float to, float from, float rate ) { 
			float ret = 0.0f;

			if ( to - from <= rate ) {
				if ( fabsf( rate ) <= to - from )
					return to;
				else
					return from - rate;
			}
			
			return rate + from;
		};

		const auto twotickstime = /* m_flLastClientSideAnimationUpdateTimeDelta */ csgo::ticks2time ( 1 ) * 2.0f;

		/* ground fraction */
		if ( ent->flags ( ) & 1 ) {
			if ( old_not_walking )
				ground_fractions -= twotickstime;
			else
				ground_fractions += twotickstime;

			if ( speed > ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && old_not_walking )
				old_not_walking = false;
			else if ( speed < ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && !old_not_walking )
				old_not_walking = true;
		}

		ground_fractions = std::clamp< float > ( ground_fractions, 0.0f, 1.0f );

		m_flGroundFraction = ground_fractions;

		if ( *reinterpret_cast<int*>(uintptr_t(ent) + 0x3984 ) != m_iMoveState )
			m_flMovePlaybackRate += 10.0f;
		
		m_iMoveState = *reinterpret_cast< int* >( uintptr_t ( ent ) + 0x3984 );

		m_flMovePlaybackRate = std::clamp<float> ( approach ( 0.0f, m_flMovePlaybackRate, /* m_flLastClientSideAnimationUpdateTimeDelta */ csgo::ticks2time ( 1 ) * 40.0f ), 0.0f, 100.0f );

		float duckspeed_clamped = std::clamp<float> ( m_flDuckingSpeed, 0.0f, 1.0f );
		float runspeed_clamped = std::clamp<float> ( m_flRunningSpeed, 0.0f, 1.0f );

		float ideal_feet_weight = ( ( duckspeed_clamped - runspeed_clamped ) * ent->crouch_amount() ) + runspeed_clamped;

		if ( ideal_feet_weight < m_flFeetWeight ) {
			float v34 = std::clamp<float> ( m_flMovePlaybackRate * 0.01f, 0.0f, 1.0f );
			float feetweight_speed_change = ( ( v34 * 18.0f ) + 2.0f ) * /* m_flLastClientSideAnimationUpdateTimeDelta */ csgo::ticks2time ( 1 );

			m_flFeetWeight = approach ( ideal_feet_weight, m_flFeetWeight, feetweight_speed_change );
		}
		else {
			m_flFeetWeight = ideal_feet_weight;
		}

		const auto vecDir = csgo::angle_vec( vec3_t(0.0f, csgo::normalize ( m_flCurrentMoveDirGoalFeetDelta + m_flGoalFeetYaw + 180.0f ), 0.0f ) );

		float movement_side = m_vecLastAcceleratingVelocity.dot_product( vecDir );
		if ( movement_side < 0.0f )
			movement_side = -movement_side;

		static auto bias = [ ] ( float x, float biasAmt ) {
			static float lastAmt = -1;
			static float lastExponent = 0;

			if ( lastAmt != biasAmt )
				lastExponent = log ( biasAmt ) * -1.4427f; // (-1.4427 = 1 / log(0.5))

			return powf ( x, lastExponent );
		};

		float newfeetweight = bias ( movement_side, 0.2f ) * m_flFeetWeight;

		m_flFeetWeight = newfeetweight;

		float newfeetweight2 = newfeetweight * m_flDuckRate;

		float layer5_weight = landing_weight /* landing weight */;

		float new_weight = 0.55f;
		if ( 1.0f - layer5_weight > 0.55f )
			new_weight = 1.0f - layer5_weight;

		float new_feet_layer_weight = new_weight * newfeetweight2;
		float feet_cycle_rate = 0.0f;

		char dest [ 64 ] { '\0' };
		sprintf_s ( dest, "move_%s", ent->animstate ( )->get_weapon_move_animation ( ) );

		int seq = ent->lookup_sequence ( dest );

		if ( seq == -1 )
			seq = ent->lookup_sequence ( _ ( "move" ) );

		if ( m_flSpeed > 0.00f ) {
			float seqcyclerate = ent->get_sequence_cycle_rate_server ( seq );

			float seqmovedist = ent->get_sequence_move_distance ( *reinterpret_cast< studiohdr_t** >( uintptr_t ( ent ) + 0x294C ), seq );
			seqmovedist *= 1.0f / ( 1.0f / seqcyclerate );
			if ( seqmovedist <= 0.001f )
				seqmovedist = 0.001f;

			float speed_multiplier = m_flSpeed / seqmovedist;
			feet_cycle_rate = ( 1.0f - ( m_flGroundFraction * 0.15f ) ) * ( speed_multiplier * seqcyclerate );
		}

		float feetcycle_playback_rate = /* m_flLastClientSideAnimationUpdateTimeDelta */ csgo::ticks2time ( 1 ) * feet_cycle_rate;

		playback_rates_track.push_back ( { features::prediction::curtime(), feetcycle_playback_rate } );
		//feet_cycle_track.push_back ( new_cycle );

		feet_weight = std::clamp <float>( new_feet_layer_weight, 0.0f, 1.0f );
		feet_sequence = seq;

		m_flDuckRate = approach ( ( float ) ( ( ent->flags() & 1 ) != 0 ), m_flDuckRate, ( ( ent->crouch_amount() + 1.0f ) * 8.0f ) * /* m_flLastClientSideAnimationUpdateTimeDelta */ csgo::ticks2time ( 1 ) );
		m_flDuckRate = std::clamp<float>( m_flDuckRate, 0.0f, 1.0f );

		bool m_bJust_Landed = ent->flags ( ) & 1 && m_bOnGround && !before_onground;
		before_onground = m_bOnGround;
		m_bJust_LeftGround = m_bOnGround && !( ent->flags ( ) & 1 );
		m_bOnGround = ent->flags ( ) & 1;
		
		jump_sequence = latest_animlayers1 [ 4 ].m_sequence;
		jump_playback_rate = latest_animlayers1 [ 4 ].m_playback_rate;

static float hit_ground_time = 0.0f;
static bool last_ground = false;
bool landed = !last_ground && g::local->flags ( ) & 1;
last_ground = g::local->flags ( ) & 1;

if ( g::local->flags ( ) & 1 ) {
	if ( landed ) {
		float newcycle = ( latest_animlayers1 [ 4 ].m_playback_rate * csgo::ticks2time ( 1 ) ) + jump_cycle;
	
		if ( newcycle >= 1.0f )
			newcycle = 0.999f;
	
		newcycle -= ( float ) ( int ) newcycle; //round to integer
	
		if ( newcycle < 0.0f )
			newcycle += 1.0f;
	
		if ( newcycle > 1.0f )
			newcycle -= 1.0f;
	
		jump_cycle = newcycle;
		hit_ground_time = features::prediction::curtime ( ) - jump_cycle;
	}
	else {
		hit_ground_time = features::prediction::curtime ( );
	}

	jump_weight = approach ( 0.0f, jump_weight, csgo::ticks2time ( 1 ) * 10.0f );
		}
else {
	jump_cycle = features::prediction::curtime ( ) - hit_ground_time;

	static auto get_ideal_weight_from_sequence_cycle = [ & ] ( ) -> float {
		float cycle = jump_cycle;

		if ( cycle >= 0.99f )
			cycle = 1.0f;

		float fadeintime = 0.15f;
		float fadeouttime = 0.95f;
		float weight = 1.0f;
		float v15;

		if ( fadeintime <= 0.0f || fadeintime <= cycle )
		{
			if ( fadeouttime >= 1.0f || cycle <= fadeouttime )
			{
				weight = std::min<float> ( weight, 1.0f );
				return weight;
			}

			v15 = ( cycle - 1.0f ) / ( fadeouttime - 1.0f );
			v15 = std::clamp<float> ( v15, 0.0f, 1.0f );
		}
		else
		{
		v15 = std::clamp<float> ( cycle / fadeintime, 0.0f, 1.0f );
		}

		weight = ( 3.0f - ( v15 + v15 ) ) * ( v15 * v15 );

		if ( weight < 0.0015f )
			weight = 0.0f;
		else
			weight = std::clamp<float> ( weight, 0.0f, 1.0f );

		return weight;
	};

	float layer4_weight = jump_weight;
	float layer4_idealweight = get_ideal_weight_from_sequence_cycle ( );

	if ( layer4_idealweight > layer4_weight )
		jump_weight = layer4_idealweight;
}
	}

	if ( ent == g::local ) {
		latest_animlayers [ 12 ].m_weight = 0.0f;
		layers [ 12 ].m_weight = 0.0f;
	}
	else
		layers [ 12 ].m_weight = 0.0f;
}

void anims::predict_animlayers ( player_t* ent ) {
	while ( !playback_rates_track.empty ( ) && playback_rates_track.size ( ) > 64 )
		playback_rates_track.pop_front ( );

	if ( playback_rates_track.empty ( ) )
		return;

	memcpy ( latest_animlayers.data ( ), latest_animlayers1.data ( ), sizeof ( latest_animlayers ) );

	const auto nci = csgo::i::engine->get_net_channel_info ( );
	const auto latency = nci->get_latency ( 0 ) + nci->get_latency ( 1 );

	/* FEET LAYER */

	for ( auto& rates : playback_rates_track ) {
		/* if we need to predict the cycle from old server data */
		if ( rates.first <= ent->simtime ( ) - latency )
			continue;

		latest_animlayers [ 6 ].m_previous_cycle = latest_animlayers [ 6 ].m_cycle;

		float new_cycle = latest_animlayers [ 6 ].m_cycle + rates.second;

		new_cycle -= floorf ( new_cycle );

		if ( new_cycle < 0.0f )
			new_cycle += 1.0f;

		if ( new_cycle > 1.0f )
			new_cycle -= 1.0f;

		latest_animlayers [ 6 ].m_cycle = new_cycle;
	}

	latest_animlayers [ 6 ].m_sequence = feet_sequence;
	latest_animlayers [ 6 ].m_weight = feet_weight;
	latest_animlayers [ 6 ].m_playback_rate = playback_rates_track.back ( ).second;

	/* JUMP LAYER */
	latest_animlayers [ 4 ].m_playback_rate = 1.0f;
	latest_animlayers [ 4 ].m_weight = jump_weight;
	latest_animlayers [ 4 ].m_cycle = jump_cycle;
}

void anims::calc_poses ( player_t* ent ) {
	static auto jump_fall = [ ] ( float air_time ) {
		const float recalc_air_time = ( air_time - 0.72f ) * 1.25f;
		const float clamped = recalc_air_time >= 0.0f ? std::min< float > ( recalc_air_time, 1.0f ) : 0.0f;
		float out = ( 3.0f - ( clamped + clamped ) ) * ( clamped * clamped );

		if ( out >= 0.0f )
			out = std::min< float > ( out, 1.0f );

		return out;
	};

	auto state = ent->animstate ( );
	auto layers = ent->layers ( );

	if ( !state || !layers )
		return;

	state->m_jump_fall_pose.set_value ( ent, jump_fall ( layers [ 4 ].m_cycle ) );

	//state->m_move_blend_walk_pose.set_value ( ent, ( 1.0f - state->m_ground_fraction ) * ( 1.0f - state->m_duck_amount ) );
	//state->m_move_blend_run_pose.set_value ( ent, ( 1.0f - state->m_duck_amount ) * state->m_ground_fraction );
	//state->m_move_blend_crouch_pose.set_value ( ent, state->m_duck_amount );

	// TODO: FIX THIS MOVE YAW
	auto move_yaw = csgo::normalize ( angle_diff ( csgo::normalize ( csgo::vec_angle ( ent->vel ( ) ).y ), csgo::normalize ( state->m_abs_yaw ) ) );
	if ( move_yaw < 0.0f )
		move_yaw += 360.0f;

	//state->m_speed_pose.set_value ( ent, ent->vel ( ).length_2d ( ) );
	state->m_move_yaw_pose.set_value ( ent, move_yaw / 360.0f );
    
    //state->m_stand_pose.set_value( ent, 1.0f - ( state->m_duck_amount * state->m_unk_fraction ) );

    //auto lean_yaw = csgo::normalize( state->m_eye_yaw + 180.0f );
    //if ( lean_yaw < 0.0f )
    //    lean_yaw += 360.0f;

    state->m_body_pitch_pose.set_value( ent, ( csgo::normalize( state->m_pitch ) + 90.0f ) / 180.0f );
    //state->m_lean_yaw_pose.set_value( ent, 0.0f );
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

	/*
    if ( !( ent->flags( ) & 1 ) )
        return predict_choke( choke_sequences [ _( "air" ) ][ ent->idx( ) ] );
    else if ( ent->weapon( ) && ent->weapon( )->last_shot_time( ) > ent->old_simtime( ) )
        return predict_choke( choke_sequences [ _( "shot" ) ][ ent->idx( ) ] );
    else if ( ent->weapon( ) && ent->weapon( )->data( ) && ent->vel( ).length_2d( ) > 5.0f && ent->vel( ).length_2d( ) <= ent->weapon( )->data( )->m_max_speed * 0.34f )
        return predict_choke( choke_sequences [ _( "moving_slow" ) ][ ent->idx( ) ] );
    else if ( ent->weapon( ) && ent->weapon( )->data( ) && ent->vel( ).length_2d( ) > 5.0f && ent->vel( ).length_2d( ) > ent->weapon( )->data( )->m_max_speed * 0.34f )
        return predict_choke( choke_sequences [ _( "moving" ) ][ ent->idx( ) ] );
		*/

    return current_choked;
}

void anims::update( player_t* ent ) {
    /* update viewmodel manually tox fix it dissappearing*/
    //using update_all_viewmodel_addons_t = int( __fastcall* )( void* );
    //static auto update_all_viewmodel_addons = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 ? 83 EC ? 53 8B D9 56 57 8B 03 FF 90 ? ? ? ? 8B F8 89 7C 24 ? 85 FF 0F 84 ? ? ? ? 8B 17 8B CF" ) ).get< update_all_viewmodel_addons_t >( );
	//
    //if ( ent->viewmodel_handle( ) != -1 && csgo::i::ent_list->get_by_handle< void* >( ent->viewmodel_handle( ) ) )
    //    update_all_viewmodel_addons( csgo::i::ent_list->get_by_handle< void* >( ent->viewmodel_handle( ) ) );

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
        csgo::i::globals->m_curtime = ent->simtime( );

    csgo::i::globals->m_frametime = csgo::ticks2time( 1 );

    /* fix other players' velocity */
    //if ( ent != g::local ) {
        /* skip call to C_BaseEntity::CalcAbsoluteVelocity */
    
	/*if ( ent != g::local )*/ {
		*reinterpret_cast< uint32_t* >( uintptr_t ( ent ) + 0xe8 ) &= ~0x1000;

		/* replace abs velocity (to be recalculated) */
		abs_vel = ent->vel ( );
		//}

		if ( abs_vel.is_valid ( ) ) {
			/* fix abs velocity rounding errors */
			if ( fabsf ( abs_vel.x ) < 0.1f )
				abs_vel.x = 0.0f;

			if ( fabsf ( abs_vel.y ) < 0.1f )
				abs_vel.y = 0.0f;

			if ( fabsf ( abs_vel.z ) < 0.1f )
				abs_vel.z = 0.0f;
		}
	}

    /* fix foot spin */
    state->m_feet_yaw_rate = 0.0f;
    state->m_feet_yaw = state->m_abs_yaw;

    /* force animation update */
    state->m_force_update = true;
	
	if ( ent != g::local ) {
		state->m_last_clientside_anim_update_time_delta = abs(features::prediction::curtime() - state->m_last_clientside_anim_update );
	}

    //state->m_on_ground = ent->flags( ) & 1;

    //if ( state->m_hit_ground )
    //    state->m_time_in_air = 0.0f;

    /* fix on ground */
    //state->m_on_ground = g::local->flags( ) & 1;

    /* remove anim and bone interpolation */
    //*reinterpret_cast< uint32_t* >( uintptr_t( ent ) + 0xf0 ) |= 8;
	//*reinterpret_cast< uint32_t* >( uintptr_t ( ent ) + 0xe8 ) |= 8;
	//*reinterpret_cast< int* >( uintptr_t ( ent ) + 0xA68 ) = 0;

	if ( state->m_last_clientside_anim_framecount == csgo::i::globals->m_framecount)
		state->m_last_clientside_anim_framecount--;

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

	/* fix foot spin */
	//state->m_feet_yaw_rate = 0.0f;
	//state->m_feet_yaw = state->m_abs_yaw;

	//if ( state->m_hit_ground )
	//	state->m_time_in_air = 0.0f;

    /* rebuild poses */
    if ( ent != g::local )
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

    /* make static, allocating too much memory on stack rn, this might be more efficient */
    static animation_frame_t animation_frame;

    animation_frame.store( ent, anim_update );

    float abs_yaw1 = 0.0f, abs_yaw2 = 0.0f, abs_yaw3 = 0.0f;
    animations::resolver::resolve( ent, abs_yaw1, abs_yaw2, abs_yaw3 );

    state->m_feet_yaw = state->m_abs_yaw = abs_yaw1;
    calc_poses( ent );
    memcpy( animation_frame.m_poses1.data( ), ent->poses( ).data( ), sizeof( ent->poses( ) ) );
    build_bones( ent, animation_frame.m_matrix1.data( ), 0x7ff00, vec3_t( 0.0f, abs_yaw1, 0.0f ), ent->origin( ), ent->simtime( ) );

    state->m_feet_yaw = state->m_abs_yaw = abs_yaw2;
    calc_poses( ent );
    memcpy( animation_frame.m_poses2.data( ), ent->poses( ).data( ), sizeof( ent->poses( ) ) );
    build_bones( ent, animation_frame.m_matrix2.data( ), 0x7ff00, vec3_t( 0.0f, abs_yaw2, 0.0f ), ent->origin( ), ent->simtime( ) );

    state->m_feet_yaw = state->m_abs_yaw = abs_yaw3;
    calc_poses( ent );
    memcpy( animation_frame.m_poses3.data( ), ent->poses( ).data( ), sizeof( ent->poses( ) ) );
    build_bones( ent, animation_frame.m_matrix3.data( ), 0x7ff00, vec3_t( 0.0f, abs_yaw3, 0.0f ), ent->origin( ), ent->simtime( ) );

    frames [ ent->idx( ) ].push_back( animation_frame );
}

void anims::animate_player( player_t* ent ) {
    const auto state = ent->animstate( );
    const auto animlayers = ent->layers( );

	auto& data = player_data [ ent->idx ( ) ];

    if ( !state || !animlayers )
		return;

    const auto choked = std::clamp< int >( csgo::time2ticks( ent->simtime( ) - ent->old_simtime( ) ), 0, g::cvars::sv_maxusrcmdprocessticks->get_int() );

    if ( ent->simtime( ) > data.last_update ) {
		choked_commands [ ent->idx ( ) ] = std::clamp<int>(csgo::time2ticks ( ent->simtime ( ) - data.last_update ) - 1, 0, g::cvars::sv_maxusrcmdprocessticks->get_int ( ) );

		data.old_origin = data.last_origin;
		data.old_velocity = data.last_velocity;

        /*auto& air_choke = choke_sequences [ _( "air" ) ][ ent->idx( ) ];
        auto& shot_choke = choke_sequences [ _( "shot" ) ][ ent->idx( ) ];
        auto& moving_slow_choke = choke_sequences [ _( "moving_slow" ) ][ ent->idx( ) ];
        auto& moving_choke = choke_sequences [ _( "moving" ) ][ ent->idx( ) ];

        if ( !( ent->flags( ) & 1 ) )
            air_choke.push_back( choked );
        else if ( ent->weapon( ) && ent->weapon( )->last_shot_time( ) > ent->old_simtime( ) )
            shot_choke.push_back( choked );
        else if ( ent->weapon( ) && ent->weapon( )->data( ) && ent->vel( ).length_2d( ) > 5.0f && ent->vel( ).length_2d( ) <= ent->weapon( )->data( )->m_max_speed * 0.33f )
            moving_slow_choke.push_back( choked );
        else if ( ent->weapon( ) && ent->weapon( )->data( ) && ent->vel( ).length_2d( ) > 5.0f && ent->vel( ).length_2d( ) > ent->weapon( )->data( )->m_max_speed * 0.33f )
            moving_choke.push_back( choked );

        while ( air_choke.size( ) > 32 )
            air_choke.pop_back( );

        while ( shot_choke.size( ) > 32 )
            shot_choke.pop_back( );

        while ( moving_slow_choke.size( ) > 32 )
            moving_slow_choke.pop_back( );

        while ( moving_choke.size( ) > 32 )
            moving_choke.pop_back( );

		std::array<animlayer_t, 13> new_layers;
		memcpy ( new_layers .data(), ent->layers ( ) , sizeof(new_layers));
		old_animlayers [ ent->idx ( ) ].push_back ( new_layers );

		while ( !old_animlayers [ ent->idx ( ) ].empty() && old_animlayers [ ent->idx ( ) ].size ( ) > 3 )
			old_animlayers [ ent->idx ( ) ].pop_front ( );*/
    }

	data.last_origin = ent->origin( );
	data.last_velocity = ent->vel( );

    /* update animations if new data was sent */
    if ( ent->simtime( ) > data.last_update ) {
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
		
		if ( features::ragebot::get_misses ( ent->idx ( ) ).bad_resolve % 3 == 0 )
			state->m_abs_yaw = csgo::normalize ( csgo::normalize ( ent->angles ( ).y ) - ( ( frames [ ent->idx ( ) ].back ( ).m_poses1[11] - 0.5f ) * 120.0f ) );
		else if ( features::ragebot::get_misses ( ent->idx ( ) ).bad_resolve % 3 == 1 )
			state->m_abs_yaw = csgo::normalize ( csgo::normalize ( ent->angles ( ).y ) - ( ( frames [ ent->idx ( ) ].back ( ).m_poses2 [ 11 ] - 0.5f ) * 120.0f ) );
		else
			state->m_abs_yaw = csgo::normalize ( csgo::normalize ( ent->angles ( ).y ) - ( ( frames [ ent->idx ( ) ].back ( ).m_poses3 [ 11 ] - 0.5f ) * 120.0f ) );

		data.last_update = ent->simtime( );

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

void anims::animate_local( bool copy ) {
	if ( !g::local->valid ( ) )
		return;

    const auto state = g::local->animstate( );
    const auto animlayers = g::local->layers( );

    if ( !state || !animlayers || !g::ucmd )
        return;

    static float latest_abs_yaw = 0.0f;
    static std::array< float, 24 > latest_poses { 0.0f };

	if ( !copy ) {
		if ( g::send_packet ) {
			predict_animlayers ( g::local );
		}

		memcpy ( animlayers, latest_animlayers.data ( ), sizeof ( latest_animlayers ) );

		state->m_last_clientside_anim_update_time_delta = csgo::ticks2time(1);

		animlayers [ 12 ].m_weight = 0.0f;

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

		const auto backup_curtime = csgo::i::globals->m_curtime;
		csgo::i::globals->m_curtime = features::prediction::curtime ( );

		update ( g::local );

		if ( g::send_packet ) {
			memcpy ( latest_poses.data ( ), g::local->poses ( ).data ( ), sizeof ( latest_poses ) );
			latest_abs_yaw = state->m_abs_yaw;
			g::hold_aim = false;
		}

		build_bones ( g::local, aim_matrix.data ( ), 0x7ff00, vec3_t ( 0.0f, latest_abs_yaw, 0.0f ), g::local->origin ( ), features::prediction::curtime ( ) );

		animate_fake ( );

		csgo::i::globals->m_curtime = backup_curtime;
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

    if ( !state || !animlayers )
        return;
	
	ent->set_abs_angles ( vec3_t ( 0.0f, state->m_abs_yaw, 0.0f ) );

	if ( frames [ ent->idx ( ) ].empty ( ) )
		return;

    /* get animation update, use this record to render */
    const auto target_frame = std::find_if( frames [ ent->idx( ) ].begin( ), frames [ ent->idx( ) ].end( ), [ & ] ( const animation_frame_t& frame ) { return frame.m_anim_update; } );

    if ( target_frame == frames [ ent->idx( ) ].end( ) )
        return;

    memcpy( ent->layers( ), target_frame->m_animlayers.data( ), sizeof( target_frame->m_animlayers ) );

    if ( features::ragebot::get_misses( ent->idx( ) ).bad_resolve % 3 == 0 )
        memcpy( ent->poses( ).data( ), target_frame->m_poses1.data( ), sizeof( ent->poses( ) ) );
    else if ( features::ragebot::get_misses( ent->idx( ) ).bad_resolve % 3 == 1 )
        memcpy( ent->poses( ).data( ), target_frame->m_poses2.data( ), sizeof( ent->poses( ) ) );
    else
        memcpy( ent->poses( ).data( ), target_frame->m_poses3.data( ), sizeof( ent->poses( ) ) );
}

void anims::run( int stage ) {
    if ( !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) )
        return;

    switch ( stage ) {
        case 5: { /* FRAME_RENDER_START */
            for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
                //if(g::local && g::local->alive())
				if(i > 0 && i <= 64 )
					animations::resolver::process_event_buffer ( i );

                const auto ent = csgo::i::ent_list->get< player_t* >( i );

				//if (!ent || !ent->is_player() || !ent->alive())
				//	features::ragebot::get_misses ( i ).bad_resolve = 0;

                if ( ent && ent->is_player() && ent->alive( ) && !ent->dormant( ) && ent != g::local ) {
                    *reinterpret_cast< int* >( uintptr_t( ent ) + 0xA30 ) = csgo::i::globals->m_framecount;
                    *reinterpret_cast< int* >( uintptr_t( ent ) + 0xA28 ) = 0;
					*reinterpret_cast< int* >( uintptr_t ( ent ) + 0xA68 ) = 0;
					//*reinterpret_cast< int* >( uintptr_t ( ent ) + 0x26F8 + 4 ) = 0;
                    copy_data( ent );
                }

				if ( ent && ent->client_class ( ) && ent->client_class ( )->m_class_id == 42 ) {
					//*reinterpret_cast< int* >( uintptr_t ( ent ) + 0x26F8 + 4 ) = 0;
				}
            }
			
            animate_local( true );
        } break;
        case 4: { /* FRAME_NET_UPDATE_END */
            csgo::for_each_player( [ & ] ( player_t* ent ) {
                if ( ent == g::local )
                    return;

				//interpolate ( ent, false );
				animate_player ( ent );
                } );
        } break;
        case 2: { /* FRAME_NET_UPDATE_POSTDATAUPDATE_START */
            //csgo::for_each_player( [ & ] ( player_t* ent ) {
            //    if ( ent == g::local )
            //        return;
			//
            //    //interpolate( ent, false );
            //    } );

			if ( g::local->valid() && g::local->simtime ( ) != old_simtime ) {
				if ( g::local->layers ( ) ) {
					memcpy ( latest_animlayers1.data ( ), g::local->layers ( ), sizeof ( latest_animlayers1 ) );

					//predict_animlayers ( g::local );
				}

				old_simtime = g::local->simtime ( );
			}
        } break;
        case 3: { /* FRAME_NET_UPDATE_POSTDATAUPDATE_END */
            //csgo::for_each_player( [ & ] ( player_t* ent ) {
            //    if ( ent == g::local )
            //        return;
			//
            //    /* fix pvs */
            //    *reinterpret_cast< int* >( uintptr_t( ent ) + 0xA30 ) = csgo::i::globals->m_framecount;
            //    *reinterpret_cast< int* >( uintptr_t( ent ) + 0xA28 ) = 0;
			//	*reinterpret_cast< int* >( uintptr_t ( ent ) + 0xA68 ) = 0;
            //    } );
        } break;
    }
}

//#pragma optimize( "2", off )