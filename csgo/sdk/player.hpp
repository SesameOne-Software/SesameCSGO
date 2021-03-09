#pragma once
#include <array>
#include "weapon.hpp"
#include "matrix3x4.hpp"
#include "mdl_info.hpp"
#include "../utils/vfunc.hpp"
#include "entity.hpp"
#include "client.hpp"

enum class flags_t : uint32_t {
	on_ground = ( 1 << 0 ),
	ducking = ( 1 << 1 ),
	water_jump = ( 1 << 2 ),
	on_train = ( 1 << 3 ),
	in_rain = ( 1 << 4 ),
	frozen = ( 1 << 5 ),
	at_controls = ( 1 << 6 ),
	client = ( 1 << 7 ),
	fake_client = ( 1 << 8 )
};

ENUM_BITMASK ( flags_t );

enum class hitbox_t : uint32_t {
	head = 0,
	neck,
	pelvis,
	stomach,
	thorax,
	l_chest,
	u_chest,
	r_thigh,
	l_thigh,
	r_calf,
	l_calf,
	r_foot,
	l_foot,
	r_hand,
	l_hand,
	r_upperarm,
	r_forearm,
	l_upperarm,
	l_forearm,
	max_hitbox,
};

enum class movetypes_t : uint8_t {
	none = 0,   
	isometric,  
	walk,       
	step,       
	fly,        
	flygravity, 
	vphysics,   
	push,       
	noclip,     
	ladder,     
	observer,   
	custom      
};

// Generated using ReClass 2016

/*
*	Reversed by oxy on 10/25/2019 for project o2
*	Do not remove.
*	Do not leak.
*/

class player_t;
class weapon_t;
class animstate_t;
class vec3_t;
class vec2_t;

struct animstate_pose_param_cache_t {
	PAD ( 4);
	uint32_t m_idx; //0x4
	char* m_name; //0x8

	void set_value( player_t* e, float val );
};

class animstate_t {
public:
	PAD ( 4 );
	bool m_force_update; //0x0005
	PAD ( 90 );
	player_t* m_entity; //0x0060
	weapon_t* m_weapon; //0x0064
	weapon_t* m_last_weapon; //0x0068
	float m_last_clientside_anim_update; //0x006C
	uint32_t m_last_clientside_anim_framecount; //0x0070
	float m_last_clientside_anim_update_time_delta; //0x0074
	float m_eye_yaw; //0x0078
	float m_pitch; //0x007C
	float m_abs_yaw; //0x0080
	float m_feet_yaw; //0x0084
	float m_body_yaw; //0x0088
	float m_body_yaw_clamped; //0x008C
	float m_feet_vel_dir_delta; //0x0090
	PAD ( 4 );
	float m_feet_cycle; //0x0098
	float m_feet_yaw_rate; //0x009C
	PAD ( 4 );
	float m_duck_amount; //0x00A4
	float m_landing_duck_additive; //0x00A8
	PAD ( 4 );
	vec3_t m_origin; //0x00B0
	vec3_t m_old_origin; //0x00BC
	vec2_t m_vel2d; //0x00C8
	uint8_t pad_0x00D0 [ 0x10 ]; //0x00D0
	vec2_t m_last_accelerating_vel; //0x00E0
	uint8_t pad_0x00E8 [ 0x4 ]; //0x00E8
	float m_speed2d; //0x00EC
	float m_up_vel; //0x00F0
	float m_speed_normalized; //0x00F4
	float m_run_speed; //0x00F8
	float m_unk_feet_speed_ratio; //0x00FC
	float m_time_since_move; //0x0100
	float m_time_since_stop; //0x0104
	bool m_on_ground; //0x0108
	bool m_hit_ground; //0x0109
	PAD ( 4 );
	float m_time_in_air; //0x0110
	PAD ( 6 );
	float m_ground_fraction; //0x011C
	PAD ( 2 );
	float m_unk_fraction; //0x0124
	PAD ( 12 );
	bool m_moving; //0x0134
	PAD ( 123 );
	animstate_pose_param_cache_t m_lean_yaw_pose; //0x1B0
	animstate_pose_param_cache_t m_speed_pose; //0x01BC
	animstate_pose_param_cache_t m_ladder_speed_pose; //0x01C8
	animstate_pose_param_cache_t m_ladder_yaw_pose; //0x01D4
	animstate_pose_param_cache_t m_move_yaw_pose; //0x01E0
	animstate_pose_param_cache_t m_run_pose; //0x01EC
	animstate_pose_param_cache_t m_body_yaw_pose; //0x01F8
	animstate_pose_param_cache_t m_body_pitch_pose; //0x0204
	animstate_pose_param_cache_t m_dead_yaw_pose; //0x0210
	animstate_pose_param_cache_t m_stand_pose; //0x021C
	animstate_pose_param_cache_t m_jump_fall_pose; //0x0228
	animstate_pose_param_cache_t m_aim_blend_stand_idle_pose; //0x0234
	animstate_pose_param_cache_t m_aim_blend_crouch_idle_pose; //0x0240
	animstate_pose_param_cache_t m_strafe_yaw_pose; //0x024C
	animstate_pose_param_cache_t m_aim_blend_stand_walk_pose; //0x0258
	animstate_pose_param_cache_t m_aim_blend_stand_run_pose; //0x0264
	animstate_pose_param_cache_t m_aim_blend_crouch_walk_pose; //0x0270
	animstate_pose_param_cache_t m_move_blend_walk_pose; //0x027C
	animstate_pose_param_cache_t m_move_blend_run_pose; //0x0288
	animstate_pose_param_cache_t m_move_blend_crouch_pose; //0x0294
	PAD ( 4 );
	float m_vel_unk; //0x02A4
	PAD ( 134 );
	float m_min_yaw; //0x0330
	float m_max_yaw; //0x0334
	float m_max_pitch; //0x0338
	float m_min_pitch; //0x033C

