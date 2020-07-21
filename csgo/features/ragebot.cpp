#include "ragebot.hpp"
#include <deque>
#include "autowall.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../animations/animations.hpp"
#include "prediction.hpp"
#include "../animations/resolver.hpp"

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
	recorded_scans_t ( const features::lagcomp::lag_record_t& rec, const vec3_t& target, float dmg, int tick, int hitbox, int priority ) {
		m_rec = rec;
		m_target = target;
		m_dmg = dmg;
		m_tick = tick;
		m_hitbox = hitbox;
		m_priority = priority;
	}

	~recorded_scans_t ( ) {

	}

	features::lagcomp::lag_record_t m_rec;
	vec3_t m_target;
	float m_dmg;
	int m_tick, m_hitbox, m_priority;
};

std::array< std::vector< recorded_scans_t >, 65 > previous_scanned_records { { } };

void features::ragebot::get_weapon_config ( weapon_config_t& const config ) {
	if ( !g::local || !g::local->alive( ) || !g::local->weapon ( ) || !g::local->weapon ( )->data ( ) )
		return;

	OPTION ( bool, knife_bot, "Sesame->A->Default->Main->Knife Bot", oxui::object_checkbox );
	OPTION ( bool, zeus_bot, "Sesame->A->Default->Main->Zeus Bot", oxui::object_checkbox );
	OPTION ( bool, ragebot, "Sesame->A->Default->Main->Main Switch", oxui::object_checkbox );
	OPTION ( int, optimization, "Sesame->A->Default->Main->Optimization", oxui::object_dropdown );
	KEYBIND ( tickbase_key, "Sesame->A->Auto->Main->Doubletap Key" );
	OPTION ( int, pistol_inherit_from, "Sesame->A->Pistol->Main->Inherit From", oxui::object_dropdown );
	OPTION ( int, revolver_inherit_from, "Sesame->A->Revolver->Main->Inherit From", oxui::object_dropdown );
	OPTION ( int, rifle_inherit_from, "Sesame->A->Rifle->Main->Inherit From", oxui::object_dropdown );
	OPTION ( int, awp_inherit_from, "Sesame->A->AWP->Main->Inherit From", oxui::object_dropdown );
	OPTION ( int, auto_inherit_from, "Sesame->A->Auto->Main->Inherit From", oxui::object_dropdown );
	OPTION ( int, scout_inherit_from, "Sesame->A->Scout->Main->Inherit From", oxui::object_dropdown );

	auto ignore_newer_hitboxes = false;
	auto config_type = 0;

	if ( g::local->weapon ( )->item_definition_index ( ) == 64 || g::local->weapon ( )->item_definition_index ( ) == 1 ) {
		config_type = 2;
	}
	else if ( g::local->weapon ( )->data ( )->m_type == 1 ) {
		config_type = 1;
	}
	else if ( g::local->weapon ( )->data ( )->m_type == 2 || g::local->weapon ( )->data ( )->m_type == 3 || g::local->weapon ( )->data ( )->m_type == 4 ) {
		config_type = 3;
	}
	else if ( g::local->weapon ( )->data ( )->m_type == 5 ) {
		config_type = 0;

		if ( g::local->weapon ( )->item_definition_index ( ) == 9 )
			config_type = 4;
		else if ( g::local->weapon ( )->item_definition_index ( ) == 38 || g::local->weapon ( )->item_definition_index ( ) == 11 )
			config_type = 5;
		else if ( g::local->weapon ( )->item_definition_index ( ) == 40 )
			config_type = 6;
	}
	//else {
	//	memset ( &config, 0, sizeof config );
	//	config.main_switch = ragebot;
	//	config.knife_bot = knife_bot;
	//	config.zeus_bot = zeus_bot;
	//	return;
	//}

	memset ( &config, 0, sizeof config );

	config.main_switch = ragebot;
	config.dt_key = tickbase_key;
	config.optimization = optimization;

reevaluate_weapon_class:

	/* Default */ {
		OPTION ( bool, head, "Sesame->A->Default->Hitboxes->Head", oxui::object_checkbox );
		OPTION ( bool, neck, "Sesame->A->Default->Hitboxes->Neck", oxui::object_checkbox );
		OPTION ( bool, chest, "Sesame->A->Default->Hitboxes->Chest", oxui::object_checkbox );
		OPTION ( bool, pelvis, "Sesame->A->Default->Hitboxes->Pelvis", oxui::object_checkbox );
		OPTION ( bool, arms, "Sesame->A->Default->Hitboxes->Arms", oxui::object_checkbox );
		OPTION ( bool, legs, "Sesame->A->Default->Hitboxes->Legs", oxui::object_checkbox );
		OPTION ( bool, feet, "Sesame->A->Default->Hitboxes->Feet", oxui::object_checkbox );
		OPTION ( bool, baim_lethal, "Sesame->A->Default->Hitscan->Baim If Lethal", oxui::object_checkbox );
		OPTION ( bool, baim_air, "Sesame->A->Default->Hitscan->Baim If In Air", oxui::object_checkbox );
		OPTION ( double, damage, "Sesame->A->Default->Main->Minimum Damage", oxui::object_slider );
		OPTION ( double, head_ps, "Sesame->A->Default->Hitscan->Head Pointscale", oxui::object_slider );
		OPTION ( double, body_ps, "Sesame->A->Default->Hitscan->Body Pointscale", oxui::object_slider );
		OPTION ( double, baim_after_misses, "Sesame->A->Default->Hitscan->Baim After X Misses", oxui::object_slider );
		OPTION ( bool, safe_point, "Sesame->A->Default->Accuracy->Safe Point", oxui::object_checkbox );
		OPTION ( double, tickbase_shift_amount, "Sesame->A->Default->Main->Maximum Doubletap Ticks", oxui::object_slider );
		OPTION ( bool, silent, "Sesame->A->Default->Main->Silent", oxui::object_checkbox );
		OPTION ( bool, autoshoot, "Sesame->A->Default->Main->Auto Shoot", oxui::object_checkbox );
		OPTION ( double, dt_hitchance, "Sesame->A->Default->Main->Doubletap Hit Chance", oxui::object_slider );
		OPTION ( double, hitchance, "Sesame->A->Default->Main->Hit Chance", oxui::object_slider );
		OPTION ( bool, autoscope, "Sesame->A->Default->Main->Auto Scope", oxui::object_checkbox );
		OPTION ( bool, autoslow, "Sesame->A->Default->Main->Auto Slow", oxui::object_checkbox );
		OPTION ( bool, choke_on_shot, "Sesame->A->Default->Main->Choke On Shot", oxui::object_checkbox );
		OPTION ( bool, dt_teleport, "Sesame->A->Default->Main->Doubletap Teleport", oxui::object_checkbox );
		//OPTION ( double, baim_if_resolver_confidence_less_than, "Sesame->A->Default->Main->Baim If Resolver Confidence Less Than", oxui::object_slider );
		OPTION ( bool, legit_mode, "Sesame->A->Default->Legit->Legit Mode", oxui::object_checkbox );
		OPTION ( bool, triggerbot, "Sesame->A->Default->Legit->Triggerbot", oxui::object_checkbox );
		KEYBIND ( triggerbot_key, "Sesame->A->Default->Legit->Triggerbot Key" );

		if ( !config_type ) {
			if ( !ignore_newer_hitboxes && ( head || neck || chest || pelvis || arms || legs || feet ) ) {
				ignore_newer_hitboxes = true;
				config.scan_head = head;
				config.scan_neck = neck;
				config.scan_chest = chest;
				config.scan_pelvis = pelvis;
				config.scan_arms = arms;
				config.scan_legs = legs;
				config.scan_feet = feet;
			}

			if ( !config.baim_lethal ) config.baim_lethal = baim_lethal;
			if ( !config.dt_teleport ) config.dt_teleport = dt_teleport;
			if ( !config.baim_air ) config.baim_air = baim_air;
			if ( !config.min_dmg ) config.min_dmg = damage;
			if ( !config.head_pointscale ) config.head_pointscale = head_ps;
			if ( !config.body_pointscale ) config.body_pointscale = body_ps;
			if ( !config.baim_after_misses ) config.baim_after_misses = baim_after_misses;
			if ( !config.safe_point ) config.safe_point = safe_point;
			if ( !config.max_dt_ticks ) config.max_dt_ticks = tickbase_shift_amount;
			if ( !config.silent ) config.silent = silent;
			if ( !config.auto_shoot ) config.auto_shoot = autoshoot;
			if ( !config.knife_bot ) config.knife_bot = knife_bot;
			if ( !config.zeus_bot ) config.zeus_bot = zeus_bot;
			if ( !config.dt_hit_chance ) config.dt_hit_chance = dt_hitchance;
			if ( !config.hit_chance ) config.hit_chance = hitchance;
			if ( !config.auto_scope ) config.auto_scope = autoscope;
			if ( !config.auto_slow ) config.auto_slow = autoslow;
			if ( !config.choke_on_shot ) config.choke_on_shot = choke_on_shot;
			//if ( !config.baim_if_resolver_confidence_less_than )config.baim_if_resolver_confidence_less_than = baim_if_resolver_confidence_less_than;
			if ( !config.legit_mode ) config.legit_mode;
			if ( !config.triggerbot ) config.triggerbot;
			if ( !config.triggerbot_key ) config.triggerbot_key;
		}
	}

	/* Revolver */ {
		OPTION ( bool, head, "Sesame->A->Revolver->Hitboxes->Head", oxui::object_checkbox );
		OPTION ( bool, neck, "Sesame->A->Revolver->Hitboxes->Neck", oxui::object_checkbox );
		OPTION ( bool, chest, "Sesame->A->Revolver->Hitboxes->Chest", oxui::object_checkbox );
		OPTION ( bool, pelvis, "Sesame->A->Revolver->Hitboxes->Pelvis", oxui::object_checkbox );
		OPTION ( bool, arms, "Sesame->A->Revolver->Hitboxes->Arms", oxui::object_checkbox );
		OPTION ( bool, legs, "Sesame->A->Revolver->Hitboxes->Legs", oxui::object_checkbox );
		OPTION ( bool, feet, "Sesame->A->Revolver->Hitboxes->Feet", oxui::object_checkbox );
		OPTION ( bool, baim_lethal, "Sesame->A->Revolver->Hitscan->Baim If Lethal", oxui::object_checkbox );
		OPTION ( bool, baim_air, "Sesame->A->Revolver->Hitscan->Baim If In Air", oxui::object_checkbox );
		OPTION ( double, damage, "Sesame->A->Revolver->Main->Minimum Damage", oxui::object_slider );
		OPTION ( double, head_ps, "Sesame->A->Revolver->Hitscan->Head Pointscale", oxui::object_slider );
		OPTION ( double, body_ps, "Sesame->A->Revolver->Hitscan->Body Pointscale", oxui::object_slider );
		OPTION ( double, baim_after_misses, "Sesame->A->Revolver->Hitscan->Baim After X Misses", oxui::object_slider );
		OPTION ( bool, safe_point, "Sesame->A->Revolver->Accuracy->Safe Point", oxui::object_checkbox );
		OPTION ( bool, silent, "Sesame->A->Revolver->Main->Silent", oxui::object_checkbox );
		OPTION ( bool, autoshoot, "Sesame->A->Revolver->Main->Auto Shoot", oxui::object_checkbox );
		OPTION ( double, hitchance, "Sesame->A->Revolver->Main->Hit Chance", oxui::object_slider );
		OPTION ( bool, autoscope, "Sesame->A->Revolver->Main->Auto Scope", oxui::object_checkbox );
		OPTION ( bool, auto_revolver, "Sesame->A->Revolver->Main->Auto Revolver", oxui::object_checkbox );
		OPTION ( bool, choke_on_shot, "Sesame->A->Revolver->Main->Choke On Shot", oxui::object_checkbox );
		OPTION ( bool, autoslow, "Sesame->A->Revolver->Main->Auto Slow", oxui::object_checkbox );
		//OPTION ( double, baim_if_resolver_confidence_less_than, "Sesame->A->Revolver->Main->Baim If Resolver Confidence Less Than", oxui::object_slider );
		OPTION ( bool, legit_mode, "Sesame->A->Revolver->Legit->Legit Mode", oxui::object_checkbox );
		OPTION ( bool, triggerbot, "Sesame->A->Revolver->Legit->Triggerbot", oxui::object_checkbox );
		KEYBIND ( triggerbot_key, "Sesame->A->Revolver->Legit->Triggerbot Key" );

		if ( config_type == 2 ) {
			if ( !ignore_newer_hitboxes && ( head || neck || chest || pelvis || arms || legs || feet ) ) {
				ignore_newer_hitboxes = true;
				config.scan_head = head;
				config.scan_neck = neck;
				config.scan_chest = chest;
				config.scan_pelvis = pelvis;
				config.scan_arms = arms;
				config.scan_legs = legs;
				config.scan_feet = feet;
			}

			config.auto_revolver = auto_revolver;
			if ( !config.baim_lethal ) config.baim_lethal = baim_lethal;
			if ( !config.baim_air ) config.baim_air = baim_air;
			if ( !config.min_dmg ) config.min_dmg = damage;
			if ( !config.head_pointscale ) config.head_pointscale = head_ps;
			if ( !config.body_pointscale ) config.body_pointscale = body_ps;
			if ( !config.baim_after_misses ) config.baim_after_misses = baim_after_misses;
			if ( !config.safe_point ) config.safe_point = safe_point;
			if ( !config.silent ) config.silent = silent;
			if ( !config.auto_shoot ) config.auto_shoot = autoshoot;
			if ( !config.hit_chance ) config.hit_chance = hitchance;
			if ( !config.auto_scope ) config.auto_scope = autoscope;
			if ( !config.auto_slow ) config.auto_slow = autoslow;
			if ( !config.choke_on_shot ) config.choke_on_shot = choke_on_shot;
			//if ( !config.baim_if_resolver_confidence_less_than )config.baim_if_resolver_confidence_less_than = baim_if_resolver_confidence_less_than;
			if ( !config.legit_mode ) config.legit_mode;
			if ( !config.triggerbot ) config.triggerbot;
			if ( !config.triggerbot_key ) config.triggerbot_key;
			config.dt_key = 0;
			if ( revolver_inherit_from && revolver_inherit_from - 1 != config_type ) {
				config_type = revolver_inherit_from - 1;
				goto reevaluate_weapon_class;
			}
		}
	}
	
	/* Pistol */ {
		OPTION ( bool, head, "Sesame->A->Pistol->Hitboxes->Head", oxui::object_checkbox );
		OPTION ( bool, neck, "Sesame->A->Pistol->Hitboxes->Neck", oxui::object_checkbox );
		OPTION ( bool, chest, "Sesame->A->Pistol->Hitboxes->Chest", oxui::object_checkbox );
		OPTION ( bool, pelvis, "Sesame->A->Pistol->Hitboxes->Pelvis", oxui::object_checkbox );
		OPTION ( bool, arms, "Sesame->A->Pistol->Hitboxes->Arms", oxui::object_checkbox );
		OPTION ( bool, legs, "Sesame->A->Pistol->Hitboxes->Legs", oxui::object_checkbox );
		OPTION ( bool, feet, "Sesame->A->Pistol->Hitboxes->Feet", oxui::object_checkbox );
		OPTION ( bool, baim_lethal, "Sesame->A->Pistol->Hitscan->Baim If Lethal", oxui::object_checkbox );
		OPTION ( bool, baim_air, "Sesame->A->Pistol->Hitscan->Baim If In Air", oxui::object_checkbox );
		OPTION ( double, damage, "Sesame->A->Pistol->Main->Minimum Damage", oxui::object_slider );
		OPTION ( double, head_ps, "Sesame->A->Pistol->Hitscan->Head Pointscale", oxui::object_slider );
		OPTION ( double, body_ps, "Sesame->A->Pistol->Hitscan->Body Pointscale", oxui::object_slider );
		OPTION ( double, baim_after_misses, "Sesame->A->Pistol->Hitscan->Baim After X Misses", oxui::object_slider );
		OPTION ( bool, safe_point, "Sesame->A->Pistol->Accuracy->Safe Point", oxui::object_checkbox );
		OPTION ( double, tickbase_shift_amount, "Sesame->A->Pistol->Main->Maximum Doubletap Ticks", oxui::object_slider );
		OPTION ( bool, silent, "Sesame->A->Pistol->Main->Silent", oxui::object_checkbox );
		OPTION ( bool, autoshoot, "Sesame->A->Pistol->Main->Auto Shoot", oxui::object_checkbox );
		OPTION ( double, dt_hitchance, "Sesame->A->Pistol->Main->Doubletap Hit Chance", oxui::object_slider );
		OPTION ( double, hitchance, "Sesame->A->Pistol->Main->Hit Chance", oxui::object_slider );
		OPTION ( bool, autoscope, "Sesame->A->Pistol->Main->Auto Scope", oxui::object_checkbox );
		OPTION ( bool, autoslow, "Sesame->A->Pistol->Main->Auto Slow", oxui::object_checkbox );
		OPTION ( bool, choke_on_shot, "Sesame->A->Pistol->Main->Choke On Shot", oxui::object_checkbox );
		OPTION ( bool, dt_teleport, "Sesame->A->Pistol->Main->Doubletap Teleport", oxui::object_checkbox );
		//OPTION ( double, baim_if_resolver_confidence_less_than, "Sesame->A->Pistol->Main->Baim If Resolver Confidence Less Than", oxui::object_slider );
		OPTION ( bool, legit_mode, "Sesame->A->Pistol->Legit->Legit Mode", oxui::object_checkbox );
		OPTION ( bool, triggerbot, "Sesame->A->Pistol->Legit->Triggerbot", oxui::object_checkbox );
		KEYBIND ( triggerbot_key, "Sesame->A->Pistol->Legit->Triggerbot Key" );

		if ( config_type == 1 ) {
			if ( !ignore_newer_hitboxes && ( head || neck || chest || pelvis || arms || legs || feet ) ) {
				ignore_newer_hitboxes = true;
				config.scan_head = head;
				config.scan_neck = neck;
				config.scan_chest = chest;
				config.scan_pelvis = pelvis;
				config.scan_arms = arms;
				config.scan_legs = legs;
				config.scan_feet = feet;
			}

			if ( !config.dt_teleport ) config.dt_teleport = dt_teleport;
			if ( !config.baim_lethal ) config.baim_lethal = baim_lethal;
			if ( !config.baim_air ) config.baim_air = baim_air;
			if ( !config.min_dmg ) config.min_dmg = damage;
			if ( !config.head_pointscale ) config.head_pointscale = head_ps;
			if ( !config.body_pointscale ) config.body_pointscale = body_ps;
			if ( !config.baim_after_misses ) config.baim_after_misses = baim_after_misses;
			if ( !config.safe_point ) config.safe_point = safe_point;
			if ( !config.max_dt_ticks ) config.max_dt_ticks = tickbase_shift_amount;
			if ( !config.silent ) config.silent = silent;
			if ( !config.auto_shoot ) config.auto_shoot = autoshoot;
			if ( !config.dt_hit_chance ) config.dt_hit_chance = dt_hitchance;
			if ( !config.hit_chance ) config.hit_chance = hitchance;
			if ( !config.auto_scope ) config.auto_scope = autoscope;
			if ( !config.auto_slow ) config.auto_slow = autoslow;
			if ( !config.choke_on_shot ) config.choke_on_shot = choke_on_shot;
			//if ( !config.baim_if_resolver_confidence_less_than )config.baim_if_resolver_confidence_less_than = baim_if_resolver_confidence_less_than;
			if ( !config.legit_mode ) config.legit_mode;
			if ( !config.triggerbot ) config.triggerbot;
			if ( !config.triggerbot_key ) config.triggerbot_key;
			if ( pistol_inherit_from && pistol_inherit_from - 1 != config_type ) {
				config_type = pistol_inherit_from - 1;
				goto reevaluate_weapon_class;
			}
		}
	}

	/* Rifle */ {
		OPTION ( bool, head, "Sesame->A->Rifle->Hitboxes->Head", oxui::object_checkbox );
		OPTION ( bool, neck, "Sesame->A->Rifle->Hitboxes->Neck", oxui::object_checkbox );
		OPTION ( bool, chest, "Sesame->A->Rifle->Hitboxes->Chest", oxui::object_checkbox );
		OPTION ( bool, pelvis, "Sesame->A->Rifle->Hitboxes->Pelvis", oxui::object_checkbox );
		OPTION ( bool, arms, "Sesame->A->Rifle->Hitboxes->Arms", oxui::object_checkbox );
		OPTION ( bool, legs, "Sesame->A->Rifle->Hitboxes->Legs", oxui::object_checkbox );
		OPTION ( bool, feet, "Sesame->A->Rifle->Hitboxes->Feet", oxui::object_checkbox );
		OPTION ( bool, baim_lethal, "Sesame->A->Rifle->Hitscan->Baim If Lethal", oxui::object_checkbox );
		OPTION ( bool, baim_air, "Sesame->A->Rifle->Hitscan->Baim If In Air", oxui::object_checkbox );
		OPTION ( double, damage, "Sesame->A->Rifle->Main->Minimum Damage", oxui::object_slider );
		OPTION ( double, head_ps, "Sesame->A->Rifle->Hitscan->Head Pointscale", oxui::object_slider );
		OPTION ( double, body_ps, "Sesame->A->Rifle->Hitscan->Body Pointscale", oxui::object_slider );
		OPTION ( double, baim_after_misses, "Sesame->A->Rifle->Hitscan->Baim After X Misses", oxui::object_slider );
		OPTION ( bool, safe_point, "Sesame->A->Rifle->Accuracy->Safe Point", oxui::object_checkbox );
		OPTION ( double, tickbase_shift_amount, "Sesame->A->Rifle->Main->Maximum Doubletap Ticks", oxui::object_slider );
		OPTION ( bool, silent, "Sesame->A->Rifle->Main->Silent", oxui::object_checkbox );
		OPTION ( bool, autoshoot, "Sesame->A->Rifle->Main->Auto Shoot", oxui::object_checkbox );
		OPTION ( double, dt_hitchance, "Sesame->A->Rifle->Main->Doubletap Hit Chance", oxui::object_slider );
		OPTION ( double, hitchance, "Sesame->A->Rifle->Main->Hit Chance", oxui::object_slider );
		OPTION ( bool, autoscope, "Sesame->A->Rifle->Main->Auto Scope", oxui::object_checkbox );
		OPTION ( bool, autoslow, "Sesame->A->Rifle->Main->Auto Slow", oxui::object_checkbox );
		OPTION ( bool, choke_on_shot, "Sesame->A->Rifle->Main->Choke On Shot", oxui::object_checkbox );
		OPTION ( bool, dt_teleport, "Sesame->A->Rifle->Main->Doubletap Teleport", oxui::object_checkbox );
		//OPTION ( double, baim_if_resolver_confidence_less_than, "Sesame->A->Rifle->Main->Baim If Resolver Confidence Less Than", oxui::object_slider );
		OPTION ( bool, legit_mode, "Sesame->A->Rifle->Legit->Legit Mode", oxui::object_checkbox );
		OPTION ( bool, triggerbot, "Sesame->A->Rifle->Legit->Triggerbot", oxui::object_checkbox );
		KEYBIND ( triggerbot_key, "Sesame->A->Rifle->Legit->Triggerbot Key" );

		if ( config_type == 3 ) {
			if ( !ignore_newer_hitboxes && ( head || neck || chest || pelvis || arms || legs || feet ) ) {
				ignore_newer_hitboxes = true;
				config.scan_head = head;
				config.scan_neck = neck;
				config.scan_chest = chest;
				config.scan_pelvis = pelvis;
				config.scan_arms = arms;
				config.scan_legs = legs;
				config.scan_feet = feet;
			}

			if ( !config.dt_teleport ) config.dt_teleport = dt_teleport;
			if ( !config.baim_lethal ) config.baim_lethal = baim_lethal;
			if ( !config.baim_air ) config.baim_air = baim_air;
			if ( !config.min_dmg ) config.min_dmg = damage;
			if ( !config.head_pointscale ) config.head_pointscale = head_ps;
			if ( !config.body_pointscale ) config.body_pointscale = body_ps;
			if ( !config.baim_after_misses ) config.baim_after_misses = baim_after_misses;
			if ( !config.safe_point ) config.safe_point = safe_point;
			if ( !config.max_dt_ticks ) config.max_dt_ticks = tickbase_shift_amount;
			if ( !config.silent ) config.silent = silent;
			if ( !config.auto_shoot ) config.auto_shoot = autoshoot;
			if ( !config.dt_hit_chance ) config.dt_hit_chance = dt_hitchance;
			if ( !config.hit_chance ) config.hit_chance = hitchance;
			if ( !config.auto_scope ) config.auto_scope = autoscope;
			if ( !config.auto_slow ) config.auto_slow = autoslow;
			if ( !config.choke_on_shot ) config.choke_on_shot = choke_on_shot;
		//	if ( !config.baim_if_resolver_confidence_less_than )config.baim_if_resolver_confidence_less_than = baim_if_resolver_confidence_less_than;
			if ( !config.legit_mode ) config.legit_mode;
			if ( !config.triggerbot ) config.triggerbot;
			if ( !config.triggerbot_key ) config.triggerbot_key;
			if ( rifle_inherit_from && rifle_inherit_from - 1 != config_type ) {
				config_type = rifle_inherit_from - 1;
				goto reevaluate_weapon_class;
			}
		}
	}

	/* AWP */ {
		OPTION ( bool, head, "Sesame->A->AWP->Hitboxes->Head", oxui::object_checkbox );
		OPTION ( bool, neck, "Sesame->A->AWP->Hitboxes->Neck", oxui::object_checkbox );
		OPTION ( bool, chest, "Sesame->A->AWP->Hitboxes->Chest", oxui::object_checkbox );
		OPTION ( bool, pelvis, "Sesame->A->AWP->Hitboxes->Pelvis", oxui::object_checkbox );
		OPTION ( bool, arms, "Sesame->A->AWP->Hitboxes->Arms", oxui::object_checkbox );
		OPTION ( bool, legs, "Sesame->A->AWP->Hitboxes->Legs", oxui::object_checkbox );
		OPTION ( bool, feet, "Sesame->A->AWP->Hitboxes->Feet", oxui::object_checkbox );
		OPTION ( bool, baim_lethal, "Sesame->A->AWP->Hitscan->Baim If Lethal", oxui::object_checkbox );
		OPTION ( bool, baim_air, "Sesame->A->AWP->Hitscan->Baim If In Air", oxui::object_checkbox );
		OPTION ( double, damage, "Sesame->A->AWP->Main->Minimum Damage", oxui::object_slider );
		OPTION ( double, head_ps, "Sesame->A->AWP->Hitscan->Head Pointscale", oxui::object_slider );
		OPTION ( double, body_ps, "Sesame->A->AWP->Hitscan->Body Pointscale", oxui::object_slider );
		OPTION ( double, baim_after_misses, "Sesame->A->AWP->Hitscan->Baim After X Misses", oxui::object_slider );
		OPTION ( bool, safe_point, "Sesame->A->AWP->Accuracy->Safe Point", oxui::object_checkbox );
		OPTION ( bool, silent, "Sesame->A->AWP->Main->Silent", oxui::object_checkbox );
		OPTION ( bool, autoshoot, "Sesame->A->AWP->Main->Auto Shoot", oxui::object_checkbox );
		OPTION ( double, hitchance, "Sesame->A->AWP->Main->Hit Chance", oxui::object_slider );
		OPTION ( bool, autoscope, "Sesame->A->AWP->Main->Auto Scope", oxui::object_checkbox );
		OPTION ( bool, choke_on_shot, "Sesame->A->AWP->Main->Choke On Shot", oxui::object_checkbox );
		OPTION ( bool, autoslow, "Sesame->A->AWP->Main->Auto Slow", oxui::object_checkbox );
		//OPTION ( double, baim_if_resolver_confidence_less_than, "Sesame->A->AWP->Main->Baim If Resolver Confidence Less Than", oxui::object_slider );
		OPTION ( bool, legit_mode, "Sesame->A->AWP->Legit->Legit Mode", oxui::object_checkbox );
		OPTION ( bool, triggerbot, "Sesame->A->AWP->Legit->Triggerbot", oxui::object_checkbox );
		KEYBIND ( triggerbot_key, "Sesame->A->AWP->Legit->Triggerbot Key" );

		if ( config_type == 4 ) {
			if ( !ignore_newer_hitboxes && ( head || neck || chest || pelvis || arms || legs || feet ) ) {
				ignore_newer_hitboxes = true;
				config.scan_head = head;
				config.scan_neck = neck;
				config.scan_chest = chest;
				config.scan_pelvis = pelvis;
				config.scan_arms = arms;
				config.scan_legs = legs;
				config.scan_feet = feet;
			}

			if ( !config.baim_lethal ) config.baim_lethal = baim_lethal;
			if ( !config.baim_air ) config.baim_air = baim_air;
			if ( !config.min_dmg ) config.min_dmg = damage;
			if ( !config.head_pointscale ) config.head_pointscale = head_ps;
			if ( !config.body_pointscale ) config.body_pointscale = body_ps;
			if ( !config.baim_after_misses ) config.baim_after_misses = baim_after_misses;
			if ( !config.safe_point ) config.safe_point = safe_point;
			if ( !config.silent ) config.silent = silent;
			if ( !config.auto_shoot ) config.auto_shoot = autoshoot;
			if ( !config.hit_chance ) config.hit_chance = hitchance;
			if ( !config.auto_scope ) config.auto_scope = autoscope;
			if ( !config.auto_slow ) config.auto_slow = autoslow;
			if ( !config.choke_on_shot ) config.choke_on_shot = choke_on_shot;
			//if ( !config.baim_if_resolver_confidence_less_than )config.baim_if_resolver_confidence_less_than = baim_if_resolver_confidence_less_than;
			if ( !config.legit_mode ) config.legit_mode;
			if ( !config.triggerbot ) config.triggerbot;
			if ( !config.triggerbot_key ) config.triggerbot_key;
			config.dt_key = 0;
			if ( awp_inherit_from && awp_inherit_from - 1 != config_type ) {
				config_type = awp_inherit_from - 1;
				goto reevaluate_weapon_class;
			}
		}
	}

	/* Auto */ {
		OPTION ( bool, head, "Sesame->A->Auto->Hitboxes->Head", oxui::object_checkbox );
		OPTION ( bool, neck, "Sesame->A->Auto->Hitboxes->Neck", oxui::object_checkbox );
		OPTION ( bool, chest, "Sesame->A->Auto->Hitboxes->Chest", oxui::object_checkbox );
		OPTION ( bool, pelvis, "Sesame->A->Auto->Hitboxes->Pelvis", oxui::object_checkbox );
		OPTION ( bool, arms, "Sesame->A->Auto->Hitboxes->Arms", oxui::object_checkbox );
		OPTION ( bool, legs, "Sesame->A->Auto->Hitboxes->Legs", oxui::object_checkbox );
		OPTION ( bool, feet, "Sesame->A->Auto->Hitboxes->Feet", oxui::object_checkbox );
		OPTION ( bool, baim_lethal, "Sesame->A->Auto->Hitscan->Baim If Lethal", oxui::object_checkbox );
		OPTION ( bool, baim_air, "Sesame->A->Auto->Hitscan->Baim If In Air", oxui::object_checkbox );
		OPTION ( double, damage, "Sesame->A->Auto->Main->Minimum Damage", oxui::object_slider );
		OPTION ( double, head_ps, "Sesame->A->Auto->Hitscan->Head Pointscale", oxui::object_slider );
		OPTION ( double, body_ps, "Sesame->A->Auto->Hitscan->Body Pointscale", oxui::object_slider );
		OPTION ( double, baim_after_misses, "Sesame->A->Auto->Hitscan->Baim After X Misses", oxui::object_slider );
		OPTION ( bool, safe_point, "Sesame->A->Auto->Accuracy->Safe Point", oxui::object_checkbox );
		OPTION ( double, tickbase_shift_amount, "Sesame->A->Auto->Main->Maximum Doubletap Ticks", oxui::object_slider );
		OPTION ( bool, silent, "Sesame->A->Auto->Main->Silent", oxui::object_checkbox );
		OPTION ( bool, autoshoot, "Sesame->A->Auto->Main->Auto Shoot", oxui::object_checkbox );
		OPTION ( double, dt_hitchance, "Sesame->A->Auto->Main->Doubletap Hit Chance", oxui::object_slider );
		OPTION ( double, hitchance, "Sesame->A->Auto->Main->Hit Chance", oxui::object_slider );
		OPTION ( bool, autoscope, "Sesame->A->Auto->Main->Auto Scope", oxui::object_checkbox );
		OPTION ( bool, autoslow, "Sesame->A->Auto->Main->Auto Slow", oxui::object_checkbox );
		OPTION ( bool, choke_on_shot, "Sesame->A->Auto->Main->Choke On Shot", oxui::object_checkbox );
		OPTION ( bool, dt_teleport, "Sesame->A->Auto->Main->Doubletap Teleport", oxui::object_checkbox );
		//OPTION ( double, baim_if_resolver_confidence_less_than, "Sesame->A->Auto->Main->Baim If Resolver Confidence Less Than", oxui::object_slider );
		OPTION ( bool, legit_mode, "Sesame->A->Auto->Legit->Legit Mode", oxui::object_checkbox );
		OPTION ( bool, triggerbot, "Sesame->A->Auto->Legit->Triggerbot", oxui::object_checkbox );
		KEYBIND ( triggerbot_key, "Sesame->A->Auto->Legit->Triggerbot Key" );

		if ( config_type == 5 ) {
			if ( !ignore_newer_hitboxes && ( head || neck || chest || pelvis || arms || legs || feet ) ) {
				ignore_newer_hitboxes = true;
				config.scan_head = head;
				config.scan_neck = neck;
				config.scan_chest = chest;
				config.scan_pelvis = pelvis;
				config.scan_arms = arms;
				config.scan_legs = legs;
				config.scan_feet = feet;
			}

			if ( !config.dt_teleport ) config.dt_teleport = dt_teleport;
			if ( !config.max_dt_ticks ) config.max_dt_ticks = tickbase_shift_amount;
			if ( !config.baim_lethal ) config.baim_lethal = baim_lethal;
			if ( !config.dt_hit_chance ) config.dt_hit_chance = dt_hitchance;
			if ( !config.baim_air ) config.baim_air = baim_air;
			if ( !config.min_dmg ) config.min_dmg = damage;
			if ( !config.head_pointscale ) config.head_pointscale = head_ps;
			if ( !config.body_pointscale ) config.body_pointscale = body_ps;
			if ( !config.baim_after_misses ) config.baim_after_misses = baim_after_misses;
			if ( !config.safe_point ) config.safe_point = safe_point;
			if ( !config.silent ) config.silent = silent;
			if ( !config.auto_shoot ) config.auto_shoot = autoshoot;
			if ( !config.hit_chance ) config.hit_chance = hitchance;
			if ( !config.auto_scope ) config.auto_scope = autoscope;
			if ( !config.auto_slow ) config.auto_slow = autoslow;
			if ( !config.choke_on_shot ) config.choke_on_shot = choke_on_shot;
			//if ( !config.baim_if_resolver_confidence_less_than )config.baim_if_resolver_confidence_less_than = baim_if_resolver_confidence_less_than;
			if ( !config.legit_mode ) config.legit_mode;
			if ( !config.triggerbot ) config.triggerbot;
			if ( !config.triggerbot_key ) config.triggerbot_key;
			if ( auto_inherit_from && auto_inherit_from - 1 != config_type ) {
				config_type = auto_inherit_from - 1;
				goto reevaluate_weapon_class;
			}
		}
	}

	/* Scout */ {
		OPTION ( bool, head, "Sesame->A->Scout->Hitboxes->Head", oxui::object_checkbox );
		OPTION ( bool, neck, "Sesame->A->Scout->Hitboxes->Neck", oxui::object_checkbox );
		OPTION ( bool, chest, "Sesame->A->Scout->Hitboxes->Chest", oxui::object_checkbox );
		OPTION ( bool, pelvis, "Sesame->A->Scout->Hitboxes->Pelvis", oxui::object_checkbox );
		OPTION ( bool, arms, "Sesame->A->Scout->Hitboxes->Arms", oxui::object_checkbox );
		OPTION ( bool, legs, "Sesame->A->Scout->Hitboxes->Legs", oxui::object_checkbox );
		OPTION ( bool, feet, "Sesame->A->Scout->Hitboxes->Feet", oxui::object_checkbox );
		OPTION ( bool, baim_lethal, "Sesame->A->Scout->Hitscan->Baim If Lethal", oxui::object_checkbox );
		OPTION ( bool, baim_air, "Sesame->A->Scout->Hitscan->Baim If In Air", oxui::object_checkbox );
		OPTION ( double, damage, "Sesame->A->Scout->Main->Minimum Damage", oxui::object_slider );
		OPTION ( double, head_ps, "Sesame->A->Scout->Hitscan->Head Pointscale", oxui::object_slider );
		OPTION ( double, body_ps, "Sesame->A->Scout->Hitscan->Body Pointscale", oxui::object_slider );
		OPTION ( double, baim_after_misses, "Sesame->A->Scout->Hitscan->Baim After X Misses", oxui::object_slider );
		OPTION ( bool, safe_point, "Sesame->A->Scout->Accuracy->Safe Point", oxui::object_checkbox );
		OPTION ( bool, silent, "Sesame->A->Scout->Main->Silent", oxui::object_checkbox );
		OPTION ( bool, autoshoot, "Sesame->A->Scout->Main->Auto Shoot", oxui::object_checkbox );
		OPTION ( double, hitchance, "Sesame->A->Scout->Main->Hit Chance", oxui::object_slider );
		OPTION ( bool, autoscope, "Sesame->A->Scout->Main->Auto Scope", oxui::object_checkbox );
		OPTION ( bool, choke_on_shot, "Sesame->A->Scout->Main->Choke On Shot", oxui::object_checkbox );
		OPTION ( bool, autoslow, "Sesame->A->Scout->Main->Auto Slow", oxui::object_checkbox );
		//OPTION ( double, baim_if_resolver_confidence_less_than, "Sesame->A->Scout->Main->Baim If Resolver Confidence Less Than", oxui::object_slider );
		OPTION ( bool, legit_mode, "Sesame->A->Scout->Legit->Legit Mode", oxui::object_checkbox );
		OPTION ( bool, triggerbot, "Sesame->A->Scout->Legit->Triggerbot", oxui::object_checkbox );
		KEYBIND ( triggerbot_key, "Sesame->A->Scout->Legit->Triggerbot Key" );

		if ( config_type == 6 ) {
			if ( !ignore_newer_hitboxes && ( head || neck || chest || pelvis || arms || legs || feet ) ) {
				ignore_newer_hitboxes = true;
				config.scan_head = head;
				config.scan_neck = neck;
				config.scan_chest = chest;
				config.scan_pelvis = pelvis;
				config.scan_arms = arms;
				config.scan_legs = legs;
				config.scan_feet = feet;
			}

			if ( !config.baim_lethal ) config.baim_lethal = baim_lethal;
			if ( !config.baim_air ) config.baim_air = baim_air;
			if ( !config.min_dmg ) config.min_dmg = damage;
			if ( !config.head_pointscale ) config.head_pointscale = head_ps;
			if ( !config.body_pointscale ) config.body_pointscale = body_ps;
			if ( !config.baim_after_misses ) config.baim_after_misses = baim_after_misses;
			if ( !config.safe_point ) config.safe_point = safe_point;
			if ( !config.silent ) config.silent = silent;
			if ( !config.auto_shoot ) config.auto_shoot = autoshoot;
			if ( !config.hit_chance ) config.hit_chance = hitchance;
			if ( !config.auto_scope ) config.auto_scope = autoscope;
			if ( !config.auto_slow ) config.auto_slow = autoslow;
			if ( !config.choke_on_shot ) config.choke_on_shot = choke_on_shot;
			//if ( !config.baim_if_resolver_confidence_less_than )config.baim_if_resolver_confidence_less_than = baim_if_resolver_confidence_less_than;
			if ( !config.legit_mode ) config.legit_mode;
			if ( !config.triggerbot ) config.triggerbot;
			if ( !config.triggerbot_key ) config.triggerbot_key;
			config.dt_key = 0;
			if ( scout_inherit_from && scout_inherit_from - 1 != config_type ) {
				config_type = scout_inherit_from - 1;
				goto reevaluate_weapon_class;
			}
		}
	}
}

