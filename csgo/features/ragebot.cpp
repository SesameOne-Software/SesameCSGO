#include "ragebot.hpp"
#include <deque>
#include "autowall.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "prediction.hpp"
#include "../animations/resolver.hpp"
#include "../menu/options.hpp"

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
std::array< features::lagcomp::lag_record_t, 65 > cur_lag_rec { 0 };
features::ragebot::weapon_config_t features::ragebot::active_config;

features::ragebot::c_scan_points features::ragebot::scan_points;

#pragma optimize( "2", on )

void features::ragebot::get_weapon_config( weapon_config_t& const config ) {
	if ( !g::local || !g::local->alive( ) || !g::local->weapon( ) || !g::local->weapon( )->data( ) )
		return;

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
}

int& features::ragebot::get_target_idx( ) {
	return target_idx;
}

features::lagcomp::lag_record_t& features::ragebot::get_lag_rec( int pl ) {
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

bool features::ragebot::dmg_hitchance( vec3_t ang, player_t* pl, vec3_t point, int rays, int hitbox ) {
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

	dbg_print( _( "damage hitchance L: %d\n" ), int(dmg_left >= features::ragebot::active_config.min_dmg) );
	dbg_print( _( "damage hitchance R: %d\n" ), int(dmg_right >= features::ragebot::active_config.min_dmg ));

	return dmg_left >= features::ragebot::active_config.min_dmg && dmg_right >= features::ragebot::active_config.min_dmg;
}

bool features::ragebot::hitchance( vec3_t ang, player_t* pl, vec3_t point, int rays, int hitbox, lagcomp::lag_record_t& rec ) {
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

	weapon->update_accuracy( );

	auto weap_spread = weapon->inaccuracy( ) + weapon->spread( );

	static auto hits_hitbox = [ & ] ( const vec3_t& src, const vec3_t& dst, int hitbox ) -> bool {
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

		const auto misses = get_misses( pl->idx( ) ).bad_resolve;

		matrix3x4_t* bone_matrix = nullptr;

		if ( misses % 3 == 0 )
			bone_matrix = rec.m_bones1;
		else if ( misses % 3 == 1 )
			bone_matrix = rec.m_bones2;
		else
			bone_matrix = rec.m_bones3;

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

		if ( g::local->weapon( )->item_definition_index( ) == weapons_t::revolver && !( g::can_fire_revolver || cs::time2ticks( cs::i::globals->m_curtime ) > g::cock_ticks ) )
			return false;

		return cs::i::globals->m_curtime >= g::local->next_attack( ) && g::local->weapon( )->next_primary_attack( ) <= cs::i::globals->m_curtime && g::local->weapon( )->next_primary_attack( ) + g::local->weapon( )->data( )->m_fire_rate <= cs::i::globals->m_curtime;
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

void features::ragebot::run( ucmd_t* ucmd, float& old_smove, float& old_fmove, vec3_t& old_angs ) {
	if ( !active_config.main_switch || !g::local || !g::local->alive( ) )
		return;

	if ( !can_shoot( ) ) {
		if ( active_config.auto_shoot )
			ucmd->m_buttons &= ~buttons_t::attack;

		return;
	}

	/* Don't knifebot without an actual knifebot lmao, currently the hack just fucking shoots the air constantly w/a knife */
	if ( g::local->weapon ( ) && g::local->weapon ( )->data ( )->m_type == weapon_type_t::knife/*type_knife*/ && g::local->weapon ( )->item_definition_index ( ) != weapons_t::taser /*taser*/ )
		return;

	/* get potential ragebot targets */
	/* sanity checks and sorting is already done here automatically */
	std::deque < aim_target_t > targets {};
	select_targets( targets );

	if ( targets.empty( ) )
		return;

	/* make static, save some space on the stack */
	struct {
		player_t* m_ent = nullptr;
		vec3_t m_point = vec3_t( );
		int m_hitbox = 0;
		lagcomp::lag_record_t m_record;
		float m_dmg = 0.0f;
	} static best;

	memset( &best, 0, sizeof( best ) );

	scan_points.clear( );

	for ( auto& target : targets ) {
		/* create only once, save some space on the stack */
		static vec3_t point = vec3_t( );
		static int hitbox_out = 0;
		static lagcomp::lag_record_t record_out;
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

	/* extrapolate autostop */ {
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

			/* needs to slow down */
			if ( vel.length_2d() > max_speed ) {
				static auto calc_velocity = [ ] ( vec3_t& vel ) {
					const auto speed = vel.length ( );

					if ( speed >= 0.1f ) {
						const auto stop_speed = std::max< float > ( speed, g::cvars::sv_stopspeed->get_float ( ) );
						vel *= std::max< float > ( 0.0f, speed - g::cvars::sv_friction->get_float ( ) * stop_speed * cs::i::globals->m_ipt / speed );
					}
				};

				auto ticks_until_accurate = 0;

				for ( ticks_until_accurate = 0; ticks_until_accurate < 16; ++ticks_until_accurate ) {
					if ( vel.length_2d ( ) <= max_speed )
						break;

					calc_velocity ( vel );
				}

				const auto autostop_time = cs::ticks2time ( ticks_until_accurate );

				const auto pred_origin = g::local->origin ( ) + g::local->vel ( ) * autostop_time;
				//const auto pred_eyes = pred_origin + vec3_t ( 0.0f, 0.0f, 64.0f );
				const auto pred_eyes = g::local->eyes ( ) + g::local->vel ( ) * autostop_time;

				const auto ent_vel = at_target->vel ( );
				const auto choke_time = cs::ticks2time ( anims::players::choked_commands [ at_target->idx ( ) ] );
				const auto ent_pred_origin = at_target->origin ( ) + ent_vel * ( choke_time + autostop_time );
				//const auto ent_pred_eyes = ent_pred_origin + vec3_t ( 0.0f, 0.0f, 64.0f );
				const auto ent_pred_eyes = at_target->origin ( ) + at_target->view_offset ( ) + ent_vel * ( choke_time + autostop_time );

				trace_t tr;
				cs::util_traceline ( pred_eyes, ent_pred_eyes, 0x46004003, g::local, &tr );

				if ( tr.m_fraction > 0.97f
					|| tr.m_hit_entity == at_target ) {
					slow ( ucmd, old_smove, old_fmove );
				}
			}
		}
	}

	if ( !best.m_ent )
		return;

	auto angle_to = cs::calc_angle( g::local->eyes( ), best.m_point );
	cs::clamp( angle_to );

	auto hc = hitchance( angle_to, best.m_ent, best.m_point, 255, best.m_hitbox, best.m_record );
	auto should_aim = best.m_dmg && hc;

	if ( best.m_dmg && !hc && g::local->vel ( ).length_2d ( ) > 0.1f )
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

		best.m_record.backtrack( ucmd );

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
}

bool features::ragebot::create_points( lagcomp::lag_record_t& rec, int i, std::deque< vec3_t >& points, multipoint_side_t multipoint_side ) {
	vec3_t hitbox_pos = vec3_t( 0.0f, 0.0f, 0.0f );
	float hitbox_rad = 0.0f;
	float hitbox_zrad = 0.0f;

	if ( !get_hitbox( rec, i, hitbox_pos, hitbox_rad, hitbox_zrad ) )
		return false;

	multipoint_mode_t multipoint_mask = multipoint_mode_t::none;

	/* enable multipoint for these_hitboxes */
	multipoint_mask |= multipoint_mode_t::center;

	float pointscale = 0.0f;

	if ( i == hitbox_head ) {
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

bool features::ragebot::get_hitbox( lagcomp::lag_record_t& rec, int i, vec3_t& pos_out, float& rad_out, float& zrad_out ) {
	auto mdl = rec.m_pl->mdl( );

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

	matrix3x4_t* bone_matrix = nullptr;

	const auto misses = get_misses( rec.m_pl->idx( ) ).bad_resolve;

	if ( misses % 3 == 0 )
		bone_matrix = rec.m_bones1;
	else if ( misses % 3 == 1 )
		bone_matrix = rec.m_bones2;
	else
		bone_matrix = rec.m_bones3;

	vec3_t vmin, vmax;
	VEC_TRANSFORM( hitbox->m_bbmin, bone_matrix [ hitbox->m_bone ], vmin );
	VEC_TRANSFORM( hitbox->m_bbmax, bone_matrix [ hitbox->m_bone ], vmax );

	pos_out = ( vmin + vmax ) * 0.5f;

	if ( hitbox->m_radius < 0.0f ) {
		rad_out = ( vmax - vmin ).length_2d() * 0.5f;
		zrad_out = ( vmax - vmin ).z * 0.5f;
	}
	else {
		rad_out = hitbox->m_radius;
		zrad_out = fabsf ( vmax.z - vmin.z + hitbox->m_radius * 2.0f ) * 0.5f;
	}

	return true;
}

bool features::ragebot::hitscan( lagcomp::lag_record_t& rec, vec3_t& pos_out, int& hitbox_out, float& best_dmg ) {
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
	auto dmg_center = autowall::dmg( g::local, rec.m_pl, src, eyes_max, 0 /* pretend player would be there */ );
	auto dmg_left = autowall::dmg( g::local, rec.m_pl, src + left_dir * 35.0f, eyes_max + left_dir * 35.0f, 0 /* pretend player would be there */ );
	auto dmg_right = autowall::dmg( g::local, rec.m_pl, src + right_dir * 35.0f, eyes_max + right_dir * 35.0f, 0 /* pretend player would be there */ );

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

	if ( active_config.headshot_only ) {
		hitboxes.clear( );
		hitboxes.push_front( hitbox_head );
	}

	/* force baim, literally removes head hitbox from hitscan  */
	float scaled_dmg = static_cast< float > ( weapon_data->m_dmg );
	autowall::scale_dmg( rec.m_pl, weapon_data, autowall::hitbox_to_hitgroup( hitbox_pelvis ), scaled_dmg );

	if ( /*(get_misses( rec.m_pl->idx( ) ).bad_resolve > active_config.baim_after_misses)
		||*/ ( active_config.baim_air && !( rec.m_flags & flags_t::on_ground ) )
		|| ( active_config.baim_lethal && scaled_dmg > rec.m_pl->health( ) )
		|| (( exploits::is_ready ( ) || exploits::has_shifted ) && active_config.max_dt_ticks > 6 && active_config.dt_enabled && utils::keybind_active ( active_config.dt_key, active_config.dt_key_mode ) )) {
		std::deque<int> new_hitboxes {};
	
		for ( auto& hitbox : hitboxes )
			if( hitbox != hitbox_head && hitbox != hitbox_neck )
				new_hitboxes.push_back ( hitbox );
	
		hitboxes = new_hitboxes;
	}

	/* allows us to make smarter choices for hitboxes if we know how many shots we will want to shoot */
	/* with doubletap, we can just go for body anyways (scale damage by 2, will calculate as if was 1 shot) */
	const auto damage_scalar = exploits::has_shifted ? 1.0f : 1.0f;
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
	matrix3x4_t* dmg_scan_matrix = nullptr;

	if ( safe_point_active ) {
		const auto misses = get_misses( rec.m_pl->idx( ) ).bad_resolve + 1;

		if ( misses % 3 == 0 )
			dmg_scan_matrix = rec.m_bones1;
		else if ( misses % 3 == 1 )
			dmg_scan_matrix = rec.m_bones2;
		else
			dmg_scan_matrix = rec.m_bones3;
	}
	else {
		const auto misses = get_misses( rec.m_pl->idx( ) ).bad_resolve;

		if ( misses % 3 == 0 )
			dmg_scan_matrix = rec.m_bones1;
		else if ( misses % 3 == 1 )
			dmg_scan_matrix = rec.m_bones2;
		else
			dmg_scan_matrix = rec.m_bones3;
	}

	auto backup_abs_origin = rec.m_pl->abs_origin( );
	const auto backup_origin = rec.m_pl->origin( );
	const auto backup_min = rec.m_pl->mins( );
	const auto backup_max = rec.m_pl->maxs( );

	/* making this static might save a ton of repetitive allocation*/
	const auto backup_bones = rec.m_pl->bone_cache ( );

	/* set playter data required for autowall and hitscan to work */
	rec.m_pl->mins( ) = rec.m_min;
	rec.m_pl->maxs( ) = rec.m_max;
	rec.m_pl->origin( ) = rec.m_origin;
	//rec.m_pl->set_abs_origin( rec.m_origin );
	rec.m_pl->bone_cache ( ) = dmg_scan_matrix;

	float best_dmg_tmp = 0.0f;
	vec3_t best_pos;
	int best_hitbox;

	/* select best point on all hitboxes */
	for ( auto& hitbox : hitboxes ) {
		std::deque< vec3_t > points { };

		if ( !create_points( rec, hitbox, points, multipoint_side ) )
			continue;

		if ( points.empty( ) )
			continue;

		vec3_t best_point = vec3_t( );
		float best_points_damage = 0.0f;

		/* select best point on hitbox */
		/* scan all selected points and take first one we find, there's no point in scanning for more */
		for ( auto& point : points ) {
			const auto dmg = autowall::dmg( g::local, rec.m_pl, src, point, -1 ) * damage_scalar;

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

	if ( best_dmg_tmp > best_dmg && ( best_dmg_tmp > active_config.min_dmg || best_dmg_tmp > rec.m_pl->health( ) ) ) {
		/* save best data */
		pos_out = best_pos;
		hitbox_out = best_hitbox;
		best_dmg = best_dmg_tmp;
	}

	/* restore player data to what it was before so we dont fuck up anything */
	rec.m_pl->mins( ) = backup_min;
	rec.m_pl->maxs( ) = backup_max;
	rec.m_pl->origin( ) = backup_origin;
	//rec.m_pl->set_abs_origin( backup_abs_origin );
	rec.m_pl->bone_cache ( ) = backup_bones;

	return best_dmg_tmp > 0.0f;
}

void features::ragebot::idealize_shot( player_t* ent, vec3_t& pos_out, int& hitbox_out, lagcomp::lag_record_t& rec_out, float& best_dmg ) {
	constexpr int SIMILAR_RECORD_THRESHOLD = 3;

	const auto recs = lagcomp::get( ent );
	const auto shot = lagcomp::get_shot( ent );

	if ( !ent->valid( ) )
		return;

	std::deque < lagcomp::lag_record_t > best_recs { };

	///* prefer onshot */
	if ( shot.second ) {
		best_recs.push_back( shot.first );
		//
		float oldest_simtime = std::numeric_limits<float>::max ( );
		//
		lagcomp::lag_record_t* oldest_rec = nullptr;
		//
		if ( recs.second ) {
			for ( auto& rec : recs.first ) {
				if ( rec.m_simtime < oldest_simtime && !rec.m_extrapolated ) {
					oldest_rec = &rec;
					oldest_simtime = rec.m_simtime;
				}
			}
		}
		//
		if ( oldest_rec )
			best_recs.push_back( *oldest_rec );
	}
	else if ( recs.second && !active_config.onshot_only ) {
		float newest_simtime = 0.0f;
		float newest_simulated_simtime = 0.0f;
		float oldest_simtime = std::numeric_limits<float>::max ( );
		float highest_speed = 0.0f;
		float highest_sideways_amount = 0.0f;

		//
		lagcomp::lag_record_t* newest_rec = nullptr;
		lagcomp::lag_record_t* oldest_rec = nullptr;
		lagcomp::lag_record_t* simulated_rec = nullptr;
		lagcomp::lag_record_t* speed_rec = nullptr;
		lagcomp::lag_record_t* ang_diff_rec = nullptr;

		const auto at_target_yaw = cs::normalize( cs::calc_angle( g::local->eyes( ), ent->eyes( ) ).y );
		//
		for ( auto& rec : recs.first ) {
			if ( rec.m_simtime < oldest_simtime && !rec.m_extrapolated ) {
				oldest_rec = &rec;
				oldest_simtime = rec.m_simtime;
			}
			//
			if ( rec.m_simtime > newest_simtime && !rec.m_extrapolated ) {
				newest_rec = &rec;
				newest_simtime = rec.m_simtime;
			}

			const auto speed2d = rec.m_vel.length_2d( );

			if ( !!(rec.m_flags & flags_t::on_ground) && speed2d > highest_speed && !rec.m_extrapolated ) {
				speed_rec = &rec;
				highest_speed = speed2d;
			}

			const auto ang_diff = fabsf( cs::normalize( at_target_yaw - cs::normalize( rec.m_ang.y ) ) );

			if ( ang_diff > highest_sideways_amount && !rec.m_extrapolated ) {
				ang_diff_rec = &rec;
				highest_sideways_amount = ang_diff;
			}
			//
						/* removed for now because lag prediction is causing inaccracies */
						//if ( rec.m_simtime > newest_simulated_simtime && rec.m_extrapolated ) {
						//	simulated_rec = &rec;
						//	newest_simulated_simtime = rec.m_simtime;
						//}
		}

		/* eliminate similar records */
		if ( ang_diff_rec ) {
			if ( speed_rec && abs( speed_rec->m_tick - ang_diff_rec->m_tick ) <= SIMILAR_RECORD_THRESHOLD )
				speed_rec = nullptr;

			if ( newest_rec && abs( newest_rec->m_tick - ang_diff_rec->m_tick ) <= SIMILAR_RECORD_THRESHOLD )
				newest_rec = nullptr;

			if ( oldest_rec && abs( oldest_rec->m_tick - ang_diff_rec->m_tick ) <= SIMILAR_RECORD_THRESHOLD )
				oldest_rec = nullptr;

			best_recs.push_back( *ang_diff_rec );
		}

		if ( speed_rec ) {
			if ( newest_rec && abs( newest_rec->m_tick - speed_rec->m_tick ) <= SIMILAR_RECORD_THRESHOLD )
				newest_rec = nullptr;

			if ( oldest_rec && abs( oldest_rec->m_tick - speed_rec->m_tick ) <= SIMILAR_RECORD_THRESHOLD )
				oldest_rec = nullptr;

			best_recs.push_back( *speed_rec );
		}

		//
		if ( newest_rec != oldest_rec && newest_rec )
			best_recs.push_back( *newest_rec );
		//
		if ( oldest_rec )
			best_recs.push_back( *oldest_rec );
		//
		if ( simulated_rec )
			best_recs.push_back( *simulated_rec );
	}

	/* manually fix annoying airstuckers */
	if ( abs( cs::i::globals->m_curtime - anims::players::anim_times [ ent->idx ( ) ] ) >= 0.5f ) {
		lagcomp::lag_record_t rec;

		if ( rec.store ( ent, ent->origin ( ), false ) ) {
			rec.m_tick = g::ucmd->m_tickcount;
			rec.m_simtime = cs::ticks2time ( g::ucmd->m_tickcount );

			best_recs.clear ( );
			best_recs.push_back ( rec );
		}
	}

	if ( best_recs.empty( ) )
		return;

	/* scan for a good shot in one of the records we have */
	for ( auto& record : best_recs ) {
		if ( hitscan( record, pos_out, hitbox_out, best_dmg ) ) {
			rec_out = record;
			break;
		}
	}
}

#pragma optimize( "2", off )