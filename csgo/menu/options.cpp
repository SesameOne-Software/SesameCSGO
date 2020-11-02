#include "options.hpp"
#include "../utils/utils.hpp"
#include "../security/security_handler.hpp"
#include "../tinyxml2/tinyxml2.h"

std::unordered_map< std::string, options::option > options::vars { };
std::unordered_map< std::string, options::option > options::script_vars { };

void options::option::add_bool( const std::string& id, bool val ) {
	vars [ id ].type = option_type_t::boolean;
	vars [ id ].val.b = val;
}

void options::option::add_list( const std::string& id, int count ) {
	vars [ id ].type = option_type_t::list;
	vars [ id ].list_size = count;
	memset( vars [ id ].val.l, 0, sizeof( vars [ id ].val.l ) );
}

void options::option::add_int( const std::string& id, int val ) {
	vars [ id ].type = option_type_t::integer;
	vars [ id ].val.i = val;
}

void options::option::add_float( const std::string& id, float val ) {
	vars [ id ].type = option_type_t::floating_point;
	vars [ id ].val.f = val;
}

void options::option::add_str( const std::string& id, const char* val ) {
	vars [ id ].type = option_type_t::string;
	strcpy_s( vars [ id ].val.s, val );
}

void options::option::add_color( const std::string& id, const colorf& val ) {
	vars [ id ].type = option_type_t::color;
	vars [ id ].val.c = val;
}

void options::option::add_script_bool( const std::string& id, bool val ) {
	script_vars [ id ].type = option_type_t::boolean;
	script_vars [ id ].val.b = val;
}

void options::option::add_script_list( const std::string& id, int count ) {
	script_vars [ id ].type = option_type_t::list;
	script_vars [ id ].list_size = count;
	memset( script_vars [ id ].val.l, 0, sizeof( script_vars [ id ].val.l ) );
}

void options::option::add_script_int( const std::string& id, int val ) {
	script_vars [ id ].type = option_type_t::integer;
	script_vars [ id ].val.i = val;
}

void options::option::add_script_float( const std::string& id, float val ) {
	script_vars [ id ].type = option_type_t::floating_point;
	script_vars [ id ].val.f = val;
}

void options::option::add_script_str( const std::string& id, const char* val ) {
	script_vars [ id ].type = option_type_t::string;
	strcpy_s ( script_vars [ id ].val.s, val );
}

void options::option::add_script_color( const std::string& id, const colorf& val ) {
	script_vars [ id ].type = option_type_t::color;
	vars [ id ].val.c = val;
}

std::vector< std::string > split( const std::string& str, const std::string& delim ) {
	std::vector< std::string > tokens;
	size_t prev = 0, pos = 0;

	do {
		pos = str.find( delim, prev );

		if ( pos == std::string::npos )
			pos = str.length( );

		std::string token = str.substr( prev, pos - prev );

		if ( !token.empty( ) )
			tokens.push_back( token );

		prev = pos + delim.length( );
	} while ( pos < str.length( ) && prev < str.length( ) );

	return tokens;
}

void options::save( const std::unordered_map< std::string, option >& options, const std::string& path ) {
	tinyxml2::XMLDocument doc;

	const auto root = doc.NewElement( _( "sesame" ) );

	doc.InsertFirstChild( root );

	for ( auto& option : options ) {
		const auto element = doc.NewElement( option.first.data( ) );

		switch ( option.second.type ) {
			case option_type_t::boolean: {
				element->SetText( option.second.val.b );
			} break;
			case option_type_t::list: {
				for ( auto i = 0; i < option.second.list_size; i++ ) {
					const auto list_element = doc.NewElement( _( "item" ) );
					list_element->SetText( option.second.val.l [ i ] );
					element->InsertEndChild( list_element );
				}
			} break;
			case option_type_t::integer: {
				element->SetText( option.second.val.i );
			} break;
			case option_type_t::floating_point: {
				element->SetText( option.second.val.f );
			} break;
			case option_type_t::string: {
				element->SetText( ( char* )option.second.val.s );
			} break;
			case option_type_t::color: {
				const auto r_element = doc.NewElement( _( "r" ) );
				const auto g_element = doc.NewElement( _( "g" ) );
				const auto b_element = doc.NewElement( _( "b" ) );
				const auto a_element = doc.NewElement( _( "a" ) );

				r_element->SetText( option.second.val.c.r );
				g_element->SetText( option.second.val.c.g );
				b_element->SetText( option.second.val.c.b );
				a_element->SetText( option.second.val.c.a );

				element->InsertEndChild( r_element );
				element->InsertEndChild( g_element );
				element->InsertEndChild( b_element );
				element->InsertEndChild( a_element );
			} break;
		}

		root->InsertEndChild( element );
	}

	doc.SaveFile( path.data( ) );
}

