#include <functional>
#include <filesystem>
#include <ShlObj.h>
#include <codecvt>
#include <fstream>
#include <span>

#include "menu.hpp"
#include "options.hpp"

#include "../utils/networking.hpp"
#include "../features/antiaim.hpp"
#include "../features/ragebot.hpp"
#include "../utils/networking.hpp"
#include "../cjson/cJSON.h"
#include "../animations/resolver.hpp"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

#include "../imgui/gui.hpp"

#include "../features/skinchanger.hpp"

#include "../resources/roboto.hpp"
#include "../resources/roboto_bold.hpp"

#include "../scripting/js_api.hpp"

extern std::string last_config_user;

extern uint64_t current_plist_player;
extern uint64_t player_to_steal_tag_from;

bool upload_to_cloud = false;
std::mutex gui::gui_mutex;

int gui::config_access = 0;
char gui::config_code[128];
char gui::config_description[128];

char gui::config_user[128];
uint64_t gui::last_update_time = 0;

bool download_config_code = false;

cJSON* cloud_config_list = nullptr;

bool gui::opened = false;
bool open_button_pressed = false;

const char* last_weapon_name = nullptr;;

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

ImFont* gui_ui_font = nullptr;
ImFont* gui_small_font = nullptr;
ImFont* gui_icons_font = nullptr;

float g_last_dpi = 0.0f;

#pragma optimize( "2", off )

namespace stb {
	static unsigned int stb_decompress_length ( const unsigned char* input ) {
		return ( input [ 8 ] << 24 ) + ( input [ 9 ] << 16 ) + ( input [ 10 ] << 8 ) + input [ 11 ];
	}

	static unsigned char* stb__barrier_out_e, * stb__barrier_out_b;
	static const unsigned char* stb__barrier_in_b;
	static unsigned char* stb__dout;
	static void stb__match ( const unsigned char* data, unsigned int length ) {
		// INVERSE of memmove... write each byte before copying the next...
		IM_ASSERT ( stb__dout + length <= stb__barrier_out_e );
		if ( stb__dout + length > stb__barrier_out_e ) { stb__dout += length; return; }
		if ( data < stb__barrier_out_b ) { stb__dout = stb__barrier_out_e + 1; return; }
		while ( length-- ) *stb__dout++ = *data++;
	}

	static void stb__lit ( const unsigned char* data, unsigned int length ) {
		IM_ASSERT ( stb__dout + length <= stb__barrier_out_e );
		if ( stb__dout + length > stb__barrier_out_e ) { stb__dout += length; return; }
		if ( data < stb__barrier_in_b ) { stb__dout = stb__barrier_out_e + 1; return; }
		memcpy ( stb__dout, data, length );
		stb__dout += length;
	}

#define stb__in2(x)   ((i[x] << 8) + i[(x)+1])
#define stb__in3(x)   ((i[x] << 16) + stb__in2((x)+1))
#define stb__in4(x)   ((i[x] << 24) + stb__in3((x)+1))

	static const unsigned char* stb_decompress_token ( const unsigned char* i ) {
		if ( *i >= 0x20 ) { // use fewer if's for cases that expand small
			if ( *i >= 0x80 )       stb__match ( stb__dout - i [ 1 ] - 1, i [ 0 ] - 0x80 + 1 ), i += 2;
			else if ( *i >= 0x40 )  stb__match ( stb__dout - ( stb__in2 ( 0 ) - 0x4000 + 1 ), i [ 2 ] + 1 ), i += 3;
			else /* *i >= 0x20 */ stb__lit ( i + 1, i [ 0 ] - 0x20 + 1 ), i += 1 + ( i [ 0 ] - 0x20 + 1 );
		}
		else { // more ifs for cases that expand large, since overhead is amortized
			if ( *i >= 0x18 )       stb__match ( stb__dout - ( stb__in3 ( 0 ) - 0x180000 + 1 ), i [ 3 ] + 1 ), i += 4;
			else if ( *i >= 0x10 )  stb__match ( stb__dout - ( stb__in3 ( 0 ) - 0x100000 + 1 ), stb__in2 ( 3 ) + 1 ), i += 5;
			else if ( *i >= 0x08 )  stb__lit ( i + 2, stb__in2 ( 0 ) - 0x0800 + 1 ), i += 2 + ( stb__in2 ( 0 ) - 0x0800 + 1 );
			else if ( *i == 0x07 )  stb__lit ( i + 3, stb__in2 ( 1 ) + 1 ), i += 3 + ( stb__in2 ( 1 ) + 1 );
			else if ( *i == 0x06 )  stb__match ( stb__dout - ( stb__in3 ( 1 ) + 1 ), i [ 4 ] + 1 ), i += 5;
			else if ( *i == 0x04 )  stb__match ( stb__dout - ( stb__in3 ( 1 ) + 1 ), stb__in2 ( 4 ) + 1 ), i += 6;
		}
		return i;
	}

	static unsigned int stb_adler32 ( unsigned int adler32, unsigned char* buffer, unsigned int buflen ) {
		const unsigned long ADLER_MOD = 65521;
		unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
		unsigned long blocklen = buflen % 5552;

		unsigned long i;
		while ( buflen ) {
			for ( i = 0; i + 7 < blocklen; i += 8 ) {
				s1 += buffer [ 0 ], s2 += s1;
				s1 += buffer [ 1 ], s2 += s1;
				s1 += buffer [ 2 ], s2 += s1;
				s1 += buffer [ 3 ], s2 += s1;
				s1 += buffer [ 4 ], s2 += s1;
				s1 += buffer [ 5 ], s2 += s1;
				s1 += buffer [ 6 ], s2 += s1;
				s1 += buffer [ 7 ], s2 += s1;

				buffer += 8;
			}

			for ( ; i < blocklen; ++i )
				s1 += *buffer++, s2 += s1;

			s1 %= ADLER_MOD, s2 %= ADLER_MOD;
			buflen -= blocklen;
			blocklen = 5552;
		}
		return ( unsigned int ) ( s2 << 16 ) + ( unsigned int ) s1;
	}

	static unsigned int stb_decompress ( unsigned char* output, const unsigned char* i, unsigned int /*length*/ ) {
		if ( stb__in4 ( 0 ) != 0x57bC0000 ) return 0;
		if ( stb__in4 ( 4 ) != 0 )          return 0; // error! stream is > 4GB
		const unsigned int olen = stb_decompress_length ( i );
		stb__barrier_in_b = i;
		stb__barrier_out_e = output + olen;
		stb__barrier_out_b = output;
		i += 16;

		stb__dout = output;
		for ( ;;) {
			const unsigned char* old_i = i;
			i = stb_decompress_token ( i );
			if ( i == old_i ) {
				if ( *i == 0x05 && i [ 1 ] == 0xfa ) {
					IM_ASSERT ( stb__dout == output + olen );
					if ( stb__dout != output + olen ) return 0;
					if ( stb_adler32 ( 1, output, olen ) != ( unsigned int ) stb__in4 ( 2 ) )
						return 0;
					return olen;
				}
				else {
					IM_ASSERT ( 0 ); /* NOTREACHED */
					return 0;
				}
			}
			IM_ASSERT ( stb__dout <= output + olen );
			if ( stb__dout > output + olen )
				return 0;
		}
	}
}

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
	//IMGUI_CHECKVERSION ( );
	ImGui::CreateContext ( );
	ImGuiIO& io = ImGui::GetIO ( );
	//io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.MouseDrawCursor = false;

	// Setup Dear ImGui style
	ImGui::StyleColorsSesame ( );
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init ( LI_FN ( FindWindowA )( nullptr, _ ( "Counter-Strike: Global Offensive" ) ) );
	ImGui_ImplDX9_Init ( cs::i::dev );

	static const ImWchar custom_font_ranges_all [ ] = { 0x20, 0xFFFF, 0 };

	auto font_cfg = ImFontConfig ( );

	font_cfg.FontDataOwnedByAtlas = false;
	font_cfg.OversampleH = 2;
	font_cfg.PixelSnapH = false;
	//io.Fonts->Build ( );

	//_("C:\\Windows\\Fonts\\segoeui.ttf")
	//font_cfg.RasterizerMultiply = 1.2f;
	gui_ui_font = io.Fonts->AddFontFromMemoryCompressedTTF ( resources::roboto, sizeof( resources::roboto ), 14.0f * options::vars [ _ ( "gui.dpi" ) ].val.f, &font_cfg, custom_font_ranges_all );
	gui_ui_font->SetFallbackChar ( '?' );

	//font_cfg.RasterizerMultiply = 1.2f;
	gui_small_font = io.Fonts->AddFontFromMemoryCompressedTTF ( resources::roboto, sizeof ( resources::roboto ), 11.0f * options::vars [ _ ( "gui.dpi" ) ].val.f, &font_cfg, io.Fonts->GetGlyphRangesCyrillic ( ) );
	gui_small_font->SetFallbackChar ( '?' );

	//font_cfg.RasterizerMultiply = 1.2f;
	gui_icons_font = io.Fonts->AddFontFromMemoryTTF ( g::resources::sesame_icons, g::resources::sesame_icons_size, 28.0f * options::vars [ _ ( "gui.dpi" ) ].val.f, nullptr, io.Fonts->GetGlyphRangesDefault ( ) );
	gui_icons_font->SetFallbackChar ( '?' );

	//font_cfg.RasterizerMultiply = 1.2f;
	render::create_font ( resources::roboto, _ ( "dbg_font" ), 9.0f * options::vars [ _ ( "gui.dpi" ) ].val.f, nullptr, &font_cfg );
	//font_cfg.RasterizerMultiply = 1.2f;
	render::create_font ( resources::roboto, _ ( "esp_font" ), 9.0f * options::vars [ _ ( "gui.dpi" ) ].val.f, custom_font_ranges_all, &font_cfg );

	/* emojis */
	//font_cfg.MergeMode = true;
	//static const ImWchar segoe_ui_emoji_ranges [ ] = { 0x2002, 0x3299, 0 };
	//io.Fonts->AddFontFromFileTTF ( _ ( "‪C:\\Windows\\Fonts\\seguiemj.ttf" ), 12.0f * options::vars [ _ ( "gui.dpi" ) ].val.f, &font_cfg, segoe_ui_emoji_ranges );
	//font_cfg.MergeMode = false;

	//font_cfg.RasterizerMultiply = 1.2f;
	render::create_font ( resources::roboto, _ ( "indicator_font" ), 32.0f * options::vars [ _ ( "gui.dpi" ) ].val.f, nullptr, &font_cfg );
	//font_cfg.RasterizerMultiply = 1.2f;
	render::create_font ( resources::roboto, _ ( "watermark_font" ), 18.0f * options::vars [ _ ( "gui.dpi" ) ].val.f, nullptr, &font_cfg );

	ImGui::GetStyle ( ).AntiAliasedFill = ImGui::GetStyle ( ).AntiAliasedLines = true;
	
	ImGui::GetStyle ( ).ScaleAllSizes ( options::vars [ _ ( "gui.dpi" ) ].val.f );

	g_last_dpi = options::vars [ _ ( "gui.dpi" ) ].val.f;

	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
}

