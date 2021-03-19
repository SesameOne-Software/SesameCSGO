#include "rebuilt.hpp"
#include "anims.hpp"

#include <numbers>

#undef min
#undef max

constexpr auto CLIENT_DLL_ANIMS = 0;

constexpr auto ANIMATION_LAYER_COUNT = 13;

constexpr auto CSGO_ANIM_DUCK_APPROACH_SPEED_DOWN = 3.1f;
constexpr auto CSGO_ANIM_DUCK_APPROACH_SPEED_UP = 6.0f;

namespace valve_math {
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
	const auto _layer = anim_state->m_entity->layers( ) + layer;

	if ( _layer->m_owner && _layer->m_sequence != sequence )
		invalidate_physics_recursive( anim_state, 16 );

	_layer->m_sequence = sequence;
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
	flCurrentCycle += anim_state->m_last_clientside_anim_update_time_delta * anim_state->m_entity->layers( )[ layer ].m_weight_delta_rate;

	if ( !allow_loop && flCurrentCycle >= 1 )
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
}

float anims::rebuilt::get_layer_ideal_weight_from_seq_cycle( animstate_t* anim_state , int layer ) {
	MDLCACHE_CRITICAL_SECTION( );
	const auto _layer = anim_state->m_entity->layers( ) + layer;

	auto seqdesc = seq_desc( get_model_ptr( anim_state->m_entity ) , _layer->m_sequence );

	float flCycle = _layer->m_cycle;

	if ( flCycle >= 0.999f )
		flCycle = 1.0f;

	float flEaseIn = *reinterpret_cast<float*>( reinterpret_cast<uintptr_t>( seqdesc ) + 104 );
	float flEaseOut = *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( seqdesc ) + 108 );
	float flIdealWeight = 1;

	auto smoothstep_bounds = [ ] ( float edge0 , float edge1 , float x ) {
		float v1 = std::clamp( ( x - edge0 ) / ( edge1 - edge0 ) , 0.0f , 1.0f );
		return v1 * v1 * ( 3.0f - 2.0f * v1 );
	};

	if ( flEaseIn > 0.0f && flCycle < flEaseIn )
		flIdealWeight = smoothstep_bounds( 0.0f , flEaseIn , flCycle );
	else if ( flEaseOut < 1 && flCycle > flEaseOut )
		flIdealWeight = smoothstep_bounds( 1.0f , flEaseOut , flCycle );

	if ( flIdealWeight < 0.0015f )
		return 0.0f;

	return std::clamp( flIdealWeight , 0.0f , 1.0f );
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

