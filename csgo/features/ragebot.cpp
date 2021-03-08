#include "ragebot.hpp"
#include <deque>
#include "autowall.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "prediction.hpp"
#include "../animations/resolver.hpp"
#include "../menu/options.hpp"
#include "../animations/anims.hpp"

#include "exploits.hpp"

#undef min
#undef max

player_t* last_target = nullptr;
int target_idx = 0;
std::array< features::ragebot::misses_t, 65 > misses { 0 };
std::array< vec3_t, 65 > target_pos { vec3_t( ) };
std::array< vec3_t, 65 > shot_pos { vec3_t( ) };
std::array< int, 65 > hits { 0 };
std::array< int, 65 > shots { 0 };
std::array< int, 65 > hitbox { 0 };
std::array< anims::anim_info_t, 65 > cur_lag_rec { {} };
features::ragebot::weapon_config_t features::ragebot::active_config;

features::ragebot::c_scan_points features::ragebot::scan_points;

#pragma optimize( "2", on )

void features::ragebot::get_weapon_config( weapon_config_t& const config ) {
	VM_TIGER_BLACK_START
		if ( !g::local || !g::local->alive ( ) || !g::local->weapon ( ) || !g::local->weapon ( )->data ( ) ) {
				return;
	}
		
	static auto& main_switch = options::vars [ _( "global.assistance_type" ) ].val.i;
	static auto& knife_bot = options::vars [ _( "ragebot.knife_bot" ) ].val.b;
	static auto& zeus_bot = options::vars [ _( "ragebot.zeus_bot" ) ].val.b;
	static auto& fix_fakelag = options::vars [ _( "ragebot.fix_fakelag" ) ].val.b;
	static auto& resolve_desync = options::vars [ _( "ragebot.resolve_desync" ) ].val.b;
	static auto& tickbase_key = options::vars [ _( "ragebot.dt_key" ) ].val.i;
	static auto& tickbase_key_mode = options::vars [ _( "ragebot.dt_key_mode" ) ].val.i;
	static auto& safe_point = options::vars [ _( "ragebot.safe_point" ) ].val.b;
	static auto& safe_point_key = options::vars [ _( "ragebot.safe_point_key" ) ].val.i;
	static auto& safe_point_key_mode = options::vars [ _( "ragebot.safe_point_key_mode" ) ].val.i;
	static auto& auto_revolver = options::vars [ _( "ragebot.auto_revolver" ) ].val.b;

	/* reset all config options */
	memset( &config, 0, sizeof config );

	config.main_switch = main_switch == 2;
	config.knife_bot = knife_bot;
	config.zeus_bot = zeus_bot;
	config.fix_fakelag = fix_fakelag;
	config.resolve_desync = resolve_desync;
	config.dt_key = tickbase_key;
	config.dt_key_mode = tickbase_key_mode;
	config.safe_point = safe_point;
	config.safe_point_key = safe_point_key;
	config.safe_point_key_mode = safe_point_key_mode;
	config.auto_revolver = auto_revolver;

	if ( g::local->weapon( )->item_definition_index( ) == weapons_t::revolver ) {
		static auto& inherit_default = options::vars [ _( "ragebot.revolver.inherit_default" ) ].val.b;

		if ( inherit_default )
			goto set_default;

		static auto& choke_onshot = options::vars [ _( "ragebot.revolver.choke_onshot" ) ].val.b;
		static auto& silent = options::vars [ _( "ragebot.revolver.silent" ) ].val.b;
		static auto& hitboxes = options::vars [ _( "ragebot.revolver.hitboxes" ) ].val.l;
		static auto& baim_lethal = options::vars [ _( "ragebot.revolver.baim_if_lethal" ) ].val.b;
		static auto& baim_air = options::vars [ _( "ragebot.revolver.baim_in_air" ) ].val.b;
		static auto& min_dmg = options::vars [ _( "ragebot.revolver.min_dmg" ) ].val.f;
		static auto& dmg_accuracy = options::vars [ _( "ragebot.revolver.dmg_accuracy" ) ].val.f;
		static auto& head_ps = options::vars [ _( "ragebot.revolver.head_pointscale" ) ].val.f;
		static auto& body_ps = options::vars [ _( "ragebot.revolver.body_pointscale" ) ].val.f;
		static auto& baim_after_misses = options::vars [ _( "ragebot.revolver.force_baim" ) ].val.i;
		static auto& tickbase_shift_amount = options::vars [ _( "ragebot.revolver.dt_ticks" ) ].val.i;
		static auto& auto_shoot = options::vars [ _( "ragebot.revolver.auto_shoot" ) ].val.b;
		static auto& auto_slow = options::vars [ _( "ragebot.revolver.auto_slow" ) ].val.b;
		static auto& auto_scope = options::vars [ _( "ragebot.revolver.auto_scope" ) ].val.b;
		static auto& dt_teleport = options::vars [ _( "ragebot.revolver.dt_teleport" ) ].val.b;
		static auto& dt_enabled = options::vars [ _( "ragebot.revolver.dt_enabled" ) ].val.b;
		static auto& hit_chance = options::vars [ _( "ragebot.revolver.hit_chance" ) ].val.f;
		static auto& dt_hit_chance = options::vars [ _( "ragebot.revolver.dt_hit_chance" ) ].val.f;
		static auto& headshot_only = options::vars [ _( "ragebot.revolver.headshot_only" ) ].val.b;
		static auto& onshot_only = options::vars [ _ ( "ragebot.revolver.onshot_only" ) ].val.b;
		static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.revolver.dt_recharge_delay" ) ].val.i;

		config.dt_recharge_delay = dt_recharge_delay;
		config.dmg_accuracy = dmg_accuracy;
		config.choke_on_shot = choke_onshot;
		config.silent = silent;
		config.headshot_only = headshot_only;
		config.scan_head = hitboxes [ 0 ];
		config.scan_neck = hitboxes [ 1 ];
		config.scan_chest = hitboxes [ 2 ];
		config.scan_pelvis = hitboxes [ 3 ];
		config.scan_arms = hitboxes [ 4 ];
		config.scan_legs = hitboxes [ 5 ];
		config.scan_feet = hitboxes [ 6 ];
		config.baim_lethal = baim_lethal;
		config.baim_air = baim_air;
		config.min_dmg = min_dmg;
		config.head_pointscale = head_ps;
		config.body_pointscale = body_ps;
		config.baim_after_misses = baim_after_misses;
		config.max_dt_ticks = tickbase_shift_amount;
		config.auto_scope = auto_scope;
		config.auto_slow = auto_slow;
		config.auto_shoot = auto_shoot;
		config.dt_teleport = dt_teleport;
		config.dt_enabled = dt_enabled;
		config.hit_chance = hit_chance;
		config.dt_hit_chance = dt_hit_chance;
		config.onshot_only = onshot_only;

		return;
	}
	else if ( g::local->weapon( )->data( )->m_type == 1 ) {
		static auto& inherit_default = options::vars [ _( "ragebot.pistol.inherit_default" ) ].val.b;

		if ( inherit_default )
			goto set_default;

		static auto& choke_onshot = options::vars [ _( "ragebot.pistol.choke_onshot" ) ].val.b;
		static auto& silent = options::vars [ _( "ragebot.pistol.silent" ) ].val.b;
		static auto& hitboxes = options::vars [ _( "ragebot.pistol.hitboxes" ) ].val.l;
		static auto& baim_lethal = options::vars [ _( "ragebot.pistol.baim_if_lethal" ) ].val.b;
		static auto& baim_air = options::vars [ _( "ragebot.pistol.baim_in_air" ) ].val.b;
		static auto& dmg_accuracy = options::vars [ _( "ragebot.pistol.dmg_accuracy" ) ].val.f;
		static auto& min_dmg = options::vars [ _( "ragebot.pistol.min_dmg" ) ].val.f;
		static auto& head_ps = options::vars [ _( "ragebot.pistol.head_pointscale" ) ].val.f;
		static auto& body_ps = options::vars [ _( "ragebot.pistol.body_pointscale" ) ].val.f;
		static auto& baim_after_misses = options::vars [ _( "ragebot.pistol.force_baim" ) ].val.i;
		static auto& tickbase_shift_amount = options::vars [ _( "ragebot.pistol.dt_ticks" ) ].val.i;
		static auto& auto_shoot = options::vars [ _( "ragebot.pistol.auto_shoot" ) ].val.b;
		static auto& auto_slow = options::vars [ _( "ragebot.pistol.auto_slow" ) ].val.b;
		static auto& auto_scope = options::vars [ _( "ragebot.pistol.auto_scope" ) ].val.b;
		static auto& dt_teleport = options::vars [ _( "ragebot.pistol.dt_teleport" ) ].val.b;
		static auto& dt_enabled = options::vars [ _( "ragebot.pistol.dt_enabled" ) ].val.b;
		static auto& hit_chance = options::vars [ _( "ragebot.pistol.hit_chance" ) ].val.f;
		static auto& dt_hit_chance = options::vars [ _( "ragebot.pistol.dt_hit_chance" ) ].val.f;
		static auto& headshot_only = options::vars [ _( "ragebot.pistol.headshot_only" ) ].val.b;
		static auto& onshot_only = options::vars [ _ ( "ragebot.pistol.onshot_only" ) ].val.b;
		static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.pistol.dt_recharge_delay" ) ].val.i;

		config.dt_recharge_delay = dt_recharge_delay;
		config.dmg_accuracy = dmg_accuracy;
		config.choke_on_shot = choke_onshot;
		config.silent = silent;
		config.headshot_only = headshot_only;
		config.scan_head = hitboxes [ 0 ];
		config.scan_neck = hitboxes [ 1 ];
		config.scan_chest = hitboxes [ 2 ];
		config.scan_pelvis = hitboxes [ 3 ];
		config.scan_arms = hitboxes [ 4 ];
		config.scan_legs = hitboxes [ 5 ];
		config.scan_feet = hitboxes [ 6 ];
		config.baim_lethal = baim_lethal;
		config.baim_air = baim_air;
		config.min_dmg = min_dmg;
		config.head_pointscale = head_ps;
		config.body_pointscale = body_ps;
		config.baim_after_misses = baim_after_misses;
		config.max_dt_ticks = tickbase_shift_amount;
		config.auto_scope = auto_scope;
		config.auto_slow = auto_slow;
		config.auto_shoot = auto_shoot;
		config.dt_teleport = dt_teleport;
		config.dt_enabled = dt_enabled;
		config.hit_chance = hit_chance;
		config.dt_hit_chance = dt_hit_chance;
		config.onshot_only = onshot_only;

		return;
	}
	else if ( g::local->weapon( )->data( )->m_type == 2 || g::local->weapon( )->data( )->m_type == 3 || g::local->weapon( )->data( )->m_type == 4 ) {
		static auto& inherit_default = options::vars [ _( "ragebot.rifle.inherit_default" ) ].val.b;

		if ( inherit_default )
			goto set_default;

		static auto& choke_onshot = options::vars [ _( "ragebot.rifle.choke_onshot" ) ].val.b;
		static auto& silent = options::vars [ _( "ragebot.rifle.silent" ) ].val.b;
		static auto& hitboxes = options::vars [ _( "ragebot.rifle.hitboxes" ) ].val.l;
		static auto& baim_lethal = options::vars [ _( "ragebot.rifle.baim_if_lethal" ) ].val.b;
		static auto& baim_air = options::vars [ _( "ragebot.rifle.baim_in_air" ) ].val.b;
		static auto& min_dmg = options::vars [ _( "ragebot.rifle.min_dmg" ) ].val.f;
		static auto& dmg_accuracy = options::vars [ _( "ragebot.rifle.dmg_accuracy" ) ].val.f;
		static auto& head_ps = options::vars [ _( "ragebot.rifle.head_pointscale" ) ].val.f;
		static auto& body_ps = options::vars [ _( "ragebot.rifle.body_pointscale" ) ].val.f;
		static auto& baim_after_misses = options::vars [ _( "ragebot.rifle.force_baim" ) ].val.i;
		static auto& tickbase_shift_amount = options::vars [ _( "ragebot.rifle.dt_ticks" ) ].val.i;
		static auto& auto_shoot = options::vars [ _( "ragebot.rifle.auto_shoot" ) ].val.b;
		static auto& auto_slow = options::vars [ _( "ragebot.rifle.auto_slow" ) ].val.b;
		static auto& auto_scope = options::vars [ _( "ragebot.rifle.auto_scope" ) ].val.b;
		static auto& dt_teleport = options::vars [ _( "ragebot.rifle.dt_teleport" ) ].val.b;
		static auto& dt_enabled = options::vars [ _( "ragebot.rifle.dt_enabled" ) ].val.b;
		static auto& hit_chance = options::vars [ _( "ragebot.rifle.hit_chance" ) ].val.f;
		static auto& dt_hit_chance = options::vars [ _( "ragebot.rifle.dt_hit_chance" ) ].val.f;
		static auto& headshot_only = options::vars [ _( "ragebot.rifle.headshot_only" ) ].val.b;
		static auto& onshot_only = options::vars [ _ ( "ragebot.rifle.onshot_only" ) ].val.b;
		static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.rifle.dt_recharge_delay" ) ].val.i;

		config.dt_recharge_delay = dt_recharge_delay;
		config.dmg_accuracy = dmg_accuracy;
		config.choke_on_shot = choke_onshot;
		config.silent = silent;
		config.headshot_only = headshot_only;
		config.scan_head = hitboxes [ 0 ];
		config.scan_neck = hitboxes [ 1 ];
		config.scan_chest = hitboxes [ 2 ];
		config.scan_pelvis = hitboxes [ 3 ];
		config.scan_arms = hitboxes [ 4 ];
		config.scan_legs = hitboxes [ 5 ];
		config.scan_feet = hitboxes [ 6 ];
		config.baim_lethal = baim_lethal;
		config.baim_air = baim_air;
		config.min_dmg = min_dmg;
		config.head_pointscale = head_ps;
		config.body_pointscale = body_ps;
		config.baim_after_misses = baim_after_misses;
		config.max_dt_ticks = tickbase_shift_amount;
		config.auto_scope = auto_scope;
		config.auto_slow = auto_slow;
		config.auto_shoot = auto_shoot;
		config.dt_teleport = dt_teleport;
		config.dt_enabled = dt_enabled;
		config.hit_chance = hit_chance;
		config.dt_hit_chance = dt_hit_chance;
		config.onshot_only = onshot_only;

		return;
	}
	else if ( g::local->weapon( )->data( )->m_type == weapon_type_t::sniper ) {
		if ( g::local->weapon( )->item_definition_index( ) == weapons_t::awp ) {
			static auto& inherit_default = options::vars [ _( "ragebot.awp.inherit_default" ) ].val.b;

			if ( inherit_default )
				goto set_default;

			static auto& choke_onshot = options::vars [ _( "ragebot.awp.choke_onshot" ) ].val.b;
			static auto& silent = options::vars [ _( "ragebot.awp.silent" ) ].val.b;
			static auto& hitboxes = options::vars [ _( "ragebot.awp.hitboxes" ) ].val.l;
			static auto& baim_lethal = options::vars [ _( "ragebot.awp.baim_if_lethal" ) ].val.b;
			static auto& baim_air = options::vars [ _( "ragebot.awp.baim_in_air" ) ].val.b;
			static auto& min_dmg = options::vars [ _( "ragebot.awp.min_dmg" ) ].val.f;
			static auto& dmg_accuracy = options::vars [ _( "ragebot.awp.dmg_accuracy" ) ].val.f;
			static auto& head_ps = options::vars [ _( "ragebot.awp.head_pointscale" ) ].val.f;
			static auto& body_ps = options::vars [ _( "ragebot.awp.body_pointscale" ) ].val.f;
			static auto& baim_after_misses = options::vars [ _( "ragebot.awp.force_baim" ) ].val.i;
			static auto& tickbase_shift_amount = options::vars [ _( "ragebot.awp.dt_ticks" ) ].val.i;
			static auto& auto_shoot = options::vars [ _( "ragebot.awp.auto_shoot" ) ].val.b;
			static auto& auto_slow = options::vars [ _( "ragebot.awp.auto_slow" ) ].val.b;
			static auto& auto_scope = options::vars [ _( "ragebot.awp.auto_scope" ) ].val.b;
			static auto& dt_teleport = options::vars [ _( "ragebot.awp.dt_teleport" ) ].val.b;
			static auto& dt_enabled = options::vars [ _( "ragebot.awp.dt_enabled" ) ].val.b;
			static auto& hit_chance = options::vars [ _( "ragebot.awp.hit_chance" ) ].val.f;
			static auto& dt_hit_chance = options::vars [ _( "ragebot.awp.dt_hit_chance" ) ].val.f;
			static auto& headshot_only = options::vars [ _( "ragebot.awp.headshot_only" ) ].val.b;
			static auto& onshot_only = options::vars [ _ ( "ragebot.awp.onshot_only" ) ].val.b;
			static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.awp.dt_recharge_delay" ) ].val.i;

			config.dt_recharge_delay = dt_recharge_delay;
			config.dmg_accuracy = dmg_accuracy;
			config.choke_on_shot = choke_onshot;
			config.silent = silent;
			config.headshot_only = headshot_only;
			config.scan_head = hitboxes [ 0 ];
			config.scan_neck = hitboxes [ 1 ];
			config.scan_chest = hitboxes [ 2 ];
			config.scan_pelvis = hitboxes [ 3 ];
			config.scan_arms = hitboxes [ 4 ];
			config.scan_legs = hitboxes [ 5 ];
			config.scan_feet = hitboxes [ 6 ];
			config.baim_lethal = baim_lethal;
			config.baim_air = baim_air;
			config.min_dmg = min_dmg;
			config.head_pointscale = head_ps;
			config.body_pointscale = body_ps;
			config.baim_after_misses = baim_after_misses;
			config.max_dt_ticks = tickbase_shift_amount;
			config.auto_scope = auto_scope;
			config.auto_slow = auto_slow;
			config.auto_shoot = auto_shoot;
			config.dt_teleport = dt_teleport;
			config.dt_enabled = dt_enabled;
			config.hit_chance = hit_chance;
			config.dt_hit_chance = dt_hit_chance;
			config.onshot_only = onshot_only;

			return;
		}
		else if ( g::local->weapon( )->item_definition_index( ) == weapons_t::scar20 || g::local->weapon( )->item_definition_index( ) == weapons_t::g3sg1 ) {
			static auto& inherit_default = options::vars [ _( "ragebot.auto.inherit_default" ) ].val.b;

			if ( inherit_default )
				goto set_default;

			static auto& choke_onshot = options::vars [ _( "ragebot.auto.choke_onshot" ) ].val.b;
			static auto& silent = options::vars [ _( "ragebot.auto.silent" ) ].val.b;
			static auto& hitboxes = options::vars [ _( "ragebot.auto.hitboxes" ) ].val.l;
			static auto& baim_lethal = options::vars [ _( "ragebot.auto.baim_if_lethal" ) ].val.b;
			static auto& baim_air = options::vars [ _( "ragebot.auto.baim_in_air" ) ].val.b;
			static auto& min_dmg = options::vars [ _( "ragebot.auto.min_dmg" ) ].val.f;
			static auto& dmg_accuracy = options::vars [ _( "ragebot.auto.dmg_accuracy" ) ].val.f;
			static auto& head_ps = options::vars [ _( "ragebot.auto.head_pointscale" ) ].val.f;
			static auto& body_ps = options::vars [ _( "ragebot.auto.body_pointscale" ) ].val.f;
			static auto& baim_after_misses = options::vars [ _( "ragebot.auto.force_baim" ) ].val.i;
			static auto& tickbase_shift_amount = options::vars [ _( "ragebot.auto.dt_ticks" ) ].val.i;
			static auto& auto_shoot = options::vars [ _( "ragebot.auto.auto_shoot" ) ].val.b;
			static auto& auto_slow = options::vars [ _( "ragebot.auto.auto_slow" ) ].val.b;
			static auto& auto_scope = options::vars [ _( "ragebot.auto.auto_scope" ) ].val.b;
			static auto& dt_teleport = options::vars [ _( "ragebot.auto.dt_teleport" ) ].val.b;
			static auto& dt_enabled = options::vars [ _( "ragebot.auto.dt_enabled" ) ].val.b;
			static auto& hit_chance = options::vars [ _( "ragebot.auto.hit_chance" ) ].val.f;
			static auto& dt_hit_chance = options::vars [ _( "ragebot.auto.dt_hit_chance" ) ].val.f;
			static auto& headshot_only = options::vars [ _( "ragebot.auto.headshot_only" ) ].val.b;
			static auto& onshot_only = options::vars [ _ ( "ragebot.auto.onshot_only" ) ].val.b;
			static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.auto.dt_recharge_delay" ) ].val.i;

			config.dt_recharge_delay = dt_recharge_delay;
			config.dmg_accuracy = dmg_accuracy;
			config.choke_on_shot = choke_onshot;
			config.silent = silent;
			config.headshot_only = headshot_only;
			config.scan_head = hitboxes [ 0 ];
			config.scan_neck = hitboxes [ 1 ];
			config.scan_chest = hitboxes [ 2 ];
			config.scan_pelvis = hitboxes [ 3 ];
			config.scan_arms = hitboxes [ 4 ];
			config.scan_legs = hitboxes [ 5 ];
			config.scan_feet = hitboxes [ 6 ];
			config.baim_lethal = baim_lethal;
			config.baim_air = baim_air;
			config.min_dmg = min_dmg;
			config.head_pointscale = head_ps;
			config.body_pointscale = body_ps;
			config.baim_after_misses = baim_after_misses;
			config.max_dt_ticks = tickbase_shift_amount;
			config.auto_scope = auto_scope;
			config.auto_slow = auto_slow;
			config.auto_shoot = auto_shoot;
			config.dt_teleport = dt_teleport;
			config.dt_enabled = dt_enabled;
			config.hit_chance = hit_chance;
			config.dt_hit_chance = dt_hit_chance;
			config.onshot_only = onshot_only;

			return;
		}
		else if ( g::local->weapon( )->item_definition_index( ) == weapons_t::ssg08 ) {
			static auto& inherit_default = options::vars [ _( "ragebot.scout.inherit_default" ) ].val.b;

			if ( inherit_default )
				goto set_default;

			static auto& choke_onshot = options::vars [ _( "ragebot.scout.choke_onshot" ) ].val.b;
			static auto& silent = options::vars [ _( "ragebot.scout.silent" ) ].val.b;
			static auto& hitboxes = options::vars [ _( "ragebot.scout.hitboxes" ) ].val.l;
			static auto& baim_lethal = options::vars [ _( "ragebot.scout.baim_if_lethal" ) ].val.b;
			static auto& baim_air = options::vars [ _( "ragebot.scout.baim_in_air" ) ].val.b;
			static auto& dmg_accuracy = options::vars [ _( "ragebot.scout.dmg_accuracy" ) ].val.f;
			static auto& min_dmg = options::vars [ _( "ragebot.scout.min_dmg" ) ].val.f;
			static auto& head_ps = options::vars [ _( "ragebot.scout.head_pointscale" ) ].val.f;
			static auto& body_ps = options::vars [ _( "ragebot.scout.body_pointscale" ) ].val.f;
			static auto& baim_after_misses = options::vars [ _( "ragebot.scout.force_baim" ) ].val.i;
			static auto& tickbase_shift_amount = options::vars [ _( "ragebot.scout.dt_ticks" ) ].val.i;
			static auto& auto_shoot = options::vars [ _( "ragebot.scout.auto_shoot" ) ].val.b;
			static auto& auto_slow = options::vars [ _( "ragebot.scout.auto_slow" ) ].val.b;
			static auto& auto_scope = options::vars [ _( "ragebot.scout.auto_scope" ) ].val.b;
			static auto& dt_teleport = options::vars [ _( "ragebot.scout.dt_teleport" ) ].val.b;
			static auto& dt_enabled = options::vars [ _( "ragebot.scout.dt_enabled" ) ].val.b;
			static auto& hit_chance = options::vars [ _( "ragebot.scout.hit_chance" ) ].val.f;
			static auto& dt_hit_chance = options::vars [ _( "ragebot.scout.dt_hit_chance" ) ].val.f;
			static auto& headshot_only = options::vars [ _( "ragebot.scout.headshot_only" ) ].val.b;
			static auto& onshot_only = options::vars [ _ ( "ragebot.scout.onshot_only" ) ].val.b;
			static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.scout.dt_recharge_delay" ) ].val.i;

			config.dt_recharge_delay = dt_recharge_delay;
			config.dmg_accuracy = dmg_accuracy;
			config.choke_on_shot = choke_onshot;
			config.silent = silent;
			config.headshot_only = headshot_only;
			config.scan_head = hitboxes [ 0 ];
			config.scan_neck = hitboxes [ 1 ];
			config.scan_chest = hitboxes [ 2 ];
			config.scan_pelvis = hitboxes [ 3 ];
			config.scan_arms = hitboxes [ 4 ];
			config.scan_legs = hitboxes [ 5 ];
			config.scan_feet = hitboxes [ 6 ];
			config.baim_lethal = baim_lethal;
			config.baim_air = baim_air;
			config.min_dmg = min_dmg;
			config.head_pointscale = head_ps;
			config.body_pointscale = body_ps;
			config.baim_after_misses = baim_after_misses;
			config.max_dt_ticks = tickbase_shift_amount;
			config.auto_scope = auto_scope;
			config.auto_slow = auto_slow;
			config.auto_shoot = auto_shoot;
			config.dt_teleport = dt_teleport;
			config.dt_enabled = dt_enabled;
			config.hit_chance = hit_chance;
			config.dt_hit_chance = dt_hit_chance;
			config.onshot_only = onshot_only;


			return;
		}
		else {
			goto set_default;
		}
	}
	else {
		goto set_default;
	}

set_default:

	/* reset all config options and load default settings */
	//memset ( &config, 0, sizeof config );

	static auto& choke_onshot = options::vars [ _( "ragebot.default.choke_onshot" ) ].val.b;
	static auto& silent = options::vars [ _( "ragebot.default.silent" ) ].val.b;
	static auto& hitboxes = options::vars [ _( "ragebot.default.hitboxes" ) ].val.l;
	static auto& baim_lethal = options::vars [ _( "ragebot.default.baim_if_lethal" ) ].val.b;
	static auto& baim_air = options::vars [ _( "ragebot.default.baim_in_air" ) ].val.b;
	static auto& min_dmg = options::vars [ _( "ragebot.default.min_dmg" ) ].val.f;
	static auto& dmg_accuracy = options::vars [ _( "ragebot.default.dmg_accuracy" ) ].val.f;
	static auto& head_ps = options::vars [ _( "ragebot.default.head_pointscale" ) ].val.f;
	static auto& body_ps = options::vars [ _( "ragebot.default.body_pointscale" ) ].val.f;
	static auto& baim_after_misses = options::vars [ _( "ragebot.default.force_baim" ) ].val.i;
	static auto& tickbase_shift_amount = options::vars [ _( "ragebot.default.dt_ticks" ) ].val.i;
	static auto& auto_shoot = options::vars [ _( "ragebot.default.auto_shoot" ) ].val.b;
	static auto& auto_slow = options::vars [ _( "ragebot.default.auto_slow" ) ].val.b;
	static auto& auto_scope = options::vars [ _( "ragebot.default.auto_scope" ) ].val.b;
	static auto& dt_teleport = options::vars [ _( "ragebot.default.dt_teleport" ) ].val.b;
	static auto& dt_enabled = options::vars [ _( "ragebot.default.dt_enabled" ) ].val.b;
	static auto& hit_chance = options::vars [ _( "ragebot.default.hit_chance" ) ].val.f;
	static auto& dt_hit_chance = options::vars [ _( "ragebot.default.dt_hit_chance" ) ].val.f;
	static auto& headshot_only = options::vars [ _( "ragebot.default.headshot_only" ) ].val.b;
	static auto& onshot_only = options::vars [ _ ( "ragebot.default.onshot_only" ) ].val.b;
	static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.default.dt_recharge_delay" ) ].val.i;

	config.dt_recharge_delay = dt_recharge_delay;
	config.dmg_accuracy = dmg_accuracy;
	config.choke_on_shot = choke_onshot;
	config.silent = silent;
	config.headshot_only = headshot_only;
	config.scan_head = hitboxes [ 0 ];
	config.scan_neck = hitboxes [ 1 ];
	config.scan_chest = hitboxes [ 2 ];
	config.scan_pelvis = hitboxes [ 3 ];
	config.scan_arms = hitboxes [ 4 ];
	config.scan_legs = hitboxes [ 5 ];
	config.scan_feet = hitboxes [ 6 ];
	config.baim_lethal = baim_lethal;
	config.baim_air = baim_air;
	config.min_dmg = min_dmg;
	config.head_pointscale = head_ps;
	config.body_pointscale = body_ps;
	config.baim_after_misses = baim_after_misses;
	config.max_dt_ticks = tickbase_shift_amount;
	config.auto_scope = auto_scope;
	config.auto_slow = auto_slow;
	config.auto_shoot = auto_shoot;
	config.auto_revolver = auto_revolver;
	config.dt_teleport = dt_teleport;
	config.dt_enabled = dt_enabled;
	config.hit_chance = hit_chance;
	config.dt_hit_chance = dt_hit_chance;
	config.onshot_only = onshot_only;

	VM_TIGER_BLACK_END
}