std::string create_cfg_dirs ( ) {
	char documents [ MAX_PATH ] { 0 };

	if ( SUCCEEDED ( LI_FN ( SHGetFolderPathA )( nullptr, N ( 5 ), nullptr, N ( 0 ), documents ) ) ) {
		LI_FN ( CreateDirectoryA )( ( std::string ( documents ) + _ ( "\\sesame" ) ).c_str ( ), nullptr );
		LI_FN ( CreateDirectoryA )( ( std::string ( documents ) + _ ( "\\sesame\\skins" ) ).c_str ( ), nullptr );
		LI_FN ( CreateDirectoryA )( ( std::string ( documents ) + _ ( "\\sesame\\configs" ) ).c_str ( ), nullptr );
		LI_FN ( CreateDirectoryA )( ( std::string ( documents ) + _ ( "\\sesame\\scripts" ) ).c_str ( ), nullptr );
	}

	return documents;
}

void gui::init( ) {
	//if ( !g::loader_data || !g::loader_data->avatar || !g::loader_data->avatar_sz )
	//	g_pfp_data = std::string( reinterpret_cast< const char* >( ses_pfp ), sizeof( ses_pfp ) );//networking::get(_("sesame.one/data/avatars/s/0/1.jpg"));
	//else
	//	g_pfp_data = std::string( g::loader_data->avatar, g::loader_data->avatar + g::loader_data->avatar_sz );

	/* initialize cheat config */
	options::init( );

	/* load default configs */
	auto cfg_dir = create_cfg_dirs ( );

	options::load ( options::skin_vars, cfg_dir.append ( _ ( "\\sesame\\skins\\skins.xml" ) ) );

	scale_dpi ( );

	gui_mutex.lock ( );
	load_cfg_list ( );
	gui_mutex.unlock ( );
}

char selected_config [ 128 ] = "default";
char selected_script [ 128 ] = "";
std::vector< std::string > configs { };
std::vector< std::string > scripts { };

void gui::load_cfg_list( ) {
	auto cfg_dir = create_cfg_dirs ( );

	auto sanitize_name = [ ] ( const std::string& dir ) {
		const auto dot = dir.find_last_of( _( "." ) );
		return dir.substr( N( 0 ), dot );
	};

	if ( !configs.empty() )
		configs.clear ( );

	for ( const auto& dir : std::filesystem::recursive_directory_iterator( cfg_dir + _( "\\sesame\\configs" ) ) ) {
		if ( dir.exists( ) && dir.is_regular_file( ) && dir.path( ).extension( ).string( ) == _( ".xml" ) ) {
			const auto sanitized = sanitize_name( dir.path( ).filename( ).string( ) );
			configs.push_back( sanitized );
		}
	}

	js_api::init ( );

	if ( !scripts.empty ( ) )
		scripts.clear ( );

	for ( const auto& dir : std::filesystem::recursive_directory_iterator ( cfg_dir + _ ( "\\sesame\\scripts" ) ) ) {
		if ( dir.exists ( ) && dir.is_regular_file ( ) && dir.path ( ).extension ( ).string ( ) == _ ( ".js" ) ) {
			const auto sanitized = sanitize_name ( dir.path ( ).filename ( ).string ( ) );
			scripts.push_back ( sanitized );
			js_api::load_script ( dir.path ( ).filename ( ).string ( ).c_str() );
		}
	}

	last_weapon_name = nullptr;
}

