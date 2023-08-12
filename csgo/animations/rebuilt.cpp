#include "rebuilt.hpp"
#include "anims.hpp"
#include "../menu/options.hpp"
#include "../sdk/netvar.hpp"

#include <numbers>

#undef min
#undef max

constexpr auto CLIENT_DLL_ANIMS = 0;

constexpr auto ANIMATION_LAYER_COUNT = 13;

constexpr auto CSGO_ANIM_DUCK_APPROACH_SPEED_DOWN = 3.1f;
constexpr auto CSGO_ANIM_DUCK_APPROACH_SPEED_UP = 6.0f;

namespace valve_math {
	//-----------------------------------------------------------------------------
	// Euler QAngle -> Basis Vectors.  Each vector is optional
	//-----------------------------------------------------------------------------
	__forceinline void AngleVectors ( const vec3_t& angles, vec3_t* forward, vec3_t* right, vec3_t* up ) {
		float sr, sp, sy, cr, cp, cy;

		cs::sin_cos ( cs::deg2rad ( angles.y ), &sy, &cy );
		cs::sin_cos ( cs::deg2rad ( angles.x ), &sp, &cp );
		cs::sin_cos ( cs::deg2rad ( angles.z ), &sr, &cr );

		if ( forward ) {
			forward->x = cp * cy;
			forward->y = cp * sy;
			forward->z = -sp;
		}

		if ( right ) {
			right->x = ( -1.0f * sr * sp * cy + -1.0f * cr * -sy );
			right->y = ( -1.0f * sr * sp * sy + -1.0f * cr * cy );
			right->z = -1.0f * sr * cp;
		}

		if ( up ) {
			up->x = ( cr * sp * cy + -sr * -sy );
			up->y = ( cr * sp * sy + -sr * cy );
			up->z = cr * cp;
		}
	}

	__forceinline float RemapValClamped ( float val, float A, float B, float C, float D ) {
		if ( A == B )
			return val >= B ? D : C;
		float cVal = ( val - A ) / ( B - A );
		cVal = std::clamp ( cVal, 0.0f, 1.0f );

		return C + ( D - C ) * cVal;
	}

	__forceinline float anglemod( float a ) {
		a = ( 360.f / 65536 ) * ( ( int ) ( a * ( 65536.f / 360.0f ) ) & 65535 );
		return a;
	}

	__forceinline float Approach( float target , float value , float speed ) {
		float delta = target - value;

		if ( delta > speed )
			value += speed;
		else if ( delta < -speed )
			value -= speed;
		else
			value = target;

		return value;
	}

	__forceinline float ApproachAngle( float target , float value , float speed ) {
		target = anglemod( target );
		value = anglemod( value );

		float delta = target - value;

		// Speed is assumed to be positive
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
	}


	__forceinline float AngleDiff( float destAngle , float srcAngle ) {
		float delta;

		delta = fmodf( destAngle - srcAngle , 360.0f );
		if ( destAngle > srcAngle )
		{
			if ( delta >= 180 )
				delta -= 360;
		}
		else
		{
			if ( delta <= -180 )
				delta += 360;
		}
		return delta;
	}

	__forceinline float AngleDistance( float next , float cur ) {
		float delta = next - cur;

		if ( delta < -180 )
			delta += 360;
		else if ( delta > 180 )
			delta -= 360;

		return delta;
	}

	__forceinline float AngleNormalize( float angle ) {
		angle = fmodf( angle , 360.0f );
		if ( angle > 180 )
		{
			angle -= 360;
		}
		if ( angle < -180 )
		{
			angle += 360;
		}
		return angle;
	}

	__forceinline float Bias( float x , float biasAmt ) {
		// WARNING: not thread safe
		static float lastAmt = -1.0f;
		static float lastExponent = 0.0f;

		if ( lastAmt != biasAmt )
			lastExponent = log( biasAmt ) * -1.4427f; // (-1.4427 = 1 / log(0.5))

		return pow( x , lastExponent );
	}

	__forceinline float Gain( float x , float biasAmt ) {
		// WARNING: not thread safe
		if ( x < 0.5f )
			return 0.5f * Bias( 2.0f * x , 1.0f - biasAmt );
		
		return 1.0f - 0.5f * Bias( 2.0f - 2.0f * x , 1.0f - biasAmt );
	}

	__forceinline vec3_t ApproachVector ( vec3_t& a, vec3_t& b, float rate ) {
		const auto delta = a - b;
		const auto delta_len = delta.length ( );

		if ( delta_len <= rate ) {
			vec3_t result;

			if ( -rate <= delta_len ) {
				return a;
			}
			else {
				const auto iradius = 1.0f / ( delta_len + std::numeric_limits<float>::epsilon ( ) );
				return b - ( ( delta * iradius ) * rate );
			}
		}
		else {
			const auto iradius = 1.0f / ( delta_len + std::numeric_limits<float>::epsilon ( ) );
			return b + ( ( delta * iradius ) * rate );
		}
	};
}

void anims::rebuilt::update_layer ( animstate_t* anim_state, int layer, int seq, float playback_rate, float weight, float cycle ) {
	static auto CCSGOPlayerAnimState__UpdateLayerOrderPreset = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 51 53 56 57 8B F9 83 7F 60" ) ).get<void*> ( );

	MDLCACHE_CRITICAL_SECTION ( );

	const auto _layer = anim_state->m_entity->layers ( ) + layer;

	if ( _layer->m_owner && _layer->m_sequence != seq )
		invalidate_physics_recursive ( anim_state, 16 );

	_layer->m_sequence = seq;
	set_rate ( anim_state, layer, playback_rate );
	set_cycle ( anim_state, layer, std::clamp( cycle, 0.0f, 1.0f ) );
	set_weight ( anim_state, layer, std::clamp ( weight, 0.0f, 1.0f ) );

	__asm {
		mov ecx, anim_state
		movss xmm0, weight
		push seq
		push layer
		call CCSGOPlayerAnimState__UpdateLayerOrderPreset
	}
}

void anims::rebuilt::invalidate_physics_recursive( animstate_t* anim_state , int flags ) {
	static auto C_BaseAnimating__InvalidatePhysicsRecursive = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 0C 53 8B 5D 08 8B C3 56" ) ).get<void( __thiscall* )( player_t* , int )>( );
	C_BaseAnimating__InvalidatePhysicsRecursive( anim_state->m_entity, flags );
}

/* animstate struct for IDA */
/*
struct vec3_t {
	float x;
	float y; 
	float z;
};

struct vec2_t {
	float x;
	float y;
};

struct animstate_pose_param_cache_t {
	char pad0 [ 4];
	int m_idx; 
	char* m_name;
};

struct animstate_t {
	char pad0[4];
	bool m_force_update;
	char pad1[90];
	void* m_entity;
	void* m_weapon;
	void* m_last_weapon;
	float m_last_clientside_anim_update;
	int m_last_clientside_anim_framecount;
	float m_last_clientside_anim_update_time_delta;
	float m_eye_yaw;
	float m_pitch;
	float m_abs_yaw;
	float m_feet_yaw;
	float m_body_yaw;
	float m_body_yaw_clamped;
	float m_feet_vel_dir_delta;
	char pad2[4];
	float m_feet_cycle;
	float m_feet_yaw_rate;
	char pad3[4];
	float m_duck_amount;
	float m_landing_duck_additive;
	char pad4[4];
	vec3_t m_origin; 
	vec3_t m_old_origin; 
	vec3_t m_vel;
	vec3_t m_vel_norm;
	vec3_t m_vel_norm_nonzero;
	float m_speed2d; 
	float m_up_vel;
	float m_speed_normalized; 
	float m_run_speed; 
	float m_unk_feet_speed_ratio; 
	float m_time_since_move; 
	float m_time_since_stop; 
	bool m_on_ground; 
	bool m_hit_ground; 
	char pad5[4];
	float m_time_in_air; 
	char pad6[6];
	float m_ground_fraction; 
	char pad7[2];
	float m_unk_fraction; 
	char pad8[12];
	bool m_moving; 
	char pad9[123];
	animstate_pose_param_cache_t m_lean_yaw_pose;
	animstate_pose_param_cache_t m_speed_pose;
	animstate_pose_param_cache_t m_ladder_speed_pose; 
	animstate_pose_param_cache_t m_ladder_yaw_pose;
	animstate_pose_param_cache_t m_move_yaw_pose; 
	animstate_pose_param_cache_t m_run_pose; 
	animstate_pose_param_cache_t m_body_yaw_pose; 
	animstate_pose_param_cache_t m_body_pitch_pose; 
	animstate_pose_param_cache_t m_dead_yaw_pose;
	animstate_pose_param_cache_t m_stand_pose;
	animstate_pose_param_cache_t m_jump_fall_pose; 
	animstate_pose_param_cache_t m_aim_blend_stand_idle_pose; 
	animstate_pose_param_cache_t m_aim_blend_crouch_idle_pose; 
	animstate_pose_param_cache_t m_strafe_yaw_pose; 
	animstate_pose_param_cache_t m_aim_blend_stand_walk_pose; 
	animstate_pose_param_cache_t m_aim_blend_stand_run_pose; 
	animstate_pose_param_cache_t m_aim_blend_crouch_walk_pose; 
	animstate_pose_param_cache_t m_move_blend_walk_pose;
	animstate_pose_param_cache_t m_move_blend_run_pose;
	animstate_pose_param_cache_t m_move_blend_crouch_pose;
	char pad10[4];
	float m_vel_unk;
	char pad11[134];
	float m_min_yaw;
	float m_max_yaw;
	float m_max_pitch;
	float m_min_pitch;
};
*/

void anims::rebuilt::reset_layer( animstate_t* anim_state , int layer ) {
	const auto _layer = anim_state->m_entity->layers( ) + layer;

	if ( _layer->m_owner ) {
		int flags = 0;

		if ( _layer->m_sequence || _layer->m_weight )
			flags |= 16;

		if ( _layer->m_cycle )
			flags |= 8;

		if ( flags )
			invalidate_physics_recursive( anim_state, flags );
	}

	_layer->m_sequence = 0;
	_layer->m_previous_cycle = 0.0f;
	_layer->m_weight = 0.0f;
	_layer->m_playback_rate = 0.0f;
	_layer->m_cycle = 0.0f;

	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( _layer ) + 0 ) = 0.0f; // m_flLayerAnimtime
	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( _layer ) + 4 ) = 0.0f; // m_flLayerFadeOuttime
}

void anims::rebuilt::set_sequence( animstate_t* anim_state , int layer , int sequence ) {
	static auto CCSGOPlayerAnimState__UpdateLayerOrderPreset = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 51 53 56 57 8B F9 83 7F 60" ) ).get<void*> ( );

	MDLCACHE_CRITICAL_SECTION ( );

	const auto _layer = anim_state->m_entity->layers( ) + layer;

	if ( _layer->m_owner && _layer->m_sequence != sequence )
		invalidate_physics_recursive( anim_state, 16 );

	_layer->m_sequence = sequence;
	set_rate ( anim_state, layer, anim_state->m_entity->get_sequence_cycle_rate_server ( sequence ) );
	set_cycle ( anim_state, layer, 0.0f );
	set_weight ( anim_state, layer, 0.0f );

	auto zero = 0.0f;
	__asm {
		mov ecx, anim_state
		movss xmm0, zero
		push sequence
		push layer
		call CCSGOPlayerAnimState__UpdateLayerOrderPreset
	}
}

void anims::rebuilt::set_cycle( animstate_t* anim_state , int layer , float cycle ) {
	const auto _layer = anim_state->m_entity->layers( ) + layer;

	auto clamp_cycle = [ ] ( float in ) {
		in -= int( in );

		if ( in < 0.0f )
			in += 1.0f;
		else if ( in > 1.0f )
			in -= 1.0f;

		return in;
	};

	const auto clamped_cycle = clamp_cycle( cycle );

	if ( _layer->m_owner && _layer->m_cycle != clamped_cycle )
		invalidate_physics_recursive( anim_state , 8 );

	_layer->m_cycle = clamped_cycle;
}

bool anims::rebuilt::is_sequence_completed( animstate_t* anim_state , int layer ) {
	return anim_state->m_entity->layers( )[ layer ].m_cycle + anim_state->m_last_clientside_anim_update_time_delta * anim_state->m_entity->layers( )[ layer ].m_playback_rate >= 1.0f;
}

void anims::rebuilt::increment_layer_cycle_weight_rate_generic( animstate_t* anim_state , int layer ) {
	float flWeightPrevious = anim_state->m_entity->layers( )[layer].m_weight;
	increment_layer_cycle( anim_state, layer , false );
	set_weight( anim_state , layer , get_layer_ideal_weight_from_seq_cycle( anim_state , layer ) );
	set_weight_rate( anim_state , layer , flWeightPrevious );
}

void anims::rebuilt::set_order( animstate_t* anim_state , int layer , int order ) {
	const auto _layer = anim_state->m_entity->layers( ) + layer;

	if ( _layer->m_owner && _layer->m_order != order )
		if ( _layer->m_order == ANIMATION_LAYER_COUNT || order == ANIMATION_LAYER_COUNT )
			invalidate_physics_recursive( anim_state , 16 );

	_layer->m_order = order;
}

void anims::rebuilt::set_weight( animstate_t* anim_state , int layer , float weight ) {
	const auto _layer = anim_state->m_entity->layers( ) + layer;

	if ( _layer->m_owner && _layer->m_weight != weight )
		if ( !_layer->m_weight || !weight )
			invalidate_physics_recursive( anim_state , 16 );

	_layer->m_weight = weight;
}

void anims::rebuilt::set_rate( animstate_t* anim_state , int layer , float rate ) {
	const auto _layer = anim_state->m_entity->layers( ) + layer;

	_layer->m_playback_rate = rate;
}

void anims::rebuilt::increment_layer_cycle( animstate_t* anim_state , int layer , bool allow_loop ) {
	const auto _layer = anim_state->m_entity->layers( ) + layer;

	if ( abs( _layer->m_playback_rate ) <= 0.0f )
		return;

	float flCurrentCycle = anim_state->m_entity->layers( )[ layer ].m_cycle;
	flCurrentCycle += anim_state->m_last_clientside_anim_update_time_delta * anim_state->m_entity->layers( )[ layer ].m_playback_rate;

	if ( !allow_loop && flCurrentCycle >= 1.0f )
		flCurrentCycle = 0.999f;

	set_cycle( anim_state, layer, flCurrentCycle );
}