int& features::ragebot::get_target_idx( ) {
	return target_idx;
}

anims::anim_info_t& features::ragebot::get_lag_rec( int pl ) {
	return cur_lag_rec [ pl ];
}

player_t*& features::ragebot::get_target( ) {
	return last_target;
}

features::ragebot::misses_t& features::ragebot::get_misses( int pl ) {
	return misses [ pl ];
}

vec3_t& features::ragebot::get_target_pos( int pl ) {
	return target_pos [ pl ];
}

vec3_t& features::ragebot::get_shot_pos( int pl ) {
	return shot_pos [ pl ];
}

int& features::ragebot::get_hits( int pl ) {
	return hits [ pl ];
}

int& features::ragebot::get_shots( int pl ) {
	return shots [ pl ];
}

int& features::ragebot::get_hitbox( int pl ) {
	return hitbox [ pl ];
}

bool features::ragebot::dmg_hitchance ( vec3_t ang, player_t* pl, vec3_t point, int rays, int hitbox ) {
	auto weapon = g::local->weapon( );

	if ( !weapon || !weapon->data( ) )
		return false;

	//weapon->update_accuracy( );

	auto src = g::local->eyes( );

	ang = cs::calc_angle( src, point );
	cs::clamp( ang );

	auto forward = cs::angle_vec( ang );
	auto right = cs::angle_vec( ang + vec3_t( 0.0f, 90.0f, 0.0f ) );
	auto up = cs::angle_vec( ang + vec3_t( 90.0f, 0.0f, 0.0f ) );

	forward.normalize( );
	right.normalize( );
	up.normalize( );

	if ( !forward.is_valid( ) || !right.is_valid( ) || !up.is_valid( ) )
		return false;

	auto weap_spread = weapon->inaccuracy( ) + weapon->spread( );

	/* check damage accuracy */
	const auto spread_coeff = weap_spread * 0.5f * ( features::ragebot::active_config.dmg_accuracy / 100.0f );

	const auto dist_to = src.dist_to ( point );

	const auto left_point = src + ( forward - right * spread_coeff ) * dist_to;
	const auto right_point = src + ( forward + right * spread_coeff ) * dist_to;

	const auto dmg_left = autowall::dmg( g::local, pl, src, left_point, hitbox );
	const auto dmg_right = autowall::dmg( g::local, pl, src, right_point, hitbox );

	//dbg_print( _( "damage hitchance L: %d\n" ), int(dmg_left >= features::ragebot::active_config.min_dmg) );
	//dbg_print( _( "damage hitchance R: %d\n" ), int(dmg_right >= features::ragebot::active_config.min_dmg ));

	return dmg_left >= features::ragebot::active_config.min_dmg && dmg_right >= features::ragebot::active_config.min_dmg;
}