void anims::rebuilt::setup_velocity( animstate_t* anim_state ) {
	//static auto CCSGOPlayerAnimState__SetUpVelocity = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 30 56 57 8B 3D" ) ).get<void( __thiscall* )( animstate_t* )>( );
	//CCSGOPlayerAnimState__SetUpVelocity( anim_state );
	static auto C_CSPlayer__EstimateAbsVelocity = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 1C 53 56 57 8B F9 F7" ) ).get<void( __thiscall* )( player_t* )>( );
	static auto m_boneSnapshots_BONESNAPSHOT_ENTIRE_BODY = pattern::search( _( "client.dll" ) , _( "0F 2F 99 ? ? ? ? 0F 82 ? ? ? ? F3 0F 10 0D ? ? ? ? 81 C1" ) ).add(3).deref( ).sub(4).get<uint32_t>( );

	auto approach_vel = [ ] ( vec3_t& a , vec3_t& b, float rate ) {
		const auto delta = a - b;
		const auto delta_len = delta.length( );

		if ( delta_len <= rate ) {
			vec3_t result;

			if ( -rate <= delta_len ) {
				return a;
			}
			else {
				const auto iradius = 1.0f / ( delta_len + std::numeric_limits<float>::epsilon( ) );
				return b - ( ( delta * iradius ) * rate );
			}
		}
		else {
			const auto iradius = 1.0f / ( delta_len + std::numeric_limits<float>::epsilon( ) );
			return b + ( ( delta * iradius ) * rate );
		}
	};

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

	if ( abs ( vecAbsVelocity.x ) < 0.001f )
		vecAbsVelocity.x = 0.0f;
	if ( abs ( vecAbsVelocity.y ) < 0.001f )
		vecAbsVelocity.y = 0.0f;
	if ( abs ( vecAbsVelocity.z ) < 0.001f )
		vecAbsVelocity.z = 0.0f;

	// save vertical velocity component
	anim_state->m_up_vel = vecAbsVelocity.z;

	// discard z component
	vecAbsVelocity.z = 0.0f;

	// remember if the player is accelerating.
	*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x1AD ) = ( reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x15C )->length_sqr( ) < vecAbsVelocity.length_sqr( ) );

	// rapidly approach ideal velocity instead of instantly adopt it. This helps smooth out instant velocity changes, like
	// when the player runs headlong into a wall and their velocity instantly becomes zero.
	anim_state->m_vel = approach_vel( vecAbsVelocity , anim_state->m_vel , anim_state->m_last_clientside_anim_update_time_delta * 2000.0f );
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
			set_sequence( anim_state, 3 , 5 /* SelectSequenceFromActMods( ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING ) */ );
			*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x138 ) = true;
		}

		if ( get_layer_activity( anim_state , 3 ) == 980 || get_layer_activity( anim_state , 3 ) == 979 ) {
			if ( *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x138 ) && anim_state->m_unk_feet_speed_ratio <= 0.25f ) {
				increment_layer_cycle_weight_rate_generic( anim_state, 3 );
				*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x138 ) = !is_sequence_completed( anim_state , 3 );
			}
			else {
				*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x138 ) = false;
				float flWeight = player->layers()[3].m_weight;
				set_weight( anim_state, 3 , valve_math::Approach( 0.0f , flWeight , anim_state->m_last_clientside_anim_update_time_delta * 5.0f ) );
				set_weight_rate( anim_state, 3 , flWeight );
			}
		}
	}

	// if the player is looking far enough to either side, turn the feet to keep them within the extent of the aim matrix
	anim_state->m_feet_yaw = anim_state->m_abs_yaw;
	anim_state->m_abs_yaw = std::clamp( anim_state->m_abs_yaw , -360.0f , 360.0f );
	float flEyeFootDelta = valve_math::AngleDiff( anim_state->m_eye_yaw , anim_state->m_abs_yaw );

	// narrow the available aim matrix width as speed increases
	float flAimMatrixWidthRange = std::lerp( 1.0f , std::lerp( anim_state->m_ground_fraction , 0.8f , 0.5f ), std::clamp( anim_state->m_run_speed , 0.0f , 1.0f ) );

	if ( anim_state->m_duck_amount > 0.0f )
		flAimMatrixWidthRange = std::lerp( flAimMatrixWidthRange , 0.5f , anim_state->m_duck_amount * std::clamp( anim_state->m_unk_feet_speed_ratio , 0.0f , 1.0f ) );

	float flTempYawMax = anim_state->m_max_yaw * flAimMatrixWidthRange;
	float flTempYawMin = anim_state->m_min_yaw * flAimMatrixWidthRange;

	if ( flEyeFootDelta > flTempYawMax )
		anim_state->m_abs_yaw = anim_state->m_eye_yaw - abs( flTempYawMax );
	else if ( flEyeFootDelta < flTempYawMin )
		anim_state->m_abs_yaw = anim_state->m_eye_yaw + abs( flTempYawMin );

	anim_state->m_abs_yaw = valve_math::AngleNormalize( anim_state->m_abs_yaw );

	// pull the lower body direction towards the eye direction, but only when the player is moving
	if ( anim_state->m_on_ground ) {
		if ( anim_state->m_speed2d > 0.1f ) {
			anim_state->m_abs_yaw = valve_math::ApproachAngle( anim_state->m_eye_yaw , anim_state->m_abs_yaw , anim_state->m_last_clientside_anim_update_time_delta * ( 30.0f + 20.0f * anim_state->m_ground_fraction ) );

			if ( !CLIENT_DLL_ANIMS ) {
				*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x10C ) = cs::i::globals->m_curtime + 0.22f;
				player->lby( ) = anim_state->m_eye_yaw;
			}
		}
		else {
			anim_state->m_abs_yaw = valve_math::ApproachAngle( player->lby( ) , anim_state->m_abs_yaw , anim_state->m_last_clientside_anim_update_time_delta * 100.0f );

			if ( !CLIENT_DLL_ANIMS ) {
				if ( cs::i::globals->m_curtime > *reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x10C ) && abs( valve_math::AngleDiff( anim_state->m_abs_yaw , anim_state->m_eye_yaw ) ) > 35.0f ) {
					*reinterpret_cast< float* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x10C ) = cs::i::globals->m_curtime + 1.1f;
					player->lby( ) = anim_state->m_eye_yaw;
				}
			}
		}
	}

	if ( !CLIENT_DLL_ANIMS ) {
		if ( anim_state->m_speed2d <= 1.0f
			&& anim_state->m_on_ground
			&& !*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t > ( anim_state ) + 0x128 )
			&& !anim_state->m_hit_ground
			&& anim_state->m_last_clientside_anim_update_time_delta > 0.0f
			&& abs( valve_math::AngleDiff( anim_state->m_feet_yaw , anim_state->m_abs_yaw ) / anim_state->m_last_clientside_anim_update_time_delta > 120.0f ) ) {
			set_sequence( anim_state , 3 , 4 /* SelectSequenceFromActMods( ACT_CSGO_IDLE_TURN_BALANCEADJUST ) */ );
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
		anim_state->m_ground_fraction = std::max ( 0.99f, anim_state->m_ground_fraction );
	}
	
	player->poses ( ) [ pose_param_t::move_blend_walk ] = ( 1.0f - anim_state->m_ground_fraction ) * ( 1.0f - anim_state->m_duck_amount );
	player->poses ( ) [ pose_param_t::move_blend_run ] = anim_state->m_ground_fraction * ( 1.0f - anim_state->m_duck_amount );
	player->poses ( ) [ pose_param_t::move_blend_crouch ] = anim_state->m_duck_amount;

	char szWeaponMoveSeq [ MAX_ANIMSTATE_ANIMNAME_CHARS ];
	V_sprintf_safe ( szWeaponMoveSeq, _("move_%s"), GetWeaponPrefix ( ) );

	int nWeaponMoveSeq = m_pPlayer->LookupSequence ( szWeaponMoveSeq );

	if ( nWeaponMoveSeq == -1 )
		nWeaponMoveSeq = m_pPlayer->LookupSequence ( _("move") );

	if ( m_pPlayer->m_iMoveState != m_nPreviousMoveState )
	{
		m_flStutterStep += 10;
	}
	m_nPreviousMoveState = m_pPlayer->m_iMoveState;
	m_flStutterStep = std::clamp ( Approach ( 0, m_flStutterStep, m_flLastUpdateIncrement * 40.0f ), 0.0f, 100.0f );
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
				set_sequence( anim_state, 9 , 224 /*SelectSequenceFromActMods( ACT_CSGO_FLASHBANG_REACTION )*/ );
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
	
			set_cycle( anim_state , 9 , 0 );
			set_rate( anim_state , 9 , 0 );

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
	static auto CCSGOPlayerAnimState__SetUpLean = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 A1 ? ? ? ? 83 EC 20 F3" ) ).get<void( __thiscall* )( animstate_t* )>( );
	CCSGOPlayerAnimState__SetUpLean( anim_state );
}

