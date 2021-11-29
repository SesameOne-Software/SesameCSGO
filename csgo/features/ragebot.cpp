#include "ragebot.hpp"
#include <deque>

#include "autowall.hpp"
#include "autowall_skeet.hpp"

#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "prediction.hpp"
#include "../animations/resolver.hpp"
#include "../menu/options.hpp"
#include "../animations/anims.hpp"

#include "exploits.hpp"

#undef min
#undef max

int target_idx = 0;
std::array< features::ragebot::misses_t, 65 > misses { 0 };
std::array< int, 65 > hits { 0 };
std::array< int, 65 > shots { 0 };
features::ragebot::weapon_config_t features::ragebot::active_config;

features::ragebot::c_scan_points features::ragebot::scan_points;

void features::ragebot::get_weapon_config( weapon_config_t& const config ) {
	if ( !g::local || !g::local->alive ( ) || !g::local->weapon ( ) || !g::local->weapon ( )->data ( ) )
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
	static auto& dt_teleport = options::vars [ _ ( "ragebot.dt_teleport" ) ].val.b;
	static auto& dt_enabled = options::vars [ _ ( "ragebot.dt_enabled" ) ].val.b;
	static auto& dt_ticks = options::vars [ _ ( "ragebot.dt_ticks" ) ].val.i;

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

	config.dt_teleport = dt_teleport;
	config.dt_enabled = dt_enabled;
	config.max_dt_ticks = dt_ticks;

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
		static auto& auto_shoot = options::vars [ _( "ragebot.revolver.auto_shoot" ) ].val.b;
		static auto& auto_slow = options::vars [ _( "ragebot.revolver.auto_slow" ) ].val.b;
		static auto& auto_scope = options::vars [ _( "ragebot.revolver.auto_scope" ) ].val.b;
		static auto& hit_chance = options::vars [ _( "ragebot.revolver.hit_chance" ) ].val.f;
		static auto& dt_hit_chance = options::vars [ _( "ragebot.revolver.dt_hit_chance" ) ].val.f;
		static auto& headshot_only = options::vars [ _( "ragebot.revolver.headshot_only" ) ].val.b;
		static auto& onshot_only = options::vars [ _ ( "ragebot.revolver.onshot_only" ) ].val.b;
		static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.revolver.dt_recharge_delay" ) ].val.i;
		static auto& dt_smooth_recharge = options::vars [ _ ( "ragebot.revolver.dt_smooth_recharge" ) ].val.b;
		static auto& min_dmg_override = options::vars [ _ ( "ragebot.revolver.min_dmg_override" ) ].val.f;

		config.min_dmg_override = min_dmg_override;
		config.dt_smooth_recharge = dt_smooth_recharge;
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
		config.auto_scope = auto_scope;
		config.auto_slow = auto_slow;
		config.auto_shoot = auto_shoot;
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
		static auto& auto_shoot = options::vars [ _( "ragebot.pistol.auto_shoot" ) ].val.b;
		static auto& auto_slow = options::vars [ _( "ragebot.pistol.auto_slow" ) ].val.b;
		static auto& auto_scope = options::vars [ _( "ragebot.pistol.auto_scope" ) ].val.b;
		static auto& hit_chance = options::vars [ _( "ragebot.pistol.hit_chance" ) ].val.f;
		static auto& dt_hit_chance = options::vars [ _( "ragebot.pistol.dt_hit_chance" ) ].val.f;
		static auto& headshot_only = options::vars [ _( "ragebot.pistol.headshot_only" ) ].val.b;
		static auto& onshot_only = options::vars [ _ ( "ragebot.pistol.onshot_only" ) ].val.b;
		static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.pistol.dt_recharge_delay" ) ].val.i;
		static auto& dt_smooth_recharge = options::vars [ _ ( "ragebot.pistol.dt_smooth_recharge" ) ].val.b;
		static auto& min_dmg_override = options::vars [ _ ( "ragebot.pistol.min_dmg_override" ) ].val.f;
		
		config.min_dmg_override = min_dmg_override;
		config.dt_smooth_recharge = dt_smooth_recharge;
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
		config.auto_scope = auto_scope;
		config.auto_slow = auto_slow;
		config.auto_shoot = auto_shoot;
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
		static auto& auto_shoot = options::vars [ _( "ragebot.rifle.auto_shoot" ) ].val.b;
		static auto& auto_slow = options::vars [ _( "ragebot.rifle.auto_slow" ) ].val.b;
		static auto& auto_scope = options::vars [ _( "ragebot.rifle.auto_scope" ) ].val.b;
		static auto& hit_chance = options::vars [ _( "ragebot.rifle.hit_chance" ) ].val.f;
		static auto& dt_hit_chance = options::vars [ _( "ragebot.rifle.dt_hit_chance" ) ].val.f;
		static auto& headshot_only = options::vars [ _( "ragebot.rifle.headshot_only" ) ].val.b;
		static auto& onshot_only = options::vars [ _ ( "ragebot.rifle.onshot_only" ) ].val.b;
		static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.rifle.dt_recharge_delay" ) ].val.i;
		static auto& dt_smooth_recharge = options::vars [ _ ( "ragebot.rifle.dt_smooth_recharge" ) ].val.b;
		static auto& min_dmg_override = options::vars [ _ ( "ragebot.rifle.min_dmg_override" ) ].val.f;

		config.min_dmg_override = min_dmg_override;
		config.dt_smooth_recharge = dt_smooth_recharge;
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
		config.auto_scope = auto_scope;
		config.auto_slow = auto_slow;
		config.auto_shoot = auto_shoot;
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
			static auto& auto_shoot = options::vars [ _( "ragebot.awp.auto_shoot" ) ].val.b;
			static auto& auto_slow = options::vars [ _( "ragebot.awp.auto_slow" ) ].val.b;
			static auto& auto_scope = options::vars [ _( "ragebot.awp.auto_scope" ) ].val.b;
			static auto& hit_chance = options::vars [ _( "ragebot.awp.hit_chance" ) ].val.f;
			static auto& dt_hit_chance = options::vars [ _( "ragebot.awp.dt_hit_chance" ) ].val.f;
			static auto& headshot_only = options::vars [ _( "ragebot.awp.headshot_only" ) ].val.b;
			static auto& onshot_only = options::vars [ _ ( "ragebot.awp.onshot_only" ) ].val.b;
			static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.awp.dt_recharge_delay" ) ].val.i;
			static auto& dt_smooth_recharge = options::vars [ _ ( "ragebot.awp.dt_smooth_recharge" ) ].val.b;
			static auto& min_dmg_override = options::vars [ _ ( "ragebot.awp.min_dmg_override" ) ].val.f;

			config.min_dmg_override = min_dmg_override;
			config.dt_smooth_recharge = dt_smooth_recharge;
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
			config.auto_scope = auto_scope;
			config.auto_slow = auto_slow;
			config.auto_shoot = auto_shoot;
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
			static auto& auto_shoot = options::vars [ _( "ragebot.auto.auto_shoot" ) ].val.b;
			static auto& auto_slow = options::vars [ _( "ragebot.auto.auto_slow" ) ].val.b;
			static auto& auto_scope = options::vars [ _( "ragebot.auto.auto_scope" ) ].val.b;
			static auto& hit_chance = options::vars [ _( "ragebot.auto.hit_chance" ) ].val.f;
			static auto& dt_hit_chance = options::vars [ _( "ragebot.auto.dt_hit_chance" ) ].val.f;
			static auto& headshot_only = options::vars [ _( "ragebot.auto.headshot_only" ) ].val.b;
			static auto& onshot_only = options::vars [ _ ( "ragebot.auto.onshot_only" ) ].val.b;
			static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.auto.dt_recharge_delay" ) ].val.i;
			static auto& dt_smooth_recharge = options::vars [ _ ( "ragebot.auto.dt_smooth_recharge" ) ].val.b;
			static auto& min_dmg_override = options::vars [ _ ( "ragebot.auto.min_dmg_override" ) ].val.f;

			config.min_dmg_override = min_dmg_override;
			config.dt_smooth_recharge = dt_smooth_recharge;
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
			config.auto_scope = auto_scope;
			config.auto_slow = auto_slow;
			config.auto_shoot = auto_shoot;
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
			static auto& auto_shoot = options::vars [ _( "ragebot.scout.auto_shoot" ) ].val.b;
			static auto& auto_slow = options::vars [ _( "ragebot.scout.auto_slow" ) ].val.b;
			static auto& auto_scope = options::vars [ _( "ragebot.scout.auto_scope" ) ].val.b;
			static auto& hit_chance = options::vars [ _( "ragebot.scout.hit_chance" ) ].val.f;
			static auto& dt_hit_chance = options::vars [ _( "ragebot.scout.dt_hit_chance" ) ].val.f;
			static auto& headshot_only = options::vars [ _( "ragebot.scout.headshot_only" ) ].val.b;
			static auto& onshot_only = options::vars [ _ ( "ragebot.scout.onshot_only" ) ].val.b;
			static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.scout.dt_recharge_delay" ) ].val.i;
			static auto& dt_smooth_recharge = options::vars [ _ ( "ragebot.scout.dt_smooth_recharge" ) ].val.b;
			static auto& min_dmg_override = options::vars [ _ ( "ragebot.scout.min_dmg_override" ) ].val.f;

			config.min_dmg_override = min_dmg_override;
			config.dt_smooth_recharge = dt_smooth_recharge;
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
			config.auto_scope = auto_scope;
			config.auto_slow = auto_slow;
			config.auto_shoot = auto_shoot;
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
	static auto& auto_shoot = options::vars [ _( "ragebot.default.auto_shoot" ) ].val.b;
	static auto& auto_slow = options::vars [ _( "ragebot.default.auto_slow" ) ].val.b;
	static auto& auto_scope = options::vars [ _( "ragebot.default.auto_scope" ) ].val.b;
	static auto& hit_chance = options::vars [ _( "ragebot.default.hit_chance" ) ].val.f;
	static auto& dt_hit_chance = options::vars [ _( "ragebot.default.dt_hit_chance" ) ].val.f;
	static auto& headshot_only = options::vars [ _( "ragebot.default.headshot_only" ) ].val.b;
	static auto& onshot_only = options::vars [ _ ( "ragebot.default.onshot_only" ) ].val.b;
	static auto& dt_recharge_delay = options::vars [ _ ( "ragebot.default.dt_recharge_delay" ) ].val.i;
	static auto& dt_smooth_recharge = options::vars [ _ ( "ragebot.default.dt_smooth_recharge" ) ].val.b;
	static auto& min_dmg_override = options::vars [ _ ( "ragebot.default.min_dmg_override" ) ].val.f;

	config.min_dmg_override = min_dmg_override;
	config.dt_smooth_recharge = dt_smooth_recharge;
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
	config.auto_scope = auto_scope;
	config.auto_slow = auto_slow;
	config.auto_shoot = auto_shoot;
	config.auto_revolver = auto_revolver;
	config.hit_chance = hit_chance;
	config.dt_hit_chance = dt_hit_chance;
	config.onshot_only = onshot_only;
}

