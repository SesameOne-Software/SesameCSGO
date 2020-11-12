#include <functional>
#include <filesystem>
#include <ShlObj.h>
#include <codecvt>
#include <fstream>

#include "menu.hpp"
#include "options.hpp"

#include "../utils/networking.hpp"
#include "../features/antiaim.hpp"
#include "../features/ragebot.hpp"
#include "../utils/networking.hpp"
#include "../cjson/cJSON.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

#include "../imgui/gui.hpp"

#include "../features/skinchanger.hpp"

extern std::string last_config_user;

bool upload_to_cloud = false;
std::mutex gui::gui_mutex;

int gui::config_access = 0;
char gui::config_code[128];
char gui::config_description[128];

char gui::config_user[128];
uint64_t gui::last_update_time = 0;

bool download_config_code = false;

cJSON* cloud_config_list = nullptr;

bool gui::opened = true;
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

ImFont* gui_ui_font = nullptr;
ImFont* gui_small_font = nullptr;
ImFont* gui_icons_font = nullptr;

float g_last_dpi = 0.0f;

extern std::unordered_map<std::string, void*> font_list;

void gui::scale_dpi ( ) {
	static bool first_scale = true;

	if ( g_last_dpi == options::vars [ _ ( "gui.dpi" ) ].val.f )
		return;

	if ( !first_scale ) {
		ImGui_ImplWin32_Shutdown ( );
		ImGui_ImplDX9_Shutdown ( );
		ImGui::DestroyContext ( );
	}

	first_scale = false;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION ( );
	ImGui::CreateContext ( );
	ImGuiIO& io = ImGui::GetIO ( );
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsSesame ( );
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init ( LI_FN ( FindWindowA )( nullptr, _ ( "Counter-Strike: Global Offensive" ) ) );
	ImGui_ImplDX9_Init ( csgo::i::dev );

	//const ImWchar custom_font_ranges [ ] = {
	//	0x0020, 0x00FF, // Basic Latin + Latin Supplement
	//	0x2000, 0x206F, // General Punctuation
	//	0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
	//	0x31F0, 0x31FF, // Katakana Phonetic Extensions
	//	0xFF00, 0xFFEF, // Half-width characters
	//	0x4e00, 0x9FAF, // CJK Ideograms
	//	0x3131, 0x3163, // Korean alphabets
	//	0xAC00, 0xD7A3, // Korean characters
	//	0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
	//	0x2DE0, 0x2DFF, // Cyrillic Extended-A
	//	0xA640, 0xA69F, // Cyrillic Extended-B
	//	0x2010, 0x205E, // Punctuations
	//	0x0E00, 0x0E7F, // Thai
	//	// Vietnamese
	//	0x0102, 0x0103,
	//	0x0110, 0x0111,
	//	0x0128, 0x0129,
	//	0x0168, 0x0169,
	//	0x01A0, 0x01A1,
	//	0x01AF, 0x01B0,
	//	0x1EA0, 0x1EF9,
	//	0
	//};

	const ImWchar custom_font_ranges [ ] = { 0x20, 0xFFFF, 0 };

	gui_ui_font = io.Fonts->AddFontFromMemoryTTF ( g::resources::sesame_ui, g::resources::sesame_ui_size, 15.0f * options::vars [ _ ( "gui.dpi" ) ].val.f, nullptr, io.Fonts->GetGlyphRangesCyrillic ( ) );
	gui_small_font = io.Fonts->AddFontFromMemoryTTF ( g::resources::sesame_ui, g::resources::sesame_ui_size, 12.0f * options::vars [ _ ( "gui.dpi" ) ].val.f, nullptr, io.Fonts->GetGlyphRangesCyrillic ( ) );
	gui_icons_font = io.Fonts->AddFontFromMemoryTTF ( g::resources::sesame_icons, g::resources::sesame_icons_size, 28.0f * options::vars [ _ ( "gui.dpi" ) ].val.f, nullptr, io.Fonts->GetGlyphRangesDefault ( ) );

	render::create_font ( g::resources::sesame_ui, g::resources::sesame_ui_size, _ ( "dbg_font" ), 18.0f );
	render::create_font ( g::resources::sesame_ui, g::resources::sesame_ui_size, _ ( "esp_font" ), 18.0f );
	render::create_font ( g::resources::sesame_ui, g::resources::sesame_ui_size, _ ( "indicator_font" ), 32.0f );
	render::create_font ( g::resources::sesame_ui, g::resources::sesame_ui_size, _ ( "watermark_font" ), 18.0f );

	ImGui::GetStyle ( ).AntiAliasedFill = ImGui::GetStyle ( ).AntiAliasedLines = true;

	ImGui::GetStyle ( ).ScaleAllSizes ( options::vars [ _ ( "gui.dpi" ) ].val.f );

	g_last_dpi = options::vars [ _ ( "gui.dpi" ) ].val.f;
}

void gui::init( ) {
	g_username = ( g::loader_data && g::loader_data->username ) ? g::loader_data->username : _ ( "sesame" );
//
	//if ( !g::loader_data || !g::loader_data->avatar || !g::loader_data->avatar_sz )
	//	g_pfp_data = std::string( reinterpret_cast< const char* >( ses_pfp ), sizeof( ses_pfp ) );//networking::get(_("sesame.one/data/avatars/s/0/1.jpg"));
	//else
	//	g_pfp_data = std::string( g::loader_data->avatar, g::loader_data->avatar + g::loader_data->avatar_sz );

	/* initialize cheat config */
	options::init( );
	//erase::erase_func ( options::init );
	//erase::erase_func ( options::add_antiaim_config );
	//erase::erase_func ( options::add_player_visual_config );
	//erase::erase_func ( options::add_weapon_config );

	scale_dpi ( );

	gui_mutex.lock ( );
	load_cfg_list ( );
	gui_mutex.unlock ( );

	END_FUNC
}

