#include "menu.hpp"
#include "options.hpp"
#include "d3d9_render.hpp"
#include "sesui_custom.hpp"
#include "../utils/networking.hpp"

#include <filesystem>
#include <ShlObj.h>
#include <codecvt>

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
std::wstring g_username;

void gui::init ( ) {
    if ( g::loader_data && g::loader_data->username ) {
        wchar_t wstr [ 64 ];
        memset ( wstr, '\0', sizeof wstr );
        mbstowcs ( wstr, g::loader_data->username, strlen( g::loader_data->username ) );
        g_username = wstr;
    }
    else
        g_username = _ ( L"Developer" );

    if (!g::loader_data || !g::loader_data->avatar || !g::loader_data->avatar_sz)
        g_pfp_data = std::string(reinterpret_cast<const char*>(ses_pfp), sizeof(ses_pfp));//networking::get(_("sesame.one/data/avatars/s/0/1.jpg"));
    else
        g_pfp_data = std::string ( g::loader_data->avatar, g::loader_data->avatar + g::loader_data->avatar_sz );

    /* initialize cheat config */
    options::init ( );
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

    load_cfg_list ( );

    END_FUNC
}

std::wstring selected_config = _ ( L"default" );
std::vector< std::wstring > configs { };

void gui::load_cfg_list ( ) {
	wchar_t appdata [ MAX_PATH ];

	if ( SUCCEEDED ( LI_FN ( SHGetFolderPathW )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
		LI_FN ( CreateDirectoryW )( ( std::wstring ( appdata ) + _ ( L"\\sesame" ) ).data ( ), nullptr );
		LI_FN ( CreateDirectoryW )( ( std::wstring ( appdata ) + _ ( L"\\sesame\\configs" ) ).data ( ), nullptr );
	}

	auto sanitize_name = [ ] ( const std::wstring& dir ) {
		const auto dot = dir.find_last_of ( _ ( L"." ) );
		return dir.substr ( N ( 0 ), dot );
	};

	configs.clear ( );

	for ( const auto& dir : std::filesystem::recursive_directory_iterator ( std::wstring ( appdata ) + _ ( L"\\sesame\\configs" ) ) ) {
		if ( dir.exists ( ) && dir.is_regular_file ( ) && dir.path ( ).extension ( ).wstring ( ) == _ ( L".xml" ) ) {
			const auto sanitized = sanitize_name ( dir.path ( ).filename ( ).wstring ( ) );
			configs.push_back ( sanitized );
		}
	}
}

void gui::weapon_controls ( const std::string& weapon_name ) {
	const auto ragebot_weapon = _ ( "ragebot." ) + weapon_name + _(".");

	namespace gui = sesui::custom;

	if ( gui::begin_group ( L"Weapon Settings", sesui::rect ( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect ( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
		//sesui::tooltip ( _ ( L"Inherits settings from default weapon configuration" ) );
		sesui::checkbox ( _ ( L"Inherit Default" ), options::vars [ ragebot_weapon + _ ( "inherit_default" ) ].val.b );
		//sesui::tooltip ( _ ( L"Allows choking packets immediately after your shot" ) );
		sesui::checkbox ( _ ( L"Choke On Shot" ), options::vars [ ragebot_weapon + _ ( "choke_onshot" ) ].val.b );
		//sesui::tooltip ( _ ( L"Hide angles from being shown on screen (clientside only)" ) );
		sesui::checkbox ( _ ( L"Silent Aim" ), options::vars [ ragebot_weapon + _ ( "silent" ) ].val.b );
		//sesui::tooltip ( _ ( L"Automatically shoot when a target is found" ) );
		sesui::checkbox ( _ ( L"Auto Shoot" ), options::vars [ ragebot_weapon + _ ( "auto_shoot" ) ].val.b );
		//sesui::tooltip ( _ ( L"Automatically scope if hitchance is not high enough" ) );
		sesui::checkbox ( _ ( L"Auto Scope" ), options::vars [ ragebot_weapon + _ ( "auto_scope" ) ].val.b );
		//sesui::tooltip ( _ ( L"Automatically slow down if hitchance requirement is not met" ) );
		sesui::checkbox ( _ ( L"Auto Slow" ), options::vars [ ragebot_weapon + _ ( "auto_slow" ) ].val.b );
		////sesui::tooltip ( _ ( L"Allow doubletap to teleport your player" ) );
		//sesui::checkbox ( _ ( L"Doubletap Teleport" ), options::vars [ ragebot_weapon + _ ( "dt_teleport" ) ].val.b );
		//sesui::tooltip ( _ ( L"Enable doubletap on current weapon" ) );
		sesui::checkbox ( _ ( L"Doubletap" ), options::vars [ ragebot_weapon + _ ( "dt_enabled" ) ].val.b );		
		//sesui::tooltip ( _ ( L"Maximum amount of ticks to shift during doubletap" ) );
		sesui::slider ( _ ( L"Doubletap Ticks" ), options::vars [ ragebot_weapon + _ ( "dt_ticks" ) ].val.i, 0, 16, _ ( L"%d ticks" ) );
		//sesui::tooltip ( _ ( L"Minimum damage required for ragebot to target players" ) );
		sesui::slider ( _ ( L"Minimum Damage" ), options::vars [ ragebot_weapon + _ ( "min_dmg" ) ].val.f, 0.0f, 150.0f, ( options::vars [ ragebot_weapon + _ ( "min_dmg" ) ].val.f > 100.0f ? ( _ ( L"hp + " ) + std::to_wstring ( static_cast< int > ( options::vars [ ragebot_weapon + _ ( "min_dmg" ) ].val.f - 100.0f ) ) + _ ( L" dmg" ) ) : ( std::to_wstring ( static_cast< int > ( options::vars [ ragebot_weapon + _ ( "min_dmg" ) ].val.f ) ) + _ ( L" dmg" ) ) ).c_str ( ) );
		//sesui::tooltip ( _ ( L"Minimum hit chance required for ragebot to target players" ) );
		sesui::slider ( _ ( L"Hit Chance" ), options::vars [ ragebot_weapon + _ ( "hit_chance" ) ].val.f, 0.0f, 100.0f, _ ( L"%.1f%%" ) );
		//sesui::tooltip ( _ ( L"Minimum hit chance required for ragebot to target players while double tapping" ) );
		sesui::slider ( _ ( L"Doubletap Hit Chance" ), options::vars [ ragebot_weapon + _ ( "dt_hit_chance" ) ].val.f, 0.0f, 100.0f, _ ( L"%.1f%%" ) );
		

		gui::end_group ( );
	}

	if ( gui::begin_group ( L"Hitscan", sesui::rect ( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect ( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
		//sesui::tooltip ( _ ( L"Force body hitbox if body shot is lethal" ) );
		sesui::checkbox ( _ ( L"Bodyaim If Lethal" ), options::vars [ ragebot_weapon + _ ( "baim_if_lethal" ) ].val.b );
		//sesui::tooltip ( _ ( L"Force body hitbox if player is in air" ) );
		sesui::checkbox ( _ ( L"Bodyaim If In Air" ), options::vars [ ragebot_weapon + _ ( "baim_in_air" ) ].val.b );
		//sesui::tooltip ( _ ( L"Forces body hitbox after x misses" ) );
		sesui::slider ( _ ( L"Bodyaim After Misses" ), options::vars [ ragebot_weapon + _ ( "force_baim" ) ].val.i, 0, 5, _ ( L"%d misses" ) );
		//sesui::tooltip ( _ ( L"Overrides all hitboxes and body aim to head hitbox" ) );
		sesui::checkbox ( _ ( L"Headshot Only" ), options::vars [ ragebot_weapon + _ ( "headshot_only" ) ].val.b );
		//sesui::tooltip ( _ ( L"Distance from center of head to scan" ) );
		sesui::slider ( _ ( L"Head Point Scale" ), options::vars [ ragebot_weapon + _ ( "head_pointscale" ) ].val.f, 0.0f, 100.0f, _ ( L"%.1f%%" ) );
		//sesui::tooltip ( _ ( L"Distance from center of body to scan" ) );
		sesui::slider ( _ ( L"Body Point Scale" ), options::vars [ ragebot_weapon + _ ( "body_pointscale" ) ].val.f, 0.0f, 100.0f, _ ( L"%.1f%%" ) );
		//sesui::tooltip ( _ ( L"Hitboxes ragebot will attempt to target" ) );
		sesui::multiselect ( _ ( L"Hitboxes" ), {
			{ _ ( L"Head" ), options::vars [ ragebot_weapon + _ ( "hitboxes" ) ].val.l [ 0 ] },
			{ _ ( L"Neck" ), options::vars [ ragebot_weapon + _ ( "hitboxes" ) ].val.l [ 1 ] },
			{ _ ( L"Chest" ), options::vars [ ragebot_weapon + _ ( "hitboxes" ) ].val.l [ 2 ] },
			{ _ ( L"Pelvis" ), options::vars [ ragebot_weapon + _ ( "hitboxes" ) ].val.l [ 3 ] },
			{ _ ( L"Arms" ), options::vars [ ragebot_weapon + _ ( "hitboxes" ) ].val.l [ 4 ] },
			{ _ ( L"Legs" ), options::vars [ ragebot_weapon + _ ( "hitboxes" ) ].val.l [ 5 ] },
			{ _ ( L"Feet" ), options::vars [ ragebot_weapon + _ ( "hitboxes" ) ].val.l [ 6 ] }
			} );

		gui::end_group ( );
	}
}

void gui::antiaim_controls ( const std::string& antiaim_name ) {
	const auto antiaim_config = _ ( "antiaim." ) + antiaim_name + _ ( "." );

	namespace gui = sesui::custom;

	if ( gui::begin_group ( L"Antiaim", sesui::rect ( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect ( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
		sesui::checkbox ( _ ( L"Enable" ), options::vars [ antiaim_config + _ ( "enabled" ) ].val.b );
		sesui::slider ( _ ( L"Fakelag Factor" ), options::vars [ antiaim_config + _ ( "fakelag_factor" ) ].val.i, 0, 16, _ ( L"%d ticks" ) );
		sesui::combobox ( _ ( L"Base Pitch" ), options::vars [ antiaim_config + _ ( "pitch" ) ].val.i, { _ ( L"None" ), _ ( L"Down" ), _ ( L"Up" ), _ ( L"Zero" ) } );
		sesui::slider ( _ ( L"Yaw Offset" ), options::vars [ antiaim_config + _ ( "yaw_offset" ) ].val.f, -180.0f, 180.0f, _ ( L"%.1f°" ) );
		sesui::combobox ( _ ( L"Base Yaw" ), options::vars [ antiaim_config + _ ( "base_yaw" ) ].val.i, { _ ( L"Relative" ), _ ( L"Absolute" ), _ ( L"At Target" ), _ ( L"Auto Direction" ) } );
		sesui::slider ( _ ( L"Auto Direction Amount" ), options::vars [ antiaim_config + _ ( "auto_direction_amount" ) ].val.f, -180.0f, 180.0f, _ ( L"%.1f°" ) );
		sesui::slider ( _ ( L"Auto Direction Range" ), options::vars [ antiaim_config + _ ( "auto_direction_range" ) ].val.f, 0.0f, 100.0f, _ ( L"%.1f units" ) );
		sesui::slider ( _ ( L"Jitter Range" ), options::vars [ antiaim_config + _ ( "jitter_range" ) ].val.f, -180.0f, 180.0f, _ ( L"%.1f°" ) );
		sesui::slider ( _ ( L"Rotation Range" ), options::vars [ antiaim_config + _ ( "rotation_range" ) ].val.f, -180.0f, 180.0f, _ ( L"%.1f°" ) );
		sesui::slider ( _ ( L"Rotation Speed" ), options::vars [ antiaim_config + _ ( "rotation_speed" ) ].val.f, 0.0f, 2.0f, _ ( L"%.1f Hz" ) );

		gui::end_group ( );
	}

	if ( gui::begin_group ( L"Desync", sesui::rect ( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect ( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
		sesui::checkbox ( _ ( L"Enable Desync" ), options::vars [ antiaim_config + _ ( "desync" ) ].val.b );
		sesui::checkbox ( _ ( L"Invert Initial Side" ), options::vars [ antiaim_config + _ ( "invert_initial_side" ) ].val.b );
		sesui::checkbox ( _ ( L"Jitter Desync" ), options::vars [ antiaim_config + _ ( "jitter_desync_side" ) ].val.b );
		sesui::checkbox ( _ ( L"Center Real" ), options::vars [ antiaim_config + _ ( "center_real" ) ].val.b );
		sesui::checkbox ( _ ( L"Anti Bruteforce" ), options::vars [ antiaim_config + _ ( "anti_bruteforce" ) ].val.b );
		sesui::checkbox ( _ ( L"Anti Freestanding Prediction" ), options::vars [ antiaim_config + _ ( "anti_freestand_prediction" ) ].val.b );
		sesui::slider ( _ ( L"Desync Range" ), options::vars [ antiaim_config + _ ( "desync_range" ) ].val.f, 0.0f, 60.0f, _ ( L"%.1f°" ) );
		sesui::slider ( _ ( L"Desync Range Inverted" ), options::vars [ antiaim_config + _ ( "desync_range_inverted" ) ].val.f, 0.0f, 60.0f, _ ( L"%.1f°" ) );

		gui::end_group ( );
	}
}

void gui::player_visuals_controls ( const std::string& visual_name ) {
	const auto visuals_config = _ ( "visuals." ) + visual_name + _ ( "." );

	namespace gui = sesui::custom;

	if ( gui::begin_group ( L"Player Visuals", sesui::rect ( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect ( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
		sesui::multiselect ( _ ( L"Filters" ), {
						{ _ ( L"Local" ), options::vars [ _ ( "visuals.filters" ) ].val.l [ 0 ]},
						{_ ( L"Teammates" ), options::vars [ _ ( "visuals.filters" ) ].val.l [ 1 ]},
						{_ ( L"Enemies" ), options::vars [ _ ( "visuals.filters" ) ].val.l [ 2 ]},
						{_ ( L"Weapons" ), options::vars [ _ ( "visuals.filters" ) ].val.l [ 3 ]},
						{_ ( L"Grenades" ), options::vars [ _ ( "visuals.filters" ) ].val.l [ 4 ]},
						{_ ( L"Bomb" ), options::vars [ _ ( "visuals.filters" ) ].val.l [ 5 ]}
			} );
		
		if ( visual_name == _ ( "local" ) ) {
			sesui::multiselect ( _ ( L"Options" ), {
						{ _ ( L"Chams" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 0 ]},
						{_ ( L"Flat Chams" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 1 ]},
						{_ ( L"XQZ Chams" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 2 ]},
						{ _ ( L"Desync Chams" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 3 ]},
						{_ ( L"Desync Chams Fakelag" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 4 ]},
						{_ ( L"Desync Chams Rimlight" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 5 ]},
						{_ ( L"Glow" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 6 ]},
						{_ ( L"Rimlight Overlay" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 7 ]},
						{_ ( L"ESP Box" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 8 ]},
						{_ ( L"Health Bar" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 9 ]},
						{_ ( L"Ammo Bar" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 10 ]},
						{_ ( L"Desync Bar" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 11 ]},
						{_ ( L"Value Text" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 12 ]},
						{_ ( L"Name Tag" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 13 ]},
						{_ ( L"Weapon Name" ), options::vars [ _ ( "visuals.local.options" ) ].val.l [ 14 ]}
				} );
		}
		else if ( visual_name == _ ( "enemies" ) ) {
			sesui::multiselect ( _ ( L"Options" ), {
						{ _ ( L"Chams" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 0 ]},
						{_ ( L"Flat Chams" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 1 ]},
						{_ ( L"XQZ Chams" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 2 ]},
						{ _ ( L"Backtrack Chams" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 3 ]},
						{_ ( L"Hit Matrix" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 4 ]},
						{_ ( L"Glow" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 5 ]},
						{_ ( L"Rimlight Overlay" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 6 ]},
						{_ ( L"ESP Box" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 7 ]},
						{_ ( L"Health Bar" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 8 ]},
						{_ ( L"Ammo Bar" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 9 ]},
						{_ ( L"Desync Bar" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 10 ]},
						{_ ( L"Value Text" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 11 ]},
						{_ ( L"Name Tag" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 12 ]},
						{_ ( L"Weapon Name" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l [ 13 ]}
				} );
		}
		else if ( visual_name == _ ( "teammates" ) ) {
			sesui::multiselect ( _ ( L"Options" ), {
						{ _ ( L"Chams" ), options::vars [ _ ( "visuals.teammates.options" ) ].val.l [ 0 ]},
						{_ ( L"Flat Chams" ), options::vars [ _ ( "visuals.teammates.options" ) ].val.l [ 1 ]},
						{_ ( L"XQZ Chams" ), options::vars [ _ ( "visuals.teammates.options" ) ].val.l [ 2 ]},
						{_ ( L"Glow" ), options::vars [ _ ( "visuals.teammates.options" ) ].val.l [ 3 ]},
						{_ ( L"Rimlight Overlay" ), options::vars [ _ ( "visuals.teammates.options" ) ].val.l [ 4 ]},
						{_ ( L"ESP Box" ), options::vars [ _ ( "visuals.teammates.options" ) ].val.l [ 5 ]},
						{_ ( L"Health Bar" ), options::vars [ _ ( "visuals.teammates.options" ) ].val.l [ 6 ]},
						{_ ( L"Ammo Bar" ), options::vars [ _ ( "visuals.teammates.options" ) ].val.l [ 7 ]},
						{_ ( L"Desync Bar" ), options::vars [ _ ( "visuals.teammates.options" ) ].val.l [ 8 ]},
						{_ ( L"Value Text" ), options::vars [ _ ( "visuals.teammates.options" ) ].val.l [ 9 ]},
						{_ ( L"Name Tag" ), options::vars [ _ ( "visuals.teammates.options" ) ].val.l [ 10 ]},
						{_ ( L"Weapon Name" ), options::vars [ _ ( "visuals.teammates.options" ) ].val.l [ 11 ]}
				} );
		}

		sesui::combobox ( _ ( L"Health Bar Location" ), options::vars [ visuals_config + _ ( "health_bar_location" ) ].val.i, { _ ( L"Left" ), _ ( L"Right" ), _ ( L"Bottom" ), _ ( L"Top" ) } );
		sesui::combobox ( _ ( L"Ammo Bar Location" ), options::vars [ visuals_config + _ ( "ammo_bar_location" ) ].val.i, { _ ( L"Left" ), _ ( L"Right" ), _ ( L"Bottom" ), _ ( L"Top" ) } );
		sesui::combobox ( _ ( L"Desync Bar Location" ), options::vars [ visuals_config + _ ( "desync_bar_location" ) ].val.i, { _ ( L"Left" ), _ ( L"Right" ), _ ( L"Bottom" ), _ ( L"Top" ) } );
		sesui::combobox ( _ ( L"Value Text Location" ), options::vars [ visuals_config + _ ( "value_text_location" ) ].val.i, { _ ( L"Left" ), _ ( L"Right" ), _ ( L"Bottom" ), _ ( L"Top" ) } );
		sesui::combobox ( _ ( L"Name Tag Location" ), options::vars [ visuals_config + _ ( "nametag_location" ) ].val.i, { _ ( L"Left" ), _ ( L"Right" ), _ ( L"Bottom" ), _ ( L"Top" ) } );
		sesui::combobox ( _ ( L"Weapon Name Location" ), options::vars [ visuals_config + _ ( "weapon_name_location" ) ].val.i, { _ ( L"Left" ), _ ( L"Right" ), _ ( L"Bottom" ), _ ( L"Top" ) } );
		sesui::slider ( _ ( L"Chams Reflectivity" ), options::vars [ visuals_config + _ ( "reflectivity" ) ].val.f, 0.0f, 100.0f, _ ( L"%.1f%%" ) );
		sesui::slider ( _ ( L"Chams Phong" ), options::vars [ visuals_config + _ ( "phong" ) ].val.f, 0.0f, 100.0f, _ ( L"%.1f%%" ) );


		gui::end_group ( );
	}

	if ( gui::begin_group ( L"Colors", sesui::rect ( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect ( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
		sesui::colorpicker ( _ ( L"Chams Color" ), options::vars [ visuals_config + _ ( "chams_color" ) ].val.c );
		sesui::colorpicker ( _ ( L"XQZ Chams Color" ), options::vars [ visuals_config + _ ( "xqz_chams_color" ) ].val.c );
		
		if ( visual_name == _ ( "enemies" ) )
			sesui::colorpicker ( _ ( L"Backtrack Chams Color" ), options::vars [ visuals_config + _ ( "backtrack_chams_color" ) ].val.c );
		
		if ( visual_name == _ ( "local" ) ) {
			sesui::colorpicker ( _ ( L"Desync Chams Color" ), options::vars [ _ ( "visuals.local.desync_chams_color" ) ].val.c );
			sesui::colorpicker ( _ ( L"Desync Chams Rimlight Color" ), options::vars [ _ ( "visuals.local.desync_rimlight_overlay_color" ) ].val.c );
		}

		if ( visual_name == _ ( "enemies" ) )
			sesui::colorpicker ( _ ( L"Hit Matrix Color" ), options::vars [ visuals_config + _ ( "hit_matrix_color" ) ].val.c );
		
		sesui::colorpicker ( _ ( L"Glow Color" ), options::vars [ visuals_config + _ ( "glow_color" ) ].val.c );
		sesui::colorpicker ( _ ( L"Rimlight Overlay Color" ), options::vars [ visuals_config + _ ( "rimlight_overlay_color" ) ].val.c );
		sesui::colorpicker ( _ ( L"ESP Box Color" ), options::vars [ visuals_config + _ ( "box_color" ) ].val.c );
		sesui::colorpicker ( _ ( L"Health Bar Color" ), options::vars [ visuals_config + _ ( "health_bar_color" ) ].val.c );
		sesui::colorpicker ( _ ( L"Ammo Bar Color" ), options::vars [ visuals_config + _ ( "ammo_bar_color" ) ].val.c );
		sesui::colorpicker ( _ ( L"Desync Bar Color" ), options::vars [ visuals_config + _ ( "desync_bar_color" ) ].val.c );
		sesui::colorpicker ( _ ( L"Name Tag Color" ), options::vars [ visuals_config + _ ( "name_color" ) ].val.c );
		sesui::colorpicker ( _ ( L"Weapon Name Color" ), options::vars [ visuals_config + _ ( "weapon_color" ) ].val.c );

		gui::end_group ( );
	}
}

void gui::draw ( ) {
	sesui::globals::dpi = options::vars [ _ ( "gui.dpi" ) ].val.f;

	if ( !utils::key_state ( VK_INSERT ) && open_button_pressed )
		opened = !opened;

	open_button_pressed = utils::key_state ( VK_INSERT );

	if ( !opened )
		return;

	sesui::begin_frame ( _(L"Counter-Strike: Global Offensive") );

	namespace gui = sesui::custom;

	if ( gui::begin_window ( _(L"Sesame"), sesui::rect ( 200, 200, 800, 600 ), opened, sesui::window_flags::no_closebutton ) ) {
		if ( gui::begin_tabs ( tab_max ) ) {
			gui::tab (_(L"A"),_( L"Legit"), current_tab_idx );
			gui::tab (_(L"B"),_( L"Rage"), current_tab_idx );
			gui::tab (_(L"C"),_( L"Antiaim"), current_tab_idx );
			gui::tab (_(L"D"),_( L"Visuals"), current_tab_idx );
			gui::tab (_(L"E"),_( L"Skins"), current_tab_idx );
			gui::tab (_(L"F"),_( L"Misc"), current_tab_idx );

			gui::end_tabs ( );
		}

		switch ( current_tab_idx ) {
		case tab_legit: {
			if ( gui::begin_subtabs ( legit_subtabs_max ) ) {
				gui::subtab ( _ ( L"General" ), _ ( L"General legitbot settings" ), sesui::rect ( 0.0f, 0.0f, 0.5f, 0.2f ), sesui::rect ( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
				gui::subtab ( _ ( L"Default" ), _ ( L"Default settings used for unconfigured weapons" ), sesui::rect ( 0.5f, 0.0f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
				gui::subtab ( _ ( L"Pistol" ), _ ( L"Pistol class cofiguration" ), sesui::rect ( 0.0f, 0.2f, 0.5f, 0.2f ), sesui::rect ( 0.0f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
				gui::subtab ( _ ( L"Revolver" ), _ ( L"Revolver class cofiguration" ), sesui::rect ( 0.5f, 0.2f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
				gui::subtab ( _ ( L"Rifle" ), _ ( L"Rifle, SMG, and shotgun class cofigurations" ), sesui::rect ( 0.0f, 0.4f, 0.5f, 0.2f ), sesui::rect ( 0.0f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
				gui::subtab ( _ ( L"AWP" ), _ ( L"AWP class cofiguration" ), sesui::rect ( 0.5f, 0.4f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
				gui::subtab ( _ ( L"Auto" ), _ ( L"Autosniper class cofiguration" ), sesui::rect ( 0.0f, 0.6f, 0.5f, 0.2f ), sesui::rect ( 0.0f, sesui::style.spacing * 0.5f * 3.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );
				gui::subtab ( _ ( L"Scout" ), _ ( L"Scout class cofiguration" ), sesui::rect ( 0.5f, 0.6f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f * 3.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), legit_subtab_idx );

				gui::end_subtabs ( );
			}

			switch ( legit_subtab_idx ) {
			case legit_subtabs_main: {
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
			if ( gui::begin_subtabs ( rage_subtabs_max ) ) {
				gui::subtab ( _ ( L"General" ), _ ( L"General ragebot and accuracy settings" ), sesui::rect ( 0.0f, 0.0f, 0.5f, 0.2f ), sesui::rect ( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
				gui::subtab ( _ ( L"Default" ), _ ( L"Default settings used for unconfigured weapons" ), sesui::rect ( 0.5f, 0.0f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
				gui::subtab ( _ ( L"Pistol" ), _ ( L"Pistol class cofiguration" ), sesui::rect ( 0.0f, 0.2f, 0.5f, 0.2f ), sesui::rect ( 0.0f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
				gui::subtab ( _ ( L"Revolver" ), _ ( L"Revolver class cofiguration" ), sesui::rect ( 0.5f, 0.2f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
				gui::subtab ( _ ( L"Rifle" ), _ ( L"Rifle, SMG, and shotgun class cofigurations" ), sesui::rect ( 0.0f, 0.4f, 0.5f, 0.2f ), sesui::rect ( 0.0f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
				gui::subtab ( _ ( L"AWP" ), _ ( L"AWP class cofiguration" ), sesui::rect ( 0.5f, 0.4f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
				gui::subtab ( _ ( L"Auto" ), _ ( L"Autosniper class cofiguration" ), sesui::rect ( 0.0f, 0.6f, 0.5f, 0.2f ), sesui::rect ( 0.0f, sesui::style.spacing * 0.5f * 3.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );
				gui::subtab ( _ ( L"Scout" ), _ ( L"Scout class cofiguration" ), sesui::rect ( 0.5f, 0.6f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f * 3.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), rage_subtab_idx );

				gui::end_subtabs ( );
			}

			switch ( rage_subtab_idx ) {
			case rage_subtabs_main: {
				if ( gui::begin_group ( L"General Settings", sesui::rect ( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect ( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
					//sesui::tooltip ( _ ( L"Enabled or disables all ragebot settings" ) );
					sesui::checkbox ( _ ( L"Main Switch" ), options::vars [ _ ( "ragebot.main_switch" ) ].val.b );
					//sesui::tooltip ( _ ( L"Automatically knife players" ) );
					sesui::checkbox ( _ ( L"Knife Bot" ), options::vars [ _ ( "ragebot.knife_bot" ) ].val.b );
					//sesui::tooltip ( _ ( L"Automatically zeus players" ) );
					sesui::checkbox ( _ ( L"Zeus Bot" ), options::vars [ _ ( "ragebot.zeus_bot" ) ].val.b );

					gui::end_group ( );
				}

				if ( gui::begin_group ( L"Accuracy", sesui::rect ( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect ( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
					//sesui::tooltip ( _ ( L"Correct and predict fakelag" ) );
					sesui::checkbox ( _ ( L"Fix Fakelag" ), options::vars [ _ ( "ragebot.fix_fakelag" ) ].val.b );
					//sesui::tooltip ( _ ( L"Resolve animation desync" ) );
					sesui::checkbox ( _ ( L"Resolve Desync" ), options::vars [ _ ( "ragebot.resolve_desync" ) ].val.b );
					//sesui::tooltip ( _ ( L"Attempts to aim at points that align with all resolved matricies" ) );
					sesui::checkbox ( _ ( L"Safe Point" ), options::vars [ _ ( "ragebot.safe_point" ) ].val.b );
					sesui::same_line ( );
					//sesui::tooltip ( _ ( L"Safe point override keybind" ) );
					sesui::keybind ( _ ( L"Safe Point Key" ), options::vars [ _ ( "ragebot.safe_point_key" ) ].val.i, options::vars [ _ ( "ragebot.safe_point_key_mode" ) ].val.i );
					//sesui::tooltip ( _ ( L"Automatically cock revolver until fully cocked" ) );
					sesui::checkbox ( _ ( L"Auto Revolver" ), options::vars [ _ ( "ragebot.auto_revolver" ) ].val.b );
					//sesui::tooltip ( _ ( L"Doubletap override key" ) );
					sesui::keybind ( _ ( L"Doubletap Key" ), options::vars [ _ ( "ragebot.dt_key" ) ].val.i, options::vars [ _ ( "ragebot.dt_key_mode" ) ].val.i );

					gui::end_group ( );
				}
			} break;
			case rage_subtabs_default: {
				weapon_controls ( _ ( "default" ) );
			} break;
			case rage_subtabs_pistol: {
				weapon_controls ( _ ( "pistol" ) );
			} break;
			case rage_subtabs_revolver: {
				weapon_controls ( _ ( "revolver" ) );
			} break;
			case rage_subtabs_rifle: {
				weapon_controls ( _ ( "rifle" ) );
			} break;
			case rage_subtabs_awp: {
				weapon_controls ( _ ( "awp" ) );
			} break;
			case rage_subtabs_auto: {
				weapon_controls ( _ ( "auto" ) );
			} break;
			case rage_subtabs_scout: {
				weapon_controls ( _ ( "scout" ) );
			} break;
			default: {
			} break;
			}
		} break;
		case tab_antiaim: {
			if ( gui::begin_subtabs ( antiaim_subtabs_max ) ) {
				gui::subtab ( _ ( L"Air" ), _ ( L"In air antiaim settings" ), sesui::rect ( 0.0f, 0.0f, 0.5f, 0.2f ), sesui::rect ( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), antiaim_subtab_idx );
				gui::subtab ( _ ( L"Moving" ), _ ( L"Moving antiaim settings" ), sesui::rect ( 0.5f, 0.0f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), antiaim_subtab_idx );
				gui::subtab ( _ ( L"Slow Walk" ), _ ( L"Slow walk antiaim settings" ), sesui::rect ( 0.0f, 0.2f, 0.5f, 0.2f ), sesui::rect ( 0.0f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), antiaim_subtab_idx );
				gui::subtab ( _ ( L"Standing" ), _ ( L"Standing antiaim settings" ), sesui::rect ( 0.5f, 0.2f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), antiaim_subtab_idx );
				gui::subtab ( _ ( L"Other" ), _ ( L"Fakelag, manual aa, and other antiaim features" ), sesui::rect ( 0.0f, 0.4f, 0.5f, 0.2f ), sesui::rect ( 0.0f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), antiaim_subtab_idx );
				
				gui::end_subtabs ( );
			}

			switch ( antiaim_subtab_idx ) {
			case antiaim_subtabs_air: {
				antiaim_controls ( _ ( "air" ) );
			} break;
			case antiaim_subtabs_moving: {
				antiaim_controls ( _ ( "moving" ) );
			} break;
			case antiaim_subtabs_slow_walk: {
				antiaim_controls ( _ ( "slow_walk" ) );
			} break;
			case antiaim_subtabs_standing: {
				antiaim_controls ( _ ( "standing" ) );
			} break;
			case antiaim_subtabs_other: {
				if ( gui::begin_group ( L"General", sesui::rect ( 0.0f, 0.0f, 1.0f, 1.0f ), sesui::rect ( 0.0f, 0.0f, 0.0f, 0.0f ) ) ) {			
					sesui::slider ( _ ( L"Slow Walk Speed" ), options::vars [ _ ( "antiaim.slow_walk_speed" ) ].val.f, 0.0f, 100.0f, _ ( L"%.1f%%" ) );
					sesui::same_line ( );
					sesui::keybind ( _ ( L"Slow Walk Key" ), options::vars [ _ ( "antiaim.slow_walk_key" ) ].val.i, options::vars [ _ ( "antiaim.slow_walk_key_mode" ) ].val.i );
					sesui::checkbox ( _ ( L"Fake Walk" ), options::vars [ _ ( "antiaim.fakewalk" ) ].val.b );
					sesui::checkbox ( _ ( L"Fake Duck" ), options::vars [ _ ( "antiaim.fakeduck" ) ].val.b );
					sesui::same_line ( );
					sesui::keybind ( _ ( L"Fake Duck Key" ), options::vars [ _ ( "antiaim.fakeduck_key" ) ].val.i, options::vars [ _ ( "antiaim.fakeduck_key_mode" ) ].val.i );
					sesui::combobox ( _ ( L"Fake Duck Mode" ), options::vars [ _ ( "antiaim.fakeduck_mode" ) ].val.i, { _ ( L"Normal" ), _ ( L"Full" ) } );
					sesui::keybind ( _ ( L"Manual Left Key" ), options::vars [ _ ( "antiaim.manual_left_key" ) ].val.i, options::vars [ _ ( "antiaim.manual_left_key_mode" ) ].val.i );
					sesui::keybind ( _ ( L"Manual Right Key" ), options::vars [ _ ( "antiaim.manual_right_key" ) ].val.i, options::vars [ _ ( "antiaim.manual_right_key_mode" ) ].val.i );
					sesui::keybind ( _ ( L"Manual Back Key" ), options::vars [ _ ( "antiaim.manual_back_key" ) ].val.i, options::vars [ _ ( "antiaim.manual_back_key_mode" ) ].val.i );
					sesui::keybind ( _ ( L"Desync Inverter Key" ), options::vars [ _ ( "antiaim.desync_invert_key" ) ].val.i, options::vars [ _ ( "antiaim.desync_invert_key_mode" ) ].val.i );

					gui::end_group ( );
				}
			} break;
			default: {
			} break;
			}
		} break;
		case tab_visuals: {
			if ( gui::begin_subtabs ( visuals_subtabs_max ) ) {
				gui::subtab ( _ ( L"Local Player" ), _ ( L"Visuals used on local player" ), sesui::rect ( 0.0f, 0.0f, 0.5f, 0.2f ), sesui::rect ( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), visuals_subtab_idx );
				gui::subtab ( _ ( L"Enemies" ), _ ( L"Visuals used on filtered enemies" ), sesui::rect ( 0.5f, 0.0f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), visuals_subtab_idx );
				gui::subtab ( _ ( L"Teammates" ), _ ( L"Visuals used on filtered teammates" ), sesui::rect ( 0.0f, 0.2f, 0.5f, 0.2f ), sesui::rect ( 0.0f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), visuals_subtab_idx );
				gui::subtab ( _ ( L"Other" ), _ ( L"Other visual options" ), sesui::rect ( 0.5f, 0.2f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), visuals_subtab_idx );
				
				gui::end_subtabs ( );
			}

			switch ( visuals_subtab_idx ) {
			case visuals_subtabs_local: {
				player_visuals_controls ( _ ( "local" ) );
			} break;
			case visuals_subtabs_enemies: {
				player_visuals_controls ( _ ( "enemies" ) );
			} break;
			case visuals_subtabs_teammates: {
				player_visuals_controls ( _ ( "teammates" ) );
			} break;
			case visuals_subtabs_other: {
				if ( gui::begin_group ( L"World", sesui::rect ( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect ( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
					sesui::checkbox ( _ ( L"Bullet Tracers" ), options::vars [ _ ( "misc.effects.bullet_tracers" ) ].val.b );
					sesui::same_line ( );
					sesui::colorpicker ( _ ( L"Bullet Tracer Color" ), options::vars [ _ ( "visuals.other.bullet_tracer_color" ) ].val.c );
					sesui::checkbox ( _ ( L"Bullet Impacts" ), options::vars [ _ ( "visuals.other.bullet_impacts" ) ].val.b );
					sesui::same_line ( );
					sesui::colorpicker ( _ ( L"Bullet Impacts Color" ), options::vars [ _ ( "visuals.other.bullet_impact_color" ) ].val.c );
					sesui::checkbox ( _ ( L"Grenade Trajectories" ), options::vars [ _ ( "visuals.other.grenade_trajectories" ) ].val.b );
					sesui::same_line ( );
					sesui::colorpicker ( _ ( L"Grenade Trajectories Color" ), options::vars [ _ ( "visuals.other.grenade_trajectory_color" ) ].val.c );
					sesui::checkbox ( _ ( L"Grenade Bounces" ), options::vars [ _ ( "visuals.other.grenade_bounces" ) ].val.b );
					sesui::same_line ( );
					sesui::colorpicker ( _ ( L"Grenade Bounces Color" ), options::vars [ _ ( "visuals.other.grenade_bounce_color" ) ].val.c );
					sesui::checkbox ( _ ( L"Grenade Blast Radii" ), options::vars [ _ ( "visuals.other.grenade_blast_radii" ) ].val.b );
					sesui::same_line ( );
					sesui::colorpicker ( _ ( L"Grenade Blast Radii Color" ), options::vars [ _ ( "visuals.other.grenade_radii_color" ) ].val.c );

					gui::end_group ( );
				}

				if ( gui::begin_group ( L"Other", sesui::rect ( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect ( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
					sesui::multiselect ( _ ( L"Removals" ), {
						{ _ ( L"Smoke" ), options::vars [ _ ( "visuals.other.removals" ) ].val.l [ 0 ]},
						{_ ( L"Flash" ), options::vars [ _ ( "visuals.other.removals" ) ].val.l [ 1 ]},
						{_ ( L"Scope" ), options::vars [ _ ( "visuals.other.removals" ) ].val.l [ 2 ]},
						{ _ ( L"Aim Punch" ), options::vars [ _ ( "visuals.other.removals" ) ].val.l [ 3 ]},
						{_ ( L"View Punch" ), options::vars [ _ ( "visuals.other.removals" ) ].val.l [ 4 ]},
						{_ ( L"Zoom" ), options::vars [ _ ( "visuals.other.removals" ) ].val.l [ 5 ]},
						{_ ( L"Occlusion" ), options::vars [ _ ( "visuals.other.removals" ) ].val.l [ 6 ]},
						{_ ( L"Killfeed Decay" ), options::vars [ _ ( "visuals.other.removals" ) ].val.l [ 7 ]},
						{_ ( L"Post Processing" ), options::vars [ _ ( "visuals.other.removals" ) ].val.l [ 8 ]}
					} );
					sesui::slider ( _ ( L"FOV" ), options::vars [ _ ( "visuals.other.fov" ) ].val.f, 0.0f, 180.0f, _ ( L"%.1f°" ) );
					sesui::slider ( _ ( L"Viewmodel FOV" ), options::vars [ _ ( "visuals.other.viewmodel_fov" ) ].val.f, 0.0f, 180.0f, _ ( L"%.1f°" ) );
					sesui::slider ( _ ( L"Aspect Ratio" ), options::vars [ _ ( "visuals.other.aspect_ratio" ) ].val.f, 0.1f, 2.0f );

					sesui::multiselect ( _ ( L"Logs" ), {
						{ _ ( L"Hits" ), options::vars [ _ ( "visuals.other.logs" ) ].val.l [ 0 ]},
						{_ ( L"Spread Misses" ), options::vars [ _ ( "visuals.other.logs" ) ].val.l [ 1 ]},
						{_ ( L"Resolver Misses" ), options::vars [ _ ( "visuals.other.logs" ) ].val.l [ 2 ]},
						{ _ ( L"Wrong Hitbox" ), options::vars [ _ ( "visuals.other.logs" ) ].val.l [ 3 ]},
						{_ ( L"Manual Shots" ), options::vars [ _ ( "visuals.other.logs" ) ].val.l [ 4 ]}
						} );

					sesui::checkbox ( _ ( L"Spread Circle" ), options::vars [ _ ( "visuals.other.spread_circle" ) ].val.b );
					sesui::same_line ( );
					sesui::colorpicker ( _ ( L"Spread Circle Color" ), options::vars [ _ ( "visuals.other.spread_circle_color" ) ].val.c );
					sesui::checkbox ( _ ( L"Gradient Spread Circle" ), options::vars [ _ ( "visuals.other.gradient_spread_circle" ) ].val.b );
					sesui::checkbox ( _ ( L"Offscreen ESP" ), options::vars [ _ ( "visuals.other.offscreen_esp" ) ].val.b );
					sesui::same_line ( );
					sesui::colorpicker ( _ ( L"Offscreen ESP Color" ), options::vars [ _ ( "visuals.other.offscreen_esp_color" ) ].val.c );
					sesui::slider ( _ ( L"Offscreen ESP Distance" ), options::vars [ _ ( "visuals.other.offscreen_esp_distance" ) ].val.f, 0.0f, 100.0f, _ ( L"%.1f%%" ) );
					sesui::slider ( _ ( L"Offscreen ESP Size" ), options::vars [ _ ( "visuals.other.offscreen_esp_size" ) ].val.f, 0.0f, 100.0f, _ ( L"%.1f px" ) );

					sesui::checkbox ( _ ( L"Watermark" ), options::vars [ _ ( "visuals.other.watermark" ) ].val.b );
					sesui::checkbox ( _ ( L"Keybind List" ), options::vars [ _ ( "visuals.other.keybind_list" ) ].val.b );
					
					sesui::colorpicker ( _ ( L"World Color" ), options::vars [ _ ( "visuals.other.world_color" ) ].val.c );
					sesui::colorpicker ( _ ( L"Prop Color" ), options::vars [ _ ( "visuals.other.prop_color" ) ].val.c );
					sesui::colorpicker ( _ ( L"Accent Color" ), options::vars [ _ ( "visuals.other.accent_color" ) ].val.c );
					sesui::colorpicker ( _ ( L"Secondary Accent Color" ), options::vars [ _ ( "visuals.other.secondary_accent_color" ) ].val.c );
					sesui::colorpicker ( _ ( L"Logo Color" ), options::vars [ _ ( "visuals.other.logo_color" ) ].val.c );
					sesui::combobox ( _ ( L"Hit Sound" ), options::vars [ _ ( "visuals.other.hit_sound" ) ].val.i, { _ ( L"None" ), _ ( L"Arena Switch" ), _ ( L"Fall Pain" ), _ ( L"Bolt" ), _ ( L"Neck Snap" ), _ ( L"Power Switch" ), _ ( L"Glass" ), _ ( L"Bell" ), _ ( L"COD" ), _ ( L"Rattle" ), _ ( L"Sesame" ) } );

					gui::end_group ( );
				}
			} break;
			default: {
			} break;
			}
		} break;
		case tab_skins: {
			if ( gui::begin_subtabs ( skins_subtabs_max ) ) {
				gui::subtab ( _ ( L"Skin Changer" ), _ ( L"Apply custom skins to your weapons" ), sesui::rect ( 0.0f, 0.0f, 0.5f, 0.2f ), sesui::rect ( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), skins_subtab_idx );
				gui::subtab ( _ ( L"Model Changer" ), _ ( L"Replace game models with your own" ), sesui::rect ( 0.5f, 0.0f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), skins_subtab_idx );
				
				gui::end_subtabs ( );
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
			if ( gui::begin_subtabs ( misc_subtabs_max ) ) {
				gui::subtab ( _ ( L"Movement" ), _ ( L"Movement related cheats" ), sesui::rect ( 0.0f, 0.0f, 0.5f, 0.2f ), sesui::rect ( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), misc_subtab_idx );
				gui::subtab ( _ ( L"Effects" ), _ ( L"Miscellaneous visual effects" ), sesui::rect ( 0.5f, 0.0f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), misc_subtab_idx );
				gui::subtab ( _ ( L"Player List" ), _ ( L"Whitelist, clantag stealer, and bodyaim priority" ), sesui::rect ( 0.0f, 0.2f, 0.5f, 0.2f ), sesui::rect ( 0.0f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), misc_subtab_idx );
				gui::subtab ( _ ( L"Cheat" ), _ ( L"Cheat settings and panic button" ), sesui::rect ( 0.5f, 0.2f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), misc_subtab_idx );
				gui::subtab ( _ ( L"Configuration" ), _ ( L"Cheat configuration manager" ), sesui::rect ( 0.0f, 0.4f, 0.5f, 0.2f ), sesui::rect ( 0.0f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), misc_subtab_idx );
				gui::subtab ( _ ( L"Scripts" ), _ ( L"Script manager" ), sesui::rect ( 0.5f, 0.4f, 0.5f, 0.2f ), sesui::rect ( sesui::style.spacing * 0.5f, sesui::style.spacing * 0.5f * 2.0f, -sesui::style.spacing * 0.5f, -sesui::style.spacing * 0.5f ), misc_subtab_idx );
				
				gui::end_subtabs ( );
			}

			switch ( misc_subtab_idx ) {
			case misc_subtabs_movement: {
				if ( gui::begin_group ( L"General", sesui::rect ( 0.0f, 0.0f, 1.0f, 1.0f ), sesui::rect ( 0.0f, 0.0f, 0.0f, 0.0f ) ) ) {
					sesui::checkbox ( _ ( L"Auto Jump" ), options::vars [ _ ( "misc.movement.bhop" ) ].val.b );
					sesui::checkbox ( _ ( L"Auto Forward" ), options::vars [ _ ( "misc.movement.auto_forward" ) ].val.b );
					sesui::checkbox ( _ ( L"Auto Strafer" ), options::vars [ _ ( "misc.movement.auto_strafer" ) ].val.b );
					sesui::checkbox ( _ ( L"Directional Auto Strafer" ), options::vars [ _ ( "misc.movement.omnidirectional_auto_strafer" ) ].val.b );
					
					gui::end_group ( );
				}
			} break;
			case misc_subtabs_effects: {
				if ( gui::begin_group ( L"General", sesui::rect ( 0.0f, 0.0f, 1.0f, 1.0f ), sesui::rect ( 0.0f, 0.0f, 0.0f, 0.0f ) ) ) {
					sesui::checkbox ( _ ( L"Third Person" ), options::vars [ _ ( "misc.effects.third_person" ) ].val.b );
					sesui::same_line ( );
					sesui::keybind ( _ ( L"Third Person Key" ), options::vars [ _ ( "misc.effects.third_person_key" ) ].val.i, options::vars [ _ ( "misc.effects.third_person_key_mode" ) ].val.i );
					sesui::slider ( _ ( L"Third Person Range" ), options::vars [ _ ( "misc.effects.third_person_range" ) ].val.f, 0.0f, 500.0f, _ ( L"%.1f units" ) );
					sesui::slider ( _ ( L"Ragdoll Force Scale" ), options::vars [ _ ( "misc.effects.ragdoll_force_scale" ) ].val.f, 0.0f, 10.0f, _ ( L"x%.1f" ) );
					sesui::checkbox ( _ ( L"Clan Tag" ), options::vars [ _ ( "misc.effects.clantag" ) ].val.b );
					sesui::combobox ( _ ( L"Clan Tag Animation" ), options::vars [ _ ( "misc.effects.clantag_animation" ) ].val.i, { _ ( L"Static" ), _ ( L"Marquee" ), _ ( L"Capitalize" ), _ ( L"Heart" ) } );
					
					std::wstring clantag_text = options::vars [ _ ( "misc.effects.clantag_text" ) ].val.s;
					sesui::textbox ( _(L"Clan Tag Text"), clantag_text );
					wcscpy_s ( options::vars [ _ ( "misc.effects.clantag_text" ) ].val.s, clantag_text.data( ) );

					sesui::slider ( _ ( L"Revolver Cock Volume" ), options::vars [ _ ( "misc.effects.revolver_cock_volume" ) ].val.f, 0.0f, 1.0f, _ ( L"x%.1f" ) );
					sesui::slider ( _ ( L"Weapon Volume" ), options::vars [ _ ( "misc.effects.weapon_volume" ) ].val.f, 0.0f, 1.0f, _ ( L"x%.1f" ) );

					gui::end_group ( );
				}
			} break;
			case misc_subtabs_plist: {
			} break;
			case misc_subtabs_cheat: {
				if ( gui::begin_group ( L"Menu", sesui::rect ( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect ( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {			
					//sesui::slider ( _ ( L"GUI DPI" ), options::vars [ _ ( "gui.dpi" ) ].val.f, 1.0f, 3.0f );

					gui::end_group ( );
				}

				if ( gui::begin_group ( L"Cheat", sesui::rect ( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect ( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
					gui::end_group ( );
				}
			} break;
			case misc_subtabs_configs: {
				if ( gui::begin_group ( L"Configs", sesui::rect ( 0.0f, 0.0f, 0.5f, 1.0f ), sesui::rect ( 0.0f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
					for ( const auto& config : configs ) {
						if ( sesui::button ( config.data ( ) ) )
							selected_config = config;
					}

					gui::end_group ( );
				}

				if ( gui::begin_group ( L"Config Actions", sesui::rect ( 0.5f, 0.0f, 0.5f, 1.0f ), sesui::rect ( sesui::style.spacing * 0.5f, 0.0f, -sesui::style.spacing * 0.5f, 0.0f ) ) ) {
					sesui::textbox ( _ ( L"Config Name" ), selected_config );

					if ( sesui::button ( _ ( L"Save" ) ) ) {
						char appdata [ MAX_PATH ];

						if ( SUCCEEDED ( LI_FN ( SHGetFolderPathA )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
							LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame" ) ).data ( ), nullptr );
							LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame\\configs" ) ).data ( ), nullptr );
						}

						options::save ( options::vars, std::string ( appdata ) + _ ( "\\sesame\\configs\\" ) + std::wstring_convert < std::codecvt_utf8 < wchar_t > > ( ).to_bytes ( selected_config ) + _ ( ".xml" ) );

						load_cfg_list ( );
					}

					if ( sesui::button ( _ ( L"Load" ) ) ) {
						char appdata [ MAX_PATH ];

						if ( SUCCEEDED ( LI_FN ( SHGetFolderPathA )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
							LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame" ) ).data ( ), nullptr );
							LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame\\configs" ) ).data ( ), nullptr );
						}

						options::load ( options::vars, std::string ( appdata ) + _ ( "\\sesame\\configs\\" ) + std::wstring_convert < std::codecvt_utf8 < wchar_t > > ( ).to_bytes ( selected_config ) + _ ( ".xml" ) );
					}

					if ( sesui::button ( _ ( L"Delete" ) ) ) {
						char appdata [ MAX_PATH ];

						if ( SUCCEEDED ( LI_FN ( SHGetFolderPathA )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
							LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame" ) ).data ( ), nullptr );
							LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame\\configs" ) ).data ( ), nullptr );
						}

						std::remove ( ( std::string ( appdata ) + _ ( "\\sesame\\configs\\" ) + std::wstring_convert < std::codecvt_utf8 < wchar_t > > ( ).to_bytes ( selected_config ) + _ ( ".xml" ) ).c_str ( ) );

						load_cfg_list ( );
					}

					if ( sesui::button ( _ ( L"Refresh List" ) ) )
						load_cfg_list ( );

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

		gui::end_window ( );
	}

	sesui::render ( );
	sesui::end_frame ( );
}
