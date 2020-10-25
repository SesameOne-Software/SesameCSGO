#include <functional>
#include <filesystem>
#include <ShlObj.h>
#include <codecvt>
#include <fstream>
#include "menu.hpp"
#include "options.hpp"
#include "d3d9_render.hpp"
#include "sesui_custom.hpp"
#include "../utils/networking.hpp"
#include "../features/antiaim.hpp"
#include "../features/ragebot.hpp"
#include "../utils/networking.hpp"
#include "../cjson/cJSON.h"

extern std::string last_config_user;

bool upload_to_cloud = false;
std::mutex gui::gui_mutex;

int gui::config_access = 0;
std::string gui::config_code = "";
std::string gui::config_description = "";

std::string gui::config_user = "";
uint64_t gui::last_update_time = 0;

bool download_config_code = false;

cJSON* cloud_config_list = nullptr;

bool gui::opened = false;
bool open_button_pressed = false;

enum tabs_t {
	tab_legit = 0,
	tab_rage,
	tab_antiaim,
	tab_visuals,
	tab_skins,
	tab_misc,
	tab_max
};

enum legit_subtabs_t {
	legit_subtabs_none = -1,
	legit_subtabs_main,
	legit_subtabs_default,
	legit_subtabs_pistol,
	legit_subtabs_revolver,
	legit_subtabs_rifle,
	legit_subtabs_awp,
	legit_subtabs_auto,
	legit_subtabs_scout,
	legit_subtabs_max
};

enum rage_subtabs_t {
	rage_subtabs_none = -1,
	rage_subtabs_main,
	rage_subtabs_default,
	rage_subtabs_pistol,
	rage_subtabs_revolver,
	rage_subtabs_rifle,
	rage_subtabs_awp,
	rage_subtabs_auto,
	rage_subtabs_scout,
	rage_subtabs_max
};

enum antiaim_subtabs_t {
	antiaim_subtabs_none = -1,
	antiaim_subtabs_air,
	antiaim_subtabs_moving,
	antiaim_subtabs_slow_walk,
	antiaim_subtabs_standing,
	antiaim_subtabs_other,
	antiaim_subtabs_max
};

enum visuals_subtabs_t {
	visuals_subtabs_none = -1,
	visuals_subtabs_local,
	visuals_subtabs_enemies,
	visuals_subtabs_teammates,
	visuals_subtabs_other,
	visuals_subtabs_max
};

enum skins_subtabs_t {
	skins_subtabs_none = -1,
	skins_subtabs_skins,
	skins_subtabs_models,
	skins_subtabs_max
};

enum misc_subtabs_t {
	misc_subtabs_none = -1,
	misc_subtabs_movement,
	misc_subtabs_effects,
	misc_subtabs_plist,
	misc_subtabs_cheat,
	misc_subtabs_configs,
	misc_subtabs_scripts,
	misc_subtabs_max
};

int current_tab_idx = tab_legit;
int legit_subtab_idx = legit_subtabs_none;
int rage_subtab_idx = rage_subtabs_none;
int antiaim_subtab_idx = antiaim_subtabs_none;
int visuals_subtab_idx = visuals_subtabs_none;
int skins_subtab_idx = skins_subtabs_none;
int misc_subtab_idx = misc_subtabs_none;

std::string g_pfp_data;
std::string g_username;

void gui::init( ) {
	g_username = ( g::loader_data && g::loader_data->username ) ? g::loader_data->username : _ ( "sesame" );
//
	if ( !g::loader_data || !g::loader_data->avatar || !g::loader_data->avatar_sz )
		g_pfp_data = std::string( reinterpret_cast< const char* >( ses_pfp ), sizeof( ses_pfp ) );//networking::get(_("sesame.one/data/avatars/s/0/1.jpg"));
	else
		g_pfp_data = std::string( g::loader_data->avatar, g::loader_data->avatar + g::loader_data->avatar_sz );

	/* initialize cheat config */
	options::init( );
	//erase::erase_func ( options::init );
	//erase::erase_func ( options::add_antiaim_config );
	//erase::erase_func ( options::add_player_visual_config );
	//erase::erase_func ( options::add_weapon_config );

	/* bind draw list methods to our own drawing functions */
	sesui::draw_list.draw_texture = sesui::binds::draw_texture;
	sesui::draw_list.draw_polygon = sesui::binds::polygon;
	sesui::draw_list.draw_multicolor_polygon = sesui::binds::multicolor_polygon;
	sesui::draw_list.draw_text = sesui::binds::text;
	sesui::draw_list.get_text_size = sesui::binds::get_text_size;
	sesui::draw_list.get_frametime = sesui::binds::get_frametime;
	sesui::draw_list.begin_clip = sesui::binds::begin_clip;
	sesui::draw_list.end_clip = sesui::binds::end_clip;
	sesui::draw_list.create_font = sesui::binds::create_font;

	gui_mutex.lock ( );
	load_cfg_list ( );
	gui_mutex.unlock ( );

	END_FUNC
}

std::string selected_config = _( "default" );
std::vector< std::string > configs { };

void gui::load_cfg_list( ) {
	char appdata [ MAX_PATH ];

	if ( SUCCEEDED( LI_FN( SHGetFolderPathA )( nullptr, N( 5 ), nullptr, N( 0 ), appdata ) ) ) {
		LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame" ) ).c_str( ), nullptr );
		LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame\\configs" ) ).c_str ( ), nullptr );
	}

	auto sanitize_name = [ ] ( const std::string& dir ) {
		const auto dot = dir.find_last_of( _( "." ) );
		return dir.substr( N( 0 ), dot );
	};

	configs.clear( );

	for ( const auto& dir : std::filesystem::recursive_directory_iterator( std::string( appdata ) + _( "\\sesame\\configs" ) ) ) {
		if ( dir.exists( ) && dir.is_regular_file( ) && dir.path( ).extension( ).string( ) == _( ".xml" ) ) {
			const auto sanitized = sanitize_name( dir.path( ).filename( ).string( ) );
			configs.push_back( sanitized );
		}
	}
}