bool features::ragebot::hitchance( vec3_t ang, player_t* pl, vec3_t point, int rays, int hitbox, anims::anim_info_t& rec ) {
	if ( g::cvars::weapon_accuracy_nospread->get_bool ( ) )
		return true;

	auto weapon = g::local->weapon( );

	if ( !weapon || !weapon->data( ) )
		return false;

	if ( weapon->item_definition_index ( ) == weapons_t::ssg08 && weapon->inaccuracy ( ) < 0.009f )
		return true;

	auto src = g::local->eyes( );

	ang = cs::calc_angle( src, point );
	cs::clamp( ang );

	auto forward = cs::angle_vec( ang );
	auto right = cs::angle_vec( ang + vec3_t( 0.0f, 90.0f, 0.0f ) );
	auto up = cs::angle_vec( ang + vec3_t( 90.0f, 0.0f, 0.0f ) );

	forward.normalize( );
	right.normalize( );
	up.normalize( );

	if ( !forward.is_valid( ) || !right.is_valid( ) || !up.is_valid( ) )
		return false;

	auto hits = 0;
	auto needed_hits = static_cast< int >( static_cast< float > ( rays ) * ( features::ragebot::active_config.hit_chance / 100.0f ) );

	auto weap_spread = weapon->inaccuracy( ) + weapon->spread( );

	auto hits_hitbox = [ & ] ( const vec3_t& src, const vec3_t& dst, int hitbox ) -> bool {
		auto mdl = pl->mdl( );

		if ( !mdl )
			return false;

		auto studio_mdl = cs::i::mdl_info->studio_mdl( mdl );

		if ( !studio_mdl )
			return false;

		auto s = studio_mdl->hitbox_set( 0 );

		if ( !s )
			return false;

		auto hb = s->hitbox( hitbox );

		if ( !hb )
			return false;

		std::array< matrix3x4_t,128>& bone_matrix = rec.m_aim_bones[rec.m_side];

		auto vmin = hb->m_bbmin;
		auto vmax = hb->m_bbmax;

		VEC_TRANSFORM( hb->m_bbmin, bone_matrix [ hb->m_bone ], vmin );
		VEC_TRANSFORM( hb->m_bbmax, bone_matrix [ hb->m_bone ], vmax );

		return autowall::trace_ray( vmin, vmax, bone_matrix [ hb->m_bone ], hb->m_radius, src, dst );
	};

	static bool rand_table = false;
	static float rand_flt1 [ 255 ];
	static float rand_flt2 [ 255 ];
	static float rand_flt3 [ 255 ];

	if ( !rand_table ) {
		for ( auto& flt : rand_flt1 )
			flt = static_cast < float > ( rand( ) ) / static_cast < float > ( RAND_MAX );

		for ( auto& flt : rand_flt2 )
			flt = static_cast < float > ( rand( ) ) / static_cast < float > ( RAND_MAX );

		for ( auto& flt : rand_flt3 )
			flt = static_cast < float > ( rand( ) ) / static_cast < float > ( RAND_MAX );

		rand_table = true;
	}

	/* normal hitchance */
	for ( auto i = 0; i < rays; i++ ) {
		const auto spread_x = -weap_spread * 0.5f + ( rand_flt1 [ i ] * weap_spread );
		const auto spread_y = -weap_spread * 0.5f + ( rand_flt2 [ i ] * weap_spread );
		const auto spread_z = -weap_spread * 0.5f + ( rand_flt3 [ i ] * weap_spread );
		const auto final_pos = src + ( ( forward + vec3_t( spread_x, spread_y, spread_z ) ) * weapon->data( )->m_range );

		if ( hits_hitbox( src, final_pos, hitbox ) )
			hits++;
	}

	const auto calc_chance = static_cast< float >( hits ) / static_cast< float > ( rays ) * 100.0f;

	//dbg_print( _( "calculated chance: %.1f\n" ), calc_chance );

	/* TODO: change when doubletap is fixed */
	if ( calc_chance < ( exploits::has_shifted ? features::ragebot::active_config.dt_hit_chance : features::ragebot::active_config.hit_chance ) )
		return false;

	return true;
	//return dmg_hitchance( ang, pl, point, rays, hitbox );
}