	const char* get_weapon_move_animation ( );
	void reset( );
	void update( vec3_t& ang );
}; //Size=0x0340

struct animlayer_t {
	PAD( 20 );
	int	m_order;
	int	m_sequence;
	float m_previous_cycle;
	float m_weight;
	float m_weight_delta_rate;
	float m_playback_rate;
	float m_cycle;
	void* m_owner;
	PAD( 4 );
};

struct anim_list_record_t {
	player_t* m_ent;
	std::uint32_t m_flags;
};

struct anim_list_t {
	anim_list_record_t* m_data;
};

class player_t : public entity_t {
public:
	NETVAR( uint32_t, ground_entity_handle, "DT_BasePlayer->m_hGroundEntity" );
	NETVAR( flags_t, flags, "DT_BasePlayer->m_fFlags" );
	NETVAR( bool, has_defuser, "DT_CSPlayer->m_bHasDefuser" );
	NETVAR( bool, immune, "DT_CSPlayer->m_bGunGameImmunity" );
	NETVAR( vec3_t, angles, "DT_CSPlayer->m_angEyeAngles[0]" );
	NETVAR( bool, has_heavy_armor, "DT_CSPlayer->m_bHasHeavyArmor" );
	NETVAR( int, health, "DT_BasePlayer->m_iHealth" );
	NETVAR( int, armor, "DT_CSPlayer->m_ArmorValue" );
	NETVAR( bool, has_helmet, "DT_CSPlayer->m_bHasHelmet" );
	NETVAR( bool, scoped, "DT_CSPlayer->m_bIsScoped" );
	NETVAR( float, lby, "DT_CSPlayer->m_flLowerBodyYawTarget" );
	NETVAR( float, flash_duration, "DT_CSPlayer->m_flFlashDuration" );
	NETVAR( float, flash_alpha, "DT_CSPlayer->m_flFlashMaxAlpha" );
	NETVAR( uint8_t, life_state, "DT_BasePlayer->m_lifeState" );
	NETVAR( uint32_t, tick_base, "DT_BasePlayer->m_nTickBase" );
	NETVAR( float, crouch_amount, "DT_BasePlayer->m_flDuckAmount" );
	NETVAR( float, crouch_speed, "DT_BasePlayer->m_flDuckSpeed" );
	NETVAR( vec3_t, view_punch, "DT_BasePlayer->m_viewPunchAngle" );
	NETVAR( vec3_t, aim_punch, "DT_BasePlayer->m_aimPunchAngle" );
	NETVAR( vec3_t , aim_punch_vel , "DT_BasePlayer->m_aimPunchAngleVel" );
	NETVAR( vec3_t, vel, "DT_BasePlayer->m_vecVelocity[0]" );
	NETVAR( bool, animate, "DT_BaseAnimating->m_bClientSideAnimation" );
	NETVAR( uint32_t, weapon_handle, "DT_BaseCombatCharacter->m_hActiveWeapon" );
	NETVAR ( uint32_t*, weapons_handle, "DT_BaseCombatCharacter->m_hMyWeapons[0]" );
	NETVAR( vec3_t, view_offset, "DT_BasePlayer->m_vecViewOffset[0]" );
	NETVAR( vec3_t , base_vel , "DT_BasePlayer->m_vecBaseVelocity" );
	NETVAR( float , fall_vel , "DT_BasePlayer->m_flFallVelocity" );
	NETVAR( float, simtime, "DT_BaseEntity->m_flSimulationTime" );
	NETVAR_ADDITIVE( movetypes_t, movetype, "DT_BaseEntity->m_nRenderMode", 1 );
	NETVAR( vec3_t, mins, "DT_CSPlayer->m_vecMins" );
	NETVAR( vec3_t, maxs, "DT_CSPlayer->m_vecMaxs" );
	NETVAR( float , velocity_modifier , "DT_CSPlayer->m_flVelocityModifier" );
	NETVAR( uint32_t, observer_mode, "DT_CSPlayer->m_iObserverMode" );
	NETVAR( uint32_t, ragdoll_handle, "DT_CSPlayer->m_hRagdoll" );
	NETVAR( uint32_t, viewmodel_handle, "DT_BasePlayer->m_hViewModel[0]" );
	NETVAR( vec3_t, force, "DT_CSRagdoll->m_vecForce" );
	NETVAR( vec3_t, ragdoll_vel, "DT_CSRagdoll->m_vecRagdollVelocity" );
	NETVAR( float, next_attack, "DT_CSPlayer->m_flNextAttack" );
	NETVAR ( uint32_t*, wearables_handle, "DT_BaseCombatCharacter->m_hMyWearables" );
	OFFSET( int, effects, 0xE8 );
	OFFSET( int, eflags, 0xF0 );
	OFFSET( void*, iks, 0x266C );
	OFFSET( bool, should_update, 0x289C );
	OFFSET( uint32_t, num_overlays, 0x298C );
	OFFSET( float, spawn_time, 0xA370 );
	OFFSET( matrix3x4a_t*, bones, 0x26A4 + 0x4 );
	OFFSET( int, readable_bones, 0x26A8 + 0x4 );
	OFFSET( int, writeable_bones, 0x26AC + 0x4 );
	NETVAR ( int, body, "DT_CSPlayer->m_nBody" );

