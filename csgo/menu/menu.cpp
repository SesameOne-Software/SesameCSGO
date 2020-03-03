#include <time.h>
#include "menu.hpp"
#include "../sdk/sdk.hpp"

std::shared_ptr< oxui::panel > panel;
std::shared_ptr< oxui::window > window;

bool menu::open( ) {
	return window->open;
}

void* menu::find_obj( const oxui::str& tab_name, const oxui::str& group_name, const oxui::str& object_name, oxui::object_type otype ) {
	return window->find_obj( tab_name, group_name, object_name, otype );
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
		_( L"Sun" ), _( L"Mon" ), _( L"Tue" ), _( L"Wed" ), _( L"Thu" ),_( L"Fri" ),_( L"Sat" )
	};

	static const wchar_t* mon_name [ ] = {
		_( L"Jan" ), _( L"Feb" ),  _( L"Mar" ), _( L"Apr" ),_( L"May" ), _( L"Jun" ),
		_( L"Jul" ), _( L"Aug" ), _( L"Sep" ), _( L"Oct" ), _( L"Nov" ),  _( L"Dec" )
	};

	static wchar_t result [ 26 ] { '\0' };

	wsprintfW( result, _( L"%s %d, %d\n" ), mon_name [ timeinfo->tm_mon ], timeinfo->tm_mday, 1900 + timeinfo->tm_year );

	return result;
}