void anims::rebuilt::increment_layer_weight( animstate_t* anim_state , int layer ) {
	static auto CCSGOPlayerAnimState__IncrementLayerWeight = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 30 56 57 8B 3D" ) ).get<void( __thiscall* )( animstate_t*, int )>( );
	CCSGOPlayerAnimState__IncrementLayerWeight( anim_state , layer );
}

void anims::rebuilt::set_weight_rate( animstate_t* anim_state , int layer , float previous ) {
	if ( !anim_state->m_last_clientside_anim_update_time_delta )
		return;

	const auto _layer = anim_state->m_entity->layers( ) + layer;
	const auto new_rate = ( _layer->m_weight - previous ) / anim_state->m_last_clientside_anim_update_time_delta;

	_layer->m_weight_delta_rate = new_rate;
}

int anims::rebuilt::get_layer_activity( animstate_t* anim_state , int layer ) {
	static auto CCSGOPlayerAnimState__GetLayerActivity = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 EC 08 53 56 8B 35 ? ? ? ? 57 8B F9 8B CE 8B 06 FF 90 84 00 00 00 8B 7F 60 83 BF ? ? ? ? 00" ) ).get<int( __thiscall* )( animstate_t* , int )>( );
	return CCSGOPlayerAnimState__GetLayerActivity( anim_state, layer );
}

int anims::rebuilt::select_sequence_from_act_mods( animstate_t* anim_state , int act ) {
	//MDLCACHE_CRITICAL_SECTION( );
	//return nSelectedActivity;
	return 0;
}

float anims::rebuilt::get_layer_ideal_weight_from_seq_cycle( animstate_t* anim_state , int layer ) {
	static auto CCSGOPlayerAnimState__GetLayerIdealWeightFromSeqCycle = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 EC 08 53 56 8B 35 ? ? ? ? 57 8B F9 8B CE 8B 06 FF 90 84 00 00 00 8B 7F 60 0F 57 DB" ) ).get<void*> ( );

	auto ret = 1.0f;

	__asm {
		push layer
		mov ecx, anim_state
		call CCSGOPlayerAnimState__GetLayerIdealWeightFromSeqCycle
		movss ret, xmm0
	}

	return ret;
}

float anims::rebuilt::get_first_sequence_anim_tag( animstate_t* anim_state , int seq , int tag , float start , float end ) {
	static auto CCSPlayer__GetFirstSequenceAnimTag = pattern::search( _( "client.dll" ) , _( "E8 ? ? ? ? F3 0F 11 86 98 00 00 00 0F 57 DB F3 0F 10 86 24 01 00 00" ) ).resolve_rip().get<float( __thiscall* )( player_t*, int, int, float, float )>( );
	
	const auto player = anim_state->m_entity;
	const auto mdl_ptr = get_model_ptr ( player );
	auto ret = 0.0f;

	__asm {
		push mdl_ptr
		push tag
		push seq
		mov ecx, player
		call CCSPlayer__GetFirstSequenceAnimTag
		movss ret, xmm0
	}

	return ret;
}

void anims::rebuilt::bone_shapshot_should_capture( void* bone_snapshot , float a2 ) {
	static auto CBoneShapshot__SetShouldCapture = pattern::search( _( "client.dll" ) , _( "0F 57 D2 C6 81" ) ).get<void( __thiscall* )( void* , float )>( );
	CBoneShapshot__SetShouldCapture( bone_snapshot, a2 );
}

void* anims::rebuilt::get_model_ptr( player_t* player ) {
	static auto CCSPlayer__GetModelPtr = pattern::search( _( "client.dll" ) , _( "E8 ? ? ? ? 83 C4 04 8B C8 E8 ? ? ? ? 83 B8 C4 00 00 00 00" ) ).resolve_rip( ).get<void* ( __thiscall* )( void* )>( );
	return CCSPlayer__GetModelPtr( player );
}

void* anims::rebuilt::seq_desc( void* mdl , int seq ) {
	static auto pSeqdesc = pattern::search( _( "client.dll" ) , _( "E8 ? ? ? ? 83 C4 04 8B C8 E8 ? ? ? ? 83 B8 C4 00 00 00 00" ) ).add(10).resolve_rip( ).get<void* ( __thiscall* )( void*, int )>( );
	return pSeqdesc( mdl, seq );
}

enum {
	act_invalid = -1,			// so we have something more succint to check for than '-1'
	act_reset = 0,				// set m_activity to this invalid value to force a reset to m_idealactivity
	act_idle,
	act_transition,
	act_cover,					// fixme: obsolete? redundant with act_cover_low?
	act_cover_med,				// fixme: unsupported?
	act_cover_low,				// fixme: rename act_idle_crouch?
	act_walk,
	act_walk_aim,
	act_walk_crouch,
	act_walk_crouch_aim,
	act_run,
	act_run_aim,
	act_run_crouch,
	act_run_crouch_aim,
	act_run_protected,
	act_script_custom_move,
	act_range_attack1,
	act_range_attack2,
	act_range_attack1_low,		// fixme: not used yet, crouched versions of the range attack
	act_range_attack2_low,		// fixme: not used yet, crouched versions of the range attack
	act_diesimple,
	act_diebackward,
	act_dieforward,
	act_dieviolent,
	act_dieragdoll,
	act_fly,				// fly (and flap if appropriate)
	act_hover,
	act_glide,
	act_swim,
	act_jump,
	act_hop,				// vertical jump
	act_leap,				// long forward jump
	act_land,
	act_climb_up,
	act_climb_down,
	act_climb_dismount,
	act_shipladder_up,
	act_shipladder_down,
	act_strafe_left,
	act_strafe_right,
	act_roll_left,			// tuck and roll, left
	act_roll_right,			// tuck and roll, right
	act_turn_left,			// turn quickly left (stationary)
	act_turn_right,			// turn quickly right (stationary)
	act_crouch,				// fixme: obsolete? only used be soldier (the act of crouching down from a standing position)
	act_crouchidle,			// fixme: obsolete? only used be soldier (holding body in crouched position (loops))
	act_stand,				// fixme: obsolete? should be transition (the act of standing from a crouched position)
	act_use,
	act_alien_burrow_idle,
	act_alien_burrow_out,

	act_signal1,
	act_signal2,
	act_signal3,

	act_signal_advance,		// squad handsignals, specific.
	act_signal_forward,
	act_signal_group,
	act_signal_halt,
	act_signal_left,
	act_signal_right,
	act_signal_takecover,

	act_lookback_right,		// look back over shoulder without turning around.
	act_lookback_left,
	act_cower,				// fixme: unused, should be more extreme version of crouching
	act_small_flinch,		// fixme: needed? shouldn't flinching be down with overlays?
	act_big_flinch,
	act_melee_attack1,
	act_melee_attack2,
	act_reload,
	act_reload_start,
	act_reload_finish,
	act_reload_low,
	act_arm,				// pull out gun, for instance
	act_disarm,				// reholster gun
	act_drop_weapon,
	act_drop_weapon_shotgun,
	act_pickup_ground,		// pick up something in front of you on the ground
	act_pickup_rack,		// pick up something from a rack or shelf in front of you.
	act_idle_angry,			// fixme: being used as an combat ready idle?  alternate idle animation in which the monster is clearly agitated. (loop)

	act_idle_relaxed,
	act_idle_stimulated,
	act_idle_agitated,
	act_idle_stealth,
	act_idle_hurt,

	act_walk_relaxed,
	act_walk_stimulated,
	act_walk_agitated,
	act_walk_stealth,

	act_run_relaxed,
	act_run_stimulated,
	act_run_agitated,
	act_run_stealth,

	act_idle_aim_relaxed,
	act_idle_aim_stimulated,
	act_idle_aim_agitated,
	act_idle_aim_stealth,

	act_walk_aim_relaxed,
	act_walk_aim_stimulated,
	act_walk_aim_agitated,
	act_walk_aim_stealth,

	act_run_aim_relaxed,
	act_run_aim_stimulated,
	act_run_aim_agitated,
	act_run_aim_stealth,

	act_crouchidle_stimulated,
	act_crouchidle_aim_stimulated,
	act_crouchidle_agitated,

	act_walk_hurt,			// limp  (loop)
	act_run_hurt,			// limp  (loop)
	act_special_attack1,	// very monster specific special attacks.
	act_special_attack2,
	act_combat_idle,		// fixme: unused?  agitated idle.
	act_walk_scared,
	act_run_scared,
	act_victory_dance,		// killed a player, do a victory dance.
	act_die_headshot,		// die, hit in head. 
	act_die_chestshot,		// die, hit in chest
	act_die_gutshot,		// die, hit in gut
	act_die_backshot,		// die, hit in back
	act_flinch_head,
	act_flinch_chest,
	act_flinch_stomach,
	act_flinch_leftarm,
	act_flinch_rightarm,
	act_flinch_leftleg,
	act_flinch_rightleg,
	act_flinch_physics,
	act_flinch_head_back,
	act_flinch_head_left,
	act_flinch_head_right,
	act_flinch_chest_back,
	act_flinch_stomach_back,
	act_flinch_crouch_front,
	act_flinch_crouch_back,
	act_flinch_crouch_left,
	act_flinch_crouch_right,

	act_idle_on_fire,		// on fire animations
	act_walk_on_fire,
	act_run_on_fire,

	act_rappel_loop,		// rappel down a rope!

	act_180_left,			// 180 degree left turn
	act_180_right,

	act_90_left,			// 90 degree turns
	act_90_right,

	act_step_left,			// single steps
	act_step_right,
	act_step_back,
	act_step_fore,

	act_gesture_range_attack1,
	act_gesture_range_attack2,
	act_gesture_melee_attack1,
	act_gesture_melee_attack2,
	act_gesture_range_attack1_low,	// fixme: not used yet, crouched versions of the range attack
	act_gesture_range_attack2_low,	// fixme: not used yet, crouched versions of the range attack

	act_melee_attack_swing_gesture,

	act_gesture_small_flinch,
	act_gesture_big_flinch,
	act_gesture_flinch_blast,			// startled by an explosion
	act_gesture_flinch_blast_shotgun,
	act_gesture_flinch_blast_damaged,	// damaged by an explosion
	act_gesture_flinch_blast_damaged_shotgun,
	act_gesture_flinch_head,
	act_gesture_flinch_chest,
	act_gesture_flinch_stomach,
	act_gesture_flinch_leftarm,
	act_gesture_flinch_rightarm,
	act_gesture_flinch_leftleg,
	act_gesture_flinch_rightleg,

	act_gesture_turn_left,
	act_gesture_turn_right,
	act_gesture_turn_left45,
	act_gesture_turn_right45,
	act_gesture_turn_left90,
	act_gesture_turn_right90,
	act_gesture_turn_left45_flat,
	act_gesture_turn_right45_flat,
	act_gesture_turn_left90_flat,
	act_gesture_turn_right90_flat,

	// half-life 1 compatability stuff goes here. temporary!
	act_barnacle_hit,		// barnacle tongue hits a monster
	act_barnacle_pull,		// barnacle is lifting the monster ( loop )
	act_barnacle_chomp,		// barnacle latches on to the monster
	act_barnacle_chew,		// barnacle is holding the monster in its mouth ( loop )

	// sometimes, you just want to set an npc's sequence to a sequence that doesn't actually
	// have an activity. the ai will reset the npc's sequence to whatever its ideal activity
	// is, though. so if you set ideal activity to do_not_disturb, the ai will not interfere
	// with the npc's current sequence. (sjb)
	act_do_not_disturb,

	act_specific_sequence,

	// viewmodel (weapon) activities
	// fixme: move these to the specific viewmodels, no need to make global
	act_vm_draw,
	act_vm_holster,
	act_vm_idle,
	act_vm_fidget,
	act_vm_pullback,
	act_vm_pullback_high,
	act_vm_pullback_low,
	act_vm_throw,
	act_vm_pullpin,
	act_vm_primaryattack,		// fire
	act_vm_secondaryattack,		// alt. fire
	act_vm_reload,
	act_vm_dryfire,				// fire with no ammo loaded.
	act_vm_hitleft,				// bludgeon, swing to left - hit (primary attk)
	act_vm_hitleft2,			// bludgeon, swing to left - hit (secondary attk)
	act_vm_hitright,			// bludgeon, swing to right - hit (primary attk)
	act_vm_hitright2,			// bludgeon, swing to right - hit (secondary attk)
	act_vm_hitcenter,			// bludgeon, swing center - hit (primary attk)
	act_vm_hitcenter2,			// bludgeon, swing center - hit (secondary attk)
	act_vm_missleft,			// bludgeon, swing to left - miss (primary attk)
	act_vm_missleft2,			// bludgeon, swing to left - miss (secondary attk)
	act_vm_missright,			// bludgeon, swing to right - miss (primary attk)
	act_vm_missright2,			// bludgeon, swing to right - miss (secondary attk)
	act_vm_misscenter,			// bludgeon, swing center - miss (primary attk)
	act_vm_misscenter2,			// bludgeon, swing center - miss (secondary attk)
	act_vm_haulback,			// bludgeon, haul the weapon back for a hard strike (secondary attk)
	act_vm_swinghard,			// bludgeon, release the hard strike (secondary attk)
	act_vm_swingmiss,
	act_vm_swinghit,
	act_vm_idle_to_lowered,
	act_vm_idle_lowered,
	act_vm_lowered_to_idle,
	act_vm_recoil1,
	act_vm_recoil2,
	act_vm_recoil3,
	act_vm_pickup,
	act_vm_release,

	act_vm_attach_silencer,
	act_vm_detach_silencer,

