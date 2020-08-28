#include "ragebot.hpp"
#include <deque>
#include "autowall.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "prediction.hpp"
#include "../animations/resolver.hpp"
#include "../menu/options.hpp"

player_t* last_target = nullptr;
int target_idx = 0;
std::array< features::ragebot::misses_t, 65 > misses { 0 };
std::array< vec3_t, 65 > target_pos { vec3_t( ) };
std::array< vec3_t, 65 > shot_pos { vec3_t( ) };
std::array< int, 65 > hits { 0 };
std::array< int, 65 > shots { 0 };
std::array< int, 65 > hitbox { 0 };
std::array< float, 65 > features::ragebot::hitchances { 0.0f };
std::array< features::lagcomp::lag_record_t, 65 > cur_lag_rec { 0 };
features::ragebot::weapon_config_t features::ragebot::active_config;

struct recorded_scans_t {
	recorded_scans_t( const features::lagcomp::lag_record_t& rec, const vec3_t& target, float dmg, int tick, int hitbox, int priority ) {
		m_rec = rec;
		m_target = target;
		m_dmg = dmg;
		m_tick = tick;
		m_hitbox = hitbox;
		m_priority = priority;
	}

	~recorded_scans_t( ) {

	}

	features::lagcomp::lag_record_t m_rec;
	vec3_t m_target;
	float m_dmg;
	int m_tick, m_hitbox, m_priority;
};