void gui::weapon_controls( const std::string& weapon_name ) {
	const auto ragebot_weapon = _( "ragebot." ) + weapon_name + _( "." );

	namespace gui = sesui::custom;

	if ( gui::begin_group( "Weapon Settings", sesui::rect( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
		sesui::tooltip ( _ ( "Inherits settings from default weapon configuration" ) );
		sesui::checkbox( _( "Inherit Default" ), options::vars [ ragebot_weapon + _( "inherit_default" ) ].val.b );
		sesui::tooltip ( _ ( "Allows choking packets immediately after your shot" ) );
		sesui::checkbox( _( "Choke On Shot" ), options::vars [ ragebot_weapon + _( "choke_onshot" ) ].val.b );
		sesui::tooltip ( _ ( "Hide angles from being shown on screen (clientside only)" ) );
		sesui::checkbox( _( "Silent Aim" ), options::vars [ ragebot_weapon + _( "silent" ) ].val.b );
		sesui::tooltip ( _ ( "Automatically shoot when a target is found" ) );
		sesui::checkbox( _( "Auto Shoot" ), options::vars [ ragebot_weapon + _( "auto_shoot" ) ].val.b );
		sesui::tooltip ( _ ( "Automatically scope if hitchance is not high enough" ) );
		sesui::checkbox( _( "Auto Scope" ), options::vars [ ragebot_weapon + _( "auto_scope" ) ].val.b );
		sesui::tooltip ( _ ( "Automatically slow down if hitchance requirement is not met" ) );
		sesui::checkbox( _( "Auto Slow" ), options::vars [ ragebot_weapon + _( "auto_slow" ) ].val.b );
		sesui::tooltip ( _ ( "Allow doubletap to teleport your player" ) );
		sesui::checkbox ( _ ( "Doubletap Teleport" ), options::vars [ ragebot_weapon + _ ( "dt_teleport" ) ].val.b );
		sesui::tooltip ( _ ( "Enable doubletap on current weapon" ) );
		sesui::checkbox( _( "Doubletap" ), options::vars [ ragebot_weapon + _( "dt_enabled" ) ].val.b );
		sesui::tooltip ( _ ( "Maximum amount of ticks to shift during doubletap" ) );
		sesui::slider( _( "Doubletap Ticks" ), options::vars [ ragebot_weapon + _( "dt_ticks" ) ].val.i, 0, 16, _( "{} ticks" ) );
		sesui::tooltip ( _ ( "Minimum damage required for ragebot to target players" ) );
		sesui::slider( _( "Minimum Damage" ), options::vars [ ragebot_weapon + _( "min_dmg" ) ].val.f, 0.0f, 150.0f, ( options::vars [ ragebot_weapon + _( "min_dmg" ) ].val.f > 100.0f ? ( _( "HP + " ) + std::to_string( static_cast< int > ( options::vars [ ragebot_weapon + _( "min_dmg" ) ].val.f - 100.0f ) ) + _( " HP" ) ) : ( std::to_string( static_cast< int > ( options::vars [ ragebot_weapon + _( "min_dmg" ) ].val.f ) ) + _( " HP" ) ) ).c_str( ) );
		sesui::tooltip ( _ ( "Minimum to chance to hit target damage" ) );
		sesui::slider( _( "Damage Accuracy" ), options::vars [ ragebot_weapon + _( "dmg_accuracy" ) ].val.f, 0.0f, 100.0f, _( "{:.1f}%" ) );
		sesui::tooltip ( _ ( "Minimum hit chance required for ragebot to target players" ) );
		sesui::slider( _( "Hit Chance" ), options::vars [ ragebot_weapon + _( "hit_chance" ) ].val.f, 0.0f, 100.0f, _( "{:.1f}%" ) );
		sesui::tooltip ( _ ( "Minimum hit chance required for ragebot to target players while double tapping" ) );
		sesui::slider( _( "Doubletap Hit Chance" ), options::vars [ ragebot_weapon + _( "dt_hit_chance" ) ].val.f, 0.0f, 100.0f, _( "{:.1f}%" ) );

		gui::end_group( );
	}

	if ( gui::begin_group( "Hitscan", sesui::rect( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
		sesui::tooltip ( _ ( "Force body hitbox if body shot is lethal" ) );
		sesui::checkbox( _( "Bodyaim If Lethal" ), options::vars [ ragebot_weapon + _( "baim_if_lethal" ) ].val.b );
		sesui::tooltip ( _ ( "Force body hitbox if player is in air" ) );
		sesui::checkbox( _( "Bodyaim If In Air" ), options::vars [ ragebot_weapon + _( "baim_in_air" ) ].val.b );
		sesui::tooltip ( _ ( "Overrides all hitboxes and records to only target onshot" ) );
		sesui::checkbox ( _ ( "Onshot Only" ), options::vars [ ragebot_weapon + _ ( "onshot_only" ) ].val.b );
		sesui::tooltip ( _ ( "Forces body hitbox after x misses" ) );
		sesui::slider( _( "Bodyaim After Misses" ), options::vars [ ragebot_weapon + _( "force_baim" ) ].val.i, 0, 5, _( "{} misses" ) );
		sesui::tooltip ( _ ( "Overrides all hitboxes and body aim to head hitbox" ) );
		sesui::checkbox( _( "Headshot Only" ), options::vars [ ragebot_weapon + _( "headshot_only" ) ].val.b );
		sesui::tooltip ( _ ( "Distance from center of head to scan" ) );
		sesui::slider( _( "Head Point Scale" ), options::vars [ ragebot_weapon + _( "head_pointscale" ) ].val.f, 0.0f, 100.0f, _( "{:.1f}%" ) );
		sesui::tooltip ( _ ( "Distance from center of body to scan" ) );
		sesui::slider( _( "Body Point Scale" ), options::vars [ ragebot_weapon + _( "body_pointscale" ) ].val.f, 0.0f, 100.0f, _( "{:.1f}%" ) );
		sesui::tooltip ( _ ( "Hitboxes ragebot will attempt to target" ) );
		sesui::multiselect( _( "Hitboxes" ), {
			{ _( "Head" ), options::vars [ ragebot_weapon + _( "hitboxes" ) ].val.l [ 0 ] },
			{ _( "Neck" ), options::vars [ ragebot_weapon + _( "hitboxes" ) ].val.l [ 1 ] },
			{ _( "Chest" ), options::vars [ ragebot_weapon + _( "hitboxes" ) ].val.l [ 2 ] },
			{ _( "Pelvis" ), options::vars [ ragebot_weapon + _( "hitboxes" ) ].val.l [ 3 ] },
			{ _( "Arms" ), options::vars [ ragebot_weapon + _( "hitboxes" ) ].val.l [ 4 ] },
			{ _( "Legs" ), options::vars [ ragebot_weapon + _( "hitboxes" ) ].val.l [ 5 ] },
			{ _( "Feet" ), options::vars [ ragebot_weapon + _( "hitboxes" ) ].val.l [ 6 ] }
			} );

		gui::end_group( );
	}
}

void gui::antiaim_controls( const std::string& antiaim_name ) {
	const auto antiaim_config = _( "antiaim." ) + antiaim_name + _( "." );

	namespace gui = sesui::custom;

	if ( gui::begin_group( "Antiaim", sesui::rect( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
		sesui::tooltip ( _ ( "Enable antiaim" ) );
		sesui::checkbox( _( "Enable" ), options::vars [ antiaim_config + _( "enabled" ) ].val.b );
		sesui::tooltip ( _ ( "Fakelag amount in ticks" ) );
		sesui::slider( _( "Fakelag Factor" ), options::vars [ antiaim_config + _( "fakelag_factor" ) ].val.i, 0, 16, _( "{} ticks" ) );
		sesui::tooltip ( _ ( "Base antiaim pitch" ) );
		sesui::combobox( _( "Base Pitch" ), options::vars [ antiaim_config + _( "pitch" ) ].val.i, { _( "None" ), _( "Down" ), _( "Up" ), _( "Zero" ) } );
		sesui::tooltip ( _ ( "Yaw offset from yaw base" ) );
		sesui::slider( _( "Yaw Offset" ), options::vars [ antiaim_config + _( "yaw_offset" ) ].val.f, -180.0f, 180.0f, (char*)_( u8"{:.1f}°" ) );
		sesui::tooltip ( _ ( "Base yaw for antiaim to build off of" ) );
		sesui::combobox( _( "Base Yaw" ), options::vars [ antiaim_config + _( "base_yaw" ) ].val.i, { _( "Relative" ), _( "Absolute" ), _( "At Target" ), _( "Auto Direction" ) } );
		sesui::tooltip ( _ ( "Auto direction flip amount" ) );
		sesui::slider( _( "Auto Direction Amount" ), options::vars [ antiaim_config + _( "auto_direction_amount" ) ].val.f, -180.0f, 180.0f, ( char* ) _( u8"{:.1f}°" ) );
		sesui::tooltip ( _ ( "Auto direction range to hide head" ) );
		sesui::slider( _( "Auto Direction Range" ), options::vars [ antiaim_config + _( "auto_direction_range" ) ].val.f, 0.0f, 100.0f, _( "{:.1f} units" ) );
		sesui::tooltip ( _ ( "Constant jitter amount" ) );
		sesui::slider( _( "Jitter Range" ), options::vars [ antiaim_config + _( "jitter_range" ) ].val.f, -180.0f, 180.0f, ( char* ) _( u8"{:.1f}°" ) );
		sesui::tooltip ( _ ( "Antiaim rotation amount" ) );
		sesui::slider( _( "Rotation Range" ), options::vars [ antiaim_config + _( "rotation_range" ) ].val.f, -180.0f, 180.0f, ( char* ) _( u8"{:.1f}°" ) );
		sesui::tooltip ( _ ( "Antiaim frequency in revolutions per second" ) );
		sesui::slider( _( "Rotation Speed" ), options::vars [ antiaim_config + _( "rotation_speed" ) ].val.f, 0.0f, 2.0f, _( "{:.1f} Hz" ) );

		gui::end_group( );
	}

	if ( gui::begin_group( "Desync", sesui::rect( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
		sesui::tooltip ( _ ( "Enable desync (fake angles)" ) );
		sesui::checkbox( _( "Enable Desync" ), options::vars [ antiaim_config + _( "desync" ) ].val.b );
		sesui::tooltip ( _ ( "Switch initial desync side to opposite side" ) );
		sesui::checkbox( _( "Invert Initial Side" ), options::vars [ antiaim_config + _( "invert_initial_side" ) ].val.b );
		sesui::tooltip ( _ ( "Automatically flips desync side rapidly" ) );
		sesui::checkbox( _( "Jitter Desync" ), options::vars [ antiaim_config + _( "jitter_desync_side" ) ].val.b );
		sesui::tooltip ( _ ( "Attempt to center real and prevent fake-real alignment" ) );
		sesui::checkbox( _( "Center Real" ), options::vars [ antiaim_config + _( "center_real" ) ].val.b );
		sesui::tooltip ( _ ( "Prevent enemies from bruteforcing head by switching side every shot" ) );
		sesui::checkbox( _( "Anti Bruteforce" ), options::vars [ antiaim_config + _( "anti_bruteforce" ) ].val.b );
		sesui::tooltip ( _ ( "Inverts desync side to face the open, messing up some resolvers" ) );
		sesui::checkbox( _( "Anti Freestanding Prediction" ), options::vars [ antiaim_config + _( "anti_freestand_prediction" ) ].val.b );
		sesui::tooltip ( _ ( "Desync range (from fake to real)" ) );
		sesui::slider( _( "Desync Range" ), options::vars [ antiaim_config + _( "desync_range" ) ].val.f, 0.0f, 60.0f, ( char* ) _( u8"{:.1f}°" ) );
		sesui::tooltip ( _ ( "Desync range on inverted side (from fake to real)" ) );
		sesui::slider( _( "Desync Range Inverted" ), options::vars [ antiaim_config + _( "desync_range_inverted" ) ].val.f, 0.0f, 60.0f, ( char* ) _( u8"{:.1f}°" ) );

		if ( antiaim_name == _( "standing" ) ) {
			sesui::tooltip ( _ ( "Standing desync mode (LBY is extended)" ) );
			sesui::combobox( _( "Desync Type" ), options::vars [ antiaim_config + _( "desync_type" ) ].val.i, { _( "Default" ), _( "LBY" ) } );
			sesui::tooltip ( _ ( "Standing desync mode on inverted side (LBY is extended)" ) );
			sesui::combobox( _( "Desync Type Inverted" ), options::vars [ antiaim_config + _( "desync_type_inverted" ) ].val.i, { _( "Default" ), _( "LBY" ) } );
		}

		gui::end_group( );
	}
}

void gui::player_visuals_controls( const std::string& visual_name ) {
	const auto visuals_config = _( "visuals." ) + visual_name + _( "." );

	namespace gui = sesui::custom;

	if ( gui::begin_group( "Player Visuals", sesui::rect( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
		sesui::tooltip ( _ ( "Select which entities to apply visuals to" ) );
		sesui::multiselect( _( "Filters" ), {
						{ _( "Local" ), options::vars [ _( "visuals.filters" ) ].val.l [ 0 ]},
						{_( "Teammates" ), options::vars [ _( "visuals.filters" ) ].val.l [ 1 ]},
						{_( "Enemies" ), options::vars [ _( "visuals.filters" ) ].val.l [ 2 ]},
						//{_( "Weapons" ), options::vars [ _( "visuals.filters" ) ].val.l [ 3 ]},
						//{_( "Grenades" ), options::vars [ _( "visuals.filters" ) ].val.l [ 4 ]},
						//{_( "Bomb" ), options::vars [ _( "visuals.filters" ) ].val.l [ 5 ]}
			} );

		if ( visual_name == _( "local" ) ) {
			sesui::tooltip ( _ ( "Local visual options" ) );
			sesui::multiselect( _( "Options" ), {
						{ _( "Chams" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 0 ]},
						{_( "Flat Chams" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 1 ]},
						{_( "XQZ Chams" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 2 ]},
						{ _( "Desync Chams" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 3 ]},
						{_( "Desync Chams Fakelag" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 4 ]},
						{_( "Desync Chams Rimlight" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 5 ]},
						{_( "Glow" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 6 ]},
						{_( "Rimlight Overlay" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 7 ]},
						{_( "ESP Box" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 8 ]},
						{_( "Health Bar" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 9 ]},
						{_( "Ammo Bar" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 10 ]},
						{_( "Desync Bar" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 11 ]},
						{_( "Value Text" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 12 ]},
						{_( "Name Tag" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 13 ]},
						{_( "Weapon Name" ), options::vars [ _( "visuals.local.options" ) ].val.l [ 14 ]}
				} );
		}
		else if ( visual_name == _( "enemies" ) ) {
			sesui::tooltip ( _ ( "Enemy visual options" ) );
			sesui::multiselect( _( "Options" ), {
						{ _( "Chams" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 0 ]},
						{_( "Flat Chams" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 1 ]},
						{_( "XQZ Chams" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 2 ]},
						{ _( "Backtrack Chams" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 3 ]},
						{_( "Hit Matrix" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 4 ]},
						{_( "Glow" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 5 ]},
						{_( "Rimlight Overlay" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 6 ]},
						{_( "ESP Box" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 7 ]},
						{_( "Health Bar" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 8 ]},
						{_( "Ammo Bar" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 9 ]},
						{_( "Desync Bar" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 10 ]},
						{_( "Value Text" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 11 ]},
						{_( "Name Tag" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 12 ]},
						{_( "Weapon Name" ), options::vars [ _( "visuals.enemies.options" ) ].val.l [ 13 ]}
				} );
		}
		else if ( visual_name == _( "teammates" ) ) {
			sesui::tooltip ( _ ( "Teammate visual options" ) );
			sesui::multiselect( _( "Options" ), {
						{ _( "Chams" ), options::vars [ _( "visuals.teammates.options" ) ].val.l [ 0 ]},
						{_( "Flat Chams" ), options::vars [ _( "visuals.teammates.options" ) ].val.l [ 1 ]},
						{_( "XQZ Chams" ), options::vars [ _( "visuals.teammates.options" ) ].val.l [ 2 ]},
						{_( "Glow" ), options::vars [ _( "visuals.teammates.options" ) ].val.l [ 3 ]},
						{_( "Rimlight Overlay" ), options::vars [ _( "visuals.teammates.options" ) ].val.l [ 4 ]},
						{_( "ESP Box" ), options::vars [ _( "visuals.teammates.options" ) ].val.l [ 5 ]},
						{_( "Health Bar" ), options::vars [ _( "visuals.teammates.options" ) ].val.l [ 6 ]},
						{_( "Ammo Bar" ), options::vars [ _( "visuals.teammates.options" ) ].val.l [ 7 ]},
						{_( "Desync Bar" ), options::vars [ _( "visuals.teammates.options" ) ].val.l [ 8 ]},
						{_( "Value Text" ), options::vars [ _( "visuals.teammates.options" ) ].val.l [ 9 ]},
						{_( "Name Tag" ), options::vars [ _( "visuals.teammates.options" ) ].val.l [ 10 ]},
						{_( "Weapon Name" ), options::vars [ _( "visuals.teammates.options" ) ].val.l [ 11 ]}
				} );
		}

		sesui::tooltip ( _ ( "Orientation of health bar along bounding box" ) );
		sesui::combobox( _( "Health Bar Location" ), options::vars [ visuals_config + _( "health_bar_location" ) ].val.i, { _( "Left" ), _( "Right" ), _( "Bottom" ), _( "Top" ) } );
		sesui::tooltip ( _ ( "Orientation of ammo bar along bounding box" ) );
		sesui::combobox( _( "Ammo Bar Location" ), options::vars [ visuals_config + _( "ammo_bar_location" ) ].val.i, { _( "Left" ), _( "Right" ), _( "Bottom" ), _( "Top" ) } );
		sesui::tooltip ( _ ( "Orientation of desync amount bar along bounding box" ) );
		sesui::combobox( _( "Desync Bar Location" ), options::vars [ visuals_config + _( "desync_bar_location" ) ].val.i, { _( "Left" ), _( "Right" ), _( "Bottom" ), _( "Top" ) } );
		sesui::tooltip ( _ ( "Orientation of value text along bounding box" ) );
		sesui::combobox( _( "Value Text Location" ), options::vars [ visuals_config + _( "value_text_location" ) ].val.i, { _( "Left" ), _( "Right" ), _( "Bottom" ), _( "Top" ) } );
		sesui::tooltip ( _ ( "Orientation of player name tag along bounding box" ) );
		sesui::combobox( _( "Name Tag Location" ), options::vars [ visuals_config + _( "nametag_location" ) ].val.i, { _( "Left" ), _( "Right" ), _( "Bottom" ), _( "Top" ) } );
		sesui::tooltip ( _ ( "Orientation of player's weapon name along bounding box" ) );
		sesui::combobox( _( "Weapon Name Location" ), options::vars [ visuals_config + _( "weapon_name_location" ) ].val.i, { _( "Left" ), _( "Right" ), _( "Bottom" ), _( "Top" ) } );
		sesui::tooltip ( _ ( "Amount of light reflectivity to apply to player model (exponential)" ) );
		sesui::slider( _( "Chams Reflectivity" ), options::vars [ visuals_config + _( "reflectivity" ) ].val.f, 0.0f, 100.0f, _( "{:.1f}%" ) );
		sesui::tooltip ( _ ( "Amount of phong reflection to apply to player model (exponential)" ) );
		sesui::slider( _( "Chams Phong" ), options::vars [ visuals_config + _( "phong" ) ].val.f, 0.0f, 100.0f, _( "{:.1f}%" ) );


		gui::end_group( );
	}

	if ( gui::begin_group( "Colors", sesui::rect( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
		sesui::tooltip ( _ ( "Chams model color" ) );
		sesui::colorpicker( _( "Chams Color" ), options::vars [ visuals_config + _( "chams_color" ) ].val.c );
		sesui::tooltip ( _ ( "XQZ Chams model color" ) );
		sesui::colorpicker( _( "XQZ Chams Color" ), options::vars [ visuals_config + _( "xqz_chams_color" ) ].val.c );

		if ( visual_name == _ ( "enemies" ) ) {
			sesui::tooltip ( _ ( "Backtrack chams model color" ) );
			sesui::colorpicker ( _ ( "Backtrack Chams Color" ), options::vars [ visuals_config + _ ( "backtrack_chams_color" ) ].val.c );
		}

		if ( visual_name == _( "local" ) ) {
			sesui::tooltip ( _ ( "Desync model color" ) );
			sesui::colorpicker( _( "Desync Chams Color" ), options::vars [ _( "visuals.local.desync_chams_color" ) ].val.c );
			sesui::tooltip ( _ ( "Desync model rimlight overlay color" ) );
			sesui::colorpicker( _( "Desync Chams Rimlight Color" ), options::vars [ _( "visuals.local.desync_rimlight_overlay_color" ) ].val.c );
		}

		if ( visual_name == _ ( "enemies" ) ) {
			sesui::tooltip ( _ ( "Hit model color" ) );
			sesui::colorpicker ( _ ( "Hit Matrix Color" ), options::vars [ visuals_config + _ ( "hit_matrix_color" ) ].val.c );
		}

		sesui::tooltip ( _ ( "Glow outline color" ) );
		sesui::colorpicker( _( "Glow Color" ), options::vars [ visuals_config + _( "glow_color" ) ].val.c );
		sesui::tooltip ( _ ( "Rimlight overlay color" ) );
		sesui::colorpicker( _( "Rimlight Overlay Color" ), options::vars [ visuals_config + _( "rimlight_overlay_color" ) ].val.c );
		sesui::tooltip ( _ ( "ESP box border color" ) );
		sesui::colorpicker( _( "ESP Box Color" ), options::vars [ visuals_config + _( "box_color" ) ].val.c );
		sesui::tooltip ( _ ( "Health bar color" ) );
		sesui::colorpicker( _( "Health Bar Color" ), options::vars [ visuals_config + _( "health_bar_color" ) ].val.c );
		sesui::tooltip ( _ ( "Ammo bar color" ) );
		sesui::colorpicker( _( "Ammo Bar Color" ), options::vars [ visuals_config + _( "ammo_bar_color" ) ].val.c );
		sesui::tooltip ( _ ( "Desync amount bar color" ) );
		sesui::colorpicker( _( "Desync Bar Color" ), options::vars [ visuals_config + _( "desync_bar_color" ) ].val.c );
		sesui::tooltip ( _ ( "Player name tag color" ) );
		sesui::colorpicker( _( "Name Tag Color" ), options::vars [ visuals_config + _( "name_color" ) ].val.c );
		sesui::tooltip ( _ ( "Player weapon name color" ) );
		sesui::colorpicker( _( "Weapon Name Color" ), options::vars [ visuals_config + _( "weapon_color" ) ].val.c );

		gui::end_group( );
	}
}

void gui::draw( ) {
	sesui::globals::dpi = options::vars [ _( "gui.dpi" ) ].val.f;

	if ( !utils::key_state( VK_INSERT ) && open_button_pressed )
		opened = !opened;

	open_button_pressed = utils::key_state( VK_INSERT );

	sesui::begin_frame( _( "Counter-Strike: Global Offensive" ) );

	namespace gui = sesui::custom;

	watermark::draw( );
	keybinds::draw( );

	if ( opened ) {
		if ( gui::begin_window( _( "Sesame" ), sesui::rect( 500, 500, 800, 600 ), opened, sesui::window_flags::no_closebutton ) ) {
			if ( gui::begin_tabs( tab_max ) ) {
				gui::tab( _( "A" ), _( "Legit" ), current_tab_idx );
				gui::tab( _( "B" ), _( "Rage" ), current_tab_idx );
				gui::tab( _( "C" ), _( "Antiaim" ), current_tab_idx );
				gui::tab( _( "D" ), _( "Visuals" ), current_tab_idx );
				gui::tab( _( "E" ), _( "Skins" ), current_tab_idx );
				gui::tab( _( "F" ), _( "Misc" ), current_tab_idx );

				gui::end_tabs( );
			}

			switch ( current_tab_idx ) {
				case tab_legit: {
					if ( gui::begin_subtabs( legit_subtabs_max ) ) {
						gui::subtab( _( "General" ), _( "General legitbot settings" ), sesui::rect( 0.0f, 0.0f, 0.5f, 0.2f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
						gui::subtab( _( "Default" ), _( "Default settings used for unconfigured weapons" ), sesui::rect( 0.5f, 0.0f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
						gui::subtab( _( "Pistol" ), _( "Pistol class configuration" ), sesui::rect( 0.0f, 0.2f, 0.5f, 0.2f ), sesui::rect( 0.0f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
						gui::subtab( _( "Revolver" ), _( "Revolver class configuration" ), sesui::rect( 0.5f, 0.2f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
						gui::subtab( _( "Rifle" ), _( "Rifle, SMG, and shotgun class configuration" ), sesui::rect( 0.0f, 0.4f, 0.5f, 0.2f ), sesui::rect( 0.0f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
						gui::subtab( _( "AWP" ), _( "AWP class configuration" ), sesui::rect( 0.5f, 0.4f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
						gui::subtab( _( "Auto" ), _( "Autosniper class configuration" ), sesui::rect( 0.0f, 0.6f, 0.5f, 0.2f ), sesui::rect( 0.0f, sesui::style.spacing * 0.5f * 3.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
						gui::subtab( _( "Scout" ), _( "Scout class configuration" ), sesui::rect( 0.5f, 0.6f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f * 3.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );

						gui::end_subtabs( );
					}

					switch ( legit_subtab_idx ) {
						case legit_subtabs_main: {
							if ( gui::begin_group( "General Settings", sesui::rect( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
								sesui::combobox( _( "Assistance Type" ), options::vars [ _( "global.assistance_type" ) ].val.i, { _( "None" ), _( "Legit" ), _( "Rage" ) } );

								gui::end_group( );
							}

							if ( gui::begin_group( "Triggerbot", sesui::rect( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
								sesui::tooltip ( _ ( "Automatically shoots when hoving over an enemy" ) );
								sesui::checkbox( _( "Triggerbot" ), options::vars [ _( "legitbot.triggerbot" ) ].val.b );
								sesui::same_line( );
								sesui::tooltip ( _ ( "Triggerbot override key" ) );
								sesui::keybind( _( "Triggerbot Key" ), options::vars [ _( "legitbot.triggerbot_key" ) ].val.i, options::vars [ _( "legitbot.triggerbot_key_mode" ) ].val.i );
								
								sesui::tooltip ( _ ( "Triggerbot will only activate on these hitboxes" ) );
								sesui::multiselect( _( "Hitboxes" ), {
											{ _( "Head" ), options::vars [ _( "legitbot.triggerbot_hitboxes" ) ].val.l [ 0 ] },
											{ _( "Neck" ), options::vars [ _( "legitbot.triggerbot_hitboxes" ) ].val.l [ 1 ] },
											{ _( "Chest" ), options::vars [ _( "legitbot.triggerbot_hitboxes" ) ].val.l [ 2 ] },
											{ _( "Pelvis" ), options::vars [ _( "legitbot.triggerbot_hitboxes" ) ].val.l [ 3 ] },
											{ _( "Arms" ), options::vars [ _( "legitbot.triggerbot_hitboxes" ) ].val.l [ 6 ] }
									} );

								gui::end_group( );
							}
						} break;
						case legit_subtabs_default: {
						} break;
						case legit_subtabs_pistol: {
						} break;
						case legit_subtabs_revolver: {
						} break;
						case legit_subtabs_rifle: {
						} break;
						case legit_subtabs_awp: {
						} break;
						case legit_subtabs_auto: {
						} break;
						case legit_subtabs_scout: {
						} break;
						default: {
						} break;
					}
				} break;
				case tab_rage: {
					if ( gui::begin_subtabs( rage_subtabs_max ) ) {
						gui::subtab( _( "General" ), _( "General ragebot and accuracy settings" ), sesui::rect( 0.0f, 0.0f, 0.5f, 0.2f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
						gui::subtab( _( "Default" ), _( "Default settings used for unconfigured weapons" ), sesui::rect( 0.5f, 0.0f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
						gui::subtab( _( "Pistol" ), _( "Pistol class configuration" ), sesui::rect( 0.0f, 0.2f, 0.5f, 0.2f ), sesui::rect( 0.0f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
						gui::subtab( _( "Revolver" ), _( "Revolver class configuration" ), sesui::rect( 0.5f, 0.2f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
						gui::subtab( _( "Rifle" ), _( "Rifle, SMG, and shotgun class configuration" ), sesui::rect( 0.0f, 0.4f, 0.5f, 0.2f ), sesui::rect( 0.0f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
						gui::subtab( _( "AWP" ), _( "AWP class configuration" ), sesui::rect( 0.5f, 0.4f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
						gui::subtab( _( "Auto" ), _( "Autosniper class configuration" ), sesui::rect( 0.0f, 0.6f, 0.5f, 0.2f ), sesui::rect( 0.0f, sesui::style.spacing * 0.5f * 3.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
						gui::subtab( _( "Scout" ), _( "Scout class configuration" ), sesui::rect( 0.5f, 0.6f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f * 3.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );

						gui::end_subtabs( );
					}

					switch ( rage_subtab_idx ) {
						case rage_subtabs_main: {
							if ( gui::begin_group( "General Settings", sesui::rect( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
								sesui::tooltip ( _ ( "Enabled or disables all ragebot settings" ) );
								sesui::combobox( _( "Assistance Type" ), options::vars [ _( "global.assistance_type" ) ].val.i, { _( "None" ), _( "Legit" ), _( "Rage" ) } );
								sesui::tooltip ( _ ( "Automatically knife players" ) );
								sesui::checkbox( _( "Knife Bot" ), options::vars [ _( "ragebot.knife_bot" ) ].val.b );
								sesui::tooltip ( _ ( "Automatically zeus players" ) );
								sesui::checkbox( _( "Zeus Bot" ), options::vars [ _( "ragebot.zeus_bot" ) ].val.b );

								gui::end_group( );
							}

							if ( gui::begin_group( "Accuracy", sesui::rect( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
								sesui::tooltip ( _ ( "Correct and predict fakelag" ) );
								sesui::checkbox( _( "Fix Fakelag" ), options::vars [ _( "ragebot.fix_fakelag" ) ].val.b );
								sesui::tooltip ( _ ( "Resolve animation desync" ) );
								sesui::checkbox( _( "Resolve Desync" ), options::vars [ _( "ragebot.resolve_desync" ) ].val.b );
								sesui::tooltip ( _ ( "Attempts to aim at points that align with all resolved matricies" ) );
								sesui::checkbox( _( "Safe Point" ), options::vars [ _( "ragebot.safe_point" ) ].val.b );
								sesui::same_line( );
								sesui::tooltip ( _ ( "Safe point override keybind" ) );
								sesui::keybind( _( "Safe Point Key" ), options::vars [ _( "ragebot.safe_point_key" ) ].val.i, options::vars [ _( "ragebot.safe_point_key_mode" ) ].val.i );
								sesui::tooltip ( _ ( "Automatically cock revolver until fully cocked" ) );
								sesui::checkbox( _( "Auto Revolver" ), options::vars [ _( "ragebot.auto_revolver" ) ].val.b );
								sesui::tooltip ( _ ( "Doubletap override key" ) );
								sesui::keybind( _( "Doubletap Key" ), options::vars [ _( "ragebot.dt_key" ) ].val.i, options::vars [ _( "ragebot.dt_key_mode" ) ].val.i );

								gui::end_group( );
							}
						} break;
						case rage_subtabs_default: {
							weapon_controls( _( "default" ) );
						} break;
						case rage_subtabs_pistol: {
							weapon_controls( _( "pistol" ) );
						} break;
						case rage_subtabs_revolver: {
							weapon_controls( _( "revolver" ) );
						} break;
						case rage_subtabs_rifle: {
							weapon_controls( _( "rifle" ) );
						} break;
						case rage_subtabs_awp: {
							weapon_controls( _( "awp" ) );
						} break;
						case rage_subtabs_auto: {
							weapon_controls( _( "auto" ) );
						} break;
						case rage_subtabs_scout: {
							weapon_controls( _( "scout" ) );
						} break;
						default: {
						} break;
					}
				} break;
				case tab_antiaim: {
					if ( gui::begin_subtabs( antiaim_subtabs_max ) ) {
						gui::subtab( _( "Air" ), _( "In air antiaim settings" ), sesui::rect( 0.0f, 0.0f, 0.5f, 0.2f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), antiaim_subtab_idx );
						gui::subtab( _( "Moving" ), _( "Moving antiaim settings" ), sesui::rect( 0.5f, 0.0f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), antiaim_subtab_idx );
						gui::subtab( _( "Slow Walk" ), _( "Slow walk antiaim settings" ), sesui::rect( 0.0f, 0.2f, 0.5f, 0.2f ), sesui::rect( 0.0f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), antiaim_subtab_idx );
						gui::subtab( _( "Standing" ), _( "Standing antiaim settings" ), sesui::rect( 0.5f, 0.2f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), antiaim_subtab_idx );
						gui::subtab( _( "Other" ), _( "Fakelag, manual aa, and other antiaim features" ), sesui::rect( 0.0f, 0.4f, 0.5f, 0.2f ), sesui::rect( 0.0f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), antiaim_subtab_idx );

						gui::end_subtabs( );
					}

					switch ( antiaim_subtab_idx ) {
						case antiaim_subtabs_air: {
							antiaim_controls( _( "air" ) );
						} break;
						case antiaim_subtabs_moving: {
							antiaim_controls( _( "moving" ) );
						} break;
						case antiaim_subtabs_slow_walk: {
							antiaim_controls( _( "slow_walk" ) );
						} break;
						case antiaim_subtabs_standing: {
							antiaim_controls( _( "standing" ) );
						} break;
						case antiaim_subtabs_other: {
							if ( gui::begin_group( "General", sesui::rect( 0.0f, 0.0f, 1.0f, 1.0f ), sesui::rect( 0.0f, 0.0f, 0.0f, 0.0f ) ) ) {
								sesui::tooltip ( _ ( "Slow walk maximum speed (in percent of units/s out of max_speed/3)" ) );
								sesui::slider( _( "Slow Walk Speed" ), options::vars [ _( "antiaim.slow_walk_speed" ) ].val.f, 0.0f, 100.0f, _( "{:.1f}%" ) );
								sesui::same_line( );
								sesui::tooltip ( _ ( "Slow walk override key" ) );
								sesui::keybind( _( "Slow Walk Key" ), options::vars [ _( "antiaim.slow_walk_key" ) ].val.i, options::vars [ _( "antiaim.slow_walk_key_mode" ) ].val.i );
								sesui::tooltip ( _ ( "Slide on slow walk" ) );
								sesui::checkbox( _( "Fake Walk" ), options::vars [ _( "antiaim.fakewalk" ) ].val.b );
								sesui::tooltip ( _ ( "Moves hitboxes to ducking position, but shoot position will remain above ducking model" ) );
								sesui::checkbox( _( "Fake Duck" ), options::vars [ _( "antiaim.fakeduck" ) ].val.b );
								sesui::same_line( );
								sesui::tooltip ( _ ( "Fake duck override key" ) );
								sesui::keybind( _( "Fake Duck Key" ), options::vars [ _( "antiaim.fakeduck_key" ) ].val.i, options::vars [ _( "antiaim.fakeduck_key_mode" ) ].val.i );
								sesui::tooltip ( _ ( "Fake duck mode" ) );
								sesui::combobox( _( "Fake Duck Mode" ), options::vars [ _( "antiaim.fakeduck_mode" ) ].val.i, { _( "Normal" ), _( "Full" ) } );
								sesui::tooltip ( _ ( "Manual left antiaim override key" ) );
								sesui::keybind( _( "Manual Left Key" ), options::vars [ _( "antiaim.manual_left_key" ) ].val.i, options::vars [ _( "antiaim.manual_left_key_mode" ) ].val.i );
								sesui::tooltip ( _ ( "Manual right antiaim override key" ) );
								sesui::keybind( _( "Manual Right Key" ), options::vars [ _( "antiaim.manual_right_key" ) ].val.i, options::vars [ _( "antiaim.manual_right_key_mode" ) ].val.i );
								sesui::tooltip ( _ ( "Manual back antiaim override key" ) );
								sesui::keybind( _( "Manual Back Key" ), options::vars [ _( "antiaim.manual_back_key" ) ].val.i, options::vars [ _( "antiaim.manual_back_key_mode" ) ].val.i );
								sesui::tooltip ( _ ( "Desync inverter override key (flips desync side on key press)" ) );
								sesui::keybind( _( "Desync Inverter Key" ), options::vars [ _( "antiaim.desync_invert_key" ) ].val.i, options::vars [ _( "antiaim.desync_invert_key_mode" ) ].val.i );

								gui::end_group( );
							}
						} break;
						default: {
						} break;
					}
				} break;
				case tab_visuals: {
					if ( gui::begin_subtabs( visuals_subtabs_max ) ) {
						gui::subtab( _( "Local Player" ), _( "Visuals used on local player" ), sesui::rect( 0.0f, 0.0f, 0.5f, 0.2f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), visuals_subtab_idx );
						gui::subtab( _( "Enemies" ), _( "Visuals used on filtered enemies" ), sesui::rect( 0.5f, 0.0f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), visuals_subtab_idx );
						gui::subtab( _( "Teammates" ), _( "Visuals used on filtered teammates" ), sesui::rect( 0.0f, 0.2f, 0.5f, 0.2f ), sesui::rect( 0.0f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), visuals_subtab_idx );
						gui::subtab( _( "Other" ), _( "Other visual options" ), sesui::rect( 0.5f, 0.2f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), visuals_subtab_idx );

						gui::end_subtabs( );
					}

					switch ( visuals_subtab_idx ) {
						case visuals_subtabs_local: {
							player_visuals_controls( _( "local" ) );
						} break;
						case visuals_subtabs_enemies: {
							player_visuals_controls( _( "enemies" ) );
						} break;
						case visuals_subtabs_teammates: {
							player_visuals_controls( _( "teammates" ) );
						} break;
						case visuals_subtabs_other: {
							if ( gui::begin_group( "World", sesui::rect( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
								sesui::tooltip ( _ ( "Locate bomb with warning icon" ) );
								sesui::checkbox( _( "Bomb ESP" ), options::vars [ _( "visuals.other.bomb_esp" ) ].val.b );
								sesui::tooltip ( _ ( "Display bomb explosion and defuse timer" ) );
								sesui::checkbox( _( "Bomb Timer" ), options::vars [ _( "visuals.other.bomb_timer" ) ].val.b );
								sesui::tooltip ( _ ( "Creates laser trail along bullet path" ) );
								sesui::checkbox( _( "Bullet Tracers" ), options::vars [ _( "visuals.other.bullet_tracers" ) ].val.b );
								sesui::same_line( );
								sesui::tooltip ( _ ( "Color of bullet laser trail" ) );
								sesui::colorpicker( _( "Bullet Tracer Color" ), options::vars [ _( "visuals.other.bullet_tracer_color" ) ].val.c );
								sesui::tooltip ( _ ( "Shows area of bullet impact" ) );
								sesui::checkbox( _( "Bullet Impacts" ), options::vars [ _( "visuals.other.bullet_impacts" ) ].val.b );
								sesui::same_line( );
								sesui::tooltip ( _ ( "Color of bullet impact" ) );
								sesui::colorpicker( _( "Bullet Impacts Color" ), options::vars [ _( "visuals.other.bullet_impact_color" ) ].val.c );
								sesui::tooltip ( _ ( "Shows path grenade will follow when thrown" ) );
								sesui::checkbox( _( "Grenade Trajectories" ), options::vars [ _( "visuals.other.grenade_trajectories" ) ].val.b );
								sesui::same_line( );
								sesui::tooltip ( _ ( "Color of predicted grenade trajectory" ) );
								sesui::colorpicker( _( "Grenade Trajectories Color" ), options::vars [ _( "visuals.other.grenade_trajectory_color" ) ].val.c );
								sesui::tooltip ( _ ( "Shows where grenades will bounce after thrown" ) );
								sesui::checkbox( _( "Grenade Bounces" ), options::vars [ _( "visuals.other.grenade_bounces" ) ].val.b );
								sesui::same_line( );
								sesui::tooltip ( _ ( "Color of grenade impacts, or bounces on walls and floor" ) );
								sesui::colorpicker( _( "Grenade Bounces Color" ), options::vars [ _( "visuals.other.grenade_bounce_color" ) ].val.c );
								sesui::tooltip ( _ ( "Visualizes grenade blast and fire radius" ) );
								sesui::checkbox( _( "Grenade Blast Radii" ), options::vars [ _( "visuals.other.grenade_blast_radii" ) ].val.b );
								sesui::same_line( );
								sesui::tooltip ( _ ( "Color of predicted grenade range or fire radius" ) );
								sesui::colorpicker( _( "Grenade Blast Radii Color" ), options::vars [ _( "visuals.other.grenade_radii_color" ) ].val.c );

								gui::end_group( );
							}

							if ( gui::begin_group( "Other", sesui::rect( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
								sesui::tooltip ( _ ( "Removals of game visuals (disable post processing to increase performance)" ) );
								sesui::multiselect( _( "Removals" ), {
									{ _( "Smoke" ), options::vars [ _( "visuals.other.removals" ) ].val.l [ 0 ]},
									{_( "Flash" ), options::vars [ _( "visuals.other.removals" ) ].val.l [ 1 ]},
									{_( "Scope" ), options::vars [ _( "visuals.other.removals" ) ].val.l [ 2 ]},
									{ _( "Aim Punch" ), options::vars [ _( "visuals.other.removals" ) ].val.l [ 3 ]},
									{_( "View Punch" ), options::vars [ _( "visuals.other.removals" ) ].val.l [ 4 ]},
									{_( "Zoom" ), options::vars [ _( "visuals.other.removals" ) ].val.l [ 5 ]},
									{_( "Occlusion" ), options::vars [ _( "visuals.other.removals" ) ].val.l [ 6 ]},
									{_( "Killfeed Decay" ), options::vars [ _( "visuals.other.removals" ) ].val.l [ 7 ]},
									{_( "Post Processing" ), options::vars [ _( "visuals.other.removals" ) ].val.l [ 8 ]}
									} );
								sesui::tooltip ( _ ( "Camera FOV" ) );
								sesui::slider( _( "FOV" ), options::vars [ _( "visuals.other.fov" ) ].val.f, 0.0f, 180.0f, ( char* ) _( u8"{:.1f}°" ) );
								sesui::tooltip ( _ ( "Viewmodel FOV" ) );
								sesui::slider( _( "Viewmodel FOV" ), options::vars [ _( "visuals.other.viewmodel_fov" ) ].val.f, 0.0f, 180.0f, ( char* ) _( u8"{:.1f}°" ) );
								sesui::tooltip ( _ ( "Screen aspect ratio (lower values will stretch screen)" ) );
								sesui::slider( _( "Aspect Ratio" ), options::vars [ _( "visuals.other.aspect_ratio" ) ].val.f, 0.1f, 2.0f );

								sesui::tooltip ( _ ( "When to log ragebot shot data" ) );
								sesui::multiselect( _( "Logs" ), {
									{ _( "Hits" ), options::vars [ _( "visuals.other.logs" ) ].val.l [ 0 ]},
									{_( "Spread Misses" ), options::vars [ _( "visuals.other.logs" ) ].val.l [ 1 ]},
									{_( "Resolver Misses" ), options::vars [ _( "visuals.other.logs" ) ].val.l [ 2 ]},
									{ _( "Wrong Hitbox" ), options::vars [ _( "visuals.other.logs" ) ].val.l [ 3 ]},
									//{_( "Manual Shots" ), options::vars [ _( "visuals.other.logs" ) ].val.l [ 4 ]}
									} );

								sesui::tooltip ( _ ( "Visualizes bullet spread area" ) );
								sesui::checkbox( _( "Spread Circle" ), options::vars [ _( "visuals.other.spread_circle" ) ].val.b );
								sesui::same_line( );
								sesui::tooltip ( _ ( "Color of bullet spread circle" ) );
								sesui::colorpicker( _( "Spread Circle Color" ), options::vars [ _( "visuals.other.spread_circle_color" ) ].val.c );
								sesui::tooltip ( _ ( "Gives spread fading or gradient effect" ) );
								sesui::checkbox( _( "Gradient Spread Circle" ), options::vars [ _( "visuals.other.gradient_spread_circle" ) ].val.b );
								sesui::tooltip ( _ ( "Creates arrows which point to off-screen players" ) );
								sesui::checkbox( _( "Offscreen ESP" ), options::vars [ _( "visuals.other.offscreen_esp" ) ].val.b );
								sesui::same_line( );
								sesui::tooltip ( _ ( "Color of off-screen arrows" ) );
								sesui::colorpicker( _( "Offscreen ESP Color" ), options::vars [ _( "visuals.other.offscreen_esp_color" ) ].val.c );
								sesui::tooltip ( _ ( "Distance of off-screen arrows from edge of screen" ) );
								sesui::slider( _( "Offscreen ESP Distance" ), options::vars [ _( "visuals.other.offscreen_esp_distance" ) ].val.f, 0.0f, 100.0f, _( "{:.1f}%" ) );
								sesui::tooltip ( _ ( "Size of off-screen arrows, in pixels" ) );
								sesui::slider( _( "Offscreen ESP Size" ), options::vars [ _( "visuals.other.offscreen_esp_size" ) ].val.f, 0.0f, 100.0f, std::to_string(static_cast<int>( options::vars [ _ ( "visuals.other.offscreen_esp_size" ) ].val.f)) + _( " px" ) );

								sesui::tooltip ( _ ( "Sesame watermark" ) );
								sesui::checkbox( _( "Watermark" ), options::vars [ _( "visuals.other.watermark" ) ].val.b );
								sesui::tooltip ( _ ( "Active keybind list" ) );
								sesui::checkbox( _( "Keybind List" ), options::vars [ _( "visuals.other.keybind_list" ) ].val.b );

								sesui::tooltip ( _ ( "World color modulation (night mode and asus walls)" ) );
								sesui::colorpicker( _( "World Color" ), options::vars [ _( "visuals.other.world_color" ) ].val.c );
								sesui::tooltip ( _ ( "Prop color modulation (prop color and asus props)" ) );
								sesui::colorpicker( _( "Prop Color" ), options::vars [ _( "visuals.other.prop_color" ) ].val.c );
								//sesui::tooltip ( _ ( "Menu accent color" ) );
								//sesui::colorpicker( _( "Accent Color" ), options::vars [ _( "visuals.other.accent_color" ) ].val.c );
								//sesui::colorpicker( _( "Secondary Accent Color" ), options::vars [ _( "visuals.other.secondary_accent_color" ) ].val.c );
								//sesui::colorpicker( _( "Logo Color" ), options::vars [ _( "visuals.other.logo_color" ) ].val.c );
								sesui::tooltip ( _ ( "Creates sound effect when players are damaged" ) );
								sesui::combobox( _( "Hit Sound" ), options::vars [ _( "visuals.other.hit_sound" ) ].val.i, { _( "None" ), _( "Arena Switch" ), _( "Fall Pain" ), _( "Bolt" ), _( "Neck Snap" ), _( "Power Switch" ), _( "Glass" ), _( "Bell" ), _( "COD" ), _( "Rattle" ), _( "Sesame" ) } );

								gui::end_group( );
							}
						} break;
						default: {
						} break;
					}
				} break;
				case tab_skins: {
					if ( gui::begin_subtabs( skins_subtabs_max ) ) {
						gui::subtab( _( "Skin Changer" ), _( "Apply custom skins to your weapons" ), sesui::rect( 0.0f, 0.0f, 0.5f, 0.2f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), skins_subtab_idx );
						gui::subtab( _( "Model Changer" ), _( "Replace game models with your own" ), sesui::rect( 0.5f, 0.0f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), skins_subtab_idx );

						gui::end_subtabs( );
					}

					switch ( skins_subtab_idx ) {
						case skins_subtabs_skins: {
						} break;
						case skins_subtabs_models: {
						} break;
						default: {
						} break;
					}
				} break;
				case tab_misc: {
					if ( gui::begin_subtabs( misc_subtabs_max ) ) {
						gui::subtab( _( "Movement" ), _( "Movement related cheats" ), sesui::rect( 0.0f, 0.0f, 0.5f, 0.2f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), misc_subtab_idx );
						gui::subtab( _( "Effects" ), _( "Miscellaneous visual effects" ), sesui::rect( 0.5f, 0.0f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), misc_subtab_idx );
						gui::subtab( _( "Player List" ), _( "Whitelist, clantag stealer, and bodyaim priority" ), sesui::rect( 0.0f, 0.2f, 0.5f, 0.2f ), sesui::rect( 0.0f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), misc_subtab_idx );
						gui::subtab( _( "Cheat" ), _( "Cheat settings and panic button" ), sesui::rect( 0.5f, 0.2f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), misc_subtab_idx );
						gui::subtab( _( "Configuration" ), _( "Cheat configuration manager" ), sesui::rect( 0.0f, 0.4f, 0.5f, 0.2f ), sesui::rect( 0.0f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), misc_subtab_idx );
						gui::subtab( _( "Scripts" ), _( "Script manager" ), sesui::rect( 0.5f, 0.4f, 0.5f, 0.2f ), sesui::rect( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), misc_subtab_idx );

						gui::end_subtabs( );
					}

					switch ( misc_subtab_idx ) {
						case misc_subtabs_movement: {
							if ( gui::begin_group( "General", sesui::rect( 0.0f, 0.0f, 1.0f, 1.0f ), sesui::rect( 0.0f, 0.0f, 0.0f, 0.0f ) ) ) {
								sesui::tooltip ( _ ( "Automatically blocks players in front of, and follows movement of players below you" ) );
								sesui::checkbox( _( "Block Bot" ), options::vars [ _( "misc.movement.block_bot" ) ].val.b );
								sesui::same_line( );
								sesui::tooltip ( _ ( "Block bot activates on key press" ) );
								sesui::keybind( _( "Block Bot Key" ), options::vars [ _( "misc.movement.block_bot_key" ) ].val.i, options::vars [ _( "misc.movement.block_bot_key_mode" ) ].val.i );
								sesui::tooltip ( _ ( "Automatic jump, or bunnyhopping" ) );
								sesui::checkbox( _( "Auto Jump" ), options::vars [ _( "misc.movement.bhop" ) ].val.b );
								sesui::tooltip ( _ ( "Automatically move forward when in air" ) );
								sesui::checkbox( _( "Auto Forward" ), options::vars [ _( "misc.movement.auto_forward" ) ].val.b );
								sesui::tooltip ( _ ( "Automatically strafe when in air" ) );
								sesui::checkbox( _( "Auto Strafer" ), options::vars [ _( "misc.movement.auto_strafer" ) ].val.b );
								sesui::tooltip ( _ ( "Automatically strafe in direction of key press" ) );
								sesui::checkbox( _( "Directional Auto Strafer" ), options::vars [ _( "misc.movement.omnidirectional_auto_strafer" ) ].val.b );
								//sesui::checkbox ( _ ( "Air Stuck" ), options::vars [ _ ( "misc.movement.airstuck" ) ].val.b );
								//sesui::keybind ( _ ( "Air Stuck Key" ), options::vars [ _ ( "misc.movement.airstuck_key" ) ].val.i, options::vars [ _ ( "misc.movement.airstuck_key_mode" ) ].val.i );

								gui::end_group( );
							}
						} break;
						case misc_subtabs_effects: {
							if ( gui::begin_group( "General", sesui::rect( 0.0f, 0.0f, 1.0f, 1.0f ), sesui::rect( 0.0f, 0.0f, 0.0f, 0.0f ) ) ) {
								sesui::tooltip ( _ ( "Third person camera mode" ) );
								sesui::checkbox( _( "Third Person" ), options::vars [ _( "misc.effects.third_person" ) ].val.b );
								sesui::same_line( );
								sesui::tooltip ( _ ( "Third person override key" ) );
								sesui::keybind( _( "Third Person Key" ), options::vars [ _( "misc.effects.third_person_key" ) ].val.i, options::vars [ _( "misc.effects.third_person_key_mode" ) ].val.i );
								sesui::tooltip ( _ ( "Third person range, in units" ) );
								sesui::slider( _( "Third Person Range" ), options::vars [ _( "misc.effects.third_person_range" ) ].val.f, 0.0f, 500.0f, _( "{:.1f} units" ) );
								sesui::tooltip ( _ ( "Ragdoll forces scale (multiplier)" ) );
								sesui::slider( _( "Ragdoll Force Scale" ), options::vars [ _( "misc.effects.ragdoll_force_scale" ) ].val.f, 0.0f, 10.0f, _( "x{:.1f}" ) );
								sesui::tooltip ( _ ( "Custom clan tag" ) );
								sesui::checkbox( _( "Clan Tag" ), options::vars [ _( "misc.effects.clantag" ) ].val.b );
								sesui::tooltip ( _ ( "Custom clan tag animation" ) );
								sesui::combobox( _( "Clan Tag Animation" ), options::vars [ _( "misc.effects.clantag_animation" ) ].val.i, { _( "Static" ), _( "Marquee" ), _( "Capitalize" ), _( "Heart" ) } );

								std::string clantag_text = options::vars [ _( "misc.effects.clantag_text" ) ].val.s;
								sesui::tooltip ( _ ( "Custom clantag message" ) );
								sesui::textbox( _( "Clan Tag Text" ), clantag_text );
								strcpy_s( options::vars [ _( "misc.effects.clantag_text" ) ].val.s, clantag_text.c_str( ) );

								sesui::tooltip ( _ ( "Revolver cock sound multiplier (reduces cricket sound)" ) );
								sesui::slider( _( "Revolver Cock Volume" ), options::vars [ _( "misc.effects.revolver_cock_volume" ) ].val.f, 0.0f, 1.0f, _( "x{:.1f}" ) );
								sesui::tooltip ( _ ( "Weapon sound efffects volume multiplier (shots and reload)" ) );
								sesui::slider( _( "Weapon Volume" ), options::vars [ _( "misc.effects.weapon_volume" ) ].val.f, 0.0f, 1.0f, _( "x{:.1f}" ) );

								gui::end_group( );
							}
						} break;
						case misc_subtabs_plist: {
						} break;
						case misc_subtabs_cheat: {
							if ( gui::begin_group( "Menu", sesui::rect( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
								//int dpi = ( options::vars [ _ ( "gui.dpi" ) ].val.f < 1.0f ) ? 0 : static_cast< int >( options::vars [ _ ( "gui.dpi" ) ].val.f );
								//
								//sesui::combobox ( _ ( "GUI DPI" ), dpi, { _ ( "0.5" ), _ ( "1.0" ), _ ( "2.0" ), _ ( "3.0" ) } );
								//
								//switch ( dpi ) {
								//case 0: options::vars [ _ ( "gui.dpi" ) ].val.f = 0.5f; break;
								//case 1: options::vars [ _ ( "gui.dpi" ) ].val.f = 1.0f; break;
								//case 2: options::vars [ _ ( "gui.dpi" ) ].val.f = 2.0f; break;
								//case 3: options::vars [ _ ( "gui.dpi" ) ].val.f = 3.0f; break;
								//}

								gui::end_group( );
							}

							if ( gui::begin_group( "Cheat", sesui::rect( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
								gui::end_group( );
							}
						} break;
						case misc_subtabs_configs: {
							if ( gui::begin_group( "Configs", sesui::rect( 0.0f, 0.0f, 0.5f, 0.5f ), sesui::rect( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
								for ( const auto& config : configs ) {
									sesui::tooltip ( config.data ( ) );
									if ( sesui::button( config.data( ) ) )
										selected_config = config;
								}

								gui::end_group( );
							}

							if ( gui::begin_group ( "Cloud Configs", sesui::rect ( 0.0f, 0.5f, 0.5f, 0.5f ), sesui::rect ( 0.0f, sesui::style.spacing, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
								//gui_mutex.lock ( );
								//const auto loading_list = last_config_user != config_user;
								//gui_mutex.unlock ( );
								//
								///* loading new config list */
								//if ( loading_list ) {
								//	sesui::text (_("Loading...") );
								//}
								//else if ( cloud_config_list ) {
								//	cJSON* iter = nullptr;
								//
								//	cJSON_ArrayForEach ( iter, cloud_config_list ) {
								//		const auto config_id = cJSON_GetObjectItemCaseSensitive ( iter, _ ( "config_id" ) );
								//		const auto config_code = cJSON_GetObjectItemCaseSensitive ( iter, _ ( "config_code" ) );
								//		const auto description = cJSON_GetObjectItemCaseSensitive ( iter, _ ( "description" ) );
								//		const auto creation_date = cJSON_GetObjectItemCaseSensitive ( iter, _ ( "creation_date" ) );
								//
								//		if ( !cJSON_IsNumber ( config_id ) )
								//			continue;
								//
								//		if ( !cJSON_IsString ( creation_date ) || !creation_date->valuestring )
								//			continue;
								//
								//		if ( !cJSON_IsString ( config_code ) || !config_code->valuestring )
								//			continue;
								//
								//		if ( !cJSON_IsString ( description ) || !description->valuestring )
								//			continue;
								//
								//		if ( sesui::button ( iter->string ) ) {
								//			gui_mutex.lock ( );
								//			::gui::config_code = config_code->valuestring;
								//			gui_mutex.unlock ( );
								//		}
								//	}
								//}

								gui::end_group ( );
							}

							if ( gui::begin_group( "Config Actions", sesui::rect( 0.5f, 0.0f, 0.5f, 0.5f ), sesui::rect( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
								sesui::tooltip ( _ ( "Load config by name" ) );
								sesui::textbox( _( "Config Name" ), selected_config );
								
								sesui::tooltip ( _ ( "Saves config with name provided above, creates config if one with name does not already exist" ) );
								if ( sesui::button( _( "Save" ) ) ) {
									char appdata [ MAX_PATH ];
								
									if ( SUCCEEDED( LI_FN( SHGetFolderPathA )( nullptr, N( 5 ), nullptr, N( 0 ), appdata ) ) ) {
										LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame" ) ).data( ), nullptr );
										LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame\\configs" ) ).data( ), nullptr );
									}
								
									options::save( options::vars, std::string( appdata ) + _( "\\sesame\\configs\\" ) + selected_config + _( ".xml" ) );
								
									gui_mutex.lock ( );
									load_cfg_list ( );
									gui_mutex.unlock ( );
								
									csgo::i::engine->client_cmd_unrestricted( _( "play ui\\buttonclick" ) );
								}
								
								sesui::tooltip ( _ ( "Loads config with name provided above" ) );
								if ( sesui::button( _( "Load" ) ) ) {
									char appdata [ MAX_PATH ];
								
									if ( SUCCEEDED( LI_FN( SHGetFolderPathA )( nullptr, N( 5 ), nullptr, N( 0 ), appdata ) ) ) {
										LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame" ) ).data( ), nullptr );
										LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame\\configs" ) ).data( ), nullptr );
									}
								
									options::load( options::vars, std::string( appdata ) + _( "\\sesame\\configs\\" ) + selected_config + _( ".xml" ) );
								
									csgo::i::engine->client_cmd_unrestricted( _( "play ui\\buttonclick" ) );
								}
								
								sesui::tooltip ( _ ( "Deletes config file with name provided above" ) );
								if ( sesui::button( _( "Delete" ) ) ) {
									char appdata [ MAX_PATH ];
								
									if ( SUCCEEDED( LI_FN( SHGetFolderPathA )( nullptr, N( 5 ), nullptr, N( 0 ), appdata ) ) ) {
										LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame" ) ).data( ), nullptr );
										LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame\\configs" ) ).data( ), nullptr );
									}
								
									std::remove( ( std::string( appdata ) + _( "\\sesame\\configs\\" ) + selected_config + _( ".xml" ) ).c_str( ) );
								
									gui_mutex.lock ( );
									load_cfg_list ( );
									gui_mutex.unlock ( );
								
									csgo::i::engine->client_cmd_unrestricted( _( "play ui\\buttonclick" ) );
								}
								
								sesui::tooltip ( _ ( "Reloads config list" ) );
								if ( sesui::button( _( "Refresh List" ) ) ) {
									gui_mutex.lock ( );
									load_cfg_list( );
									gui_mutex.unlock ( );
									csgo::i::engine->client_cmd_unrestricted( _( "play ui\\buttonclick" ) );
								}
								
								//sesui::tooltip ( _ ( "Config description when uploading to cloud" ) );
								//sesui::textbox ( _ ( "Config Description" ), config_description );
								//sesui::tooltip ( _ ( "Config access permissions\Public: allows anyone to view and access the config\nPrivate: allows only you to access the config\nUnlisted: allows only those with a special code to access the config" ) );
								//sesui::combobox ( _ ( "Config Access" ), config_access, { _ ( "Public" ) , _ ( "Private" ) , _ ( "Unlisted" ) } );
								//
								//sesui::tooltip ( _ ( "Uploads selected config to cloud" ) );
								//if ( sesui::button ( _ ( "Upload To Cloud" ) ) )
								//	upload_to_cloud = true;

								gui::end_group( );
							}

							if ( gui::begin_group ( "Cloud Config Actions", sesui::rect ( 0.5f, 0.5f, 0.5f, 0.5f ), sesui::rect ( 0.0f, sesui::style.spacing, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
								//sesui::textbox ( _ ( "Search By User" ), config_user );
								//
								//if ( sesui::button ( _ ( "My Configs" ) ) )
								//	config_user = g_username;							
								//
								//gui_mutex.lock ( );
								//sesui::textbox ( _ ( "Config Code" ), config_code );
								//gui_mutex.unlock ( );
								//
								//if ( sesui::button ( _ ( "Download Config" ) ) ) {
								//	gui_mutex.lock ( );
								//	download_config_code = true;
								//	gui_mutex.unlock ( );
								//}

								gui::end_group ( );
							}
						} break;
						case misc_subtabs_scripts: {
						} break;
						default: {
						} break;
					}
				} break;
			}

			gui::end_window( );
		}
	}

	sesui::render( );
	sesui::end_frame( );
}

void gui::watermark::draw( ) {

}

void gui::keybinds::draw( ) {
	std::vector< std::string > entries {

	};

	struct keybind_t {
		int& key;
		int& mode;
	};

	auto find_keybind = [ ] ( std::string key ) -> keybind_t {
		return { options::vars [ key ].val.i, options::vars [ key + _( "_mode" ) ].val.i };
	};

	auto add_key_entry = [ & ] ( const keybind_t& keybind, std::string key ) {
		if ( !utils::keybind_active( keybind.key, keybind.mode ) )
			return;

		switch ( keybind.mode ) {
			case 0: entries.push_back( _( "[HOLD] " ) + key ); break;
			case 1: entries.push_back( _( "[TOGGLE] " ) + key ); break;
			case 2: entries.push_back( _( "[ALWAYS] " ) + key ); break;
		}
	};

	static auto& blockbot = options::vars [ _( "misc.movement.block_bot" ) ].val.b;
	static auto& keybind_list = options::vars [ _( "visuals.other.keybind_list" ) ].val.b;
	static auto& triggerbot = options::vars [ _( "legitbot.triggerbot" ) ].val.b;
	static auto& fakeduck = options::vars [ _( "antiaim.fakeduck" ) ].val.b;
	static auto& assistance_type = options::vars [ _( "global.assistance_type" ) ].val.i;
	static auto& third_person = options::vars [ _( "misc.effects.third_person" ) ].val.b;

	static auto triggerbot_key = find_keybind( _( "legitbot.triggerbot_key" ) );
	static auto safe_point_key = find_keybind( _( "ragebot.safe_point_key" ) );
	static auto doubletap_key = find_keybind( _( "ragebot.dt_key" ) );
	static auto slow_walk_key = find_keybind( _( "antiaim.slow_walk_key" ) );
	static auto fakeduck_key = find_keybind( _( "antiaim.fakeduck_key" ) );
	static auto left_key = find_keybind( _( "antiaim.manual_left_key" ) );
	static auto right_key = find_keybind( _( "antiaim.manual_right_key" ) );
	static auto back_key = find_keybind( _( "antiaim.manual_back_key" ) );
	static auto inverter_key = find_keybind( _( "antiaim.desync_invert_key" ) );
	static auto blockbot_key = find_keybind( _( "misc.movement.block_bot_key" ) );
	static auto thirdperson_key = find_keybind( _( "misc.effects.third_person_key" ) );

	if ( csgo::i::engine->is_in_game( ) && csgo::i::engine->is_connected( ) ) {
		if ( triggerbot && assistance_type == 1 )
			add_key_entry( triggerbot_key, _( "Trigger Bot" ) );
		if ( features::ragebot::active_config.main_switch && features::ragebot::active_config.safe_point )
			add_key_entry( safe_point_key, _( "Safe Point" ) );
		if ( features::ragebot::active_config.main_switch && features::ragebot::active_config.dt_enabled )
			add_key_entry( doubletap_key, _( "Double Tap (" ) + std::to_string( static_cast< int > ( features::ragebot::active_config.max_dt_ticks ) ) + _( " ticks)" ) );
		if ( features::antiaim::antiaiming )
			add_key_entry( slow_walk_key, _( "Slow Walk" ) );
		if ( fakeduck )
			add_key_entry( fakeduck_key, _( "Fake Duck" ) );
		if ( features::antiaim::antiaiming )
			add_key_entry( inverter_key, _( "Antiaim Inverter" ) );
		if ( blockbot )
			add_key_entry( blockbot_key, _( "Block Bot" ) );
		if ( third_person )
			add_key_entry( thirdperson_key, _( "Third-Person" ) );

		if ( features::antiaim::antiaiming ) {
			switch ( features::antiaim::side ) {
				case -1: break;
				case 0: entries.push_back( _( "[TOGGLE] Manual Antiaim (Back)" ) ); break;
				case 1: entries.push_back( _( "[TOGGLE] Manual Antiaim (Left)" ) ); break;
				case 2: entries.push_back( _( "[TOGGLE] Manual Antiaim (Right)" ) ); break;
			}
		}
	}

	if ( sesui::begin_window( _( "Keybinds" ), sesui::rect( 100, 100, 300, 75 ), keybind_list, sesui::window_flags::no_resize | sesui::window_flags::no_closebutton | ( opened ? sesui::window_flags::none : sesui::window_flags::no_move ) ) ) {
		sesui::style.slider_size.x = 300.0f - sesui::style.initial_offset.x * 2.0f;
		sesui::style.button_size.x = 300.0f - sesui::style.initial_offset.x * 2.0f;
		sesui::style.same_line_offset = 300.0f - sesui::style.initial_offset.x * 2.0f - sesui::style.inline_button_size.x;

		if ( !entries.empty( ) ) {
			for ( auto& entry : entries )
				sesui::text( entry.data( ) );
		}

		sesui::end_window( );

		sesui::globals::window_ctx [ _( "Keybinds" ) ].bounds.h = sesui::style.button_size.y * entries.size( ) + sesui::style.initial_offset.y * 4.0f;
	}
}