void menu::init( ) {
	panel = std::make_shared< oxui::panel >( ); {
		window = std::make_shared< oxui::window >( oxui::rect( 200, 200, 700, 500 ), OSTR( "WeCheat" ) /* + oxui::str( cur_date( ) ) */ ); {
			window->bind_key( VK_INSERT );

			auto rage = std::make_shared< oxui::tab >( OSTR( "Rage" ) ); {
				auto aimbot = std::make_shared< oxui::group >( OSTR( "Aimbot" ) ); {
					aimbot->add_element( std::make_shared< oxui::checkbox >( OSTR( "Ragebot" ) ) );
					aimbot->add_element( std::make_shared< oxui::slider >( OSTR( "Min. Dmg" ), 0.0, 0.0, 100.0 ) );
					aimbot->add_element( std::make_shared< oxui::slider >( OSTR( "Hit Chance" ), 0.0, 0.0, 100.0 ) );
					aimbot->add_element( std::make_shared< oxui::slider >( OSTR( "Hit Chance Tolerance" ), 0.0, 0.0, 100.0 ) );
					aimbot->add_element( std::make_shared< oxui::checkbox >( OSTR( "Auto-Shoot" ) ) );
					aimbot->add_element( std::make_shared< oxui::checkbox >( OSTR( "Silent" ) ) );
					aimbot->add_element( std::make_shared< oxui::checkbox >( OSTR( "Auto-Scope" ) ) );
					aimbot->add_element( std::make_shared< oxui::dropdown >( OSTR( "Lag-Comp Mode" ), std::vector< oxui::str > { OSTR( "None" ), OSTR( "Delay" ), OSTR( "Predict" ), OSTR( "Simulate" ) } ) );

					rage->add_group( aimbot );
					rage->add_columns( 1 );
				}

				{
					auto target_selection = std::make_shared< oxui::group >( OSTR( "Target Selection" ) ); {
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR( "Head" ) ) );
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR( "Neck" ) ) );
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR( "Chest" ) ) );
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR( "Pelvis" ) ) );
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR( "Arms" ) ) );
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR( "Legs" ) ) );
						target_selection->add_element( std::make_shared< oxui::checkbox >( OSTR( "Feet" ) ) );

						rage->add_group( target_selection );
						rage->add_columns( 1 );
					}
				}

				window->add_tab( rage );
			}

			auto antiaim = std::make_shared< oxui::tab >( OSTR( "Anti-Aim" ) ); {
				auto air = std::make_shared< oxui::group >( OSTR( "Air" ) ); {
					air->add_element( std::make_shared< oxui::checkbox >( OSTR( "AA in Air" ) ) );
					air->add_element( std::make_shared< oxui::checkbox >( OSTR( "Desync" ) ) );
					air->add_element( std::make_shared< oxui::checkbox >( OSTR( "Desync Side" ) ) );
					air->add_element( std::make_shared< oxui::checkbox >( OSTR( "Jitter Desync" ) ) );
					air->add_element( std::make_shared< oxui::slider >( OSTR( "Lag" ), 0.0, 0.0, 16.0 ) );
					air->add_element( std::make_shared< oxui::slider >( OSTR( "Pitch" ), 0.0, -89.0, 89.0 ) );
					air->add_element( std::make_shared< oxui::slider >( OSTR( "Base Yaw" ), 0.0, 0.0, 360.0 ) );
					air->add_element( std::make_shared< oxui::slider >( OSTR( "Jitter Amount" ), 0.0, 0.0, 180.0 ) );

					antiaim->add_group( air );
					antiaim->add_columns( 1 );
				}

				auto moving = std::make_shared< oxui::group >( OSTR( "Moving" ) ); {
					moving->add_element( std::make_shared< oxui::checkbox >( OSTR( "AA on Move" ) ) );
					moving->add_element( std::make_shared< oxui::checkbox >( OSTR( "Desync" ) ) );
					moving->add_element( std::make_shared< oxui::checkbox >( OSTR( "Desync Side" ) ) );
					moving->add_element( std::make_shared< oxui::checkbox >( OSTR( "Jitter Desync" ) ) );
					moving->add_element( std::make_shared< oxui::slider >( OSTR( "Slow Walk Speed" ), 0.0, 0.0, 100.0 ) );
					moving->add_element( std::make_shared< oxui::slider >( OSTR( "Lag" ), 0.0, 0.0, 16.0 ) );
					moving->add_element( std::make_shared< oxui::slider >( OSTR( "Pitch" ), 0.0, -89.0, 89.0 ) );
					moving->add_element( std::make_shared< oxui::slider >( OSTR( "Base Yaw" ), 0.0, 0.0, 360.0 ) );
					moving->add_element( std::make_shared< oxui::slider >( OSTR( "Jitter Amount" ), 0.0, 0.0, 180.0 ) );

					antiaim->add_group( moving );
					antiaim->add_columns( 1 );
				}

				auto standing = std::make_shared< oxui::group >( OSTR( "Standing" ) ); {
					standing->add_element( std::make_shared< oxui::checkbox >( OSTR( "AA on Stand" ) ) );
					standing->add_element( std::make_shared< oxui::checkbox >( OSTR( "Desync" ) ) );
					standing->add_element( std::make_shared< oxui::checkbox >( OSTR( "Desync Side" ) ) );
					standing->add_element( std::make_shared< oxui::checkbox >( OSTR( "Jitter Desync" ) ) );
					standing->add_element( std::make_shared< oxui::slider >( OSTR( "Lag" ), 0.0, 0.0, 16.0 ) );
					standing->add_element( std::make_shared< oxui::slider >( OSTR( "Pitch" ), 0.0, -89.0, 89.0 ) );
					standing->add_element( std::make_shared< oxui::slider >( OSTR( "Base Yaw" ), 0.0, 0.0, 360.0 ) );
					standing->add_element( std::make_shared< oxui::slider >( OSTR( "Jitter Amount" ), 0.0, 0.0, 180.0 ) );

					antiaim->add_group( standing );
					antiaim->add_columns( 1 );
				}

				window->add_tab( antiaim );
			}

			auto legit = std::make_shared< oxui::tab >( OSTR( "Legit" ) ); {
				auto aimbot = std::make_shared< oxui::group >( OSTR( "Aimbot" ) ); {
					aimbot->add_element( std::make_shared< oxui::checkbox >( OSTR( "Legitbot" ) ) );

					legit->add_group( aimbot );
					legit->add_columns( 1 );
				}

				auto triggerbot = std::make_shared< oxui::group >( OSTR( "Triggerbot" ) ); {
					triggerbot->add_element( std::make_shared< oxui::checkbox >( OSTR( "Triggerbot" ) ) );

					legit->add_group( triggerbot );
					legit->add_columns( 1 );
				}

				window->add_tab( legit );
			}

			auto visuals = std::make_shared< oxui::tab >( OSTR( "Visuals" ) ); {
				{
					auto targets = std::make_shared< oxui::group >( OSTR( "Targets" ) ); {
						targets->add_element( std::make_shared< oxui::checkbox >( OSTR( "Team" ) ) );
						targets->add_element( std::make_shared< oxui::checkbox >( OSTR( "Enemy" ) ) );
						targets->add_element( std::make_shared< oxui::checkbox >( OSTR( "Local" ) ) );
						targets->add_element( std::make_shared< oxui::checkbox >( OSTR( "Weapon" ) ) );

						visuals->add_group( targets );
					}

					auto glow = std::make_shared< oxui::group >( OSTR( "Glow" ) ); {
						glow->add_element( std::make_shared< oxui::checkbox >( OSTR( "Glow" ) ) );
						glow->add_element( std::make_shared< oxui::checkbox >( OSTR( "Rim" ) ) );

						visuals->add_group( glow );
					}

					visuals->add_columns( 2 );
				}

				auto esp = std::make_shared< oxui::group >( OSTR( "ESP" ) ); {
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR( "ESP" ) ) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR( "Box" ) ) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR( "Health" ) ) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR( "Name" ) ) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR( "Weapon" ) ) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR( "Desync" ) ) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR( "979" ) ) );
					esp->add_element( std::make_shared< oxui::checkbox >( OSTR( "Lag-Comp" ) ) );

					visuals->add_group( esp );
					visuals->add_columns( 1 );
				}

				{
					auto chams = std::make_shared< oxui::group >( OSTR( "Chams" ) ); {
						chams->add_element( std::make_shared< oxui::checkbox >( OSTR( "Chams" ) ) );
						chams->add_element( std::make_shared< oxui::checkbox >( OSTR( "Flat" ) ) );
						chams->add_element( std::make_shared< oxui::checkbox >( OSTR( "XQZ" ) ) );
						chams->add_element( std::make_shared< oxui::checkbox >( OSTR( "Lag-Comp" ) ) );
						chams->add_element( std::make_shared< oxui::slider >( OSTR( "Reflectivity" ), 0.0, 0.0, 1.0 ) );
						chams->add_element( std::make_shared< oxui::slider >( OSTR( "Luminance" ), 0.0, 0.0, 1.0 ) );

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

			auto misc = std::make_shared< oxui::tab >( OSTR( "Misc." ) ); {
				auto movement = std::make_shared< oxui::group >( OSTR( "Movement" ) ); {
					movement->add_element( std::make_shared< oxui::checkbox >( OSTR( "Bhop" ) ) );
					movement->add_element( std::make_shared< oxui::checkbox >( OSTR( "Strafer" ) ) );
					movement->add_element( std::make_shared< oxui::checkbox >( OSTR( "Directional" ) ) );

					misc->add_group( movement );
					misc->add_columns( 1 );
				}

				auto effects = std::make_shared< oxui::group >( OSTR( "Effects" ) ); {
					effects->add_element( std::make_shared< oxui::checkbox >( OSTR( "Third-Person" ) ) );

					misc->add_group( effects );
					misc->add_columns( 1 );
				}

				window->add_tab( misc );
			}

			auto configs = std::make_shared< oxui::tab >( OSTR( "Settings" ) ); {
				auto configs_list = std::make_shared< oxui::group >( OSTR( "Config List" ) ); {
					configs->add_group( configs_list );
					configs->add_columns( 1 );
				}

				{
					auto config_controls = std::make_shared< oxui::group >( OSTR( "Config Actions" ) ); {
						config_controls->add_element( std::make_shared< oxui::checkbox >( OSTR( "Auto-Save" ) ) );
						config_controls->add_element( std::make_shared< oxui::button >( OSTR( "Save" ), [ & ] ( ) { window->save_state( OSTR( "wcdef.json" ) ); } ) );
						config_controls->add_element( std::make_shared< oxui::button >( OSTR( "Load" ), [ & ] ( ) { window->load_state( OSTR( "wcdef.json" ) ); } ) );

						configs->add_group( config_controls );
					}

					auto language_settings = std::make_shared< oxui::group >( OSTR( "Language Settings" ) ); {
						configs->add_group( language_settings );
					}

					configs->add_columns( 2 );
				}

				window->add_tab( configs );
			}

			auto scripts = std::make_shared< oxui::tab >( OSTR( "Scripts" ) ); {
				auto scripts_list = std::make_shared< oxui::group >( OSTR( "Scripts List" ) ); {
					scripts->add_group( scripts_list );
					scripts->add_columns( 1 );
				}

				auto script_controls = std::make_shared< oxui::group >( OSTR( "Script Controls" ) ); {
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