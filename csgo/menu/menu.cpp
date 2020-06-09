#include <time.h>
#include "menu.hpp"
#include "../sdk/sdk.hpp"
#include <ShlObj.h>
#include <filesystem>
#include <algorithm>
#include "../globals.hpp"

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
		window = std::make_shared< oxui::window >( oxui::rect( 200, 200, 650 * 0.87, 575 * 0.87 ), OSTR( "Sesame" ) /* + oxui::str( cur_date( ) ) */ ); {
			window->bind_key( VK_INSERT );

			auto aimbot = std::make_shared< oxui::tab > ( OSTR ( "A" ) ); {
				auto legit_aimbot = std::make_shared< oxui::subtab > ( OSTR ( "Legit Aimbot" ) ); {
					aimbot->add_element ( legit_aimbot );
				}

				auto rage_aimbot = std::make_shared< oxui::subtab > ( OSTR ( "Rage Aimbot" ) ); {
					auto main = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.4f } ); {
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Main Switch" ) ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Minimum Damage" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Hit Chance" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Doubletap Hit Chance" ), 0.0, 0.0, 100.0 ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Silent" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Shoot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Scope" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Slow" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Auto Revolver" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Knife Bot" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Zeus Bot" ) ) );
						main->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Maximum Doubletap Ticks" ), 0.0, 0.0, 16.0 ) );
						main->add_element ( std::make_shared< oxui::keybind > ( OSTR ( "Doubletap Key" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Doubletap Teleport" ) ) );

						rage_aimbot->add_element ( main );
					}

					auto accuracy = std::make_shared< oxui::group > ( OSTR ( "Accuracy" ), std::vector< float > { 0.0f, 0.4f, 1.0f, 0.2f } ); {
						accuracy->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Fix Fakelag" ) ) );
						//accuracy->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Predict Fakelag" ) ) );
						accuracy->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Resolve Desync" ) ) );
						//accuracy->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Beta Resolver" ) ) );
						accuracy->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Safe Point" ) ) );

						rage_aimbot->add_element ( accuracy );
					}

					auto target_selection = std::make_shared< oxui::group > ( OSTR ( "Hitscan" ), std::vector< float > { 0.0f, 0.6f, 0.5f, 0.4f }, false, true ); {
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If Lethal" ) ) );
						target_selection->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Baim If In Air" ) ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Baim After X Misses" ), 0.0, 0.0, 4.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Head Pointscale" ), 0.0, 0.0, 100.0 ) );
						target_selection->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Body Pointscale" ), 0.0, 0.0, 100.0 ) );

						rage_aimbot->add_element ( target_selection );
					}

					auto hitscan = std::make_shared< oxui::group > ( OSTR ( "Hitboxes" ), std::vector< float > { 0.5f, 0.6f, 0.5f, 0.4f }, true ); {
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Head" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Neck" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Chest" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Pelvis" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Arms" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Legs" ) ) );
						hitscan->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Feet" ) ) );

						rage_aimbot->add_element ( hitscan );
					}

					aimbot->add_element ( rage_aimbot );
				}

				window->add_tab ( aimbot );
			}

			auto antiaim = std::make_shared< oxui::tab > ( OSTR ( "B" ) ); {
				auto air_aa = std::make_shared< oxui::subtab > ( OSTR ( "Air" ) ); {
					auto base = std::make_shared< oxui::group > ( OSTR ( "Base" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.333f } ); {
						base->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "In Air" ) ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Base Pitch" ), 0.0, -89.0, 89.0 ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Yaw Offset" ), 0.0, 0.0, 360.0 ) );
						base->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Base Yaw" ), std::vector< oxui::str > { OSTR ( "Relative" ), OSTR ( "Absolute" ), OSTR ( "At Target" ), OSTR ( "Auto Direction" ) } ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Auto Direction Amount" ), 0.0, 0.0, 360.0 ) );
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
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Base Pitch" ), 0.0, -89.0, 89.0 ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Yaw Offset" ), 0.0, 0.0, 360.0 ) );
						base->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Base Yaw" ), std::vector< oxui::str > { OSTR ( "Relative" ), OSTR ( "Absolute" ), OSTR ( "At Target" ), OSTR ( "Auto Direction" ) } ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Auto Direction Amount" ), 0.0, 0.0, 360.0 ) );
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
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Base Pitch" ), 0.0, -89.0, 89.0 ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Yaw Offset" ), 0.0, 0.0, 360.0 ) );
						base->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Base Yaw" ), std::vector< oxui::str > { OSTR ( "Relative" ), OSTR ( "Absolute" ), OSTR ( "At Target" ), OSTR ( "Auto Direction" ) } ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Auto Direction Amount" ), 0.0, 0.0, 360.0 ) );
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
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Base Pitch" ), 0.0, -89.0, 89.0 ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Yaw Offset" ), 0.0, 0.0, 360.0 ) );
						base->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Base Yaw" ), std::vector< oxui::str > { OSTR ( "Relative" ), OSTR ( "Absolute" ), OSTR ( "At Target" ), OSTR ( "Auto Direction" ) } ) );
						base->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Auto Direction Amount" ), 0.0, 0.0, 360.0 ) );
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
				auto esp = std::make_shared< oxui::subtab > ( OSTR ( "ESP" ) ); {
					auto settings = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.5f } ); {
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "ESP" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Box" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Health Bar" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Name" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Weapon" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Desync Bar" ) ) );

						esp->add_element ( settings );
					}

					auto targets = std::make_shared< oxui::group > ( OSTR ( "Targets" ), std::vector< float > { 0.0f, 0.5f, 0.5f, 0.5f } ); {
						targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Team" ) ) );
						targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Enemy" ) ) );
						targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Local" ) ) );

						esp->add_element ( targets );
					}

					auto colors = std::make_shared< oxui::group > ( OSTR ( "Colors" ), std::vector< float > { 0.5f, 0.5f, 0.5f, 0.5f } ); {
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Box" ), oxui::color ( 156, 155, 255, 80 ) ) );
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Name" ), oxui::color ( 255, 255, 255, 255 ) ) );
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Health Bar" ), oxui::color ( 98, 255, 0, 255 ) ) );
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Desync Bar" ), oxui::color ( 79, 20, 255, 255 ) ) );
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Flag On" ), oxui::color ( 115, 0, 255, 65 ) ) );
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Flag Off" ), oxui::color ( 115, 0, 255, 65 ) ) );

						esp->add_element ( colors );
					}

					visuals->add_element ( esp );
				}

				auto chams = std::make_shared< oxui::subtab > ( OSTR ( "Chams" ) ); {
					auto settings = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.5f } ); {
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Chams" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Flat" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "XQZ" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Backtrack" ) ) );

						chams->add_element ( settings );
					}

					auto targets = std::make_shared< oxui::group > ( OSTR ( "Targets" ), std::vector< float > { 0.0f, 0.5f, 0.5f, 0.5f } ); {
						targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Team" ) ) );
						targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Enemy" ) ) );
						targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Local" ) ) );

						chams->add_element ( targets );
					}

					auto colors = std::make_shared< oxui::group > ( OSTR ( "Colors" ), std::vector< float > { 0.5f, 0.5f, 0.5f, 0.5f } ); {
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Chams" ), oxui::color ( 155, 255, 232, 45 ) ) );
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "XQZ" ), oxui::color ( 155, 163, 255, 45 ) ) );
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Backtrack" ), oxui::color ( 255, 255, 255, 255 ) ) );
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Fake" ), oxui::color ( 255, 0, 119, 45 ) ) );
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Reflected" ), oxui::color ( 255, 168, 5, 65 ) ) );
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Phong" ), oxui::color ( 255, 168, 5, 65 ) ) );

						chams->add_element ( colors );
					}

					visuals->add_element ( chams );
				}

				auto glow = std::make_shared< oxui::subtab > ( OSTR ( "Glow" ) ); {
					auto settings = std::make_shared< oxui::group > ( OSTR ( "Main" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.5f } ); {
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Rimlight" ) ) );
						settings->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Glow" ) ) );

						glow->add_element ( settings );
					}

					auto targets = std::make_shared< oxui::group > ( OSTR ( "Targets" ), std::vector< float > { 0.0f, 0.5f, 0.5f, 0.5f } ); {
						targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Team" ) ) );
						targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Enemy" ) ) );
						targets->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Local" ) ) );

						glow->add_element ( targets );
					}

					auto colors = std::make_shared< oxui::group > ( OSTR ( "Colors" ), std::vector< float > { 0.5f, 0.5f, 0.5f, 0.5f } ); {
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Rimlight" ), oxui::color ( 132, 0, 255, 80 ) ) );
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "Glow" ), oxui::color ( 115, 0, 255, 65 ) ) );

						glow->add_element ( colors );
					}

					visuals->add_element ( glow );
				}

				auto other = std::make_shared< oxui::subtab > ( OSTR ( "Other" ) ); {
					auto settings = std::make_shared< oxui::group > ( OSTR ( "Removals" ), std::vector< float > { 0.0f, 0.0f, 1.0f, 0.5f } ); {
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

					auto colors = std::make_shared< oxui::group > ( OSTR ( "World" ), std::vector< float > { 0.0f, 0.5f, 1.0f, 0.5f } ); {
						colors->add_element ( std::make_shared< oxui::color_picker > ( OSTR ( "World Color" ), oxui::color ( 255, 255, 255, 255 ) ) );
						colors->add_element ( std::make_shared< oxui::slider > ( OSTR ( "Prop Alpha" ), 255.0, 0.0, 255.0 ) );

						other->add_element ( colors );
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
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Autostrafer" ) ) );
						main->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Omni-Directional Autostrafer" ) ) );

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
						main->add_element ( std::make_shared< oxui::dropdown > ( OSTR ( "Clantag Animation" ), std::vector< oxui::str > { OSTR ( "Static" ), OSTR ( "Marquee" ), OSTR ( "Capitalize" ) } ) );

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

	/* darken the screen */
	if ( window->open ) {
		int w, h;
		render::screen_size ( w, h );
		render::rectangle ( -2, -2, w + 2, h + 2, D3DCOLOR_RGBA( 0, 0, 0, 166 ) );
	}

	security_handler::update ( );

	panel->render( static_cast< double > ( std::chrono::duration_cast< std::chrono::milliseconds > ( std::chrono::system_clock::now ( ).time_since_epoch ( ) ).count( ) ) / 1000.0 );
}