void options::load( std::unordered_map< std::string, option >& options, const std::string& path ) {
	tinyxml2::XMLDocument doc;

	const auto err = doc.LoadFile( path.data( ) );

	if ( err != tinyxml2::XML_SUCCESS ) {
		dbg_print( _( "Failed to open configuration file.\n" ) );
		return;
	}

	const auto root = doc.FirstChild( );

	if ( !root ) {
		dbg_print( _( "Failed to open configuration file.\n" ) );
		return;
	}

	for ( auto& option : options ) {
		const auto element = root->FirstChildElement( option.first.data( ) );

		if ( !element ) {
			dbg_print( _( "Failed to find element.\n" ) );
			continue;
		}

		auto err = tinyxml2::XML_SUCCESS;

		switch ( option.second.type ) {
			case option_type_t::boolean: {
				err = element->QueryBoolText( &option.second.val.b );
			} break;
			case option_type_t::list: {
				auto list_element = element->FirstChildElement( _( "item" ) );
				auto i = 0;

				if ( !list_element ) {
					dbg_print( _( "Element found had invalid value.\n" ) );
					continue;
				}

				while ( list_element && i < option.second.list_size ) {
					err = list_element->QueryBoolText( &option.second.val.l [ i ] );

					if ( err != tinyxml2::XML_SUCCESS ) {
						dbg_print( _( "Element found had invalid value.\n" ) );
						break;
					}

					list_element = list_element->NextSiblingElement( _( "item" ) );
					i++;
				}
			} break;
			case option_type_t::integer: {
				err = element->QueryIntText( &option.second.val.i );
			} break;
			case option_type_t::floating_point: {
				err = element->QueryFloatText( &option.second.val.f );
			} break;
			case option_type_t::string: {
				const auto str = element->GetText( );
				
				if ( !str ) {
					dbg_print( _( "Element found had invalid value.\n" ) );
					continue;
				}

				strcpy_s( option.second.val.s, str );
			} break;
			case option_type_t::color: {
				const auto r_element = element->FirstChildElement( _( "r" ) );
				const auto g_element = element->FirstChildElement( _( "g" ) );
				const auto b_element = element->FirstChildElement( _( "b" ) );
				const auto a_element = element->FirstChildElement( _( "a" ) );

				if ( !r_element || !g_element || !b_element || !a_element ) {
					dbg_print( _( "Element found had invalid value.\n" ) );
					continue;
				}

				err = r_element->QueryFloatText( &option.second.val.c.r );

				if ( err != tinyxml2::XML_SUCCESS ) {
					dbg_print( _( "Element found had invalid value.\n" ) );
					continue;
				}

				err = g_element->QueryFloatText( &option.second.val.c.g );

				if ( err != tinyxml2::XML_SUCCESS ) {
					dbg_print( _( "Element found had invalid value.\n" ) );
					continue;
				}

				err = b_element->QueryFloatText( &option.second.val.c.b );

				if ( err != tinyxml2::XML_SUCCESS ) {
					dbg_print( _( "Element found had invalid value.\n" ) );
					continue;
				}

				err = a_element->QueryFloatText( &option.second.val.c.a );
			} break;
		}

		if ( err != tinyxml2::XML_SUCCESS ) {
			dbg_print( _( "Element found had invalid value.\n" ) );
			continue;
		}
	}
}

void options::add_weapon_config( const std::string& weapon_category ) {
	const std::string prefix = _( "ragebot." ) + weapon_category + _( "." );

	options::option::add_bool( prefix + _( "inherit_default" ), false );
	options::option::add_bool( prefix + _( "headshot_only" ), false );
	options::option::add_bool( prefix + _( "choke_onshot" ), false );
	options::option::add_bool( prefix + _( "silent" ), false );
	options::option::add_bool( prefix + _( "auto_shoot" ), false );
	options::option::add_bool( prefix + _( "auto_scope" ), false );
	options::option::add_bool( prefix + _( "auto_slow" ), false );
	options::option::add_bool( prefix + _( "dt_teleport" ), false );
	options::option::add_bool( prefix + _( "dt_enabled" ), false );
	options::option::add_int( prefix + _( "dt_ticks" ), 0 );
	options::option::add_float( prefix + _( "min_dmg" ), 0.0f );
	options::option::add_float( prefix + _( "dmg_accuracy" ), 0.0f );
	options::option::add_float( prefix + _( "hit_chance" ), 0.0f );
	options::option::add_float( prefix + _( "dt_hit_chance" ), 0.0f );
	options::option::add_bool( prefix + _( "baim_if_lethal" ), false );
	options::option::add_bool( prefix + _( "baim_in_air" ), false );
	options::option::add_bool ( prefix + _ ( "onshot_only" ), false );
	options::option::add_int( prefix + _( "force_baim" ), 0 );
	options::option::add_float( prefix + _( "head_pointscale" ), 0.0f );
	options::option::add_float( prefix + _( "body_pointscale" ), 0.0f );
	options::option::add_list( prefix + _( "hitboxes" ), 7 ); /* head, neck, chest, pelvis, arms, legs, feet */

	END_FUNC
}