int& features::ragebot::get_target_idx ( ) {
	return target_idx;
}

features::lagcomp::lag_record_t& features::ragebot::get_lag_rec ( int pl ) {
	return cur_lag_rec [ pl ];
}

player_t* &features::ragebot::get_target( ) {
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

int& features::ragebot::get_shots ( int pl ) {
	return shots [ pl ];
}

int& features::ragebot::get_hitbox ( int pl ) {
	return hitbox [ pl ];
}

bool run_hitchance ( vec3_t ang, player_t* pl, vec3_t point, int rays, int hitbox ) {
	auto weapon = g::local->weapon ( );

	if ( !weapon || !weapon->data ( ) ) {
		features::ragebot::hitchances [ pl->idx ( ) ] = 0.0f;
		return false;
	}

	auto src = g::local->eyes ( );
	csgo::clamp ( ang );
	auto forward = csgo::angle_vec ( ang );
	auto right = csgo::angle_vec ( ang + vec3_t ( 0.0f, 90.0f, 0.0f ) );
	auto up = csgo::angle_vec ( ang + vec3_t ( 90.0f, 0.0f, 0.0f ) );

	forward.normalize ( );
	right.normalize ( );
	up.normalize ( );

	auto hits = 0;
	auto needed_hits = static_cast< int >( static_cast< float > ( rays )* ( features::ragebot::active_config.hit_chance / 100.0f ) );

	weapon->update_accuracy ( );

	auto weap_spread = weapon->inaccuracy ( ) + weapon->spread ( );

	const auto as_hitgroup = autowall::hitbox_to_hitgroup ( hitbox );

	for ( auto i = 0; i < rays; i++ ) {
		const auto spread_x = -weap_spread * 0.5f + ( ( static_cast < float > ( rand ( ) ) / static_cast < float > ( RAND_MAX ) )* weap_spread );
		const auto spread_y = -weap_spread * 0.5f + ( ( static_cast < float > ( rand ( ) ) / static_cast < float > ( RAND_MAX ) )* weap_spread );
		const auto spread_z = -weap_spread * 0.5f + ( ( static_cast < float > ( rand ( ) ) / static_cast < float > ( RAND_MAX ) )* weap_spread );
		const auto final_pos = src + ( ( forward + vec3_t ( spread_x, spread_y, spread_z ) ) * weapon->data ( )->m_range );

		trace_t tr;
		ray_t ray;

		ray.init ( src, final_pos );
		csgo::i::trace->clip_ray_to_entity ( ray, mask_shot, pl, &tr );

		// dbg_print( "%3.f\n", dst );

		if ( tr.m_hit_entity == pl && tr.m_hitgroup == as_hitgroup )
			hits++;

		//if ( ray_intersects_sphere( src, final_pos, point, 5.0f ) )
		//	hits++;
	}

	const auto calculated_chance = ( static_cast< float >( hits ) / static_cast< float > ( rays ) ) * 100.0f;
	features::ragebot::hitchances [ pl->idx ( ) ] = calculated_chance;
	return calculated_chance >= ( g::next_tickbase_shot ? features::ragebot::active_config.dt_hit_chance : features::ragebot::active_config.hit_chance );
}

void features::ragebot::hitscan( player_t* pl, vec3_t& point, float& dmg, lagcomp::lag_record_t& rec_out, int& hitbox_out) {
	OPTION ( bool, safe_point, "Sesame->A->Default->Accuracy->Safe Point", oxui::object_checkbox );
	KEYBIND ( safe_point_key, "Sesame->A->Default->Accuracy->Safe Point Key" );
	//OPTION ( bool, predict_fakelag, "Sesame->A->Rage Aimbot->Accuracy->Predict Fakelag", oxui::object_checkbox );

	const auto recs = lagcomp::get( pl );
	//auto extrapolated = lagcomp::get_extrapolated( pl );
	auto shot = lagcomp::get_shot( pl );

	dmg = 0.0f;

	if ( !g::local || !g::local->weapon ( ) || !pl->valid( ) || !pl->weapon( ) || !pl->weapon( )->data( ) || !pl->bone_accessor ( ).get_bone_arr_for_write ( ) || !pl->layers ( ) || !pl->animstate( ) || ( !recs.second && !shot.second ) )
		return;

	//build_record_bones ( extrapolated.first );
	//build_record_bones ( shot.first );

	const auto backup_origin = pl->origin( );
	auto backup_abs_origin = pl->abs_origin( );
	const auto backup_min = pl->mins( );
	const auto backup_max = pl->maxs( );
	matrix3x4_t backup_bones [ 128 ];
	std::memcpy( backup_bones, pl->bone_accessor( ).get_bone_arr_for_write( ), sizeof matrix3x4_t * pl->bone_count( ) );

	auto newest_moving_tick = 0;
	std::deque < lagcomp::lag_record_t > best_recs { };

	auto head_only = false;
	
	///* added a ton of tiny optimization features to imcrease speed and performance */
	///* if we have a shot, we don't have to look at other records... tap their head off if it's visible */
	//if ( shot.second && csgo::is_visible( shot.first.m_bones [ 8 ].origin( ) ) ) {
	//	shot.first.m_priority = 2;
	//	best_recs.push_back( shot.first );
	//	head_only = true;
	//}
	//else {
	//	if ( shot.second ) {
	//		shot.first.m_priority = 2;
	//		best_recs.push_back( shot.first );
	//	}
	//
	//	/* if extrapolated rec is visible, negate all other records */
	//	/*if ( extrapolated.second && mode >= 2
	//		&& ( csgo::is_visible( extrapolated.first.m_bones [ 3 ].origin( ) )
	//			|| csgo::is_visible( extrapolated.first.m_bones [ 8 ].origin( ) ) ) ) {
	//		shot.first.m_priority = 0;
	//		best_recs.push_back( extrapolated.first );
	//	}
	//	else */{
	//		//if ( extrapolated.second && mode >= 2 ) {
	//		//	shot.first.m_priority = 0;
	//		//	best_recs.push_back( extrapolated.first );
	//		//}
	//
	//		/* get records by priority */
	//		std::for_each( recs.first.begin( ), recs.first.end( ), [ & ] ( lagcomp::lag_record_t& rec ) {
	//			//build_record_bones ( rec );
	//
	//			rec.m_priority = 1;
	//
	//			/* newest (might be exposed) */
	//			if ( rec.m_tick == recs.first.front( ).m_tick )
	//				best_recs.push_back( rec );
	//
	//			/* oldest (hit old position) */
	//			if ( recs.first.size( ) >= 2 && rec.m_tick == recs.first.back( ).m_tick )
	//				best_recs.push_back( rec );
	//
	//			/* moving on ground at speed2d > max_speed * 0.34f (lower desync range) */
	//			if ( rec.m_tick != recs.first.begin( )->m_tick && rec.m_state.m_speed2d > pl->weapon( )->data( )->m_max_speed * 0.5f && rec.m_state.m_hit_ground && rec.m_tick > newest_moving_tick ) {
	//				if ( !best_recs.empty( ) && newest_moving_tick ) {
	//					auto rec_num = best_recs.begin( );
	//
	//					std::for_each( best_recs.begin( ), best_recs.end( ), [ & ] ( const lagcomp::lag_record_t& replaceable ) {
	//						if ( replaceable.m_tick == newest_moving_tick )
	//							best_recs.erase( rec_num );
	//
	//						rec_num++;
	//					} );
	//				}
	//
	//				/* prioritize definite records, but aim at extrapolated data if it's our only option */
	//				best_recs.push_back( rec );
	//				newest_moving_tick = rec.m_tick;
	//			}
	//		} );
	//	}
	//}

	if ( shot.second ) {
		best_recs.push_back ( shot.first );
		best_recs.push_back ( recs.first.back ( ) );
		head_only = true;
	}
	else {
		/*if ( !lagcomp::data::extrapolated_records [ pl->idx ( ) ].empty ( ) && csgo::time2ticks ( csgo::i::globals->m_curtime - pl->simtime ( ) ) > 2 && csgo::time2ticks ( csgo::i::globals->m_curtime - pl->simtime ( ) ) < 7 ) {
			best_recs.push_back ( lagcomp::data::extrapolated_records [ pl->idx ( ) ].front ( ) );
		}
		else*/ if ( recs.first.size ( ) >= 2 ) {
			best_recs.push_back ( recs.first.back ( ) );
		}
		
		best_recs.push_back ( recs.first.front ( ) );
	}

	//if ( best_recs.size ( ) >= 2 ) {
	//	const auto dmg_old = std::max< float > ( autowall::dmg ( g::local, pl, g::local->eyes ( ), best_recs [ 1 ].m_bones [ 0 ].origin ( ), 0 ), autowall::dmg ( g::local, pl, g::local->eyes ( ), best_recs [ 1 ].m_bones [ 8 ].origin ( ), 0 ) );
	//	const auto dmg_new = std::max< float > ( autowall::dmg ( g::local, pl, g::local->eyes ( ), best_recs [ 0 ].m_bones [ 0 ].origin ( ), 0 ), autowall::dmg ( g::local, pl, g::local->eyes ( ), best_recs [ 0 ].m_bones [ 8 ].origin ( ), 0 ) );
	//
	//	/* ghetto record scanning optimization */
	//	if ( dmg_new && !dmg_old )
	//		best_recs.pop_back ( );
	//	else if ( !dmg_new && dmg_old )
	//		best_recs.pop_front ( );
	//}

	if ( best_recs.empty( ) )
		return;

	lagcomp::lag_record_t now_rec;

	std::deque< int > hitboxes { };

	if ( active_config.scan_pelvis )
		hitboxes.push_back ( 2 ); // pelvis

	if ( active_config.scan_head )
		hitboxes.push_back( 0 ); // head

	if ( active_config.scan_neck )
		hitboxes.push_back( 1 ); // neck

	if ( active_config.scan_feet ) {
		hitboxes.push_back( 11 ); // right foot
		hitboxes.push_back( 12 ); // left foot
	}

	if ( active_config.scan_chest ) {
		hitboxes.push_back( 5 ); // chest
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
	if ( head_only ) {
		hitboxes.clear( );
		hitboxes.push_back ( 2 ); // pelvis
		hitboxes.push_front( 0 );
	}

	auto should_baim = false;

	/* override to baim if we can doubletap */ {
		/* tickbase manip controller */

		auto can_shoot = [ & ] ( ) {
			if ( !g::local->weapon ( ) || !g::local->weapon ( )->ammo ( ) )
				return false;

			//if ( g::shifted_tickbase )
			//	return csgo::ticks2time ( g::local->tick_base ( ) - ( ( g::shifted_tickbase > 0 ) ? 1 + g::shifted_tickbase : 0 ) ) <= g::local->weapon ( )->next_primary_attack ( );

			return g::local->weapon ( )->next_primary_attack ( ) <= csgo::i::globals->m_curtime;
		};

		const auto weapon_data = ( g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) ) ? g::local->weapon ( )->data ( ) : nullptr;
		const auto fire_rate = weapon_data ? weapon_data->m_fire_rate : 0.0f;
		auto tickbase_as_int = std::clamp< int > ( csgo::time2ticks ( fire_rate ), 0, std::clamp( static_cast< int >( active_config.max_dt_ticks ), 0, csgo::is_valve_server ( ) ? 8 : 16 ) );

		if ( !active_config.dt_key )
			tickbase_as_int = 0;

		if ( tickbase_as_int && ( ( can_shoot ( ) && ( std::abs ( g::ucmd->m_tickcount - g::dt_recharge_time ) >= tickbase_as_int && !g::dt_ticks_to_shift ) ) || g::next_tickbase_shot ) && g::local->weapon ( )->item_definition_index ( ) != 64 ) {
			should_baim = true;
		}
	}

	//if ( animations::resolver::get_confidence ( pl->idx ( ) ) < active_config.baim_if_resolver_confidence_less_than )
	//	should_baim = true;

	auto scan_safe_points = true;

	/* retry with safe points if they aren't valid */
retry_without_safe_points:

	auto best_priority = 0;
	auto best_point = vec3_t( );
	auto best_dmg = 0.0f;
	lagcomp::lag_record_t best_rec;
	int best_hitbox = 0;

	/* only keep previous scan records which are still valid */
	if ( !previous_scanned_records [ pl->idx ( ) ].empty ( ) ) {
		int cur_scan_record = 0;

		for ( auto& previous_scans : previous_scanned_records [ pl->idx ( ) ] ) {
			if ( !previous_scans.m_rec.valid( ) ) {
				previous_scanned_records [ pl->idx ( ) ].erase ( previous_scanned_records [ pl->idx ( ) ].begin ( ) + cur_scan_record );
				continue;
			}

			cur_scan_record++;
		}
	}

	auto is_similar_scan = [ ] ( const recorded_scans_t& scan_record, const lagcomp::lag_record_t& lag_rec ) {
		if ( active_config.optimization == 1 /* optimization low */ )
			return ( scan_record.m_tick == lag_rec.m_tick ) || scan_record.m_rec.m_origin.dist_to ( lag_rec.m_origin ) < 10.0f;
		else if ( active_config.optimization == 2 /* optimization medium */ )
			return std::abs ( scan_record.m_tick - lag_rec.m_tick ) <= 1 || scan_record.m_rec.m_origin.dist_to ( lag_rec.m_origin ) < 10.0f;

		/* optimization high */
		return std::abs ( scan_record.m_tick - lag_rec.m_tick ) <= 2 || scan_record.m_rec.m_origin.dist_to ( lag_rec.m_origin ) < 10.0f;
	};

	/* find best record */
	for ( auto& rec_it : best_recs ) {
		if ( rec_it.m_needs_matrix_construction || rec_it.m_priority < best_priority )
			continue;

		/* if we already scanned this before, or is within close proximity of another record we scanned, let's consider it the same record for now */
		/* this should drastically speed up the ragebot */
		auto record_existed = false;

		if ( !previous_scanned_records [ pl->idx ( ) ].empty( ) && active_config.optimization != 0 /* optimization is not disabled */ ) {
			recorded_scans_t* similar_scan = nullptr;

			for ( auto& scan_record : previous_scanned_records [ pl->idx ( ) ] ) {
				if ( is_similar_scan ( scan_record, rec_it ) ) {
					similar_scan = &scan_record;
					break;
				}
			}			

			if ( similar_scan ) {
				if ( /* double check validity i guess */ similar_scan->m_rec.valid( )
					/* make sure overwriting this record is worth it, would probably be a good thing */ && similar_scan->m_dmg > best_dmg
					/* if the shot is fatal, it doesn't matter */ || best_dmg >= pl->health ( ) ) {
					best_dmg = similar_scan->m_dmg;
					best_point = similar_scan->m_target;
					best_rec = similar_scan->m_rec;
					best_hitbox = similar_scan->m_hitbox;
					best_priority = similar_scan->m_priority;
					record_existed = true;
				}

				if ( best_dmg >= pl->health ( ) ) {
					pl->mins ( ) = backup_min;
					pl->maxs ( ) = backup_max;
					pl->origin ( ) = backup_origin;
					pl->set_abs_origin ( backup_abs_origin );
					std::memcpy ( pl->bone_accessor ( ).get_bone_arr_for_write ( ), backup_bones, sizeof matrix3x4_t * pl->bone_count ( ) );

					point = best_point;
					dmg = best_dmg;
					rec_out = best_rec;
					hitbox_out = best_hitbox;

					return;
				}
				else {
					continue;
				}
			}
		}

		std::array< matrix3x4_t, 128 > safe_point_bones;

		/* for safe point scanning */
		/* override aim matrix with safe point matrix so we scan safe points from safe point matrix on current aim matrix */
		if ( safe_point && safe_point_key && scan_safe_points ) {
			const auto next_resolve = get_misses ( pl->idx ( ) ).bad_resolve + 1;

			if ( next_resolve % 3 == 0 )
				std::memcpy ( &safe_point_bones, rec_it.m_bones1, sizeof matrix3x4_t * pl->bone_count ( ) );
			else if ( next_resolve % 3 == 1 )
				std::memcpy ( &safe_point_bones, rec_it.m_bones2, sizeof matrix3x4_t * pl->bone_count ( ) );
			else
				std::memcpy ( &safe_point_bones, rec_it.m_bones3, sizeof matrix3x4_t * pl->bone_count ( ) );
		}

		pl->mins ( ) = rec_it.m_min;
		pl->maxs ( ) = rec_it.m_max;
		pl->origin ( ) = rec_it.m_origin;
		pl->set_abs_origin ( rec_it.m_origin );

		if ( get_misses ( pl->idx() ).bad_resolve % 3 == 0 )
			std::memcpy ( pl->bone_accessor ( ).get_bone_arr_for_write ( ), rec_it.m_bones1, sizeof matrix3x4_t * pl->bone_count ( ) );
		else if ( get_misses ( pl->idx ( ) ).bad_resolve % 3 == 1 )
			std::memcpy ( pl->bone_accessor ( ).get_bone_arr_for_write ( ), rec_it.m_bones2, sizeof matrix3x4_t * pl->bone_count ( ) );
		else
			std::memcpy ( pl->bone_accessor ( ).get_bone_arr_for_write ( ), rec_it.m_bones3, sizeof matrix3x4_t * pl->bone_count ( ) );

		auto best_hitbox_this_rec = 0;
		auto best_dmg_this_rec = 0.0f;
		auto best_point_this_rec = vec3_t ( 0.0f, 0.0f, 0.0f );

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
				if ( safe_point && safe_point_key && scan_safe_points && ( hb != 0 || force_safe_point ) ) {
					VEC_TRANSFORM ( hitbox->m_bbmin, safe_point_bones [ hitbox->m_bone ], vmin );
					VEC_TRANSFORM ( hitbox->m_bbmax, safe_point_bones [ hitbox->m_bone ], vmax );
				}
				else {
					VEC_TRANSFORM ( hitbox->m_bbmin, pl->bone_accessor( ).get_bone_arr_for_write( )[ hitbox->m_bone ], vmin );
					VEC_TRANSFORM ( hitbox->m_bbmax, pl->bone_accessor( ).get_bone_arr_for_write( )[ hitbox->m_bone ], vmax );
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
				auto body = get_hitbox_pos ( );
				auto body_left = body + left * rad_coeff;
				auto body_right = body + right * rad_coeff;
				auto dmg1 = autowall::dmg ( g::local, pl, g::local->eyes ( ), body, -1 /* do not scan floating points */ );
				auto dmg2 = autowall::dmg ( g::local, pl, g::local->eyes ( ), body_left, -1 /* do not scan floating points */ );
				auto dmg3 = autowall::dmg ( g::local, pl, g::local->eyes ( ), body_right, -1 /* do not scan floating points */ );

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

				if ( best_dmg >= pl->health ( ) || ( ( should_baim || misses [ pl->idx ( ) ].bad_resolve >= active_config.baim_after_misses ) && ( best_dmg * 1.666f >= active_config.min_dmg || best_dmg * 1.666f >= pl->health ( ) ) ) ) {
					pl->mins ( ) = backup_min;
					pl->maxs ( ) = backup_max;
					pl->origin ( ) = backup_origin;
					pl->set_abs_origin ( backup_abs_origin );
					std::memcpy ( pl->bone_accessor ( ).get_bone_arr_for_write ( ), backup_bones, sizeof matrix3x4_t * pl->bone_count ( ) );

					point = best_point;
					dmg = best_dmg;
					rec_out = best_rec;
					hitbox_out = best_hitbox;

					if ( !record_existed )
						previous_scanned_records [ pl->idx ( ) ].push_back ( recorded_scans_t( best_rec, best_point, best_dmg, best_rec.m_tick, best_hitbox, rec_it.m_priority ) );

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
				if ( safe_point && safe_point_key && scan_safe_points ) {
					auto head = get_hitbox_pos ( true );
					auto head_top = head + top * rad_coeff;
					auto dmg1 = autowall::dmg ( g::local, pl, g::local->eyes ( ), head, -1 /* do not scan floating points */ );
					auto dmg2 = autowall::dmg ( g::local, pl, g::local->eyes ( ), head_top, -1 /* do not scan floating points */ );

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
			case 3:
			case 7:
			case 8:
			case 11:
			case 12: {
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
			case 9:
			case 10:
			case 13:
			case 14: {
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
			previous_scanned_records [ pl->idx ( ) ].push_back ( recorded_scans_t( rec_it, best_point_this_rec, best_dmg_this_rec, rec_it.m_tick, best_hitbox_this_rec, rec_it.m_priority ) );
	}

	pl->mins( ) = backup_min;
	pl->maxs( ) = backup_max;
	pl->origin( ) = backup_origin;
	pl->set_abs_origin ( backup_abs_origin );
	std::memcpy ( pl->bone_accessor ( ).get_bone_arr_for_write ( ), backup_bones, sizeof matrix3x4_t* pl->bone_count ( ) );
	
	if ( best_dmg > 0.0f && ( best_dmg >= active_config.min_dmg || best_dmg >= pl->health( ) ) ) {
		point = best_point;
		dmg = best_dmg;
		rec_out = best_rec;
		hitbox_out = best_hitbox;
	}
	/* only retry if it's our first pass, or else we will freeze the game by looping indefinitely */
	else if ( safe_point && safe_point_key && scan_safe_points ) {
		/* retry, but this time w/out safe points since they weren't valid */
		scan_safe_points = false;
		goto retry_without_safe_points;
	}
}

void meleebot ( ucmd_t* ucmd ) {
	if ( g::local->weapon ( )->item_definition_index ( ) == 31 && !features::ragebot::active_config.zeus_bot )
		return;
	else if ( g::local->weapon ( )->data ( )->m_type == 0 && !features::ragebot::active_config.knife_bot )
		return;

	/*auto can_shoot = [ & ] ( ) {
		return g::local->weapon ( ) && g::local->weapon ( )->next_primary_attack ( ) <= csgo::i::globals->m_curtime && g::local->weapon ( )->ammo ( );
	};

	if ( !can_shoot ( ) )
		return;*/

	vec3_t engine_ang;
	csgo::i::engine->get_viewangles ( engine_ang );

	features::lagcomp::lag_record_t* best_rec = nullptr;
	player_t* best_pl = nullptr;
	float best_fov = 180.0f;
	vec3_t best_point, best_ang;

	csgo::for_each_player ( [ & ] ( player_t* pl ) {
		if ( pl->team ( ) == g::local->team ( ) || pl->immune ( ) )
			return;
		
		auto rec = features::lagcomp::get ( pl );

		if ( !rec.second )
			return;

		auto mdl = pl->mdl ( );

		if ( !mdl )
			return;

		auto studio_mdl = csgo::i::mdl_info->studio_mdl ( mdl );

		if ( !studio_mdl )
			return;

		auto s = studio_mdl->hitbox_set ( 0 );

		if ( !s )
			return;

		auto hitbox = s->hitbox ( 2 );

		if ( !hitbox )
			return;

		auto get_hitbox_pos = [ & ] ( ) {
			vec3_t vmin, vmax;
			
			if ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 3 == 0 ) {
				VEC_TRANSFORM ( hitbox->m_bbmin, rec.first [ 0 ].m_bones1 [ hitbox->m_bone ], vmin );
				VEC_TRANSFORM ( hitbox->m_bbmax, rec.first [ 0 ].m_bones1 [ hitbox->m_bone ], vmax );
			}
			else if ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 3 == 1 ) {
				VEC_TRANSFORM ( hitbox->m_bbmin, rec.first [ 0 ].m_bones2 [ hitbox->m_bone ], vmin );
				VEC_TRANSFORM ( hitbox->m_bbmax, rec.first [ 0 ].m_bones2 [ hitbox->m_bone ], vmax );
			}
			else {
				VEC_TRANSFORM ( hitbox->m_bbmin, rec.first [ 0 ].m_bones3 [ hitbox->m_bone ], vmin );
				VEC_TRANSFORM ( hitbox->m_bbmax, rec.first [ 0 ].m_bones3 [ hitbox->m_bone ], vmax );
			}

			auto pos = ( vmin + vmax ) * 0.5f;

			return pos;
		};

		const auto hitbox_pos = get_hitbox_pos ( );

		auto ang = csgo::calc_angle ( g::local->eyes ( ), hitbox_pos );
		csgo::clamp ( ang );

		const auto fov = csgo::normalize( ang.dist_to ( engine_ang ) );

		auto can_use = false;

		if ( g::local->weapon ( )->item_definition_index ( ) == 31 ) {
			can_use = g::local->eyes ( ).dist_to ( hitbox_pos ) < 150.0f;
		}
		else {
			can_use = g::local->origin ( ).dist_to ( rec.first [ 0 ].m_origin ) < 48.0f;
		}

		if ( csgo::is_visible( hitbox_pos ) && fov < best_fov && can_use ) {
			best_pl = pl;
			best_fov = fov;
			best_point = hitbox_pos;
			best_ang = ang;
			best_rec = &rec.first[ 0 ];
		}
	} );

	if ( !features::ragebot::active_config.auto_shoot && ( ( g::local->weapon ( )->data ( )->m_type == 0 ) ? !( ucmd->m_buttons & 1 ) : !( ucmd->m_buttons & 2048 ) ) )
		return;

	if ( !best_pl )
		return;

	csgo::clamp ( best_ang );

	ucmd->m_angs = best_ang;

	if ( !features::ragebot::active_config.silent )
		csgo::i::engine->set_viewangles ( best_ang );

	(*best_rec).backtrack ( ucmd );

	features::ragebot::get_target_pos ( best_pl->idx ( ) ) = best_point;
	features::ragebot::get_target ( ) = best_pl;
	features::ragebot::get_shots ( best_pl->idx ( ) )++;
	features::ragebot::get_shot_pos ( best_pl->idx ( ) ) = g::local->eyes ( );
	features::ragebot::get_lag_rec ( best_pl->idx ( ) ) = *best_rec;
	features::ragebot::get_target_idx ( ) = best_pl->idx ( );
	features::ragebot::get_hitbox ( best_pl->idx ( ) ) = 5;

	if ( features::ragebot::active_config.auto_shoot && g::local->weapon ( )->item_definition_index ( ) != 31 ) {
		auto back = best_pl->angles ( );
		back.y = csgo::normalize( back.y + 180.0f );
		auto backstab = best_ang.dist_to ( back ) < 45.0f;

		if ( backstab ) {
			ucmd->m_buttons |= 2048;
		}
		else {
			auto hp = best_pl->health ( );
			auto armor = best_pl->armor ( ) > 1;
			auto min_dmg1 = armor ? 34 : 40;
			auto min_dmg2 = armor ? 55 : 65;

			if ( hp <= min_dmg2 )
				ucmd->m_buttons |= 2048;
			else if ( hp <= min_dmg1 )
				ucmd->m_buttons |= 1;
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
	security_handler::update ( );

	revolver_firing = false;
	//get_target ( ) = nullptr;

	if ( g::round == round_t::starting )
		return;

	if ( active_config.main_switch && g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) && (g::local->weapon()->item_definition_index() == 31 || g::local->weapon ( )->data ( )->m_type == 0 )) {
		meleebot ( ucmd );
		return;
	}

	if ( !active_config.main_switch || !g::local || !g::local->weapon( ) || !g::local->weapon( )->data( ) || g::local->weapon( )->data( )->m_type == 9 || g::local->weapon( )->data( )->m_type == 7 || g::local->weapon( )->data( )->m_type == 0 )
		return;

	vec3_t engine_ang;
	csgo::i::engine->get_viewangles( engine_ang );

	auto can_shoot = [ & ] ( ) {
		if ( !g::local->weapon ( ) || !g::local->weapon ( )->ammo ( ) )
			return false;

		//if ( g::shifted_tickbase )
		//	return csgo::ticks2time ( g::local->tick_base ( ) - ( ( g::shifted_tickbase > 0 ) ? 1 + g::shifted_tickbase : 0 ) ) <= g::local->weapon ( )->next_primary_attack ( );

		return g::local->weapon ( )->next_primary_attack ( ) <= csgo::i::globals->m_curtime;
	};

	static const auto ray_intersects_sphere = [ ] ( const vec3_t& from, const vec3_t& to, const vec3_t& sphere, float rad ) -> bool {
		auto q = sphere - from;
		auto v = q.dot_product ( csgo::angle_vec ( csgo::calc_angle ( from, to ) ).normalized ( ) );
		auto d = rad - ( q.length_sqr ( ) - v * v );

		return d >= FLT_EPSILON;
	};

	lagcomp::lag_record_t best_rec;
	player_t* best_pl = nullptr;
	auto best_ang = vec3_t( );
	auto best_point = vec3_t( );
	auto best_fov = 180.0f;
	auto best_dmg = 0.0f;
	auto best_hitbox = 0;

	for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
		auto entity = csgo::i::ent_list->get< player_t* > ( i );

		if ( !entity->valid ( ) )
			continue;

		if ( entity->team ( ) == g::local->team ( ) || entity->immune ( ) )
			continue;

		vec3_t point;
		float dmg = 0.0f;

		hitscan ( entity, point, dmg, best_rec, best_hitbox );

		auto ang = csgo::calc_angle ( g::local->eyes ( ), point );
		csgo::clamp ( ang );

		const auto fov = csgo::normalize ( ang.dist_to ( engine_ang ) );

		if ( dmg > 0.0f && dmg > best_dmg && fov < best_fov ) {
			best_pl = entity;
			best_dmg = dmg;
			best_fov = fov;
			best_ang = ang;
			best_point = point;
		}
	}

	if ( !best_pl )
		return;

	if ( active_config.auto_shoot && !can_shoot( ) ) {
		ucmd->m_buttons &= ~1;
	}
	else if ( can_shoot( ) ) {
		const auto backup_origin = best_pl->origin ( );
		auto backup_abs_origin = best_pl->abs_origin ( );
		const auto backup_min = best_pl->mins ( );
		const auto backup_max = best_pl->maxs ( );
		matrix3x4_t backup_bones [ 128 ];
		std::memcpy ( backup_bones, best_pl->bone_accessor ( ).get_bone_arr_for_write ( ), sizeof matrix3x4_t * best_pl->bone_count ( ) );

		best_pl->mins ( ) = best_rec.m_min;
		best_pl->maxs ( ) = best_rec.m_max;
		best_pl->origin ( ) = best_rec.m_origin;
		best_pl->set_abs_origin ( best_rec.m_origin );
		
		if ( get_misses ( best_pl->idx ( ) ).bad_resolve % 3 == 0 )
			std::memcpy ( best_pl->bone_accessor ( ).get_bone_arr_for_write ( ), best_rec.m_bones1, sizeof matrix3x4_t * best_pl->bone_count ( ) );
		else if ( get_misses ( best_pl->idx ( ) ).bad_resolve % 3 == 1 )
			std::memcpy ( best_pl->bone_accessor ( ).get_bone_arr_for_write ( ), best_rec.m_bones2, sizeof matrix3x4_t * best_pl->bone_count ( ) );
		else
			std::memcpy ( best_pl->bone_accessor ( ).get_bone_arr_for_write ( ), best_rec.m_bones3, sizeof matrix3x4_t * best_pl->bone_count ( ) );

		auto hc = run_hitchance ( best_ang, best_pl, best_point, 150, best_hitbox );
		auto should_aim = best_dmg > 0.0f && hc;

		best_pl->mins ( ) = backup_min;
		best_pl->maxs ( ) = backup_max;
		best_pl->origin ( ) = backup_origin;
		best_pl->set_abs_origin ( backup_abs_origin );
		std::memcpy ( best_pl->bone_accessor ( ).get_bone_arr_for_write ( ), backup_bones, sizeof matrix3x4_t * best_pl->bone_count ( ) );

		/* TODO: EXTRAPOLATE POSITION TO SLOW DOWN EXACTLY WHEN WE SHOOT */
		if ( active_config.auto_slow && best_dmg > 0.0f && !hc && g::local->vel ( ).length_2d ( ) > 0.1f ) {
			const auto vec_move = vec3_t ( ucmd->m_fmove, ucmd->m_smove, ucmd->m_umove );
			const auto magnitude = vec_move.length_2d ( );
			const auto max_speed = g::local->weapon ( )->data ( )->m_max_speed;
			const auto move_to_button_ratio = 250.0f / 450.0f;
			const auto speed_ratio = ( max_speed * 0.34f ) * 0.7f;
			const auto move_ratio = speed_ratio * move_to_button_ratio;

			if ( g::local->vel ( ).length_2d ( ) > g::local->weapon ( )->data ( )->m_max_speed * 0.34f ) {
				auto vel_ang = csgo::vec_angle ( vec_move );
				vel_ang.y = csgo::normalize( vel_ang.y + 180.0f );

				const auto normal = csgo::angle_vec( vel_ang ).normalized ( );
				const auto speed_2d = g::local->vel ( ).length_2d ( );

				old_fmove = normal.x * speed_2d;
				old_smove = normal.y * speed_2d;
			}
			else if ( old_fmove != 0.0f || old_smove != 0.0f ) {
				old_fmove = ( old_fmove / magnitude ) * move_ratio;
				old_smove = ( old_smove / magnitude ) * move_ratio;
			}
		}

		if ( !active_config.auto_shoot )
			should_aim = ucmd->m_buttons & 1 && best_dmg > 0.0f;

		if ( !active_config.auto_shoot && g::local->weapon ( )->item_definition_index ( ) == 64 )
			should_aim = ucmd->m_buttons & 1 && !revolver_cocking && should_aim;
		else if ( g::local->weapon ( )->item_definition_index ( ) == 64 )
			should_aim = should_aim && !revolver_cocking;

		if ( should_aim ) {
			revolver_firing = true;

			if ( active_config.auto_shoot ) {
				ucmd->m_buttons |= 1;
				g::send_packet = true;
			}

			if ( active_config.auto_scope && !g::local->scoped( ) &&
				( g::local->weapon ( )->item_definition_index ( ) == 9
					|| g::local->weapon ( )->item_definition_index ( ) == 40
					|| g::local->weapon ( )->item_definition_index ( ) == 38
					|| g::local->weapon ( )->item_definition_index ( ) == 11 ) )
				ucmd->m_buttons |= 2048;

			auto ang = best_ang - g::local->aim_punch( ) * 2.0f;
			csgo::clamp( ang );

			ucmd->m_angs = ang;

			if ( !active_config.silent )
				csgo::i::engine->set_viewangles( ang );

			best_rec.backtrack ( ucmd );

			get_target_pos( best_pl->idx( ) ) = best_point;
			get_target( ) = best_pl;
			get_shots( best_pl->idx( ) )++;
			get_shot_pos( best_pl->idx( ) ) = g::local->eyes( );
			get_lag_rec ( best_pl->idx ( ) ) = best_rec;
			get_target_idx ( ) = best_pl->idx ( );
			get_hitbox ( best_pl->idx ( ) ) = best_hitbox;
		}
		else if ( best_dmg > 0.0f ) {
			if ( g::local->weapon ( )->item_definition_index ( ) == 9
				|| g::local->weapon ( )->item_definition_index ( ) == 40
				|| g::local->weapon ( )->item_definition_index ( ) == 38
				|| g::local->weapon ( )->item_definition_index ( ) == 11 ) {
				if ( active_config.auto_scope && !g::local->scoped ( ) )
					ucmd->m_buttons |= 2048;
				else if ( active_config.auto_scope && g::local->scoped ( ) )
					ucmd->m_buttons &= ~2048;
			}
		}
	}
}

void features::ragebot::tickbase_controller( ucmd_t* ucmd ) {
	auto can_shoot = [ & ] ( ) {
		if ( !g::local->weapon ( ) || !g::local->weapon ( )->ammo ( ) || !g::local->weapon ( )->data ( ) )
			return false;

		//if ( g::shifted_tickbase )
		//	return csgo::ticks2time ( g::local->tick_base ( ) - ( ( g::shifted_tickbase > 0 ) ? 1 + g::shifted_tickbase : 0 ) ) <= g::local->weapon ( )->next_primary_attack ( );
		
		return g::local->weapon ( )->next_primary_attack ( ) <= csgo::i::globals->m_curtime && g::local->weapon ( )->next_primary_attack ( ) + g::local->weapon ( )->data ( )->m_fire_rate <= csgo::i::globals->m_curtime;
	};

	/* tickbase manip controller */
	OPTION ( int, _fd_mode, "Sesame->B->Other->Other->Fakeduck Mode", oxui::object_dropdown );
	KEYBIND ( _fd_key, "Sesame->B->Other->Other->Fakeduck Key" );

	const auto weapon_data = ( g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) ) ? g::local->weapon ( )->data ( ) : nullptr;
	const auto fire_rate = weapon_data ? weapon_data->m_fire_rate : 0.0f;
	auto tickbase_as_int = std::clamp< int > ( csgo::time2ticks( fire_rate ) /*- 1*/, 0, std::clamp( static_cast< int >( active_config.max_dt_ticks ), 0, csgo::is_valve_server ( ) ? 8 : 16 ) );

	if ( !active_config.dt_key )
		tickbase_as_int = 0;

	if ( g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) && tickbase_as_int && ucmd->m_buttons & 1 && can_shoot( ) && std::abs( ucmd->m_cmdnum - g::dt_recharge_time ) > tickbase_as_int && !g::dt_ticks_to_shift && !( g::local->weapon ( )->item_definition_index ( ) == 64 || g::local->weapon ( )->data ( )->m_type == 0 || g::local->weapon ( )->data ( )->m_type >= 7 ) && !_fd_key ) {
		g::dt_ticks_to_shift = tickbase_as_int;
		g::dt_recharge_time = ucmd->m_cmdnum + tickbase_as_int;
	}
}