	bool is_player( ) {
		return client_class ( ) && client_class ( )->m_class_id == 40;
	}

	animlayer_t* layers( ) {
		return *reinterpret_cast< animlayer_t** >( reinterpret_cast<uintptr_t>(this) + 0x2980 );
	}

	std::array< float, 24 >& poses( ) {
		return *reinterpret_cast< std::array< float, 24 >* >( reinterpret_cast< uintptr_t >( this ) + 0x2774 );
	}

	void* seq_desc( int seq ) {
		auto group_hdr = *reinterpret_cast< void** >( std::uintptr_t( this ) + 0xA53 );
		auto i = seq;

		if ( seq < 0 || seq >= *reinterpret_cast< std::uint32_t* >( std::uintptr_t( group_hdr ) + 0xBC ) )
			i = 0;

		return reinterpret_cast< void* >( std::uintptr_t( group_hdr ) + *reinterpret_cast< std::uintptr_t* >( std::uintptr_t( group_hdr ) + 0xC0 ) + 0xD4 * i );
	}

	void set_local_viewangles( const vec3_t& ang ) {
		using fn = void( __thiscall* )( void*, const vec3_t& );
		vfunc< fn >( this, 371 )( this, ang );
	}

	bool physics_run_think( int unk01 );

	void think( ) {
		vfunc< void( __thiscall* )( void* ) >( this, 138 )( this );
	}