void options::add_antiaim_config( const std::string& antiaim_category ) {
	const std::string prefix = _( "antiaim." ) + antiaim_category + _( "." );

	options::option::add_int( prefix + _( "fakelag_factor" ), 0 );
	options::option::add_bool( prefix + _( "enabled" ), false );
	options::option::add_int( prefix + _( "pitch" ), 0 ); /* none, down, up, zero */
	options::option::add_float( prefix + _( "yaw_offset" ), 0.0f );
	options::option::add_int( prefix + _( "base_yaw" ), 0 ); /* relative, absolute, at target, auto direction */
	options::option::add_float( prefix + _( "auto_direction_amount" ), 0.0f );
	options::option::add_float( prefix + _( "auto_direction_range" ), 0.0f );
	options::option::add_float( prefix + _( "jitter_range" ), 0.0f );
	options::option::add_float( prefix + _( "rotation_range" ), 0.0f );
	options::option::add_float( prefix + _( "rotation_speed" ), 0.0f );
	options::option::add_bool( prefix + _( "desync" ), false );
	options::option::add_float( prefix + _( "desync_range_inverted" ), 0.0f );
	options::option::add_float( prefix + _( "desync_range" ), 0.0f );
	options::option::add_bool( prefix + _( "invert_initial_side" ), false );
	options::option::add_bool( prefix + _( "jitter_desync_side" ), false );
	options::option::add_bool( prefix + _( "center_real" ), false );
	options::option::add_bool( prefix + _( "anti_bruteforce" ), false );
	options::option::add_bool( prefix + _( "anti_freestand_prediction" ), false );

	END_FUNC
}

