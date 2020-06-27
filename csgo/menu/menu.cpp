#include <time.h>
#include "menu.hpp"
#include "../sdk/sdk.hpp"
#include <ShlObj.h>
#include <filesystem>
#include <algorithm>
#include "../globals.hpp"
#include "../features/esp.hpp"
#include "../features/ragebot.hpp"

std::shared_ptr< oxui::panel > panel;
std::shared_ptr< oxui::window > window;
std::shared_ptr< oxui::group > config_list;
std::shared_ptr< oxui::group > player_list;

int current_plist_player = 0;
extern int player_to_steal_tag_from;

time_t last_plist_refresh = 0;

void refresh_player_list ( ) {
	if ( std::abs ( last_plist_refresh - time ( nullptr ) ) < 2 )
		return;

	last_plist_refresh = time ( nullptr );

	player_list->objects.clear ( );

	/* remove selected player if they don't exist */
	auto cur_pl = csgo::i::ent_list->get< player_t* > ( current_plist_player );

	if ( !cur_pl )
		current_plist_player = 0;

	/* remove selected player if they don't exist */
	cur_pl = csgo::i::ent_list->get< player_t* > ( player_to_steal_tag_from );

	if ( !cur_pl )
		player_to_steal_tag_from = 0;

	/* scan for players in the server */
	if ( g::local ) {
		/* show selected player */ {
			/* remove selected player if they don't exist */
			auto cur_pl = csgo::i::ent_list->get< player_t* > ( current_plist_player );

			if ( cur_pl && current_plist_player ) {
				player_info_t info;
				csgo::i::engine->get_player_info ( cur_pl->idx ( ), &info );

				wchar_t buf [ 36 ];

				if ( MultiByteToWideChar ( CP_UTF8, 0, info.m_name, -1, buf, 36 ) > 0 )
					player_list->add_element ( std::make_shared< oxui::label > ( OSTR ( "Selected: " ) + oxui::str( buf ) ) );

				if ( current_plist_player == player_to_steal_tag_from /* || whatever */ ) {
					oxui::str label;

					if ( current_plist_player == player_to_steal_tag_from ) {
						if ( !label.empty ( ) )
							label += OSTR( ", " );

						label += OSTR ( "Stealing Clantag" );
					}

					player_list->add_element ( std::make_shared< oxui::label > ( label ) );
				}
			}
		}

		for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
			auto pl = csgo::i::ent_list->get< player_t* > ( i );

			if ( !pl || !pl->client_class ( ) || pl->client_class ( )->m_class_id != 40 )
				continue;

			const auto pl_idx = pl->idx ( );

			player_info_t info;
			csgo::i::engine->get_player_info ( pl_idx, &info );

			if ( info.m_fake_player )
				continue;

			wchar_t buf [ 36 ];

			if ( MultiByteToWideChar ( CP_UTF8, 0, info.m_name, -1, buf, 36 ) > 0 ) {
				player_list->add_element ( std::make_shared< oxui::button > ( buf, [ = ] ( ) {
					/* instantly refresh list */
					last_plist_refresh = 0;

					current_plist_player = pl_idx;
				} ) );
			}
		}
	}
}