	void pre_think( ) {
		vfunc< void( __thiscall* )( void* ) >( this, 317 )( this );
	}

	void post_think( ) {
		vfunc< void( __thiscall* )( void* ) >( this, 318 )( this );
	}

	vec3_t world_space( ) {
		vec3_t wspace;
		vec3_t va, vb;

		using fn = void( __thiscall* )( void*, vec3_t&, vec3_t& );
		vfunc< fn >( this + 0x4, 17 )( this + 0x4, va, vb );

		wspace = origin( );
		wspace.z += ( va.z + vb.z ) * 0.5f;

		return wspace;
	}

	float& old_simtime( ) {
		return *( float* )( std::uintptr_t( &simtime( ) ) + 4 );
	}

	animstate_t* animstate( );

	void* mdl( void ) {
		void* r = this->renderable( );

		if ( !r )
			return nullptr;

		using getmdl_fn = void* ( __thiscall* )( void* );
		return vfunc< getmdl_fn >( r, 8 )( r );
	}

	studiohdr_t* studio_mdl( void* mdl ) {
		using getstudiomdl_fn = studiohdr_t * ( __thiscall* )( void*, void* );
		return vfunc< getstudiomdl_fn >( this, 32 )( this, mdl );
	}

	bool alive( ) {
		return !!health( );
	}

	bool valid( ) {
		return this && alive( ) && !dormant( ) && !life_state( );
	}

	vec3_t& abs_origin( ) {
		using fn = vec3_t & ( __thiscall* )( void* );
		return vfunc< fn >( this, 10 )( this );
	}

	vec3_t& abs_angles( ) {
		using fn = vec3_t & ( __thiscall* )( void* );
		return vfunc< fn >( this, 11 )( this );
	}

	std::uint32_t handle( );

	bool setup_bones( matrix3x4_t* m, std::uint32_t max, std::uint32_t mask, float seed );

	void estimate_abs_vel( vec3_t& vec ) {
		using fn = void( __thiscall* )( void*, vec3_t& );
		vfunc< fn >( this, 144 )( this, vec );
	}

	vec3_t& const render_origin( ) {
		using fn = vec3_t& const( __thiscall* )( void* );
		return vfunc< fn >( renderable( ), 1 )( renderable( ) );
	}

	void create_animstate( animstate_t* state );
	void inval_bone_cache( );
	void set_abs_angles( const vec3_t& ang );
	void update( );
	vec3_t& abs_vel( );
	void set_abs_vel( vec3_t& vel );
	void set_abs_origin( const vec3_t& vec );

	vec3_t eyes( );
	std::uint32_t& bone_count( );
	matrix3x4_t*& bone_cache( );
	weapon_t* weapon ( );
	std::vector<weapon_t*> weapons ( );
	std::vector<weapon_t*> wearables ( );

	float desync_amount( ) {
		auto state = animstate( );

		if ( !state )
			return 0.0f;

		auto running_speed = std::clamp( state->m_run_speed, 0.0f, 1.0f );
		auto yaw_modifier = ( ( ( state->m_ground_fraction * -0.3f ) - 0.2f ) * running_speed ) + 1.0f;

		if ( state->m_duck_amount > 0.0f ) {
			auto speed_factor = std::clamp( state->m_unk_feet_speed_ratio, 0.0f, 1.0f );
			yaw_modifier += ( ( state->m_duck_amount * speed_factor ) * ( 0.5f - yaw_modifier ) );
		}

		return yaw_modifier * state->m_max_yaw;
	}

	void get_sequence_linear_motion ( void* studio_hdr, int sequence, float* poses, vec3_t* vec );
	float get_sequence_move_distance ( void* studio_hdr, int sequence );
	int lookup_sequence ( const char* seq );
	float sequence_duration ( int sequence );
	float get_sequence_cycle_rate_server ( int sequence );
};