void options::add_player_visual_config( const std::string& player_category ) {
	const std::string prefix = _( "visuals." ) + player_category + _( "." );

	/* ENEMIES: chams, chams flat, chams xqz, backtrack chams, hit matrix, glow, rimlight overlay, esp box, health bar, ammo bar, desync bar, value text, nametag, weapon name */
	/* TEAMMATES: chams, chams flat, chams xqz, glow, rimlight overlay, esp box, health bar, ammo bar, desync bar, value text, nametag, weapon name */
	/* LOCAL: chams, chams flat, chams xqz, desync chams, desync chams fakelag, desync chams rimlight, glow, rimlight overlay, esp box, health bar, ammo bar, desync bar, value text, nametag, weapon name */
	if ( player_category == _( "local" ) )
		options::option::add_list( prefix + _( "options" ), 15 );
	else if ( player_category == _( "enemies" ) )
		options::option::add_list( prefix + _( "options" ), 14 );
	else if ( player_category == _( "teammates" ) )
		options::option::add_list( prefix + _( "options" ), 12 );

	options::option::add_int( prefix + _( "health_bar_location" ), 0 ); /* left, right, bottom, top */
	options::option::add_int( prefix + _( "ammo_bar_location" ), 0 ); /* left, right, bottom, top */
	options::option::add_int( prefix + _( "desync_bar_location" ), 0 ); /* left, right, bottom, top */
	options::option::add_int( prefix + _( "value_text_location" ), 0 ); /* left, right, bottom, top */
	options::option::add_int( prefix + _( "nametag_location" ), 0 ); /* left, right, bottom, top */
	options::option::add_int( prefix + _( "weapon_name_location" ), 0 ); /* left, right, bottom, top */
	options::option::add_float( prefix + _( "reflectivity" ), 0.0f );
	options::option::add_float( prefix + _( "phong" ), 0.0f );
	options::option::add_color( prefix + _( "chams_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	options::option::add_color( prefix + _( "xqz_chams_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	options::option::add_color( prefix + _( "backtrack_chams_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	options::option::add_color( prefix + _( "hit_matrix_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	options::option::add_color( prefix + _( "glow_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	options::option::add_color( prefix + _( "rimlight_overlay_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	options::option::add_color( prefix + _( "box_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	options::option::add_color( prefix + _( "health_bar_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	options::option::add_color( prefix + _( "ammo_bar_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	options::option::add_color( prefix + _( "desync_bar_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	options::option::add_color( prefix + _( "name_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	options::option::add_color( prefix + _( "weapon_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );

	END_FUNC
}

void options::init( ) {
	/* options should be structered in the following format: */
	/* TAB.GROUP.OPTION */

	option::add_int( _( "global.assistance_type" ), 0 ); /* disabled, legit, rage */

	/* LEGIT */
	/* global legit */
	option::add_bool( _( "legitbot.triggerbot" ), false );
	option::add_list( _( "legitbot.triggerbot_hitboxes" ), 7 ); /* head, neck, chest, pelvis, arms, legs, feet */
	option::add_int( _( "legitbot.triggerbot_key" ), 0 );
	option::add_int( _( "legitbot.triggerbot_key_mode" ), 0 );

	/* RAGE */
	/* global rage */
	option::add_bool( _( "ragebot.knife_bot" ), false );
	option::add_bool( _( "ragebot.zeus_bot" ), false );
	option::add_bool( _( "ragebot.fix_fakelag" ), false );
	option::add_bool( _( "ragebot.resolve_desync" ), false );
	option::add_bool( _( "ragebot.safe_point" ), false );
	option::add_bool( _( "ragebot.auto_revolver" ), false );
	option::add_int( _( "ragebot.safe_point_key" ), 0 );
	option::add_int( _( "ragebot.safe_point_key_mode" ), 0 );
	option::add_int( _( "ragebot.dt_key" ), 0 );
	option::add_int( _( "ragebot.dt_key_mode" ), 0 );
	/* weapon configs */
	add_weapon_config( _( "default" ) );
	add_weapon_config( _( "pistol" ) );
	add_weapon_config( _( "revolver" ) );
	add_weapon_config( _( "rifle" ) );
	add_weapon_config( _( "awp" ) );
	add_weapon_config( _( "auto" ) );
	add_weapon_config( _( "scout" ) );

	/* ANTIAIM */
	/* global antiaim */
	option::add_int( _( "antiaim.fakeduck_mode" ), 0 ); /* normal, full */
	option::add_bool( _( "antiaim.fakewalk" ), false );
	option::add_float( _( "antiaim.slow_walk_speed" ), 0.0f );
	option::add_int( _( "antiaim.slow_walk_key" ), 0 );
	option::add_int( _( "antiaim.slow_walk_key_mode" ), 0 );
	option::add_bool( _( "antiaim.fakeduck" ), false );
	option::add_int( _( "antiaim.fakeduck_key" ), 0 );
	option::add_int( _( "antiaim.fakeduck_key_mode" ), 0 );
	option::add_int( _( "antiaim.manual_left_key" ), 0 );
	option::add_int( _( "antiaim.manual_left_key_mode" ), 0 );
	option::add_int( _( "antiaim.manual_right_key" ), 0 );
	option::add_int( _( "antiaim.manual_right_key_mode" ), 0 );
	option::add_int( _( "antiaim.manual_back_key" ), 0 );
	option::add_int( _( "antiaim.manual_back_key_mode" ), 0 );
	option::add_int( _( "antiaim.desync_invert_key" ), 0 );
	option::add_int( _( "antiaim.desync_invert_key_mode" ), 0 );
	/* antiaim configs */
	add_antiaim_config( _( "air" ) );
	add_antiaim_config( _( "moving" ) );
	add_antiaim_config( _( "slow_walk" ) );
	add_antiaim_config( _( "standing" ) );
	options::option::add_int( _( "antiaim.standing.desync_type" ), 0 ); /* real around fake, fake around real */
	options::option::add_int( _( "antiaim.standing.desync_type_inverted" ), 0 ); /* real around fake, fake around real */

	/* VISUALS */
	/* global visuals */
	option::add_list( _( "visuals.filters" ), 6 ); /* local, teammates, enemies, weapon, grenade, bomb */
	/* player visuals configs */
	add_player_visual_config( _( "local" ) );
	option::add_color ( _ ( "visuals.local.desync_chams_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	option::add_color ( _ ( "visuals.local.desync_rimlight_overlay_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	add_player_visual_config( _( "enemies" ) );
	add_player_visual_config( _( "teammates" ) );
	/* other visuals */
	option::add_list( _( "visuals.other.removals" ), 9 ); /* smoke, flash, scope, aimpunch, viewpunch, zoom, occlusion, killfeed decay, post processing */
	option::add_float( _( "visuals.other.fov" ), 90.0f );
	option::add_float( _( "visuals.other.viewmodel_fov" ), 68.0f );
	option::add_float( _( "visuals.other.aspect_ratio" ), 1.0f );
	option::add_bool( _( "visuals.other.bullet_tracers" ), false );
	option::add_bool( _( "visuals.other.bullet_impacts" ), false );
	option::add_list( _( "visuals.other.logs" ), 4 ); /* hits, spread misses, resolver misses, wrong hitbox, manuals */
	option::add_bool( _( "visuals.other.grenade_trajectories" ), false );
	option::add_bool( _( "visuals.other.grenade_bounces" ), false );
	option::add_bool( _( "visuals.other.grenade_blast_radii" ), false );
	option::add_bool( _( "visuals.other.spread_circle" ), false );
	option::add_bool( _( "visuals.other.gradient_spread_circle" ), false );
	option::add_bool( _( "visuals.other.offscreen_esp" ), false );
	option::add_bool( _( "visuals.other.bomb_esp" ), false );
	option::add_bool( _( "visuals.other.bomb_timer" ), false );
	option::add_float( _( "visuals.other.offscreen_esp_distance" ), 0.0f );
	option::add_float( _( "visuals.other.offscreen_esp_size" ), 0.0f );
	option::add_float( _( "visuals.other.grenade_path_fade_time" ), 0.0f );
	option::add_color( _( "visuals.other.offscreen_esp_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	option::add_color( _( "visuals.other.bullet_tracer_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	option::add_color( _( "visuals.other.bullet_impact_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	option::add_color( _( "visuals.other.grenade_trajectory_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	option::add_color( _( "visuals.other.grenade_bounce_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	option::add_color( _( "visuals.other.grenade_radii_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	option::add_color( _( "visuals.other.spread_circle_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	option::add_color( _( "visuals.other.world_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	option::add_color( _( "visuals.other.prop_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	option::add_int( _( "visuals.other.hit_sound" ), 0 ); /* none, arena switch, fall pain, bolt, neck snap, power switch, glass, bell, cod, rattle, sesame */
	option::add_bool( _( "visuals.other.watermark" ), false );
	option::add_bool( _( "visuals.other.keybind_list" ), false );
	option::add_color( _( "visuals.other.accent_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	option::add_color( _( "visuals.other.secondary_accent_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );
	option::add_color( _( "visuals.other.logo_color" ), { 1.0f, 1.0f, 1.0f, 1.0f } );

	/* SKINS */


	/* MISC */
	option::add_bool( _( "misc.movement.bhop" ), false );
	option::add_bool( _( "misc.movement.block_bot" ), false );
	option::add_int( _( "misc.movement.block_bot_key" ), 0 );
	option::add_int( _( "misc.movement.block_bot_key_mode" ), 0 );
	option::add_bool ( _ ( "misc.movement.airstuck" ), false );
	option::add_int ( _ ( "misc.movement.airstuck_key" ), 0 );
	option::add_int ( _ ( "misc.movement.airstuck_key_mode" ), 0 );
	option::add_bool( _( "misc.movement.auto_forward" ), false );
	option::add_bool( _( "misc.movement.auto_strafer" ), false );
	option::add_bool( _( "misc.movement.omnidirectional_auto_strafer" ), false );
	option::add_bool( _( "misc.effects.third_person" ), false );
	option::add_float( _( "misc.effects.third_person_range" ), 0.0f );
	option::add_int( _( "misc.effects.third_person_key" ), 0 );
	option::add_int( _( "misc.effects.third_person_key_mode" ), 0 );
	option::add_float( _( "misc.effects.ragdoll_force_scale" ), 1.0f );
	option::add_bool( _( "misc.effects.clantag" ), false );
	option::add_int( _( "misc.effects.clantag_animation" ), 0 ); /* static, marquee, capitalize, heart */
	option::add_str( _( "misc.effects.clantag_text" ), _( "sesame" ) );
	option::add_float( _( "misc.effects.revolver_cock_volume" ), 1.0f );
	option::add_float( _( "misc.effects.weapon_volume" ), 1.0f );

	option::add_float( _( "gui.dpi" ), 1.0f );

	END_FUNC
}