	act_vm_empty_fire,			// fire last round in magazine
	act_vm_empty_reload,        // reload from an empty state
	act_vm_empty_draw,			// deploy an empty weapon
	act_vm_empty_idle,			// idle in an empty state

//===========================
// hl2 specific activities
//===========================
	// slam	specialty activities
	act_slam_stickwall_idle,
	act_slam_stickwall_nd_idle,
	act_slam_stickwall_attach,
	act_slam_stickwall_attach2,
	act_slam_stickwall_nd_attach,
	act_slam_stickwall_nd_attach2,
	act_slam_stickwall_detonate,
	act_slam_stickwall_detonator_holster,
	act_slam_stickwall_draw,
	act_slam_stickwall_nd_draw,
	act_slam_stickwall_to_throw,
	act_slam_stickwall_to_throw_nd,
	act_slam_stickwall_to_tripmine_nd,
	act_slam_throw_idle,
	act_slam_throw_nd_idle,
	act_slam_throw_throw,
	act_slam_throw_throw2,
	act_slam_throw_throw_nd,
	act_slam_throw_throw_nd2,
	act_slam_throw_draw,
	act_slam_throw_nd_draw,
	act_slam_throw_to_stickwall,
	act_slam_throw_to_stickwall_nd,
	act_slam_throw_detonate,
	act_slam_throw_detonator_holster,
	act_slam_throw_to_tripmine_nd,
	act_slam_tripmine_idle,
	act_slam_tripmine_draw,
	act_slam_tripmine_attach,
	act_slam_tripmine_attach2,
	act_slam_tripmine_to_stickwall_nd,
	act_slam_tripmine_to_throw_nd,
	act_slam_detonator_idle,
	act_slam_detonator_draw,
	act_slam_detonator_detonate,
	act_slam_detonator_holster,
	act_slam_detonator_stickwall_draw,
	act_slam_detonator_throw_draw,

	// shotgun specialty activities
	act_shotgun_reload_start,
	act_shotgun_reload_finish,
	act_shotgun_pump,

	// smg2 special activities
	act_smg2_idle2,
	act_smg2_fire2,
	act_smg2_draw2,
	act_smg2_reload2,
	act_smg2_dryfire2,
	act_smg2_toauto,
	act_smg2_toburst,

	// physcannon special activities
	act_physcannon_upgrade,

	// weapon override activities
	act_range_attack_ar1,
	act_range_attack_ar2,
	act_range_attack_ar2_low,
	act_range_attack_ar2_grenade,
	act_range_attack_hmg1,
	act_range_attack_ml,
	act_range_attack_smg1,
	act_range_attack_smg1_low,
	act_range_attack_smg2,
	act_range_attack_shotgun,
	act_range_attack_shotgun_low,
	act_range_attack_pistol,
	act_range_attack_pistol_low,
	act_range_attack_slam,
	act_range_attack_tripwire,
	act_range_attack_throw,
	act_range_attack_sniper_rifle,
	act_range_attack_rpg,
	act_melee_attack_swing,

	act_range_aim_low,
	act_range_aim_smg1_low,
	act_range_aim_pistol_low,
	act_range_aim_ar2_low,

	act_cover_pistol_low,
	act_cover_smg1_low,

	// weapon override activities
	act_gesture_range_attack_ar1,
	act_gesture_range_attack_ar2,
	act_gesture_range_attack_ar2_grenade,
	act_gesture_range_attack_hmg1,
	act_gesture_range_attack_ml,
	act_gesture_range_attack_smg1,
	act_gesture_range_attack_smg1_low,
	act_gesture_range_attack_smg2,
	act_gesture_range_attack_shotgun,
	act_gesture_range_attack_pistol,
	act_gesture_range_attack_pistol_low,
	act_gesture_range_attack_slam,
	act_gesture_range_attack_tripwire,
	act_gesture_range_attack_throw,
	act_gesture_range_attack_sniper_rifle,
	act_gesture_melee_attack_swing,

	act_idle_rifle,
	act_idle_smg1,
	act_idle_angry_smg1,
	act_idle_pistol,
	act_idle_angry_pistol,
	act_idle_angry_shotgun,
	act_idle_stealth_pistol,

	act_idle_package,
	act_walk_package,
	act_idle_suitcase,
	act_walk_suitcase,

	act_idle_smg1_relaxed,
	act_idle_smg1_stimulated,
	act_walk_rifle_relaxed,
	act_run_rifle_relaxed,
	act_walk_rifle_stimulated,
	act_run_rifle_stimulated,

	act_idle_aim_rifle_stimulated,
	act_walk_aim_rifle_stimulated,
	act_run_aim_rifle_stimulated,

	act_idle_shotgun_relaxed,
	act_idle_shotgun_stimulated,
	act_idle_shotgun_agitated,

	// policing activities
	act_walk_angry,
	act_police_harass1,
	act_police_harass2,

	// manned guns
	act_idle_mannedgun,

	// melee weapon
	act_idle_melee,
	act_idle_angry_melee,

	// rpg activities
	act_idle_rpg_relaxed,
	act_idle_rpg,
	act_idle_angry_rpg,
	act_cover_low_rpg,
	act_walk_rpg,
	act_run_rpg,
	act_walk_crouch_rpg,
	act_run_crouch_rpg,
	act_walk_rpg_relaxed,
	act_run_rpg_relaxed,

	act_walk_rifle,
	act_walk_aim_rifle,
	act_walk_crouch_rifle,
	act_walk_crouch_aim_rifle,
	act_run_rifle,
	act_run_aim_rifle,
	act_run_crouch_rifle,
	act_run_crouch_aim_rifle,
	act_run_stealth_pistol,

	act_walk_aim_shotgun,
	act_run_aim_shotgun,

	act_walk_pistol,
	act_run_pistol,
	act_walk_aim_pistol,
	act_run_aim_pistol,
	act_walk_stealth_pistol,
	act_walk_aim_stealth_pistol,
	act_run_aim_stealth_pistol,

	// reloads
	act_reload_pistol,
	act_reload_pistol_low,
	act_reload_smg1,
	act_reload_smg1_low,
	act_reload_shotgun,
	act_reload_shotgun_low,

	act_gesture_reload,
	act_gesture_reload_pistol,
	act_gesture_reload_smg1,
	act_gesture_reload_shotgun,

	// busy animations
	act_busy_lean_left,
	act_busy_lean_left_entry,
	act_busy_lean_left_exit,
	act_busy_lean_back,
	act_busy_lean_back_entry,
	act_busy_lean_back_exit,
	act_busy_sit_ground,
	act_busy_sit_ground_entry,
	act_busy_sit_ground_exit,
	act_busy_sit_chair,
	act_busy_sit_chair_entry,
	act_busy_sit_chair_exit,
	act_busy_stand,
	act_busy_queue,

	// dodge animations
	act_duck_dodge,

	// for npcs being lifted/eaten by barnacles:
	// being swallowed by a barnacle
	act_die_barnacle_swallow,
	// being lifted by a barnacle
	act_gesture_barnacle_strangle,

	act_physcannon_detach,	// an activity to be played if we're picking this up with the physcannon
	act_physcannon_animate, // an activity to be played by an object being picked up with the physcannon, but has different behavior to detach
	act_physcannon_animate_pre,	// an activity to be played by an object being picked up with the physcannon, before playing the act_physcannon_animate
	act_physcannon_animate_post,// an activity to be played by an object being picked up with the physcannon, after playing the act_physcannon_animate

	act_die_frontside,
	act_die_rightside,
	act_die_backside,
	act_die_leftside,

	act_die_crouch_frontside,
	act_die_crouch_rightside,
	act_die_crouch_backside,
	act_die_crouch_leftside,

	act_open_door,

	// dynamic interactions
	act_di_alyx_zombie_melee,
	act_di_alyx_zombie_torso_melee,
	act_di_alyx_headcrab_melee,
	act_di_alyx_antlion,

	act_di_alyx_zombie_shotgun64,
	act_di_alyx_zombie_shotgun26,

	act_readiness_relaxed_to_stimulated,
	act_readiness_relaxed_to_stimulated_walk,
	act_readiness_agitated_to_stimulated,
	act_readiness_stimulated_to_relaxed,

	act_readiness_pistol_relaxed_to_stimulated,
	act_readiness_pistol_relaxed_to_stimulated_walk,
	act_readiness_pistol_agitated_to_stimulated,
	act_readiness_pistol_stimulated_to_relaxed,

	act_idle_carry,
	act_walk_carry,

	//===========================
	// tf2 specific activities
	//===========================
	act_startdying,
	act_dyingloop,
	act_dyingtodead,

	act_ride_manned_gun,

	// all viewmodels
	act_vm_sprint_enter,
	act_vm_sprint_idle,
	act_vm_sprint_leave,

	// looping weapon firing
	act_fire_start,
	act_fire_loop,
	act_fire_end,

	act_crouching_grenadeidle,
	act_crouching_grenadeready,
	act_crouching_primaryattack,
	act_overlay_grenadeidle,
	act_overlay_grenadeready,
	act_overlay_primaryattack,
	act_overlay_shield_up,
	act_overlay_shield_down,
	act_overlay_shield_up_idle,
	act_overlay_shield_attack,
	act_overlay_shield_knockback,
	act_shield_up,
	act_shield_down,
	act_shield_up_idle,
	act_shield_attack,
	act_shield_knockback,
	act_crouching_shield_up,
	act_crouching_shield_down,
	act_crouching_shield_up_idle,
	act_crouching_shield_attack,
	act_crouching_shield_knockback,

	// turning in place
	act_turnright45,
	act_turnleft45,

	act_turn,

	act_obj_assembling,
	act_obj_dismantling,
	act_obj_startup,
	act_obj_running,
	act_obj_idle,
	act_obj_placing,
	act_obj_deteriorating,
	act_obj_upgrading,

	// deploy
	act_deploy,
	act_deploy_idle,
	act_undeploy,

	// crossbow
	act_crossbow_draw_unloaded,

	// gauss
	act_gauss_spinup,
	act_gauss_spincycle,

	//===========================
	// csport specific activities
	//===========================

	act_vm_primaryattack_silenced,		// fire
	act_vm_reload_silenced,
	act_vm_dryfire_silenced,				// fire with no ammo loaded.
	act_vm_idle_silenced,
	act_vm_draw_silenced,
	act_vm_idle_empty_left,
	act_vm_dryfire_left,

	// new for cs2
	act_vm_is_draw,
	act_vm_is_holster,
	act_vm_is_idle,
	act_vm_is_primaryattack,

	act_player_idle_fire,
	act_player_crouch_fire,
	act_player_crouch_walk_fire,
	act_player_walk_fire,
	act_player_run_fire,

	act_idletorun,
	act_runtoidle,

	act_vm_draw_deployed,

	act_hl2mp_idle_melee,
	act_hl2mp_run_melee,
	act_hl2mp_idle_crouch_melee,
	act_hl2mp_walk_crouch_melee,
	act_hl2mp_gesture_range_attack_melee,
	act_hl2mp_gesture_reload_melee,
	act_hl2mp_jump_melee,

	// portal!
	act_vm_fizzle,

	// multiplayer
	act_mp_stand_idle,
	act_mp_crouch_idle,
	act_mp_crouch_deployed_idle,
	act_mp_crouch_deployed,
	act_mp_deployed_idle,
	act_mp_run,
	act_mp_walk,
	act_mp_airwalk,
	act_mp_crouchwalk,
	act_mp_sprint,
	act_mp_jump,
	act_mp_jump_start,
	act_mp_jump_float,
	act_mp_jump_land,
	act_mp_jump_impact_n,
	act_mp_jump_impact_e,
	act_mp_jump_impact_w,
	act_mp_jump_impact_s,
	act_mp_jump_impact_top,
	act_mp_doublejump,
	act_mp_swim,
	act_mp_deployed,
	act_mp_swim_deployed,
	act_mp_vcd,

	act_mp_attack_stand_primaryfire,
	act_mp_attack_stand_primaryfire_deployed,
	act_mp_attack_stand_secondaryfire,
	act_mp_attack_stand_grenade,
	act_mp_attack_crouch_primaryfire,
	act_mp_attack_crouch_primaryfire_deployed,
	act_mp_attack_crouch_secondaryfire,
	act_mp_attack_crouch_grenade,
	act_mp_attack_swim_primaryfire,
	act_mp_attack_swim_secondaryfire,
	act_mp_attack_swim_grenade,
	act_mp_attack_airwalk_primaryfire,
	act_mp_attack_airwalk_secondaryfire,
	act_mp_attack_airwalk_grenade,
	act_mp_reload_stand,
	act_mp_reload_stand_loop,
	act_mp_reload_stand_end,
	act_mp_reload_crouch,
	act_mp_reload_crouch_loop,
	act_mp_reload_crouch_end,
	act_mp_reload_swim,
	act_mp_reload_swim_loop,
	act_mp_reload_swim_end,
	act_mp_reload_airwalk,
	act_mp_reload_airwalk_loop,
	act_mp_reload_airwalk_end,
	act_mp_attack_stand_prefire,
	act_mp_attack_stand_postfire,
	act_mp_attack_stand_startfire,
	act_mp_attack_crouch_prefire,
	act_mp_attack_crouch_postfire,
	act_mp_attack_swim_prefire,
	act_mp_attack_swim_postfire,

	// multiplayer - primary
	act_mp_stand_primary,
	act_mp_crouch_primary,
	act_mp_run_primary,
	act_mp_walk_primary,
	act_mp_airwalk_primary,
	act_mp_crouchwalk_primary,
	act_mp_jump_primary,
	act_mp_jump_start_primary,
	act_mp_jump_float_primary,
	act_mp_jump_land_primary,
	act_mp_swim_primary,
	act_mp_deployed_primary,
	act_mp_swim_deployed_primary,

	act_mp_attack_stand_primary,		// run, walk
	act_mp_attack_stand_primary_deployed,
	act_mp_attack_crouch_primary,		// crouchwalk
	act_mp_attack_crouch_primary_deployed,
	act_mp_attack_swim_primary,
	act_mp_attack_airwalk_primary,

	act_mp_reload_stand_primary,		// run, walk
	act_mp_reload_stand_primary_loop,
	act_mp_reload_stand_primary_end,
	act_mp_reload_crouch_primary,		// crouchwalk
	act_mp_reload_crouch_primary_loop,
	act_mp_reload_crouch_primary_end,
	act_mp_reload_swim_primary,
	act_mp_reload_swim_primary_loop,
	act_mp_reload_swim_primary_end,
	act_mp_reload_airwalk_primary,
	act_mp_reload_airwalk_primary_loop,
	act_mp_reload_airwalk_primary_end,

	act_mp_attack_stand_grenade_primary,		// run, walk
	act_mp_attack_crouch_grenade_primary,		// crouchwalk
	act_mp_attack_swim_grenade_primary,
	act_mp_attack_airwalk_grenade_primary,