char selected_config [ 128 ] = "default";
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

	ImGui::BeginChildFrame ( ImGui::GetID ( "Weapon Settings" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
		ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Weapon Settings" ).x * 0.5f );
		ImGui::Text ( "Weapon Settings" );
		ImGui::Separator ( );

		ImGui::Checkbox( _( "Inherit Default" ), &options::vars [ ragebot_weapon + _( "inherit_default" ) ].val.b );
		ImGui::Checkbox( _( "Choke On Shot" ), &options::vars [ ragebot_weapon + _( "choke_onshot" ) ].val.b );
		ImGui::Checkbox( _( "Silent Aim" ), &options::vars [ ragebot_weapon + _( "silent" ) ].val.b );
		ImGui::Checkbox( _( "Auto Shoot" ), &options::vars [ ragebot_weapon + _( "auto_shoot" ) ].val.b );
		ImGui::Checkbox( _( "Auto Scope" ), &options::vars [ ragebot_weapon + _( "auto_scope" ) ].val.b );
		ImGui::Checkbox( _( "Auto Slow" ), &options::vars [ ragebot_weapon + _( "auto_slow" ) ].val.b );
		ImGui::Checkbox ( _ ( "Doubletap Teleport" ), &options::vars [ ragebot_weapon + _ ( "dt_teleport" ) ].val.b );
		ImGui::Checkbox( _( "Doubletap" ), &options::vars [ ragebot_weapon + _( "dt_enabled" ) ].val.b );
		ImGui::PushItemWidth ( -1.0f );
		ImGui::SliderInt ( _ ( "Doubletap Recharge Delay" ), &options::vars [ ragebot_weapon + _ ( "dt_recharge_delay" ) ].val.i, 0, 1000, _ ( "%d ms" ) );
		ImGui::SliderInt( _( "Doubletap Amount" ), &options::vars [ ragebot_weapon + _( "dt_ticks" ) ].val.i, 0, 16, _( "%d ticks" ) );
		ImGui::SliderFloat( _( "Minimum Damage" ), &options::vars [ ragebot_weapon + _( "min_dmg" ) ].val.f, 0.0f, 150.0f, ( options::vars [ ragebot_weapon + _( "min_dmg" ) ].val.f > 100.0f ? ( _( "HP + " ) + std::to_string( static_cast< int > ( options::vars [ ragebot_weapon + _( "min_dmg" ) ].val.f - 100.0f ) ) + _( " HP" ) ) : ( std::to_string( static_cast< int > ( options::vars [ ragebot_weapon + _( "min_dmg" ) ].val.f ) ) + _( " HP" ) ) ).c_str( ) );
		ImGui::SliderFloat( _( "Damage Accuracy" ), &options::vars [ ragebot_weapon + _( "dmg_accuracy" ) ].val.f, 0.0f, 100.0f, _( "%.1f%" ) );
		ImGui::SliderFloat( _( "Hit Chance" ), &options::vars [ ragebot_weapon + _( "hit_chance" ) ].val.f, 0.0f, 100.0f, _( "%.1f%" ) );
		ImGui::SliderFloat( _( "Doubletap Hit Chance" ), &options::vars [ ragebot_weapon + _( "dt_hit_chance" ) ].val.f, 0.0f, 100.0f, _( "%.1f%" ) );
		ImGui::PopItemWidth ( );

		ImGui::EndChildFrame( );
	}

	ImGui::SameLine ( );

	ImGui::BeginChildFrame ( ImGui::GetID ( "Hitscan" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
		ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Hitscan" ).x * 0.5f );
		ImGui::Text ( "Hitscan" );
		ImGui::Separator ( );

		ImGui::Checkbox( _( "Bodyaim If Lethal" ), &options::vars [ ragebot_weapon + _( "baim_if_lethal" ) ].val.b );
		ImGui::Checkbox( _( "Bodyaim If In Air" ), &options::vars [ ragebot_weapon + _( "baim_in_air" ) ].val.b );
		ImGui::Checkbox ( _ ( "Onshot Only" ), &options::vars [ ragebot_weapon + _ ( "onshot_only" ) ].val.b );
		ImGui::PushItemWidth ( -1.0f );
		ImGui::SliderInt ( _( "Bodyaim After Misses" ), &options::vars [ ragebot_weapon + _( "force_baim" ) ].val.i, 0, 5, _( "%d misses" ) );
		ImGui::PopItemWidth ( );
		ImGui::Checkbox( _( "Headshot Only" ), &options::vars [ ragebot_weapon + _( "headshot_only" ) ].val.b );
		ImGui::PushItemWidth ( -1.0f );
		ImGui::SliderFloat( _( "Head Point Scale" ), &options::vars [ ragebot_weapon + _( "head_pointscale" ) ].val.f, 0.0f, 100.0f, _( "%.1f%" ) );
		ImGui::SliderFloat( _( "Body Point Scale" ), &options::vars [ ragebot_weapon + _( "body_pointscale" ) ].val.f, 0.0f, 100.0f, _( "%.1f%" ) );
		ImGui::PopItemWidth ( );

		static std::vector<const char*> hitboxes {
			"Head",
			"Neck",
			"Chest",
			"Pelvis",
			"Arms",
			"Legs",
			"Feet",
		};

		ImGui::MultiCombo ( _ ( "Hitboxes" ), options::vars [ ragebot_weapon + _ ( "hitboxes" ) ].val.l, hitboxes.data(), hitboxes.size());

		ImGui::EndChildFrame( );
	}
}

void gui::antiaim_controls( const std::string& antiaim_name ) {
	const auto antiaim_config = _( "antiaim." ) + antiaim_name + _( "." );

	ImGui::BeginChildFrame ( ImGui::GetID ( "Antiaim" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
		ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Antiaim" ).x * 0.5f );
		ImGui::Text ( "Antiaim" );
		ImGui::Separator ( );

		ImGui::Checkbox( _( "Enable" ), &options::vars [ antiaim_config + _( "enabled" ) ].val.b );
		ImGui::PushItemWidth ( -1.0f );
		ImGui::SliderInt ( _( "Fakelag Factor" ), &options::vars [ antiaim_config + _( "fakelag_factor" ) ].val.i, 0, 16, _( "%d ticks" ) );
		ImGui::PopItemWidth ( );

		static std::vector<const char*> pitches { "None", "Down", "Up", "Zero" };
		ImGui::PushItemWidth ( -1.0f );
		ImGui::Combo( _( "Base Pitch" ), &options::vars [ antiaim_config + _( "pitch" ) ].val.i, pitches.data(), pitches.size() );
		ImGui::PopItemWidth ( );
		ImGui::PushItemWidth ( -1.0f );
		ImGui::SliderFloat( _( "Yaw Offset" ), &options::vars [ antiaim_config + _( "yaw_offset" ) ].val.f, -180.0f, 180.0f, (char*)_( u8"%.1f°" ) );
		ImGui::PopItemWidth ( );

		static std::vector<const char*> base_yaws { "Relative", "Absolute", "At Target", "Auto Direction" };
		ImGui::PushItemWidth ( -1.0f );
		ImGui::Combo ( _( "Base Yaw" ), &options::vars [ antiaim_config + _( "base_yaw" ) ].val.i, base_yaws.data(), base_yaws.size());
		ImGui::SliderFloat( _( "Auto Direction Amount" ), &options::vars [ antiaim_config + _( "auto_direction_amount" ) ].val.f, -180.0f, 180.0f, ( char* ) _( u8"%.1f°" ) );
		ImGui::SliderFloat( _( "Auto Direction Range" ), &options::vars [ antiaim_config + _( "auto_direction_range" ) ].val.f, 0.0f, 100.0f, _( "%.1f units" ) );
		ImGui::SliderFloat( _( "Jitter Range" ), &options::vars [ antiaim_config + _( "jitter_range" ) ].val.f, -180.0f, 180.0f, ( char* ) _( u8"%.1f°" ) );
		ImGui::SliderFloat( _( "Rotation Range" ), &options::vars [ antiaim_config + _( "rotation_range" ) ].val.f, -180.0f, 180.0f, ( char* ) _( u8"%.1f°" ) );
		ImGui::SliderFloat( _( "Rotation Speed" ), &options::vars [ antiaim_config + _( "rotation_speed" ) ].val.f, 0.0f, 2.0f, _( "%.1f Hz" ) );
		ImGui::PopItemWidth ( );

		ImGui::EndChildFrame( );
	}

	ImGui::SameLine ( );

	ImGui::BeginChildFrame ( ImGui::GetID ( "Desync" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
		ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Desync" ).x * 0.5f );
		ImGui::Text ( "Desync" );
		ImGui::Separator ( );

		ImGui::Checkbox( _( "Enable Desync" ), &options::vars [ antiaim_config + _( "desync" ) ].val.b );
		ImGui::Checkbox( _( "Invert Initial Side" ), &options::vars [ antiaim_config + _( "invert_initial_side" ) ].val.b );
		ImGui::Checkbox( _( "Jitter Desync" ), &options::vars [ antiaim_config + _( "jitter_desync_side" ) ].val.b );
		ImGui::Checkbox( _( "Center Real" ), &options::vars [ antiaim_config + _( "center_real" ) ].val.b );
		ImGui::Checkbox( _( "Anti Bruteforce" ), &options::vars [ antiaim_config + _( "anti_bruteforce" ) ].val.b );
		ImGui::Checkbox( _( "Anti Freestanding Prediction" ), &options::vars [ antiaim_config + _( "anti_freestand_prediction" ) ].val.b );
		ImGui::PushItemWidth ( -1.0f );
		ImGui::SliderFloat( _( "Desync Range" ), &options::vars [ antiaim_config + _( "desync_range" ) ].val.f, 0.0f, 60.0f, ( char* ) _( u8"%.1f°" ) );
		ImGui::SliderFloat( _( "Desync Range Inverted" ), &options::vars [ antiaim_config + _( "desync_range_inverted" ) ].val.f, 0.0f, 60.0f, ( char* ) _( u8"%.1f°" ) );
		ImGui::PopItemWidth ( );

		if ( antiaim_name == _( "standing" ) ) {
			static std::vector<const char*> desync_types { "Default","LBY"  };
			ImGui::PushItemWidth ( -1.0f );
			ImGui::Combo ( _( "Desync Type" ), &options::vars [ antiaim_config + _( "desync_type" ) ].val.i, desync_types.data(), desync_types.size() );
			ImGui::PopItemWidth (  );
			static std::vector<const char*> desync_types_inverted {"Default", "LBY" };
			ImGui::PushItemWidth ( -1.0f );
			ImGui::Combo ( _( "Desync Type Inverted" ), &options::vars [ antiaim_config + _( "desync_type_inverted" ) ].val.i, desync_types_inverted.data(), desync_types_inverted.size() );
			ImGui::PopItemWidth ( );
		}

		ImGui::EndChildFrame( );
	}
}

void gui::player_visuals_controls( const std::string& visual_name ) {
	const auto visuals_config = _( "visuals." ) + visual_name + _( "." );

	ImGui::BeginChildFrame ( ImGui::GetID ( "Player Visuals" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
		ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Player Visuals" ).x * 0.5f );
		ImGui::Text ( "Player Visuals" );
		ImGui::Separator ( );

		static std::vector<const char*> filters { 
			"Local",
			"Teammates",
			"Enemies"
		};

		ImGui::MultiCombo( _( "Filters" ), options::vars [ _ ( "visuals.filters" ) ].val.l, filters.data(), filters.size());

		if ( visual_name == _( "local" ) ) {
			static std::vector<const char*> visual_options { 
			"Chams" ,
			"Flat Chams" ,
			"XQZ Chams" ,
			"Desync Chams" ,
			"Desync Chams Fakelag",
			"Desync Chams Rimlight",
			"Glow" , 
			"Rimlight Overlay",
			"ESP Box", 
			"Health Bar",
			"Ammo Bar",
			"Desync Bar", 
			"Value Text",
			"Name Tag",
			"Weapon Name",
			};

			ImGui::MultiCombo( _( "Options" ), options::vars [ _ ( "visuals.local.options" ) ].val.l, visual_options.data(), visual_options.size() );
		}
		else if ( visual_name == _( "enemies" ) ) {
			static std::vector<const char*> visual_options {
			"Chams" ,
			"Flat Chams",
			"XQZ Chams" ,
			"Backtrack Chams" ,
			"Hit Matrix" ,
			"Glow" ,
			"Rimlight Overlay" ,
			"ESP Box" ,
			"Health Bar" ,
			"Ammo Bar" ,
			"Desync Bar" ,
			"Value Text" ,
			"Name Tag" ,
			"Weapon Name" ,
			};

			ImGui::MultiCombo( _( "Options" ), options::vars [ _ ( "visuals.enemies.options" ) ].val.l , visual_options .data(), visual_options .size());
		}
		else if ( visual_name == _( "teammates" ) ) {
			static std::vector<const char*> visual_options {
			"Chams" ,
			"Flat Chams" ,
			"XQZ Chams" ,
			"Glow" ,
			"Rimlight Overlay" ,
			"ESP Box" ,
			"Health Bar" ,
			"Ammo Bar" ,
			"Desync Bar" ,
			"Value Text" ,
			"Name Tag" ,
			"Weapon Name" ,
			};

			ImGui::MultiCombo( _( "Options" ), options::vars [ _ ( "visuals.teammates.options" ) ].val.l , visual_options .data(), visual_options .size());
		}

		static std::vector<const char*> element_locations { "Left",  "Right", "Bottom", "Top" };
		ImGui::PushItemWidth ( -1.0f );
		ImGui::Combo( _( "Health Bar Location" ), &options::vars [ visuals_config + _( "health_bar_location" ) ].val.i, element_locations.data(), element_locations.size() );
		ImGui::Combo( _( "Ammo Bar Location" ), &options::vars [ visuals_config + _( "ammo_bar_location" ) ].val.i, element_locations.data(), element_locations.size() );
		ImGui::Combo( _( "Desync Bar Location" ), &options::vars [ visuals_config + _( "desync_bar_location" ) ].val.i, element_locations.data(), element_locations.size() );
		ImGui::Combo( _( "Value Text Location" ), &options::vars [ visuals_config + _( "value_text_location" ) ].val.i, element_locations.data(), element_locations.size() );
		ImGui::Combo( _( "Name Tag Location" ), &options::vars [ visuals_config + _( "nametag_location" ) ].val.i, element_locations.data(), element_locations.size() );
		ImGui::Combo( _( "Weapon Name Location" ), &options::vars [ visuals_config + _( "weapon_name_location" ) ].val.i, element_locations.data(), element_locations.size() );
		ImGui::SliderFloat( _( "Chams Reflectivity" ), &options::vars [ visuals_config + _( "reflectivity" ) ].val.f, 0.0f, 100.0f, _( "%.1f%" ) );
		ImGui::SliderFloat( _( "Chams Phong" ), &options::vars [ visuals_config + _( "phong" ) ].val.f, 0.0f, 100.0f, _( "%.1f%" ) );
		ImGui::PopItemWidth ( );

		ImGui::EndChildFrame( );
	}

	ImGui::SameLine ( );

	ImGui::BeginChildFrame ( ImGui::GetID ( "Colors" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
		ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Colors" ).x * 0.5f );
		ImGui::Text ( "Colors" );
		ImGui::Separator ( );

		ImGui::ColorEdit4( _( "Chams Color" ), (float*)&options::vars [ visuals_config + _( "chams_color" ) ].val.c );
		ImGui::ColorEdit4( _( "XQZ Chams Color" ), ( float* ) &options::vars [ visuals_config + _( "xqz_chams_color" ) ].val.c );

		if ( visual_name == _ ( "enemies" ) ) {
			ImGui::ColorEdit4 ( _ ( "Backtrack Chams Color" ), ( float* ) &options::vars [ visuals_config + _ ( "backtrack_chams_color" ) ].val.c );
		}

		if ( visual_name == _( "local" ) ) {
			ImGui::ColorEdit4( _( "Desync Chams Color" ), ( float* ) &options::vars [ _( "visuals.local.desync_chams_color" ) ].val.c );
			ImGui::ColorEdit4( _( "Desync Chams Rimlight Color" ), ( float* ) &options::vars [ _( "visuals.local.desync_rimlight_overlay_color" ) ].val.c );
		}

		if ( visual_name == _ ( "enemies" ) ) {
			ImGui::ColorEdit4 ( _ ( "Hit Matrix Color" ), ( float* ) &options::vars [ visuals_config + _ ( "hit_matrix_color" ) ].val.c );
		}

		ImGui::ColorEdit4( _( "Glow Color" ), ( float* ) &options::vars [ visuals_config + _( "glow_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Rimlight Overlay Color" ), ( float* ) &options::vars [ visuals_config + _( "rimlight_overlay_color" ) ].val.c );
		ImGui::ColorEdit4( _( "ESP Box Color" ), ( float* ) &options::vars [ visuals_config + _( "box_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Health Bar Color" ), ( float* ) &options::vars [ visuals_config + _( "health_bar_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Ammo Bar Color" ), ( float* ) &options::vars [ visuals_config + _( "ammo_bar_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Desync Bar Color" ), ( float* ) &options::vars [ visuals_config + _( "desync_bar_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Name Tag Color" ), ( float* ) &options::vars [ visuals_config + _( "name_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Weapon Name Color" ), ( float* ) &options::vars [ visuals_config + _( "weapon_color" ) ].val.c );

		ImGui::EndChildFrame( );
	}
}

void gui::draw( ) {
	/* HANDLE DPI */
	//sesui::globals::dpi = options::vars [ _( "gui.dpi" ) ].val.f;

	if ( ImGui::IsKeyReleased ( VK_INSERT ) )
		opened = !opened;

	open_button_pressed = utils::key_state( VK_INSERT );
	
	watermark::draw( );
	keybinds::draw( );

	if ( opened ) {
		ImGui::PushFont ( gui_ui_font );

		if ( ImGui::custom::Begin ( SESAME_VERSION, &opened, gui_small_font ) ) {			
			/* main menu objects */
			if ( ImGui::custom::BeginTabs ( &current_tab_idx, gui_icons_font ) ) {
				ImGui::custom::AddTab ( "A" );
				ImGui::custom::AddTab ( "B" );
				ImGui::custom::AddTab ( "C" );
				ImGui::custom::AddTab ( "D" );
				ImGui::custom::AddTab ( "E" );
				ImGui::custom::AddTab ( "F" );

				ImGui::custom::EndTabs ( );
			}

			/* confirm config save (overwrite) popup */
			ImGui::SetNextWindowPos ( ImVec2 ( ImGui::GetWindowPos ( ).x + ImGui::GetWindowSize ( ).x * 0.5f, ImGui::GetWindowPos ( ).y + ImGui::GetWindowSize ( ).y * 0.5f ), ImGuiCond_Always, ImVec2 ( 0.5f, 0.5f ) );

			if ( ImGui::BeginPopupModal ( "Save Config##popup", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings ) ) {
				ImGui::TextColored ( ImVec4 ( 1.0f, 0.1f, 0.1f, 1.0f ), "There already is a config with the same name in this location.\nAre you sure you want to overwrite the config?" );

				if ( ImGui::Button ( "Confirm", ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ) ) ) {
					char appdata [ MAX_PATH ];

					if ( SUCCEEDED ( LI_FN ( SHGetFolderPathA )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
						LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame" ) ).data ( ), nullptr );
						LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame\\configs" ) ).data ( ), nullptr );
					}

					const auto file = std::string ( appdata ).append ( _ ( "\\sesame\\configs\\" ) ).append ( selected_config ).append ( _ ( ".xml" ) );

					options::save ( options::vars, file );

					gui_mutex.lock ( );
					load_cfg_list ( );
					gui_mutex.unlock ( );

					csgo::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );

					ImGui::CloseCurrentPopup ( );
				}

				ImGui::SameLine ( );

				if ( ImGui::Button ( "Cancel", ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ) ) ) {
					ImGui::CloseCurrentPopup ( );
				}

				ImGui::EndPopup ( );
			}

			bool open_save_modal = false;

			switch ( current_tab_idx ) {
				case tab_legit: {
					ImGui::custom::AddSubtab ( "General", "General ragebot and accuracy settings", [ & ] ( ) {
					} );

					ImGui::custom::AddSubtab ( "Default", "Default settings used for unconfigured weapons", [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( "General Settings" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "General Settings" ).x * 0.5f );
							ImGui::Text ( "General Settings" );
							ImGui::Separator ( );

							static std::vector<const char*> assist_type { "None", "Legit" ,  "Rage" };
							ImGui::PushItemWidth ( -1.0f );
							ImGui::Combo ( _ ( "Assistance Type" ), &options::vars [ _ ( "global.assistance_type" ) ].val.i, assist_type.data ( ), assist_type.size ( ) );
							ImGui::PopItemWidth ( );

							ImGui::EndChildFrame ( );
						}

						ImGui::SameLine ( );

						ImGui::BeginChildFrame ( ImGui::GetID ( "Triggerbot" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Triggerbot" ).x * 0.5f );
							ImGui::Text ( "Triggerbot" );
							ImGui::Separator ( );

							ImGui::Checkbox ( _ ( "Triggerbot" ), &options::vars [ _ ( "legitbot.triggerbot" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::Keybind ( _ ( "##Triggerbot Key" ), &options::vars [ _ ( "legitbot.triggerbot_key" ) ].val.i, &options::vars [ _ ( "legitbot.triggerbot_key_mode" ) ].val.i, ImVec2(-1.0f, 0.0f) );

							static std::vector<const char*> hitboxes {
										"Head" ,
										"Neck" ,
										"Chest" ,
										"Pelvis" ,
										"Arms" ,
							};

							ImGui::MultiCombo ( _ ( "Hitboxes" ), options::vars [ _ ( "legitbot.triggerbot_hitboxes" ) ].val.l, hitboxes.data ( ), hitboxes.size ( ) );

							ImGui::EndChildFrame ( );
						}
					} );

					ImGui::custom::AddSubtab ( "Pistol", "Pistol class configuration", [ & ] ( ) {
					} );
					ImGui::custom::AddSubtab ( "Revolver", "Revolver class configuration", [ & ] ( ) {
					} );
					ImGui::custom::AddSubtab ( "Rifle", "Rifle, SMG, and shotgun class configuration", [ & ] ( ) {
					} );
					ImGui::custom::AddSubtab ( "AWP", "AWP class configuration", [ & ] ( ) {
					} );
					ImGui::custom::AddSubtab ( "Auto", "Autosniper class configuration", [ & ] ( ) {
					} );
					ImGui::custom::AddSubtab ( "Scout", "Scout class configuration", [ & ] ( ) {
					} );
				} break;
				case tab_rage: {
					ImGui::custom::AddSubtab ( "General", "General ragebot and accuracy settings", [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( "General Settings" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "General Settings" ).x * 0.5f );
							ImGui::Text ( "General Settings" );
							ImGui::Separator ( );

							static std::vector<const char*> assist_type {  "None", "Legit", "Rage" };

							ImGui::PushItemWidth ( -1.0f );
							ImGui::Combo ( _ ( "Assistance Type" ), &options::vars [ _ ( "global.assistance_type" ) ].val.i, assist_type.data ( ), assist_type.size ( ) );
							ImGui::PopItemWidth ( );
							ImGui::Checkbox ( _ ( "Knife Bot" ), &options::vars [ _ ( "ragebot.knife_bot" ) ].val.b );
							ImGui::Checkbox ( _ ( "Zeus Bot" ), &options::vars [ _ ( "ragebot.zeus_bot" ) ].val.b );
							ImGui::Checkbox ( _ ( "Auto Peek" ), &options::vars [ _ ( "ragebot.autopeek" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::Keybind ( _ ( "##Auto Peek Key" ), &options::vars [ _ ( "ragebot.autopeek_key" ) ].val.i, &options::vars [ _ ( "ragebot.autopeek_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );

							ImGui::EndChildFrame ( );
						}

						ImGui::SameLine ( );

						ImGui::BeginChildFrame ( ImGui::GetID ( "Accuracy" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Accuracy" ).x * 0.5f );
							ImGui::Text ( "Accuracy" );
							ImGui::Separator ( );

							ImGui::Checkbox ( _ ( "Fix Fakelag" ), &options::vars [ _ ( "ragebot.fix_fakelag" ) ].val.b );
							ImGui::Checkbox ( _ ( "Resolve Desync" ), &options::vars [ _ ( "ragebot.resolve_desync" ) ].val.b );
							ImGui::Checkbox ( _ ( "Safe Point" ), &options::vars [ _ ( "ragebot.safe_point" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::Keybind ( _ ( "##Safe Point Key" ), &options::vars [ _ ( "ragebot.safe_point_key" ) ].val.i, &options::vars [ _ ( "ragebot.safe_point_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Checkbox ( _ ( "Auto Revolver" ), &options::vars [ _ ( "ragebot.auto_revolver" ) ].val.b );
							ImGui::Keybind ( _ ( "Doubletap Key" ), &options::vars [ _ ( "ragebot.dt_key" ) ].val.i, &options::vars [ _ ( "ragebot.dt_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );

							ImGui::EndChildFrame ( );
						}
					} );
					ImGui::custom::AddSubtab ( "Default", "Default settings used for unconfigured weapons", [ & ] ( ) {
						weapon_controls ( _ ( "default" ) );
					} );
					ImGui::custom::AddSubtab ( "Pistol", "Pistol class configuration", [ & ] ( ) {
						weapon_controls ( _ ( "pistol" ) );
					} );
					ImGui::custom::AddSubtab ( "Revolver", "Revolver class configuration", [ & ] ( ) {
						weapon_controls ( _ ( "revolver" ) );
					} );
					ImGui::custom::AddSubtab ( "Rifle", "Rifle, SMG, and shotgun class configuration", [ & ] ( ) {
						weapon_controls ( _ ( "rifle" ) );
					} );
					ImGui::custom::AddSubtab ( "AWP", "AWP class configuration", [ & ] ( ) {
						weapon_controls ( _ ( "awp" ) );
					} );
					ImGui::custom::AddSubtab ( "Auto", "Autosniper class configuration", [ & ] ( ) {
						weapon_controls ( _ ( "auto" ) );
					} );
					ImGui::custom::AddSubtab ( "Scout", "Scout class configuration", [ & ] ( ) {
						weapon_controls ( _ ( "scout" ) );
					} );

				} break;
				case tab_antiaim: {
					ImGui::custom::AddSubtab ( "Air", "In air antiaim settings", [ & ] ( ) {
						antiaim_controls ( _ ( "air" ) );
					} );
					ImGui::custom::AddSubtab ( "Moving", "Moving antiaim settings", [ & ] ( ) {
						antiaim_controls ( _ ( "moving" ) );
					} );
					ImGui::custom::AddSubtab ( "Slow Walk", "Slow walk antiaim settings", [ & ] ( ) {
						antiaim_controls ( _ ( "slow_walk" ) );
					} );
					ImGui::custom::AddSubtab ( "Standing", "Standing antiaim settings", [ & ] ( ) {
						antiaim_controls ( _ ( "standing" ) );
					} );
					ImGui::custom::AddSubtab ( "Other", "Fakelag, manual aa, and other antiaim features", [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( "General" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "General" ).x * 0.5f );
							ImGui::Text ( "General" );
							ImGui::Separator ( );

							ImGui::PushItemWidth ( -1.0f );
							ImGui::SliderFloat ( _ ( "Slow Walk Speed" ), &options::vars [ _ ( "antiaim.slow_walk_speed" ) ].val.f, 0.0f, 100.0f, _ ( "%.1f%" ) );
							ImGui::PopItemWidth ( );
							ImGui::SameLine ( );
							ImGui::Keybind ( _ ( "Slow Walk Key" ), &options::vars [ _ ( "antiaim.slow_walk_key" ) ].val.i, &options::vars [ _ ( "antiaim.slow_walk_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Checkbox ( _ ( "Fake Walk" ), &options::vars [ _ ( "antiaim.fakewalk" ) ].val.b );
							ImGui::Checkbox ( _ ( "Fake Duck" ), &options::vars [ _ ( "antiaim.fakeduck" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::Keybind ( _ ( "Fake Duck Key" ), &options::vars [ _ ( "antiaim.fakeduck_key" ) ].val.i, &options::vars [ _ ( "antiaim.fakeduck_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							static std::vector<const char*> fake_duck_modes { "Normal" ,  "Full" };

							ImGui::PushItemWidth ( -1.0f );
							ImGui::Combo ( _ ( "Fake Duck Mode" ), &options::vars [ _ ( "antiaim.fakeduck_mode" ) ].val.i, fake_duck_modes.data ( ), fake_duck_modes.size ( ) );
							ImGui::PopItemWidth ( );
							ImGui::Keybind ( _ ( "Manual Left Key" ), &options::vars [ _ ( "antiaim.manual_left_key" ) ].val.i, &options::vars [ _ ( "antiaim.manual_left_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Keybind ( _ ( "Manual Right Key" ), &options::vars [ _ ( "antiaim.manual_right_key" ) ].val.i, &options::vars [ _ ( "antiaim.manual_right_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Keybind ( _ ( "Manual Back Key" ), &options::vars [ _ ( "antiaim.manual_back_key" ) ].val.i, &options::vars [ _ ( "antiaim.manual_back_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Keybind ( _ ( "Desync Inverter Key" ), &options::vars [ _ ( "antiaim.desync_invert_key" ) ].val.i, &options::vars [ _ ( "antiaim.desync_invert_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );

							ImGui::EndChildFrame ( );
						}
					} );
				} break;
				case tab_visuals: {
					ImGui::custom::AddSubtab ( "Local Player", "Visuals used on local player", [ & ] ( ) {
						player_visuals_controls ( _ ( "local" ) );
					} );
					ImGui::custom::AddSubtab ( "Enemies", "Visuals used on filtered enemies", [ & ] ( ) {
						player_visuals_controls ( _ ( "enemies" ) );
					} );
					ImGui::custom::AddSubtab ( "Teammates", "Visuals used on filtered teammates", [ & ] ( ) {
						player_visuals_controls ( _ ( "teammates" ) );
					} );
					ImGui::custom::AddSubtab ( "Other", "Other visual options", [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( "World" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "World" ).x * 0.5f );
							ImGui::Text ( "World" );
							ImGui::Separator ( );

							ImGui::Checkbox ( _ ( "Bomb ESP" ), &options::vars [ _ ( "visuals.other.bomb_esp" ) ].val.b );
							ImGui::Checkbox ( _ ( "Bomb Timer" ), &options::vars [ _ ( "visuals.other.bomb_timer" ) ].val.b );
							ImGui::Checkbox ( _ ( "Bullet Tracers" ), &options::vars [ _ ( "visuals.other.bullet_tracers" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Bullet Tracer Color" ), ( float* ) &options::vars [ _ ( "visuals.other.bullet_tracer_color" ) ].val.c );
							ImGui::Checkbox ( _ ( "Bullet Impacts" ), &options::vars [ _ ( "visuals.other.bullet_impacts" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Bullet Impacts Color" ), ( float* ) &options::vars [ _ ( "visuals.other.bullet_impact_color" ) ].val.c );
							ImGui::Checkbox ( _ ( "Grenade Trajectories" ), &options::vars [ _ ( "visuals.other.grenade_trajectories" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Grenade Trajectories Color" ), ( float* ) &options::vars [ _ ( "visuals.other.grenade_trajectory_color" ) ].val.c );
							ImGui::Checkbox ( _ ( "Grenade Bounces" ), &options::vars [ _ ( "visuals.other.grenade_bounces" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Grenade Bounces Color" ), ( float* ) &options::vars [ _ ( "visuals.other.grenade_bounce_color" ) ].val.c );
							ImGui::Checkbox ( _ ( "Grenade Blast Radii" ), &options::vars [ _ ( "visuals.other.grenade_blast_radii" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Grenade Blast Radii Color" ), ( float* ) &options::vars [ _ ( "visuals.other.grenade_radii_color" ) ].val.c );

							ImGui::EndChildFrame ( );
						}

						ImGui::SameLine ( );

						ImGui::BeginChildFrame ( ImGui::GetID ( "Other" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Other" ).x * 0.5f );
							ImGui::Text ( "Other" );
							ImGui::Separator ( );

							static std::vector<const char*> removals {
								"Smoke" ,
								"Flash",
								"Scope" ,
								 "Aim Punch" ,
								"View Punch" ,
								"Zoom" ,
								"Occlusion" ,
								"Killfeed Decay" ,
								"Post Processing" ,
							};

							ImGui::MultiCombo ( _ ( "Removals" ), options::vars [ _ ( "visuals.other.removals" ) ].val.l, removals.data ( ), removals.size ( ) );
							ImGui::PushItemWidth ( -1.0f );
							ImGui::SliderFloat ( _ ( "FOV" ), &options::vars [ _ ( "visuals.other.fov" ) ].val.f, 0.0f, 180.0f, ( char* ) _ ( u8"%.1f°" ) );
							ImGui::SliderFloat ( _ ( "Viewmodel FOV" ), &options::vars [ _ ( "visuals.other.viewmodel_fov" ) ].val.f, 0.0f, 180.0f, ( char* ) _ ( u8"%.1f°" ) );
							ImGui::SliderFloat ( _ ( "Aspect Ratio" ), &options::vars [ _ ( "visuals.other.aspect_ratio" ) ].val.f, 0.1f, 2.0f );
							ImGui::PopItemWidth ( );

							static std::vector<const char*> logs {
								  "Hits" ,
								"Spread Misses" ,
								"Resolver Misses" ,
								  "Wrong Hitbox" ,
								 "Manual Shots" ,
							};

							ImGui::MultiCombo ( _ ( "Logs" ), options::vars [ _ ( "visuals.other.logs" ) ].val.l, logs.data ( ), logs.size ( ) );

							ImGui::Checkbox ( _ ( "Spread Circle" ), &options::vars [ _ ( "visuals.other.spread_circle" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Spread Circle Color" ), ( float* ) &options::vars [ _ ( "visuals.other.spread_circle_color" ) ].val.c );
							ImGui::Checkbox ( _ ( "Gradient Spread Circle" ), &options::vars [ _ ( "visuals.other.gradient_spread_circle" ) ].val.b );
							ImGui::Checkbox ( _ ( "Offscreen ESP" ), &options::vars [ _ ( "visuals.other.offscreen_esp" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Offscreen ESP Color" ), ( float* ) &options::vars [ _ ( "visuals.other.offscreen_esp_color" ) ].val.c );
							ImGui::PushItemWidth ( -1.0f );
							ImGui::SliderFloat ( _ ( "Offscreen ESP Distance" ), &options::vars [ _ ( "visuals.other.offscreen_esp_distance" ) ].val.f, 0.0f, 100.0f, _ ( "%.1f%" ) );
							ImGui::SliderFloat ( _ ( "Offscreen ESP Size" ), &options::vars [ _ ( "visuals.other.offscreen_esp_size" ) ].val.f, 0.0f, 100.0f, (std::to_string ( static_cast< int >( options::vars [ _ ( "visuals.other.offscreen_esp_size" ) ].val.f ) ) + _ ( " px" )).c_str() );
							ImGui::PopItemWidth ( );

							ImGui::Checkbox ( _ ( "Watermark" ), &options::vars [ _ ( "visuals.other.watermark" ) ].val.b );
							ImGui::Checkbox ( _ ( "Keybind List" ), &options::vars [ _ ( "visuals.other.keybind_list" ) ].val.b );

							ImGui::ColorEdit4 ( _ ( "World Color" ), ( float* ) &options::vars [ _ ( "visuals.other.world_color" ) ].val.c );
							ImGui::ColorEdit4 ( _ ( "Prop Color" ), ( float* ) &options::vars [ _ ( "visuals.other.prop_color" ) ].val.c );
							ImGui::ColorEdit4 ( _ ( "Auto Peek Target" ), ( float* ) &options::vars [ _ ( "visuals.other.autopeek_color" ) ].val.c );
							//ImGui::ColorEdit4( _( "Accent Color" ), &options::vars [ _( "visuals.other.accent_color" ) ].val.c );
							//ImGui::ColorEdit4( _( "Secondary Accent Color" ), &options::vars [ _( "visuals.other.secondary_accent_color" ) ].val.c );
							//ImGui::ColorEdit4( _( "Logo Color" ), &options::vars [ _( "visuals.other.logo_color" ) ].val.c );
							static std::vector<const char*> hitsounds { "None", "Arena Switch", "Fall Pain" ,  "Bolt" ,  "Neck Snap" ,  "Power Switch" , "Glass" , "Bell",  "COD" , "Rattle" ,  "Sesame"  };

							ImGui::PushItemWidth ( -1.0f );
							ImGui::Combo ( _ ( "Hit Sound" ), &options::vars [ _ ( "visuals.other.hit_sound" ) ].val.i, hitsounds.data ( ), hitsounds.size ( ) );
							ImGui::PopItemWidth ( );

							ImGui::EndChildFrame ( );
						}
					} );
				} break;
				case tab_skins: {
					ImGui::custom::AddSubtab ( "Skin Changer", "Apply custom skins to your weapons", [ & ] ( ) {
						if ( ImGui::custom::InventoryBegin ( 2, 3 ) ) {
							if ( ImGui::custom::InventoryButton ( _ ( "Add Item" ) ) ) {
								features::skinchanger::add_item ( {
									false,
									knife_skeleton,
									0,
									43,
									rand ( ),
									0.0f,
									{}
									} );
							}

							for ( auto& skin : features::skinchanger::skins ) {
								const auto kit = skin.get_kit ( );

								if ( !kit )
									continue;

								ImGui::custom::InventoryButton ( kit.value ( ).name.c_str ( ), &skin );
							}

							ImGui::custom::InventoryEnd ( );
						}
					} );

					ImGui::custom::AddSubtab ( "Model Changer", "Replace game models with your own", [ & ] ( ) {

					} );
				} break;
				case tab_misc: {
					ImGui::custom::AddSubtab ( "Movement", "Movement related cheats", [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( "General" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "General" ).x * 0.5f );
							ImGui::Text ( "General" );
							ImGui::Separator ( );

							ImGui::Checkbox ( _ ( "Block Bot" ), &options::vars [ _ ( "misc.movement.block_bot" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::Keybind ( _ ( "##Block Bot Key" ), &options::vars [ _ ( "misc.movement.block_bot_key" ) ].val.i, &options::vars [ _ ( "misc.movement.block_bot_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Checkbox ( _ ( "Auto Jump" ), &options::vars [ _ ( "misc.movement.bhop" ) ].val.b );
							ImGui::Checkbox ( _ ( "Auto Forward" ), &options::vars [ _ ( "misc.movement.auto_forward" ) ].val.b );
							ImGui::Checkbox ( _ ( "Auto Strafer" ), &options::vars [ _ ( "misc.movement.auto_strafer" ) ].val.b );
							ImGui::Checkbox ( _ ( "Directional Auto Strafer" ), &options::vars [ _ ( "misc.movement.omnidirectional_auto_strafer" ) ].val.b );
							ImGui::Checkbox ( _ ( "Air Stuck" ), &options::vars [ _ ( "misc.movement.airstuck" ) ].val.b );
							ImGui::Keybind ( _ ( "Air Stuck Key" ), &options::vars [ _ ( "misc.movement.airstuck_key" ) ].val.i, &options::vars [ _ ( "misc.movement.airstuck_key_mode" ) ].val.i );

							ImGui::EndChildFrame ( );
						}
					} );
					ImGui::custom::AddSubtab ( "Effects", "Miscellaneous visual effects", [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( "General" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "General" ).x * 0.5f );
							ImGui::Text ( "General" );
							ImGui::Separator ( );

							ImGui::Checkbox ( _ ( "Third Person" ), &options::vars [ _ ( "misc.effects.third_person" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::Keybind ( _ ( "##Third Person Key" ), &options::vars [ _ ( "misc.effects.third_person_key" ) ].val.i, &options::vars [ _ ( "misc.effects.third_person_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::PushItemWidth ( -1.0f );
							ImGui::SliderFloat ( _ ( "Third Person Range" ), &options::vars [ _ ( "misc.effects.third_person_range" ) ].val.f, 0.0f, 500.0f, _ ( "%.1f units" ) );
							ImGui::SliderFloat ( _ ( "Ragdoll Force Scale" ), &options::vars [ _ ( "misc.effects.ragdoll_force_scale" ) ].val.f, 0.0f, 10.0f, _ ( "x%.1f" ) );
							ImGui::PopItemWidth ( );
							ImGui::Checkbox ( _ ( "Clan Tag" ), &options::vars [ _ ( "misc.effects.clantag" ) ].val.b );
							static std::vector<const char*> tag_anims {  "Static",  "Marquee", "Capitalize" ,  "Heart" };

							ImGui::PushItemWidth ( -1.0f );
							ImGui::Combo ( _ ( "Clan Tag Animation" ), &options::vars [ _ ( "misc.effects.clantag_animation" ) ].val.i, tag_anims.data ( ), tag_anims.size ( ) );
							ImGui::InputText ( _ ( "Clan Tag Text" ), options::vars [ _ ( "misc.effects.clantag_text" ) ].val.s, 128 );
							ImGui::SliderFloat ( _ ( "Revolver Cock Volume" ), &options::vars [ _ ( "misc.effects.revolver_cock_volume" ) ].val.f, 0.0f, 1.0f, _ ( "x%.1f" ) );
							ImGui::SliderFloat ( _ ( "Weapon Volume" ), &options::vars [ _ ( "misc.effects.weapon_volume" ) ].val.f, 0.0f, 1.0f, _ ( "x%.1f" ) );
							ImGui::PopItemWidth ( );

							ImGui::EndChildFrame ( );
						}
					} );
					ImGui::custom::AddSubtab ( "Player List", "Whitelist, clantag stealer, and bodyaim priority", [ & ] ( ) {

					} );
					ImGui::custom::AddSubtab ( "Cheat", "Cheat settings and panic button", [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( "Menu" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Menu" ).x * 0.5f );
							ImGui::Text ( "Menu" );
							ImGui::Separator ( );

							//int dpi = ( options::vars [ _ ( "gui.dpi" ) ].val.f < 1.0f ) ? 0 : static_cast< int >( options::vars [ _ ( "gui.dpi" ) ].val.f );
							//static std::vector<const char*> dpis { "0.5",  "1.0", "2.0" , "3.0" };
							//
							//ImGui::PushItemWidth ( -1.0f );
							//ImGui::Combo ( _ ( "GUI DPI" ), &dpi, dpis.data ( ), dpis.size ( ) );
							//ImGui::PopItemWidth ( );
							//
							//switch ( dpi ) {
							//case 0: options::vars [ _ ( "gui.dpi" ) ].val.f = 0.5f; break;
							//case 1: options::vars [ _ ( "gui.dpi" ) ].val.f = 1.0f; break;
							//case 2: options::vars [ _ ( "gui.dpi" ) ].val.f = 2.0f; break;
							//case 3: options::vars [ _ ( "gui.dpi" ) ].val.f = 3.0f; break;
							//}

							ImGui::EndChildFrame ( );
						}

						ImGui::SameLine ( );

						ImGui::BeginChildFrame ( ImGui::GetID ( "Cheat" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Cheat" ).x * 0.5f );
							ImGui::Text ( "Cheat" );
							ImGui::Separator ( );

							ImGui::EndChildFrame ( );
						}
					} );
					
					ImGui::custom::AddSubtab ( "Configuration", "Cheat configuration manager", [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( "Configs" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Configs" ).x * 0.5f );
							ImGui::Text ( "Configs" );
							ImGui::Separator ( );

							for ( const auto& config : configs ) {
								if ( ImGui::Button ( config.data ( ), ImVec2( -1.0f, 0.0f ) ) )
									strcpy_s ( selected_config, config.c_str ( ) );
							}

							ImGui::EndChildFrame ( );
						}

						ImGui::SameLine ( );

						ImGui::BeginChildFrame ( ImGui::GetID ( "Config Actions" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Config Actions" ).x * 0.5f );
							ImGui::Text ( "Config Actions" );
							ImGui::Separator ( );

							ImGui::PushItemWidth ( -1.0f );
							ImGui::InputText ( _ ( "Config Name" ), selected_config, sizeof ( selected_config ) );
							ImGui::PopItemWidth ( );

							if ( ImGui::Button ( _ ( "Save" ), ImVec2 ( -1.0f, 0.0f ) ) ) {
								char appdata [ MAX_PATH ];

								if ( SUCCEEDED ( LI_FN ( SHGetFolderPathA )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
									LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame" ) ).data ( ), nullptr );
									LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame\\configs" ) ).data ( ), nullptr );
								}

								auto file_exists = [ ] ( const std::string& path ) {
									std::ifstream file ( path );
									return file.good ( );
								};

								const auto file = std::string ( appdata ).append( _ ( "\\sesame\\configs\\" ) ).append( selected_config ).append(_ ( ".xml" ));

								if ( file_exists ( file ) ) {
									open_save_modal = true;
								}
								else {
									options::save ( options::vars, file );

									gui_mutex.lock ( );
									load_cfg_list ( );
									gui_mutex.unlock ( );

									csgo::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
								}
							}

							if ( ImGui::Button ( _ ( "Load" ), ImVec2 ( -1.0f, 0.0f ) ) ) {
								char appdata [ MAX_PATH ];

								if ( SUCCEEDED ( LI_FN ( SHGetFolderPathA )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
									LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame" ) ).data ( ), nullptr );
									LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame\\configs" ) ).data ( ), nullptr );
								}

								options::load ( options::vars, std::string ( appdata ) + _ ( "\\sesame\\configs\\" ) + selected_config + _ ( ".xml" ) );

								csgo::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
							}

							if ( ImGui::Button ( _ ( "Delete" ), ImVec2 ( -1.0f, 0.0f ) ) ) {
								char appdata [ MAX_PATH ];

								if ( SUCCEEDED ( LI_FN ( SHGetFolderPathA )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
									LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame" ) ).data ( ), nullptr );
									LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame\\configs" ) ).data ( ), nullptr );
								}

								std::remove ( ( std::string ( appdata ) + _ ( "\\sesame\\configs\\" ) + selected_config + _ ( ".xml" ) ).c_str ( ) );

								gui_mutex.lock ( );
								load_cfg_list ( );
								gui_mutex.unlock ( );

								csgo::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
							}

							if ( ImGui::Button ( _ ( "Refresh List" ), ImVec2 ( -1.0f, 0.0f ) ) ) {
								gui_mutex.lock ( );
								load_cfg_list ( );
								gui_mutex.unlock ( );
								csgo::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
							}

							ImGui::PushItemWidth ( -1.0f );
							ImGui::InputText ( _ ( "Config Description" ), config_description, 128 );
							static std::vector<const char*> access_perms { "Public" ,  "Private" , "Unlisted" };
							ImGui::Combo ( _ ( "Config Access" ), &config_access, access_perms.data(),access_perms.size() );
							ImGui::PopItemWidth();
							
							if ( ImGui::Button ( _ ( "Upload To Cloud" ), ImVec2( -1.0f, 0.0f ) ) )
								upload_to_cloud = true;

							ImGui::EndChildFrame ( );
						}
					} );

					ImGui::custom::AddSubtab ( "Cloud Configuration", "Access and share configs via the cloud", [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( "Cloud Configs" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Cloud Configs" ).x * 0.5f );
							ImGui::Text ( "Cloud Configs" );
							ImGui::Separator ( );

							gui_mutex.lock ( );
							const auto loading_list = last_config_user != config_user;
							gui_mutex.unlock ( );

							/* loading new config list */
							if ( loading_list ) {
								ImGui::Text ( _ ( "Loading..." ) );
							}
							else if ( cloud_config_list ) {
								cJSON* iter = nullptr;

								cJSON_ArrayForEach ( iter, cloud_config_list ) {
									const auto config_id = cJSON_GetObjectItemCaseSensitive ( iter, _ ( "config_id" ) );
									const auto config_code = cJSON_GetObjectItemCaseSensitive ( iter, _ ( "config_code" ) );
									const auto description = cJSON_GetObjectItemCaseSensitive ( iter, _ ( "description" ) );
									const auto creation_date = cJSON_GetObjectItemCaseSensitive ( iter, _ ( "creation_date" ) );

									if ( !cJSON_IsNumber ( config_id ) )
										continue;

									if ( !cJSON_IsString ( creation_date ) || !creation_date->valuestring )
										continue;

									if ( !cJSON_IsString ( config_code ) || !config_code->valuestring )
										continue;

									if ( !cJSON_IsString ( description ) || !description->valuestring )
										continue;

									if ( ImGui::Button ( iter->string, ImVec2 ( -1.0f, 0.0f ) ) ) {
										gui_mutex.lock ( );
										strcpy_s ( ::gui::config_code, config_code->valuestring );
										gui_mutex.unlock ( );
									}
								}
							}

							ImGui::EndChildFrame ( );
						}

						ImGui::SameLine ( );
						
						ImGui::BeginChildFrame ( ImGui::GetID ( "Cloud Config Actions" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Cloud Config Actions" ).x * 0.5f );
							ImGui::Text ( "Cloud Config Actions" );
							ImGui::Separator ( );

							ImGui::PushItemWidth ( -1.0f );
							ImGui::InputText ( _ ( "Search By User" ), config_user, 128 );
							ImGui::PopItemWidth ( );

							if ( ImGui::Button ( _ ( "My Configs" ), ImVec2 ( -1.0f, 0.0f ) ) )
								strcpy_s ( config_user, g_username.c_str() );
							
							gui_mutex.lock ( );
							ImGui::PushItemWidth ( -1.0f );
							ImGui::InputText ( _ ( "Config Code" ), config_code, 128 );
							ImGui::PopItemWidth ( );
							gui_mutex.unlock ( );
							
							if ( ImGui::Button ( _ ( "Download Config" ), ImVec2( -1.0f, 0.0f ) ) ) {
								gui_mutex.lock ( );
								download_config_code = true;
								gui_mutex.unlock ( );
							}
						
							ImGui::EndChildFrame ( );
						}
					} );

					ImGui::custom::AddSubtab ( "Scripts", "Script manager", [ & ] ( ) {

					} );
				} break;
			}

			//auto load_texture = [ ] ( IDirect3DTexture9*& out_texture, float& out_width, float& out_height ) {
			//	IDirect3DTexture9* texture;
			//	HRESULT hr = D3DXCreateTextureFromFileInMemory ( csgo::i::dev, g_pfp_data.data ( ), g_pfp_data.size ( ), &texture );
			//
			//	if ( hr != S_OK )
			//		return false;
			//
			//	D3DSURFACE_DESC my_image_desc;
			//	texture->GetLevelDesc ( 0, &my_image_desc );
			//
			//	out_texture = texture;
			//	out_width = my_image_desc.Width;
			//	out_height = my_image_desc.Height;
			//
			//	return true;
			//};
			//
			//IDirect3DTexture9* tex = nullptr;
			//float width = 0.0f, height = 0.0f;
			//
			//if ( load_texture ( tex, width, height ) ) {
			//	ImGui::Image ( reinterpret_cast< void* >( tex ), ImVec2 ( 20.0f, 20.0f ) );
			//	tex->Release ( );
			//}

			if ( open_save_modal )
				ImGui::OpenPopup ( "Save Config##popup" );

			ImGui::custom::End ( );
		}

		ImGui::PopFont ( );
	}
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
	static auto& autopeek = options::vars [ _ ( "ragebot.autopeek" ) ].val.b;

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
	static auto autopeek_key = find_keybind ( _ ( "ragebot.autopeek_key" ) );

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

		if ( autopeek )
			add_key_entry ( autopeek_key, _ ( "Autopeek" ) );

		if ( features::antiaim::antiaiming ) {
			switch ( features::antiaim::side ) {
				case -1: break;
				case 0: entries.push_back( _( "[TOGGLE] Manual Antiaim (Back)" ) ); break;
				case 1: entries.push_back( _( "[TOGGLE] Manual Antiaim (Left)" ) ); break;
				case 2: entries.push_back( _( "[TOGGLE] Manual Antiaim (Right)" ) ); break;
			}
		}
	}

	if ( keybind_list && ImGui::Begin( _( "Keybinds" ), &keybind_list, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize ) ) {
		ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Keybinds" ).x * 0.5f );
		ImGui::Text ( "Keybinds" );
		ImGui::Separator ( );

		if ( !entries.empty( ) ) {
			for ( auto& entry : entries )
				ImGui::Text( entry.c_str( ) );
		}

		ImGui::End( );
	}
}