int& features::ragebot::get_target_idx( ) {
	return target_idx;
}

features::ragebot::misses_t& features::ragebot::get_misses( int pl ) {
	return misses [ pl ];
}

int& features::ragebot::get_hits( int pl ) {
	return hits [ pl ];
}

bool features::ragebot::hitchance( vec3_t ang, player_t* pl, vec3_t point, int rays, hitbox_t hitbox, anims::anim_info_t& rec, float& hc_out ) {
	static auto& min_dmg_override_key = options::vars [ _ ( "ragebot.min_dmg_override_key" ) ].val.i;
	static auto& min_dmg_override_key_mode = options::vars [ _ ( "ragebot.min_dmg_override_key_mode" ) ].val.i;
	const auto min_dmg = utils::keybind_active ( min_dmg_override_key, min_dmg_override_key_mode ) ? active_config.min_dmg_override : active_config.min_dmg;

	if ( g::cvars::weapon_accuracy_nospread->get_bool ( ) ) {
		hc_out = -1.0f;
		return true;
	}

	auto weapon = g::local->weapon( );

	if ( !weapon || !weapon->data ( ) ) {
		hc_out = -1.0f;
		return false;
	}

	auto weapon_data = weapon->data ( );
	auto weapon_id = weapon->item_definition_index ( );

	const auto round_acc = [ ] ( const float accuracy ) { return roundf ( accuracy * 1000.0f ) / 1000.0f; };
	const auto sniper = weapon_id == weapons_t::awp || weapon_id == weapons_t::g3sg1 || weapon_id == weapons_t::scar20 || weapon_id == weapons_t::ssg08;
	const auto crouched = !!( g::local->flags ( ) & flags_t::ducking );

	// calculate inaccuracy.
	const auto weapon_inaccuracy = weapon->inaccuracy ( );
	
	if ( weapon_id == weapons_t::revolver ) {
		if ( weapon_inaccuracy < ( crouched ? 0.0020f : 0.0055f ) ) {
			hc_out = -1.0f;
			return true;
		}
	}
	
	const auto zoomed = g::local->scoped ( );
	
	// no need for hitchance, if we can't increase it anyway.
	if ( crouched ) {
		if ( round_acc ( weapon_inaccuracy ) == round_acc ( ( sniper && zoomed ) ? weapon_data->m_inaccuracy_crouch_alt : weapon_data->m_inaccuracy_crouch ) ) {
			hc_out = -1.0f;
			return true;
		}
	}
	else {
		if ( round_acc ( weapon_inaccuracy ) == round_acc ( ( sniper && zoomed ) ? weapon_data->m_inaccuracy_stand_alt : weapon_data->m_inaccuracy_stand ) ) {
			hc_out = -1.0f;
			return true;
		}
	}

	auto src = g::local->eyes( );

	ang = cs::calc_angle( src, point );
	cs::clamp( ang );

	auto forward = cs::angle_vec( ang );
	auto right = cs::angle_vec( ang + vec3_t( 0.0f, 90.0f, 0.0f ) );
	auto up = cs::angle_vec( ang + vec3_t( 90.0f, 0.0f, 0.0f ) );

	forward.normalize( );
	right.normalize( );
	up.normalize( );

	if ( !forward.is_valid( ) || !right.is_valid( ) || !up.is_valid( ) ) {
		hc_out = -1.0f;
		return false;
	}

	auto hits = 0;
	auto needed_hits = static_cast< int >( static_cast< float > ( rays ) * ( features::ragebot::active_config.hit_chance / 100.0f ) );

	const auto innacuracy = weapon->inaccuracy ( );
	const auto spread = weapon->spread ( );

	std::array< matrix3x4_t, 128>& bone_matrix = rec.m_aim_bones [ rec.m_side ];

	static bool rand_table = false;
	static std::array< std::array<float, 6>, 256> rand_components { };

	if ( !rand_table ) {
		for ( auto& comp : rand_components ) {
			auto a = std::lerp ( 0.0f, 1.0f, static_cast < float > ( rand ( ) ) / static_cast < float > ( RAND_MAX ) );
			auto b = std::lerp ( 0.0f, 2.0f * cs::pi, static_cast < float > ( rand ( ) ) / static_cast < float > ( RAND_MAX ) );
			auto c = std::lerp ( 0.0f, 1.0f, static_cast < float > ( rand ( ) ) / static_cast < float > ( RAND_MAX ) );
			auto d = std::lerp ( 0.0f, 2.0f * cs::pi, static_cast < float > ( rand ( ) ) / static_cast < float > ( RAND_MAX ) );

			comp [ 0 ] = a;
			comp [ 1 ] = c;

			auto sin_b = 0.0f, cos_b = 0.0f;
			cs::sin_cos ( b, &sin_b, &cos_b );

			auto sin_d = 0.0f, cos_d = 0.0f;
			cs::sin_cos ( d, &sin_d, &cos_d );

			comp [ 2 ] = sin_b;
			comp [ 3 ] = cos_b;
			comp [ 4 ] = sin_d;
			comp [ 5 ] = cos_d;
		}

		rand_table = true;
	}

	auto studio_mdl = cs::i::mdl_info->studio_mdl ( pl->mdl ( ) );

	if ( !studio_mdl ) {
		hc_out = -1.0f;
		return false;
	}

	auto set = studio_mdl->hitbox_set ( 0 );

	if ( !set ) {
		hc_out = -1.0f;
		return false;
	}

	auto hhitbox = set->hitbox ( static_cast<int>( hitbox ) );

	if ( !hhitbox ) {
		hc_out = -1.0f;
		return false;
	}

	vec3_t vmin, vmax;
	VEC_TRANSFORM ( hhitbox->m_bbmin, bone_matrix [ hhitbox->m_bone ], vmin );
	VEC_TRANSFORM ( hhitbox->m_bbmax, bone_matrix [ hhitbox->m_bone ], vmax );

	/* normal hitchance */
	const auto weapon_range = weapon_data->m_range;
	const auto hitbox_as_hitgroup = autowall::hitbox_to_hitgroup ( hitbox );

	for ( auto i = 0; i < rays; i++ ) {
		auto inaccuracy_scale = rand_components [ i ][ 0 ] * innacuracy;
		auto spread_scale = rand_components [ i ][ 1 ] * spread;

		const auto spread_dir = vec3_t (
			rand_components [ i ][ 3 ] * inaccuracy_scale + rand_components [ i ][ 5 ] * spread_scale,
			rand_components [ i ][ 2 ] * inaccuracy_scale + rand_components [ i ][ 4 ] * spread_scale, 
			0.0f
		);
		
		const auto direction = vec3_t (
			forward.x + right.x * spread_dir.x + up.x * spread_dir.y,
			forward.y + right.y * spread_dir.x + up.y * spread_dir.y,
			forward.z + right.z * spread_dir.x + up.z * spread_dir.y
		);

		trace_t tr;
		ray_t ray;

		ray.init ( src, src + direction * weapon_range );
		cs::i::trace->clip_ray_to_entity ( ray, mask_shot | contents_grate, pl, &tr );

		if ( tr.m_hit_entity == pl && tr.m_hitbox == static_cast<int>( hitbox ) )
			hits++;
	}

	hc_out = static_cast< float >( hits ) / static_cast< float > ( rays ) * 100.0f;

	if ( hc_out < ( ( exploits::has_shifted || exploits::in_exploit ) ? features::ragebot::active_config.dt_hit_chance : features::ragebot::active_config.hit_chance ) )
		return false;

	return true;
}