	// secondary
	act_mp_stand_secondary,
	act_mp_crouch_secondary,
	act_mp_run_secondary,
	act_mp_walk_secondary,
	act_mp_airwalk_secondary,
	act_mp_crouchwalk_secondary,
	act_mp_jump_secondary,
	act_mp_jump_start_secondary,
	act_mp_jump_float_secondary,
	act_mp_jump_land_secondary,
	act_mp_swim_secondary,

	act_mp_attack_stand_secondary,		// run, walk
	act_mp_attack_crouch_secondary,		// crouchwalk
	act_mp_attack_swim_secondary,
	act_mp_attack_airwalk_secondary,

	act_mp_reload_stand_secondary,		// run, walk
	act_mp_reload_stand_secondary_loop,
	act_mp_reload_stand_secondary_end,
	act_mp_reload_crouch_secondary,		// crouchwalk
	act_mp_reload_crouch_secondary_loop,
	act_mp_reload_crouch_secondary_end,
	act_mp_reload_swim_secondary,
	act_mp_reload_swim_secondary_loop,
	act_mp_reload_swim_secondary_end,
	act_mp_reload_airwalk_secondary,
	act_mp_reload_airwalk_secondary_loop,
	act_mp_reload_airwalk_secondary_end,

	act_mp_attack_stand_grenade_secondary,		// run, walk
	act_mp_attack_crouch_grenade_secondary,		// crouchwalk
	act_mp_attack_swim_grenade_secondary,
	act_mp_attack_airwalk_grenade_secondary,

	// melee
	act_mp_stand_melee,
	act_mp_crouch_melee,
	act_mp_run_melee,
	act_mp_walk_melee,
	act_mp_airwalk_melee,
	act_mp_crouchwalk_melee,
	act_mp_jump_melee,
	act_mp_jump_start_melee,
	act_mp_jump_float_melee,
	act_mp_jump_land_melee,
	act_mp_swim_melee,

	act_mp_attack_stand_melee,		// run, walk
	act_mp_attack_stand_melee_secondary,
	act_mp_attack_crouch_melee,		// crouchwalk
	act_mp_attack_crouch_melee_secondary,
	act_mp_attack_swim_melee,
	act_mp_attack_airwalk_melee,

	act_mp_attack_stand_grenade_melee,		// run, walk
	act_mp_attack_crouch_grenade_melee,		// crouchwalk
	act_mp_attack_swim_grenade_melee,
	act_mp_attack_airwalk_grenade_melee,

	// item1
	act_mp_stand_item1,
	act_mp_crouch_item1,
	act_mp_run_item1,
	act_mp_walk_item1,
	act_mp_airwalk_item1,
	act_mp_crouchwalk_item1,
	act_mp_jump_item1,
	act_mp_jump_start_item1,
	act_mp_jump_float_item1,
	act_mp_jump_land_item1,
	act_mp_swim_item1,

	act_mp_attack_stand_item1,		// run, walk
	act_mp_attack_stand_item1_secondary,
	act_mp_attack_crouch_item1,		// crouchwalk
	act_mp_attack_crouch_item1_secondary,
	act_mp_attack_swim_item1,
	act_mp_attack_airwalk_item1,

	// item2
	act_mp_stand_item2,
	act_mp_crouch_item2,
	act_mp_run_item2,
	act_mp_walk_item2,
	act_mp_airwalk_item2,
	act_mp_crouchwalk_item2,
	act_mp_jump_item2,
	act_mp_jump_start_item2,
	act_mp_jump_float_item2,
	act_mp_jump_land_item2,
	act_mp_swim_item2,

	act_mp_attack_stand_item2,		// run, walk
	act_mp_attack_stand_item2_secondary,
	act_mp_attack_crouch_item2,		// crouchwalk
	act_mp_attack_crouch_item2_secondary,
	act_mp_attack_swim_item2,
	act_mp_attack_airwalk_item2,

	// flinches
	act_mp_gesture_flinch,
	act_mp_gesture_flinch_primary,
	act_mp_gesture_flinch_secondary,
	act_mp_gesture_flinch_melee,
	act_mp_gesture_flinch_item1,
	act_mp_gesture_flinch_item2,

	act_mp_gesture_flinch_head,
	act_mp_gesture_flinch_chest,
	act_mp_gesture_flinch_stomach,
	act_mp_gesture_flinch_leftarm,
	act_mp_gesture_flinch_rightarm,
	act_mp_gesture_flinch_leftleg,
	act_mp_gesture_flinch_rightleg,

	// team fortress specific - medic heal, medic infect, etc.....
	act_mp_grenade1_draw,
	act_mp_grenade1_idle,
	act_mp_grenade1_attack,
	act_mp_grenade2_draw,
	act_mp_grenade2_idle,
	act_mp_grenade2_attack,

	act_mp_primary_grenade1_draw,
	act_mp_primary_grenade1_idle,
	act_mp_primary_grenade1_attack,
	act_mp_primary_grenade2_draw,
	act_mp_primary_grenade2_idle,
	act_mp_primary_grenade2_attack,

	act_mp_secondary_grenade1_draw,
	act_mp_secondary_grenade1_idle,
	act_mp_secondary_grenade1_attack,
	act_mp_secondary_grenade2_draw,
	act_mp_secondary_grenade2_idle,
	act_mp_secondary_grenade2_attack,

	act_mp_melee_grenade1_draw,
	act_mp_melee_grenade1_idle,
	act_mp_melee_grenade1_attack,
	act_mp_melee_grenade2_draw,
	act_mp_melee_grenade2_idle,
	act_mp_melee_grenade2_attack,

	act_mp_item1_grenade1_draw,
	act_mp_item1_grenade1_idle,
	act_mp_item1_grenade1_attack,
	act_mp_item1_grenade2_draw,
	act_mp_item1_grenade2_idle,
	act_mp_item1_grenade2_attack,

	act_mp_item2_grenade1_draw,
	act_mp_item2_grenade1_idle,
	act_mp_item2_grenade1_attack,
	act_mp_item2_grenade2_draw,
	act_mp_item2_grenade2_idle,
	act_mp_item2_grenade2_attack,

	// building
	act_mp_stand_building,
	act_mp_crouch_building,
	act_mp_run_building,
	act_mp_walk_building,
	act_mp_airwalk_building,
	act_mp_crouchwalk_building,
	act_mp_jump_building,
	act_mp_jump_start_building,
	act_mp_jump_float_building,
	act_mp_jump_land_building,
	act_mp_swim_building,

	act_mp_attack_stand_building,		// run, walk
	act_mp_attack_crouch_building,		// crouchwalk
	act_mp_attack_swim_building,
	act_mp_attack_airwalk_building,

	act_mp_attack_stand_grenade_building,		// run, walk
	act_mp_attack_crouch_grenade_building,		// crouchwalk
	act_mp_attack_swim_grenade_building,
	act_mp_attack_airwalk_grenade_building,

	act_mp_stand_pda,
	act_mp_crouch_pda,
	act_mp_run_pda,
	act_mp_walk_pda,
	act_mp_airwalk_pda,
	act_mp_crouchwalk_pda,
	act_mp_jump_pda,
	act_mp_jump_start_pda,
	act_mp_jump_float_pda,
	act_mp_jump_land_pda,
	act_mp_swim_pda,

	act_mp_attack_stand_pda,
	act_mp_attack_swim_pda,

	act_mp_gesture_vc_handmouth,
	act_mp_gesture_vc_fingerpoint,
	act_mp_gesture_vc_fistpump,
	act_mp_gesture_vc_thumbsup,
	act_mp_gesture_vc_nodyes,
	act_mp_gesture_vc_nodno,

	act_mp_gesture_vc_handmouth_primary,
	act_mp_gesture_vc_fingerpoint_primary,
	act_mp_gesture_vc_fistpump_primary,
	act_mp_gesture_vc_thumbsup_primary,
	act_mp_gesture_vc_nodyes_primary,
	act_mp_gesture_vc_nodno_primary,

	act_mp_gesture_vc_handmouth_secondary,
	act_mp_gesture_vc_fingerpoint_secondary,
	act_mp_gesture_vc_fistpump_secondary,
	act_mp_gesture_vc_thumbsup_secondary,
	act_mp_gesture_vc_nodyes_secondary,
	act_mp_gesture_vc_nodno_secondary,

	act_mp_gesture_vc_handmouth_melee,
	act_mp_gesture_vc_fingerpoint_melee,
	act_mp_gesture_vc_fistpump_melee,
	act_mp_gesture_vc_thumbsup_melee,
	act_mp_gesture_vc_nodyes_melee,
	act_mp_gesture_vc_nodno_melee,

	act_mp_gesture_vc_handmouth_item1,
	act_mp_gesture_vc_fingerpoint_item1,
	act_mp_gesture_vc_fistpump_item1,
	act_mp_gesture_vc_thumbsup_item1,
	act_mp_gesture_vc_nodyes_item1,
	act_mp_gesture_vc_nodno_item1,

	act_mp_gesture_vc_handmouth_item2,
	act_mp_gesture_vc_fingerpoint_item2,
	act_mp_gesture_vc_fistpump_item2,
	act_mp_gesture_vc_thumbsup_item2,
	act_mp_gesture_vc_nodyes_item2,
	act_mp_gesture_vc_nodno_item2,

	act_mp_gesture_vc_handmouth_building,
	act_mp_gesture_vc_fingerpoint_building,
	act_mp_gesture_vc_fistpump_building,
	act_mp_gesture_vc_thumbsup_building,
	act_mp_gesture_vc_nodyes_building,
	act_mp_gesture_vc_nodno_building,

	act_mp_gesture_vc_handmouth_pda,
	act_mp_gesture_vc_fingerpoint_pda,
	act_mp_gesture_vc_fistpump_pda,
	act_mp_gesture_vc_thumbsup_pda,
	act_mp_gesture_vc_nodyes_pda,
	act_mp_gesture_vc_nodno_pda,


	act_vm_unusable,
	act_vm_unusable_to_usable,
	act_vm_usable_to_unusable,

	// specific viewmodel activities for weapon roles
	act_primary_vm_draw,
	act_primary_vm_holster,
	act_primary_vm_idle,
	act_primary_vm_pullback,
	act_primary_vm_primaryattack,
	act_primary_vm_secondaryattack,
	act_primary_vm_reload,
	act_primary_vm_dryfire,
	act_primary_vm_idle_to_lowered,
	act_primary_vm_idle_lowered,
	act_primary_vm_lowered_to_idle,

	act_secondary_vm_draw,
	act_secondary_vm_holster,
	act_secondary_vm_idle,
	act_secondary_vm_pullback,
	act_secondary_vm_primaryattack,
	act_secondary_vm_secondaryattack,
	act_secondary_vm_reload,
	act_secondary_vm_dryfire,
	act_secondary_vm_idle_to_lowered,
	act_secondary_vm_idle_lowered,
	act_secondary_vm_lowered_to_idle,

	act_melee_vm_draw,
	act_melee_vm_holster,
	act_melee_vm_idle,
	act_melee_vm_pullback,
	act_melee_vm_primaryattack,
	act_melee_vm_secondaryattack,
	act_melee_vm_reload,
	act_melee_vm_dryfire,
	act_melee_vm_idle_to_lowered,
	act_melee_vm_idle_lowered,
	act_melee_vm_lowered_to_idle,

	act_pda_vm_draw,
	act_pda_vm_holster,
	act_pda_vm_idle,
	act_pda_vm_pullback,
	act_pda_vm_primaryattack,
	act_pda_vm_secondaryattack,
	act_pda_vm_reload,
	act_pda_vm_dryfire,
	act_pda_vm_idle_to_lowered,
	act_pda_vm_idle_lowered,
	act_pda_vm_lowered_to_idle,

	act_item1_vm_draw,
	act_item1_vm_holster,
	act_item1_vm_idle,
	act_item1_vm_pullback,
	act_item1_vm_primaryattack,
	act_item1_vm_secondaryattack,
	act_item1_vm_reload,
	act_item1_vm_dryfire,
	act_item1_vm_idle_to_lowered,
	act_item1_vm_idle_lowered,
	act_item1_vm_lowered_to_idle,

	act_item2_vm_draw,
	act_item2_vm_holster,
	act_item2_vm_idle,
	act_item2_vm_pullback,
	act_item2_vm_primaryattack,
	act_item2_vm_secondaryattack,
	act_item2_vm_reload,
	act_item2_vm_dryfire,
	act_item2_vm_idle_to_lowered,
	act_item2_vm_idle_lowered,
	act_item2_vm_lowered_to_idle,

	// infested activities
	act_reload_succeed,
	act_reload_fail,
	// autogun
	act_walk_aim_autogun,
	act_run_aim_autogun,
	act_idle_autogun,
	act_idle_aim_autogun,
	act_reload_autogun,
	act_crouch_idle_autogun,
	act_range_attack_autogun,
	act_jump_autogun,
	// pistol
	act_idle_aim_pistol,
	// pdw
	act_walk_aim_dual,
	act_run_aim_dual,
	act_idle_dual,
	act_idle_aim_dual,
	act_reload_dual,
	act_crouch_idle_dual,
	act_range_attack_dual,
	act_jump_dual,
	// shotgun
	act_idle_shotgun,
	act_idle_aim_shotgun,
	act_crouch_idle_shotgun,
	act_jump_shotgun,
	// rifle
	act_idle_aim_rifle,
	act_reload_rifle,
	act_crouch_idle_rifle,
	act_range_attack_rifle,
	act_jump_rifle,

	// infested general ai
	act_sleep,
	act_wake,

	// shield bug
	act_flick_left,
	act_flick_left_middle,
	act_flick_right_middle,
	act_flick_right,
	act_spinaround,

	// mortar bug
	act_prep_to_fire,
	act_fire,
	act_fire_recover,

	// shaman
	act_spray,

	// boomer
	act_prep_explode,
	act_explode,

	///******************
	///dota animations
	///******************