void gui::weapon_controls( const std::string& weapon_name ) {
	const auto ragebot_weapon = _( "ragebot." ) + weapon_name + _( "." );
	
	ImGui::BeginChildFrame ( ImGui::GetID ( "Weapon Settings" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
		ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Weapon Settings" ).x * 0.5f );
		ImGui::Text ( "Weapon Settings" );
		ImGui::Separator ( );

		ImGui::Checkbox( _( "Inherit Default" ), &options::vars [ ragebot_weapon + _( "inherit_default" ) ].val.b );
		ImGui::Checkbox( _( "Silent Aim" ), &options::vars [ ragebot_weapon + _( "silent" ) ].val.b );
		ImGui::Checkbox( _( "Auto Shoot" ), &options::vars [ ragebot_weapon + _( "auto_shoot" ) ].val.b );

		if ( weapon_name != _("pistol") )
			ImGui::Checkbox( _( "Auto Scope" ), &options::vars [ ragebot_weapon + _( "auto_scope" ) ].val.b );

		ImGui::Checkbox( _( "Auto Slow" ), &options::vars [ ragebot_weapon + _( "auto_slow" ) ].val.b );
		ImGui::PushItemWidth ( -1.0f );
		ImGui::Checkbox ( _ ( "Tickbase Smooth Recharge" ), &options::vars [ ragebot_weapon + _ ( "dt_smooth_recharge" ) ].val.b );
		ImGui::SliderInt ( _ ( "Tickbase Recharge Delay" ), &options::vars [ ragebot_weapon + _ ( "dt_recharge_delay" ) ].val.i, 0, 1000, _ ( "%d ms" ) );
		ImGui::SliderFloat( _( "Minimum Damage" ), &options::vars [ ragebot_weapon + _( "min_dmg" ) ].val.f, 0.0f, 150.0f, ( options::vars [ ragebot_weapon + _( "min_dmg" ) ].val.f > 100.0f ? ( _( "HP + " ) + std::to_string( static_cast< int > ( options::vars [ ragebot_weapon + _( "min_dmg" ) ].val.f - 100.0f ) ) + _( " HP" ) ) : ( std::to_string( static_cast< int > ( options::vars [ ragebot_weapon + _( "min_dmg" ) ].val.f ) ) + _( " HP" ) ) ).c_str( ) );
		ImGui::SliderFloat ( _ ( "Overrided Minimum Damage" ), &options::vars [ ragebot_weapon + _ ( "min_dmg_override" ) ].val.f, 0.0f, 150.0f, ( options::vars [ ragebot_weapon + _ ( "min_dmg_override" ) ].val.f > 100.0f ? ( _ ( "HP + " ) + std::to_string ( static_cast< int > ( options::vars [ ragebot_weapon + _ ( "min_dmg_override" ) ].val.f - 100.0f ) ) + _ ( " HP" ) ) : ( std::to_string ( static_cast< int > ( options::vars [ ragebot_weapon + _ ( "min_dmg_override" ) ].val.f ) ) + _ ( " HP" ) ) ).c_str ( ) );
		//ImGui::SliderFloat( _( "Damage Accuracy" ), &options::vars [ ragebot_weapon + _( "dmg_accuracy" ) ].val.f, 0.0f, 100.0f, _( "%.1f%%" ) );
		ImGui::SliderFloat( _( "Hit Chance" ), &options::vars [ ragebot_weapon + _( "hit_chance" ) ].val.f, 0.0f, 100.0f, _( "%.1f%%" ) );
		ImGui::SliderFloat( _( "Tickbase Hit Chance" ), &options::vars [ ragebot_weapon + _( "dt_hit_chance" ) ].val.f, 0.0f, 100.0f, _( "%.1f%%" ) );
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
		ImGui::SliderFloat( _( "Head Point Scale" ), &options::vars [ ragebot_weapon + _( "head_pointscale" ) ].val.f, 0.0f, 100.0f, _( "%.1f%%" ) );
		ImGui::SliderFloat( _( "Body Point Scale" ), &options::vars [ ragebot_weapon + _( "body_pointscale" ) ].val.f, 0.0f, 100.0f, _( "%.1f%%" ) );
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
		static std::vector<const char*> pitches { "None", "Down", "Up", "Zero" };
		ImGui::PushItemWidth ( -1.0f );
		ImGui::Combo( _( "Base Pitch" ), &options::vars [ antiaim_config + _( "pitch" ) ].val.i, pitches.data(), pitches.size() );
		ImGui::PopItemWidth ( );
		ImGui::PushItemWidth ( -1.0f );
		ImGui::SliderFloat( _( "Yaw Offset" ), &options::vars [ antiaim_config + _( "yaw_offset" ) ].val.f, -180.0f, 180.0f, _( "%.1f°" ) );
		ImGui::PopItemWidth ( );

		static std::vector<const char*> base_yaws { "Relative", "Absolute", "At Target", "Auto Direction" };
		ImGui::PushItemWidth ( -1.0f );
		ImGui::Combo ( _( "Base Yaw" ), &options::vars [ antiaim_config + _( "base_yaw" ) ].val.i, base_yaws.data(), base_yaws.size());
		ImGui::SliderFloat( _( "Auto Direction Amount" ), &options::vars [ antiaim_config + _( "auto_direction_amount" ) ].val.f, -180.0f, 180.0f, _( "%.1f°" ) );
		ImGui::SliderFloat( _( "Auto Direction Range" ), &options::vars [ antiaim_config + _( "auto_direction_range" ) ].val.f, 0.0f, 100.0f, _( "%.1f units" ) );
		ImGui::SliderFloat( _( "Jitter Range" ), &options::vars [ antiaim_config + _( "jitter_range" ) ].val.f, -180.0f, 180.0f, _( "%.1f°" ) );
		ImGui::SliderFloat( _( "Rotation Range" ), &options::vars [ antiaim_config + _( "rotation_range" ) ].val.f, -180.0f, 180.0f, _( "%.1f°" ) );
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
		ImGui::SliderFloat( _( "Desync Range" ), &options::vars [ antiaim_config + _( "desync_range" ) ].val.f, 0.0f, 60.0f, _( "%.1f°" ) );
		ImGui::SliderFloat( _( "Desync Range Inverted" ), &options::vars [ antiaim_config + _( "desync_range_inverted" ) ].val.f, 0.0f, 60.0f,  _( "%.1f°" ) );
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
			"Fakeduck Flag",
			"Reloading Flag",
			"Fatal Flag",
			"Zoom Flag",
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
			"Fakeduck Flag",
			"Reloading Flag",
			"Fatal Flag",
			"Zoom Flag",
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
			"Fakeduck Flag",
			"Reloading Flag",
			"Fatal Flag",
			"Zoom Flag",
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
		ImGui::Combo ( _ ( "Fakeduck Flag Location" ), &options::vars [ visuals_config + _ ( "fakeduck_flag_location" ) ].val.i, element_locations.data ( ), element_locations.size ( ) );
		ImGui::Combo ( _ ( "Reloading Flag Location" ), &options::vars [ visuals_config + _ ( "reloading_flag_location" ) ].val.i, element_locations.data ( ), element_locations.size ( ) );
		ImGui::Combo ( _ ( "Fatal Flag Location" ), &options::vars [ visuals_config + _ ( "fatal_flag_location" ) ].val.i, element_locations.data ( ), element_locations.size ( ) );
		ImGui::Combo ( _ ( "Zoom Flag Location" ), &options::vars [ visuals_config + _ ( "zoom_flag_location" ) ].val.i, element_locations.data ( ), element_locations.size ( ) );
		ImGui::SliderFloat( _( "Chams Reflectivity" ), &options::vars [ visuals_config + _( "reflectivity" ) ].val.f, 0.0f, 100.0f, _( "%.1f%%" ) );
		ImGui::SliderFloat( _( "Chams Phong" ), &options::vars [ visuals_config + _( "phong" ) ].val.f, 0.0f, 100.0f, _( "%.1f%%" ) );
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
		ImGui::ColorEdit4 ( _ ( "Fakeduck Flag Color" ), ( float* ) &options::vars [ visuals_config + _ ( "fakeduck_color" ) ].val.c );
		ImGui::ColorEdit4 ( _ ( "Reloading Flag Color" ), ( float* ) &options::vars [ visuals_config + _ ( "reloading_color" ) ].val.c );
		ImGui::ColorEdit4 ( _ ( "Fatal Flag Color" ), ( float* ) &options::vars [ visuals_config + _ ( "fatal_color" ) ].val.c );
		ImGui::ColorEdit4 ( _ ( "Zoom Flag Color" ), ( float* ) &options::vars [ visuals_config + _ ( "zoom_color" ) ].val.c );

		ImGui::EndChildFrame( );
	}
}

static bool gui_paintkit_getter ( void* data, int idx, const char** out_text ) {
	const auto items = reinterpret_cast< const features::skinchanger::paint_kit* >( data );

	if ( out_text )
		*out_text = items [ idx ].name.c_str ( );

	return true;
}

void gui::draw( ) {
	/* HANDLE DPI */
	//sesui::globals::dpi = options::vars [ _( "gui.dpi" ) ].val.f;

	/* set animation speed */
	static auto& gui_anim_speed = options::vars [ _ ( "gui.animation_speed" ) ].val.i;
	gui_anim_multiplier = static_cast< float >( gui_anim_speed ) / 100.0f;

	/* toggle menu */
	if ( ImGui::IsKeyReleased ( VK_INSERT ) ) {
		opened = !opened;

		/* reset animations on close */
		if ( !opened )
			ImGui::custom::reset_anims ( );
	}

	open_button_pressed = utils::key_state( VK_INSERT );
	
	watermark::draw( );
	keybinds::draw( );

	if ( opened ) {
		ImGui::PushFont ( gui_ui_font );

		if ( ImGui::custom::Begin (_( SESAME_VERSION), &opened, gui_small_font ) ) {
			/* main menu objects */
			if ( ImGui::custom::BeginTabs ( &current_tab_idx, gui_icons_font ) ) {
				ImGui::custom::AddTab ( _ ( "A" ) );
				ImGui::custom::AddTab ( _ ( "B" ) );
				ImGui::custom::AddTab ( _ ( "C" ) );
				ImGui::custom::AddTab ( _ ( "D" ) );
				ImGui::custom::AddTab ( _ ( "E" ) );
				ImGui::custom::AddTab ( _ ( "F" ) );

				ImGui::custom::EndTabs ( );
			}

			/* confirm config save (overwrite) popup */
			ImGui::SetNextWindowPos ( ImVec2 ( ImGui::GetWindowPos ( ).x + ImGui::GetWindowSize ( ).x * 0.5f, ImGui::GetWindowPos ( ).y + ImGui::GetWindowSize ( ).y * 0.5f ), ImGuiCond_Always, ImVec2 ( 0.5f, 0.5f ) );

			if ( ImGui::BeginPopupModal ( _("Save Config##popup"), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings ) ) {
				ImGui::TextColored ( ImVec4 ( 1.0f, 0.1f, 0.1f, 1.0f ), _("There already is a config with the same name in this location.\nAre you sure you want to overwrite the config?" ));

				if ( ImGui::Button ( _("Confirm"), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ) ) ) {
					auto cfg_dir = create_cfg_dirs ( );
					const auto file = cfg_dir.append ( _ ( "\\sesame\\configs\\" ) ).append ( selected_config ).append ( _ ( ".xml" ) );

					options::save ( options::vars, file );

					gui_mutex.lock ( );
					load_cfg_list ( );
					gui_mutex.unlock ( );

					cs::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );

					ImGui::CloseCurrentPopup ( );
				}

				ImGui::SameLine ( );

				if ( ImGui::Button ( _("Cancel"), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ) ) ) {
					ImGui::CloseCurrentPopup ( );
				}

				ImGui::EndPopup ( );
			}

			ImGui::SetNextWindowPos ( ImVec2 ( ImGui::GetWindowPos ( ).x + ImGui::GetWindowSize ( ).x * 0.5f, ImGui::GetWindowPos ( ).y + ImGui::GetWindowSize ( ).y * 0.5f ), ImGuiCond_Always, ImVec2 ( 0.5f, 0.5f ) );

			if ( ImGui::BeginPopupModal ( _ ( "Delete Config##popup" ), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings ) ) {
				ImGui::TextColored ( ImVec4 ( 1.0f, 0.1f, 0.1f, 1.0f ), _ ( "Are you sure you want to delete the config?" ) );

				if ( ImGui::Button ( _ ( "Confirm" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ) ) ) {
					auto cfg_dir = create_cfg_dirs ( );

					std::remove ( ( cfg_dir + _ ( "\\sesame\\configs\\" ) + selected_config + _ ( ".xml" ) ).c_str ( ) );

					gui_mutex.lock ( );
					load_cfg_list ( );
					gui_mutex.unlock ( );

					cs::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );

					ImGui::CloseCurrentPopup ( );
				}

				ImGui::SameLine ( );

				if ( ImGui::Button ( _("Cancel"), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ) ) ) {
					ImGui::CloseCurrentPopup ( );
				}

				ImGui::EndPopup ( );
			}

			ImGui::SetNextWindowPos ( ImVec2 ( ImGui::GetWindowPos ( ).x + ImGui::GetWindowSize ( ).x * 0.5f, ImGui::GetWindowPos ( ).y + ImGui::GetWindowSize ( ).y * 0.5f ), ImGuiCond_Always, ImVec2 ( 0.5f, 0.5f ) );

			if ( ImGui::BeginPopupModal ( _ ( "Delete Script##popup" ), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings ) ) {
				ImGui::TextColored ( ImVec4 ( 1.0f, 0.1f, 0.1f, 1.0f ), _ ( "Are you sure you want to delete the script?" ) );

				if ( ImGui::Button ( _ ( "Confirm" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ) ) ) {
					auto cfg_dir = create_cfg_dirs ( );
					const auto file = cfg_dir.append ( _ ( "\\sesame\\scripts\\" ) ).append ( selected_script ).append ( _ ( ".js" ) );

					std::remove ( ( cfg_dir + _ ( "\\sesame\\scripts\\" ) + selected_script + _ ( ".js" ) ).c_str ( ) );

					gui_mutex.lock ( );
					load_cfg_list ( );
					gui_mutex.unlock ( );

					cs::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );

					ImGui::CloseCurrentPopup ( );
				}

				ImGui::SameLine ( );

				if ( ImGui::Button ( _("Cancel"), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ) ) ) {
					ImGui::CloseCurrentPopup ( );
				}

				ImGui::EndPopup ( );
			}

			bool open_save_modal = false;
			bool open_delete_modal = false;
			bool open_delete_script_modal = false;

			switch ( current_tab_idx ) {
				case tab_legit: {
					ImGui::custom::AddSubtab ( _("General"), _("General legitbot and accuracy settings"), [ & ] ( ) {
					} );

					ImGui::custom::AddSubtab ( _("Default"), _("Default settings used for unconfigured weapons"), [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( _("General Settings") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( _("General Settings") ).x * 0.5f );
							ImGui::Text ( _("General Settings") );
							ImGui::Separator ( );

							static std::vector<const char*> assist_type { "None", "Legit" ,  "Rage" };
							ImGui::PushItemWidth ( -1.0f );
							ImGui::Combo ( _ ( "Assistance Type" ), &options::vars [ _ ( "global.assistance_type" ) ].val.i, assist_type.data ( ), assist_type.size ( ) );
							ImGui::PopItemWidth ( );

							ImGui::EndChildFrame ( );
						}

						ImGui::SameLine ( );

						ImGui::BeginChildFrame ( ImGui::GetID ( _("Triggerbot" )), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( _("Triggerbot") ).x * 0.5f );
							ImGui::Text ( _("Triggerbot" ));
							ImGui::Separator ( );

							ImGui::Checkbox ( _ ( "Triggerbot" ), &options::vars [ _ ( "legitbot.triggerbot" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::Keybind ( _ ( "##Triggerbot Key" ), &options::vars [ _ ( "legitbot.triggerbot_key" ) ].val.i, &options::vars [ _ ( "legitbot.triggerbot_key_mode" ) ].val.i, ImVec2(-1.0f, 0.0f) );

							static std::vector<const char*> hitboxes {
								"Head",
								"Neck",
								"Chest",
								"Pelvis",
								"Arms",
							};

							ImGui::MultiCombo ( _ ( "Hitboxes" ), options::vars [ _ ( "legitbot.triggerbot_hitboxes" ) ].val.l, hitboxes.data ( ), hitboxes.size ( ) );

							ImGui::EndChildFrame ( );
						}
					} );

					ImGui::custom::AddSubtab ( _("Pistol"),_( "Pistol class configuration"), [ & ] ( ) {
					} );
					ImGui::custom::AddSubtab ( _("Revolver"),_( "Revolver class configuration"), [ & ] ( ) {
					} );
					ImGui::custom::AddSubtab ( _("Rifle"), _("Rifle, SMG, and shotgun class configuration"), [ & ] ( ) {
					} );
					ImGui::custom::AddSubtab ( _("AWP"),_( "AWP class configuration"), [ & ] ( ) {
					} );
					ImGui::custom::AddSubtab ( _("Auto"),_( "Autosniper class configuration"), [ & ] ( ) {
					} );
					ImGui::custom::AddSubtab ( _("Scout"), _("Scout class configuration"), [ & ] ( ) {
					} );
				} break;
				case tab_rage: {
					ImGui::custom::AddSubtab ( _("General"), _("General ragebot and accuracy settings"), [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( _("General Settings") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( _("General Settings" )).x * 0.5f );
							ImGui::Text ( _("General Settings" ));
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
							ImGui::Checkbox ( _ ( "Shift Tickbase" ), &options::vars [ _ ( "ragebot.dt_enabled" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::Keybind ( _ ( "Tickbase Shift Key" ), &options::vars [ _ ( "ragebot.dt_key" ) ].val.i, &options::vars [ _ ( "ragebot.dt_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Checkbox ( _ ( "Teleport On Shift" ), &options::vars [ _ ( "ragebot.dt_teleport" ) ].val.b );
							ImGui::PushItemWidth ( -1.0f );
							ImGui::SliderInt ( _ ( "Tickbase Shift Amount" ), &options::vars [ _ ( "ragebot.dt_ticks" ) ].val.i, 0, 16, _ ( "%d ticks" ) );
							ImGui::PopItemWidth ( );
							ImGui::Checkbox ( _ ( "Extended Lagcomp" ), &options::vars [ _ ( "ragebot.extended_lagcomp_enabled" ) ].val.b );
							ImGui::PushItemWidth ( -1.0f );
							ImGui::SliderInt ( _ ( "Extended Lagcomp Time" ), &options::vars [ _ ( "ragebot.extended_lagcomp_ms" ) ].val.i, 0, 200, _ ( "+%dms" ) );
							ImGui::PopItemWidth ( );

							ImGui::EndChildFrame ( );
						}

						ImGui::SameLine ( );

						ImGui::BeginChildFrame ( ImGui::GetID ( _("Accuracy") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize (_( "Accuracy") ).x * 0.5f );
							ImGui::Text (_( "Accuracy") );
							ImGui::Separator ( );

							ImGui::Checkbox ( _ ( "Fix Fakelag" ), &options::vars [ _ ( "ragebot.fix_fakelag" ) ].val.b );
							ImGui::Checkbox ( _ ( "Resolve Desync" ), &options::vars [ _ ( "ragebot.resolve_desync" ) ].val.b );
							ImGui::Checkbox ( _ ( "Safe Point" ), &options::vars [ _ ( "ragebot.safe_point" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::Keybind ( _ ( "##Safe Point Key" ), &options::vars [ _ ( "ragebot.safe_point_key" ) ].val.i, &options::vars [ _ ( "ragebot.safe_point_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Checkbox ( _ ( "Auto Revolver" ), &options::vars [ _ ( "ragebot.auto_revolver" ) ].val.b );
							ImGui::Keybind ( _ ( "Damage Override Key" ), &options::vars [ _ ( "ragebot.min_dmg_override_key" ) ].val.i, &options::vars [ _ ( "ragebot.min_dmg_override_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							
							ImGui::EndChildFrame ( );
						}
					} );
					ImGui::custom::AddSubtab ( _("Default"),_( "Default settings used for unconfigured weapons"), [ & ] ( ) {
						weapon_controls ( _ ( "default" ) );
					} );
					ImGui::custom::AddSubtab ( _("Pistol"), _("Pistol class configuration"), [ & ] ( ) {
						weapon_controls ( _ ( "pistol" ) );
					} );
					ImGui::custom::AddSubtab ( _("Revolver"), _("Revolver class configuration"), [ & ] ( ) {
						weapon_controls ( _ ( "revolver" ) );
					} );
					ImGui::custom::AddSubtab ( _("Rifle"),_( "Rifle, SMG, and shotgun class configuration"), [ & ] ( ) {
						weapon_controls ( _ ( "rifle" ) );
					} );
					ImGui::custom::AddSubtab ( _("AWP"), _("AWP class configuration"), [ & ] ( ) {
						weapon_controls ( _ ( "awp" ) );
					} );
					ImGui::custom::AddSubtab (_( "Auto"), _("Autosniper class configuration"), [ & ] ( ) {
						weapon_controls ( _ ( "auto" ) );
					} );
					ImGui::custom::AddSubtab (_( "Scout"), _("Scout class configuration"), [ & ] ( ) {
						weapon_controls ( _ ( "scout" ) );
					} );
				} break;
				case tab_antiaim: {
					ImGui::custom::AddSubtab ( _("Air"), _("In air antiaim settings"), [ & ] ( ) {
						antiaim_controls ( _ ( "air" ) );
					} );
					ImGui::custom::AddSubtab ( _("Moving"),_( "Moving antiaim settings"), [ & ] ( ) {
						antiaim_controls ( _ ( "moving" ) );
					} );
					ImGui::custom::AddSubtab (_( "Slow Walk"), _("Slow walk antiaim settings"), [ & ] ( ) {
						antiaim_controls ( _ ( "slow_walk" ) );
					} );
					ImGui::custom::AddSubtab ( _("Standing"), _("Standing antiaim settings"), [ & ] ( ) {
						antiaim_controls ( _ ( "standing" ) );
					} );
					ImGui::custom::AddSubtab ( _("Other"), _("Fakelag, manual aa, and other antiaim features"), [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID (_( "General") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize (_( "General" )).x * 0.5f );
							ImGui::Text (_( "General") );
							ImGui::Separator ( );

							ImGui::PushItemWidth ( -1.0f );
							static std::vector<const char*> leg_movement { "Normal", "Never Slide", "Always Slide", "Jitter" };
							ImGui::Combo( _( "Leg Movement" ) , &options::vars[ _( "antiaim.leg_movement" ) ].val.i, leg_movement.data(), leg_movement.size() );
							ImGui::SliderFloat ( _ ( "Slow Walk Speed" ), &options::vars [ _ ( "antiaim.slow_walk_speed" ) ].val.f, 0.0f, 100.0f, _ ( "%.1f%%" ) );
							ImGui::PopItemWidth ( );
							ImGui::Keybind ( _ ( "Slow Walk Key" ), &options::vars [ _ ( "antiaim.slow_walk_key" ) ].val.i, &options::vars [ _ ( "antiaim.slow_walk_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Checkbox ( _ ( "Slide on Slow Walk" ), &options::vars [ _ ( "antiaim.fakewalk" ) ].val.b );
							ImGui::Checkbox ( _ ( "Fake Duck" ), &options::vars [ _ ( "antiaim.fakeduck" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::Keybind ( _ ( "Fake Duck Key" ), &options::vars [ _ ( "antiaim.fakeduck_key" ) ].val.i, &options::vars [ _ ( "antiaim.fakeduck_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							//static std::vector<const char*> fake_duck_modes { "Offensive" , "Defensive" };
							//
							//ImGui::PushItemWidth ( -1.0f );
							//ImGui::Combo ( _ ( "Fake Duck Mode" ), &options::vars [ _ ( "antiaim.fakeduck_mode" ) ].val.i, fake_duck_modes.data ( ), fake_duck_modes.size ( ) );
							//ImGui::PopItemWidth ( );
							//ImGui::Checkbox ( _ ( "Break Backtrack" ), &options::vars [ _ ( "antiaim.break_backtrack" ) ].val.b );
							//ImGui::SameLine ( );
							//ImGui::Keybind ( _ ( "Break Backtrack Key" ), &options::vars [ _ ( "antiaim.break_backtrack_key" ) ].val.i, &options::vars [ _ ( "antiaim.break_backtrack_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Keybind ( _ ( "Manual Left Key" ), &options::vars [ _ ( "antiaim.manual_left_key" ) ].val.i, &options::vars [ _ ( "antiaim.manual_left_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Keybind ( _ ( "Manual Right Key" ), &options::vars [ _ ( "antiaim.manual_right_key" ) ].val.i, &options::vars [ _ ( "antiaim.manual_right_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Keybind ( _ ( "Manual Back Key" ), &options::vars [ _ ( "antiaim.manual_back_key" ) ].val.i, &options::vars [ _ ( "antiaim.manual_back_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Keybind ( _ ( "Desync Inverter Key" ), &options::vars [ _ ( "antiaim.desync_invert_key" ) ].val.i, &options::vars [ _ ( "antiaim.desync_invert_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );

							ImGui::EndChildFrame ( );
						}

						ImGui::SameLine ( );

						ImGui::BeginChildFrame ( ImGui::GetID ( _ ( "Fakelag" ) ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( _ ( "Fakelag" ) ).x * 0.5f );
							ImGui::Text ( _ ( "Fakelag" ) );
							ImGui::Separator ( );

							ImGui::Checkbox ( _ ( "Enable Fakelag" ), &options::vars [ _ ( "antiaim.fakelag" ) ].val.b );
							ImGui::PushItemWidth ( -1.0f );
							ImGui::SliderInt ( _ ( "Fakelag Limit" ), &options::vars [ _ ( "antiaim.fakelag_limit" ) ].val.i, 0, 16, _ ( "%d ticks" ) );
							ImGui::SliderInt ( _ ( "Fakelag Jitter" ), &options::vars [ _ ( "antiaim.fakelag_jitter" ) ].val.i, 0, 100, _ ( "%d%%" ) );
							static std::vector<const char*> fakelag_triggers { "In Air", "On Peek", "On Shot", "On Land", "Reloading", "Weapon Switch", "Velocity Change", "Break Lagcomp" };
							ImGui::MultiCombo ( _ ( "Fakelag Triggers" ), options::vars [ _ ( "antiaim.fakelag_triggers" ) ].val.l, fakelag_triggers.data ( ), fakelag_triggers.size ( ) );
							ImGui::SliderInt ( _ ( "Fakelag Trigger Limit" ), &options::vars [ _ ( "antiaim.fakelag_trigger_limit" ) ].val.i, 0, 16, _ ( "%d ticks" ) );
							ImGui::PopItemWidth ( );

							ImGui::EndChildFrame ( );
						}
					} );
				} break;
				case tab_visuals: {
					ImGui::custom::AddSubtab ( _("Local Player"), _("Visuals used on local player"), [ & ] ( ) {
						player_visuals_controls ( _ ( "local" ) );
					} );
					ImGui::custom::AddSubtab ( _("Enemies"),_( "Visuals used on filtered enemies"), [ & ] ( ) {
						player_visuals_controls ( _ ( "enemies" ) );
					} );
					ImGui::custom::AddSubtab ( _("Teammates"), _("Visuals used on filtered teammates"), [ & ] ( ) {
						player_visuals_controls ( _ ( "teammates" ) );
					} );
					ImGui::custom::AddSubtab (_( "Other"), _("Other visual options"), [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID (_( "World") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize (_( "World") ).x * 0.5f );
							ImGui::Text ( _("World" ));
							ImGui::Separator ( );

							ImGui::Checkbox ( _ ( "Fog" ), &options::vars [ _ ( "visuals.other.fog" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Fog Color" ), ( float* ) &options::vars [ _ ( "visuals.other.fog_color" ) ].val.c );
							ImGui::PushItemWidth ( -1.0f );
							ImGui::SliderFloat ( _ ( "Fog Distance" ), &options::vars [ _ ( "visuals.other.fog_distance" ) ].val.f, 0.0f, 6000.0f, _ ( "%.1f units" ) );
							ImGui::SliderFloat ( _ ( "Fog Density" ), &options::vars [ _ ( "visuals.other.fog_density" ) ].val.f, 0.0f, 1.0f, _ ( "x%.1f" ) );
							ImGui::PopItemWidth ( );
							ImGui::Checkbox ( _ ( "Bloom" ), &options::vars [ _ ( "visuals.other.bloom" ) ].val.b );
							ImGui::PushItemWidth ( -1.0f );
							ImGui::SliderFloat ( _ ( "Bloom Scale" ), &options::vars [ _ ( "visuals.other.bloom_scale" ) ].val.f, 0.0f, 10.0f, _ ( "x%.1f" ) );
							ImGui::SliderFloat ( _ ( "Bloom Exponent" ), &options::vars [ _ ( "visuals.other.bloom_exponent" ) ].val.f, 0.0f, 10.0f,  _ ( "%.1f" ) );
							ImGui::SliderFloat ( _ ( "Bloom Saturation" ), &options::vars [ _ ( "visuals.other.bloom_saturation" ) ].val.f, 0.0f, 10.0f,  _ ( "x%.1f" ) );
							ImGui::PopItemWidth ( );

							ImGui::Checkbox ( _ ( "Bomb ESP" ), &options::vars [ _ ( "visuals.other.bomb_esp" ) ].val.b );
							ImGui::Checkbox ( _ ( "Bomb Timer" ), &options::vars [ _ ( "visuals.other.bomb_timer" ) ].val.b );
							ImGui::Checkbox ( _ ( "Bullet Tracers" ), &options::vars [ _ ( "visuals.other.bullet_tracers" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Bullet Tracer Color" ), ( float* ) &options::vars [ _ ( "visuals.other.bullet_tracer_color" ) ].val.c );
							ImGui::Checkbox ( _ ( "Client Bullet Impacts" ), &options::vars [ _ ( "visuals.other.bullet_impacts_client" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Client Bullet Impacts Color" ), ( float* ) &options::vars [ _ ( "visuals.other.bullet_impacts_client_color" ) ].val.c );
							ImGui::Checkbox ( _ ( "Server Bullet Impacts" ), &options::vars [ _ ( "visuals.other.bullet_impacts_server" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Server Bullet Impacts Color" ), ( float* ) &options::vars [ _ ( "visuals.other.bullet_impacts_server_color" ) ].val.c );
							ImGui::Checkbox ( _ ( "Damage Indicator" ), &options::vars [ _ ( "visuals.other.damage_indicator" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Damage Indicator Color" ), ( float* ) &options::vars [ _ ( "visuals.other.damage_indicator_color" ) ].val.c );
							ImGui::Checkbox ( _ ( "Player Hits" ), &options::vars [ _ ( "visuals.other.player_hits" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Player Hits Color" ), ( float* ) &options::vars [ _ ( "visuals.other.player_hits_color" ) ].val.c );
							ImGui::Checkbox ( _ ( "Grenade Trajectories" ), &options::vars [ _ ( "visuals.other.grenade_trajectories" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Grenade Trajectories Color" ), ( float* ) &options::vars [ _ ( "visuals.other.grenade_trajectory_color" ) ].val.c );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Grenade Trajectories Hit Color" ), ( float* ) &options::vars [ _ ( "visuals.other.grenade_trajectory_color_hit" ) ].val.c );
							ImGui::Checkbox ( _ ( "Grenade Bounces" ), &options::vars [ _ ( "visuals.other.grenade_bounces" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Grenade Bounces Color" ), ( float* ) &options::vars [ _ ( "visuals.other.grenade_bounce_color" ) ].val.c );
							ImGui::Checkbox ( _ ( "Grenade Blast Radii" ), &options::vars [ _ ( "visuals.other.grenade_blast_radii" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::ColorEdit4 ( _ ( "##Grenade Blast Radii Color" ), ( float* ) &options::vars [ _ ( "visuals.other.grenade_radii_color" ) ].val.c );

							ImGui::EndChildFrame ( );
						}

						ImGui::SameLine ( );

						ImGui::BeginChildFrame ( ImGui::GetID ( _("Other") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize (_( "Other" )).x * 0.5f );
							ImGui::Text ( _("Other") );
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
								//"Landing Bob" ,
							};

							ImGui::MultiCombo ( _ ( "Removals" ), options::vars [ _ ( "visuals.other.removals" ) ].val.l, removals.data ( ), removals.size ( ) );
							ImGui::PushItemWidth ( -1.0f );
							ImGui::Checkbox( _( "Modify Model Blend" ) , &options::vars[ _( "visuals.other.blend" ) ].val.b );
							ImGui::SliderFloat( _( "Opacity" ) , &options::vars[ _( "visuals.other.blend_opacity" ) ].val.f , 0.f , 255.f , _( "%.1f" ) );
							ImGui::SliderFloat ( _ ( "FOV" ), &options::vars [ _ ( "visuals.other.fov" ) ].val.f, 0.0f, 180.0f, ( char* ) _ ( "%.1f°" ) );
							ImGui::SliderFloat ( _ ( "Viewmodel FOV" ), &options::vars [ _ ( "visuals.other.viewmodel_fov" ) ].val.f, 0.0f, 180.0f, _ ( "%.1f°" ) );
							ImGui::SliderFloat ( _ ( "Viewmodel Offset X" ), &options::vars [ _ ( "visuals.other.viewmodel_offset_x" ) ].val.f, -10.0f, 10.0f, _ ( "%.1f units" ) );
							ImGui::SliderFloat ( _ ( "Viewmodel Offset Y" ), &options::vars [ _ ( "visuals.other.viewmodel_offset_y" ) ].val.f, -10.0f, 10.0f, _ ( "%.1f units" ) );
							ImGui::SliderFloat ( _ ( "Viewmodel Offset Z" ), &options::vars [ _ ( "visuals.other.viewmodel_offset_z" ) ].val.f, -10.0f, 10.0f, _ ( "%.1f units" ) );
							ImGui::SliderFloat ( _ ( "Aspect Ratio" ), &options::vars [ _ ( "visuals.other.aspect_ratio" ) ].val.f, 0.1f, 2.0f );
							ImGui::PopItemWidth ( );

							static std::vector<const char*> logs {
								"Ragebot",
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
							ImGui::SliderFloat ( _ ( "Offscreen ESP Distance" ), &options::vars [ _ ( "visuals.other.offscreen_esp_distance" ) ].val.f, 0.0f, 100.0f, _ ( "%.1f%%" ) );
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
							static std::vector<const char*> hitsounds { "None", "Arena Switch", "Fall Pain" , "Bolt Back" , "Grenade" , "Sesame"  };

							ImGui::PushItemWidth ( -1.0f );
							ImGui::Combo ( _ ( "Hit Sound" ), &options::vars [ _ ( "visuals.other.hit_sound" ) ].val.i, hitsounds.data ( ), hitsounds.size ( ) );
							ImGui::PopItemWidth ( );

							ImGui::EndChildFrame ( );
						}
					} );
				} break;
				case tab_skins: {
					ImGui::custom::AddSubtab ( _ ( "Skin Changer" ), _ ( "Change your weapon or glove skins!" ), [ & ] ( ) {
						/*
						-- Model Options --
						x Override Knife
						[M9 Bayonet v]

						-- Glove Options --
						x Override Gloves
						[Driver v]
						[Listbox w/ Search for Glove Skins]

						-- Skin Options --
						x Enabled
						x StatTrak
						x Filter by Weapon
						Quality 0-100
						Seed 1-1000 or TextInput numbers only
						[Listbox w/ Search for Skins]
						*/
						ImGui::BeginChildFrame ( ImGui::GetID ( _ ( "Knives" ) ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, ImGui::GetContentRegionAvail( ).y * 0.5f - ImGui::GetStyle ( ).FramePadding.y ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( _ ( "Knives" ) ).x * 0.5f );
							ImGui::Text ( _ ( "Knives" ) );
							ImGui::Separator ( );

							ImGui::PushItemWidth ( -1.0f );
							if(ImGui::Checkbox ( _ ( "Override Knife" ), &options::skin_vars [ _ ( "skins.skin.override_knife" ) ].val.b )) features::skinchanger::skin_changed = true;
							if ( ImGui::Combo ( _ ( "##Knife Models" ), &options::skin_vars [ _ ( "skins.skin.knife" ) ].val.i, features::skinchanger::knife_names.data ( ), features::skinchanger::knife_names.size ( ) )) features::skinchanger::skin_changed = true;
							ImGui::PopItemWidth ( );

							ImGui::EndChildFrame ( );
						}

						const auto knives_group_end = ImGui::GetCursorPosY ( );

						ImGui::SameLine ( );

						ImGui::BeginChildFrame ( ImGui::GetID ( _ ( "Weapon Skin" ) ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( _ ( "Weapon Skin" ) ).x * 0.5f );
							ImGui::Text ( _ ( "Weapon Skin" ) );
							ImGui::Separator ( );

							ImGui::PushItemWidth ( -1.0f );
							if ( ImGui::Checkbox ( _ ( "Enabled" ), &options::skin_vars [ _ ( "skins.skin.override_weapon" ) ].val.b ) ) features::skinchanger::skin_changed = true;

							/* config per skin */
							if ( g::local && g::local->alive() && g::local->weapon ( ) && g::local->weapon ( )->data( ) ) {
								auto weapon = g::local->weapon ( );
								auto weapon_data = weapon->data ( );

								auto cur_gun_skin = options::skin_vars.find ( std::string ( _ ( "skins.skin." ) ).append ( weapon_data->m_weapon_name ) );

								if ( cur_gun_skin != options::skin_vars.end ( ) ) {
									if ( ImGui::Checkbox ( _ ( "StatTrak" ), &cur_gun_skin->second.val.skin.is_stattrak ) ) features::skinchanger::skin_changed = true;
									if ( ImGui::SliderFloat ( _ ( "Quality" ), &cur_gun_skin->second.val.skin.wear, 0.0f, 100.0f, _ ( "%.0f%%" ) ) ) features::skinchanger::skin_changed = true;
									if ( ImGui::SliderInt ( _ ( "Seed" ), &cur_gun_skin->second.val.skin.seed, 0, 100 ) ) features::skinchanger::skin_changed = true;
									if ( ImGui::InputText ( _ ( "Nametag" ), cur_gun_skin->second.val.skin.nametag, sizeof ( cur_gun_skin->second.val.skin.nametag ) ) ) features::skinchanger::skin_changed = true;

									//ImGui::Checkbox ( _ ( "Filter By Weapon" ), &options::skin_vars [ _ ( "skins.skin.filter_by_weapon" ) ].val.b );

									static std::vector<features::skinchanger::paint_kit> items;

									int cur_paintkit = 0;

									if ( items.empty( ) ) {
										int index = 0;

										for ( auto& paintkit : features::skinchanger::skin_kits ) {
											if ( paintkit.id == cur_gun_skin->second.val.skin.paintkit )
												cur_paintkit = index;

											items.push_back ( paintkit );
											index++;
										}
									}

									/* skin search */
									auto stristr = [ ] ( const char* str1, const char* str2 ) -> char* {
										const char* p1 = str1;
										const char* p2 = str2;
										const char* r = *p2 == 0 ? str1 : 0;

										while ( *p1 != 0 && *p2 != 0 ) {
											if ( tolower ( ( unsigned char ) *p1 ) == tolower ( ( unsigned char ) *p2 ) ) {
												if ( !r )
													r = p1;

												p2++;
											}
											else {
												p2 = str2;

												if ( r )
													p1 = r + 1;

												if ( tolower ( ( unsigned char ) *p1 ) == tolower ( ( unsigned char ) *p2 ) ) {
													r = p1;
													p2++;
												}
												else
													r = 0;
											}

											p1++;
										}

										return *p2 == 0 ? ( char* ) r : 0;
									};

									static char skin_search [ 64 ] = { 0 };

									ImGui::NewLine ( );

									if ( ImGui::InputText ( _ ( "##Skin Search" ), skin_search, sizeof ( skin_search ) ) || last_weapon_name != weapon_data->m_weapon_name ) {
										if ( !items.empty ( ) )
											items.clear ( );

										int index = 0;

										if ( !strlen ( skin_search ) ) {
											for ( auto& paintkit : features::skinchanger::skin_kits ) {
												if ( paintkit.id == cur_gun_skin->second.val.skin.paintkit )
													cur_paintkit = index;

												items.push_back ( paintkit );
												index++;
											}
										}
										else {
											for ( auto& paintkit : features::skinchanger::skin_kits ) {
												if ( stristr ( paintkit.name.data ( ), skin_search ) ) {
													if ( paintkit.id == cur_gun_skin->second.val.skin.paintkit )
														cur_paintkit = index;

													items.push_back ( paintkit );
												}

												index++;
											}
										}
									}

									/* skin listbox goes here */
									if ( ImGui::ListBox ( _ ( "##Skin List" ), &cur_paintkit, gui_paintkit_getter, items.data ( ), items.size ( ), 5 ) ) {
										cur_gun_skin->second.val.skin.paintkit = items [ cur_paintkit ].id;
										features::skinchanger::skin_changed = true;
									}
								}

								last_weapon_name = weapon_data->m_weapon_name;
							}
							ImGui::PopItemWidth ( );

							ImGui::EndChildFrame ( );
						}

						ImGui::SetCursorPosY ( knives_group_end );

						ImGui::BeginChildFrame ( ImGui::GetID ( _ ( "Glove Options" ) ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, ImGui::GetContentRegionAvail ( ).y ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( _ ( "Glove Options" ) ).x * 0.5f );
							ImGui::Text ( _ ( "Glove Options" ) );
							ImGui::Separator ( );

							ImGui::PushItemWidth ( -1.0f );
							if( ImGui::Checkbox ( _ ( "Override Gloves" ), &options::skin_vars [ _ ( "skins.skin.override_gloves" ) ].val.b )) features::skinchanger::skin_changed = true;
							ImGui::PopItemWidth ( );

							/* combo for glove model goes here */
							/* skin listbox goes here */

							ImGui::EndChildFrame ( );
						}
					} );
				
					ImGui::custom::AddSubtab ( _("Model Changer"), _("Replace game models with your own"), [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( _ ( "Player Models" ) ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( _ ( "Player Models" ) ).x * 0.5f );
							ImGui::Text ( _ ( "Player Models" ) );
							ImGui::Separator ( );

							std::vector<const char*> models {
								_ ( "Default" ),
								_ ( "Special Agent Ava | FBI" ),
								_ ( "Operator | FBI SWAT" ),
								_ ( "Markus Delrow | FBI HRT" ),
								_ ( "Michael Syfers | FBI Sniper" ),
								_ ( "B Squadron Officer | SAS" ),
								_ ( "Seal Team 6 Soldier | NSWC SEAL" ),
								_ ( "Buckshot | NSWC SEAL" ),
								_ ( "Lt. Commander Ricksaw | NSWC SEAL" ),
								_ ( "3rd Commando Company | KSK" ),
								_ ( "'Two Times' McCoy | USAF TACP" ),
								_ ( "Dragomir | Sabre" ),
								_ ( "Rezan The Ready | Sabre" ),
								_ ( "'The Doctor' Romanov | Sabre" ),
								_ ( "Maximus | Sabre" ),
								_ ( "Blackwolf | Sabre" ),
								_ ( "The Elite Mr. Muhlik | Elite Crew" ),
								_ ( "Ground Rebel | Elite Crew" ),
								_ ( "Osiris | Elite Crew" ),
								_ ( "Prof. Shahmat | Elite Crew" ),
								_ ( "Enforcer | Phoenix" ),
								_ ( "Slingshot | Phoenix" ),
								_ ( "Soldier | Phoenix" ),
								_ ( "Jumpsuit (Variant B)" ),
							};

							ImGui::PushItemWidth ( -1.0f );
							ImGui::Combo ( _ ( "T Player Model" ), &options::skin_vars [ _ ( "skins.models.player_model_t" ) ].val.i, models.data ( ), models.size ( ) );
							ImGui::Combo ( _ ( "CT Player Model" ), &options::skin_vars [ _ ( "skins.models.player_model_ct" ) ].val.i, models.data ( ), models.size ( ) );
							ImGui::PopItemWidth ( );

							//if ( ImGui::Button ( _ ( "Save" ), ImVec2 ( -1.0f, 0.0f ) ) ) {
							//	auto cfg_dir = create_cfg_dirs ( );
							//
							//	options::save ( options::skin_vars, cfg_dir.append ( _ ( "\\sesame\\skins\\skins.xml" ) ) );
							//	cs::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
							//}
							//
							//if ( ImGui::Button ( _ ( "Load" ), ImVec2 ( -1.0f, 0.0f ) ) ) {
							//	auto cfg_dir = create_cfg_dirs ( );
							//
							//	options::load ( options::skin_vars, cfg_dir.append ( _ ( "\\sesame\\skins\\skins.xml" ) ) );
							//	cs::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
							//
							//	last_weapon_name = nullptr;
							//}

							ImGui::EndChildFrame ( );
						}
					} );
				} break;
				case tab_misc: {
					ImGui::custom::AddSubtab ( _("Movement"), _("Movement related cheats"), [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID (_( "General") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( _("General") ).x * 0.5f );
							ImGui::Text (_( "General" ));
							ImGui::Separator ( );

							ImGui::Checkbox ( _ ( "Block Bot" ), &options::vars [ _ ( "misc.movement.block_bot" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::Keybind ( _ ( "##Block Bot Key" ), &options::vars [ _ ( "misc.movement.block_bot_key" ) ].val.i, &options::vars [ _ ( "misc.movement.block_bot_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::Checkbox ( _ ( "Auto Jump" ), &options::vars [ _ ( "misc.movement.bhop" ) ].val.b );
							//ImGui::Checkbox ( _ ( "Auto Forward" ), &options::vars [ _ ( "misc.movement.auto_forward" ) ].val.b );
							ImGui::Checkbox ( _ ( "Auto Strafer" ), &options::vars [ _ ( "misc.movement.auto_strafer" ) ].val.b );
							ImGui::Checkbox ( _ ( "Directional Auto Strafer" ), &options::vars [ _ ( "misc.movement.omnidirectional_auto_strafer" ) ].val.b );
							ImGui::Checkbox ( _ ( "Fast Stop" ), &options::vars [ _ ( "misc.movement.accurate_move" ) ].val.b );
							ImGui::Checkbox ( _ ( "Air Stuck" ), &options::vars [ _ ( "misc.movement.airstuck" ) ].val.b );
							ImGui::Keybind ( _ ( "Air Stuck Key" ), &options::vars [ _ ( "misc.movement.airstuck_key" ) ].val.i, &options::vars [ _ ( "misc.movement.airstuck_key_mode" ) ].val.i );

							ImGui::EndChildFrame ( );
						}
					} );
					ImGui::custom::AddSubtab ( _("Effects"),_( "Miscellaneous visual effects"), [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( _("General") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize (_( "General") ).x * 0.5f );
							ImGui::Text ( _("General") );
							ImGui::Separator ( );

							ImGui::Checkbox ( _ ( "Static Legs in Air" ), &options::vars [ _ ( "misc.effects.static_legs" ) ].val.b );
							ImGui::Checkbox ( _ ( "Slide on Slow Walk" ), &options::vars [ _ ( "misc.effects.slowwalk_slide" ) ].val.b );
							ImGui::Checkbox ( _ ( "No Pitch on Land" ), &options::vars [ _ ( "misc.effects.no_pitch_on_land" ) ].val.b );
							ImGui::Checkbox ( _ ( "Third Person" ), &options::vars [ _ ( "misc.effects.third_person" ) ].val.b );
							ImGui::SameLine ( );
							ImGui::Keybind ( _ ( "##Third Person Key" ), &options::vars [ _ ( "misc.effects.third_person_key" ) ].val.i, &options::vars [ _ ( "misc.effects.third_person_key_mode" ) ].val.i, ImVec2 ( -1.0f, 0.0f ) );
							ImGui::PushItemWidth ( -1.0f );
							ImGui::SliderFloat ( _ ( "Third Person Range" ), &options::vars [ _ ( "misc.effects.third_person_range" ) ].val.f, 0.0f, 500.0f, _ ( "%.1f units" ) );
							ImGui::SliderFloat ( _ ( "Ragdoll Force Scale" ), &options::vars [ _ ( "misc.effects.ragdoll_force_scale" ) ].val.f, 0.0f, 10.0f, _ ( "x%.1f" ) );
							ImGui::PopItemWidth ( );
							ImGui::Checkbox ( _ ( "Clan Tag" ), &options::vars [ _ ( "misc.effects.clantag" ) ].val.b );
							static std::vector<const char*> tag_anims {  "Static",  "Marquee", "Capitalize" ,  "Heart", "gamesense" };

							ImGui::PushItemWidth ( -1.0f );
							ImGui::Combo ( _ ( "Clan Tag Animation" ), &options::vars [ _ ( "misc.effects.clantag_animation" ) ].val.i, tag_anims.data ( ), tag_anims.size ( ) );
							ImGui::InputText ( _ ( "Clan Tag Text" ), options::vars [ _ ( "misc.effects.clantag_text" ) ].val.s, sizeof( options::vars [ _ ( "misc.effects.clantag_text" ) ].val.s ) );
							ImGui::SliderFloat ( _ ( "Revolver Cock Volume" ), &options::vars [ _ ( "misc.effects.revolver_cock_volume" ) ].val.f, 0.0f, 1.0f, _ ( "x%.1f" ) );
							ImGui::SliderFloat ( _ ( "Weapon Volume" ), &options::vars [ _ ( "misc.effects.weapon_volume" ) ].val.f, 0.0f, 1.0f, _ ( "x%.1f" ) );
							ImGui::SliderInt ( _ ( "View Interpolation" ), &options::vars [ _ ( "misc.effects.view_interpolation" ) ].val.i, -1, 16, ( options::vars [ _ ( "misc.effects.view_interpolation" ) ].val.i == -1 ) ? _ ( "Default" ) : _ ( "%d ticks" ) );
							ImGui::PopItemWidth ( );

							ImGui::EndChildFrame ( );
						}
					} );
					ImGui::custom::AddSubtab ( _("Player List"),_( "Whitelist, clantag stealer, and bodyaim priority"), [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( _("Players") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( _("Players") ).x * 0.5f );
							ImGui::Text ( _("Players") );
							ImGui::Separator ( );

							for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
								const auto ent = cs::i::ent_list->get<player_t*> ( i);

								if ( !ent || !ent->is_player ( ) )
									continue;

								player_info_t player_info { };
								
								if ( !cs::i::engine->get_player_info ( i, &player_info ) || player_info.m_fake_player )
									continue;

								if ( ImGui::Button ( player_info.m_name, ImVec2 ( -1.0f, 0.0f ) ) )
									current_plist_player = player_info.m_steam_id;
							}

							ImGui::EndChildFrame ( );
						}

						ImGui::SameLine ( );

						ImGui::BeginChildFrame ( ImGui::GetID ( _("Player Actions") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( _("Player Actions") ).x * 0.5f );
							ImGui::Text (_( "Player Actions") );
							ImGui::Separator ( );

							if ( current_plist_player ) {
								if ( ImGui::Button ( !player_to_steal_tag_from ? _ ( "Steal Clantag" ) : _ ( "Stop Stealing Clantag" ), ImVec2 ( -1.0f, 0.0f ) ) )
									player_to_steal_tag_from = ( player_to_steal_tag_from ? 0 : current_plist_player );
							}

							ImGui::EndChildFrame ( );
						}
					} );
					ImGui::custom::AddSubtab ( _("Cheat"), _("Cheat settings and panic button"), [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( _("Menu") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize (_( "Menu" )).x * 0.5f );
							ImGui::Text ( _("Menu" ));
							ImGui::Separator ( );

							ImGui::PushItemWidth ( -1.0f );
							ImGui::SliderInt ( _ ( "GUI Animation Speed" ), &options::vars [ _ ( "gui.animation_speed" ) ].val.i, 10, 200, _ ( "%d%%" ) );
							ImGui::PopItemWidth ( );

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

							//ImGui::PushItemWidth ( -1.0f );
							//ImGui::SliderInt ( _ ( "Extrapolation Ticks" ), &features::ragebot::extrap_amount, 0, 16, _ ( "%d ticks" ) );
							//ImGui::PopItemWidth ( );

							ImGui::EndChildFrame ( );
						}

						ImGui::SameLine ( );

						ImGui::BeginChildFrame ( ImGui::GetID ( _("Cheat") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( _("Cheat") ).x * 0.5f );
							ImGui::Text ( _("Cheat" ));
							ImGui::Separator ( );

							//static std::vector<const char*> angle_interp_mode { "None",  "Last Tick", "Lerp" };
							//
							//ImGui::PushItemWidth( -1.0f );
							//ImGui::Combo( _( "Angle Interpolation Mode" ) , &anims::angle_interp_mode , angle_interp_mode.data( ) , angle_interp_mode.size( ) );
							//ImGui::PopItemWidth( );

#ifdef ANIMATION_LOGGER
							ImGui::SliderFloat ( _ ( "Debug Desync Amount" ), &options::vars [ _ ( "debug.desync_amount" ) ].val.f, -60.0f, 60.0f, _ ( "%.1f deg" ) );

							if ( ImGui::Button("Dump Animation Data") ) {
								std::ofstream animation_dump ( "anim_dump.hpp" );

								if ( animation_dump.is_open ( ) ) {
									animation_dump << anims::logger::dump ( );
									animation_dump.close ( );
								}
							}
#endif

							ImGui::EndChildFrame ( );
						}
					} );
					
					ImGui::custom::AddSubtab ( _("Configuration"), _("Cheat configuration manager"), [ & ] ( ) {
						ImGui::BeginChildFrame ( ImGui::GetID ( _("Configs") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( _("Configs") ).x * 0.5f );
							ImGui::Text ( _("Configs") );
							ImGui::Separator ( );

							for ( const auto& config : configs ) {
								if ( ImGui::Button ( config.data ( ), ImVec2( -1.0f, 0.0f ) ) )
									strcpy_s ( selected_config, config.c_str ( ) );
							}

							ImGui::EndChildFrame ( );
						}

						ImGui::SameLine ( );

						ImGui::BeginChildFrame ( ImGui::GetID ( _("Config Actions") ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
							ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize (_( "Config Actions" )).x * 0.5f );
							ImGui::Text ( _("Config Actions" ));
							ImGui::Separator ( );

							ImGui::PushItemWidth ( -1.0f );
							ImGui::InputText ( _ ( "Config Name" ), selected_config, sizeof ( selected_config ) );
							ImGui::PopItemWidth ( );

							if ( ImGui::Button ( _ ( "Save" ), ImVec2 ( -1.0f, 0.0f ) ) ) {
								auto cfg_dir = create_cfg_dirs ( );

								auto file_exists = [ ] ( const std::string& path ) {
									std::ifstream file ( path );
									return file.good ( );
								};

								auto file = cfg_dir.append( _ ( "\\sesame\\configs\\" ) ).append( selected_config ).append(_ ( ".xml" ));

								if ( file_exists ( file ) ) {
									open_save_modal = true;
								}
								else {
									options::save ( options::skin_vars, cfg_dir + _ ( "\\sesame\\skins\\skins.xml" ) );
									options::save ( options::vars, file );

									gui_mutex.lock ( );
									load_cfg_list ( );
									gui_mutex.unlock ( );

									cs::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
								}
							}

							if ( ImGui::Button ( _ ( "Load" ), ImVec2 ( -1.0f, 0.0f ) ) ) {
								const auto cfg_dir = create_cfg_dirs ( );

								options::load ( options::skin_vars, cfg_dir + _ ( "\\sesame\\skins\\skins.xml" ) );
								options::load ( options::vars, cfg_dir + _ ( "\\sesame\\configs\\" ) + selected_config + _ ( ".xml" ) );
								last_weapon_name = nullptr;

								cs::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
							}

							if ( ImGui::Button ( _ ( "Delete" ), ImVec2 ( -1.0f, 0.0f ) ) ) {
								auto cfg_dir = create_cfg_dirs ( );

								auto file_exists = [ ] ( const std::string& path ) {
									std::ifstream file ( path );
									return file.good ( );
								};

								auto file = cfg_dir.append ( _ ( "\\sesame\\configs\\" ) ).append ( selected_config ).append ( _ ( ".xml" ) );

								if ( file_exists ( file ) ) {
									open_delete_modal = true;
								}
								else {
									std::remove ( ( cfg_dir + _ ( "\\sesame\\configs\\" ) + selected_config + _ ( ".xml" ) ).c_str ( ) );

									gui_mutex.lock ( );
									load_cfg_list ( );
									gui_mutex.unlock ( );

									cs::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
								}
							}

							if ( ImGui::Button ( _ ( "Refresh List" ), ImVec2 ( -1.0f, 0.0f ) ) ) {
								gui_mutex.lock ( );
								load_cfg_list ( );
								gui_mutex.unlock ( );
								cs::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
							}

							//ImGui::PushItemWidth ( -1.0f );
							//ImGui::InputText ( _ ( "Config Description" ), config_description, 128 );
							//static std::vector<const char*> access_perms { "Public" ,  "Private" , "Unlisted" };
							//ImGui::Combo ( _ ( "Config Access" ), &config_access, access_perms.data(),access_perms.size() );
							//ImGui::PopItemWidth();
							//
							//if ( ImGui::Button ( _ ( "Upload To Cloud" ), ImVec2( -1.0f, 0.0f ) ) )
							//	upload_to_cloud = true;

							ImGui::EndChildFrame ( );
						}
					} );

					//ImGui::custom::AddSubtab ( "Scripts", "Script manager", [ & ] ( ) {
					//	ImGui::BeginChildFrame ( ImGui::GetID ( "Scripts" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
					//		ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Scripts" ).x * 0.5f );
					//		ImGui::Text ( "Scripts" );
					//		ImGui::Separator ( );
					//
					//		for ( const auto& script : scripts ) {
					//			if ( ImGui::Button ( script.data ( ), ImVec2 ( -1.0f, 0.0f ) ) )
					//				strcpy_s ( selected_script, script.c_str ( ) );
					//		}
					//
					//		ImGui::EndChildFrame ( );
					//	}
					//
					//	ImGui::SameLine ( );
					//
					//	ImGui::BeginChildFrame ( ImGui::GetID ( "Script Actions" ), ImVec2 ( ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::GetStyle ( ).FramePadding.x, 0.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
					//		ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Script Actions" ).x * 0.5f );
					//		ImGui::Text ( "Script Actions" );
					//		ImGui::Separator ( );
					//
					//		ImGui::PushItemWidth ( -1.0f );
					//		ImGui::InputText ( _ ( "Script Name" ), selected_script, sizeof ( selected_script ) );
					//		ImGui::PopItemWidth ( );
					//
					//		if ( ImGui::Button ( _ ( "Delete" ), ImVec2 ( -1.0f, 0.0f ) ) ) {
					//			const auto cfg_dir = create_cfg_dirs ( );
					//
					//			auto file_exists = [ ] ( const std::string& path ) {
					//				std::ifstream file ( path );
					//				return file.good ( );
					//			};
					//
					//			const auto file = std::string ( appdata ).append ( _ ( "\\sesame\\scripts\\" ) ).append ( selected_script ).append ( _ ( ".js" ) );
					//
					//			if ( file_exists ( file ) ) {
					//				open_delete_script_modal = true;
					//			}
					//			else {
					//				std::remove ( ( std::string ( appdata ) + _ ( "\\sesame\\scripts\\" ) + selected_script + _ ( ".js" ) ).c_str ( ) );
					//
					//				gui_mutex.lock ( );
					//				load_cfg_list ( );
					//				gui_mutex.unlock ( );
					//
					//				cs::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
					//			}
					//		}
					//
					//		if ( ImGui::Button ( _ ( "Reload Scripts" ), ImVec2 ( -1.0f, 0.0f ) ) ) {
					//			gui_mutex.lock ( );
					//			load_cfg_list ( );
					//			gui_mutex.unlock ( );
					//			cs::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
					//		}
					//
					//		ImGui::EndChildFrame ( );
					//	}
					//} );
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
				ImGui::OpenPopup (_( "Save Config##popup") );

			if ( open_delete_modal )
				ImGui::OpenPopup ( _ ( "Delete Config##popup" ) );

			if ( open_delete_script_modal )
				ImGui::OpenPopup ( _ ( "Delete Script##popup" ) );

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
	static auto& break_backtrack = options::vars [ _ ( "antiaim.break_backtrack" ) ].val.b;
	static auto& airstuck = options::vars [ _ ( "misc.movement.airstuck" ) ].val.b;

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

	static auto break_backtrack_key = find_keybind ( _ ( "antiaim.break_backtrack_key" ) );
	static auto airstuck_key = find_keybind ( _ ( "misc.movement.airstuck_key" ) );
	static auto min_dmg_override_key = find_keybind ( _ ( "ragebot.min_dmg_override_key" ) );

	if ( cs::i::engine->is_in_game( ) && cs::i::engine->is_connected( ) ) {
		if ( triggerbot && assistance_type == 1 )
			add_key_entry( triggerbot_key, _( "Trigger Bot" ) );

		if ( features::ragebot::active_config.main_switch && features::ragebot::active_config.safe_point )
			add_key_entry( safe_point_key, _( "Safe Point" ) );

		if ( features::ragebot::active_config.main_switch && features::ragebot::active_config.dt_enabled )
			add_key_entry( doubletap_key, _( "Tickbase Shift" ) );

		if ( features::ragebot::active_config.main_switch )
			add_key_entry ( min_dmg_override_key, _ ( "Damage Override" ) );

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

		if ( break_backtrack )
			add_key_entry ( break_backtrack_key, _ ( "Break Backtrack" ) );

		if ( airstuck )
			add_key_entry ( airstuck_key, _ ( "Airstuck" ) );

		if ( features::antiaim::antiaiming ) {
			switch ( features::antiaim::side ) {
				case -1: break;
				case 0: entries.push_back( _( "[TOGGLE] Manual Antiaim (Back)" ) ); break;
				case 1: entries.push_back( _( "[TOGGLE] Manual Antiaim (Left)" ) ); break;
				case 2: entries.push_back( _( "[TOGGLE] Manual Antiaim (Right)" ) ); break;
			}
		}
	}

	if ( keybind_list && ImGui::Begin( _( "Keybinds" ), &keybind_list, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ( gui::opened ? 0 : ImGuiWindowFlags_NoMove ) ) ) {
		ImGui::SetCursorPosX ( ImGui::GetCursorPosX ( ) + ImGui::GetWindowContentRegionWidth ( ) * 0.5f - ImGui::CalcTextSize ( "Keybinds" ).x * 0.5f );
		ImGui::Text ( _("Keybinds" )); 
		ImGui::Separator ( );

		if ( !entries.empty( ) ) {
			for ( auto& entry : entries )
				ImGui::Text( entry.c_str( ) );
		}

		ImGui::End( );
	}
}

#pragma optimize( "2", on )