void features::ragebot::tickbase_controller( ucmd_t* ucmd ) {
	/* tickbase manip controller */
	static auto& fd_enabled = options::vars [ _( "antiaim.fakeduck" ) ].val.b;
	static auto& fd_key = options::vars [ _( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _( "antiaim.fakeduck_key_mode" ) ].val.i;

	const auto weapon_data = ( g::local && g::local->weapon( ) && g::local->weapon( )->data( ) ) ? g::local->weapon( )->data( ) : nullptr;
	auto tickbase_as_int = std::clamp<int>( static_cast< int >( active_config.max_dt_ticks ), 0, g::cvars::sv_maxusrcmdprocessticks->get_int ( ) - 1 );

	if ( !active_config.dt_enabled || !utils::keybind_active( active_config.dt_key, active_config.dt_key_mode ) )
		tickbase_as_int = 0;

	if ( g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) && tickbase_as_int && !!( ucmd->m_buttons & buttons_t::attack ) && exploits::can_shoot ( ) && g::local->weapon ( )->data ( )->m_type < weapon_type_t::c4 && !( fd_enabled && utils::keybind_active ( fd_key, fd_key_mode ) ) ) {
		exploits::shift_tickbase ( tickbase_as_int, cs::time2ticks ( static_cast< float >( active_config.dt_recharge_delay ) / 1000.0f ) );
		exploits::will_shift = true;
	}
}

void features::ragebot::select_targets( std::deque < aim_target_t >& targets_out ) {
	vec3_t engine_angles;
	cs::i::engine->get_viewangles( engine_angles );

	cs::clamp( engine_angles );

	cs::for_each_player( [ & ] ( player_t* ent ) {
		if ( !g::local->is_enemy ( ent ) || ent->immune( ) )
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
	} );

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

float features::ragebot::skeet_accelerate_rebuilt ( ucmd_t* cmd, player_t* player, const vec3_t& wish_dir, const vec3_t& wish_speed, bool& ducking ) {
	VMP_BEGINMUTATION ( );
	auto wanted_vel = wish_dir * wish_speed;
	auto wanted_speed = wanted_vel.length_2d ( );

	auto weapon = player->weapon ( );
	auto duck_amount = player->crouch_amount ( );
	auto flags_check = player->flags ( );

	ducking = false;

	if ( !( cmd->m_buttons & buttons_t::duck ) ) {
		auto is_ducking = *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( &player->fall_vel ( ) ) + 0x31 );

		if ( duck_amount > 0.0 )
			is_ducking = true;

		if ( is_ducking || duck_amount >= 1.0f ) {
			auto duck_speed = 2.0f;

			if ( player->crouch_speed ( ) >= 1.5f )
				duck_speed = player->crouch_speed ( );

			auto next_duck = std::max ( 0.0f, duck_amount - ( duck_speed * cs::ticks2time ( 1 ) ) );
			auto backup_next_duck = next_duck;

			if ( next_duck <= 0.0f )
				next_duck = 0.0f;

			auto new_flags = flags_check & ~flags_t::ducking;

			if ( backup_next_duck > 0.0f )
				new_flags = player->flags ( );

			flags_check = new_flags & ~flags_t::ducking;

			if ( next_duck > 0.75f )
				flags_check = new_flags;

			auto new_is_ducking = false;

			if ( backup_next_duck > 0.0f )
				new_is_ducking = is_ducking;

			if ( new_is_ducking )
				ducking = true;
		}

		if ( !!( flags_check & flags_t::ducking ) )
			ducking = true;
	}

	auto is_walking = !!( cmd->m_buttons & buttons_t::speed ) && !ducking;

	auto max_speed = 250.0f;
	auto acceleration_scale = std::max ( max_speed, wanted_speed );
	auto goal_speed = acceleration_scale;

	static auto sv_accelerate_use_weapon_speed = cs::i::cvar->find ( _ ( "sv_accelerate_use_weapon_speed" ) );

	auto slowed_by_scope = false;

	if ( sv_accelerate_use_weapon_speed->get_bool ( ) && weapon ) {
		auto max_speed = 260.0f;
		auto zoom_levels = 0;
		const auto weapon = g::local->weapon ( );

		if ( weapon ) {
			if ( auto data = weapon->data ( ) )
				max_speed = g::local->scoped ( ) ? data->m_max_speed_alt : data->m_max_speed;
			zoom_levels = *reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( weapon ) + 0x33E0 );
		}

		slowed_by_scope = ( zoom_levels > 0 && zoom_levels > 1 && ( max_speed * 0.52f ) < 110.0f );
		goal_speed *= std::min ( 1.0f, ( max_speed / max_speed ) );

		if ( ( !ducking && !is_walking ) || ( ( is_walking || ducking ) && slowed_by_scope ) )
			acceleration_scale *= std::min ( 1.0f, ( max_speed / max_speed ) );
	}

	if ( ducking ) {
		if ( !slowed_by_scope )
			acceleration_scale *= 0.34f;

		goal_speed *= 0.34f;
	}

	if ( is_walking ) {
		if ( !player->has_heavy_armor ( ) && !slowed_by_scope )
			acceleration_scale *= 0.52f;

		goal_speed *= 0.52f;
	}

	auto accel = g::cvars::sv_accelerate->get_float ( );

	if ( is_walking && wanted_speed > ( goal_speed - 5.0f ) )
		accel *= std::clamp ( 1.0f - ( std::max ( 0.0f, wanted_speed - ( goal_speed - 5.0f ) ) / std::max ( 0.0f, goal_speed - ( goal_speed - 5.0f ) ) ), 0.0f, 1.0f );

	return cs::ticks2time ( 1 ) * accel * acceleration_scale;
	VMP_END ( );
};