	act_dota_idle,
	act_dota_run,
	act_dota_attack,
	act_dota_attack_event,
	act_dota_die,
	act_dota_flinch,
	act_dota_disabled,
	act_dota_cast_ability_1,
	act_dota_cast_ability_2,
	act_dota_cast_ability_3,
	act_dota_cast_ability_4,
	act_dota_override_ability_1,
	act_dota_override_ability_2,
	act_dota_override_ability_3,
	act_dota_override_ability_4,
	act_dota_channel_ability_1,
	act_dota_channel_ability_2,
	act_dota_channel_ability_3,
	act_dota_channel_ability_4,
	act_dota_channel_end_ability_1,
	act_dota_channel_end_ability_2,
	act_dota_channel_end_ability_3,
	act_dota_channel_end_ability_4,


	// portal2
	act_mp_run_speedpaint,  // running on speed paint
	act_mp_long_fall, // falling a long way
	act_mp_tractorbeam_float, // floating in a tractor beam
	act_mp_death_crush,

	act_mp_run_speedpaint_primary, // player with portalgun running on speed paint
	act_mp_drowning_primary, // drowning while holding portalgun
	act_mp_long_fall_primary, // falling a long way while holding portalgun
	act_mp_tractorbeam_float_primary, // floating in a tractor beam while holding portalgun 
	act_mp_death_crush_primary,


	// csgo death anims that don't require direction (direction is pose-param driven for more granularity)
	act_die_stand,
	act_die_stand_headshot,
	act_die_crouch,
	act_die_crouch_headshot,



	// csgo action activities
	act_csgo_null,
	act_csgo_defuse,
	act_csgo_defuse_with_kit,
	act_csgo_flashbang_reaction,
	act_csgo_fire_primary,
	act_csgo_fire_primary_opt_1,
	act_csgo_fire_primary_opt_2,
	act_csgo_fire_secondary,
	act_csgo_fire_secondary_opt_1,
	act_csgo_fire_secondary_opt_2,
	act_csgo_reload,
	act_csgo_reload_start,
	act_csgo_reload_loop,
	act_csgo_reload_end,
	act_csgo_operate,
	act_csgo_deploy,
	act_csgo_catch,
	act_csgo_silencer_detach,
	act_csgo_silencer_attach,
	act_csgo_twitch,
	act_csgo_twitch_buyzone,
	act_csgo_plant_bomb,
	act_csgo_idle_turn_balanceadjust,
	act_csgo_idle_adjust_stoppedmoving,
	act_csgo_alive_loop,
	act_csgo_flinch,
	act_csgo_flinch_head,
	act_csgo_flinch_molotov,
	act_csgo_jump,
	act_csgo_fall,
	act_csgo_climb_ladder,
	act_csgo_land_light,
	act_csgo_land_heavy,
	act_csgo_exit_ladder_top,
	act_csgo_exit_ladder_bottom,

	// this is the end of the global activities, private per-monster activities start here.
	last_shared_activity,
};

inline int select_weighted_seq ( animstate_t* animstate, int act ) {
	int seq = -1;

	const bool crouching = animstate->m_duck_amount > 0.55f;
	const bool moving = animstate->m_unk_feet_speed_ratio > 0.25f;

	switch ( act ) {
	case act_csgo_land_heavy:
		seq = 23;
		if ( crouching )
			seq = 24;
		break;
	case act_csgo_fall:
		seq = 14;
		break;
	case act_csgo_jump:
		seq = moving + 17;
		if ( !crouching )
			seq = moving + 15;
		break;
	case act_csgo_climb_ladder:
		seq = 13;
		break;
	case act_csgo_land_light:
		seq = 2 * moving + 20;
		if ( crouching ) {
			seq = 21;
			if ( moving )
				seq = 19;
		}
		break;
	case act_csgo_idle_turn_balanceadjust:
		seq = 4;
		break;
	case act_csgo_idle_adjust_stoppedmoving:
		seq = 5;
		break;
	case act_csgo_flashbang_reaction:
		seq = 224;
		if ( crouching )
			seq = 225;
		break;
	case act_csgo_alive_loop:
		seq = 8;
		if ( animstate->m_weapon
			&& animstate->m_weapon->data()
			&& animstate->m_weapon->data ( )->m_type == weapon_type_t::knife )
			seq = 9;
		break;
	default:
		return -1;
	}

	if ( seq < 2 )
		return -1;

	return seq;
}

/* to fix delayed anim events later */
/* let's rebuild the animation events in a ghetto way, i cba */
/* maybe we can use events to trigger do_animation_event later? */
void anims::rebuilt::do_animation_event ( animstate_t* anim_state, int id, int data ) {
	const auto active_weapon = anim_state->m_entity ? anim_state->m_entity->weapon ( ) : nullptr;

	switch ( id ) {
		/* this is the only one we would probably care about rn */
	case 8 /* PLAYERANIMEVENT_JUMP */:
		/* 
		*	use 0x10A as m_bJumping instead of 0x109 since m_bJumping does not seem to exist on the client
		*	(i'm too lazy to reverse server animstate class, so lets use this workaround and use the "free" space we have in the struct)
		*	animstate + 0x10A seems to be unused, and is in a great position (m_bJumping is supposed to be after m_bOnGround on the server?)
		*	good enough.
		*/
		*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x10A ) = true;
		set_sequence ( anim_state, 4, select_weighted_seq ( anim_state, act_csgo_jump ) );
		break;
	}
}

void anims::rebuilt::trigger_animation_events ( animstate_t* anim_state ) {
	const auto player = anim_state->m_entity;

	/* work on running more animation events on the client later */
	/* handle jump events in ghetto way (what is a ghetto jump?) */
	if ( !( player->flags ( ) & flags_t::on_ground )
		&& anim_state->m_on_ground
		&& ( ( player == g::local && g::local ) ? reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( player ) + 0x94 )->z : player->vel ( ).z ) > 0.0f ) {
		do_animation_event ( anim_state, 8, 0 /* is this event used?*/ );
	}
}

std::array<float, 65> last_lby_update_time { 0.0f };

