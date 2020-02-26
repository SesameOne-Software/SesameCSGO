#include <time.h>
#include "menu.hpp"
#include "../sdk/sdk.hpp"

std::shared_ptr< oxui::panel > panel;
std::shared_ptr< oxui::window > window;

bool menu::open( ){
	return window->open;
}

void* menu::find_obj( const oxui::str& tab_name, const oxui::str& group_name, const oxui::str& object_name, oxui::object_type otype ) {
	return window->find_obj( tab_name, group_name, object_name, otype );
}

long __stdcall menu::wndproc( HWND hwnd, std::uint32_t msg, std::uintptr_t wparam, std::uint32_t lparam ) {
	return window->wndproc( hwnd, msg, wparam, lparam );
}

void menu::load_default( ) {
	window->load_state( OSTR("wcdef.json") );
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

	static const wchar_t* wday_name[ ] = {
		L"sun", L"mon", L"tue", L"wed", L"thu", L"fri", L"sat"
	};

	static const wchar_t* mon_name[ ] = {
		L"jan", L"feb",  L"mar",  L"apr",  L"may",  L"jun",
		L"jul", L"aug",  L"sep",  L"oct",  L"nov",  L"dec"
	};

	static wchar_t result [ 26 ] { '\0' };

	wsprintfW( result, L"%s %d, %d\n", mon_name [ timeinfo->tm_mon ], timeinfo->tm_mday, 1900 + timeinfo->tm_year );

	return result;
}

