#pragma once
#include <array>
#include "weapon.hpp"
#include "matrix3x4.hpp"
#include "mdl_info.hpp"
#include "../utils/vfunc.hpp"

enum class movetypes {
	movetype_none = 0,
	movetype_isometric,
	movetype_walk,
	movetype_step,
	movetype_fly,
	movetype_flygravity,
	movetype_vphysics,
	movetype_push,
	movetype_noclip,
	movetype_ladder,
	movetype_observer,
	movetype_custom,
	movetype_last = movetype_custom,
	movetype_max_bits = 4
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
	std::uint8_t pad_0x0 [ 0x4 ]; //0x0
	std::uint32_t m_idx; //0x4
	char* m_name; //0x8

	void set_value( player_t* e, float val );
};

class animstate_t {
public:
	std::uint8_t pad_0x0000 [ 0x4 ]; //0x0000
	bool m_force_update; //0x0005
	std::uint8_t pad_0x0006 [ 0x5A ]; //0x0006
	player_t* m_entity; //0x0060
	weapon_t* m_weapon; //0x0064
	weapon_t* m_last_weapon; //0x0068
	float m_last_clientside_anim_update; //0x006C
	std::uint32_t m_last_clientside_anim_framecount; //0x0070
	float m_last_clientside_anim_update_time_delta; //0x0074
	float m_eye_yaw; //0x0078
	float m_pitch; //0x007C
	float m_abs_yaw; //0x0080
	float m_feet_yaw; //0x0084
	float m_body_yaw; //0x0088
	float m_body_yaw_clamped; //0x008C
	float m_feet_vel_dir_delta; //0x0090
	std::uint8_t pad_0x0094 [ 0x4 ]; //0x0094
	float m_feet_cycle; //0x0098
	float m_feet_yaw_rate; //0x009C
	std::uint8_t pad_0x00A0 [ 0x4 ]; //0x00A0
	float m_duck_amount; //0x00A4
	float m_landing_duck_additive; //0x00A8
	std::uint8_t pad_0x00AC [ 0x4 ]; //0x00AC
	vec3_t m_origin; //0x00B0
	vec3_t m_old_origin; //0x00BC
	vec2_t m_vel2d; //0x00C8
	std::uint8_t pad_0x00D0 [ 0x10 ]; //0x00D0
	vec2_t m_last_accelerating_vel; //0x00E0
	std::uint8_t pad_0x00E8 [ 0x4 ]; //0x00E8
	float m_speed2d; //0x00EC
	float m_up_vel; //0x00F0
	float m_speed_normalized; //0x00F4
	float m_run_speed; //0x00F8
	float m_unk_feet_speed_ratio; //0x00FC
	float m_time_since_move; //0x0100
	float m_time_since_stop; //0x0104
	bool m_on_ground; //0x0108
	bool m_hit_ground; //0x0109
	std::uint8_t pad_0x010A [ 0x4 ]; //0x010A
	float m_time_in_air; //0x0110
	std::uint8_t pad_0x0114 [ 0x6 ]; //0x0114
	float m_ground_fraction; //0x011C
	std::uint8_t pad_0x0120 [ 0x2 ]; //0x0120
	float m_unk_fraction; //0x0124
	std::uint8_t pad_0x0128 [ 0xC ]; //0x0128
	bool m_moving; //0x0134
	std::uint8_t pad_0x0135 [ 0x7B ]; //0x0135
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
	std::uint8_t pad_0x02A0 [ 0x4 ]; //0x02A0
	float m_vel_unk; //0x02A4
	std::uint8_t pad_0x02A8 [ 0x86 ]; //0x02A8
	float m_min_yaw; //0x0330
	float m_max_yaw; //0x0334
	float m_max_pitch; //0x0338
	float m_min_pitch; //0x033C

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
	NETVAR ( uint32_t, ground_entity_handle, "DT_BasePlayer->m_hGroundEntity" );
	NETVAR( std::uint32_t, flags, "DT_BasePlayer->m_fFlags" );
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
	NETVAR( std::uint8_t, life_state, "DT_BasePlayer->m_lifeState" );
	NETVAR( std::uint32_t, tick_base, "DT_BasePlayer->m_nTickBase" );
	NETVAR ( float, crouch_amount, "DT_BasePlayer->m_flDuckAmount" );
	NETVAR( float, crouch_speed, "DT_BasePlayer->m_flDuckSpeed" );
	NETVAR( vec3_t, view_punch, "DT_BasePlayer->m_viewPunchAngle" );
	NETVAR( vec3_t, aim_punch, "DT_BasePlayer->m_aimPunchAngle" );
	NETVAR( vec3_t, vel, "DT_BasePlayer->m_vecVelocity[0]" );
	NETVAR( bool, animate, "DT_BaseAnimating->m_bClientSideAnimation" );
	NETVAR( std::uint32_t, weapon_handle, "DT_BaseCombatCharacter->m_hActiveWeapon" );
	NETVAR( vec3_t, view_offset, "DT_BasePlayer->m_vecViewOffset[0]" );
	NETVAR( float, simtime, "DT_BaseEntity->m_flSimulationTime" );
	NETVAR_ADDITIVE( movetypes, movetype, "DT_BaseEntity->m_nRenderMode", 1 );
	NETVAR( vec3_t, mins, "DT_CSPlayer->m_vecMins" );
	NETVAR( vec3_t, maxs, "DT_CSPlayer->m_vecMaxs" );
	NETVAR( std::uint32_t, observer_mode, "DT_CSPlayer->m_iObserverMode" );
	NETVAR ( uint32_t, ragdoll_handle, "DT_CSPlayer->m_hRagdoll" );
	NETVAR( uint32_t, viewmodel_handle, "DT_BasePlayer->m_hViewModel[0]" );
	NETVAR ( vec3_t, force, "DT_CSRagdoll->m_vecForce" );
	NETVAR ( vec3_t, ragdoll_vel, "DT_CSRagdoll->m_vecRagdollVelocity" );
	OFFSET( int, effects, 0xE4 );
	OFFSET( int, eflags, 0xE8 );
	OFFSET( void*, iks, 0x266C );
	OFFSET( bool, should_update, 0x289C );
	OFFSET( std::uint32_t, num_overlays, 0x298C );
	OFFSET( float, spawn_time, 0xA360 );
	OFFSET ( matrix3x4a_t*, bones, 0x26A4 + 0x4 );
	OFFSET ( int, readable_bones, 0x26A8 + 0x4 );
	OFFSET ( int, writeable_bones, 0x26AC + 0x4 );