void anims::rebuilt::setup_velocity( animstate_t* anim_state, bool force_feet_yaw ) {
	//static auto& angle_mode = options::vars [ _ ( "debug.angle_mode" ) ].val.i;

	//static auto CCSGOPlayerAnimState__SetUpVelocity = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 30 56 57 8B 3D" ) ).get<void( __thiscall* )( animstate_t* )>( );
	//CCSGOPlayerAnimState__SetUpVelocity( anim_state );
	static auto C_CSPlayer__EstimateAbsVelocity = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 1C 53 56 57 8B F9 F7" ) ).get<void( __thiscall* )( player_t* )>( );
	static auto m_boneSnapshots_BONESNAPSHOT_ENTIRE_BODY = pattern::search( _( "client.dll" ) , _( "0F 2F 99 ? ? ? ? 0F 82 ? ? ? ? F3 0F 10 0D ? ? ? ? 81 C1" ) ).add(3).deref( ).sub(4).get<uint32_t>( );

	MDLCACHE_CRITICAL_SECTION( );

	// update the local velocity variables so we don't recalculate them unnecessarily

	const auto player = anim_state->m_entity;

	vec3_t vecAbsVelocity;

	if ( !CLIENT_DLL_ANIMS ) {
		vecAbsVelocity = *reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( player ) + 0x94 );
	}
	else {
		vecAbsVelocity = player->vel();

		if ( cs::i::engine->is_hltv( ) || cs::i::engine->is_playing_demo( ) ) {
			// Estimating velocity when playing demos is prone to fail, especially in POVs. Fall back to GetAbsVelocity.
			vecAbsVelocity = *reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( player ) + 0x94 );
		}
		else {
			C_CSPlayer__EstimateAbsVelocity( player );	// Using this accessor if the client is starved of information, 
																// the player doesn't run on the spot. Note this is unreliable
																// and could fail to populate the value if prediction fails.
			vecAbsVelocity = *reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( player ) + 0x94 );
		}

		if ( vecAbsVelocity.length_sqr( ) > 97344.008f )
			vecAbsVelocity = vecAbsVelocity.normalized( ) * 312.0f;
	}

	// save vertical velocity component
	anim_state->m_up_vel = vecAbsVelocity.z;

	// discard z component
	vecAbsVelocity.z = 0.0f;

	// remember if the player is accelerating.
	*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x1AD ) = ( reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x15C )->length_sqr( ) < vecAbsVelocity.length_sqr( ) );

	// rapidly approach ideal velocity instead of instantly adopt it. This helps smooth out instant velocity changes, like
	// when the player runs headlong into a wall and their velocity instantly becomes zero.
	anim_state->m_vel = valve_math::ApproachVector ( vecAbsVelocity , anim_state->m_vel , anim_state->m_last_clientside_anim_update_time_delta * 2000.0f );
	anim_state->m_vel_norm = anim_state->m_vel.normalized( );

	// save horizontal velocity length
	anim_state->m_speed2d = std::min( anim_state->m_vel.length( ) , 260.0f );

	if ( anim_state->m_speed2d > 0.0f )
		anim_state->m_vel_norm_nonzero = anim_state->m_vel_norm;

	//compute speed in various normalized forms
	float flMaxSpeedRun = (anim_state->m_weapon && anim_state->m_weapon->data( ) ) ? std::max( player->scoped() ? anim_state->m_weapon->data( )->m_max_speed_alt : anim_state->m_weapon->data( )->m_max_speed /* may change randomly */ , 0.001f ) : 260.0f;

	anim_state->m_speed_normalized = std::clamp( anim_state->m_speed2d / flMaxSpeedRun , 0.0f , 1.0f );
	anim_state->m_unk_feet_speed_ratio = ( 2.9411764f / flMaxSpeedRun ) * anim_state->m_speed2d;
	anim_state->m_run_speed = ( 1.923077f / flMaxSpeedRun ) * anim_state->m_speed2d;

	if ( anim_state->m_run_speed >= 1.0f )
		anim_state->m_vel_unk = anim_state->m_speed2d;
	else if ( anim_state->m_run_speed < 0.5f )
		anim_state->m_vel_unk = valve_math::Approach( 80.0f , anim_state->m_vel_unk , anim_state->m_last_clientside_anim_update_time_delta * 60.0f );

	bool bStartedMovingThisFrame = false;
	bool bStoppedMovingThisFrame = false;

	if ( anim_state->m_speed2d <= 0.0f ) {
		bStoppedMovingThisFrame = anim_state->m_time_since_stop <= 0.0f;
		anim_state->m_time_since_move = 0.0f;
		anim_state->m_time_since_stop += anim_state->m_last_clientside_anim_update_time_delta;
	}
	else {
		bStartedMovingThisFrame = anim_state->m_time_since_move <= 0.0f;
		anim_state->m_time_since_stop = 0.0f;
		anim_state->m_time_since_move += anim_state->m_last_clientside_anim_update_time_delta;
	}

	if ( !CLIENT_DLL_ANIMS ) {
		if ( !*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x138 )
			&& bStoppedMovingThisFrame
			&& anim_state->m_on_ground
			&& !*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x128 )
			&& !anim_state->m_hit_ground
			&& *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x2AC ) < 50.0f ) {
			set_sequence( anim_state, 3 , select_weighted_seq ( anim_state, act_csgo_idle_adjust_stoppedmoving ) );
			*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x138 ) = true;
		}

		if ( get_layer_activity( anim_state , 3 ) == act_csgo_idle_adjust_stoppedmoving || get_layer_activity( anim_state , 3 ) == act_csgo_idle_turn_balanceadjust ) {
			if ( *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x138 ) && anim_state->m_unk_feet_speed_ratio <= 0.25f ) {
				increment_layer_cycle_weight_rate_generic( anim_state, 3 );
				*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x138 ) = !is_sequence_completed( anim_state , 3 );
			}
			else {
				*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x138 ) = false;
				float flWeight = player->layers ( ) [ 3 ].m_weight;
				set_weight( anim_state, 3 , valve_math::Approach( 0.0f , flWeight , anim_state->m_last_clientside_anim_update_time_delta * 5.0f ) );
				set_weight_rate( anim_state, 3 , flWeight );
			}
		}
	}

	// if the player is looking far enough to either side, turn the feet to keep them within the extent of the aim matrix
	anim_state->m_feet_yaw = anim_state->m_abs_yaw;
	anim_state->m_abs_yaw = std::clamp( anim_state->m_abs_yaw , -360.0f , 360.0f );

	const auto delta_feet = valve_math::AngleDiff ( anim_state->m_eye_yaw, anim_state->m_abs_yaw );

	// narrow the available aim matrix width as speed increases
	auto matrix_width = std::lerp ( 1.0f, std::lerp ( 0.8f, 0.5f, anim_state->m_ground_fraction ), std::clamp ( anim_state->m_run_speed, 0.0f, 1.0f ) );

	if ( anim_state->m_duck_amount > 0.0f )
		matrix_width = std::lerp ( matrix_width, 0.5f, anim_state->m_duck_amount * std::clamp ( anim_state->m_unk_feet_speed_ratio, 0.0f, 1.0f ) );

	const auto max_yaw = anim_state->m_max_yaw * matrix_width;
	const auto min_yaw = anim_state->m_min_yaw * matrix_width;

	if ( delta_feet > max_yaw )
		anim_state->m_abs_yaw = anim_state->m_eye_yaw - abs ( max_yaw );
	else if ( delta_feet < min_yaw )
		anim_state->m_abs_yaw = anim_state->m_eye_yaw + abs ( min_yaw );

	anim_state->m_abs_yaw = valve_math::AngleNormalize( anim_state->m_abs_yaw );

	// pull the lower body direction towards the eye direction, but only when the player is moving
	if ( anim_state->m_on_ground && !force_feet_yaw ) {
		//if ( !CLIENT_DLL_ANIMS && player == g::local )
		//	lby::in_update = false;

		if ( anim_state->m_speed2d > 0.1f ) {
			anim_state->m_abs_yaw = valve_math::ApproachAngle ( anim_state->m_eye_yaw, anim_state->m_abs_yaw, anim_state->m_last_clientside_anim_update_time_delta * ( 30.0f + 20.0f * anim_state->m_ground_fraction ) );

			//if ( !CLIENT_DLL_ANIMS ) {
			//	last_lby_update_time [ player->idx ( ) ] = cs::i::globals->m_realtime + 0.22f;
			//	player->lby ( ) = anim_state->m_eye_yaw;
			//}
		}
		else {
			anim_state->m_abs_yaw = valve_math::ApproachAngle ( player->lby ( ), anim_state->m_abs_yaw, anim_state->m_last_clientside_anim_update_time_delta * 100.0f );

			//if ( !CLIENT_DLL_ANIMS ) {
			//	if ( cs::i::globals->m_realtime > last_lby_update_time [ player->idx ( ) ] && abs ( valve_math::AngleDiff ( anim_state->m_abs_yaw, anim_state->m_eye_yaw ) ) > 35.0f ) {
			//		last_lby_update_time [ player->idx ( ) ] = cs::i::globals->m_realtime + 1.1f;
			//		player->lby ( ) = anim_state->m_eye_yaw;
			//	}
			//}
		}
	}

	if ( !CLIENT_DLL_ANIMS ) {
		if ( anim_state->m_speed2d <= 1.0f
			&& anim_state->m_on_ground
			&& !*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x128 )
			&& !anim_state->m_hit_ground
			&& anim_state->m_last_clientside_anim_update_time_delta > 0.0f
			&& abs( valve_math::AngleDiff( anim_state->m_feet_yaw , anim_state->m_abs_yaw ) / anim_state->m_last_clientside_anim_update_time_delta > 120.0f ) ) {
			set_sequence( anim_state , 3 , select_weighted_seq ( anim_state, act_csgo_idle_turn_balanceadjust ) );
			*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x138 ) = true;
		}
	}
	else {
		if ( player->layers()[3].m_weight > 0.0f ) {
			increment_layer_cycle( anim_state, 3 , false );
			increment_layer_weight( anim_state , 3 );
		}
	}

	// the final model render yaw is aligned to the foot yaw
	if ( anim_state->m_speed2d > 0.0f && anim_state->m_on_ground ) {
		// convert horizontal velocity vec to angular yaw
		float flRawYawIdeal = ( atan2( -anim_state->m_vel.y , -anim_state->m_vel.x ) * 180.0f / std::numbers::pi );

		if ( flRawYawIdeal < 0.0f )
			flRawYawIdeal += 360.0f;

		anim_state->m_body_yaw_clamped = valve_math::AngleNormalize( valve_math::AngleDiff( flRawYawIdeal , anim_state->m_abs_yaw ) );
	}

	// delta between current yaw and ideal velocity derived target (possibly negative!)
	anim_state->m_feet_vel_dir_delta = valve_math::AngleNormalize( valve_math::AngleDiff( anim_state->m_body_yaw_clamped , anim_state->m_body_yaw ) );

	if ( bStartedMovingThisFrame && anim_state->m_feet_yaw_rate <= 0.0f ) {
		anim_state->m_body_yaw = anim_state->m_body_yaw_clamped;

		// select a special starting cycle that's set by the animator in content
		const auto nMoveSeq = player->layers()[6].m_sequence;

		if ( nMoveSeq != -1 ) {
			auto seqdesc = seq_desc( get_model_ptr( player ), nMoveSeq );

			if ( *reinterpret_cast< int* >( reinterpret_cast<uintptr_t>( seqdesc ) + 196 ) /* seqdesc.numanimtags */ > 0 ) {
				if ( abs( valve_math::AngleDiff( anim_state->m_body_yaw , 180.0f ) ) <= 22.5f ) //N
					anim_state->m_feet_cycle = get_first_sequence_anim_tag( anim_state , nMoveSeq , 1 , 0.0f , 1.0f );
				else if ( abs( valve_math::AngleDiff( anim_state->m_body_yaw , 135.0f ) ) <= 22.5f ) //NE
					anim_state->m_feet_cycle = get_first_sequence_anim_tag( anim_state , nMoveSeq , 2 , 0.0f, 1.0f );
				else if ( abs( valve_math::AngleDiff( anim_state->m_body_yaw , 90.0f ) ) <= 22.5f ) //E
					anim_state->m_feet_cycle = get_first_sequence_anim_tag( anim_state , nMoveSeq , 3 , 0.0f, 1.0f );
				else if ( abs( valve_math::AngleDiff( anim_state->m_body_yaw , 45.0f ) ) <= 22.5f ) //SE
					anim_state->m_feet_cycle = get_first_sequence_anim_tag( anim_state , nMoveSeq , 4 , 0.0f, 1.0f );
				else if ( abs( valve_math::AngleDiff( anim_state->m_body_yaw , 0.0f ) ) <= 22.5f ) //S
					anim_state->m_feet_cycle = get_first_sequence_anim_tag( anim_state , nMoveSeq , 5 , 0.0f, 1.0f );
				else if ( abs( valve_math::AngleDiff( anim_state->m_body_yaw , -45.0f ) ) <= 22.5f ) //SW
					anim_state->m_feet_cycle = get_first_sequence_anim_tag( anim_state , nMoveSeq , 6 , 0.0f, 1.0f );
				else if ( abs( valve_math::AngleDiff( anim_state->m_body_yaw , -90.0f ) ) <= 22.5f ) //W
					anim_state->m_feet_cycle = get_first_sequence_anim_tag( anim_state , nMoveSeq , 7 , 0.0f, 1.0f );
				else if ( abs( valve_math::AngleDiff( anim_state->m_body_yaw , -135.0f ) ) <= 22.5f ) //NW
					anim_state->m_feet_cycle = get_first_sequence_anim_tag( anim_state , nMoveSeq , 8 , 0.0f, 1.0f );
			}
		}

		if ( CLIENT_DLL_ANIMS ) {
			if ( anim_state->m_unk_fraction >= 1.0f
				&& !anim_state->m_force_update
				&& abs( anim_state->m_feet_vel_dir_delta ) > 45.0f
				&& anim_state->m_on_ground
				&& *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( player ) + m_boneSnapshots_BONESNAPSHOT_ENTIRE_BODY + 4 ) <= 0.0f ) {
				bone_shapshot_should_capture( reinterpret_cast< void* >( reinterpret_cast< uintptr_t >( player ) + m_boneSnapshots_BONESNAPSHOT_ENTIRE_BODY ), 0.3f );
			}
		}
	}
	else {
		if ( player->layers()[7].m_weight >= 1.0f ) {
			anim_state->m_body_yaw = anim_state->m_body_yaw_clamped;
		}
		else {
			if ( CLIENT_DLL_ANIMS) {
				if ( anim_state->m_unk_fraction >= 1.0f
					&& !anim_state->m_force_update
					&& abs( anim_state->m_feet_vel_dir_delta ) > 100.0f
					&& anim_state->m_on_ground
					&& *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( player ) + m_boneSnapshots_BONESNAPSHOT_ENTIRE_BODY + 4 ) <= 0.0f ) {
					bone_shapshot_should_capture( reinterpret_cast< void* >( reinterpret_cast< uintptr_t >( player ) + m_boneSnapshots_BONESNAPSHOT_ENTIRE_BODY ) , 0.3f );
				}
			}

			float flMoveWeight = std::lerp( std::clamp( anim_state->m_run_speed , 0.0f , 1.0f ) , std::clamp( anim_state->m_unk_feet_speed_ratio , 0.0f , 1.0f ), anim_state->m_duck_amount );
			float flRatio = valve_math::Bias( flMoveWeight , 0.18f ) + 0.1f;

			anim_state->m_body_yaw = valve_math::AngleNormalize( anim_state->m_body_yaw + ( anim_state->m_feet_vel_dir_delta * flRatio ) );
		}
	}

	player->poses( )[ pose_param_t::move_yaw ] = std::clamp( valve_math::AngleNormalize( anim_state->m_body_yaw ) , -180.0f , 180.0f ) / 360.0f + 0.5f;
	//anim_state->m_move_yaw_pose.set_value( player, anim_state->m_body_yaw );
	
	float flAimYaw = valve_math::AngleDiff( anim_state->m_eye_yaw , anim_state->m_abs_yaw );

	if ( flAimYaw >= 0.0f && anim_state->m_max_yaw != 0.0f )
		flAimYaw = ( flAimYaw / anim_state->m_max_yaw ) * 58.0f;
	else if ( anim_state->m_min_yaw != 0.0f )
		flAimYaw = ( flAimYaw / anim_state->m_min_yaw ) * -58.0f;

	player->poses( )[ pose_param_t::body_yaw ] = std::clamp( flAimYaw , -58.0f , 58.0f ) / 116.0f + 0.5f;
	//anim_state->m_body_yaw_pose.set_value( player , flAimYaw );

	// we need non-symmetrical arbitrary min/max bounds for vertical aim (pitch) too
	float flPitch = anim_state->m_pitch;

	if ( flPitch <= 0.0f )
		flPitch = ( flPitch / anim_state->m_max_pitch ) * -90.0f;
	else
		flPitch = ( flPitch / anim_state->m_min_pitch ) * 90.0f;

	player->poses( )[ pose_param_t::body_pitch ] = std::clamp( flPitch , -90.0f , 90.0f ) / 180.0f + 0.5f;
	//anim_state->m_body_pitch_pose.set_value( player , flPitch );
	player->poses( )[ pose_param_t::speed ] = std::clamp( anim_state->m_run_speed, 0.0f, 1.0f );
	//anim_state->m_speed_pose.set_value( player , anim_state->m_run_speed );
	player->poses( )[ pose_param_t::stand ] = std::clamp( 1.0f - ( anim_state->m_unk_fraction * anim_state->m_duck_amount ) , 0.0f , 1.0f );
	//anim_state->m_stand_pose.set_value( player , 1.0f - ( anim_state->m_unk_fraction * anim_state->m_duck_amount ) );
}

void anims::rebuilt::setup_aim_matrix( animstate_t* anim_state ) {
	static auto CCSGOPlayerAnimState__SetUpAimMatrix = pattern::search( _( "client.dll" ) , _( "55 8B EC 81 EC ? ? ? ? 53 56 57 8B 3D" ) ).get<void( __thiscall* )( animstate_t* )>( );
	CCSGOPlayerAnimState__SetUpAimMatrix( anim_state );
}

void anims::rebuilt::setup_weapon_action( animstate_t* anim_state ) {
	static auto CCSGOPlayerAnimState__SetUpWeaponAction = pattern::search( _( "client.dll" ) , _( "55 8B EC 51 53 56 57 8B F9 8B 77 60" ) ).get<void( __thiscall* )( animstate_t* )>( );
	CCSGOPlayerAnimState__SetUpWeaponAction( anim_state );
}