void refresh_config_list ( ) {
	wchar_t appdata [ MAX_PATH ];

	if ( SUCCEEDED ( LI_FN ( SHGetFolderPathW )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
		LI_FN ( CreateDirectoryW )( ( std::wstring ( appdata ) + _ ( L"\\sesame" ) ).data ( ), nullptr );
		LI_FN ( CreateDirectoryW )( ( std::wstring ( appdata ) + _ ( L"\\sesame\\configs" ) ).data ( ), nullptr );
	}
	
	auto sanitize_name = [ ] ( const std::wstring& dir ) {
		const auto dot = dir.find_last_of ( _ ( L"." ) );
		return dir.substr ( N ( 0 ), dot );
	};

	config_list->objects.clear ( );

	try {
		for ( const auto& dir : std::filesystem::recursive_directory_iterator ( std::wstring ( appdata ) + _ ( L"\\sesame\\configs" ) ) )
			if ( dir.exists ( ) && dir.is_regular_file ( ) && dir.path ( ).extension ( ).wstring ( ) == _ ( L".json" ) ) {
				const auto sanitized = sanitize_name ( dir.path ( ).filename ( ).wstring ( ) );

				config_list->add_element ( std::make_shared< oxui::button > ( sanitized, [ = ] ( ) {
					OPTION ( oxui::str, cfg_name, "Sesame->Customization->Configs->Actions->Config Name", oxui::object_textbox );
					cfg_name = sanitized;
				} ) );
			}
	}
	catch ( const std::exception & ex ) {
		dbg_print ( ex.what ( ) );
	}
}

const oxui::str& get_keybind_mode_name ( oxui::keybind* kb ) {
	static oxui::str mode_hold = _ ( L"Hold - " );
	static oxui::str mode_toggle = _ ( L"Toggle - " );
	static oxui::str mode_always = _ ( L"Always - " );

	switch ( kb->mode ) {
	case oxui::keybind_mode::hold: return mode_hold; break;
	case oxui::keybind_mode::toggle: return mode_toggle; break;
	case oxui::keybind_mode::always: return mode_always; break;
	}

	return mode_hold;
};

void menu::draw_watermark ( ) {
	OPTION ( oxui::color, accent_color, "Sesame->C->Other->GUI->Accent Color", oxui::object_colorpicker );
	OPTION ( oxui::color, secondary_accent_color, "Sesame->C->Other->GUI->Secondary Accent Color", oxui::object_colorpicker );
	OPTION ( oxui::color, logo_color, "Sesame->C->Other->GUI->Logo Color", oxui::object_colorpicker );
	OPTION ( bool, watermark_enabled, "Sesame->C->Other->GUI->Watermark", oxui::object_checkbox );
	OPTION ( bool, keybind_list_enabled, "Sesame->C->Other->GUI->Keybind List", oxui::object_checkbox );

	oxui::theme.main = oxui::theme.title_bar = accent_color;
	oxui::theme.title_bar_low = secondary_accent_color;
	oxui::theme.logo = logo_color;
	oxui::theme.main.a = oxui::theme.title_bar.a = oxui::theme.title_bar_low.a = 255;

	auto cur_popups_y = 24;

	int w = 0, h = 0;
	render::screen_size ( w, h );

	const auto dim = oxui::rect { 0, 0, 330, 35 };

	if ( watermark_enabled ) {
		const auto bounds = oxui::rect { w - dim.w - 24, cur_popups_y, dim.w, dim.h };

		render::rounded_rect ( bounds.x, bounds.y, dim.h, dim.h, 8, 4, D3DCOLOR_RGBA ( oxui::theme.main.r, oxui::theme.main.g, oxui::theme.main.b, oxui::theme.main.a ), false );
		render::rounded_rect ( bounds.x + dim.h + 12, bounds.y, dim.w - ( dim.h + 12 ), dim.h, 8, 4, D3DCOLOR_RGBA ( oxui::theme.bg.r, oxui::theme.bg.g, oxui::theme.bg.b, oxui::theme.bg.a ), false );
		render::rounded_rect ( bounds.x + dim.h + 12, bounds.y, dim.w - ( dim.h + 12 ), dim.h, 8, 4, D3DCOLOR_RGBA ( oxui::theme.container_bg.r, oxui::theme.container_bg.g, oxui::theme.container_bg.b, oxui::theme.container_bg.a ), true );

		const auto target_logo_dim = oxui::pos { static_cast < int > ( ( 565.5 / 6 ) / 3 ),static_cast < int > ( 500.25 / 20 ) };
		const auto logo_scale = static_cast< float > ( target_logo_dim.y ) / 107.0f;
		const auto logo_dim = oxui::pos { static_cast < int > ( 150.0f * logo_scale ), static_cast < int > ( 107.0f * logo_scale ) };

		render::texture ( panel->sprite, panel->tex, ( bounds.x + dim.h / 2 ) - logo_dim.x + logo_dim.x / 3, ( bounds.y + dim.h / 2 - 4 ) - logo_dim.y / 2, target_logo_dim.x, target_logo_dim.y, logo_scale * 0.8f, logo_scale * 1.333f * 0.8f, D3DCOLOR_RGBA ( oxui::theme.logo.r, oxui::theme.logo.g, oxui::theme.logo.b, oxui::theme.logo.a ) );

		static float last_counter_update = 0.0f;
		static int last_framerate = 0;

		if ( std::fabs ( csgo::i::globals->m_curtime - last_counter_update ) > 1.0f ) {
			last_framerate = static_cast< int >( 1.0f / csgo::i::globals->m_abs_frametime );
			last_counter_update = csgo::i::globals->m_curtime;
		}

		const auto framerate_str = std::to_wstring ( std::clamp ( last_framerate, 0, 999 ) );

		/* fps counter */
		render::dim fps_dim;
		render::dim fps_dim1;
		render::dim key_dim;

		render::text_size ( features::esp::watermark_font, framerate_str, fps_dim );
		render::text_size ( features::esp::watermark_font, _ ( L"FPS" ), fps_dim1 );
		render::text_size ( reinterpret_cast < void* >( panel->fonts [ OSTR ( "tab" ) ] ), _ ( L"O" ), key_dim );

		render::text ( bounds.x + dim.h + 12 + 12, bounds.y + dim.h / 2 - fps_dim.h / 2, ( last_framerate < 64 ) ? D3DCOLOR_RGBA ( 235, 64, 52, 255 ) : D3DCOLOR_RGBA ( 255, 255, 255, 255 ), features::esp::watermark_font, std::to_wstring ( last_framerate ), true );
		const auto fps_w = fps_dim.w;
		render::text_size ( features::esp::watermark_font, _ ( L"FPS" ), fps_dim );
		render::text ( bounds.x + dim.h + 12 + 12 + fps_w + 6, bounds.y + dim.h / 2 - fps_dim.h / 2, ( last_framerate < 64 ) ? D3DCOLOR_RGBA ( 235, 64, 52, 255 ) : D3DCOLOR_RGBA ( 255, 255, 255, 255 ), features::esp::watermark_font, _ ( L"FPS" ), true );
		
		/* server information */
		render::dim info_str;
		render::text_size ( features::esp::dbg_font, csgo::is_valve_server ( ) ? _ ( L"VALVE DS" ) : _ ( L"COMMUNITY" ), info_str );
		render::text ( bounds.x + dim.h + 12 + 12 + 75, bounds.y + dim.h / 2 - key_dim.h / 2, csgo::is_valve_server ( ) ? D3DCOLOR_RGBA ( 98, 235, 70, 255 ) : D3DCOLOR_RGBA ( 235, 64, 52, 255 ), reinterpret_cast < void* >( panel->fonts [ OSTR ( "check" ) ] ), _ ( L"O" ), true );
		render::text ( bounds.x + dim.h + 12 + 12 + 75 + key_dim.w / 2 - info_str.w / 2, bounds.y + dim.h / 2 + 3, csgo::is_valve_server ( ) ? D3DCOLOR_RGBA ( 98, 235, 70, 255 ) : D3DCOLOR_RGBA ( 235, 64, 52, 255 ), features::esp::dbg_font, csgo::is_valve_server ( ) ? _ ( L"VALVE DS" ) : _ ( L"COMMUNITY" ), true );

		/* clock */
		std::time_t t = std::time ( nullptr );
		wchar_t wstr [ 100 ];

		if ( std::wcsftime ( wstr, 100, L"%c", std::localtime ( &t ) ) ) {
			render::dim date_dim;
			render::text_size ( features::esp::watermark_font, wstr, date_dim );
			render::text ( w - date_dim.w - 12 - 12 - 6, bounds.y + dim.h / 2 - fps_dim.h / 2, D3DCOLOR_RGBA ( 255, 255, 255, 255 ), features::esp::watermark_font, wstr, true );
		}

		cur_popups_y += bounds.h + 24;
	}

	/* active binds */
	KEYBIND ( safe_point_key, "Sesame->A->Default->Accuracy->Safe Point Key" );
	KEYBIND ( tickbase_key, "Sesame->A->Default->Main->Doubletap Key" );
	KEYBIND ( slow_walk_key, "Sesame->B->Slow Walk->Slow Walk->Slow Walk Key" );
	KEYBIND ( fakeduck_key, "Sesame->B->Other->Other->Fakeduck Key" );
	KEYBIND ( inverter_key, "Sesame->B->Other->Other->Desync Flip Key" );
	KEYBIND ( third_person_key, "Sesame->E->Effects->Main->Third Person Key" );

	if ( keybind_list_enabled ) {
		std::vector < std::pair< oxui::keybind_mode /* keybind mode */, oxui::str /* keybind string */ > > active_binds { };

		if ( features::ragebot::active_config.dt_key ) active_binds.push_back ( { keybind_obj_tickbase_key->mode, get_keybind_mode_name ( keybind_obj_tickbase_key ) + keybind_obj_tickbase_key->label.substr ( 0, keybind_obj_tickbase_key->label.find ( _ ( L" Key" ) ) ) } );
		if ( slow_walk_key ) active_binds.push_back ( { keybind_obj_slow_walk_key->mode,get_keybind_mode_name ( keybind_obj_slow_walk_key ) + keybind_obj_slow_walk_key->label.substr ( 0, keybind_obj_slow_walk_key->label.find ( _ ( L" Key" ) ) ) } );
		if ( fakeduck_key ) active_binds.push_back ( { keybind_obj_fakeduck_key->mode, get_keybind_mode_name ( keybind_obj_fakeduck_key ) + keybind_obj_fakeduck_key->label.substr ( 0, keybind_obj_fakeduck_key->label.find ( _ ( L" Key" ) ) ) } );
		if ( inverter_key ) active_binds.push_back ( { keybind_obj_inverter_key->mode, get_keybind_mode_name ( keybind_obj_inverter_key ) + keybind_obj_inverter_key->label.substr ( 0, keybind_obj_inverter_key->label.find ( _ ( L" Key" ) ) ) } );
		if ( third_person_key ) active_binds.push_back ( { keybind_obj_third_person_key->mode, get_keybind_mode_name ( keybind_obj_third_person_key ) + keybind_obj_third_person_key->label.substr ( 0, keybind_obj_third_person_key->label.find ( _ ( L" Key" ) ) ) } );
		if ( safe_point_key ) active_binds.push_back ( { keybind_obj_safe_point_key->mode, get_keybind_mode_name ( keybind_obj_safe_point_key ) + keybind_obj_safe_point_key->label.substr ( 0, keybind_obj_safe_point_key->label.find ( _ ( L" Key" ) ) ) } );

		std::sort ( active_binds.begin ( ), active_binds.end ( ), [ ] ( const std::pair< oxui::keybind_mode /* keybind mode */, oxui::str /* keybind string */ >& lhs, const std::pair< oxui::keybind_mode /* keybind mode */, oxui::str /* keybind string */ >& rhs ) {
			return static_cast < int > ( lhs.first ) > static_cast < int > ( rhs.first );
		} );

		const auto keybinds_bounds = oxui::rect { w - dim.w - 24, cur_popups_y, dim.w, ( 18 + 4 ) * static_cast < int > ( active_binds.size ( ) ) + 12 * 2 };

		render::rounded_rect ( keybinds_bounds.x, keybinds_bounds.y, keybinds_bounds.w, keybinds_bounds.h, 8, 4, D3DCOLOR_RGBA ( oxui::theme.bg.r, oxui::theme.bg.g, oxui::theme.bg.b, oxui::theme.bg.a ), false );
		render::rounded_rect ( keybinds_bounds.x, keybinds_bounds.y, keybinds_bounds.w, keybinds_bounds.h, 8, 4, D3DCOLOR_RGBA ( oxui::theme.container_bg.r, oxui::theme.container_bg.g, oxui::theme.container_bg.b, oxui::theme.container_bg.a ), true );

		auto cur_y_pos = keybinds_bounds.y + 12;

		for ( auto& bind_entry : active_binds ) {
			render::dim bind_label_dim;
			render::text_size ( features::esp::watermark_font, bind_entry.second, bind_label_dim );
			render::text ( keybinds_bounds.x + keybinds_bounds.w / 2 - bind_label_dim.w / 2, cur_y_pos + 18 / 2 - bind_label_dim.h / 2, D3DCOLOR_RGBA ( 255, 255, 255, 255 ), features::esp::watermark_font, bind_entry.second, true );

			cur_y_pos += 18 + 4;
		}
	}
}

bool menu::open( ) {
	return window->open;
}

void* menu::find_obj ( const oxui::str& item, oxui::object_type otype ) {
	return window->find_obj ( item, otype );
}

long __stdcall menu::wndproc( HWND hwnd, std::uint32_t msg, std::uintptr_t wparam, std::uint32_t lparam ) {
	return window->wndproc( hwnd, msg, wparam, lparam );
}

void menu::load_default( ) {
	window->load_state( OSTR( "wcdef.json" ) );
}

void menu::destroy( ) {
	panel->destroy( );
}

void menu::reset( ) {
	panel->reset( );
}

const wchar_t* cur_date( ) {
	time_t rawtime = 0;
	tm* timeinfo = nullptr;

	time( &rawtime );
	timeinfo = localtime( &rawtime );

	static const wchar_t* wday_name [ ] = {
		_( L"sun" ), _( L"mon" ), _( L"tue" ), _( L"wed" ), _( L"thu" ),_( L"fri" ),_( L"sat" )
	};

	static const wchar_t* mon_name [ ] = {
		_( L"jan" ), _( L"feb" ),  _( L"mar" ), _( L"apr" ),_( L"may" ), _( L"jun" ),
		_( L"jul" ), _( L"aug" ), _( L"sep" ), _( L"oct" ), _( L"nov" ),  _( L"dec" )
	};

	static wchar_t result [ 26 ] { '\0' };

	wsprintfW( result, _( L"%s %d, %d\n" ), mon_name [ timeinfo->tm_mon ], timeinfo->tm_mday, 1900 + timeinfo->tm_year );

	return result;
}

void menu::init( ) {
	panel = std::make_shared< oxui::panel >( ); {
		window = std::make_shared< oxui::window >( oxui::rect( 200, 200, 565, 500 ), OSTR( "Sesame" ) /* + oxui::str( cur_date( ) ) */ ); {
			window->bind_key( VK_INSERT );

			auto aimbot = std::make_shared< oxui::tab > ( OSTR ( "A" ) ); {
				auto accuracy = std::make_shared< oxui::group > ( OSTR ( "Accuracy" ), std::vector< float > { 0.0f, 0.4f, 1.0f, 0.2f } ); {
					accuracy->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Fix Fakelag" ) ) );
					accuracy->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Resolve Desync" ) ) );
					accuracy->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Crash If Miss Due To Resolve" ) ) );
					accuracy->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Safe Point" ) ) );
					accuracy->add_element ( std::make_shared< oxui::keybind > ( OSTR ( "Safe Point Key" ) ) );
				}

				auto main_switch = std::make_shared< oxui::checkbox > ( OSTR ( "Main Switch" ) );
				auto optimization = std::make_shared< oxui::dropdown > ( OSTR ( "Optimization" ), std::vector< oxui::str > { OSTR ( "None" ), OSTR ( "Low" ), OSTR ( "Medium" ), OSTR ( "High" ) } );
				auto knife_bot = std::make_shared< oxui::checkbox > ( OSTR ( "Knife Bot" ) );
				auto zeus_bot = std::make_shared< oxui::checkbox > ( OSTR ( "Zeus Bot" ) );
				auto dt_key = std::make_shared< oxui::keybind > ( OSTR ( "Doubletap Key" ) );

				auto default_aimbot = std::make_shared< oxui::subtab > ( OSTR ( "Default" ) ); {
					auto main = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.4f } ); {
						main->add_element ( main_switch );
						main->add_element ( optimization );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Minimum Damage" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Hit Chance" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Doubletap Hit Chance" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Choke On Shot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Silent" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Shoot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Scope" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Slow" ) ) );
						main->add_element ( knife_bot );
						main->add_element ( zeus_bot );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim If Resolver Confidence Less Than" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Maximum Doubletap Ticks" ), 0.0, 0.0, 16.0 ) );
						main->add_element ( dt_key );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Doubletap Teleport" ) ) );

						default_aimbot->add_element ( main );
					}

					default_aimbot->add_element ( accuracy );

					auto target_selection = std::make_shared< oxui::group > ( OSTR ( "Hitscan" ), std::vector< float > { 0.0f, 0.6f, 0.5f, 0.4f }, false, true ); {
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If Lethal" ) ) );
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If In Air" ) ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim After X Misses" ), 0.0, 0.0, 4.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Head Pointscale" ), 0.0, 0.0, 100.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Body Pointscale" ), 0.0, 0.0, 100.0 ) );

						default_aimbot->add_element ( target_selection );
					}

					auto hitscan = std::make_shared< oxui::group > ( OSTR ( "Hitboxes" ), std::vector< float > { 0.5f, 0.6f, 0.5f, 0.4f }, true ); {
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Head" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Neck" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Chest" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Pelvis" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Arms" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Legs" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Feet" ) ) );

						default_aimbot->add_element ( hitscan );
					}

					aimbot->add_element ( default_aimbot );
				}

				auto pistol_aimbot = std::make_shared< oxui::subtab > ( OSTR ( "Pistol" ) ); {
					auto main = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.4f } ); {
						main->add_element ( main_switch );
						main->add_element ( optimization );
						main->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Inherit From" ), std::vector< oxui::str > { OSTR ( "None" ), OSTR ( "Default" ), OSTR ( "Pistol" ), OSTR ( "Revolver" ), OSTR ( "Rifle" ), OSTR ( "AWP" ), OSTR ( "Auto" ), OSTR ( "Scout" ) } ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Minimum Damage" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Hit Chance" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Doubletap Hit Chance" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Choke On Shot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Silent" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Shoot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Scope" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Slow" ) ) );
						main->add_element ( knife_bot );
						main->add_element ( zeus_bot );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim If Resolver Confidence Less Than" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Maximum Doubletap Ticks" ), 0.0, 0.0, 16.0 ) );
						main->add_element ( dt_key );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Doubletap Teleport" ) ) );

						pistol_aimbot->add_element ( main );
					}

					pistol_aimbot->add_element ( accuracy );

					auto target_selection = std::make_shared< oxui::group > ( OSTR ( "Hitscan" ), std::vector< float > { 0.0f, 0.6f, 0.5f, 0.4f }, false, true ); {
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If Lethal" ) ) );
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If In Air" ) ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim After X Misses" ), 0.0, 0.0, 4.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Head Pointscale" ), 0.0, 0.0, 100.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Body Pointscale" ), 0.0, 0.0, 100.0 ) );

						pistol_aimbot->add_element ( target_selection );
					}

					auto hitscan = std::make_shared< oxui::group > ( OSTR ( "Hitboxes" ), std::vector< float > { 0.5f, 0.6f, 0.5f, 0.4f }, true ); {
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Head" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Neck" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Chest" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Pelvis" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Arms" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Legs" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Feet" ) ) );

						pistol_aimbot->add_element ( hitscan );
					}

					aimbot->add_element ( pistol_aimbot );
				}

				auto revolver_aimbot = std::make_shared< oxui::subtab > ( OSTR ( "Revolver" ) ); {
					auto main = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.4f } ); {
						main->add_element ( main_switch );
						main->add_element ( optimization );
						main->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Inherit From" ), std::vector< oxui::str > { OSTR ( "None" ), OSTR ( "Default" ), OSTR ( "Pistol" ), OSTR ( "Revolver" ), OSTR ( "Rifle" ), OSTR ( "AWP" ), OSTR ( "Auto" ), OSTR ( "Scout" ) } ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Minimum Damage" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Hit Chance" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Choke On Shot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Silent" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Shoot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Scope" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Slow" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Revolver" ) ) );
						main->add_element ( knife_bot );
						main->add_element ( zeus_bot );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim If Resolver Confidence Less Than" ), 0.0, 0.0, 100.0 ) );

						revolver_aimbot->add_element ( main );
					}

					revolver_aimbot->add_element ( accuracy );

					auto target_selection = std::make_shared< oxui::group > ( OSTR ( "Hitscan" ), std::vector< float > { 0.0f, 0.6f, 0.5f, 0.4f }, false, true ); {
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If Lethal" ) ) );
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If In Air" ) ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim After X Misses" ), 0.0, 0.0, 4.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Head Pointscale" ), 0.0, 0.0, 100.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Body Pointscale" ), 0.0, 0.0, 100.0 ) );

						revolver_aimbot->add_element ( target_selection );
					}

					auto hitscan = std::make_shared< oxui::group > ( OSTR ( "Hitboxes" ), std::vector< float > { 0.5f, 0.6f, 0.5f, 0.4f }, true ); {
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Head" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Neck" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Chest" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Pelvis" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Arms" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Legs" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Feet" ) ) );

						revolver_aimbot->add_element ( hitscan );
					}

					aimbot->add_element ( revolver_aimbot );
				}

				auto rifle_aimbot = std::make_shared< oxui::subtab > ( OSTR ( "Rifle" ) ); {
					auto main = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.4f } ); {
						main->add_element ( main_switch );
						main->add_element ( optimization );
						main->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Inherit From" ), std::vector< oxui::str > { OSTR ( "None" ), OSTR ( "Default" ), OSTR ( "Pistol" ), OSTR ( "Revolver" ), OSTR ( "Rifle" ), OSTR ( "AWP" ), OSTR ( "Auto" ), OSTR ( "Scout" ) } ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Minimum Damage" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Hit Chance" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Doubletap Hit Chance" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Choke On Shot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Silent" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Shoot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Scope" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Slow" ) ) );
						main->add_element ( knife_bot );
						main->add_element ( zeus_bot );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Maximum Doubletap Ticks" ), 0.0, 0.0, 16.0 ) );
						main->add_element ( dt_key );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Doubletap Teleport" ) ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim If Resolver Confidence Less Than" ), 0.0, 0.0, 100.0 ) );

						rifle_aimbot->add_element ( main );
					}

					rifle_aimbot->add_element ( accuracy );

					auto target_selection = std::make_shared< oxui::group > ( OSTR ( "Hitscan" ), std::vector< float > { 0.0f, 0.6f, 0.5f, 0.4f }, false, true ); {
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If Lethal" ) ) );
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If In Air" ) ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim After X Misses" ), 0.0, 0.0, 4.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Head Pointscale" ), 0.0, 0.0, 100.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Body Pointscale" ), 0.0, 0.0, 100.0 ) );

						rifle_aimbot->add_element ( target_selection );
					}

					auto hitscan = std::make_shared< oxui::group > ( OSTR ( "Hitboxes" ), std::vector< float > { 0.5f, 0.6f, 0.5f, 0.4f }, true ); {
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Head" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Neck" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Chest" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Pelvis" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Arms" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Legs" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Feet" ) ) );

						rifle_aimbot->add_element ( hitscan );
					}

					aimbot->add_element ( rifle_aimbot );
				}

				auto awp_aimbot = std::make_shared< oxui::subtab > ( OSTR ( "AWP" ) ); {
					auto main = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.4f } ); {
						main->add_element ( main_switch );
						main->add_element ( optimization );
						main->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Inherit From" ), std::vector< oxui::str > { OSTR ( "None" ), OSTR ( "Default" ), OSTR ( "Pistol" ), OSTR ( "Revolver" ), OSTR ( "Rifle" ), OSTR ( "AWP" ), OSTR ( "Auto" ), OSTR ( "Scout" ) } ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Minimum Damage" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Hit Chance" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Choke On Shot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Silent" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Shoot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Scope" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Slow" ) ) );
						main->add_element ( knife_bot );
						main->add_element ( zeus_bot );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim If Resolver Confidence Less Than" ), 0.0, 0.0, 100.0 ) );

						awp_aimbot->add_element ( main );
					}

					awp_aimbot->add_element ( accuracy );

					auto target_selection = std::make_shared< oxui::group > ( OSTR ( "Hitscan" ), std::vector< float > { 0.0f, 0.6f, 0.5f, 0.4f }, false, true ); {
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If Lethal" ) ) );
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If In Air" ) ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim After X Misses" ), 0.0, 0.0, 4.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Head Pointscale" ), 0.0, 0.0, 100.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Body Pointscale" ), 0.0, 0.0, 100.0 ) );

						awp_aimbot->add_element ( target_selection );
					}

					auto hitscan = std::make_shared< oxui::group > ( OSTR ( "Hitboxes" ), std::vector< float > { 0.5f, 0.6f, 0.5f, 0.4f }, true ); {
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Head" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Neck" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Chest" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Pelvis" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Arms" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Legs" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Feet" ) ) );

						awp_aimbot->add_element ( hitscan );
					}

					aimbot->add_element ( awp_aimbot );
				}

				auto auto_aimbot = std::make_shared< oxui::subtab > ( OSTR ( "Auto" ) ); {
					auto main = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.4f } ); {
						main->add_element ( main_switch );
						main->add_element ( optimization );
						main->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Inherit From" ), std::vector< oxui::str > { OSTR ( "None" ), OSTR ( "Default" ), OSTR ( "Pistol" ), OSTR ( "Revolver" ), OSTR ( "Rifle" ), OSTR ( "AWP" ), OSTR ( "Auto" ), OSTR ( "Scout" ) } ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Minimum Damage" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Hit Chance" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Doubletap Hit Chance" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Choke On Shot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Silent" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Shoot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Scope" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Slow" ) ) );
						main->add_element ( knife_bot );
						main->add_element ( zeus_bot );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim If Resolver Confidence Less Than" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Maximum Doubletap Ticks" ), 0.0, 0.0, 16.0 ) );
						main->add_element ( dt_key );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Doubletap Teleport" ) ) );

						auto_aimbot->add_element ( main );
					}

					auto_aimbot->add_element ( accuracy );

					auto target_selection = std::make_shared< oxui::group > ( OSTR ( "Hitscan" ), std::vector< float > { 0.0f, 0.6f, 0.5f, 0.4f }, false, true ); {
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If Lethal" ) ) );
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If In Air" ) ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim After X Misses" ), 0.0, 0.0, 4.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Head Pointscale" ), 0.0, 0.0, 100.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Body Pointscale" ), 0.0, 0.0, 100.0 ) );

						auto_aimbot->add_element ( target_selection );
					}

					auto hitscan = std::make_shared< oxui::group > ( OSTR ( "Hitboxes" ), std::vector< float > { 0.5f, 0.6f, 0.5f, 0.4f }, true ); {
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Head" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Neck" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Chest" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Pelvis" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Arms" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Legs" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Feet" ) ) );

						auto_aimbot->add_element ( hitscan );
					}

					aimbot->add_element ( auto_aimbot );
				}

				auto scout_aimbot = std::make_shared< oxui::subtab > ( OSTR ( "Scout" ) ); {
					auto main = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.4f } ); {
						main->add_element ( main_switch );
						main->add_element ( optimization );
						main->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Inherit From" ), std::vector< oxui::str > { OSTR ( "None" ), OSTR ( "Default" ), OSTR ( "Pistol" ), OSTR ( "Revolver" ), OSTR ( "Rifle" ), OSTR ( "AWP" ), OSTR ( "Auto" ), OSTR ( "Scout" ) } ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Minimum Damage" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Hit Chance" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Choke On Shot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Silent" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Shoot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Scope" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Slow" ) ) );
						main->add_element ( knife_bot );
						main->add_element ( zeus_bot );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim If Resolver Confidence Less Than" ), 0.0, 0.0, 100.0 ) );

						scout_aimbot->add_element ( main );
					}

					scout_aimbot->add_element ( accuracy );

					auto target_selection = std::make_shared< oxui::group > ( OSTR ( "Hitscan" ), std::vector< float > { 0.0f, 0.6f, 0.5f, 0.4f }, false, true ); {
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If Lethal" ) ) );
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If In Air" ) ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim After X Misses" ), 0.0, 0.0, 4.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Head Pointscale" ), 0.0, 0.0, 100.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Body Pointscale" ), 0.0, 0.0, 100.0 ) );

						scout_aimbot->add_element ( target_selection );
					}

					auto hitscan = std::make_shared< oxui::group > ( OSTR ( "Hitboxes" ), std::vector< float > { 0.5f, 0.6f, 0.5f, 0.4f }, true ); {
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Head" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Neck" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Chest" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Pelvis" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Arms" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Legs" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Feet" ) ) );

						scout_aimbot->add_element ( hitscan );
					}

					aimbot->add_element ( scout_aimbot );
				}

				window->add_tab ( aimbot );
			}

			auto antiaim = std::make_shared< oxui::tab > ( OSTR ( "B" ) ); {
				auto air_aa = std::make_shared< oxui::subtab > ( OSTR ( "Air" ) ); {
					auto base = std::make_shared< oxui::group > ( OSTR ( "Base" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.333f } ); {
						base->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "In Air" ) ) );
						base->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Pitch" ), std::vector< oxui::str > { OSTR ( "None" ), OSTR ( "Down" ), OSTR ( "Up" ), OSTR ( "Zero" ) } ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Yaw Offset" ), 0.0, 0.0, 360.0 ) );
						base->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Base Yaw" ), std::vector< oxui::str > { OSTR ( "Relative" ), OSTR ( "Absolute" ), OSTR ( "At Target" ), OSTR ( "Auto Direction" ) } ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Auto Direction Amount" ), 0.0, 0.0, 360.0 ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Auto Direction Range" ), 0.0, 0.0, 64.0 ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Jitter Range" ), 0.0, 0.0, 180.0 ) );

						air_aa->add_element ( base );
					}

					auto desync = std::make_shared< oxui::group > ( OSTR ( "Desync" ), std::vector< float > { 0.0f, 0.333f, 1.0f, 0.333f } ); {
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Desync" ) ) );
						desync->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Desync Range" ), 0.0, 0.0, 100.0 ) );
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Flip Desync Side" ) ) );
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Jitter Desync Side" ) ) );
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Center Real" ) ) );

						air_aa->add_element ( desync );
					}

					auto anti_hit = std::make_shared< oxui::group > ( OSTR ( "Anti-Hit" ), std::vector< float > { 0.0f, 0.666f, 1.0f, 0.333f } ); {
						anti_hit->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Anti-Bruteforce" ) ) );
						anti_hit->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Anti-Freestand Prediction" ) ) );

						air_aa->add_element ( anti_hit );
					}

					antiaim->add_element ( air_aa );
				}

				auto moving_aa = std::make_shared< oxui::subtab > ( OSTR ( "Moving" ) ); {
					auto base = std::make_shared< oxui::group > ( OSTR ( "Base" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.333f } ); {
						base->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "On Moving" ) ) );
						base->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Pitch" ), std::vector< oxui::str > { OSTR ( "None" ), OSTR ( "Down" ), OSTR ( "Up" ), OSTR ( "Zero" ) } ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Yaw Offset" ), 0.0, 0.0, 360.0 ) );
						base->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Base Yaw" ), std::vector< oxui::str > { OSTR ( "Relative" ), OSTR ( "Absolute" ), OSTR ( "At Target" ), OSTR ( "Auto Direction" ) } ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Auto Direction Amount" ), 0.0, 0.0, 360.0 ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Auto Direction Range" ), 0.0, 0.0, 64.0 ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Jitter Range" ), 0.0, 0.0, 180.0 ) );

						moving_aa->add_element ( base );
					}

					auto desync = std::make_shared< oxui::group > ( OSTR ( "Desync" ), std::vector< float > { 0.0f, 0.333f, 1.0f, 0.333f } ); {
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Desync" ) ) );
						desync->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Desync Range" ), 0.0, 0.0, 100.0 ) );
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Flip Desync Side" ) ) );
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Jitter Desync Side" ) ) );
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Center Real" ) ) );

						moving_aa->add_element ( desync );
					}

					auto anti_hit = std::make_shared< oxui::group > ( OSTR ( "Anti-Hit" ), std::vector< float > { 0.0f, 0.666f, 1.0f, 0.333f } ); {
						anti_hit->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Anti-Bruteforce" ) ) );
						anti_hit->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Anti-Freestand Prediction" ) ) );

						moving_aa->add_element ( anti_hit );
					}

					antiaim->add_element ( moving_aa );
				}

				auto slowwalk_aa = std::make_shared< oxui::subtab > ( OSTR ( "Slow Walk" ) ); {
					auto base = std::make_shared< oxui::group > ( OSTR ( "Base" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.333f } ); {
						base->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "On Slow Walk" ) ) );
						base->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Pitch" ), std::vector< oxui::str > { OSTR ( "None" ), OSTR ( "Down" ), OSTR ( "Up" ), OSTR ( "Zero" ) } ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Yaw Offset" ), 0.0, 0.0, 360.0 ) );
						base->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Base Yaw" ), std::vector< oxui::str > { OSTR ( "Relative" ), OSTR ( "Absolute" ), OSTR ( "At Target" ), OSTR ( "Auto Direction" ) } ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Auto Direction Amount" ), 0.0, 0.0, 360.0 ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Auto Direction Range" ), 0.0, 0.0, 64.0 ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Jitter Range" ), 0.0, 0.0, 180.0 ) );

						slowwalk_aa->add_element ( base );
					}

					auto desync = std::make_shared< oxui::group > ( OSTR ( "Desync" ), std::vector< float > { 0.0f, 0.333f, 1.0f, 0.333f } ); {
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Desync" ) ) );
						desync->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Desync Range" ), 0.0, 0.0, 100.0 ) );
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Flip Desync Side" ) ) );
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Jitter Desync Side" ) ) );
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Center Real" ) ) );

						slowwalk_aa->add_element ( desync );
					}

					auto anti_hit = std::make_shared< oxui::group > ( OSTR ( "Anti-Hit" ), std::vector< float > { 0.0f, 0.666f, 0.5f, 0.333f } ); {
						anti_hit->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Anti-Bruteforce" ) ) );
						anti_hit->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Anti-Freestand Prediction" ) ) );

						slowwalk_aa->add_element ( anti_hit );
					}

					auto slow_walk = std::make_shared< oxui::group > ( OSTR ( "Slow Walk" ), std::vector< float > { 0.5f, 0.666f, 0.5f, 0.333f } ); {
						slow_walk->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Slow Walk Speed" ), 0.0, 0.0, 100.0 ) );
						slow_walk->add_element ( std::make_shared< oxui::keybind > ( OSTR ( "Slow Walk Key" ) ) );

						slowwalk_aa->add_element ( slow_walk );
					}

					antiaim->add_element ( slowwalk_aa );
				}

				auto ground_aa = std::make_shared< oxui::subtab > ( OSTR ( "Standing" ) ); {
					auto base = std::make_shared< oxui::group > ( OSTR ( "Base" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.333f } ); {
						base->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "On Standing" ) ) );
						base->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Pitch" ), std::vector< oxui::str > { OSTR ( "None" ), OSTR ( "Down" ), OSTR ( "Up" ), OSTR ( "Zero" ) } ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Yaw Offset" ), 0.0, 0.0, 360.0 ) );
						base->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Base Yaw" ), std::vector< oxui::str > { OSTR ( "Relative" ), OSTR ( "Absolute" ), OSTR ( "At Target" ), OSTR ( "Auto Direction" ) } ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Auto Direction Amount" ), 0.0, 0.0, 360.0 ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Auto Direction Range" ), 0.0, 0.0, 64.0 ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Jitter Range" ), 0.0, 0.0, 180.0 ) );

						ground_aa->add_element ( base );
					}

					auto desync = std::make_shared< oxui::group > ( OSTR ( "Desync" ), std::vector< float > { 0.0f, 0.333f, 1.0f, 0.333f } ); {
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Desync" ) ) );
						desync->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Desync Range" ), 0.0, 0.0, 100.0 ) );
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Flip Desync Side" ) ) );
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Jitter Desync Side" ) ) );
						desync->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Center Real" ) ) );
						
						ground_aa->add_element ( desync );
					}

					auto anti_hit = std::make_shared< oxui::group > ( OSTR ( "Anti-Hit" ), std::vector< float > { 0.0f, 0.666f, 1.0f, 0.333f } ); {
						anti_hit->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Anti-Bruteforce" ) ) );
						anti_hit->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Anti-Freestand Prediction" ) ) );

						ground_aa->add_element ( anti_hit );
					}

					antiaim->add_element ( ground_aa );
				}

				auto exploits = std::make_shared< oxui::subtab > ( OSTR ( "Other" ) ); {
					auto fakelag = std::make_shared< oxui::group > ( OSTR ( "Fakelag" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.333f } ); {
						fakelag->add_element ( std::make_shared< oxui::slider > ( OSTR ( "In Air Fakelag" ), 0.0, 0.0, 16.0 ) );
						fakelag->add_element ( std::make_shared< oxui::slider > ( OSTR ( "In Air Send Ticks" ), 0.0, 0.0, 16.0 ) );
						fakelag->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Slow Walk Fakelag" ), 0.0, 0.0, 16.0 ) );
						fakelag->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Slow Walk Send Ticks" ), 0.0, 0.0, 16.0 ) );
						fakelag->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Moving Fakelag" ), 0.0, 0.0, 16.0 ) );
						fakelag->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Moving Send Ticks" ), 0.0, 0.0, 16.0 ) );
						fakelag->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Standing Fakelag" ), 0.0, 0.0, 16.0 ) );
						fakelag->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Standing Send Ticks" ), 0.0, 0.0, 16.0 ) );

						exploits->add_element ( fakelag );
					}

					auto other = std::make_shared< oxui::group > ( OSTR ( "Other" ), std::vector< float > { 0.0f, 0.333f, 1.0f, 0.333f } ); {
						other->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Fakeduck Mode" ), std::vector< oxui::str > { OSTR ( "None" ), OSTR ( "Normal" ), OSTR ( "Full" ) } ) );
						other->add_element ( std::make_shared< oxui::keybind > ( OSTR ( "Fakeduck Key" ) ) );
						other->add_element ( std::make_shared< oxui::keybind > ( OSTR ( "Left Side Key" ) ) );
						other->add_element ( std::make_shared< oxui::keybind > ( OSTR ( "Right Side Key" ) ) );
						other->add_element ( std::make_shared< oxui::keybind > ( OSTR ( "Back Side Key" ) ) );
						other->add_element ( std::make_shared< oxui::keybind > ( OSTR ( "Desync Flip Key" ) ) );

						exploits->add_element ( other );
					}

					antiaim->add_element ( exploits );
				}

				window->add_tab ( antiaim );
			}

			auto visuals = std::make_shared< oxui::tab > ( OSTR ( "C" ) ); {
				auto local_visuals = std::make_shared< oxui::subtab > ( OSTR ( "Local" ) ); {
					const auto chams_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Chams" ), oxui::color ( 155, 217, 249, 255 ) );
					const auto xqz_chams_picker = std::make_shared< oxui::color_picker > ( OSTR ( "XQZ Chams" ), oxui::color ( 246, 155, 249, 255 ) );
					const auto backtrack_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Backtrack" ), oxui::color ( 255, 255, 255, 50 ) );
					const auto hit_matrix_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Hit Matrix" ), oxui::color ( 190, 255, 156, 100 ) );
					const auto glow_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Glow" ), oxui::color ( 197, 104, 237, 121 ) );
					const auto rimlight_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Rimlight" ), oxui::color ( 255, 255, 255, 150 ) );
					const auto box_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Box" ), oxui::color ( 255, 255, 255, 162 ) );
					const auto health_bar_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Health Bar" ), oxui::color ( 129, 255, 56, 118 ) );
					const auto ammo_bar_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Ammo Bar" ), oxui::color ( 125, 233, 255, 118 ) );
					const auto desync_bar_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Desync Bar" ), oxui::color ( 221, 110, 255, 132 ) );
					const auto name_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Name" ), oxui::color ( 255, 255, 255, 200 ) );
					const auto weapon_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Weapon" ), oxui::color ( 255, 255, 255, 200 ) );
					const auto reflectivity = std::make_shared< oxui::slider > ( OSTR ( "Reflectivity" ), 0.0, 0.0, 100.0 );
					const auto phong = std::make_shared< oxui::slider > ( OSTR ( "Phong" ), 0.0, 0.0, 100.0 );

					auto designer = std::make_shared< oxui::group > ( OSTR ( "Visual Editor" ), std::vector< float > { 0.0f, 0.0f, 0.5f, 1.0f } ); {
						designer->add_element ( std::make_shared< oxui::visual_editor > (
							chams_picker,
							reflectivity,
							phong,
							backtrack_picker,
							hit_matrix_picker,
							xqz_chams_picker,
							glow_picker,
							rimlight_picker,
							box_picker,
							health_bar_picker,
							ammo_bar_picker,
							desync_bar_picker,
							name_picker,
							weapon_picker
							) );

						local_visuals->add_element ( designer );
					}

					auto colors = std::make_shared< oxui::group > ( OSTR ( "Options" ), std::vector< float > { 0.5f, 0.0f, 0.5f, 1.0f } ); {
						colors->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Show Fakelag On Desync Chams" ) ) );
						colors->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Desync Chams" ) ) );
						colors->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Rimlight Desync Chams" ) ) );
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Desync Chams Color" ), oxui::color ( 149, 245, 242, 255 ) ) );
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Rimlight Desync Color" ), oxui::color ( 221, 113, 245, 255 ) ) );
						colors->add_element ( reflectivity );
						colors->add_element ( phong );
						colors->add_element ( chams_picker );
						colors->add_element ( xqz_chams_picker );
						colors->add_element ( backtrack_picker );
						colors->add_element ( hit_matrix_picker );
						colors->add_element ( glow_picker );
						colors->add_element ( rimlight_picker );
						colors->add_element ( box_picker );
						colors->add_element ( health_bar_picker );
						colors->add_element ( ammo_bar_picker );
						colors->add_element ( desync_bar_picker );
						colors->add_element ( name_picker );
						colors->add_element ( weapon_picker );

						local_visuals->add_element ( colors );
					}

					visuals->add_element ( local_visuals );
				}

				auto enemy_visuals = std::make_shared< oxui::subtab > ( OSTR ( "Enemy" ) ); {
					const auto chams_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Chams" ), oxui::color ( 155, 217, 249, 255 ) );
					const auto xqz_chams_picker = std::make_shared< oxui::color_picker > ( OSTR ( "XQZ Chams" ), oxui::color ( 246, 155, 249, 255 ) );
					const auto backtrack_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Backtrack" ), oxui::color ( 255, 255, 255, 50 ) );
					const auto hit_matrix_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Hit Matrix" ), oxui::color ( 190, 255, 156, 100 ) );
					const auto glow_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Glow" ), oxui::color ( 197, 104, 237, 121 ) );
					const auto rimlight_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Rimlight" ), oxui::color ( 255, 255, 255, 150 ) );
					const auto box_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Box" ), oxui::color ( 255, 255, 255, 162 ) );
					const auto health_bar_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Health Bar" ), oxui::color ( 129, 255, 56, 118 ) );
					const auto ammo_bar_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Ammo Bar" ), oxui::color ( 125, 233, 255, 118 ) );
					const auto desync_bar_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Desync Bar" ), oxui::color ( 221, 110, 255, 132 ) );
					const auto name_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Name" ), oxui::color ( 255, 255, 255, 200 ) );
					const auto weapon_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Weapon" ), oxui::color ( 255, 255, 255, 200 ) );
					const auto reflectivity = std::make_shared< oxui::slider > ( OSTR ( "Reflectivity" ), 0.0, 0.0, 100.0 );
					const auto phong = std::make_shared< oxui::slider > ( OSTR ( "Phong" ), 0.0, 0.0, 100.0 );

					auto designer = std::make_shared< oxui::group > ( OSTR ( "Visual Editor" ), std::vector< float > { 0.0f, 0.0f, 0.5f, 1.0f } ); {
						designer->add_element ( std::make_shared< oxui::visual_editor > (
							chams_picker,
							reflectivity,
							phong,
							backtrack_picker,
							hit_matrix_picker,
							xqz_chams_picker,
							glow_picker,
							rimlight_picker,
							box_picker,
							health_bar_picker,
							ammo_bar_picker,
							desync_bar_picker,
							name_picker,
							weapon_picker
							) );

						enemy_visuals->add_element ( designer );
					}

					auto colors = std::make_shared< oxui::group > ( OSTR ( "Options" ), std::vector< float > { 0.5f, 0.0f, 0.5f, 1.0f } ); {
						colors->add_element ( reflectivity );
						colors->add_element ( phong );
						colors->add_element ( chams_picker );
						colors->add_element ( xqz_chams_picker );
						colors->add_element ( backtrack_picker );
						colors->add_element ( hit_matrix_picker );
						colors->add_element ( glow_picker );
						colors->add_element ( rimlight_picker );
						colors->add_element ( box_picker );
						colors->add_element ( health_bar_picker );
						colors->add_element ( ammo_bar_picker );
						colors->add_element ( desync_bar_picker );
						colors->add_element ( name_picker );
						colors->add_element ( weapon_picker );

						enemy_visuals->add_element ( colors );
					}

					visuals->add_element ( enemy_visuals );
				}

				auto team_visuals = std::make_shared< oxui::subtab > ( OSTR ( "Team" ) ); {
					const auto chams_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Chams" ), oxui::color ( 155, 217, 249, 255 ) );
					const auto xqz_chams_picker = std::make_shared< oxui::color_picker > ( OSTR ( "XQZ Chams" ), oxui::color ( 246, 155, 249, 255 ) );
					const auto backtrack_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Backtrack" ), oxui::color ( 255, 255, 255, 50 ) );
					const auto hit_matrix_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Hit Matrix" ), oxui::color ( 190, 255, 156, 100 ) );
					const auto glow_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Glow" ), oxui::color ( 197, 104, 237, 121 ) );
					const auto rimlight_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Rimlight" ), oxui::color ( 255, 255, 255, 150 ) );
					const auto box_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Box" ), oxui::color ( 255, 255, 255, 162 ) );
					const auto health_bar_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Health Bar" ), oxui::color ( 129, 255, 56, 118 ) );
					const auto ammo_bar_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Ammo Bar" ), oxui::color ( 125, 233, 255, 118 ) );
					const auto desync_bar_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Desync Bar" ), oxui::color ( 221, 110, 255, 132 ) );
					const auto name_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Name" ), oxui::color ( 255, 255, 255, 200 ) );
					const auto weapon_picker = std::make_shared< oxui::color_picker > ( OSTR ( "Weapon" ), oxui::color ( 255, 255, 255, 200 ) );
					const auto reflectivity = std::make_shared< oxui::slider > ( OSTR ( "Reflectivity" ), 0.0, 0.0, 100.0 );
					const auto phong = std::make_shared< oxui::slider > ( OSTR ( "Phong" ), 0.0, 0.0, 100.0 );

					auto designer = std::make_shared< oxui::group > ( OSTR ( "Visual Editor" ), std::vector< float > { 0.0f, 0.0f, 0.5f, 1.0f } ); {
						designer->add_element ( std::make_shared< oxui::visual_editor > (
							chams_picker,
							reflectivity,
							phong,
							backtrack_picker,
							hit_matrix_picker,
							xqz_chams_picker,
							glow_picker,
							rimlight_picker,
							box_picker,
							health_bar_picker,
							ammo_bar_picker,
							desync_bar_picker,
							name_picker,
							weapon_picker
							) );

						team_visuals->add_element ( designer );
					}

					auto colors = std::make_shared< oxui::group > ( OSTR ( "Options" ), std::vector< float > { 0.5f, 0.0f, 0.5f, 1.0f } ); {
						colors->add_element ( reflectivity );
						colors->add_element ( phong );
						colors->add_element ( chams_picker );
						colors->add_element ( xqz_chams_picker );
						colors->add_element ( backtrack_picker );
						colors->add_element ( hit_matrix_picker );
						colors->add_element ( glow_picker );
						colors->add_element ( rimlight_picker );
						colors->add_element ( box_picker );
						colors->add_element ( health_bar_picker );
						colors->add_element ( ammo_bar_picker );
						colors->add_element ( desync_bar_picker );
						colors->add_element ( name_picker );
						colors->add_element ( weapon_picker );

						team_visuals->add_element ( colors );
					}

					visuals->add_element ( team_visuals );
				}

				//auto chams = std::make_shared< oxui::subtab > ( OSTR ( "Chams" ) ); {
				//	auto settings = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.5f } ); {
				//		settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Chams" ) ) );
				//		settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Flat" ) ) );
				//		settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "XQZ" ) ) );
				//		settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Backtrack" ) ) );
				//		settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Hit Matrix" ) ) );
				//		settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Fake Matrix" ) ) );
				//
				//		chams->add_element ( settings );
				//	}
				//
				//	auto targets = std::make_shared< oxui::group > ( OSTR ( "Targets" ), std::vector< float > { 0.0f, 0.5f, 0.5f, 0.5f } ); {
				//		targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Team" ) ) );
				//		targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Enemy" ) ) );
				//		targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Local" ) ) );
				//
				//		chams->add_element ( targets );
				//	}
				//
				//	auto colors = std::make_shared< oxui::group > ( OSTR ( "Colors" ), std::vector< float > { 0.5f, 0.5f, 0.5f, 0.5f } ); {
				//		colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Chams" ), oxui::color ( 155, 255, 232, 45 ) ) );
				//		colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Hit Matrix" ), oxui::color ( 155, 255, 232, 45 ) ) );
				//		colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "XQZ" ), oxui::color ( 155, 163, 255, 45 ) ) );
				//		colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Backtrack" ), oxui::color ( 255, 255, 255, 255 ) ) );
				//		colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Fake" ), oxui::color ( 255, 0, 119, 45 ) ) );
				//		colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Reflected" ), oxui::color ( 255, 168, 5, 65 ) ) );
				//		colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Phong" ), oxui::color ( 255, 168, 5, 65 ) ) );
				//
				//		chams->add_element ( colors );
				//	}
				//
				//	visuals->add_element ( chams );
				//}

				//auto glow = std::make_shared< oxui::subtab > ( OSTR ( "Glow" ) ); {
				//	auto settings = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.5f } ); {
				//		settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Rimlight" ) ) );
				//		settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Glow" ) ) );
				//
				//		glow->add_element ( settings );
				//	}
				//
				//	auto targets = std::make_shared< oxui::group > ( OSTR ( "Targets" ), std::vector< float > { 0.0f, 0.5f, 0.5f, 0.5f } ); {
				//		targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Team" ) ) );
				//		targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Enemy" ) ) );
				//		targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Local" ) ) );
				//
				//		glow->add_element ( targets );
				//	}
				//
				//	auto colors = std::make_shared< oxui::group > ( OSTR ( "Colors" ), std::vector< float > { 0.5f, 0.5f, 0.5f, 0.5f } ); {
				//		colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Rimlight" ), oxui::color ( 132, 0, 255, 80 ) ) );
				//		colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Glow" ), oxui::color ( 115, 0, 255, 65 ) ) );
				//
				//		glow->add_element ( colors );
				//	}
				//
				//	visuals->add_element ( glow );
				//}

				auto other = std::make_shared< oxui::subtab > ( OSTR ( "Other" ) ); {
					auto settings = std::make_shared< oxui::group > ( OSTR ( "Removals" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.333f } ); {
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "No Smoke" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "No Flash" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "No Scope" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "No Aimpunch" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "No Viewpunch" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "No Zoom" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Skip Occlusion" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Preserve Killfeed" ) ) );
						settings->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Custom FOV" ), 90.0, 0.0, 180.0 ) );
						settings->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Custom Aspect Ratio" ), 1.0, 0.0, 2.0 ) );
						settings->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Viewmodel FOV" ), 68.0, 0.0, 180.0 ) );

						other->add_element ( settings );
					}

					auto world = std::make_shared< oxui::group > ( OSTR ( "World" ), std::vector< float > { 0.0f, 0.333f, 1.0f, 0.333f } ); {
						world->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Bullet Tracers" ) ) );
						world->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Bullet Impacts" ) ) );
						world->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Shot Logs" ) ) );
						world->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Grenade Trajectories" ) ) );
						world->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Grenade Bounces" ) ) );
						world->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Grenade Radii" ) ) );
						world->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Spread Circle" ) ) );
						world->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Gradient Spread Circle" ) ) );
						world->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Hit Sound" ), std::vector< oxui::str > { OSTR ( "None" ), OSTR ( "Arena Switch" ), OSTR ( "Fall Pain" ), OSTR ( "Bolt" ), OSTR ( "Neck Snap" ), OSTR ( "Power Switch" ), OSTR ( "Glass" ), OSTR ( "Bell" ), OSTR ( "COD" ), OSTR ( "Rattle" ) } ) );
						world->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Bullet Tracer" ), oxui::color ( 161, 66, 245, 255 ) ) );
						world->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Bullet Impact" ), oxui::color ( 201, 145, 250, 255 ) ) );
						world->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Grenade Trajectory" ), oxui::color ( 161, 66, 245, 255 ) ) );
						world->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Grenade Bounce" ), oxui::color ( 201, 145, 250, 255 ) ) );
						world->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Grenade Radius" ), oxui::color ( 245, 20, 20, 255 ) ) );
						world->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Spread Circle Color" ), oxui::color ( 245, 20, 20, 255 ) ) );
						world->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "World Color" ), oxui::color ( 255, 255, 255, 255 ) ) );
						world->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Prop Color" ), oxui::color ( 255, 255, 255, 255 ) ) );
						world->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Nade Path Fade Time" ), 0.0, 0.0, 20.0 ) );

						other->add_element ( world );
					}

					auto menu = std::make_shared< oxui::group > ( OSTR ( "GUI" ), std::vector< float > { 0.0f, 0.666f, 1.0f, 0.333f } ); {
						menu->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Watermark" ) ) );
						menu->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Keybind List" ) ) );
						menu->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Accent Color" ), oxui::theme.main ) );
						menu->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Secondary Accent Color" ), oxui::theme.title_bar_low ) );
						menu->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Logo Color" ), oxui::theme.logo ) );

						other->add_element ( menu );
					}

					visuals->add_element ( other );
				}

				window->add_tab ( visuals );
			}

			auto skins = std::make_shared< oxui::tab > ( OSTR ( "F" ) ); {
				auto weapon_skins = std::make_shared< oxui::subtab > ( OSTR ( "Skins" ) ); {
					auto item = std::make_shared< oxui::group > ( OSTR ( "Item" ), std::vector< float > { 0.0f, 0.0f, 0.5f, 1.0f } ); {
						weapon_skins->add_element ( item );
					}

					auto skin = std::make_shared< oxui::group > ( OSTR ( "Skin" ), std::vector< float > { 0.5f, 0.0f, 0.5f, 1.0f } ); {
						weapon_skins->add_element ( skin );
					}

					skins->add_element ( weapon_skins );
				}

				auto models = std::make_shared< oxui::subtab > ( OSTR ( "Models" ) ); {
					auto item = std::make_shared< oxui::group > ( OSTR ( "Item" ), std::vector< float > { 0.0f, 0.0f, 0.5f, 1.0f } ); {
						models->add_element ( item );
					}

					auto model = std::make_shared< oxui::group > ( OSTR ( "Model" ), std::vector< float > { 0.5f, 0.0f, 0.5f, 1.0f } ); {
						models->add_element ( model );
					}

					skins->add_element ( models );
				}

				window->add_tab ( skins );
			}

			auto misc = std::make_shared< oxui::tab > ( OSTR ( "E" ) ); {
				auto movement = std::make_shared< oxui::subtab > ( OSTR ( "Movement" ) ); {
					auto main = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 1.0f } ); {
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Bunnyhop" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Forward" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Strafer" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Omni-Directional Auto Strafer" ) ) );

						movement->add_element ( main );
					}

					misc->add_element ( movement );
				}

				auto effects = std::make_shared< oxui::subtab > ( OSTR ( "Effects" ) ); {
					auto main = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 1.0f } ); {
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Third Person" ) ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Third Person Range" ), 0.0, 0.0, 500.0 ) );
						main->add_element ( std::make_shared< oxui::keybind > ( OSTR ( "Third Person Key" ) ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Ragdoll Force" ), 1.0, 0.0, 25.0 ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Clantag" ) ) );
						main->add_element ( std::make_shared< oxui::textbox > ( OSTR ( "Clantag Text" ), OSTR ( "sesame.one" ) ) );
						main->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Clantag Animation" ), std::vector< oxui::str > { OSTR ( "Marquee" ), OSTR ( "Static" ), OSTR ( "Capitalize" ), OSTR ( "Heart" ) } ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Revolver Cock Volume" ), 1.0, 0.0, 1.0 ) ); /* Weapon_Revolver.Prepare */
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Weapon Volume" ), 1.0, 0.0, 1.0 ) ); /* Weapon_ */

						effects->add_element ( main );
					}

					misc->add_element ( effects );
				}

				auto other = std::make_shared< oxui::subtab > ( OSTR ( "Player List" ) ); {
					player_list = std::make_shared< oxui::group > ( OSTR ( "Players" ), std::vector< float > { 0.0f, 0.0f, 0.5f, 1.0f } ); {
						other->add_element ( player_list );
					}

					auto actions = std::make_shared< oxui::group > ( OSTR ( "Actions" ), std::vector< float > { 0.5f, 0.0f, 0.5f, 1.0f } ); {
						actions->add_element ( std::make_shared< oxui::button > ( OSTR ( "Steal Clantag" ), [ & ] ( ) {
							/* instantly refresh list */
							last_plist_refresh = 0;

							player_to_steal_tag_from = current_plist_player;
						} ) );

						actions->add_element ( std::make_shared< oxui::button > ( OSTR ( "Stop Clantag Stealer" ), [ & ] ( ) {
							/* instantly refresh list */
							last_plist_refresh = 0;

							player_to_steal_tag_from = 0;
						} ) );

						other->add_element ( actions );
					}

					misc->add_element ( other );
				}

				window->add_tab ( misc );
			}

			auto customization = std::make_shared< oxui::tab > ( OSTR ( "Customization" ) ); {
				auto configs = std::make_shared< oxui::subtab > ( OSTR ( "Configs" ) ); {
					config_list = std::make_shared< oxui::group > ( OSTR ( "Config List" ), std::vector< float > { 0.0f, 0.0f, 0.5f, 1.0f } ); {
						refresh_config_list ( );

						configs->add_element ( config_list );
					}

					auto actions = std::make_shared< oxui::group > ( OSTR ( "Actions" ), std::vector< float > { 0.5f, 0.0f, 0.5f, 1.0f } ); {
						actions->add_element ( std::make_shared< oxui::textbox > ( OSTR ( "Config Name" ), OSTR ( "default" ) ) );

						actions->add_element ( std::make_shared< oxui::button > ( OSTR ( "Save" ), [ & ] ( ) {
							OPTION ( oxui::str, cfg_name, "Sesame->Customization->Configs->Actions->Config Name", oxui::object_textbox );

							if ( !cfg_name.length ( ) )
								return;

							window->save_state ( cfg_name + OSTR ( ".json" ) );
							refresh_config_list ( );
						} ) );

						actions->add_element ( std::make_shared< oxui::button > ( OSTR ( "Load" ), [ & ] ( ) {
							OPTION ( oxui::str, cfg_name, "Sesame->Customization->Configs->Actions->Config Name", oxui::object_textbox );

							if ( !cfg_name.length ( ) )
								return;

							window->load_state ( cfg_name + OSTR ( ".json" ) );
						} ) );

						actions->add_element ( std::make_shared< oxui::button > ( OSTR ( "Refresh" ), [ & ] ( ) {
							refresh_config_list ( );
						} ) );

						actions->add_element ( std::make_shared< oxui::button > ( OSTR ( "Delete" ), [ & ] ( ) {
							OPTION ( oxui::str, cfg_name, "Sesame->Customization->Configs->Actions->Config Name", oxui::object_textbox );

							if ( !cfg_name.length ( ) )
								return;

							wchar_t appdata [ MAX_PATH ];

							if ( SUCCEEDED ( LI_FN ( SHGetFolderPathW )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
								LI_FN ( CreateDirectoryW )( ( std::wstring ( appdata ) + _ ( L"\\sesame" ) ).data ( ), nullptr );
								LI_FN ( CreateDirectoryW )( ( std::wstring ( appdata ) + _ ( L"\\sesame\\configs" ) ).data ( ), nullptr );
							}

							_wremove ( ( std::wstring ( appdata ) + _ ( L"\\sesame\\configs\\" ) + cfg_name + OSTR ( ".json" ) ).data ( ) );

							refresh_config_list ( );
						} ) );

						actions->add_element ( std::make_shared< oxui::button > ( OSTR ( "Load Testing Settings" ), [ & ] ( ) {
							csgo::i::engine->client_cmd_unrestricted ( _ ( "sv_cheats 1; bot_kick; bot_add_ct; mp_warmuptime 999999999; mp_warmup_start; give weapon_scar20; bot_add_ct; weapon_accuracy_nospread 1; bot_mimic 1; sv_showlagcompensation 1; sv_showimpacts 1" ) );
						} ) );

						configs->add_element ( actions );
					}

					customization->add_element ( configs );
				}

				auto lua = std::make_shared< oxui::subtab > ( OSTR ( "LUA Scripts" ) ); {
					auto main = std::make_shared< oxui::group > ( OSTR ( "LUA Script List" ), std::vector< float > { 0.0f, 0.0f, 0.5f, 1.0f } ); {
						
						lua->add_element ( main );
					}

					auto actions = std::make_shared< oxui::group > ( OSTR ( "Actions" ), std::vector< float > { 0.5f, 0.0f, 0.5f, 1.0f } ); {
						

						lua->add_element ( actions );
					}

					customization->add_element ( lua );
				}

				window->add_tab ( customization );
			}

			panel->add_window( window );
		}
	}

	bool first_selected = false;

	std::for_each ( window->objects.begin ( ), window->objects.end ( ), [ & ] ( std::shared_ptr< oxui::obj > object ) {
		if ( object->type == oxui::object_tab ) {
			auto as_tab = std::static_pointer_cast< oxui::tab >( object );
			as_tab->selected = false;

			if ( !first_selected ) {
				as_tab->selected = true;
				first_selected = true;
			}

			auto subtab_selected = false;

			std::for_each ( as_tab->objects.begin ( ), as_tab->objects.end ( ), [ & ] ( std::shared_ptr< oxui::obj > object ) {
				if ( object->type == oxui::object_subtab ) {
					auto as_subtab = std::static_pointer_cast< oxui::subtab >( object );
					as_subtab->selected = false;

					if ( !subtab_selected ) {
						as_subtab->selected = true;
						subtab_selected = true;
					}
				}
			} );
		}
	} );

	END_FUNC
}