void features::ragebot::skeet_slow ( ucmd_t* cmd, float wanted_speed, vec3_t& old_angs ) {
	VMP_BEGINMUTATION ( );
	static auto deployable_limited_max_speed = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 EC 0C 56 8B F1 80 BE ? ? ? ? ? 75" ) ).get<float ( __thiscall* )( player_t* )> ( );

	if ( !g::local || !g::local->alive ( ) )
		return;

	const auto weapon = g::local->weapon ( );
	const auto vel = g::local->vel ( );
	const auto speed2d = vel.length_2d ( );
	const auto move_vec = vec3_t ( cmd->m_fmove, cmd->m_smove, cmd->m_umove );

	if ( !( cmd->m_buttons & buttons_t::jump ) && move_vec.length ( ) >= 38.0f ) {
		if ( speed2d >= 28.0f ) {
			const auto stop_speed = std::max ( g::cvars::sv_stopspeed->get_float ( ), speed2d );
			const auto friction = g::cvars::sv_friction->get_float ( );

			const auto max_stop_speed = std::max ( speed2d - ( ( stop_speed * friction ) * cs::ticks2time ( 1 ) ), 0.0f );

			auto max_stop_speed_vec = vel * ( max_stop_speed / speed2d );
			max_stop_speed_vec.z = 0.0f;

			auto delta_target_speed = wanted_speed - max_stop_speed;

			if ( abs( delta_target_speed ) >= 0.5f ) {
				vec3_t move_dir;

				vec3_t angles;
				cs::i::engine->get_viewangles ( angles );

				vec3_t fwd, right, up;
				cs::angle_vec ( angles, &fwd, &right, nullptr );

				fwd.normalize ( );
				right.normalize ( );

				if ( delta_target_speed >= 0.0f ) {
					cmd->m_buttons &= ~buttons_t::speed;

					move_dir.x = ( cmd->m_fmove * fwd.x ) + ( cmd->m_smove * right.x );
					move_dir.y = ( cmd->m_smove * right.y ) + ( cmd->m_fmove * fwd.y );
					move_dir.z = 0.0f;
					
					move_dir.normalize ( );
				}
				else {
					move_dir = vel.normalized ( );
					move_dir.z = 0.0f;

					cmd->m_fmove = 450.0f;
					cmd->m_smove = 0.0f;

					old_angs.y = cs::normalize( cs::vec_angle ( move_dir ).y + 180.0f );
					old_angs.x = old_angs.z = 0.0f;

					delta_target_speed *= -1.0f;
				}

				auto ducking = false;
				const auto accel = skeet_accelerate_rebuilt ( cmd, g::local, move_dir, max_stop_speed_vec, ducking );

				const auto currentspeed = max_stop_speed_vec.dot_product ( move_dir );
				const auto addspeed = std::clamp ( std::min( g::local->max_speed( ), deployable_limited_max_speed ( g::local ) ) - currentspeed, 0.0f, accel );

				if ( addspeed > delta_target_speed + 0.5f ) {
					auto move_length = currentspeed + delta_target_speed;

					if ( ducking ) {
						if ( g::local->crouch_amount ( ) == 1.0f )
							move_length /= 0.34f;
					}

					const auto move_scale = move_length / move_vec.length ( );
					
					cmd->m_fmove = cmd->m_fmove * move_scale;
					cmd->m_smove = cmd->m_smove * move_scale;
					cmd->m_umove = cmd->m_umove * move_scale;
				}
			}
			else {
				cmd->m_smove = 0.0f;
				cmd->m_fmove = 0.0f;
			}
		}
	}

	VMP_END ( );
}

float features::ragebot::get_scaled_min_dmg ( player_t* ent ) {
	static auto& min_dmg_override_key = options::vars [ _ ( "ragebot.min_dmg_override_key" ) ].val.i;
	static auto& min_dmg_override_key_mode = options::vars [ _ ( "ragebot.min_dmg_override_key_mode" ) ].val.i;

	const auto min_dmg = utils::keybind_active ( min_dmg_override_key, min_dmg_override_key_mode ) ? active_config.min_dmg_override : active_config.min_dmg;
	const auto new_min = static_cast< float >( min_dmg ) > 100.0f ? ( ent ? static_cast< float >( ent->health ( ) + ( min_dmg - 100.0f ) ) : min_dmg ) : std::min<float> ( ent->health ( ), static_cast< float >( min_dmg ) );

	return std::max<float> ( 1.0f, new_min );
}

/* ty cbrs */
void features::ragebot::slow ( ucmd_t* ucmd ) {
	VMP_BEGINMUTATION ( );
	if ( !active_config.auto_slow || !g::local || !g::local->alive( ) || !( g::local->flags ( ) & flags_t::on_ground ) || !g::local->weapon ( ) || !g::local->weapon ( )->data ( ) )
		return;

	auto quick_stop = [ & ] ( ) {
		const auto target_vel = -g::local->vel ( ).normalized( ) * g::cvars::cl_forwardspeed->get_float ( );

		vec3_t angles;
		cs::i::engine->get_viewangles ( angles );

		const auto fwd = cs::angle_vec ( angles );
		const auto right = fwd.cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );

		ucmd->m_fmove = ( target_vel.y - ( right.y / right.x ) * target_vel.x ) / ( fwd.y - ( right.y / right.x ) * fwd.x );
		ucmd->m_smove = ( target_vel.x - fwd.x * ucmd->m_fmove ) / right.x;

		ucmd->m_fmove = std::clamp <float> ( ucmd->m_fmove, -g::cvars::cl_forwardspeed->get_float ( ), g::cvars::cl_forwardspeed->get_float ( ) );
		ucmd->m_smove = std::clamp <float> ( ucmd->m_smove, -g::cvars::cl_sidespeed->get_float ( ), g::cvars::cl_sidespeed->get_float ( ) );

		ucmd->m_buttons &= ~buttons_t::walk;
		ucmd->m_buttons |= buttons_t::speed;
	};

	const auto speed = g::local->vel ( ).length_2d ( );

	//    we are slow
	if ( speed <= 5.0f )
		return;

	auto max_speed = 260.0f;
	const auto weapon = g::local->weapon ( );

	if ( weapon && weapon->data ( ) )
		max_speed = g::local->scoped ( ) ? weapon->data ( )->m_max_speed_alt : weapon->data ( )->m_max_speed;

	const auto pure_accurate_speed = max_speed * 0.34f;
	const auto accurate_speed = max_speed * 0.315f;

	//    actually slowwalk
	if ( speed <= pure_accurate_speed ) {
		const auto cmd_speed = sqrt( ucmd->m_fmove * ucmd->m_fmove + ucmd->m_smove * ucmd->m_smove );
		const auto local_speed = std::max ( g::local->vel ( ).length_2d ( ), 0.1f );
		const auto speed_multiplier = ( local_speed / cmd_speed ) * ( accurate_speed / local_speed );

		ucmd->m_fmove = std::clamp <float> ( ucmd->m_fmove * speed_multiplier, -g::cvars::cl_forwardspeed->get_float ( ), g::cvars::cl_forwardspeed->get_float ( ) );
		ucmd->m_smove = std::clamp <float> ( ucmd->m_smove * speed_multiplier, -g::cvars::cl_sidespeed->get_float ( ), g::cvars::cl_sidespeed->get_float ( ) );

		ucmd->m_buttons &= ~buttons_t::walk;
		ucmd->m_buttons |= buttons_t::speed;
	}
	//    we are fast
	else {
		quick_stop ( );
	}
	VMP_END ( );
}