void anims::rebuilt::setup_movement( animstate_t* anim_state ) {
	//static auto CCSGOPlayerAnimState__SetUpMovement = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 81 EC 88 00 00 00 56 57 8B 3D ? ? ? ? 8B F1 8B CF 89" ) ).get<void( __thiscall* )( animstate_t* )>( );
	//CCSGOPlayerAnimState__SetUpMovement( anim_state );

	static auto CCSGOPlayerAnimState__GetWeaponPrefix = pattern::search ( _ ( "client.dll" ), _ ( "53 56 57 8B F9 33 F6 8B 4F 60" ) ).get<const char* ( __thiscall* )( animstate_t* )> ( );
	static auto m_iMoveState = pattern::search ( _ ( "client.dll" ), _ ( "8B 81 ? ? ? ? 3B 86 ? ? ? ? 74" ) ).add(2).deref().get<uint32_t> ( );

	MDLCACHE_CRITICAL_SECTION ( );
	
	const auto player = anim_state->m_entity;
	
	if ( anim_state->m_ground_fraction > 0.0f && anim_state->m_ground_fraction < 1.0f ) {
		if ( !anim_state->m_moving )
			anim_state->m_ground_fraction += anim_state->m_last_clientside_anim_update_time_delta * 2.0f;
		else
			anim_state->m_ground_fraction -= anim_state->m_last_clientside_anim_update_time_delta * 2.0f;
	
		anim_state->m_ground_fraction = std::clamp ( anim_state->m_ground_fraction, 0.0f, 1.0f );
	}
	
	if ( anim_state->m_speed2d > 135.2f && anim_state->m_moving ) {
		anim_state->m_moving = false;
		anim_state->m_ground_fraction = std::max ( 0.01f, anim_state->m_ground_fraction );
	}
	else if ( anim_state->m_speed2d < 135.2f && !anim_state->m_moving ) {
		anim_state->m_moving = true;
		anim_state->m_ground_fraction = std::min ( 0.99f, anim_state->m_ground_fraction );
	}

	player->poses ( ) [ pose_param_t::move_blend_walk ] = ( 1.0f - anim_state->m_ground_fraction ) * ( 1.0f - anim_state->m_duck_amount );
	player->poses ( ) [ pose_param_t::move_blend_run ] = anim_state->m_ground_fraction * ( 1.0f - anim_state->m_duck_amount );
	player->poses ( ) [ pose_param_t::move_blend_crouch ] = anim_state->m_duck_amount;
	
	char szWeaponMoveSeq [ 64 ];
	sprintf_s ( szWeaponMoveSeq, _("move_%s"), CCSGOPlayerAnimState__GetWeaponPrefix ( anim_state ) );
	
	auto nWeaponMoveSeq = player->lookup_sequence ( szWeaponMoveSeq );
	
	if ( nWeaponMoveSeq == -1 )
		nWeaponMoveSeq = player->lookup_sequence ( _("move") );
	
	if ( *reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( player ) + m_iMoveState ) != *reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x2A8 ) )
		*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x2AC ) += 10.0f;

	*reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x2A8 ) = *reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( player ) + m_iMoveState );
	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x2AC ) = std::clamp ( valve_math::Approach ( 0, *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x2AC ), anim_state->m_last_clientside_anim_update_time_delta * 40.0f ), 0.0f, 100.0f );

	// recompute moveweight

	const auto flTargetMoveWeight = std::lerp ( std::clamp ( anim_state->m_run_speed, 0.0f, 1.0f ), std::clamp ( anim_state->m_unk_feet_speed_ratio, 0.0f, 1.0f ), anim_state->m_duck_amount );

	if ( anim_state->m_feet_yaw_rate <= flTargetMoveWeight )
		anim_state->m_feet_yaw_rate = flTargetMoveWeight;
	else
		anim_state->m_feet_yaw_rate = valve_math::Approach ( flTargetMoveWeight, anim_state->m_feet_yaw_rate, anim_state->m_last_clientside_anim_update_time_delta * valve_math::RemapValClamped ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x2AC ), 0.0f, 100.0f, 2, 20 ) );

	auto vecMoveYawDir = cs::angle_vec( vec3_t ( 0.0f, valve_math::AngleNormalize ( anim_state->m_abs_yaw + anim_state->m_body_yaw + 180.0f ), 0.0f ) );
	const auto flYawDeltaAbsDot = abs ( anim_state->m_vel_norm_nonzero.dot_product( vecMoveYawDir ) );
	anim_state->m_feet_yaw_rate *= valve_math::Bias ( flYawDeltaAbsDot, 0.2f );

	auto flMoveWeightWithAirSmooth = anim_state->m_feet_yaw_rate * anim_state->m_unk_fraction;

	// dampen move weight for landings
	flMoveWeightWithAirSmooth *= std::max ( ( 1.0f - player->layers()[5].m_weight ), 0.55f );

	auto flMoveCycleRate = 0.0f;
	if ( anim_state->m_speed2d > 0.0f ) {
		flMoveCycleRate = player->get_sequence_cycle_rate_server ( nWeaponMoveSeq );
		
		const auto flSequenceGroundSpeed = std::max ( player->get_sequence_move_distance ( get_model_ptr(player), nWeaponMoveSeq ) / ( 1.0f / flMoveCycleRate ), 0.001f );

		flMoveCycleRate *= anim_state->m_speed2d / flSequenceGroundSpeed;
		flMoveCycleRate *= std::lerp ( 1.0f, 0.85f, anim_state->m_ground_fraction );
	}

	auto clamp_cycle = [ ] ( float in ) {
		in -= int ( in );

		if ( in < 0.0f )
			in += 1.0f;
		else if ( in > 1.0f )
			in -= 1.0f;

		return in;
	};

	float flLocalCycleIncrement = ( flMoveCycleRate * anim_state->m_last_clientside_anim_update_time_delta );
	anim_state->m_feet_cycle = clamp_cycle ( anim_state->m_feet_cycle + flLocalCycleIncrement );

	flMoveWeightWithAirSmooth = std::clamp ( flMoveWeightWithAirSmooth, 0.0f, 1.0f );
	update_layer ( anim_state, 6, nWeaponMoveSeq, flLocalCycleIncrement, flMoveWeightWithAirSmooth, anim_state->m_feet_cycle );

	if ( !CLIENT_DLL_ANIMS && player == g::local && g::local ) {
		/* @ m_iFOV + 0x14 */
		const auto buttons = *reinterpret_cast< buttons_t* > ( reinterpret_cast< uintptr_t >( player ) + 0x3208 );

		const auto moveRight = !!( buttons & buttons_t::right );
		const auto moveLeft = !!( buttons & buttons_t::left );
		const auto moveForward = !!( buttons & buttons_t::forward );
		const auto moveBackward = !!( buttons & buttons_t::back );

		vec3_t vecForward;
		vec3_t vecRight;
		valve_math::AngleVectors ( vec3_t ( 0.0f, anim_state->m_abs_yaw, 0.0f ), &vecForward, &vecRight, nullptr );

		vecRight.normalize_place ( );

		const auto flVelToRightDot = anim_state->m_vel_norm_nonzero.dot_product ( vecRight );
		const auto flVelToForwardDot = anim_state->m_vel_norm_nonzero.dot_product ( vecForward );

		const auto bStrafeRight = ( anim_state->m_run_speed >= 0.73f && moveRight && !moveLeft && flVelToRightDot < -0.63f );
		const auto bStrafeLeft = ( anim_state->m_run_speed >= 0.73f && moveLeft && !moveRight && flVelToRightDot > 0.63f );
		const auto bStrafeForward = ( anim_state->m_run_speed >= 0.65f && moveForward && !moveBackward && flVelToForwardDot < -0.55f );
		const auto bStrafeBackward = ( anim_state->m_run_speed >= 0.65f && moveBackward && !moveForward && flVelToForwardDot > 0.55f );

		player->strafing ( ) = ( bStrafeRight || bStrafeLeft || bStrafeForward || bStrafeBackward );
	}

	if ( player->strafing() ) {
		if ( !*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x1A0 ) ) {
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x1A4 ) = 0.0f;

			if ( CLIENT_DLL_ANIMS ) {
				//if ( !anim_state->m_force_update && !*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x1A0 ) && anim_state->m_on_ground && m_pPlayer->m_boneSnapshots [ BONESNAPSHOT_UPPER_BODY ].GetCurrentWeight ( ) <= 0.0f )
				//	m_pPlayer->m_boneSnapshots [ BONESNAPSHOT_UPPER_BODY ].SetShouldCapture ( bonesnapshot_get ( cl_bonesnapshot_speed_strafestart ) );
			}
		}

		*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x1A0 ) = true;
		*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x190 ) = valve_math::Approach ( 1.0f, *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x190 ), anim_state->m_last_clientside_anim_update_time_delta * 20.0f );
		*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x198 ) = valve_math::Approach ( 0.0f, *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x198 ), anim_state->m_last_clientside_anim_update_time_delta * 10.0f );

		player->poses ( ) [ pose_param_t::strafe_yaw ] = std::clamp ( valve_math::AngleNormalize ( anim_state->m_body_yaw ), -180.0f, 180.0f ) / 360.0f + 0.5f;
	}
	else if ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x190 ) > 0.0f ) {
		*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x1A4 ) += anim_state->m_last_clientside_anim_update_time_delta;

		if ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x1A4 ) > 0.08f )
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x190 ) = valve_math::Approach ( 0.0f, *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x190 ), anim_state->m_last_clientside_anim_update_time_delta * 5.0f );
		
		/* save performance */
		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x19C ) = player->lookup_sequence ( _("strafe" ));
		float flRate = player->get_sequence_cycle_rate_server ( *reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x19C ) );
		*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x198 ) = std::clamp ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x198 ) + anim_state->m_last_clientside_anim_update_time_delta * flRate, 0.0f, 1.0f );
	}

	if ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x190 ) <= 0.0f )
		*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x1A0 ) = false;

	// keep track of if the player is on the ground, and if the player has just touched or left the ground since the last check
	const auto bPreviousGroundState = anim_state->m_on_ground;
	anim_state->m_on_ground = !!( player->flags() & flags_t::on_ground );

	*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x120 ) = ( !anim_state->m_force_update && bPreviousGroundState != anim_state->m_on_ground && anim_state->m_on_ground );
	*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x121 ) = ( bPreviousGroundState != anim_state->m_on_ground && !anim_state->m_on_ground );

	auto flDistanceFell = 0.0f;

	if ( *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x121 ) )
		*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x114 ) = anim_state->m_origin.z;

	if ( *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x120 ) ) {
		flDistanceFell = abs ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x114 ) - anim_state->m_origin.z );
		float flDistanceFallNormalizedBiasRange = valve_math::Bias ( valve_math::RemapValClamped ( flDistanceFell, 12.0f, 72.0f, 0.0f, 1.0f ), 0.4f );

		*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x118 ) = std::clamp ( valve_math::Bias ( anim_state->m_time_in_air, 0.3f ), 0.1f, 1.0f );
		anim_state->m_landing_duck_additive = std::max ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x118 ), flDistanceFallNormalizedBiasRange );
	}
	else {
		anim_state->m_landing_duck_additive = valve_math::Approach ( 0.0f, anim_state->m_landing_duck_additive, anim_state->m_last_clientside_anim_update_time_delta * 2.0f );
	}

	// the in-air smooth value is a fuzzier representation of if the player is on the ground or not.
	// It will approach 1 when the player is on the ground and 0 when in the air. Useful for blending jump animations.
	anim_state->m_unk_fraction = valve_math::Approach ( anim_state->m_on_ground ? 1.0f : 0.0f, anim_state->m_unk_fraction, std::lerp ( 8.0f, 16.0f, anim_state->m_duck_amount ) * anim_state->m_last_clientside_anim_update_time_delta );
	anim_state->m_unk_fraction = std::clamp ( anim_state->m_unk_fraction, 0.0f, 1.0f );

	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x190 ) *= ( 1.0f - anim_state->m_duck_amount );
	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x190 ) *= anim_state->m_unk_fraction;
	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x190 ) = std::clamp ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x190 ), 0.0f, 1.0f );

	if ( *reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x19C ) != -1 )
		update_layer ( anim_state, 7, *reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x19C ), 0.0f, *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x190 ), *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x198 ) );

	//ladders
	bool bPreviouslyOnLadder = *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x128 );
	*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x128 ) = !anim_state->m_on_ground && player->movetype ( ) == movetypes_t::ladder;
	bool bStartedLadderingThisFrame = ( !bPreviouslyOnLadder && *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x128 ) );
	bool bStoppedLadderingThisFrame = ( bPreviouslyOnLadder && !*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x128 ) );

	if ( bStartedLadderingThisFrame || bStoppedLadderingThisFrame ) {
		if ( CLIENT_DLL_ANIMS ) {
			//m_footLeft.m_flLateralWeight = 0;
		//m_footRight.m_flLateralWeight = 0;
		}
	}

	if ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x12C ) > 0.0f || *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x128 ) ) {
		if ( !CLIENT_DLL_ANIMS ) {
			if ( bStartedLadderingThisFrame ) {
				set_sequence ( anim_state, 5, select_weighted_seq ( anim_state, act_csgo_climb_ladder ) );
			}
		}

		if ( abs ( anim_state->m_up_vel ) > 100.0f )
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x130 ) = valve_math::Approach ( 1.0f, *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x130 ), anim_state->m_last_clientside_anim_update_time_delta * 10.0f );
		else
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x130 ) = valve_math::Approach ( 0.0f, *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x130 ), anim_state->m_last_clientside_anim_update_time_delta * 10.0f );

		*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x130 ) = std::clamp ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x130 ), 0.0f, 1.0f );

		if ( *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x128 ) )
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x12C ) = valve_math::Approach ( 1.0f, *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x12C ), anim_state->m_last_clientside_anim_update_time_delta * 5.0f );
		else
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x12C ) = valve_math::Approach ( 0.0f, *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x12C ), anim_state->m_last_clientside_anim_update_time_delta * 10.0f );

		*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x12C ) = std::clamp ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x12C ), 0.0f, 1.0f );

		const auto flLadderYaw = valve_math::AngleDiff ( cs::vec_angle( player->ladder_norm ( ) ).y, anim_state->m_abs_yaw );

		player->poses ( ) [ pose_param_t::ladder_yaw ] = std::clamp ( valve_math::AngleNormalize ( flLadderYaw ), -180.0f, 180.0f ) / 360.0f + 0.5f;

		auto flLadderClimbCycle = player->layers ( ) [ 5 ].m_cycle;
		flLadderClimbCycle += ( anim_state->m_origin.z - anim_state->m_old_origin.z ) * std::lerp ( 0.010f, 0.004f, *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x130 ) );

		player->poses ( ) [ pose_param_t::ladder_speed ] = std::clamp ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x130 ), 0.0f, 1.0f );

		if ( get_layer_activity ( anim_state, 5 ) == 987 )
			set_weight ( anim_state, 5, *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x12C ) );

		set_cycle ( anim_state, 5, flLadderClimbCycle );

		// fade out jump if we're climbing
		if ( *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x128 ) ) {
			const auto flIdealJumpWeight = 1.0f - *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x12C );

			if ( player->layers()[4].m_weight > flIdealJumpWeight ) {
				set_weight ( anim_state, 4, flIdealJumpWeight );
			}
		}
	}
	else {
		*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x130 ) = 0.0f;
	}

	// jumping
	if ( anim_state->m_on_ground ) {
		if ( !anim_state->m_hit_ground && ( *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x120 ) || bStoppedLadderingThisFrame ) ) {
			if ( !CLIENT_DLL_ANIMS) {
				set_sequence ( anim_state, 5, select_weighted_seq ( anim_state, anim_state->m_time_in_air > 1.0f ? act_csgo_land_heavy : act_csgo_land_light ) );
			}

			set_cycle ( anim_state, 5, 0.0f );

			anim_state->m_hit_ground = true;
		}

		anim_state->m_time_in_air = 0.0f;

		if ( anim_state->m_hit_ground && get_layer_activity ( anim_state, 5 ) != 987 ) {
			if ( !CLIENT_DLL_ANIMS ) {
				*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x10A ) = false;
			}

			increment_layer_cycle ( anim_state, 5, false );
			increment_layer_cycle ( anim_state, 4, false );

			player->poses ( ) [ pose_param_t::jump_fall ] = 0.0f;

			if ( is_sequence_completed ( anim_state, 5 ) ) {
				anim_state->m_hit_ground = false;

				set_weight ( anim_state, 5, 0.0f );
				set_weight ( anim_state, 4, 0.0f );

				*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x118 ) = 1.0f;
			}
			else {
				auto land_weight = get_layer_ideal_weight_from_seq_cycle ( anim_state, 5 ) * *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x118 );

				// if we hit the ground crouched, reduce the land animation as a function of crouch, since the land animations move the head up a bit ( and this is undesirable )
				land_weight *= std::clamp ( ( 1.0f - anim_state->m_duck_amount ), 0.2f, 1.0f );

				set_weight ( anim_state, 5, land_weight );

				// fade out jump because land is taking over
				if ( player->layers ( ) [ 4 ].m_weight > 0.0f )
					set_weight ( anim_state, 4, valve_math::Approach ( 0.0f, player->layers ( ) [ 4 ].m_weight, anim_state->m_last_clientside_anim_update_time_delta * 10.0f ) );
			}
		}

		if ( !CLIENT_DLL_ANIMS ) {
			if ( !anim_state->m_hit_ground && !*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x10A ) && *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x12C ) <= 0.0f )
				set_weight ( anim_state, 5, 0.0f );
		}
	}
	else if ( !*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x128 ) ) {
		anim_state->m_hit_ground = false;

		// we're in the air
		if ( *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x121 ) || bStoppedLadderingThisFrame ) {
			if ( !CLIENT_DLL_ANIMS ) {
				// If entered the air by jumping, then we already set the jump activity.
				// But if we're in the air because we strolled off a ledge or the floor collapsed or something,
				// we need to set the fall activity here.
				if ( !*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x10A ) ) {
					set_sequence ( anim_state, 4, 14 );
				}
			}

			anim_state->m_time_in_air = 0.0f;
		}

		anim_state->m_time_in_air += anim_state->m_last_clientside_anim_update_time_delta;

		increment_layer_cycle ( anim_state, 4, false );

		// increase jump weight
		const auto next_jump_weight = get_layer_ideal_weight_from_seq_cycle ( anim_state, 4 );

		if ( next_jump_weight > player->layers()[4].m_weight )
			set_weight ( anim_state, 4, next_jump_weight );

		// bash any lingering land weight to zero
		auto smoothstep_bounds = [ ] ( float a, float b, float x ) {
			const auto r = std::clamp ( ( x - a ) * b, 0.0f, 1.0f );
			return ( 3.0f - ( r + r ) ) * ( r * r );
		};

		if ( player->layers ( ) [ 5 ].m_weight > 0.0f )
			set_weight ( anim_state, 5, player->layers ( ) [ 5 ].m_weight * smoothstep_bounds ( 0.2f, 0.0f, anim_state->m_time_in_air ) );

		// blend jump into fall. This is a no-op if we're playing a fall anim.
		player->poses ( ) [ pose_param_t::jump_fall ] = std::clamp ( smoothstep_bounds ( 0.72f, 1.52f, anim_state->m_time_in_air ), 0.0f, 1.0f );
	}
}