void menu::draw( ) {
	//FIND ( bool, info_window, "settings", "menu settings", "info window", oxui::object_checkbox );
	//FIND ( oxui::color, bg, "settings", "menu settings", "background", oxui::object_colorpicker );
	//FIND ( oxui::color, text, "settings", "menu settings", "text", oxui::object_colorpicker );
	//FIND ( oxui::color, title_text, "settings", "menu settings", "title text", oxui::object_colorpicker );
	//FIND ( oxui::color, container, "settings", "menu settings", "container background", oxui::object_colorpicker );
	//FIND ( oxui::color, accent, "settings", "menu settings", "accent", oxui::object_colorpicker );
	//FIND ( oxui::color, title_bar, "settings", "menu settings", "title bar", oxui::object_colorpicker );
	//
	//oxui::theme.bg = bg;
	//oxui::theme.text = text;
	//oxui::theme.title_text = title_text;
	//oxui::theme.container_bg = container;
	//oxui::theme.main = accent;
	//oxui::theme.title_bar = title_bar;

	//info->open = info_window;

	refresh_player_list ( );

	///* darken the screen */
	//if ( window->open ) {
	//	int w, h;
	//	render::screen_size ( w, h );
	//	render::rectangle ( -2, -2, w + 2, h + 2, D3DCOLOR_RGBA( 0, 0, 0, 166 ) );
	//}

	security_handler::update ( );

	panel->render( static_cast< double > ( std::chrono::duration_cast< std::chrono::milliseconds > ( std::chrono::system_clock::now ( ).time_since_epoch ( ) ).count( ) ) / 1000.0 );
}