std::array< std::vector< recorded_scans_t >, 65 > previous_scanned_records { { } };

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

	if ( g::local->weapon( )->item_definition_index( ) == 64 && g::local->weapon( )->item_definition_index( ) != 1 ) {
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

		return;
	}
	else if ( g::local->weapon( )->data( )->m_type == 5 ) {
		if ( g::local->weapon( )->item_definition_index( ) == 9 ) {
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

			return;
		}
		else if ( g::local->weapon( )->item_definition_index( ) == 38 || g::local->weapon( )->item_definition_index( ) == 11 ) {
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

			return;
		}
		else if ( g::local->weapon( )->item_definition_index( ) == 40 ) {
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

bool run_hitchance( vec3_t ang, player_t* pl, vec3_t point, int rays, int hitbox ) {
	auto weapon = g::local->weapon( );

	if ( !weapon || !weapon->data( ) ) {
		features::ragebot::hitchances [ pl->idx( ) ] = 0.0f;
		return false;
	}

	auto src = g::local->eyes( );

	ang = csgo::calc_angle( src, point );
	csgo::clamp( ang );

	auto forward = csgo::angle_vec( ang );
	auto right = csgo::angle_vec( ang + vec3_t( 0.0f, 90.0f, 0.0f ) );
	auto up = csgo::angle_vec( ang + vec3_t( 90.0f, 0.0f, 0.0f ) );

	forward.normalize( );
	right.normalize( );
	up.normalize( );

	auto hits = 0;
	auto needed_hits = static_cast< int >( static_cast< float > ( rays ) * ( features::ragebot::active_config.hit_chance / 100.0f ) );

	weapon->update_accuracy( );

	auto weap_spread = weapon->inaccuracy( ) + weapon->spread( );

	const auto as_hitgroup = autowall::hitbox_to_hitgroup( hitbox );

	auto ray_intersects_sphere = [ ] ( const vec3_t& from, const vec3_t& to, const vec3_t& sphere, float rad ) -> bool {
		auto q = sphere - from;
		auto v = q.dot_product( ( to - from ).normalized( ) );
		auto d = ( rad * rad ) - ( q.length_sqr( ) - v * v );

		return d >= FLT_EPSILON;
	};

	auto get_hitbox = [ & ] ( vec3_t& pos_out ) -> float {
		pos_out = vec3_t( 0.0f, 0.0f, 0.0f );

		auto mdl = pl->mdl( );

		if ( !mdl )
			return 0.0f;

		auto studio_mdl = csgo::i::mdl_info->studio_mdl( mdl );

		if ( !studio_mdl )
			return 0.0f;

		auto s = studio_mdl->hitbox_set( 0 );

		if ( !s )
			return 0.0f;

		auto hb = s->hitbox( hitbox );

		if ( !hb )
			return 0.0f;

		auto vmin = hb->m_bbmin;
		auto vmax = hb->m_bbmax;

		VEC_TRANSFORM( hb->m_bbmin, pl->bone_cache( ) [ hb->m_bone ], vmin );
		VEC_TRANSFORM( hb->m_bbmax, pl->bone_cache( ) [ hb->m_bone ], vmax );

		pos_out = ( vmin + vmax ) * 0.5f;

		return hb->m_radius;
	};

	vec3_t hitbox_center;
	const auto hitbox_rad = get_hitbox( hitbox_center );

	hitbox_center = point;

	if ( !hitbox_rad )
		return false;

	/* normal hitchance */
	for ( auto i = 0; i < rays; i++ ) {
		const auto spread_x = -weap_spread * 0.5f + ( ( static_cast < float > ( rand( ) ) / static_cast < float > ( RAND_MAX ) ) * weap_spread );
		const auto spread_y = -weap_spread * 0.5f + ( ( static_cast < float > ( rand( ) ) / static_cast < float > ( RAND_MAX ) ) * weap_spread );
		const auto spread_z = -weap_spread * 0.5f + ( ( static_cast < float > ( rand( ) ) / static_cast < float > ( RAND_MAX ) ) * weap_spread );
		const auto final_pos = src + ( ( forward + vec3_t( spread_x, spread_y, spread_z ) ) * weapon->data( )->m_range );

		if ( ray_intersects_sphere( src, final_pos, hitbox_center, hitbox_rad ) )
			hits++;
	}

	const auto calc_chance = static_cast< float >( hits ) / static_cast< float > ( rays ) * 100.0f;

	if ( calc_chance < features::ragebot::active_config.hit_chance )
		return false;

	/* check damage accuracy */
	const auto spread_coeff = weap_spread * 0.5f * ( features::ragebot::active_config.dmg_accuracy / 100.0f );
	const auto left_point = src + ( forward - right * spread_coeff ) * src.dist_to( point );
	const auto right_point = src + ( forward + right * spread_coeff ) * src.dist_to( point );
	const auto dmg_left = autowall::dmg( g::local, pl, src, left_point, hitbox );
	const auto dmg_right = autowall::dmg( g::local, pl, src, right_point, hitbox );

	return dmg_left >= features::ragebot::active_config.min_dmg && dmg_right >= features::ragebot::active_config.min_dmg;
}

void features::ragebot::hitscan( player_t* pl, vec3_t& point, float& dmg, lagcomp::lag_record_t& rec_out, int& hitbox_out ) {
	const auto recs = lagcomp::get( pl );
	auto shot = lagcomp::get_shot( pl );

	dmg = 0.0f;

	if ( !g::local || !g::local->weapon( ) || !pl->valid( ) || !pl->weapon( ) || !pl->weapon( )->data( ) || !pl->bone_cache( ) || !pl->layers( ) || !pl->animstate( ) || ( !recs.second && !shot.second ) )
		return;

	const auto backup_origin = pl->origin( );
	auto backup_abs_origin = pl->abs_origin( );
	const auto backup_min = pl->mins( );
	const auto backup_max = pl->maxs( );
	matrix3x4_t backup_bones [ 128 ];
	std::memcpy( backup_bones, pl->bone_cache( ), sizeof matrix3x4_t * pl->bone_count( ) );

	auto newest_moving_tick = 0;
	std::deque < lagcomp::lag_record_t > best_recs { };

	auto head_only = false;

	if ( shot.second ) {
		best_recs.push_back( shot.first );
		best_recs.push_back( recs.first.back( ) );
		head_only = true;
	}
	else {
		/*if ( !lagcomp::data::extrapolated_records [ pl->idx ( ) ].empty ( ) && csgo::time2ticks ( csgo::i::globals->m_curtime - pl->simtime ( ) ) > 2 ) {
			best_recs.push_back ( lagcomp::data::extrapolated_records [ pl->idx ( ) ].front ( ) );
		}
		else*/ if ( recs.first.size( ) >= 2 ) {
			best_recs.push_back( recs.first.back( ) );
		}

		best_recs.push_back( recs.first.front( ) );
	}

	if ( best_recs.empty( ) )
		return;

	lagcomp::lag_record_t now_rec;

	std::deque< int > hitboxes { };

	if ( active_config.scan_pelvis )
		hitboxes.push_back( 2 ); // pelvis

	if ( active_config.scan_head )
		hitboxes.push_back( 0 ); // head

	if ( active_config.scan_neck )
		hitboxes.push_back( 1 ); // neck

	if ( active_config.scan_feet ) {
		hitboxes.push_back( 11 ); // right foot
		hitboxes.push_back( 12 ); // left foot
	}

	if ( active_config.scan_chest ) {
		hitboxes.push_back( 6 ); // chest
	}

	if ( active_config.scan_legs ) {
		hitboxes.push_back( 7 ); // right thigh
		hitboxes.push_back( 8 ); // left thigh
	}

	if ( active_config.scan_arms ) {
		hitboxes.push_back( 18 ); // right forearm
		hitboxes.push_back( 16 ); // left forearm
	}

	/* skip all hitboxes but head if we have their onshot */
	//if ( head_only ) {
	//	hitboxes.clear( );
	//	hitboxes.push_back( 2 ); // pelvis
	//	hitboxes.push_front( 0 );
	//}

	if ( active_config.headshot_only ) {
		hitboxes.clear( );
		hitboxes.push_front( 0 );
	}

	auto should_baim = false;

	/* override to baim if we can doubletap */ {
		/* tickbase manip controller */

		auto can_shoot = [ & ] ( ) {
			if ( !g::local->weapon( ) || !g::local->weapon( )->ammo( ) )
				return false;

			if ( g::local->weapon( )->item_definition_index( ) == 64 && !( g::can_fire_revolver || csgo::time2ticks( csgo::i::globals->m_curtime ) > g::cock_ticks ) )
				return false;

			return csgo::i::globals->m_curtime >= g::local->next_attack( ) && csgo::i::globals->m_curtime >= g::local->weapon( )->next_primary_attack( );
		};

		const auto weapon_data = ( g::local && g::local->weapon( ) && g::local->weapon( )->data( ) ) ? g::local->weapon( )->data( ) : nullptr;
		const auto fire_rate = weapon_data ? weapon_data->m_fire_rate : 0.0f;
		auto tickbase_as_int = std::clamp< int >( csgo::time2ticks( fire_rate ) - 1, 0, std::clamp( static_cast< int >( active_config.max_dt_ticks ), 0, csgo::is_valve_server( ) ? 8 : 16 ) );

		if ( !active_config.dt_enabled || !utils::keybind_active( active_config.dt_key, active_config.dt_key_mode ) )
			tickbase_as_int = 0;

		if ( tickbase_as_int && ( ( can_shoot( ) && ( std::abs( g::ucmd->m_tickcount - g::dt_recharge_time ) >= tickbase_as_int && !g::dt_ticks_to_shift ) ) || g::next_tickbase_shot ) && g::local->weapon( )->item_definition_index( ) != 64 ) {
			should_baim = true;
		}
	}

	auto scan_safe_points = true;

	/* retry with safe points if they aren't valid */
retry_without_safe_points:

	auto best_priority = 0;
	auto best_point = vec3_t( );
	auto best_dmg = 0.0f;
	lagcomp::lag_record_t best_rec;
	int best_hitbox = 0;

	/* only keep previous scan records which are still valid */
	if ( !previous_scanned_records [ pl->idx( ) ].empty( ) ) {
		int cur_scan_record = 0;

		for ( auto& previous_scans : previous_scanned_records [ pl->idx( ) ] ) {
			if ( !previous_scans.m_rec.valid( ) ) {
				previous_scanned_records [ pl->idx( ) ].erase( previous_scanned_records [ pl->idx( ) ].begin( ) + cur_scan_record );
				continue;
			}

			cur_scan_record++;
		}
	}

	/* optimization - increases our framerate by a shit ton, but decreases ragebot performance by a bit */
	auto is_similar_scan = [ ] ( const recorded_scans_t& scan_record, const lagcomp::lag_record_t& lag_rec ) {
		return scan_record.m_rec.m_pl && ( std::abs( scan_record.m_tick - lag_rec.m_tick ) <= 2 || scan_record.m_rec.m_origin.dist_to( lag_rec.m_origin ) < 5.0f );
	};

	std::array< matrix3x4_t, 128 > safe_point_bones;

	/* find best record */
	for ( auto& rec_it : best_recs ) {
		if ( rec_it.m_needs_matrix_construction || rec_it.m_priority < best_priority )
			continue;

		/* if we already scanned this before, or is within close proximity of another record we scanned, let's consider it the same record for now */
		/* this should drastically speed up the ragebot */
		auto record_existed = false;

		//if ( !previous_scanned_records [ pl->idx( ) ].empty( ) ) {
		//	recorded_scans_t* similar_scan = nullptr;
		//	//
		//	for ( auto& scan_record : previous_scanned_records [ pl->idx( ) ] ) {
		//		if ( is_similar_scan( scan_record, rec_it ) ) {
		//			similar_scan = &scan_record;
		//			break;
		//		}
		//	}
		//	//
		//	if ( similar_scan ) {
		//		/* if they become shootable soon, maybe we should rescan them to make sure we can shoot as soon as possible */ {
		//			const auto eyes_max = pl->origin( ) + vec3_t( 0.0f, 0.0f, 64.0f );
		//			const auto fwd = csgo::angle_vec( eyes_max - g::local->eyes( ) );
		//			const auto right_dir = fwd.cross_product( vec3_t( 0.0f, 0.0f, 1.0f ) );
		//			const auto left_dir = -right_dir;
		//			const auto dmg_left = autowall::dmg( g::local, pl, g::local->eyes( ) + left_dir * 35.0f, eyes_max + left_dir * 35.0f, 0 /* pretend player would be there */ );
		//			const auto dmg_right = autowall::dmg( g::local, pl, g::local->eyes( ) + right_dir * 35.0f, eyes_max + right_dir * 35.0f, 0 /* pretend player would be there */ );
		//			//
		//								/* scan this record again to make sure we might be able to hit this one if state changed */
		//			if ( dmg_left || dmg_right ) {
		//				similar_scan->m_rec.m_pl = nullptr;
		//				similar_scan = nullptr;
		//				record_existed = false;
		//			}
		//		}
		//		//
		//		if ( similar_scan ) {
		//			if ( /* double check validity i guess */ similar_scan->m_rec.valid( )
		//				/* make sure overwriting this record is worth it, would probably be a good thing */ && similar_scan->m_dmg > best_dmg
		//				/* if the shot is fatal, it doesn't matter */ || best_dmg >= pl->health( ) ) {
		//				best_dmg = similar_scan->m_dmg;
		//				best_point = similar_scan->m_target;
		//				best_rec = similar_scan->m_rec;
		//				best_hitbox = similar_scan->m_hitbox;
		//				best_priority = similar_scan->m_priority;
		//				record_existed = true;
		//			}
		//			//
		//			if ( best_dmg >= pl->health( ) ) {
		//				pl->mins( ) = backup_min;
		//				pl->maxs( ) = backup_max;
		//				pl->origin( ) = backup_origin;
		//				pl->set_abs_origin( backup_abs_origin );
		//				std::memcpy( pl->bone_cache( ), backup_bones, sizeof matrix3x4_t * pl->bone_count( ) );
		//				//
		//				point = best_point;
		//				dmg = best_dmg;
		//				rec_out = best_rec;
		//				hitbox_out = best_hitbox;
		//				//
		//				return;
		//			}
		//			else {
		//				continue;
		//			}
		//		}
		//	}
		//}

		/* for safe point scanning */
		/* override aim matrix with safe point matrix so we scan safe points from safe point matrix on current aim matrix */
		if ( active_config.safe_point && utils::keybind_active( active_config.safe_point_key, active_config.safe_point_key_mode ) && scan_safe_points ) {
			const auto next_resolve = get_misses( pl->idx( ) ).bad_resolve + 1;

			if ( next_resolve % 3 == 0 )
				std::memcpy( &safe_point_bones, rec_it.m_bones1, sizeof matrix3x4_t * pl->bone_count( ) );
			else if ( next_resolve % 3 == 1 )
				std::memcpy( &safe_point_bones, rec_it.m_bones2, sizeof matrix3x4_t * pl->bone_count( ) );
			else
				std::memcpy( &safe_point_bones, rec_it.m_bones3, sizeof matrix3x4_t * pl->bone_count( ) );
		}

		pl->mins( ) = rec_it.m_min;
		pl->maxs( ) = rec_it.m_max;
		pl->origin( ) = rec_it.m_origin;
		pl->set_abs_origin( rec_it.m_origin );

		if ( get_misses( pl->idx( ) ).bad_resolve % 3 == 0 )
			std::memcpy( pl->bone_cache( ), rec_it.m_bones1, sizeof matrix3x4_t * pl->bone_count( ) );
		else if ( get_misses( pl->idx( ) ).bad_resolve % 3 == 1 )
			std::memcpy( pl->bone_cache( ), rec_it.m_bones2, sizeof matrix3x4_t * pl->bone_count( ) );
		else
			std::memcpy( pl->bone_cache( ), rec_it.m_bones3, sizeof matrix3x4_t * pl->bone_count( ) );

		auto best_hitbox_this_rec = 0;
		auto best_dmg_this_rec = 0.0f;
		auto best_point_this_rec = vec3_t( 0.0f, 0.0f, 0.0f );

		/* find best point on best hitbox */
		for ( auto& hb : hitboxes ) {
			auto mdl = pl->mdl( );

			if ( !mdl )
				continue;

			auto studio_mdl = csgo::i::mdl_info->studio_mdl( mdl );

			if ( !studio_mdl )
				continue;

			auto s = studio_mdl->hitbox_set( 0 );

			if ( !s )
				continue;

			auto hitbox = s->hitbox( hb );

			if ( !hitbox )
				continue;

			auto get_hitbox_pos = [ & ] ( bool force_safe_point = false ) {
				vec3_t vmin, vmax;

				auto safe_point_on = true;

				/* override aim matrix with safe point matrix so we scan safe points from safe point matrix on current aim matrix */
				if ( active_config.safe_point && utils::keybind_active( active_config.safe_point_key, active_config.safe_point_key_mode ) && scan_safe_points /*&& ( hb != 0 || force_safe_point )*/ ) {
					VEC_TRANSFORM( hitbox->m_bbmin, safe_point_bones [ hitbox->m_bone ], vmin );
					VEC_TRANSFORM( hitbox->m_bbmax, safe_point_bones [ hitbox->m_bone ], vmax );
				}
				else {
					VEC_TRANSFORM( hitbox->m_bbmin, pl->bone_cache( ) [ hitbox->m_bone ], vmin );
					VEC_TRANSFORM( hitbox->m_bbmax, pl->bone_cache( ) [ hitbox->m_bone ], vmax );
				}

				auto pos = ( vmin + vmax ) * 0.5f;

				return pos;
			};

			auto a1 = csgo::calc_angle( get_hitbox_pos( ), g::local->eyes( ) );
			auto fwd = csgo::angle_vec( a1 );
			auto right = fwd.cross_product( vec3_t( 0.0f, 0.0f, 1.0f ) );
			auto left = -right;
			auto top = vec3_t( 0.0f, 0.0f, 1.0f );

			auto pointscale = 0.5f;

			/* handle pointscale */
			if ( hb == 0 )
				pointscale = static_cast< float >( active_config.head_pointscale ) / 100.0f;
			else
				pointscale = static_cast< float >( active_config.body_pointscale ) / 100.0f;

			auto rad_coeff = pointscale * hitbox->m_radius;
			auto can_tickbase = false;

			/* try to baim */
			if ( hb == 2 ) {
				auto body = get_hitbox_pos( );
				auto body_left = body + left * rad_coeff;
				auto body_right = body + right * rad_coeff;
				auto dmg1 = autowall::dmg( g::local, pl, g::local->eyes( ), body, -1 /* do not scan floating points */ );
				auto dmg2 = autowall::dmg( g::local, pl, g::local->eyes( ), body_left, -1 /* do not scan floating points */ );
				auto dmg3 = autowall::dmg( g::local, pl, g::local->eyes( ), body_right, -1 /* do not scan floating points */ );

				if ( dmg1 > best_dmg || dmg2 > best_dmg || dmg3 > best_dmg ) {
					if ( dmg1 > dmg2 && dmg1 > dmg3 || ( dmg1 == dmg2 && dmg1 == dmg3 ) ) {
						best_dmg = dmg1;
						best_point = body;
						best_rec = rec_it;
						best_hitbox = hb;
						best_priority = rec_it.m_priority;
					}
					else if ( dmg2 > dmg1 && dmg2 > dmg3 ) {
						best_dmg = dmg2;
						best_point = body_left;
						best_rec = rec_it;
						best_hitbox = hb;
						best_priority = rec_it.m_priority;
					}
					else if ( dmg3 > dmg1 && dmg3 > dmg2 ) {
						best_dmg = dmg3;
						best_point = body_right;
						best_rec = rec_it;
						best_hitbox = hb;
						best_priority = rec_it.m_priority;
					}
				}

				if ( best_dmg >= pl->health( ) || ( ( should_baim || misses [ pl->idx( ) ].bad_resolve >= active_config.baim_after_misses ) && ( best_dmg /** 1.666f*/ >= active_config.min_dmg || best_dmg /** 1.666f*/ >= pl->health( ) ) ) ) {
					pl->mins( ) = backup_min;
					pl->maxs( ) = backup_max;
					pl->origin( ) = backup_origin;
					pl->set_abs_origin( backup_abs_origin );
					std::memcpy( pl->bone_cache( ), backup_bones, sizeof matrix3x4_t * pl->bone_count( ) );

					point = best_point;
					dmg = best_dmg;
					rec_out = best_rec;
					hitbox_out = best_hitbox;

					if ( !record_existed )
						previous_scanned_records [ pl->idx( ) ].push_back( recorded_scans_t( best_rec, best_point, best_dmg, best_rec.m_tick, best_hitbox, rec_it.m_priority ) );

					return;
				}
			}

			// calculate best points on hitbox
			switch ( hb ) {
				// aim above head to try to hit
				case 0: {
					auto head = get_hitbox_pos( );
					auto head_top = head + top * rad_coeff;
					auto dmg1 = autowall::dmg( g::local, pl, g::local->eyes( ), head, -1 /* do not scan floating points */ );
					auto dmg2 = autowall::dmg( g::local, pl, g::local->eyes( ), head_top, -1 /* do not scan floating points */ );

					if ( dmg1 > best_dmg || dmg2 > best_dmg ) {
						if ( dmg2 > dmg1 ) {
							best_dmg = dmg2;
							best_point = head_top;
							best_rec = rec_it;
							best_hitbox = hb;
							best_priority = rec_it.m_priority;
						}
						else {
							best_dmg = dmg1;
							best_point = head;
							best_rec = rec_it;
							best_hitbox = hb;
							best_priority = rec_it.m_priority;
						}
					}

					if ( dmg1 > best_dmg_this_rec || dmg2 > best_dmg_this_rec ) {
						if ( dmg2 > dmg1 ) {
							best_point_this_rec = head_top;
							best_dmg_this_rec = dmg2;
						}
						else {
							best_point_this_rec = head;
							best_dmg_this_rec = dmg1;
						}

						best_hitbox_this_rec = hb;
					}

					/* prefer safe point on head if available, else fall back to normal point */
					if ( active_config.safe_point && utils::keybind_active( active_config.safe_point_key, active_config.safe_point_key_mode ) && scan_safe_points ) {
						auto head = get_hitbox_pos( true );
						auto head_top = head + top * rad_coeff;
						auto dmg1 = autowall::dmg( g::local, pl, g::local->eyes( ), head, -1 /* do not scan floating points */ );
						auto dmg2 = autowall::dmg( g::local, pl, g::local->eyes( ), head_top, -1 /* do not scan floating points */ );

						if ( dmg1 >= best_dmg || dmg2 >= best_dmg ) {
							if ( dmg2 > dmg1 ) {
								best_dmg = dmg2;
								best_point = head_top;
								best_rec = rec_it;
								best_hitbox = hb;
								best_priority = rec_it.m_priority;
							}
							else {
								best_dmg = dmg1;
								best_point = head;
								best_rec = rec_it;
								best_hitbox = hb;
								best_priority = rec_it.m_priority;
							}
						}

						if ( dmg1 >= best_dmg_this_rec || dmg2 >= best_dmg_this_rec ) {
							if ( dmg2 > dmg1 ) {
								best_point_this_rec = head_top;
								best_dmg_this_rec = dmg2;
							}
							else {
								best_point_this_rec = head;
								best_dmg_this_rec = dmg1;
							}

							best_hitbox_this_rec = hb;
						}
					}
				} break;
					// body stuff (aim outwards)
				case 2:
				case 11:
				case 12:
				case 6:
				case 7:
				case 8: {
					auto body = get_hitbox_pos( );
					auto body_left = body + left * rad_coeff;
					auto body_right = body + right * rad_coeff;
					auto dmg1 = autowall::dmg( g::local, pl, g::local->eyes( ), body, -1 /* do not scan floating points */ );
					auto dmg2 = autowall::dmg( g::local, pl, g::local->eyes( ), body_left, -1 /* do not scan floating points */ );
					auto dmg3 = autowall::dmg( g::local, pl, g::local->eyes( ), body_right, -1 /* do not scan floating points */ );

					if ( dmg1 > best_dmg || dmg2 > best_dmg || dmg3 > best_dmg ) {
						if ( ( dmg1 > dmg2 && dmg1 > dmg3 ) || ( dmg1 == dmg2 && dmg1 == dmg3 ) ) {
							best_dmg = dmg1;
							best_point = body;
							best_rec = rec_it;
							best_hitbox = hb;
							best_priority = rec_it.m_priority;
						}
						else if ( dmg2 > dmg1 && dmg2 > dmg3 ) {
							best_dmg = dmg2;
							best_point = body_left;
							best_rec = rec_it;
							best_hitbox = hb;
							best_priority = rec_it.m_priority;
						}
						else if ( dmg3 > dmg1 && dmg3 > dmg2 ) {
							best_dmg = dmg3;
							best_point = body_right;
							best_rec = rec_it;
							best_hitbox = hb;
							best_priority = rec_it.m_priority;
						}
					}

					if ( dmg1 > best_dmg_this_rec || dmg2 > best_dmg_this_rec || dmg3 > best_dmg_this_rec ) {
						if ( dmg3 > dmg1 && dmg3 > dmg2 ) {
							best_point_this_rec = body_right;
							best_dmg_this_rec = dmg3;
						}
						else if ( dmg2 > dmg1 && dmg2 > dmg3 ) {
							best_point_this_rec = body_left;
							best_dmg_this_rec = dmg2;
						}
						else {
							best_point_this_rec = body;
							best_dmg_this_rec = dmg1;
						}

						best_hitbox_this_rec = hb;
					}
				} break;
					// no need to test multiple points on these
				case 18:
				case 16:
				case 1: {
					auto hitbox_pos = get_hitbox_pos( );
					auto dmg = autowall::dmg( g::local, pl, g::local->eyes( ), hitbox_pos, -1 /* do not scan floating points */ );

					if ( dmg > best_dmg ) {
						best_dmg = dmg;
						best_point = hitbox_pos;
						best_rec = rec_it;
						best_hitbox = hb;
						best_priority = rec_it.m_priority;
					}

					if ( dmg > best_dmg_this_rec ) {
						best_point_this_rec = hitbox_pos;
						best_dmg_this_rec = dmg;
						best_hitbox_this_rec = hb;
					}
				} break;
			}
		}

		if ( !record_existed )
			previous_scanned_records [ pl->idx( ) ].push_back( recorded_scans_t( rec_it, best_point_this_rec, best_dmg_this_rec, rec_it.m_tick, best_hitbox_this_rec, rec_it.m_priority ) );
	}

	pl->mins( ) = backup_min;
	pl->maxs( ) = backup_max;
	pl->origin( ) = backup_origin;
	pl->set_abs_origin( backup_abs_origin );
	std::memcpy( pl->bone_cache( ), backup_bones, sizeof matrix3x4_t * pl->bone_count( ) );

	if ( best_dmg > 0.0f && ( best_dmg >= active_config.min_dmg || best_dmg >= pl->health( ) ) ) {
		point = best_point;
		dmg = best_dmg;
		rec_out = best_rec;
		hitbox_out = best_hitbox;
	}
	/* only retry if it's our first pass, or else we will freeze the game by looping indefinitely */
	else if ( active_config.safe_point && utils::keybind_active( active_config.safe_point_key, active_config.safe_point_key_mode ) && scan_safe_points ) {
		/* retry, but this time w/out safe points since they weren't valid */
		scan_safe_points = false;
		goto retry_without_safe_points;
	}
}

void meleebot( ucmd_t* ucmd ) {
	if ( g::local->weapon( )->item_definition_index( ) == 31 && !features::ragebot::active_config.zeus_bot )
		return;
	else if ( g::local->weapon( )->data( )->m_type == 0 && !features::ragebot::active_config.knife_bot )
		return;

	/*auto can_shoot = [ & ] ( ) {
		return g::local->weapon ( ) && g::local->weapon ( )->next_primary_attack ( ) <= csgo::i::globals->m_curtime && g::local->weapon ( )->ammo ( );
	};

	if ( !can_shoot ( ) )
		return;*/

	vec3_t engine_ang;
	csgo::i::engine->get_viewangles( engine_ang );

	features::lagcomp::lag_record_t* best_rec = nullptr;
	player_t* best_pl = nullptr;
	float best_fov = 180.0f;
	vec3_t best_point, best_ang;

	csgo::for_each_player( [ & ] ( player_t* pl ) {
		if ( pl->team( ) == g::local->team( ) || pl->immune( ) )
			return;

		auto rec = features::lagcomp::get( pl );

		if ( !rec.second )
			return;

		auto mdl = pl->mdl( );

		if ( !mdl )
			return;

		auto studio_mdl = csgo::i::mdl_info->studio_mdl( mdl );

		if ( !studio_mdl )
			return;

		auto s = studio_mdl->hitbox_set( 0 );

		if ( !s )
			return;

		auto hitbox = s->hitbox( 2 );

		if ( !hitbox )
			return;

		auto get_hitbox_pos = [ & ] ( ) {
			vec3_t vmin, vmax;

			if ( features::ragebot::get_misses( pl->idx( ) ).bad_resolve % 3 == 0 ) {
				VEC_TRANSFORM( hitbox->m_bbmin, rec.first [ 0 ].m_bones1 [ hitbox->m_bone ], vmin );
				VEC_TRANSFORM( hitbox->m_bbmax, rec.first [ 0 ].m_bones1 [ hitbox->m_bone ], vmax );
			}
			else if ( features::ragebot::get_misses( pl->idx( ) ).bad_resolve % 3 == 1 ) {
				VEC_TRANSFORM( hitbox->m_bbmin, rec.first [ 0 ].m_bones2 [ hitbox->m_bone ], vmin );
				VEC_TRANSFORM( hitbox->m_bbmax, rec.first [ 0 ].m_bones2 [ hitbox->m_bone ], vmax );
			}
			else {
				VEC_TRANSFORM( hitbox->m_bbmin, rec.first [ 0 ].m_bones3 [ hitbox->m_bone ], vmin );
				VEC_TRANSFORM( hitbox->m_bbmax, rec.first [ 0 ].m_bones3 [ hitbox->m_bone ], vmax );
			}

			auto pos = ( vmin + vmax ) * 0.5f;

			return pos;
		};

		const auto hitbox_pos = get_hitbox_pos( );

		auto ang = csgo::calc_angle( g::local->eyes( ), hitbox_pos );
		csgo::clamp( ang );

		const auto fov = csgo::calc_fov( ang, engine_ang );

		auto can_use = false;

		if ( g::local->weapon( )->item_definition_index( ) == 31 ) {
			can_use = g::local->eyes( ).dist_to( hitbox_pos ) < 150.0f;
		}
		else {
			can_use = g::local->origin( ).dist_to( rec.first [ 0 ].m_origin ) < 48.0f;
		}

		if ( csgo::is_visible( hitbox_pos ) && fov < best_fov && can_use ) {
			best_pl = pl;
			best_fov = fov;
			best_point = hitbox_pos;
			best_ang = ang;
			best_rec = &rec.first [ 0 ];
		}
		} );

	if ( !features::ragebot::active_config.auto_shoot && ( ( g::local->weapon( )->data( )->m_type == 0 ) ? !( ucmd->m_buttons & 1 ) : !( ucmd->m_buttons & 2048 ) ) )
		return;

	if ( !best_pl )
		return;

	csgo::clamp( best_ang );

	ucmd->m_angs = best_ang;

	if ( !features::ragebot::active_config.silent )
		csgo::i::engine->set_viewangles( best_ang );

	( *best_rec ).backtrack( ucmd );

	features::ragebot::get_target_pos( best_pl->idx( ) ) = best_point;
	features::ragebot::get_target( ) = best_pl;
	features::ragebot::get_shots( best_pl->idx( ) )++;
	features::ragebot::get_shot_pos( best_pl->idx( ) ) = g::local->eyes( );
	features::ragebot::get_lag_rec( best_pl->idx( ) ) = *best_rec;
	features::ragebot::get_target_idx( ) = best_pl->idx( );
	features::ragebot::get_hitbox( best_pl->idx( ) ) = 5;

	if ( features::ragebot::active_config.auto_shoot && g::local->weapon( )->item_definition_index( ) != 31 ) {
		auto back = best_pl->angles( );
		back.y = csgo::normalize( back.y + 180.0f );
		auto backstab = best_ang.dist_to( back ) < 45.0f;

		if ( backstab ) {
			ucmd->m_buttons |= 2048;
		}
		else {
			auto hp = best_pl->health( );
			auto armor = best_pl->armor( ) > 1;
			auto min_dmg1 = armor ? 34 : 40;
			auto min_dmg2 = armor ? 55 : 65;

			if ( hp <= min_dmg2 )
				ucmd->m_buttons |= 2048;
			else
				ucmd->m_buttons |= 1;
		}

		g::send_packet = true;
	}
	else if ( features::ragebot::active_config.auto_shoot ) {
		ucmd->m_buttons |= 1;
		g::send_packet = true;
	}
}

void features::ragebot::run( ucmd_t* ucmd, float& old_smove, float& old_fmove, vec3_t& old_angs ) {
	security_handler::update( );

	revolver_firing = false;
	//get_target ( ) = nullptr;

	if ( g::round == round_t::starting )
		return;

	if ( active_config.main_switch && g::local && g::local->weapon( ) && g::local->weapon( )->data( ) && ( g::local->weapon( )->item_definition_index( ) == 31 || g::local->weapon( )->data( )->m_type == 0 ) ) {
		meleebot( ucmd );
		return;
	}

	if ( !active_config.main_switch || !g::local || !g::local->weapon( ) || !g::local->weapon( )->data( ) || g::local->weapon( )->data( )->m_type == 9 || g::local->weapon( )->data( )->m_type == 7 || g::local->weapon( )->data( )->m_type == 0 )
		return;

	vec3_t engine_ang;
	csgo::i::engine->get_viewangles( engine_ang );

	auto can_shoot = [ & ] ( ) {
		if ( !g::local->weapon( ) || !g::local->weapon( )->ammo( ) )
			return false;

		if ( g::local->weapon( )->item_definition_index( ) == 64 && !( g::can_fire_revolver || csgo::time2ticks( csgo::i::globals->m_curtime ) > g::cock_ticks ) )
			return false;

		//if ( g::shifted_tickbase )
		//	return csgo::ticks2time ( g::local->tick_base ( ) - ( ( g::shifted_tickbase > 0 ) ? 1 + g::shifted_tickbase : 0 ) ) <= g::local->weapon ( )->next_primary_attack ( );

		return ( csgo::i::globals->m_curtime >= g::local->next_attack( ) && csgo::i::globals->m_curtime >= g::local->weapon( )->next_primary_attack( ) ) || g::next_tickbase_shot;
	};

	static const auto ray_intersects_sphere = [ ] ( const vec3_t& from, const vec3_t& to, const vec3_t& sphere, float rad ) -> bool {
		auto q = sphere - from;
		auto v = q.dot_product( csgo::angle_vec( csgo::calc_angle( from, to ) ).normalized( ) );
		auto d = rad - ( q.length_sqr( ) - v * v );

		return d >= FLT_EPSILON;
	};

	lagcomp::lag_record_t best_rec;
	player_t* best_pl = nullptr;
	auto best_ang = vec3_t( );
	auto best_point = vec3_t( );
	auto best_fov = 180.0f;
	auto best_dmg = 0.0f;
	auto best_hitbox = 0;

	struct potential_target_t {
		player_t* m_ent;
		float m_fov;
		float m_dist;
		int m_health;
	};

	std::vector < potential_target_t > targets { };

	csgo::for_each_player( [ & ] ( player_t* pl ) {
		if ( pl->team( ) == g::local->team( ) || pl->immune( ) )
			return;

		targets.push_back( {
			pl,
			csgo::calc_fov( engine_ang, csgo::calc_angle( g::local->eyes( ), pl->eyes( ) ) ),
			g::local->origin( ).dist_to( pl->origin( ) ),
			pl->health( )
			} );

		auto ang = csgo::calc_angle( g::local->eyes( ), pl->origin( ) );
		} );

	if ( targets.empty( ) )
		return;

	std::sort( targets.begin( ), targets.end( ), [ & ] ( potential_target_t& lhs, potential_target_t& rhs ) {
		if ( fabsf( lhs.m_fov - rhs.m_fov ) < 20.0f )
			return lhs.m_health < rhs.m_health;

		return lhs.m_fov < rhs.m_fov;
		} );

	std::for_each( targets.begin( ), targets.end( ), [ & ] ( potential_target_t& target ) {
		vec3_t point;
		float dmg = 0.0f;

		hitscan( target.m_ent, point, dmg, best_rec, best_hitbox );

		if ( dmg && target.m_fov < best_fov ) {
			best_pl = target.m_ent;
			best_dmg = dmg;
			best_fov = target.m_fov;
			best_ang = csgo::calc_angle( g::local->eyes( ), point );
			csgo::clamp( best_ang );
			best_point = point;
		}
		} );

	if ( !best_pl )
		return;

	if ( active_config.auto_shoot && !can_shoot( ) ) {
		ucmd->m_buttons &= ~1;
	}
	else if ( can_shoot( ) ) {
		//const auto backup_origin = best_pl->origin( );
		//auto backup_abs_origin = best_pl->abs_origin( );
		//const auto backup_min = best_pl->mins( );
		//const auto backup_max = best_pl->maxs( );
		//matrix3x4_t backup_bones [ 128 ];
		//std::memcpy( backup_bones, best_pl->bone_cache( ), sizeof matrix3x4_t * best_pl->bone_count( ) );
//
		//best_pl->mins( ) = best_rec.m_min;
		//best_pl->maxs( ) = best_rec.m_max;
		//best_pl->origin( ) = best_rec.m_origin;
		//best_pl->set_abs_origin( best_rec.m_origin );
//
		//if ( get_misses( best_pl->idx( ) ).bad_resolve % 3 == 0 )
		//	std::memcpy( best_pl->bone_cache( ), best_rec.m_bones1, sizeof matrix3x4_t * best_pl->bone_count( ) );
		//else if ( get_misses( best_pl->idx( ) ).bad_resolve % 3 == 1 )
		//	std::memcpy( best_pl->bone_cache( ), best_rec.m_bones2, sizeof matrix3x4_t * best_pl->bone_count( ) );
		//else
		//	std::memcpy( best_pl->bone_cache( ), best_rec.m_bones3, sizeof matrix3x4_t * best_pl->bone_count( ) );

		auto hc = run_hitchance( best_ang, best_pl, best_point, 150, best_hitbox );
		auto should_aim = best_dmg && hc;

		//best_pl->mins( ) = backup_min;
		//best_pl->maxs( ) = backup_max;
		//best_pl->origin( ) = backup_origin;
		//best_pl->set_abs_origin( backup_abs_origin );
		//std::memcpy( best_pl->bone_cache( ), backup_bones, sizeof matrix3x4_t * best_pl->bone_count( ) );

		/* TODO: EXTRAPOLATE POSITION TO SLOW DOWN EXACTLY WHEN WE SHOOT */
		if ( active_config.auto_slow && best_dmg && !hc && g::local->vel( ).length_2d( ) > 0.1f ) {
			const auto vec_move = vec3_t( ucmd->m_fmove, ucmd->m_smove, ucmd->m_umove );
			const auto magnitude = vec_move.length_2d( );
			const auto max_speed = g::local->weapon( )->data( )->m_max_speed;
			const auto move_to_button_ratio = 250.0f / 450.0f;
			const auto speed_ratio = ( max_speed * 0.34f ) * 0.7f;
			const auto move_ratio = speed_ratio * move_to_button_ratio;

			if ( g::local->vel( ).length_2d( ) > g::local->weapon( )->data( )->m_max_speed * 0.34f ) {
				auto vel_ang = csgo::vec_angle( vec_move );
				vel_ang.y = csgo::normalize( vel_ang.y + 180.0f );

				const auto normal = csgo::angle_vec( vel_ang ).normalized( );
				const auto speed_2d = g::local->vel( ).length_2d( );

				old_fmove = normal.x * speed_2d;
				old_smove = normal.y * speed_2d;
			}
			else if ( old_fmove != 0.0f || old_smove != 0.0f ) {
				old_fmove = ( old_fmove / magnitude ) * move_ratio;
				old_smove = ( old_smove / magnitude ) * move_ratio;
			}
		}

		if ( !active_config.auto_shoot )
			should_aim = ucmd->m_buttons & 1 && best_dmg;

		if ( !active_config.auto_shoot && g::local->weapon( )->item_definition_index( ) == 64 )
			should_aim = ucmd->m_buttons & 1 && !revolver_cocking && should_aim;
		else if ( g::local->weapon( )->item_definition_index( ) == 64 )
			should_aim = should_aim && !revolver_cocking;

		if ( should_aim ) {
			revolver_firing = true;

			if ( active_config.auto_shoot ) {
				ucmd->m_buttons |= 1;
				g::send_packet = true;
			}

			if ( active_config.auto_scope && !g::local->scoped( ) &&
				( g::local->weapon( )->item_definition_index( ) == 9
					|| g::local->weapon( )->item_definition_index( ) == 40
					|| g::local->weapon( )->item_definition_index( ) == 38
					|| g::local->weapon( )->item_definition_index( ) == 11 ) )
				ucmd->m_buttons |= 2048;

			auto ang = best_ang - g::local->aim_punch( ) * 2.0f;
			csgo::clamp( ang );

			ucmd->m_angs = ang;

			if ( !active_config.silent )
				csgo::i::engine->set_viewangles( ang );

			best_rec.backtrack( ucmd );

			get_target_pos( best_pl->idx( ) ) = best_point;
			get_target( ) = best_pl;
			get_shots( best_pl->idx( ) )++;
			get_shot_pos( best_pl->idx( ) ) = g::local->eyes( );
			get_lag_rec( best_pl->idx( ) ) = best_rec;
			get_target_idx( ) = best_pl->idx( );
			get_hitbox( best_pl->idx( ) ) = best_hitbox;
		}
		else if ( best_dmg ) {
			if ( g::local->weapon( )->item_definition_index( ) == 9
				|| g::local->weapon( )->item_definition_index( ) == 40
				|| g::local->weapon( )->item_definition_index( ) == 38
				|| g::local->weapon( )->item_definition_index( ) == 11 ) {
				if ( active_config.auto_scope && !g::local->scoped( ) )
					ucmd->m_buttons |= 2048;
				else if ( active_config.auto_scope && g::local->scoped( ) )
					ucmd->m_buttons &= ~2048;
			}
		}
	}
}

void features::ragebot::tickbase_controller( ucmd_t* ucmd ) {
	auto can_shoot = [ & ] ( ) {
		if ( !g::local->weapon( ) || !g::local->weapon( )->ammo( ) || !g::local->weapon( )->data( ) )
			return false;

		if ( g::local->weapon( )->item_definition_index( ) == 64 && !( g::can_fire_revolver || csgo::time2ticks( csgo::i::globals->m_curtime ) > g::cock_ticks ) )
			return false;

		//if ( g::shifted_tickbase )
		//	return csgo::ticks2time ( g::local->tick_base ( ) - ( ( g::shifted_tickbase > 0 ) ? 1 + g::shifted_tickbase : 0 ) ) <= g::local->weapon ( )->next_primary_attack ( );
		return csgo::i::globals->m_curtime >= g::local->next_attack( ) && g::local->weapon( )->next_primary_attack( ) <= csgo::i::globals->m_curtime && g::local->weapon( )->next_primary_attack( ) + g::local->weapon( )->data( )->m_fire_rate <= features::prediction::predicted_curtime;
	};

	/* tickbase manip controller */
	static auto& fd_mode = options::vars [ _( "antiaim.fakeduck_mode" ) ].val.i;
	static auto& fd_enabled = options::vars [ _( "antiaim.fakeduck" ) ].val.b;
	static auto& fd_key = options::vars [ _( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _( "antiaim.fakeduck_key_mode" ) ].val.i;

	const auto weapon_data = ( g::local && g::local->weapon( ) && g::local->weapon( )->data( ) ) ? g::local->weapon( )->data( ) : nullptr;
	const auto fire_rate = weapon_data ? weapon_data->m_fire_rate : 0.0f;
	auto tickbase_as_int = std::clamp< int >( csgo::time2ticks( fire_rate ) - 1, 0, std::clamp( static_cast< int >( active_config.max_dt_ticks ), 0, csgo::is_valve_server( ) ? 8 : 16 ) );

	if ( !active_config.dt_enabled || !utils::keybind_active( active_config.dt_key, active_config.dt_key_mode ) )
		tickbase_as_int = 0;

	if ( g::local && g::local->weapon( ) && g::local->weapon( )->data( ) && tickbase_as_int && ucmd->m_buttons & 1 && can_shoot( ) && std::abs( ucmd->m_cmdnum - g::dt_recharge_time ) > tickbase_as_int && !g::dt_ticks_to_shift && !( g::local->weapon( )->item_definition_index( ) == 64 || g::local->weapon( )->data( )->m_type == 0 || g::local->weapon( )->data( )->m_type >= 7 ) && !( fd_enabled && utils::keybind_active( fd_key, fd_key_mode ) ) ) {
		g::dt_ticks_to_shift = tickbase_as_int;
		g::dt_recharge_time = ucmd->m_cmdnum + tickbase_as_int;
	}
}