void anims::rebuilt::setup_alive_loop( animstate_t* anim_state ) {
	static auto CCSGOPlayerAnimState__SetUpAliveloop = pattern::search( _( "client.dll" ) , _( "55 8B EC 51 56 8B 71 60 83 BE ? ? ? ? 00 0F 84 ? ? ? ? 8B B6 ? ? ? ? 81 C6 68 02 00 00" ) ).get<void( __thiscall* )( animstate_t* )>( );
	CCSGOPlayerAnimState__SetUpAliveloop( anim_state );
}

void anims::rebuilt::setup_whole_body_action( animstate_t* anim_state ) {
	static auto CCSGOPlayerAnimState__SetUpWholeBodyAction = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 EC 08 56 57 8B F9 8B 77" ) ).get<void( __thiscall* )( animstate_t* )>( );
	CCSGOPlayerAnimState__SetUpWholeBodyAction( anim_state );
}

std::array<float , 65> last_flash_duration { 0.0f };

void anims::rebuilt::setup_flashed_reaction( animstate_t* anim_state ) {
	const auto player = anim_state->m_entity;

	if ( !CLIENT_DLL_ANIMS ) {
		/* part of CCSPlayer::Blind */
		// Magic numbers to reduce the fade time to within 'perceptible' range.
		// Players can see well enough to shoot back somewhere around 50% white plus burn-in effect.
		// Varies by player and amount of panic ;)
		// So this makes raised arm goes down earlier, making it a better representation of actual blindness.

		// we can detect if player was just flashed by increase of flash duration
		const auto just_flashed = player->flash_duration( ) > last_flash_duration[ player->idx( ) ];
		last_flash_duration [ player->idx() ] = player->flash_duration( );
		if ( just_flashed ) {
			float holdTime = player->flash_duration( ) * 0.7f;
			float fadeTime = holdTime * 2.0f;

			fadeTime *= 0.7f;

			float flAdjustedHold = holdTime * 0.45f;
			float flAdjustedEnd = fadeTime * 0.7f;

			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0xC ) = cs::i::globals->m_curtime + flAdjustedHold;
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x10 ) = cs::i::globals->m_curtime + flAdjustedEnd;

			// This check moves the ease-out start and end to account for a non-255 starting alpha.
			// However it looks like starting alpha is ALWAYS 255, since no current code path seems to ever pass in less.
			//if ( player->flash_alpha( ) < 255.0f ) {
			//	const auto flScaleBack = 1.0f - ( player->flash_alpha( ) * flAdjustedEnd * 0.0039215689f );
			//
			//	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0xC ) -= flScaleBack;
			//	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x10 ) -= flScaleBack;
			//}

			// when fade out time is very soon, don't pull the arm up all the way. It looks silly and robotic.
			if ( flAdjustedEnd < 1.5f )
				*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0xC ) -= 1.0f;
		}

		/* continued */
		if ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x10 ) < cs::i::globals->m_curtime ) {
			set_weight( anim_state, 9 , 0.0f );
			*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x18C ) = false;
		}
		else {
			if ( !*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x18C ) ) {
				set_sequence( anim_state, 9 , select_weighted_seq ( anim_state, act_csgo_flashbang_reaction ) );
				*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x18C ) = true;
			}

			auto flash_weight = 0.0f;
	
			if ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0xC ) == *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x10 ) ) {
				if ( cs::i::globals->m_curtime - *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x10 ) < 0.0f )
					flash_weight = 1.0f;
				else
					flash_weight = 0.0f;
			}
			else {
				const auto time_left = ( cs::i::globals->m_curtime - *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0xC ) ) / ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x10 ) - *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0xC ) );
				flash_weight = 1.0f - (( time_left >= 0.0f ) ? std::min( time_left , 1.0f ) : 0.0f);
			}
	
			set_cycle( anim_state , 9 , 0.0f );
			set_rate( anim_state , 9 , 0.0f );

			float flWeightPrevious = player->layers()[9].m_weight;
			float flWeightNew = ( ( flash_weight * flash_weight ) * 3.0f ) - ( ( ( flash_weight * flash_weight ) * 2.0f ) * flash_weight );

			set_weight( anim_state , 9 , flWeightNew );
			set_weight_rate( anim_state , 9 , ( flWeightNew >= flWeightPrevious ) ? 0.0f : flWeightPrevious );
		}
	}
	else {
		if ( player->layers( ) && player->layers( )[ 9 ].m_weight > 0.0f ) {
			if ( player->layers( )[ 9 ].m_weight_delta_rate < 0.0f )
				increment_layer_weight( anim_state , 9 );
		}
	}
}

void anims::rebuilt::setup_flinch( animstate_t* anim_state ) {
	static auto CCSGOPlayerAnimState__SetUpFlinch = pattern::search( _( "client.dll" ) , _( "55 8B EC 51 56 8B 71 60 83 BE ? ? ? ? 00 0F 84 ? ? ? ? 8B B6 ? ? ? ? 81 C6 30 02 00 00" ) ).get<void( __thiscall* )( animstate_t* )>( );
	CCSGOPlayerAnimState__SetUpFlinch( anim_state );
}

void anims::rebuilt::setup_lean( animstate_t* anim_state ) {	
	// lean the body into velocity derivative (acceleration) to simulate maintaining a center of gravity
	float flInterval = cs::i::globals->m_curtime - *reinterpret_cast<float*>( reinterpret_cast<uintptr_t>( anim_state ) + 0x158 );
	
	if ( flInterval > 0.025f ) {
		flInterval = std::min ( flInterval, 0.1f );
		*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x158 ) = cs::i::globals->m_curtime;
	
		*reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x168 ) = ( anim_state->m_vel - *reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x15C ) ) / flInterval;
		reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x168 )->z = 0.0f;
	
		*reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x15C ) = anim_state->m_vel;
	}
	
	*reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x174 ) = valve_math::ApproachVector ( *reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x168 ), *reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x174 ), anim_state->m_last_clientside_anim_update_time_delta * 800.0f );
	
	const auto temp = reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x174 )->cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );
	
	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x180 ) = std::clamp ( ( reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x174 )->length ( ) / 260.0f ) * anim_state->m_speed_normalized, 0.0f, 1.0f );
	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x180 ) *= ( 1.0f - *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x12C ) );
	
	anim_state->m_entity->poses ( ) [ pose_param_t::lean_yaw ] = std::clamp ( valve_math::AngleNormalize ( anim_state->m_abs_yaw - temp.y ), -180.0f, 180.0f ) / 360.0f + 0.5f;
	
	if ( anim_state->m_entity->layers ( ) && anim_state->m_entity->layers()[12].m_sequence <= 0 ) {
		MDLCACHE_CRITICAL_SECTION ( );
		set_sequence ( anim_state, 12, anim_state->m_entity->lookup_sequence ( _ ( "lean" ) ) );
	}
	
	set_weight ( anim_state, 12, *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( anim_state ) + 0x180 ) );
}

void anims::rebuilt::update( animstate_t* anim_state , vec3_t angles , vec3_t origin, bool force_feet_yaw ) {
	static auto m_flThirdpersonRecoil = pattern::search( _( "client.dll" ) , _( "F3 0F 10 86 ? ? ? ? F3 0F 58 44 24 0C" ) ).add(4).deref().get< uint32_t >( );
	static auto CCSGOPlayerAnimState__CacheSequences = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 34 53 56 8B F1 57 8B" ) ).get< bool( __thiscall* )( animstate_t* ) >( );
	static auto& s_bEnableInvalidateBoneCache = *pattern::search( _( "client.dll" ) , _( "C6 05 ? ? ? ? 00 F3 0F 5F 05 ? ? ? ? F3 0F 11 47 74" ) ).add(2).deref().get< bool* >( );

	if ( !anim_state )
		return;

	const auto player = anim_state->m_entity;

	if ( !player || !player->alive() || !CCSGOPlayerAnimState__CacheSequences( anim_state ) )
		return;

	// Apply recoil angle to aim matrix so bullets still come out of the gun straight while spraying
	angles.x = valve_math::AngleNormalize( angles.x + *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>( player ) + m_flThirdpersonRecoil ) );

	// negative values possible when clocks on client and server go out of sync..
	anim_state->m_last_clientside_anim_update_time_delta = std::max( 0.0f , cs::i::globals->m_curtime - anim_state->m_last_clientside_anim_update );

	//if ( CLIENT_DLL_ANIMS ) {
		// suspend bonecache invalidation
		s_bEnableInvalidateBoneCache = false;
	//}

	anim_state->m_eye_yaw = valve_math::AngleNormalize( angles.y );
	anim_state->m_pitch = valve_math::AngleNormalize( angles.x );
	anim_state->m_origin = origin;
	anim_state->m_weapon = player->weapon( );

	// purge layer dispatches on weapon change and init
	if ( anim_state->m_weapon != anim_state->m_last_weapon || anim_state->m_force_update ) {
		//if ( CLIENT_DLL_ANIMS ) {
			// changing weapons will change the pose of leafy bones like fingers. The next time we
			// set up this player's bones, treat it like a clean first setup.
			*reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( player ) + 0xA30 ) = 0;
		//}

		for ( auto i = 0; i < ANIMATION_LAYER_COUNT; i++ ) {
			const auto layer = player->layers( ) ? ( player->layers( ) + i ) : nullptr;

			if ( layer ) {
				*reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( layer ) + 8 ) = 0;
				*reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( layer ) + 12 ) = -1;
				*reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( layer ) + 16 ) = -1;
			}
		}
	}

	//if ( CLIENT_DLL_ANIMS && /*IsPreCrouchUpdateDemo( )*/ false ) {
	//	// compatibility for old demos using old crouch values
	//	const auto target_duck = ( static_cast< uint32_t >( player->flags( ) ) & 4 ) ? 1.0f : anim_state->m_landing_duck_additive;
	//
	//	anim_state->m_duck_amount = valve_math::Approach( target_duck , anim_state->m_duck_amount , anim_state->m_last_clientside_anim_update_time_delta * ( ( anim_state->m_duck_amount < target_duck ) ? CSGO_ANIM_DUCK_APPROACH_SPEED_DOWN : CSGO_ANIM_DUCK_APPROACH_SPEED_UP ) );
	//	anim_state->m_duck_amount = std::clamp( anim_state->m_duck_amount , 0.0f , 1.0f );
	//}
	//else {
		anim_state->m_duck_amount = std::clamp( valve_math::Approach( std::clamp( player->crouch_amount() + anim_state->m_landing_duck_additive , 0.0f , 1.0f ) , anim_state->m_duck_amount , anim_state->m_last_clientside_anim_update_time_delta * 6.0f ) , 0.0f , 1.0f );
	//}

	// no matter what, we're always playing 'default' underneath
	{
		MDLCACHE_CRITICAL_SECTION( );

		vfunc<void( __thiscall* )( player_t*, int )>( player, 219 )( player, 0 ); // m_pPlayer->SetSequence( 0 ); might become outdated
		*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( player ) + 0xA18 ) = 0.0f;
		
		if ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( player ) + 0xA14 ) != 0.0f ) {
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( player ) + 0xA14 ) = 0.0f;
			invalidate_physics_recursive( anim_state , 8 );
		}
	}

	/* **THIS IS NOT SUPPOSED TO BE HERE, CHANGE LATER. TEMPORARY SOLUTION.** */
	/* maybe using events would be better? */
	/* or animation layers to get the start time of animation events */
	trigger_animation_events ( anim_state );

	// all the layers get set up here
	setup_velocity( anim_state, force_feet_yaw );			// calculate speed and set up body yaw values
	setup_aim_matrix( anim_state );			// aim matrices are full body, so they not only point the weapon down the eye dir, they can crouch the idle player
	setup_weapon_action( anim_state );		// firing, reloading, silencer-swapping, deploying
	setup_movement( anim_state );			// jumping, climbing, ground locomotion, post-weapon crouch-stand
	setup_alive_loop( anim_state );			// breathe and fidget
	setup_whole_body_action( anim_state );	// defusing, planting, whole-body custom events
	setup_flashed_reaction( anim_state );	// shield eyes from flashbang
	setup_flinch( anim_state );				// flinch when shot
	setup_lean( anim_state );				// lean into acceleration

	//if ( CLIENT_DLL_ANIMS ) {
		// zero-sequences are un-set and should have zero weight on the client
		for ( auto i = 0; i < ANIMATION_LAYER_COUNT; i++ ) {
			const auto layer = player->layers( ) ? ( player->layers( ) + i ) : nullptr;

			if ( layer && !layer->m_sequence )
				set_weight( anim_state, i, 0.0f );
		}
	//}

	// force abs angles on client and server to line up hitboxes
	player->set_abs_angles( vec3_t( 0.0f , anim_state->m_abs_yaw , 0.0f ) );

	//if ( CLIENT_DLL_ANIMS ) {
		// restore bonecache invalidation
		s_bEnableInvalidateBoneCache = true;
	//}

	anim_state->m_last_weapon = anim_state->m_weapon;
	anim_state->m_old_origin = anim_state->m_origin;
	anim_state->m_force_update = false;
	anim_state->m_last_clientside_anim_update = cs::i::globals->m_curtime;
	anim_state->m_last_clientside_anim_framecount = cs::i::globals->m_framecount;
}