void menu::init( ) {
	panel = std::make_shared< oxui::panel >( ); {
		window = std::make_shared< oxui::window >( oxui::rect( 200, 200, 550, 425 ), OSTR("WeCheat | ") + oxui::str( cur_date( ) ) ); {
			window->bind_key( VK_INSERT );

			auto rage = std::make_shared< oxui::tab >( OSTR( "rage") ); {
				auto aimbot = std::make_shared< oxui::group >( OSTR( "aimbot") ); {
					aimbot->add_element( std::make_shared< oxui::checkbox >( OSTR( "ragebot") ) );
					aimbot->add_element( std::make_shared< oxui::slider >( OSTR( "min dmg" ), 0.0, 0.0, 100.0 ) );
					aimbot->add_element( std::make_shared< oxui::slider >( OSTR( "hit chance" ), 0.0, 0.0, 100.0 ) );
					aimbot->add_element( std::make_shared< oxui::checkbox >( OSTR( "auto-shoot" ) ) );
					aimbot->add_element( std::make_shared< oxui::checkbox >( OSTR( "silent" ) ) );
					aimbot->add_element( std::make_shared< oxui::checkbox >( OSTR( "auto-scope" ) ) );
					aimbot->add_element( std::make_shared< oxui::checkbox >( OSTR( "lagcomp") ) );

					rage->add_group( aimbot );
					rage->add_columns( 1 );
				}

				{
					auto target_selection = std::make_shared< oxui::group >( OSTR( "target selection" )); {
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR("head" )) );
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR("neck" )) );
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR("chest") ) );
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR("pelvis" )) );
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR("arms" )) );
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR("legs") ) );
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR("feet") ) );

						rage->add_group( target_selection );
						rage->add_columns( 1 );
					}
				}

				window->add_tab( rage );
			}

			auto antiaim = std::make_shared< oxui::tab >( OSTR( "antiaim") ); {
				auto air = std::make_shared< oxui::group >( OSTR( "air") ); {
					air->add_element( std::make_shared< oxui::checkbox >( OSTR( "aa on air" ) ) );
					air->add_element( std::make_shared< oxui::checkbox >( OSTR( "desync" ) ) );
					air->add_element( std::make_shared< oxui::checkbox >( OSTR( "desync side" ) ) );
					air->add_element( std::make_shared< oxui::checkbox >( OSTR( "jitter desync" ) ) );
					air->add_element( std::make_shared< oxui::slider >( OSTR( "lag" ), 0.0, 0.0, 16.0 ) );
					air->add_element( std::make_shared< oxui::slider >( OSTR( "pitch" ), 0.0, -89.0, 89.0 ) );
					air->add_element( std::make_shared< oxui::slider >( OSTR( "base yaw" ), 0.0, 0.0, 360.0 ) );
					air->add_element( std::make_shared< oxui::slider >( OSTR( "jitter amount" ), 0.0, 0.0, 180.0 ) );

					antiaim->add_group( air );
					antiaim->add_columns( 1 );
				}

				auto moving = std::make_shared< oxui::group >( OSTR( "moving" )); {
					moving->add_element( std::make_shared< oxui::checkbox >( OSTR( "aa on move") ) );
					moving->add_element( std::make_shared< oxui::checkbox >( OSTR( "desync" ) ) );
					moving->add_element( std::make_shared< oxui::checkbox >( OSTR( "desync side" ) ) );
					moving->add_element( std::make_shared< oxui::checkbox >( OSTR( "jitter desync" ) ) );
					moving->add_element( std::make_shared< oxui::slider >( OSTR( "slow walk speed" ), 0.0, 0.0, 100.0 ) );
					moving->add_element( std::make_shared< oxui::slider >( OSTR( "lag" ), 0.0, 0.0, 16.0 ) );
					moving->add_element( std::make_shared< oxui::slider >( OSTR( "pitch" ), 0.0, -89.0, 89.0 ) );
					moving->add_element( std::make_shared< oxui::slider >( OSTR( "base yaw" ), 0.0, 0.0, 360.0 ) );
					moving->add_element( std::make_shared< oxui::slider >( OSTR( "jitter amount" ), 0.0, 0.0, 180.0 ) );

					antiaim->add_group( moving );
					antiaim->add_columns( 1 );
				}

				auto standing = std::make_shared< oxui::group >( OSTR( "standing") ); {
					standing->add_element( std::make_shared< oxui::checkbox >( OSTR( "aa on stand") ) );
					standing->add_element( std::make_shared< oxui::checkbox >( OSTR( "desync" ) ) );
					standing->add_element( std::make_shared< oxui::checkbox >( OSTR( "desync side" ) ) );
					standing->add_element( std::make_shared< oxui::checkbox >( OSTR( "jitter desync" ) ) );
					standing->add_element( std::make_shared< oxui::slider >( OSTR( "lag" ), 0.0, 0.0, 16.0 ) );
					standing->add_element( std::make_shared< oxui::slider >( OSTR( "pitch" ), 0.0, -89.0, 89.0 ) );
					standing->add_element( std::make_shared< oxui::slider >( OSTR( "base yaw" ), 0.0, 0.0, 360.0 ) );
					standing->add_element( std::make_shared< oxui::slider >( OSTR( "jitter amount" ), 0.0, 0.0, 180.0 ) );

					antiaim->add_group( standing );
					antiaim->add_columns( 1 );
				}

				window->add_tab( antiaim );
			}

			auto legit = std::make_shared< oxui::tab >( OSTR( "legit") ); {
				auto aimbot = std::make_shared< oxui::group >( OSTR( "aimbot") ); {
					aimbot->add_element( std::make_shared< oxui::checkbox >( OSTR( "legitbot") ) );

					legit->add_group( aimbot );
					legit->add_columns( 1 );
				}

				auto triggerbot = std::make_shared< oxui::group >( OSTR( "triggerbot") ); {
					triggerbot->add_element( std::make_shared< oxui::checkbox >( OSTR( "triggerbot") ) );

					legit->add_group( triggerbot );
					legit->add_columns( 1 );
				}

				window->add_tab( legit );
			}

			auto visuals = std::make_shared< oxui::tab >( OSTR( "visuals") ); {
				{
					auto targets = std::make_shared< oxui::group >( OSTR( "targets") ); {
						targets->add_element( std::make_shared< oxui::checkbox >( OSTR("team") ) );
						targets->add_element( std::make_shared< oxui::checkbox >( OSTR("enemy") ) );
						targets->add_element( std::make_shared< oxui::checkbox >( OSTR("local") ) );
						targets->add_element( std::make_shared< oxui::checkbox >( OSTR("weapon") ) );

						visuals->add_group( targets );
					}

					auto glow = std::make_shared< oxui::group >( OSTR( "glow" )); {
						glow->add_element( std::make_shared< oxui::checkbox >( OSTR("glow" )) );
						glow->add_element( std::make_shared< oxui::checkbox >( OSTR("rim" )) );

						visuals->add_group( glow );
					}

					visuals->add_columns( 2 );
				}

				auto esp = std::make_shared< oxui::group >( OSTR( "esp") ); {
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR("esp" )) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR("box" )) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR("health" )) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR("name" )) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR("weapon" )) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR("desync" )) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR( "979" ) ) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR("lagcomp") ) );

					visuals->add_group( esp );
					visuals->add_columns( 1 );
				}

				{
					auto chams = std::make_shared< oxui::group >( OSTR( "chams") ); {
						chams->add_element( std::make_shared< oxui::checkbox >( OSTR( "chams" ) ) );
						chams->add_element( std::make_shared< oxui::checkbox >( OSTR( "flat" ) ) );
						chams->add_element( std::make_shared< oxui::checkbox >(OSTR("xqz") ) );
						chams->add_element( std::make_shared< oxui::checkbox >( OSTR( "lagcomp" ) ) );
						chams->add_element( std::make_shared< oxui::slider >( OSTR( "reflectivity" ), 0.0, 0.0, 1.0 ) );
						chams->add_element( std::make_shared< oxui::slider >( OSTR( "luminance" ), 0.0, 0.0, 1.0 ) );

						visuals->add_group( chams );
					}

					/*
					auto world = std::make_shared< oxui::group >( OSTR( "world" )); {
						world->add_element( std::make_shared< oxui::checkbox >( OSTR( "night") ) );

						visuals->add_group( world );
					}
					*/

					visuals->add_columns( 1 );
				}

				window->add_tab( visuals );
			}

			auto misc = std::make_shared< oxui::tab >( OSTR( "misc") ); {
				auto movement = std::make_shared< oxui::group >( OSTR( "movement" )); {
					movement->add_element( std::make_shared< oxui::checkbox >( OSTR("bhop" )) );
					movement->add_element( std::make_shared< oxui::checkbox >( OSTR("strafer" )) );
					movement->add_element( std::make_shared< oxui::checkbox >( OSTR("directional") ) );

					misc->add_group( movement );
					misc->add_columns( 1 );
				}

				auto effects = std::make_shared< oxui::group >( OSTR( "effects") ); {
					effects->add_element( std::make_shared< oxui::checkbox >( OSTR( "third-person" ) ) );

					misc->add_group( effects );
					misc->add_columns( 1 );
				}

				window->add_tab( misc );
			}

			auto configs = std::make_shared< oxui::tab >( OSTR( "settings") ); {
				auto configs_list = std::make_shared< oxui::group >( OSTR( "config list") ); {
					configs->add_group( configs_list );
					configs->add_columns( 1 );
				}

				{
					auto config_controls = std::make_shared< oxui::group >( OSTR( "config actions" ) ); {
						config_controls->add_element( std::make_shared< oxui::checkbox >( OSTR( "auto-save" ) ) );
						config_controls->add_element( std::make_shared< oxui::button >( OSTR( "save" ), [ & ] ( ) { window->save_state( OSTR( "wcdef.json" ) ); } ) );
						config_controls->add_element( std::make_shared< oxui::button >( OSTR( "load" ), [ & ] ( ) { window->load_state( OSTR( "wcdef.json" ) ); } ) );

						configs->add_group( config_controls );
					}

					auto language_settings = std::make_shared< oxui::group >( OSTR( "language settings" ) ); {
						language_settings->add_element( std::make_shared< oxui::checkbox >( OSTR( "简体中文" ) ) );
						language_settings->add_element( std::make_shared< oxui::checkbox >( OSTR( "日本語" ) ) );

						configs->add_group( language_settings );
					}

					configs->add_columns( 2 );
				}

				window->add_tab( configs );
			}

			auto scripts = std::make_shared< oxui::tab >( OSTR( "scripts" )); {
				auto scripts_list = std::make_shared< oxui::group >( OSTR( "scripts list" )); {
					scripts->add_group( scripts_list );
					scripts->add_columns( 1 );
				}

				auto script_controls = std::make_shared< oxui::group >( OSTR( "script controls" )); {
					scripts->add_group( script_controls );
					scripts->add_columns( 1 );
				}

				window->add_tab( scripts );
			}

			panel->add_window( window );
		}
	}
}

void menu::draw( ) {
	panel->render( csgo::i::globals->m_curtime );
}