void features::ragebot::tickbase_controller( ucmd_t* ucmd ) {
	auto can_shoot = [ & ] ( ) {
		if ( !g::local->weapon( ) || !g::local->weapon( )->ammo( ) || !g::local->weapon( )->data( ) )
			return false;

		if ( g::local->weapon( )->item_definition_index( ) == weapons_t::revolver && !( g::can_fire_revolver || cs::time2ticks( prediction::curtime ( ) ) > g::cock_ticks ) )
			return false;

		return prediction::curtime ( ) >= g::local->next_attack( ) && g::local->weapon( )->next_primary_attack( ) <= prediction::curtime ( ) && g::local->weapon( )->next_primary_attack( ) + g::local->weapon( )->data( )->m_fire_rate <= prediction::curtime ( );
	};

	/* tickbase manip controller */
	static auto& fd_mode = options::vars [ _( "antiaim.fakeduck_mode" ) ].val.i;
	static auto& fd_enabled = options::vars [ _( "antiaim.fakeduck" ) ].val.b;
	static auto& fd_key = options::vars [ _( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _( "antiaim.fakeduck_key_mode" ) ].val.i;

	const auto weapon_data = ( g::local && g::local->weapon( ) && g::local->weapon( )->data( ) ) ? g::local->weapon( )->data( ) : nullptr;
	const auto fire_rate = weapon_data ? weapon_data->m_fire_rate : 0.0f;
	auto tickbase_as_int = std::clamp<int>( static_cast< int >( active_config.max_dt_ticks ), 0, g::cvars::sv_maxusrcmdprocessticks->get_int ( ) - cs::i::client_state->choked() - 1 );

	if ( !active_config.dt_enabled || !utils::keybind_active( active_config.dt_key, active_config.dt_key_mode ) )
		tickbase_as_int = 0;

	if ( g::local && g::local->weapon( ) && g::local->weapon( )->data( ) && tickbase_as_int && !!( ucmd->m_buttons & buttons_t::attack ) && can_shoot( ) && !( g::local->weapon( )->item_definition_index( ) == weapons_t::revolver || g::local->weapon( )->data( )->m_type == weapon_type_t::knife || g::local->weapon( )->data( )->m_type >= weapon_type_t::c4 ) && !( fd_enabled && utils::keybind_active( fd_key, fd_key_mode ) ) )
		exploits::shift_tickbase( tickbase_as_int, cs::time2ticks( static_cast<float>( active_config.dt_recharge_delay ) / 1000.0f ) );
}

bool features::ragebot::can_shoot( ) {
	if ( !g::local->weapon( ) || !g::local->weapon( )->ammo( ) )
		return false;

	if ( g::local->weapon( )->item_definition_index( ) == weapons_t::revolver && !( g::can_fire_revolver || cs::time2ticks( cs::i::globals->m_curtime ) > g::cock_ticks ) )
		return false;

	return cs::i::globals->m_curtime >= g::local->next_attack( ) && cs::i::globals->m_curtime >= g::local->weapon( )->next_primary_attack( );
}

void features::ragebot::select_targets( std::deque < aim_target_t >& targets_out ) {
	vec3_t engine_angles;
	cs::i::engine->get_viewangles( engine_angles );

	cs::clamp( engine_angles );

	cs::for_each_player( [ & ] ( player_t* ent ) {
		if ( ent->team( ) == g::local->team( ) || ent->immune( ) )
			return;

		auto angle_to = cs::calc_angle( g::local->eyes( ), ent->eyes( ) );

		cs::clamp( angle_to );

		targets_out.push_back( {
			ent,
			cs::calc_fov( engine_angles, angle_to ),
			g::local->origin( ).dist_to( ent->origin( ) ),
			ent->health( )
			}
		);

		}
	);

	if ( targets_out.empty( ) )
		return;

	/* sort targets by relevance, to aim at most effective target */
	/* prioritize entity by fov, but select those that are easier to kill if they are both valid options*/
	std::sort( targets_out.begin( ), targets_out.end( ), [ & ] ( aim_target_t& lhs, aim_target_t& rhs ) {
		if ( fabsf( lhs.m_fov - rhs.m_fov ) < 20.0f )
			return lhs.m_health < rhs.m_health;

		return lhs.m_fov < rhs.m_fov;
		}
	);
}

void features::ragebot::slow ( ucmd_t* ucmd, float& old_smove, float& old_fmove ) {
	if ( !active_config.auto_slow || !( g::local->flags ( ) & flags_t::on_ground ) || !g::local->weapon ( ) || !g::local->weapon ( )->data ( ) )
		return;

	const auto vec_move = vec3_t ( ucmd->m_fmove, ucmd->m_smove, ucmd->m_umove );
	const auto magnitude = vec_move.length_2d ( );
	const auto max_speed = g::local->scoped ( ) ? g::local->weapon ( )->data ( )->m_max_speed_alt : g::local->weapon ( )->data ( )->m_max_speed;
	const auto move_to_button_ratio = 250.0f / g::cvars::cl_forwardspeed->get_float ( );
	const auto speed_ratio = max_speed * 0.33f;

	if ( g::local->vel ( ).length_2d ( ) > max_speed * 0.34f ) {
		auto vel_ang = cs::vec_angle ( vec_move );

		if ( !vel_ang.is_valid() )
			return;

		vel_ang.y = cs::normalize ( vel_ang.y + 180.0f );

		auto vel_dir = cs::angle_vec ( vel_ang );

		if ( !vel_dir.is_valid ( ) )
			return;

		const auto normal = vel_dir.normalized ( );
		const auto speed_2d = g::local->vel ( ).length_2d ( );

		old_fmove = normal.x * speed_2d;
		old_smove = normal.y * speed_2d;
	}
	/*else if ( ( old_fmove || old_smove ) && magnitude ) {
		old_fmove = ( old_fmove / magnitude ) * speed_ratio;
		old_smove = ( old_smove / magnitude ) * speed_ratio;
	}*/

	ucmd->m_buttons &= ~buttons_t::walk;
}

void features::ragebot::run_meleebot ( ucmd_t* ucmd ) {
	if ( g::local->weapon ( )->item_definition_index ( ) == weapons_t::taser && !active_config.zeus_bot )
		return;
	else if ( g::local->weapon ( )->data ( )->m_type == weapon_type_t::knife && !active_config.knife_bot )
		return;

	/*auto can_shoot = [ & ] ( ) {
		return g::local->weapon ( ) && g::local->weapon ( )->next_primary_attack ( ) <= csgo::i::globals->m_curtime && g::local->weapon ( )->ammo ( );
	};

	if ( !can_shoot ( ) )
		return;*/

	vec3_t engine_ang;
	cs::i::engine->get_viewangles ( engine_ang );

	static anims::anim_info_t best_rec {};
	player_t* best_pl = nullptr;
	float best_fov = 180.0f;
	vec3_t best_point, best_ang;

	cs::for_each_player ( [ & ] ( player_t* pl ) {
		if ( pl->team ( ) == g::local->team ( ) || pl->immune ( ) )
			return;

		const auto recs = anims::get_lagcomp_records ( pl );
		const auto simulated_rec = active_config.fix_fakelag ? anims::get_simulated_record ( pl ) : std::nullopt;

		if ( recs.empty ( ) && !simulated_rec )
			return;

		const anims::anim_info_t& rec = simulated_rec ? simulated_rec.value() : recs.front();

		auto mdl = pl->mdl ( );

		if ( !mdl )
			return;

		auto studio_mdl = cs::i::mdl_info->studio_mdl ( mdl );

		if ( !studio_mdl )
			return;

		auto s = studio_mdl->hitbox_set ( 0 );

		if ( !s )
			return;

		auto hitbox = s->hitbox ( static_cast<int>( hitbox_t::pelvis ) );

		if ( !hitbox )
			return;

		const std::array< matrix3x4_t, 128>& bone_matrix = rec.m_aim_bones[ rec.m_side ];

		auto vmin = hitbox->m_bbmin;
		auto vmax = hitbox->m_bbmax;

		VEC_TRANSFORM ( hitbox->m_bbmin, bone_matrix [ hitbox->m_bone ], vmin );
		VEC_TRANSFORM ( hitbox->m_bbmax, bone_matrix [ hitbox->m_bone ], vmax );

		const auto hitbox_pos = ( vmin + vmax ) * 0.5f;

		auto ang = cs::calc_angle ( g::local->eyes ( ), hitbox_pos );
		cs::clamp ( ang );

		const auto fov = cs::calc_fov ( ang, engine_ang );

		auto can_use = false;

		if ( g::local->weapon ( )->item_definition_index ( ) == weapons_t::taser )
			can_use = g::local->eyes ( ).dist_to ( hitbox_pos ) < 150.0f;
		else
			can_use = g::local->origin ( ).dist_to ( rec.m_origin ) < 48.0f;

		if ( cs::is_visible ( hitbox_pos ) && fov < best_fov && can_use ) {
			best_pl = pl;
			best_fov = fov;
			best_point = hitbox_pos;
			best_ang = ang;
			best_rec = rec;
		}
	} );

	if ( !active_config.auto_shoot && ( ( g::local->weapon ( )->data ( )->m_type == weapon_type_t::knife ) ? !( ucmd->m_buttons & buttons_t::attack ) : !( ucmd->m_buttons & buttons_t::attack2 ) ) )
		return;

	if ( !best_pl )
		return;

	cs::clamp ( best_ang );

	ucmd->m_angs = best_ang;

	if ( !features::ragebot::active_config.silent )
		cs::i::engine->set_viewangles ( best_ang );

	ucmd->m_tickcount = cs::time2ticks ( best_rec.m_simtime ) + cs::time2ticks( anims::lerp_time ( ) );

	get_target_pos ( best_pl->idx ( ) ) = best_point;
	get_target ( ) = best_pl;
	get_shots ( best_pl->idx ( ) )++;
	get_shot_pos ( best_pl->idx ( ) ) = g::local->eyes ( );
	get_lag_rec ( best_pl->idx ( ) ) = best_rec;
	get_target_idx ( ) = best_pl->idx ( );
	get_hitbox ( best_pl->idx ( ) ) = 5;

	if ( features::ragebot::active_config.auto_shoot && g::local->weapon ( )->item_definition_index ( ) != weapons_t::taser ) {
		auto back = best_pl->angles ( );
		back.y = cs::normalize ( back.y + 180.0f );
		auto backstab = best_ang.dist_to ( back ) < 45.0f;

		if ( backstab ) {
			ucmd->m_buttons |= buttons_t::attack2;
		}
		else {
			auto hp = best_pl->health ( );
			auto armor = best_pl->armor ( ) > 1;
			auto min_dmg1 = armor ? 34 : 40;
			auto min_dmg2 = armor ? 55 : 65;

			if ( hp <= min_dmg2 )
				ucmd->m_buttons |= buttons_t::attack2;
			else
				ucmd->m_buttons |= buttons_t::attack;
		}
	}
	else if ( features::ragebot::active_config.auto_shoot ) {
		ucmd->m_buttons |= buttons_t::attack;
	}
}

void features::ragebot::run ( ucmd_t* ucmd, float& old_smove, float& old_fmove, vec3_t& old_angs ) {
	VM_TIGER_BLACK_START
	if ( !active_config.main_switch || !g::local || !g::local->alive ( ) )
		return;

	/* Don't knifebot without an actual knifebot lmao, currently the hack just fucking shoots the air constantly w/a knife */
	if ( g::local->weapon ( ) && ( g::local->weapon ( )->data ( )->m_type == weapon_type_t::knife || g::local->weapon ( )->item_definition_index ( ) == weapons_t::taser ) ) {
		run_meleebot ( ucmd );
		return;
	}

	if ( !can_shoot ( ) ) {
		if ( active_config.auto_shoot )
			ucmd->m_buttons &= ~buttons_t::attack;

		return;
	}

	/* get potential ragebot targets */
	/* sanity checks and sorting is already done here automatically */
	std::deque < aim_target_t > targets {};
	select_targets( targets );

	if ( targets.empty ( ) )
			return;

	/* make static, save some space on the stack */
	struct {
		player_t* m_ent = nullptr;
		vec3_t m_point = vec3_t( );
		int m_hitbox = 0;
		anims::anim_info_t m_record;
		float m_dmg = 0.0f;
	} static best;

	memset( &best, 0, sizeof( best ) );

	scan_points.clear( );

	for ( auto& target : targets ) {
		/* create only once, save some space on the stack */
		static vec3_t point = vec3_t( );
		static int hitbox_out = 0;
		static anims::anim_info_t record_out;
		static float dmg_out = 0.0f;

		point = vec3_t( );
		hitbox_out = 0;
		memset( &record_out, 0, sizeof( record_out ) );
		dmg_out = 0.0f;

		idealize_shot( target.m_ent, point, hitbox_out, record_out, dmg_out );

		if ( dmg_out > 0.0f && dmg_out > best.m_dmg ) {
			best.m_ent = target.m_ent;
			best.m_point = point;
			best.m_hitbox = hitbox_out;
			best.m_record = record_out;
			best.m_dmg = dmg_out;
		}
	}

	scan_points.sync( );

	/* extrapolate autostop */ /*{
		static auto looking_at = [ ] ( ) -> player_t* {
			player_t* ret = nullptr;

			vec3_t angs;
			cs::i::engine->get_viewangles ( angs );
			cs::clamp ( angs );

			auto best_fov = 180.0f;

			cs::for_each_player ( [ & ] ( player_t* pl ) {
				if ( pl->team ( ) == g::local->team ( ) )
					return;

				auto angle_to = cs::calc_angle ( g::local->origin ( ), pl->origin ( ) );
				cs::clamp ( angle_to );
				auto fov = cs::calc_fov ( angle_to, angs );

				if ( fov < best_fov ) {
					ret = pl;
					best_fov = fov;
				}
			} );

			return ret;
		};

		const auto at_target = looking_at ( );

		if ( at_target ) {
			auto vel = g::local->vel ( );
			const auto max_speed = ( g::local->scoped ( ) ? g::local->weapon ( )->data ( )->m_max_speed_alt : g::local->weapon ( )->data ( )->m_max_speed ) * 0.34f;

			if ( vel.length_2d() > max_speed ) {
				static auto calc_velocity = [ ] ( vec3_t& vel ) {
					const auto speed = vel.length_2d ( );

					if ( speed >= 0.1f ) {
						const auto stop_speed = std::max< float > ( speed, g::cvars::sv_stopspeed->get_float ( ) );
						vel *= std::max< float > ( 0.0f, speed - g::cvars::sv_friction->get_float ( ) * stop_speed * cs::i::globals->m_ipt / speed );
					}
				};

				auto ticks_until_accurate = 0;

				for ( ticks_until_accurate = 0; ticks_until_accurate < 16; ticks_until_accurate++ ) {
					if ( vel.length_2d ( ) <= max_speed )
						break;

					calc_velocity ( vel );
				}

				const auto autostop_time = cs::ticks2time ( ticks_until_accurate );
				const auto predicted_eyes = g::local->eyes ( ) + g::local->vel ( ) * autostop_time;

				trace_t tr;
				cs::util_traceline ( predicted_eyes, at_target->origin() + at_target->view_offset(), 0x46004003, g::local, &tr );

				if ( tr.is_visible() || tr.m_hit_entity == at_target )
					slow ( ucmd, old_smove, old_fmove );
			}
		}
	}*/

	if ( !best.m_ent )
		return;
		
	auto angle_to = cs::calc_angle( g::local->eyes( ), best.m_point );
	cs::clamp( angle_to );

	auto hc = hitchance( angle_to, best.m_ent, best.m_point, 255, best.m_hitbox, best.m_record );
	auto should_aim = best.m_dmg && hc;
	
	if ( best.m_dmg && !hc
		&& g::local->weapon ( )
		&& g::local->weapon ( )->data()
		&& g::local->vel ( ).length_2d ( ) > ( g::local->scoped ( ) ? g::local->weapon ( )->data ( )->m_max_speed_alt : g::local->weapon ( )->data ( )->m_max_speed ) * 0.34f )
		slow ( ucmd, old_smove, old_fmove);

	if ( !active_config.auto_shoot )
		should_aim = !!( ucmd->m_buttons & buttons_t::attack ) && best.m_dmg;

	if ( should_aim ) {
		if ( active_config.auto_shoot ) {
			ucmd->m_buttons |= buttons_t::attack;
			g::send_packet = true;
		}

		if ( active_config.auto_scope && !g::local->scoped( ) &&
			( g::local->weapon( )->item_definition_index( ) == weapons_t::awp
				|| g::local->weapon( )->item_definition_index( ) == weapons_t::scar20
				|| g::local->weapon( )->item_definition_index( ) == weapons_t::g3sg1
				|| g::local->weapon ( )->item_definition_index ( ) == weapons_t::ssg08
				|| g::local->weapon ( )->item_definition_index ( ) == weapons_t::aug 
				|| g::local->weapon ( )->item_definition_index ( ) == weapons_t::sg553 ) )
			ucmd->m_buttons |= buttons_t::attack2;

		auto ang = angle_to - g::local->aim_punch( ) * g::cvars::weapon_recoil_scale->get_float();
		cs::clamp( ang );

		ucmd->m_angs = ang;

		if ( !active_config.silent )
			cs::i::engine->set_viewangles( ang );

		ucmd->m_tickcount = cs::time2ticks ( best.m_record.m_simtime ) + cs::time2ticks ( anims::lerp_time ( ) );

		get_target_pos( best.m_ent->idx( ) ) = best.m_point;
		get_target( ) = best.m_ent;
		get_shots( best.m_ent->idx( ) )++;
		get_shot_pos( best.m_ent->idx( ) ) = g::local->eyes( );
		get_lag_rec( best.m_ent->idx( ) ) = best.m_record;
		get_target_idx( ) = best.m_ent->idx( );
		get_hitbox( best.m_ent->idx( ) ) = best.m_hitbox;
	}
	else if ( best.m_dmg ) {
		if ( g::local->weapon ( )->item_definition_index ( ) == weapons_t::awp
			|| g::local->weapon ( )->item_definition_index ( ) == weapons_t::scar20
			|| g::local->weapon ( )->item_definition_index ( ) == weapons_t::g3sg1
			|| g::local->weapon ( )->item_definition_index ( ) == weapons_t::ssg08
			|| g::local->weapon ( )->item_definition_index ( ) == weapons_t::aug
			|| g::local->weapon ( )->item_definition_index ( ) == weapons_t::sg553 ) {
			if ( active_config.auto_scope && !g::local->scoped( ) )
				ucmd->m_buttons |= buttons_t::attack2;
			else if ( active_config.auto_scope && g::local->scoped( ) )
				ucmd->m_buttons &= ~buttons_t::attack2;
		}
	}

	VM_TIGER_BLACK_END
}

bool features::ragebot::create_points( player_t* ent, anims::anim_info_t& rec, int i, std::deque< vec3_t >& points, multipoint_side_t multipoint_side ) {
	vec3_t hitbox_pos = vec3_t( 0.0f, 0.0f, 0.0f );
	float hitbox_rad = 0.0f;
	float hitbox_zrad = 0.0f;

	if ( !get_hitbox( ent, rec, i, hitbox_pos, hitbox_rad, hitbox_zrad ) )
		return false;

	multipoint_mode_t multipoint_mask = multipoint_mode_t::none;

	/* enable multipoint for these_hitboxes */
	multipoint_mask |= multipoint_mode_t::center;

	float pointscale = 0.0f;

	if ( i == hitbox_head ) {
		/* fix accidental "body aim" by aiming at the higher point only (when needed ?is facing backwards?) */
		if (abs( cs::normalize ( cs::calc_angle ( g::local->eyes ( ), rec.m_origin ).y - cs::normalize ( rec.m_angles.y ) ) ) < 60.0f )
			multipoint_mask &= ~multipoint_mode_t::center;

		multipoint_mask |= multipoint_mode_t::top;
		pointscale = active_config.head_pointscale;
	}
	else if ( i >= hitbox_pelvis && i <= hitbox_upper_chest ) {
		/* we can optimize and save quite a bit of points to scan by just guessing what points we wont be able to hit*/
		/* all we need to do is see if we can hit most points on either side before scanning all of them */
		/* (scan 1 point which will help us rule out one side completely... maybe even 9+ points saved) */
		switch ( multipoint_side ) {
			case multipoint_side_t::none: multipoint_mask |= multipoint_mode_t::none; break;
			case multipoint_side_t::left: multipoint_mask |= multipoint_mode_t::left; break;
			case multipoint_side_t::right: multipoint_mask |= multipoint_mode_t::right; break;
		}

		pointscale = active_config.body_pointscale;
	}
	else {
		/* no need to multipoint*/
		pointscale = 0.0f;
	}

	/* forgot this last time, we need a scalar, but pointscale goes from 0 - 100 */
	pointscale /= 100.0f;

	auto fwd_vec = g::local->eyes( ) - hitbox_pos;
	fwd_vec.normalize( );

	auto side_vec = fwd_vec.cross_product( vec3_t( 0.0f, 0.0f, 1.0f ) );

	/* force multipoint to only center if pointscale is 0 */
	if ( !pointscale )
		multipoint_mask = multipoint_mode_t::none | multipoint_mode_t::center;

	if ( !!(multipoint_mask & multipoint_mode_t::center) )
		points.push_back( hitbox_pos );

	if ( !!(multipoint_mask & multipoint_mode_t::left ))
		points.push_back( hitbox_pos + side_vec * hitbox_rad * pointscale );

	if ( !!(multipoint_mask & multipoint_mode_t::right) )
		points.push_back( hitbox_pos + vec3_t( -side_vec.x, -side_vec.y, side_vec.z ) * hitbox_rad * pointscale );

	if ( !!(multipoint_mask & multipoint_mode_t::bottom ))
		points.push_back( hitbox_pos - vec3_t( 0.0f, 0.0f, hitbox_zrad ) * pointscale );

	if ( !!(multipoint_mask & multipoint_mode_t::top ))
		points.push_back( hitbox_pos + vec3_t( 0.0f, 0.0f, hitbox_zrad ) * pointscale );

	for ( auto& point : points )
		scan_points.emplace( point );

	return true;
}

bool features::ragebot::get_hitbox( player_t* ent, anims::anim_info_t& rec, int i, vec3_t& pos_out, float& rad_out, float& zrad_out ) {
	auto pl = ent;

	if ( !pl )
		return false;

	auto mdl = pl->mdl( );

	if ( !mdl )
		return false;

	auto studio_mdl = cs::i::mdl_info->studio_mdl( mdl );

	if ( !studio_mdl )
		return false;

	auto set = studio_mdl->hitbox_set( 0 );

	if ( !set )
		return false;

	auto hitbox = set->hitbox( i );

	if ( !hitbox )
		return false;

	std::array< matrix3x4_t, 128>& bone_matrix = rec.m_aim_bones[ rec.m_side ];

	vec3_t vmin, vmax;
	VEC_TRANSFORM ( hitbox->m_bbmin, bone_matrix [ hitbox->m_bone ], vmin );
	VEC_TRANSFORM( hitbox->m_bbmax, bone_matrix [ hitbox->m_bone ], vmax );

	pos_out = ( vmin + vmax ) * 0.5f;

	if ( hitbox->m_radius < 0.0f ) {
		rad_out = ( vmax - vmin ).length_2d() * 0.5f;
		zrad_out = ( vmax - vmin ).z * 0.5f;
	}
	else {
		rad_out = hitbox->m_radius;
		zrad_out = abs ( vmax.z - vmin.z + hitbox->m_radius * 2.0f ) * 0.5f;
	}

	return true;
}

bool features::ragebot::hitscan( player_t* ent, anims::anim_info_t& rec, vec3_t& pos_out, int& hitbox_out, float& best_dmg ) {
	VM_TIGER_BLACK_START
	auto pl = ent;

	if ( !pl )
		return false;

	const auto weapon = g::local->weapon( );

	if ( !weapon )
		return false;

	const auto weapon_data = weapon->data( );

	if ( !weapon_data )
		return false;

	/* select side to multipoint on (only one side at a time, saves a shit ton of frames) */
	multipoint_side_t multipoint_side = multipoint_side_t::none;

	const auto src = g::local->eyes( );

	auto eyes_max = rec.m_origin + vec3_t( 0.0f, 0.0f, 64.0f );
	auto fwd = eyes_max - src;
	fwd.normalize( );

	if ( !fwd.is_valid( ) )
		return false;

	auto right_dir = fwd.cross_product( vec3_t( 0.0f, 0.0f, 1.0f ) );
	auto left_dir = -right_dir;
	auto dmg_center = autowall::dmg( g::local, pl, src, eyes_max, 0 /* pretend player would be there */ );
	auto dmg_left = autowall::dmg( g::local, pl, src + left_dir * 35.0f, eyes_max + left_dir * 35.0f, 0 /* pretend player would be there */ );
	auto dmg_right = autowall::dmg( g::local, pl, src + right_dir * 35.0f, eyes_max + right_dir * 35.0f, 0 /* pretend player would be there */ );

	/* will we most likely not do damage at all? then let's cancel target selection altogether i guess */
	if ( !dmg_center && !dmg_left && !dmg_right )
		return false;

	std::deque< int > hitboxes { };

	if ( active_config.scan_pelvis )
		hitboxes.push_back( hitbox_pelvis );

	if ( active_config.scan_chest ) {
		hitboxes.push_back( hitbox_chest );
	}

	if ( active_config.scan_head )
		hitboxes.push_back( hitbox_head );

	if ( active_config.scan_neck )
		hitboxes.push_back( hitbox_neck );

	if ( active_config.scan_feet ) {
		hitboxes.push_back( hitbox_right_foot );
		hitboxes.push_back( hitbox_left_foot );
	}

	if ( active_config.scan_legs ) {
		hitboxes.push_back( hitbox_right_calf );
		hitboxes.push_back( hitbox_left_calf );
	}

	if ( active_config.scan_arms ) {
		hitboxes.push_back( hitbox_right_forearm );
		hitboxes.push_back( hitbox_left_forearm );
	}

	/* force baim, literally removes head hitbox from hitscan  */
	float scaled_dmg = static_cast< float > ( weapon_data->m_dmg );
	autowall::scale_dmg( pl, weapon_data, autowall::hitbox_to_hitgroup( hitbox_pelvis ), scaled_dmg );

	if ( (/*(get_misses( rec.m_pl->idx( ) ).bad_resolve > active_config.baim_after_misses)
		||*/ ( active_config.baim_air && !( rec.m_flags & flags_t::on_ground ) )
		|| ( active_config.baim_lethal && scaled_dmg > pl->health( ) )
		|| (( exploits::is_ready ( ) || exploits::has_shifted ) && active_config.max_dt_ticks > 6 && active_config.dt_enabled && utils::keybind_active ( active_config.dt_key, active_config.dt_key_mode ) ))
		&& !active_config.headshot_only ) {
		std::deque<int> new_hitboxes {};
	
		for ( auto& hitbox : hitboxes )
			if( hitbox != hitbox_head && hitbox != hitbox_neck )
				new_hitboxes.push_back ( hitbox );
	
		hitboxes = new_hitboxes;
	}

	/* allows us to make smarter choices for hitboxes if we know how many shots we will want to shoot */
	/* with doubletap, we can just go for body anyways (scale damage by 2, will calculate as if was 1 shot) */
	const auto damage_scalar = ( exploits::is_ready ( ) || exploits::has_shifted ) ? 1.0f : 1.0f;
	/* TODO: remove when doubletap fixed */

	//const auto body_priority = g::next_tickbase_shot || rec.m_pl->health( ) < weapon_data->m_dmg;
//
	///* put body hitbox at front of queue if it is prioritized */
	//if ( body_priority && !hitboxes.empty( ) ) {
	//	const auto body_element = std::find_if( hitboxes.begin( ), hitboxes.end( ), [ ] ( int& hitbox ) { return hitbox == hitbox_pelvis; } );
//
	//	if ( body_element != hitboxes.end( ) ) {
	//		/* erase body element and place at front of queue to be scanned first */
	//		hitboxes.erase( body_element, body_element );
	//		hitboxes.push_front( hitbox_pelvis );
	//	}
	//}

	if ( active_config.headshot_only ) {
		hitboxes.clear ( );
		hitboxes.push_front ( hitbox_head );
	}

	if ( hitboxes.empty( ) )
		return false;

	/* select side to multipoint on (only one side at a time, saves a shit ton of frames) */
	if ( !dmg_left && !dmg_right )
		multipoint_side = multipoint_side_t::none;
	else if ( dmg_left > dmg_right )
		multipoint_side = multipoint_side_t::left;
	else if ( dmg_right > dmg_left )
		multipoint_side = multipoint_side_t::right;

	const auto safe_point_active = active_config.safe_point && utils::keybind_active( active_config.safe_point_key, active_config.safe_point_key_mode );

	/* use other matrix with same points, trying to find alignments in the two matricies */
	/* shift over by 1 entry in our records */
	// SAFE POINT BONE MATRIX DOESN'T WORK RIGHT NOW
	auto opposite_side_max = anims::desync_side_t::desync_middle;

	switch ( rec.m_side ) {
	case anims::desync_side_t::desync_left_max:
	case anims::desync_side_t::desync_left_half:
		opposite_side_max = anims::desync_side_t::desync_right_max;
		break;
	case anims::desync_side_t::desync_middle:
		opposite_side_max = anims::desync_side_t::desync_left_max;
		break;
	case anims::desync_side_t::desync_right_max:
	case anims::desync_side_t::desync_right_half:
		opposite_side_max = anims::desync_side_t::desync_left_max;
		break;
	}

	std::array< matrix3x4_t, 128>& dmg_scan_matrix = safe_point_active ? rec.m_aim_bones[ opposite_side_max ] : rec.m_aim_bones[ rec.m_side ];

	const auto backup_origin = pl->origin ( );
	auto backup_abs_origin = pl->abs_origin ( );
	const auto backup_bone_cache = pl->bone_cache ( );
	const auto backup_mins = pl->mins ( );
	const auto backup_maxs = pl->maxs ( );

	pl->origin ( ) = rec.m_origin;
	pl->set_abs_origin ( rec.m_origin );
	pl->bone_cache ( ) = dmg_scan_matrix.data();
	pl->mins ( ) = rec.m_mins;
	pl->maxs ( ) = rec.m_maxs;

	float best_dmg_tmp = 0.0f;
	vec3_t best_pos = vec3_t(0.0f, 0.0f, 0.0f);
	int best_hitbox = 0;

	/* select best point on all hitboxes */
	for ( auto& hitbox : hitboxes ) {
		std::deque< vec3_t > points { };

		if ( !create_points( ent, rec, hitbox, points, multipoint_side ) )
			continue;

		if ( points.empty( ) )
			continue;

		vec3_t best_point = vec3_t( );
		float best_points_damage = 0.0f;

		/* select best point on hitbox */
		/* scan all selected points and take first one we find, there's no point in scanning for more */
		for ( auto& point : points ) {
			const auto dmg = autowall::dmg ( g::local, pl, src, point, -1 ) * damage_scalar;
			//dbg_print ( _("calculated damage: %.1f"), dmg );

			if ( dmg > best_points_damage ) {
				best_point = point;
				best_points_damage = dmg;
			}
		}

		if ( best_points_damage > best_dmg_tmp ) {
			/* save best data */
			best_pos = best_point;
			best_hitbox = hitbox;

			best_dmg_tmp = best_points_damage;
		}

		/* if we meet min dmg requirement or shot will be fatal, we can immediately break out */
		//if ( best_dmg_tmp > active_config.min_dmg || best_dmg_tmp > rec.m_pl->health( ) ) {
		//	best_dmg = best_dmg_tmp;
		//	break;
		//}
	}

	if ( best_dmg_tmp > best_dmg && ( best_dmg_tmp > active_config.min_dmg || best_dmg_tmp > pl->health( ) ) ) {
		/* save best data */
		pos_out = best_pos;
		hitbox_out = best_hitbox;
		best_dmg = best_dmg_tmp;
	}

	/* restore player data to what it was before so we dont mess up anything */
	pl->origin ( ) = backup_origin;
	pl->set_abs_origin ( backup_abs_origin );
	pl->bone_cache ( ) = backup_bone_cache;
	pl->mins ( ) = backup_mins;
	pl->maxs ( ) = backup_maxs;

	VM_TIGER_BLACK_END

	return best_dmg_tmp > 0.0f;
}

void features::ragebot::idealize_shot( player_t* ent, vec3_t& pos_out, int& hitbox_out, anims::anim_info_t& rec_out, float& best_dmg ) {
	VM_TIGER_BLACK_START
	constexpr int SIMILAR_RECORD_THRESHOLD = 2;

	if ( !ent->valid ( ) )
		return;

	const auto recs = anims::get_lagcomp_records ( ent );
	const auto simulated_rec = active_config.fix_fakelag ? anims::get_simulated_record ( ent ) : std::nullopt;

	if ( recs.empty ( ) && !simulated_rec )
		return;

	const auto shot = anims::get_onshot( recs );

	std::deque < anims::anim_info_t > best_recs { };

	/* prefer onshot */
	if ( shot ) {
		best_recs.push_back( shot.value ( ) );

		if ( !active_config.onshot_only ) {
			if ( cs::time2ticks( shot.value ( ).m_simtime ) != cs::time2ticks( recs.back ( ).m_simtime ) )
				best_recs.push_back ( recs.back ( ) );
		}
	}
	else if ( !active_config.onshot_only ) {
		if ( recs.empty ( ) ) {
			best_recs.push_back ( simulated_rec.value() );
		}
		else {
			float highest_speed = 0.0f;
			float highest_sideways_amount = 0.0f;

			const anims::anim_info_t* newest_rec = &recs.front ( );
			const anims::anim_info_t* oldest_rec = &recs.back ( );
			const anims::anim_info_t* speed_rec = nullptr;
			const anims::anim_info_t* angdiff_rec = nullptr;

			const auto at_target_yaw = cs::normalize ( cs::calc_angle ( g::local->eyes ( ), ent->eyes ( ) ).y );

			for ( auto& rec : recs ) {
				const auto speed2d = rec.m_vel.length_2d ( );

				if ( !!( rec.m_flags & flags_t::on_ground ) && speed2d > highest_speed ) {
					speed_rec = &rec;
					highest_speed = speed2d;
				}

				const auto ang_diff = abs ( cs::normalize ( at_target_yaw - cs::normalize ( rec.m_angles.y ) ) );

				if ( ang_diff > highest_sideways_amount ) {
					angdiff_rec = &rec;
					highest_sideways_amount = ang_diff;
				}
			}

			/* eliminate similar records */
			if ( angdiff_rec ) {
				if ( speed_rec && abs ( cs::time2ticks ( speed_rec->m_simtime ) - cs::time2ticks ( angdiff_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					speed_rec = nullptr;

				if ( newest_rec && abs ( cs::time2ticks ( newest_rec->m_simtime ) - cs::time2ticks ( angdiff_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					newest_rec = nullptr;

				if ( oldest_rec && abs ( cs::time2ticks ( oldest_rec->m_simtime ) - cs::time2ticks ( angdiff_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					oldest_rec = nullptr;

				best_recs.push_back ( *angdiff_rec );
			}

			if ( speed_rec ) {
				if ( newest_rec && abs ( cs::time2ticks ( newest_rec->m_simtime ) - cs::time2ticks ( speed_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					newest_rec = nullptr;

				if ( oldest_rec && abs ( cs::time2ticks ( oldest_rec->m_simtime ) - cs::time2ticks ( speed_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					oldest_rec = nullptr;

				best_recs.push_back ( *speed_rec );
			}

			if ( newest_rec && oldest_rec && cs::time2ticks ( newest_rec->m_simtime ) != cs::time2ticks ( oldest_rec->m_simtime ) )
				best_recs.push_back ( *newest_rec );

			if ( oldest_rec )
				best_recs.push_back ( *oldest_rec );

			if ( simulated_rec )
				best_recs.push_back ( simulated_rec.value ( ) );
		}
	}

	if ( best_recs.empty( ) )
		return;

	/* scan for a good shot in one of the records we have */
	for ( auto& record : best_recs ) {
		if ( hitscan( ent, record, pos_out, hitbox_out, best_dmg ) ) {
			rec_out = record;
			break;
		}
	}

	VM_TIGER_BLACK_END
}

#pragma optimize( "2", off )