void features::ragebot::run_meleebot ( ucmd_t* ucmd ) {
	VMP_BEGINMUTATION ( );
	if ( g::local->weapon ( )->item_definition_index ( ) == weapons_t::taser && !active_config.zeus_bot )
		return;
	else if ( g::local->weapon ( )->data ( )->m_type == weapon_type_t::knife && !active_config.knife_bot )
		return;

	vec3_t engine_ang;
	cs::i::engine->get_viewangles ( engine_ang );

	anims::anim_info_t* best_rec = nullptr;
	player_t* best_pl = nullptr;
	float best_fov = 180.0f;
	float best_dist = FLT_MAX;
	vec3_t best_point, best_ang;

	cs::for_each_player ( [ & ] ( player_t* pl ) {
		if ( !g::local->is_enemy ( pl ) || pl->immune ( ) )
			return;

		const auto recs = anims::get_lagcomp_records ( pl );
		const auto simulated_rec = active_config.fix_fakelag ? anims::get_simulated_record ( pl ) : std::nullopt;

		if ( recs.empty ( ) && !simulated_rec )
			return;

		anims::anim_info_t* rec = simulated_rec ? simulated_rec.value ( ) : recs.front ( );

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

		const std::array< matrix3x4_t, 128>& bone_matrix = rec->m_aim_bones[ rec->m_side ];

		auto vmin = hitbox->m_bbmin;
		auto vmax = hitbox->m_bbmax;

		VEC_TRANSFORM ( hitbox->m_bbmin, bone_matrix [ hitbox->m_bone ], vmin );
		VEC_TRANSFORM ( hitbox->m_bbmax, bone_matrix [ hitbox->m_bone ], vmax );

		const auto hitbox_pos = ( vmin + vmax ) * 0.5f;

		auto ang = cs::calc_angle ( g::local->eyes ( ), hitbox_pos );
		cs::clamp ( ang );

		const auto fov = cs::calc_fov ( ang, engine_ang );

		auto can_use = false;

		if ( g::local->weapon ( )->item_definition_index ( ) == weapons_t::taser ) {
			best_dist = g::local->eyes ( ).dist_to ( hitbox_pos );
			can_use = best_dist < 150.0f;
		}
		else {
			best_dist = g::local->eyes ( ).dist_to ( pl->eyes( ) );
			can_use = best_dist < 65.0f;
		}

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

	if ( features::ragebot::active_config.auto_shoot && g::local->weapon ( )->item_definition_index ( ) != weapons_t::taser ) {
		ucmd->m_angs = best_ang;
		ucmd->m_tickcount = cs::time2ticks ( best_rec->m_simtime + anims::lerp_time ( ) );

		if ( !features::ragebot::active_config.silent )
			cs::i::engine->set_viewangles ( best_ang );

		shots.push_back ( shot_t {
			best_pl->idx ( ),
			*best_rec,
			g::local->tick_base ( ) - cs::time2ticks ( best_rec->m_simtime ),
			hitbox_t::pelvis,
			g::local->eyes ( ),
			best_point ,
			-1.0f,
			cs::normalize ( cs::normalize ( best_rec->m_anim_state [ best_rec->m_side ].m_abs_yaw ) - cs::normalize ( best_rec->m_anim_state [ best_rec->m_side ].m_eye_yaw ) ),
			-1,
			g::local->velocity_modifier( ),
			vec3_t ( ),
			0, 0, 0, false, false, 0
		} );

		get_target_idx ( ) = best_pl->idx ( );

		if ( best_pl->health ( ) <= 35 && best_dist < 65.0f ) {
			ucmd->m_buttons |= buttons_t::attack;
		}
		else {
			if ( abs ( cs::normalize ( cs::normalize ( best_rec->m_angles.y ) - ucmd->m_angs.y ) ) < 35.0f ) {
				if ( best_dist < 50.0f ) {
					ucmd->m_buttons |= buttons_t::attack2;
					exploits::extend_recharge_delay ( cs::time2ticks ( static_cast< float >( active_config.dt_recharge_delay ) / 1000.0f ) );
				}
			}
			else {
				if ( best_dist < 50.0f ) {
					ucmd->m_buttons |= buttons_t::attack2;
					exploits::extend_recharge_delay ( cs::time2ticks ( static_cast< float >( active_config.dt_recharge_delay ) / 1000.0f ) );
				}
				else if ( best_dist < 65.0f )
					ucmd->m_buttons |= buttons_t::attack;
			}
		}
	}
	else if ( features::ragebot::active_config.auto_shoot ) {
		ucmd->m_buttons |= buttons_t::attack;
		exploits::extend_recharge_delay ( cs::time2ticks ( static_cast< float >( active_config.dt_recharge_delay ) / 1000.0f ) );

		ucmd->m_angs = best_ang;
		ucmd->m_tickcount = cs::time2ticks ( best_rec->m_simtime + anims::lerp_time ( ) );

		if ( !features::ragebot::active_config.silent )
			cs::i::engine->set_viewangles ( best_ang );

		shots.push_back ( shot_t {
			best_pl->idx ( ),
			*best_rec,
			g::local->tick_base ( ) - cs::time2ticks ( best_rec->m_simtime ),
			hitbox_t::pelvis,
			g::local->eyes ( ),
			best_point ,
			-1.0f,
			cs::normalize ( cs::normalize ( best_rec->m_anim_state [ best_rec->m_side ].m_abs_yaw ) - cs::normalize ( best_rec->m_anim_state [ best_rec->m_side ].m_eye_yaw ) ),
			100,
			g::local->velocity_modifier ( ),
			vec3_t ( ),
			0, 0, 0, false, false, 0
			} );

		get_target_idx ( ) = best_pl->idx ( );
	}
	VMP_END ( );
}

void features::ragebot::run ( ucmd_t* ucmd, vec3_t& old_angs ) {
	VMP_BEGINMUTATION ( );
	if ( !active_config.main_switch || !g::local || !g::local->alive ( ) || !g::local->weapon ( ) || !g::local->weapon ( )->data ( ) ) {
		scan_points.clear ( );
		return;
	}

	/* Don't knifebot without an actual knifebot lmao, currently the hack just fucking shoots the air constantly w/a knife */
	if ( g::local->weapon ( ) && ( g::local->weapon ( )->data ( )->m_type == weapon_type_t::knife || g::local->weapon ( )->item_definition_index ( ) == weapons_t::taser ) ) {
		run_meleebot ( ucmd );
		scan_points.clear ( );
		return;
	}

	if ( !exploits::can_shoot ( ) && g::local->weapon ( )->data ( ) && g::local->weapon ( )->data ( )->m_type > weapon_type_t::knife && g::local->weapon ( )->data ( )->m_type < weapon_type_t::c4 ) {
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
		hitbox_t m_hitbox = hitbox_t::invalid;
		anims::anim_info_t m_record;
		float m_dmg = 0.0f;
	} static best;

	memset( &best, 0, sizeof( best ) );

	scan_points.clear( );

	for ( auto& target : targets ) {
		/* create only once, save some space on the stack */
		static anims::anim_info_t record_out;

		auto point = vec3_t( );
		auto hitbox_out = hitbox_t::invalid;
		memset( &record_out, 0, sizeof( record_out ) );
		auto dmg_out = 0.0f;

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

	/* extrapolate autostop */ 
	const auto max_speed = g::local->scoped ( ) ? g::local->weapon ( )->data ( )->m_max_speed_alt : g::local->weapon ( )->data ( )->m_max_speed;
	const auto cur_speed = g::local->vel ( ).length_2d ( );
	auto pre_autostop_working = false;

	const auto stop_to_speed = ( max_speed - 1.0f ) * 0.34f;

	if ( active_config.auto_slow && cur_speed > 0.0f && !!( g::local->flags ( ) & flags_t::on_ground ) ) {
		const auto vel_norm = g::local->vel ( ).normalized ( );
	
		auto speed = cur_speed;
		auto move_dir = -vel_norm;
		auto move_dir_max_speed = move_dir * speed;
		auto predicted_eyes = g::local->eyes ( );
	
		auto ticks_until_slow = 0;
	
		while ( speed > stop_to_speed ) {
			auto ducking = false;
	
			const auto accel = skeet_accelerate_rebuilt ( ucmd, g::local, move_dir, move_dir_max_speed, ducking );
	
			speed -= accel;
			move_dir_max_speed = move_dir * speed;
			predicted_eyes += vel_norm * ( speed * cs::ticks2time ( 1 ) );
	
			ticks_until_slow++;
	
			if ( ticks_until_slow >= 16 )
				break;
		}
	
		player_t* at_target = nullptr;
	
		for ( auto i = 1; i < cs::i::globals->m_max_clients; i++ ) {
			const auto ent = cs::i::ent_list->get<player_t*> ( i );
	
			if ( ent->valid ( ) && !ent->immune() && g::local->is_enemy ( ent ) ) {
				const auto min_dmg = get_scaled_min_dmg ( ent );
	
				const auto pred_ent_pos = ent->origin ( ) + ent->view_offset ( ) + ent->vel ( ) * std::clamp ( ent->simtime ( ) - ent->old_simtime ( ), cs::ticks2time ( 1 ), cs::ticks2time ( 16 ) );
				const auto dmg = awall_skeet::dmg ( g::local, predicted_eyes, pred_ent_pos, g::local->weapon ( ), ent, min_dmg, true );
	
				if ( dmg >= min_dmg ) {
					at_target = ent;
					break;
				}
			}
		}
	
		if ( at_target ) {
			skeet_slow ( ucmd, stop_to_speed, old_angs );
			pre_autostop_working = true;
		}
	}

	if ( !best.m_ent )
		return;
		
	auto angle_to = cs::calc_angle( g::local->eyes( ), best.m_point );
	cs::clamp( angle_to );

	auto hc_out = -1.0f;
	auto hc = hitchance( angle_to, best.m_ent, best.m_point, 256, best.m_hitbox, best.m_record, hc_out );
	auto should_aim = best.m_dmg && hc;
	
	if ( !pre_autostop_working && best.m_dmg )
		skeet_slow ( ucmd, stop_to_speed, old_angs );

	if ( !active_config.auto_shoot )
		should_aim = !!( ucmd->m_buttons & buttons_t::attack ) && best.m_dmg;

	if ( should_aim ) {
		if ( active_config.auto_shoot )
			ucmd->m_buttons |= buttons_t::attack;

		if ( active_config.auto_scope && !g::local->scoped ( ) &&
			( g::local->weapon ( )->item_definition_index ( ) == weapons_t::awp
				|| g::local->weapon ( )->item_definition_index ( ) == weapons_t::scar20
				|| g::local->weapon ( )->item_definition_index ( ) == weapons_t::g3sg1
				|| g::local->weapon ( )->item_definition_index ( ) == weapons_t::ssg08
				|| g::local->weapon ( )->item_definition_index ( ) == weapons_t::aug
				|| g::local->weapon ( )->item_definition_index ( ) == weapons_t::sg553 ) )
			ucmd->m_buttons |= buttons_t::attack2;

		auto ang = angle_to - g::local->aim_punch( ) * g::cvars::weapon_recoil_scale->get_float();
		cs::clamp( ang );

		ucmd->m_angs = ang;

		if ( !active_config.silent )
			cs::i::engine->set_viewangles( ang );

		ucmd->m_tickcount = cs::time2ticks ( best.m_record.m_simtime + anims::lerp_time ( ) );

		shots.push_back ( shot_t {
			best.m_ent->idx ( ),
			best.m_record,
			g::local->tick_base ( ) - cs::time2ticks ( best.m_record.m_simtime ),
			best.m_hitbox,
			g::local->eyes ( ),
			best.m_point ,
			hc_out,
			cs::normalize ( cs::normalize ( best.m_record.m_anim_state [ best.m_record.m_side ].m_abs_yaw ) - cs::normalize ( best.m_record.m_anim_state [ best.m_record.m_side ].m_eye_yaw ) ),
			static_cast<int>( best.m_dmg ),
			g::local->velocity_modifier ( ),
			vec3_t ( ),
			0, 0, 0, false, false, 0
			} );

		get_target_idx( ) = best.m_ent->idx( );
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
	VMP_END ( );
}

bool features::ragebot::create_points( player_t* ent, anims::anim_info_t& rec, hitbox_t i, std::deque< vec3_t >& points, multipoint_side_t multipoint_side ) {
	VMP_BEGINMUTATION ( );
	vec3_t hitbox_pos = vec3_t( 0.0f, 0.0f, 0.0f );
	float hitbox_rad = 0.0f;
	float hitbox_zrad = 0.0f;

	if ( !get_hitbox( ent, rec, i, hitbox_pos, hitbox_rad, hitbox_zrad ) )
		return false;

	multipoint_mode_t multipoint_mask = multipoint_mode_t::none;

	/* enable multipoint for these_hitboxes */
	multipoint_mask |= multipoint_mode_t::center;

	float pointscale = 0.0f;

	if ( i == hitbox_t::head ) {
		/* fix accidental "body aim" by aiming at the higher point only (when needed ?is facing backwards?) */
		if (abs( cs::normalize ( cs::calc_angle ( g::local->eyes ( ), rec.m_origin ).y - cs::normalize ( rec.m_angles.y ) ) ) < 60.0f )
			multipoint_mask &= ~multipoint_mode_t::center;

		multipoint_mask |= multipoint_mode_t::top;
		multipoint_mask |= multipoint_mode_t::left;
		multipoint_mask |= multipoint_mode_t::right;

		pointscale = active_config.head_pointscale;
	}
	else if ( i >= hitbox_t::pelvis && i <= hitbox_t::l_calf ) {
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

	///* dynamic pointscale */
	///* only scale dynamically if player is above us or on our level */
	//if ( cs::calc_angle ( g::local->eyes ( ), ent->eyes ( ) ).x > -5.0f ) {
	//	/* scale according to desync amount (moving also) NOTE: only scale multipoint upwards */
	//	if ( pointscale > 0.0f && !!( multipoint_mask & multipoint_mode_t::top ) && !!( rec.m_flags & flags_t::on_ground ) )
	//		pointscale *= ent->desync_amount ( ) / 58.0f;
	//}

	auto fwd_vec = g::local->eyes( ) - hitbox_pos;
	fwd_vec.normalize( );

	auto side_vec = fwd_vec.cross_product( vec3_t( 0.0f, 0.0f, 1.0f ) );

	/* force multipoint to only center if pointscale is 0 */
	if ( !pointscale )
		multipoint_mask = multipoint_mode_t::center;

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
	VMP_END ( );
}

bool features::ragebot::get_hitbox( player_t* ent, anims::anim_info_t& rec, hitbox_t i, vec3_t& pos_out, float& rad_out, float& zrad_out ) {
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

	auto hitbox = set->hitbox( static_cast<int>( i ) );

	if ( !hitbox )
		return false;

	std::array< matrix3x4_t, 128>& bone_matrix = rec.m_aim_bones[ rec.m_side ];

	vec3_t vmin, vmax;
	VEC_TRANSFORM ( hitbox->m_bbmin, bone_matrix [ hitbox->m_bone ], vmin );
	VEC_TRANSFORM( hitbox->m_bbmax, bone_matrix [ hitbox->m_bone ], vmax );

	pos_out = ( vmin + vmax ) * 0.5f;

	if ( hitbox->m_radius < 0.0f ) {
		rad_out = ( vmax - vmin ).length_2d( ) * 0.5f;
		zrad_out = ( vmax - vmin ).z * 0.5f;
	}
	else {
		rad_out = hitbox->m_radius;
		zrad_out = abs ( vmax.z - vmin.z + hitbox->m_radius * 2.0f ) * 0.5f;
	}

	return true;
}

bool features::ragebot::hitscan( player_t* ent, anims::anim_info_t& rec, vec3_t& pos_out, hitbox_t& hitbox_out, float& best_dmg ) {
	VMP_BEGINMUTATION ( );
	auto pl = ent;

	if ( !pl )
		return false;

	const auto weapon = g::local->weapon( );

	if ( !weapon )
		return false;

	const auto weapon_data = weapon->data( );

	if ( !weapon_data )
		return false;

	auto min_dmg = get_scaled_min_dmg ( ent );

	/* select side to multipoint on (only one side at a time, saves a shit ton of frames) */
	multipoint_side_t multipoint_side = multipoint_side_t::none;

	const auto src = g::local->eyes( );

	auto eyes_max = rec.m_origin + vec3_t( 0.0f, 0.0f, 64.0f );
	auto fwd = eyes_max - src;
	fwd.normalize( );

	if ( !fwd.is_valid( ) )
		return false;

	auto right_dir = fwd.cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );
	auto left_dir = -right_dir;
	auto dmg_center = autowall::dmg ( g::local, pl, src, eyes_max, hitbox_t::head /* pretend player would be there */ );
	auto dmg_left = awall_skeet::dmg ( g::local, src, eyes_max + left_dir * 35.0f, weapon, pl, min_dmg, true );
	auto dmg_right = awall_skeet::dmg ( g::local, src, eyes_max + right_dir * 35.0f, weapon, pl, min_dmg, true );
	auto dmg_feet = autowall::dmg ( g::local, pl, src, rec.m_origin + vec3_t ( 0.0f, 0.0f, 2.0f ), hitbox_t::head /* pretend player would be there */ );

	/* will we most likely not do damage at all? then let's cancel target selection altogether i guess */
	if ( !dmg_center && !dmg_left && !dmg_right && !dmg_feet )
		return false;

	std::deque< hitbox_t > hitboxes { };

	if ( active_config.scan_pelvis )
		hitboxes.push_back( hitbox_t::pelvis );

	if ( active_config.scan_chest )
		hitboxes.push_back( hitbox_t::u_chest );

	if ( active_config.scan_head )
		hitboxes.push_back( hitbox_t::head );

	if ( active_config.scan_neck )
		hitboxes.push_back( hitbox_t::neck );

	if ( active_config.scan_feet ) {
		hitboxes.push_back( hitbox_t::r_foot );
		hitboxes.push_back( hitbox_t::l_foot );
	}

	if ( active_config.scan_legs ) {
		hitboxes.push_back( hitbox_t::r_calf );
		hitboxes.push_back( hitbox_t::l_calf );

		hitboxes.push_back ( hitbox_t::r_thigh );
		hitboxes.push_back ( hitbox_t::l_thigh );
	}

	if ( active_config.scan_arms ) {
		hitboxes.push_back ( hitbox_t::r_forearm );
		hitboxes.push_back ( hitbox_t::l_forearm );

		hitboxes.push_back ( hitbox_t::r_upperarm );
		hitboxes.push_back ( hitbox_t::l_upperarm );
	}

	/* force baim */
	const auto scaled_dmg = awall_skeet::scale_dmg( autowall::hitbox_to_hitgroup ( hitbox_t::pelvis ), pl, weapon_data->m_dmg, weapon_data->m_armor_ratio, weapon->item_definition_index ( ) );
	const auto scaled_dmg_head = awall_skeet::scale_dmg ( autowall::hitbox_to_hitgroup ( hitbox_t::head ), pl, weapon_data->m_dmg, weapon_data->m_armor_ratio, weapon->item_definition_index ( ) );
	
	auto should_baim = false;

	if ( ((get_misses( pl->idx( ) ).bad_resolve >= active_config.baim_after_misses && active_config.baim_after_misses )
		|| ( active_config.baim_air && !( rec.m_flags & flags_t::on_ground ) )
		|| ( active_config.baim_lethal && scaled_dmg > pl->health( ) )
		|| (( exploits::is_ready ( ) || exploits::has_shifted || exploits::in_exploit ) && active_config.max_dt_ticks > 6 && weapon->item_definition_index ( ) != weapons_t::ssg08 ))
		&& !active_config.headshot_only ) {
		should_baim = true;
	}

	/* allows us to make smarter choices for hitboxes if we know how many shots we will want to shoot */
	/* with doubletap, we can just go for body anyways (scale damage by 2, will calculate as if was 1 shot) */
	//const auto damage_scalar = ( ( exploits::is_ready ( ) || exploits::has_shifted ) && active_config.max_dt_ticks > 6 && active_config.dt_enabled && utils::keybind_active ( active_config.dt_key, active_config.dt_key_mode ) ) ? 2.0f : 1.0f;
	//const auto damage_scalar = 1.0f;
	
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
		hitboxes.push_front ( hitbox_t::head );
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
	//pl->set_abs_origin ( rec.m_origin );
	pl->bone_cache ( ) = dmg_scan_matrix.data();
	pl->mins ( ) = rec.m_mins;
	pl->maxs ( ) = rec.m_maxs;

	float best_dmg_tmp = 0.0f;
	vec3_t best_pos = vec3_t ( 0.0f, 0.0f, 0.0f );
	hitbox_t best_hitbox = hitbox_t::invalid;

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
			auto dmg = awall_skeet::dmg ( g::local, src, point, weapon, pl, min_dmg, false, autowall::hitbox_to_hitgroup ( hitbox ) ); /** ( ( hitbox == hitbox_pelvis || hitbox == hitbox_upper_chest ) ? damage_scalar : 1.0f )*/;
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
		if ( should_baim
			&& hitbox != hitbox_t::head && hitbox != hitbox_t::neck
			&& ( ( ( exploits::is_ready ( ) || exploits::has_shifted || exploits::in_exploit ) && active_config.max_dt_ticks > 6 ) ? ( weapon->item_definition_index ( ) != weapons_t::ssg08 ) : (best_dmg_tmp >= ent->health ( )) ) )
			break;

		//if ( best_dmg_tmp > min_dmg || best_dmg_tmp > rec.m_pl->health( ) ) {
		//	best_dmg = best_dmg_tmp;
		//	break;
		//}
	}
	
	/* save best data */
	if ( best_dmg_tmp > best_dmg && best_dmg_tmp > min_dmg ) {
		pos_out = best_pos;
		hitbox_out = best_hitbox;
		best_dmg = best_dmg_tmp;
	}

	/* restore player data to what it was before so we dont mess up anything */
	pl->origin ( ) = backup_origin;
	//pl->set_abs_origin ( backup_abs_origin );
	pl->bone_cache ( ) = backup_bone_cache;
	pl->mins ( ) = backup_mins;
	pl->maxs ( ) = backup_maxs;

	return best_dmg_tmp > 0.0f;
	VMP_END ( );
}

void features::ragebot::idealize_shot( player_t* ent, vec3_t& pos_out, hitbox_t& hitbox_out, anims::anim_info_t& rec_out, float& best_dmg ) {
	VMP_BEGINMUTATION ( );
	constexpr int SIMILAR_RECORD_THRESHOLD = 0;

	if ( !ent->valid ( ) )
		return;

	const auto recs = anims::get_lagcomp_records ( ent );
	const auto simulated_rec = active_config.fix_fakelag ? anims::get_simulated_record ( ent ) : std::nullopt;

	if ( recs.empty ( ) && !simulated_rec )
		return;

	std::deque < anims::anim_info_t* > best_recs { };

	/* extrapolation */
	/* TODO: BODYAIM ONLY ON THESE RECORDS */
	//if ( !recs.empty ( ) && extrap_amount > 0 ) {
	//	anims::anim_info_t* newest_non_onshot_rec = nullptr;
	//	float newest_time = 0.0f;
	//
	//	/* prefer non-onshot records */
	//	for ( auto& rec : recs ) {
	//		if ( rec->m_simtime > newest_time && !rec->m_shot ) {
	//			newest_non_onshot_rec = rec;
	//			newest_time = rec->m_simtime;
	//		}
	//	}
	//
	//	/* if there are no non-onshot records, we have no choice */
	//	auto newest_rec_for_bones = newest_non_onshot_rec ? newest_non_onshot_rec : recs.front ( );
	//	auto newest_rec = recs.front ( );
	//
	//	if ( newest_rec->m_choked_commands <= extrap_amount ) {
	//		auto net = cs::i::engine->get_net_channel_info ( );
	//		auto choked_ticks = newest_rec->m_choked_commands;
	//		auto hit_tick = cs::time2ticks ( newest_rec->m_simtime );
	//		auto lat_ticks = cs::time2ticks ( net->get_latency ( 0 ) );
	//
	//		auto arrival_tick = g::server_tick + 1 + lat_ticks;
	//		auto next = g::server_tick + 1;
	//
	//		if ( next + choked_ticks < arrival_tick ) {
	//			auto extrap_ticks = 0;
	//
	//			for ( ; next < arrival_tick; arrival_tick += choked_ticks ) {
	//				extrap_ticks++;
	//
	//				auto extrap_rec = new anims::anim_info_t { *newest_rec };
	//				
	//				extrap_rec->m_aim_bones = newest_rec_for_bones->m_aim_bones;
	//
	//				/* simulate player movement (do this properly later) */
	//				for ( int sim = 0; sim < choked_ticks; sim++ ) {
	//					extrap_rec->m_origin = newest_rec->m_origin + newest_rec->m_vel * cs::ticks2time ( extrap_ticks );
	//					hit_tick++;
	//				}
	//
	//				/* we also need to update animations for the choked commands */
	//
	//				/* move bone matrix */
	//				for ( auto& mat : extrap_rec->m_aim_bones [ extrap_rec->m_side ] )
	//					mat.set_origin ( mat.origin ( ) - newest_rec_for_bones->m_origin + extrap_rec->m_origin );
	//
	//				/* set new player simtime */
	//				extrap_rec->m_simtime = cs::ticks2time ( hit_tick );
	//				extrap_rec->m_forward_track = true;
	//
	//				best_recs.push_back ( extrap_rec );
	//
	//				/* only add 4 records max */
	//				if ( best_recs.size ( ) > extrap_amount )
	//					break;
	//			}
	//		}
	//	}
	//}

	const auto shot = anims::get_onshot ( recs );

	/* prefer onshot */
	if ( shot ) {
		best_recs.push_back( shot.value ( ) );

		if ( !active_config.onshot_only )
			if ( cs::time2ticks( shot.value ( )->m_simtime ) != cs::time2ticks( recs.back ( )->m_simtime ) )
				best_recs.push_back ( recs.back ( ) );
	}
	else if ( !active_config.onshot_only ) {
		if ( recs.empty ( ) ) {
			best_recs.push_back ( simulated_rec.value ( ) );
		}
		else {
			float highest_speed = 0.0f;
			float highest_sideways_amount = 0.0f;
			float highest_resolved = 0.0f;

			anims::anim_info_t* newest_rec = recs.front ( );
			anims::anim_info_t* oldest_rec = recs.back ( );
			anims::anim_info_t* speed_rec = nullptr;
			anims::anim_info_t* angdiff_rec = nullptr;
			anims::anim_info_t* resolved_rec = nullptr;

			const auto at_target_yaw = cs::normalize ( cs::calc_angle ( g::local->eyes ( ), ent->eyes ( ) ).y );

			for ( auto& rec : recs ) {
				const auto speed2d = rec->m_vel.length_2d ( );

				if ( !!( rec->m_flags & flags_t::on_ground ) && speed2d > highest_speed ) {
					speed_rec = rec;
					highest_speed = speed2d;
				}

				const auto ang_diff = abs ( cs::normalize ( at_target_yaw - cs::normalize ( rec->m_angles.y ) ) );

				if ( ang_diff > highest_sideways_amount ) {
					angdiff_rec = rec;
					highest_sideways_amount = ang_diff;
				}

				if ( rec->m_simtime > highest_resolved && rec->m_resolved ) {
					resolved_rec = rec;
					highest_resolved = rec->m_simtime;
				}
			}

			/* eliminate similar records */
			if ( resolved_rec ) {
				if ( angdiff_rec && abs ( cs::time2ticks ( angdiff_rec->m_simtime ) - cs::time2ticks ( resolved_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					angdiff_rec = nullptr;

				if ( speed_rec && abs ( cs::time2ticks ( speed_rec->m_simtime ) - cs::time2ticks ( resolved_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					speed_rec = nullptr;

				if ( newest_rec && abs ( cs::time2ticks ( newest_rec->m_simtime ) - cs::time2ticks ( resolved_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					newest_rec = nullptr;

				if ( oldest_rec && abs ( cs::time2ticks ( oldest_rec->m_simtime ) - cs::time2ticks ( resolved_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					oldest_rec = nullptr;

				best_recs.push_back ( resolved_rec );
			}

			if ( angdiff_rec ) {
				if ( speed_rec && abs ( cs::time2ticks ( speed_rec->m_simtime ) - cs::time2ticks ( angdiff_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					speed_rec = nullptr;

				if ( newest_rec && abs ( cs::time2ticks ( newest_rec->m_simtime ) - cs::time2ticks ( angdiff_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					newest_rec = nullptr;

				if ( oldest_rec && abs ( cs::time2ticks ( oldest_rec->m_simtime ) - cs::time2ticks ( angdiff_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					oldest_rec = nullptr;

				best_recs.push_back ( angdiff_rec );
			}

			if ( speed_rec ) {
				if ( newest_rec && abs ( cs::time2ticks ( newest_rec->m_simtime ) - cs::time2ticks ( speed_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					newest_rec = nullptr;

				if ( oldest_rec && abs ( cs::time2ticks ( oldest_rec->m_simtime ) - cs::time2ticks ( speed_rec->m_simtime ) ) <= SIMILAR_RECORD_THRESHOLD )
					oldest_rec = nullptr;

				best_recs.push_back ( speed_rec );
			}

			if ( newest_rec && oldest_rec && cs::time2ticks ( newest_rec->m_simtime ) != cs::time2ticks ( oldest_rec->m_simtime ) )
				best_recs.push_back ( newest_rec );

			if ( oldest_rec )
				best_recs.push_back ( oldest_rec );

			if ( simulated_rec )
				best_recs.push_back ( simulated_rec.value ( ) );
		}
	}

	if ( best_recs.empty( ) )
		return;

	/* scan for a good shot in one of the records we have */
	for ( auto& record : best_recs ) {
		if ( hitscan( ent, *record, pos_out, hitbox_out, best_dmg ) ) {
			rec_out = *record;
			break;
		}
	}

	for ( auto& record : best_recs )
		if ( record->m_forward_track )
			delete record;

	VMP_END ( );
}