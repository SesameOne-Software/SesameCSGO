#include "window.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"
#include "tab.hpp"
#include "../json/cJSON.h"
#include <string>
#include <fstream>

/* menu objects */
#include "checkbox.hpp"
#include "slider.hpp"
#include "dropdown.hpp"
#include "textbox.hpp"
#include "keybind.hpp"
#include "colorpicker.hpp"
#include "subtab.hpp"
#include "visual_editor.hpp"
#include <ShlObj.h>
#include <sstream>

std::vector< oxui::str > split ( const oxui::str& str, const oxui::str& delim ) {
	std::vector< oxui::str > tokens;
	size_t prev = 0, pos = 0;

	do {
		pos = str.find ( delim, prev );

		if ( pos == oxui::str::npos )
			pos = str.length ( );

		oxui::str token = str.substr ( prev, pos - prev );

		if ( !token.empty ( ) )
			tokens.push_back ( token );

		prev = pos + delim.length ( );
	} while ( pos < str.length ( ) && prev < str.length ( ) );

	return tokens;
}

void* oxui::window::find_obj( const str& option, object_type type ) {
	const auto tree = split ( option, OSTR( "->" ) );

	if ( tree.size( ) < 5 )
		return nullptr;

	if ( tree [ 0 ] != OSTR ( "Sesame" ) )
		return nullptr;

	for ( auto& _tab : objects ) {
		auto tab = std::static_pointer_cast< oxui::tab >( _tab );

		if ( tab->title == tree[ 1 ] ) {
			for ( auto& _subtab : tab->objects ) {
				auto subtab = std::static_pointer_cast< oxui::subtab >( _subtab );

				if ( subtab->title == tree [ 2 ] ) {
					for ( auto& _group : subtab->objects ) {
						auto group = std::static_pointer_cast< oxui::group >( _group );

						if ( group->title == tree [ 3 ] ) {
							for ( auto& _control : group->objects ) {
								if ( _control->type == type ) {
									switch ( _control->type ) {
									case object_checkbox: {
										auto as_checkbox = std::static_pointer_cast< checkbox >( _control );

										if ( as_checkbox->label == tree [ 4 ] )
											return &as_checkbox->checked;

										break;
									}
									case object_slider: {
										auto as_slider = std::static_pointer_cast< slider >( _control );

										if ( as_slider->label == tree [ 4 ] )
											return &as_slider->value;

										break;
									}
									case object_dropdown: {
										auto as_dropdown = std::static_pointer_cast< dropdown >( _control );

										if ( as_dropdown->label == tree [ 4 ] )
											return &as_dropdown->value;

										break;
									}
									case object_textbox: {
										auto as_textbox = std::static_pointer_cast< textbox >( _control );

										if ( as_textbox->label == tree [ 4 ] )
											return &as_textbox->buf;

										break;
									}
									case object_keybind: {
										auto as_keybind = std::static_pointer_cast< keybind >( _control );

										if ( as_keybind->label == tree [ 4 ] )
											return as_keybind.get ( );

										break;
									}
									case object_colorpicker: {
										auto as_colorpicker = std::static_pointer_cast< color_picker >( _control );

										if ( as_colorpicker->label == tree [ 4 ] )
											return &as_colorpicker->clr;

										break;
									}
									case object_visual_editor: {
										auto as_visual_editor = std::static_pointer_cast< visual_editor >( _control );

										return &as_visual_editor->settings;
										break;
									}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return nullptr;
}

void oxui::window::save_state( const str& file ) {
	auto save_esp_widget = [ ] ( cJSON* editor, const esp_widget_t& widget, const char* widget_name ) {
		const auto jwidget = cJSON_CreateObject ( );

		cJSON_AddItemToObject ( jwidget, _ ( "Queued Location" ), cJSON_CreateNumber ( static_cast< int > ( widget.queued_location ) ) );
		cJSON_AddItemToObject ( jwidget, _ ( "Location" ), cJSON_CreateNumber ( static_cast< int > ( widget.location ) ) );
		cJSON_AddItemToObject ( jwidget, _ ( "Type" ), cJSON_CreateNumber ( static_cast< int > ( widget.type ) ) );
		cJSON_AddItemToObject ( jwidget, _ ( "Enabled" ), cJSON_CreateBool( widget.enabled ) );
		cJSON_AddItemToObject ( editor, widget_name, jwidget );
	};

	const auto json = cJSON_CreateObject ( );

	auto window_str = std::string( title.begin( ), title.end( ) );

	const auto window = cJSON_CreateObject ( );

	std::for_each ( objects.begin ( ), objects.end ( ), [ & ] ( std::shared_ptr< obj >& tab_obj ) {
		auto tab = std::static_pointer_cast< oxui::tab >( tab_obj );
		auto tab_str = std::string ( tab->title.begin ( ), tab->title.end ( ) );

		const auto jtab = cJSON_CreateObject ( );

		std::for_each ( tab->objects.begin ( ), tab->objects.end ( ), [ & ] ( std::shared_ptr< obj >& subtab_obj ) {
			auto subtab = std::static_pointer_cast< oxui::subtab >( subtab_obj );
			auto subtab_str = std::string ( subtab->title.begin ( ), subtab->title.end ( ) );

			const auto jsubtab = cJSON_CreateObject ( );

			std::for_each ( subtab->objects.begin ( ), subtab->objects.end ( ), [ & ] ( std::shared_ptr< obj >& group_obj ) {
				auto group = std::static_pointer_cast< oxui::group >( group_obj );
				auto group_str = std::string ( group->title.begin ( ), group->title.end ( ) );

				const auto jgroup = cJSON_CreateObject ( );

				std::for_each ( group->objects.begin ( ), group->objects.end ( ), [ & ] ( std::shared_ptr< obj >& object ) {
					switch ( object->type ) {
					case object_checkbox: {
						auto as_checkbox = ( ( std::shared_ptr< checkbox >& ) object );
						auto obj_str = std::string ( as_checkbox->label.begin ( ), as_checkbox->label.end ( ) );
						cJSON_AddItemToObject ( jgroup, obj_str.c_str ( ), cJSON_CreateBool ( as_checkbox->checked ) );
						break;
					}
					case object_slider: {
						auto as_slider = ( ( std::shared_ptr< slider >& ) object );
						auto obj_str = std::string ( as_slider->label.begin ( ), as_slider->label.end ( ) );
						cJSON_AddItemToObject ( jgroup, obj_str.c_str ( ), cJSON_CreateNumber ( as_slider->value ) );
						break;
					}
					case object_dropdown: {
						auto as_dropdown = ( ( std::shared_ptr< dropdown >& ) object );
						auto obj_str = std::string ( as_dropdown->label.begin ( ), as_dropdown->label.end ( ) );
						cJSON_AddItemToObject ( jgroup, obj_str.c_str ( ), cJSON_CreateNumber ( as_dropdown->value ) );
						break;
					}
										//case object_textbox: {
										//	auto as_textbox = ( ( std::shared_ptr< textbox >& ) object );
										//	auto obj_str = std::string ( as_textbox->label.begin ( ), as_textbox->label.end ( ) );
										//	json [ window_str ][ tab_str ][ group_str ][ obj_str ] = as_textbox->buf;
										//	break;
										//}
					case object_keybind: {
						auto as_keybind = ( ( std::shared_ptr< keybind >& ) object );
						auto obj_str = std::string ( as_keybind->label.begin ( ), as_keybind->label.end ( ) );
						const auto keybind_data_arr = cJSON_CreateArray ( );
						cJSON_AddItemToArray ( keybind_data_arr, cJSON_CreateNumber ( as_keybind->key ) );
						cJSON_AddItemToArray ( keybind_data_arr, cJSON_CreateNumber ( static_cast< int > ( as_keybind->mode ) ) );
						cJSON_AddItemToObject ( jgroup, obj_str.c_str ( ), keybind_data_arr );
						break;
					}
					case object_colorpicker: {
						auto as_colorpicker = ( ( std::shared_ptr< color_picker >& ) object );
						auto obj_str = std::string ( as_colorpicker->label.begin ( ), as_colorpicker->label.end ( ) );
						const auto clr_arr = cJSON_CreateArray ( );
						cJSON_AddItemToArray ( clr_arr, cJSON_CreateNumber ( as_colorpicker->clr.r ) );
						cJSON_AddItemToArray ( clr_arr, cJSON_CreateNumber ( as_colorpicker->clr.g ) );
						cJSON_AddItemToArray ( clr_arr, cJSON_CreateNumber ( as_colorpicker->clr.b ) );
						cJSON_AddItemToArray ( clr_arr, cJSON_CreateNumber ( as_colorpicker->clr.a ) );
						cJSON_AddItemToObject ( jgroup, obj_str.c_str ( ), clr_arr );
						break;
					}
					case object_visual_editor: {
						const auto jvisual_editor = cJSON_CreateObject ( );
						auto as_visual_editor = ( ( std::shared_ptr< visual_editor >& ) object );

						cJSON_AddItemToObject ( jvisual_editor, _ ( "Model Type" ), cJSON_CreateNumber ( static_cast< int >( as_visual_editor->settings.model_type ) ) );
						cJSON_AddItemToObject ( jvisual_editor, _ ( "XQZ Chams" ), cJSON_CreateBool ( as_visual_editor->settings.xqz ) );
						cJSON_AddItemToObject ( jvisual_editor, _ ( "Glow" ), cJSON_CreateBool ( as_visual_editor->settings.glow ) );
						cJSON_AddItemToObject ( jvisual_editor, _ ( "Rimlight Overlay" ), cJSON_CreateBool ( as_visual_editor->settings.rimlight ) );
						cJSON_AddItemToObject ( jvisual_editor, _ ( "Backtrack Chams" ), cJSON_CreateBool ( as_visual_editor->settings.backtrack ) );
						cJSON_AddItemToObject ( jvisual_editor, _ ( "Hit Matrix" ), cJSON_CreateBool ( as_visual_editor->settings.hit_matrix ) );
						cJSON_AddItemToObject ( jvisual_editor, _ ( "Show Value" ), cJSON_CreateBool ( as_visual_editor->settings.show_value ) );

						save_esp_widget ( jvisual_editor, as_visual_editor->settings.esp_box, _ ( "ESP Box" ) );
						save_esp_widget ( jvisual_editor, as_visual_editor->settings.health_bar, _ ( "Health Bar" ) );
						save_esp_widget ( jvisual_editor, as_visual_editor->settings.ammo_bar, _ ( "Ammo Bar" ) );
						save_esp_widget ( jvisual_editor, as_visual_editor->settings.desync_bar, _ ( "Desync Bar" ) );
						save_esp_widget ( jvisual_editor, as_visual_editor->settings.esp_name, _ ( "Name ESP" ) );
						save_esp_widget ( jvisual_editor, as_visual_editor->settings.esp_weapon, _ ( "Weapon ESP" ) );

						cJSON_AddItemToObject ( jgroup, _ ( "Visual Editor Data" ), jvisual_editor );

						break;
					}
					}
				} );

				cJSON_AddItemToObject ( jsubtab, group_str.c_str ( ), jgroup );
			} );

			cJSON_AddItemToObject ( jtab, subtab_str.c_str ( ), jsubtab );
		} );

		cJSON_AddItemToObject ( window, tab_str.c_str ( ), jtab );
	} );

	cJSON_AddItemToObject ( json, window_str.c_str ( ), window );

	const auto dump = cJSON_Print ( json );

	wchar_t appdata [ MAX_PATH ];

	if ( SUCCEEDED ( LI_FN ( SHGetFolderPathW )( nullptr, N ( CSIDL_PERSONAL ), nullptr, N ( int ( SHGFP_TYPE_CURRENT ) ), appdata ) ) ) {
		LI_FN ( CreateDirectoryW )( ( std::wstring ( appdata ) + _ ( L"\\sesame" ) ).data ( ), nullptr );
		LI_FN ( CreateDirectoryW )( ( std::wstring ( appdata ) + _ ( L"\\sesame\\configs" ) ).data ( ), nullptr );

		std::ofstream ofile ( std::wstring ( appdata ) + _ ( L"\\sesame\\configs\\" ) + file );

		if ( ofile.is_open ( ) ) {
			ofile.write ( dump, strlen ( dump ) );
			ofile.close ( );
		}
	}

	cJSON_Delete ( json );
}

void oxui::window::load_state( const str& file ) {
	auto load_esp_widget = [ ] ( cJSON* visual_editor, esp_widget_t& widget, const char* widget_name ) {
		const auto jwidget = cJSON_GetObjectItemCaseSensitive ( visual_editor, widget_name );

		if ( !jwidget || !cJSON_IsObject ( jwidget ) ) {
			dbg_print ( _ ( "Error while parsing visual editor: Failed to find widget \"%s\".\n" ), widget_name );
			return;
		}

		const auto jqueued_location = cJSON_GetObjectItemCaseSensitive ( jwidget, _ ( "Queued Location" ) );
		const auto jlocation = cJSON_GetObjectItemCaseSensitive ( jwidget, _ ( "Location" ) );
		const auto jtype = cJSON_GetObjectItemCaseSensitive ( jwidget, _ ( "Type" ) );
		const auto jenabled = cJSON_GetObjectItemCaseSensitive ( jwidget, _ ( "Enabled" ) );

		if ( !jqueued_location || !cJSON_IsNumber ( jqueued_location )
			|| !jlocation || !cJSON_IsNumber ( jlocation )
			|| !jtype || !cJSON_IsNumber ( jtype )
			|| !jenabled || !cJSON_IsBool ( jenabled ) ) {
			dbg_print ( _ ( "Error while parsing visual editor: Failed to find widget \"%s\"'s properties.\n" ), widget_name );
			return;
		}

		widget.enabled = jenabled->valueint;
		widget.queued_location = static_cast < esp_widget_pos_t >( jqueued_location->valueint );
		widget.location = static_cast < esp_widget_pos_t >( jlocation->valueint );
		widget.type = static_cast < esp_widget_type_t > ( jtype->valueint );
	};

	std::string dump;

	wchar_t appdata [ MAX_PATH ];

	if ( SUCCEEDED ( LI_FN ( SHGetFolderPathW )( nullptr, N ( CSIDL_PERSONAL ), nullptr, N ( int ( SHGFP_TYPE_CURRENT ) ), appdata ) ) ) {
		LI_FN ( CreateDirectoryW )( ( std::wstring ( appdata ) + _ ( L"\\sesame" ) ).data ( ), nullptr );
		LI_FN ( CreateDirectoryW )( ( std::wstring ( appdata ) + _ ( L"\\sesame\\configs" ) ).data ( ), nullptr );
	}

	std::ifstream ofile ( std::wstring ( appdata ) + _ ( L"\\sesame\\configs\\" ) + file );

	if ( !ofile.is_open ( ) )
		return;

	ofile.seekg ( 0, std::ios::end );
	const auto fsize = ofile.tellg ( );
	ofile.seekg ( 0, std::ios::beg );
	char* str = new char [ fsize ];
	ofile.read ( str, fsize );
	dump = str;
	delete [ ] str;
	ofile.close ( );

	if ( dump.empty ( ) )
		return;

	const auto json = cJSON_Parse ( dump.c_str ( ) );

	if ( !json ) {
		dbg_print ( cJSON_GetErrorPtr ( ) );
		cJSON_Delete ( json );
		return;
	}

	auto window_str = std::string ( title.begin ( ), title.end ( ) );

	const auto jwindow = cJSON_GetObjectItemCaseSensitive ( json, window_str.c_str ( ) );

	if ( !jwindow ) {
		dbg_print ( cJSON_GetErrorPtr ( ) );
		cJSON_Delete ( json );
		return;
	}

	std::for_each ( objects.begin ( ), objects.end ( ), [ & ] ( std::shared_ptr< obj > tab_obj ) {
		auto tab = std::static_pointer_cast< oxui::tab >( tab_obj );
		auto tab_str = std::string ( tab->title.begin ( ), tab->title.end ( ) );

		const auto jtab = cJSON_GetObjectItemCaseSensitive ( jwindow, tab_str.c_str ( ) );

		if ( !jtab ) {
			dbg_print ( cJSON_GetErrorPtr ( ) );
			return;
		}

		std::for_each ( tab->objects.begin ( ), tab->objects.end ( ), [ & ] ( std::shared_ptr< obj > subtab_obj ) {
			auto subtab = std::static_pointer_cast< oxui::tab >( subtab_obj );
			auto subtab_str = std::string ( subtab->title.begin ( ), subtab->title.end ( ) );

			const auto jsubtab = cJSON_GetObjectItemCaseSensitive ( jtab, subtab_str.c_str ( ) );

			if ( !jsubtab ) {
				dbg_print ( cJSON_GetErrorPtr ( ) );
				return;
			}

			std::for_each ( subtab->objects.begin ( ), subtab->objects.end ( ), [ & ] ( std::shared_ptr< obj > group_obj ) {
				auto group = std::static_pointer_cast< oxui::group >( group_obj );
				auto group_str = std::string ( group->title.begin ( ), group->title.end ( ) );

				const auto jgroup = cJSON_GetObjectItemCaseSensitive ( jsubtab, group_str.c_str ( ) );

				if ( !jgroup ) {
					dbg_print ( cJSON_GetErrorPtr ( ) );
					return;
				}

				std::for_each ( group->objects.begin ( ), group->objects.end ( ), [ & ] ( std::shared_ptr< obj >& object ) {
					switch ( object->type ) {
					case object_checkbox: {
						auto as_checkbox = ( checkbox* ) object.get ( );
						auto obj_str = std::string ( as_checkbox->label.begin ( ), as_checkbox->label.end ( ) );

						const auto jitem = cJSON_GetObjectItemCaseSensitive ( jgroup, obj_str.c_str ( ) );

						if ( !jitem || !cJSON_IsBool ( jitem ) ) {
							dbg_print ( _ ( "Error while parsing checkbox \"%s\": Failed to find object.\n" ), obj_str.c_str( ) );
							return;
						}

						as_checkbox->checked = jitem->valueint;
						break;
					}
					case object_slider: {
						auto as_slider = ( slider* ) object.get ( );
						auto obj_str = std::string ( as_slider->label.begin ( ), as_slider->label.end ( ) );

						const auto jitem = cJSON_GetObjectItemCaseSensitive ( jgroup, obj_str.c_str ( ) );

						if ( !jitem || !cJSON_IsNumber ( jitem ) ) {
							dbg_print ( _ ( "Error while parsing slider \"%s\": Failed to find object.\n" ), obj_str.c_str ( ) );
							return;
						}

						as_slider->value = jitem->valuedouble;
						break;
					}
					case object_dropdown: {
						auto as_dropdown = ( dropdown* ) object.get ( );
						auto obj_str = std::string ( as_dropdown->label.begin ( ), as_dropdown->label.end ( ) );

						const auto jitem = cJSON_GetObjectItemCaseSensitive ( jgroup, obj_str.c_str ( ) );

						if ( !jitem || !cJSON_IsNumber ( jitem ) ) {
							dbg_print ( _ ( "Error while parsing dropdown \"%s\": Failed to find object.\n" ), obj_str.c_str ( ) );
							return;
						}

						as_dropdown->value = jitem->valueint;
						break;
					}
										//case object_textbox: {
										//	auto as_textbox = ( textbox* ) object.get ( );
										//	auto obj_str = std::string ( as_textbox->label.begin ( ), as_textbox->label.end ( ) );
										//
										//	/* control doesn't exist */
										//	if ( !json [ window_str ][ tab_str ][ group_str ].contains ( obj_str ) )
										//		return;
										//
										//	const auto as_str = json [ window_str ][ tab_str ][ group_str ][ obj_str ].get< std::string > ( );
										//	as_textbox->buf = std::wstring ( as_str.begin ( ), as_str.end ( ) );
										//	break;
										//}
					case object_keybind: {
						auto as_keybind = ( keybind* ) object.get ( );
						auto obj_str = std::string ( as_keybind->label.begin ( ), as_keybind->label.end ( ) );

						const auto jitem = cJSON_GetObjectItemCaseSensitive ( jgroup, obj_str.c_str ( ) );

						if ( !jitem || !cJSON_IsArray ( jitem ) ) {
							dbg_print ( _ ( "Error while parsing keybind \"%s\": Failed to find object.\n" ), obj_str.c_str ( ) );
							return;
						}

						const cJSON* cur_keybind_data = nullptr;
						int keybind_data_index = 0;

						cJSON_ArrayForEach ( cur_keybind_data, jitem ) {
							if ( !cur_keybind_data || !cJSON_IsNumber ( cur_keybind_data ) ) {
								if ( !keybind_data_index )
									dbg_print ( _ ( "Error while parsing keybind data \"%s\": Failed to find key code.\n" ), obj_str.c_str ( ) );
								else if ( keybind_data_index == 1 )
									dbg_print ( _ ( "Error while parsing keybind data \"%s\": Failed to find key mode.\n" ), obj_str.c_str ( ) );

								keybind_data_index++;
								continue;
							}

							switch ( keybind_data_index ) {
							case 0: as_keybind->key = cur_keybind_data->valueint; break;
							case 1: as_keybind->mode = static_cast < oxui::keybind_mode > ( cur_keybind_data->valueint ); break;
							}

							keybind_data_index++;
						}

						break;
					}
					case object_colorpicker: {
						auto as_colorpicker = ( color_picker* ) object.get ( );
						auto obj_str = std::string ( as_colorpicker->label.begin ( ), as_colorpicker->label.end ( ) );

						const auto jitem = cJSON_GetObjectItemCaseSensitive ( jgroup, obj_str.c_str ( ) );
						
						if ( !jitem || !cJSON_IsArray ( jitem ) ) {
							dbg_print ( _ ( "Error while parsing color picker \"%s\": Failed to find object.\n" ), obj_str.c_str ( ) );
							return;
						}

						const cJSON* clr_channel = nullptr;
						int clr_channel_num = 0;

						cJSON_ArrayForEach ( clr_channel, jitem ) {
							if ( !clr_channel || !cJSON_IsNumber ( clr_channel ) ) {
								if ( !clr_channel_num )
									dbg_print ( _ ( "Error while parsing color picker data \"%s\": Failed to find red channel.\n" ), obj_str.c_str ( ) );
								else if ( clr_channel_num == 1 )
									dbg_print ( _ ( "Error while parsing color picker data \"%s\": Failed to find green channel.\n" ), obj_str.c_str ( ) );
								else if ( clr_channel_num == 2 )
									dbg_print ( _ ( "Error while parsing color picker data \"%s\": Failed to find blue channel.\n" ), obj_str.c_str ( ) );
								else if ( clr_channel_num == 3 )
									dbg_print ( _ ( "Error while parsing color picker data \"%s\": Failed to find alpha channel.\n" ), obj_str.c_str ( ) );

								clr_channel_num++;
								continue;
							}

							switch ( clr_channel_num ) {
							case 0: as_colorpicker->clr.r = clr_channel->valueint; break;
							case 1: as_colorpicker->clr.g = clr_channel->valueint; break;
							case 2: as_colorpicker->clr.b = clr_channel->valueint; break;
							case 3: as_colorpicker->clr.a = clr_channel->valueint; break;
							}

							clr_channel_num++;
						}

						break;
					}
					case object_visual_editor: {
						auto as_visual_editor = ( visual_editor* ) object.get ( );

						const auto jvisual_editor = cJSON_GetObjectItemCaseSensitive ( jgroup, _ ( "Visual Editor Data" ) );

						if ( !jvisual_editor || !cJSON_IsObject ( jvisual_editor ) ) {
							dbg_print ( _ ( "Error while parsing visual editor: Failed to find object.\n" ) );
							return;
						}

						const auto jmodel_type = cJSON_GetObjectItemCaseSensitive ( jvisual_editor, _ ( "Model Type" ) );

						if ( !jmodel_type || !cJSON_IsNumber ( jmodel_type ) ) {
							dbg_print ( _ ( "Error while parsing visual editor: Failed to find model type.\n" ) );
							return;
						}

						as_visual_editor->settings.model_type = static_cast< model_type_t > ( jmodel_type->valueint );

						/*
						
						cJSON_AddItemToObject ( jvisual_editor, _ ( "Show Value" ), cJSON_CreateBool ( as_visual_editor->settings.show_value ) );
						*/

						const auto jxqz_chams = cJSON_GetObjectItemCaseSensitive ( jvisual_editor, _ ( "XQZ Chams" ) );
						const auto jglow = cJSON_GetObjectItemCaseSensitive ( jvisual_editor, _ ( "Glow" ) );
						const auto jrimlight_overlay = cJSON_GetObjectItemCaseSensitive ( jvisual_editor, _ ( "Rimlight Overlay" ) );
						const auto jbacktrack_chams = cJSON_GetObjectItemCaseSensitive ( jvisual_editor, _ ( "Backtrack Chams" ) );
						const auto jhit_matrix = cJSON_GetObjectItemCaseSensitive ( jvisual_editor, _ ( "Hit Matrix" ) );
						const auto jshow_value = cJSON_GetObjectItemCaseSensitive ( jvisual_editor, _ ( "Show Value" ) );

						if ( jxqz_chams && cJSON_IsBool ( jxqz_chams ) )
							as_visual_editor->settings.xqz = jxqz_chams->valueint;

						if ( jglow && cJSON_IsBool ( jglow ) )
							as_visual_editor->settings.glow = jglow->valueint;

						if ( jrimlight_overlay && cJSON_IsBool ( jrimlight_overlay ) )
							as_visual_editor->settings.rimlight = jrimlight_overlay->valueint;

						if ( jbacktrack_chams && cJSON_IsBool ( jbacktrack_chams ) )
							as_visual_editor->settings.backtrack = jbacktrack_chams->valueint;

						if ( jhit_matrix && cJSON_IsBool ( jhit_matrix ) )
							as_visual_editor->settings.hit_matrix = jhit_matrix->valueint;

						if ( jshow_value && cJSON_IsBool ( jshow_value ) )
							as_visual_editor->settings.show_value = jshow_value->valueint;

						load_esp_widget ( jvisual_editor, as_visual_editor->settings.esp_box, _ ( "ESP Box" ) );
						load_esp_widget ( jvisual_editor, as_visual_editor->settings.health_bar, _ ( "Health Bar" ) );
						load_esp_widget ( jvisual_editor, as_visual_editor->settings.ammo_bar, _ ( "Ammo Bar" ) );
						load_esp_widget ( jvisual_editor, as_visual_editor->settings.desync_bar, _ ( "Desync Bar" ) );
						load_esp_widget ( jvisual_editor, as_visual_editor->settings.esp_name, _ ( "Name ESP" ) );
						load_esp_widget ( jvisual_editor, as_visual_editor->settings.esp_weapon, _ ( "Weapon ESP" ) );

						break;
					}
					}
				} );
			} );
		} );
	} );

	cJSON_Delete ( json );
}

void oxui::window::think( ) {
	if ( utils::key_state ( VK_LBUTTON ) )
		binds::mouse_pos ( not_clicked_offset );

	if ( !pressing_move_key && utils::key_state( VK_LBUTTON ) && shapes::hovering( rect( area.x - area.w / 6 - theme.spacing / 2, area.y, area.w / 6, area.w / 6 ) ) ) {
		pressing_move_key = true;
		pos mouse_mpos;
		binds::mouse_pos( mouse_mpos );
		click_offset = pos( mouse_mpos.x - area.x, mouse_mpos.y - area.y );
	}
	else if ( pressing_move_key && utils::key_state ( VK_LBUTTON ) ) {
		pos mouse_mpos;
		binds::mouse_pos( mouse_mpos );

		area.x = mouse_mpos.x - click_offset.x;
		area.y = mouse_mpos.y - click_offset.y;
	}
	else {
		pressing_move_key = false;
	}

	/* clamp window pos on scren */
	render::dim screen_size;
	render::screen_size ( screen_size.w, screen_size.h );

	if ( area.x < area.w / 6 - theme.spacing / 2 + 16 )
		area.x = area.w / 6 - theme.spacing / 2 + 16;

	if ( area.y < 0 )
		area.y = 0;

	if ( ( area.x + area.w ) > screen_size.w )
		area.x = screen_size.w - area.w;

	if ( ( area.y + area.h ) > screen_size.h )
		area.y = screen_size.h - area.h;

	//shapes::hovering ( area, true );
}

void oxui::window::draw( ) {
	shapes::finished_input_frame = false;

	render_overlay = false;

	if ( toggle_bind ) {
		if ( !pressing_open_key && utils::key_state ( toggle_bind ) ) {
			pressing_open_key = true;
		}
		else if ( pressing_open_key && !utils::key_state ( toggle_bind ) ) {
			open = !open;
			pressing_open_key = false;

			if ( open )
				g_input = true;
		}
	}

	if ( !open ) {
		render_overlay = false;
		handle_keyboard = false;
		g_input = false;
		return;
	}

	auto& parent_panel = find_parent< panel >( object_panel );

	/* menu physics */
	think( );

	/* animate dropdown popup */
	/* fix how the button looks to match loader */
	/* resizeable menu */
	/* add hitmarker option, local fake chams, bullet tracers, bullet impacts, hit matrix */
	/* change default visuals color */

	cursor_pos = pos( area.x + theme.spacing, area.y + theme.spacing );

	/* draw window rect */
	//binds::rounded_rect ( { area.x, area.y, area.w + 3, area.h + 3 }, 16, 16, color( 0, 0, 0, 65 ), false );
	binds::rounded_rect( area, 16, 16, theme.bg, false );

	///* draw tab bar */
	//binds::rounded_rect ( { area.x - area.w / 6 - theme.spacing / 2, area.y, area.w / 6, area.h }, 16, 16, theme.title_bar, false );

	/* TODO: un-ghetto this pls */
	/* round gradient tab bar (not radial) (so fucking ghetto) (pls fix)*/
	//binds::rounded_rect ( { area.x - area.w / 6 - theme.spacing / 2, area.y, area.w / 6 + 3, area.h + 3 }, 16, 16, color ( 0, 0, 0, 65 ), false );
	binds::rounded_rect ( { area.x - area.w / 6 - theme.spacing / 2, area.y, area.w / 6, area.h / 2 }, 16, 16, theme.title_bar, false );
	binds::rounded_rect ( { area.x - area.w / 6 - theme.spacing / 2, area.y + area.h / 2, area.w / 6, area.h / 2 }, 16, 16, theme.title_bar_low, false );
	binds::gradient_rect ( { area.x - area.w / 6 - theme.spacing / 2 - 1, area.y + 16, area.w / 6 + 1, area.h - 16 * 2 }, theme.title_bar, theme.title_bar_low, false );

	const auto center_w = area.x - area.w / 6 - theme.spacing / 2 + ( area.w / 6 ) / 2;
	const auto target_logo_dim = pos { ( area.w / 6 ) / 3, area.h / 20 };
	const auto logo_scale = static_cast< float > ( target_logo_dim.y ) / 107.0f;
	const auto logo_dim = pos { static_cast < int > ( 150.0f * logo_scale ), static_cast < int > ( 107.0f * logo_scale ) };

	render::texture ( parent_panel.sprite, parent_panel.tex, center_w - logo_dim.x * 0.9f, area.y + area.h / 14 - logo_dim.y / 2, target_logo_dim.x, target_logo_dim.y, logo_scale, logo_scale * 1.333f, D3DCOLOR_RGBA( theme.logo.r, theme.logo.g, theme.logo.b, theme.logo.a ) );

	const auto separator_y = area.y + area.h / 14 + logo_dim.y / 2 + area.h / 20;

	binds::gradient_rect ( { center_w - ( area.w / 6 ) / 3, separator_y, ( area.w / 6 ) / 3, 1 }, { 0, 0, 0, 0 }, { 0, 0, 0, 66 }, true );
	binds::gradient_rect ( { center_w, separator_y, ( area.w / 6 ) / 3, 1 }, { 0, 0, 0, 66 }, { 0, 0, 0, 0 }, true );

	const auto tabs_start_y = separator_y + area.h / 12;

	std::vector< std::shared_ptr< tab > > tab_list;

	std::for_each ( objects.begin ( ), objects.end ( ), [ & ] ( std::shared_ptr< obj > object ) {
		if ( object->type == object_tab )
			tab_list.push_back ( std::static_pointer_cast< tab >( object ) );
	} );
	
	auto cur_tab_y = 0;

	std::for_each ( tab_list.begin ( ), tab_list.end ( ), [ & ] ( std::shared_ptr< tab > object ) {
		if ( object->title == OSTR ( "Customization" ) ) {
			rect bounds;
			binds::text_bounds ( parent_panel.fonts [ OSTR ( "tab" ) ], OSTR ( "D" ), bounds );

			if ( shapes::clicking ( rect ( center_w - bounds.w / 2 + 1 - 2, area.y + area.h - 56 - bounds.h / 2 - 2, bounds.w + 4, bounds.h + 4 ) ) ) {
				/* update click timer */
				object->time = parent_panel.time;

				/* deselect all */
				std::for_each (
					tab_list.begin ( ),
					tab_list.end ( ),
					[ & ] ( std::shared_ptr< tab > child ) {
					/* re-run animation */
					if ( child->selected )
						child->time = parent_panel.time;

					child->selected = false;
				}
				);

				/* select clicked tab */
				object->selected = true;
			}

			return;
		}

		if ( shapes::clicking ( rect ( center_w - 15, tabs_start_y + cur_tab_y - 15, 30, 30 ) ) ) {
			/* update click timer */
			object->time = parent_panel.time;

			/* deselect all */
			std::for_each (
				tab_list.begin ( ),
				tab_list.end ( ),
				[ & ] ( std::shared_ptr< tab > child ) {
				/* re-run animation */
				if ( child->selected )
					child->time = parent_panel.time;

				child->selected = false;
			}
			);

			/* select clicked tab */
			object->selected = true;
		}

		auto time_since_click = std::clamp ( parent_panel.time - object->time, 0.0, theme.animation_speed );
		auto selection_rad = 22.0 - 10.0 * ( time_since_click * ( 1.0 / theme.animation_speed ) );
		auto selection_alpha = 90.0 - int ( time_since_click * ( 1.0 / theme.animation_speed ) * 90.0 );

		if ( object->selected ) {
			selection_alpha = int ( time_since_click * ( 1.0 / theme.animation_speed ) * 90.0 );
			selection_rad = 12.0 + 10.0 * ( time_since_click * ( 1.0 / theme.animation_speed ) );
		}

		if ( selection_rad > 0 && selection_alpha > 0 ) {
			binds::circle ( { center_w, tabs_start_y + cur_tab_y }, selection_rad, 20, { 0, 0, 0, static_cast < int > ( selection_alpha ) }, false );
			binds::circle ( { center_w, tabs_start_y + cur_tab_y }, selection_rad, 20, { 0, 0, 0, static_cast < int > ( selection_alpha ) }, true );
		}

		rect bounds;
		binds::text_bounds ( parent_panel.fonts [ OSTR ( "tab" ) ], object->title, bounds );
		binds::text ( { center_w - bounds.w / 2, tabs_start_y + cur_tab_y - bounds.h / 2 + 1 }, parent_panel.fonts [ OSTR ( "tab" ) ], object->title, object->selected ? color ( theme.title_text.r, theme.title_text.g, theme.title_text.b, 150 ) : color ( 0, 0, 0, 80 ), object->selected );

		cur_tab_y += 46;
	} );

	/* lower half */ {
		/* configuration button */
		rect bounds;
		binds::text_bounds ( parent_panel.fonts [ OSTR ( "tab" ) ], OSTR ( "D" ), bounds );

		auto time_since_click = std::clamp ( parent_panel.time - tab_list.back ( )->time, 0.0, theme.animation_speed );
		auto selection_rad = 16.0 - 6.0 * ( time_since_click * ( 1.0 / theme.animation_speed ) );
		auto selection_alpha = 90.0 - int ( time_since_click * ( 1.0 / theme.animation_speed ) * 90.0 );

		if ( tab_list.back ( )->selected ) {
			selection_alpha = int ( time_since_click * ( 1.0 / theme.animation_speed ) * 90.0 );
			selection_rad = 10.0 + 6.0 * ( time_since_click * ( 1.0 / theme.animation_speed ) );
		}

		if ( tab_list.back( )->selected ) {
			binds::circle ( { center_w + 1, area.y + area.h - 56 - 1 }, bounds.w / 2 + 2, selection_rad, { 0, 0, 0, static_cast < int > ( selection_alpha ) }, false );
			binds::circle ( { center_w + 1, area.y + area.h - 56 - 1 }, bounds.w / 2 + 2, selection_rad, { 0, 0, 0, static_cast < int > ( selection_alpha ) }, true );
		}

		binds::text ( { center_w - bounds.w / 2 + 1, area.y + area.h - 56 - bounds.h / 2 }, parent_panel.fonts [ OSTR ( "tab" ) ], OSTR ( "D" ), tab_list.back ( )->selected ? color ( theme.title_text.r, theme.title_text.g, theme.title_text.b, 150 ) : color ( 0, 0, 0, 80 ), tab_list.back ( )->selected );

		/* user profile picture */
		binds::text_bounds ( parent_panel.fonts [ OSTR ( "tab" ) ], OSTR ( "N" ), bounds );
		binds::text ( { center_w - bounds.w / 2 + 1, area.y + area.h - 32 - bounds.h / 2 }, parent_panel.fonts [ OSTR ( "tab" ) ], OSTR ( "N" ), color ( 0, 0, 0, 150 ), false );

		///* watermark text */
		//binds::text_bounds ( parent_panel.fonts [ OSTR ( "watermark" ) ], OSTR ( "sesame.one - 2020" ), bounds );
		//binds::text ( { center_w - bounds.w / 2 + 1, area.y + area.h - 9 - bounds.h / 2 }, parent_panel.fonts [ OSTR ( "watermark" ) ], OSTR ( "sesame.one - 2020" ), color ( 0xcc, 0x76, 0xef, 255 ), false );
	}

	///* title bar */
	//binds::fill_rect( rect( area.x, area.y - 26, area.w, 26 ), theme.title_bar );
	//shapes::box( rect( area.x, area.y - 26, area.w, 26 ), 0.0, false, false, true, false, false, false );
	//// render::texture( rsc::images::weixin_logo, area.x + 4, area.y - 28 + 6, 118, 100, 0.133f );
	//binds::text( pos( area.x + 4 /* + 24 */, area.y - 28 + 6 ), parent_panel.fonts [ OSTR( "title" ) ], title, theme.title_text, false );
	//binds::rect ( rect ( area.x - 1, area.y - 1 - 26, area.w + 1, area.h + 1 + 26 ), color ( 0, 0, 0, 255 ) );
	//
	//std::vector< std::pair< std::shared_ptr< tab >, int > > tab_list;
	//auto total_tabs_w = 0;
	//std::for_each( objects.begin( ), objects.end( ), [ & ] ( std::shared_ptr< obj > object ) {
	//	if ( object->type == object_tab ) {
	//		auto as_tab = std::static_pointer_cast< tab >( object );
	//
	//		rect bounds;
	//		binds::text_bounds( parent_panel.fonts [ OSTR( "object" ) ], as_tab->title, bounds );
	//		total_tabs_w += bounds.w + 4;
	//
	//		tab_list.push_back( std::pair< std::shared_ptr< tab >, int >( as_tab, bounds.w ) );
	//	}
	//	} );
	//auto last_tab_pos = pos( area.x + area.w - total_tabs_w - 6, area.y - 28 + 6 );
	//std::for_each( tab_list.begin( ), tab_list.end( ), [ & ] ( std::pair< std::shared_ptr< tab >, int > object ) {
	//	if ( shapes::clicking( rect( last_tab_pos.x, last_tab_pos.y, object.second, 16 ) ) ) {
	//		/* update click timer */
	//		object.first->time = parent_panel.time;
	//
	//		/* deselect all */
	//		std::for_each(
	//			tab_list.begin( ),
	//			tab_list.end( ),
	//			[ & ] ( std::pair< std::shared_ptr< tab >, int > child ) {
	//				/* re-run animation */
	//				if ( child.first->selected )
	//					child.first->time = parent_panel.time;
	//
	//				child.first->selected = false;
	//			}
	//		);
	//
	//		/* select clicked tab */
	//		object.first->selected = true;
	//	}
	//
	//	auto time_since_click = std::clamp( parent_panel.time - object.first->time, 0.0, theme.animation_speed );
	//	auto bar_width = object.second - object.second * ( time_since_click * ( 1.0 / theme.animation_speed ) );
	//	auto text_height = 0;
	//	auto alpha = 255 - int( time_since_click * ( 1.0 / theme.animation_speed ) * 80.0 );
	//
	//	if ( object.first->selected ) {
	//		alpha = 175 + int( time_since_click * ( 1.0 / theme.animation_speed ) * 80.0 );
	//		bar_width = object.second * ( time_since_click * ( 1.0 / theme.animation_speed ) );
	//		text_height = 2 * ( time_since_click * ( 1.0 / theme.animation_speed ) );
	//	}
	//
	//	binds::text( pos( last_tab_pos.x, area.y - 28 + 6 - text_height ), parent_panel.fonts [ OSTR( "object" ) ], object.first->title, color( theme.title_text.r, theme.title_text.g, theme.title_text.b, alpha ), false );
	//	binds::line( pos( last_tab_pos.x + object.second / 2 - bar_width / 2, area.y - 28 + 6 + 16 ), pos( last_tab_pos.x + object.second / 2 + bar_width / 2, area.y - 28 + 6 + 16 ), theme.main );
	//
	//	last_tab_pos.x += object.second + 4;
	//	} );
	//
	///* draw window objects */
	//binds::clip( area, [ & ] ( ) {
	//	std::for_each(
	//		objects.begin( ),
	//		objects.end( ),
	//		[ ] ( std::shared_ptr< obj >& child ) {
	//			child->draw( );
	//		}
	//	);
	//	} );
	//
	//if ( render_overlay )
	//	overlay_func( );

	/* draw window objects */
	binds::clip ( area, [ & ] ( ) {
		std::for_each (
			objects.begin ( ),
			objects.end ( ),
			[ ] ( std::shared_ptr< obj >& child ) {
			child->draw ( );
		}
		);
	} );

	if ( render_overlay && overlay_func.size( ) )
		std::for_each ( overlay_func.begin ( ), overlay_func.end ( ), [ ] ( std::function < void ( ) >& func ) { func ( ); } );

	overlay_func.clear ( );

	shapes::click_switch = false;

	scroll_delta = 0.0;
}