	bool is_player( ) {
		using fn = bool( __thiscall* )( void* );
		return vfunc< fn >( this, 155 )( this );
	}

	animlayer_t* layers( ) {
		return *reinterpret_cast< animlayer_t** >( std::uintptr_t( this ) + 0x2980 );
	}

	std::array< float, 24 >& poses( ) {
		return *reinterpret_cast< std::array< float, 24 >* >( std::uintptr_t( this ) + 0x2774 );
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
		vfunc< void( __thiscall* )( void* ) >( this, 318 )( this );
	}

	void post_think( ) {
		vfunc< void( __thiscall* )( void* ) >( this, 319 )( this );
	}

	vec3_t world_space( ) {
		vec3_t wspace;
		vec3_t va, vb;

		using fn = void( __thiscall* )( void*, vec3_t&, vec3_t& );
		vfunc< fn >( this + 0x4, 17 )( this + 0x4, va, vb );

		wspace = abs_origin( );
		wspace.z += ( va.z + vb.z ) * 0.5f;

		return wspace;
	}

	float& old_simtime( ) {
		return *( float* ) ( std::uintptr_t( &simtime( ) ) + 4 );
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

	bool setup_bones( std::array< matrix3x4_t, 128 >& m, std::uint32_t max, std::uint32_t mask, float seed ) {
		using setupbones_fn = bool( __thiscall* )( void*, std::array< matrix3x4_t, 128 >&, std::uint32_t, std::uint32_t, float );
		return vfunc< setupbones_fn >( renderable( ), 13 )( renderable( ), m, max, mask, seed );
	}

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
	void set_abs_origin( vec3_t& vec );

	vec3_t eyes( );
	std::uint32_t& bone_count( );
	matrix3x4_t* bone_cache( );
	weapon_t* weapon( );

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
};