void anims::rebuilt::update( animstate_t* anim_state , vec3_t angles , vec3_t origin ) {
	static auto m_flThirdpersonRecoil = pattern::search( _( "client.dll" ) , _( "F3 0F 10 86 ? ? ? ? F3 0F 58 44 24 0C" ) ).add(4).deref().get< uint32_t >( );

	static auto CCSGOPlayerAnimState__CacheSequences = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 34 53 56 8B F1 57 8B" ) ).get< bool( __thiscall* )( animstate_t* ) >( );
	static auto& s_bEnableInvalidateBoneCache = *pattern::search( _( "client.dll" ) , _( "C6 05 ? ? ? ? 00 F3 0F 5F 05 ? ? ? ? F3 0F 11 47 74" ) ).add(2).deref().get< bool* >( );
	static auto IsPreCrouchUpdateDemo = pattern::search( _( "client.dll" ) , _( "8B 0D 94 01 2C 15 8B 01 8B 80 ? ? ? ? FF D0 84 C0 75 14" ) ).get< bool( * )( ) >( );

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

		vfunc<void( __thiscall* )( player_t*, int )>( player, 218 )( player, 0 ); // m_pPlayer->SetSequence( 0 ); might become outdated
		*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( player ) + 0xA18 ) = 0.0f;
		
		if ( *reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( player ) + 0xA14 ) != 0.0f ) {
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( player ) + 0xA14 ) = 0.0f;
			invalidate_physics_recursive( anim_state , 8 );
		}
	}

	// all the layers get set up here
	setup_velocity( anim_state );			// calculate speed and set up body yaw values
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
				set_weight( anim_state, 1, 0.0f );
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