#include "visual_editor.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"
#include "../../features/esp.hpp"

constexpr const auto player_image_res_w = 200;
constexpr const auto player_image_res_h = 420;

void oxui::visual_editor::think( ) {
	auto& parent_window = find_parent< window >( object_window );
	auto& cursor_pos = parent_window.cursor_pos;

	if ( shapes::hovering ( rect ( cursor_pos.x + area.w / 2 - player_image_res_w / 2, cursor_pos.y, player_image_res_w * 0.7f, player_image_res_h * 0.7f ), false, true ) && !utils::key_state ( VK_RBUTTON ) && last_rkey ) {
		opened_shortcut_menu = true;
		binds::mouse_pos ( rclick_pos );
	}

	last_rkey = utils::key_state ( VK_RBUTTON );

	if ( opened_shortcut_menu ) {
		std::vector< str > rclick_menu_items {
			OSTR ( "Default Model" ),
			OSTR ( "Textured Model" ),
			OSTR ( "Flat Model" ),
			settings.xqz ? OSTR ( "Disable XQZ" ) : OSTR ( "Enable XQZ" ),
			settings.backtrack ? OSTR ( "Disable Backtrack Chams" ) : OSTR ( "Enable Backtrack Chams" ),
			settings.hit_matrix ? OSTR ( "Disable Hit Matrix" ) : OSTR ( "Enable Hit Matrix" ),
			settings.glow ? OSTR ( "Remove Glow" ) : OSTR ( "Add Glow" ),
			settings.rimlight ? OSTR ( "Remove Rimlight" ) : OSTR ( "Add Rimlight" ),
			settings.esp_box.enabled ? OSTR ( "Remove ESP Box" ) : OSTR ( "Add ESP Box" ),
			settings.health_bar.enabled ? OSTR ( "Remove Health Bar" ) : OSTR ( "Add Health Bar" ),
			settings.ammo_bar.enabled ? OSTR ( "Remove Ammo Bar" ) : OSTR ( "Add Ammo Bar" ),
			settings.desync_bar.enabled ? OSTR ( "Remove Desync Bar" ) : OSTR ( "Add Desync Bar" ),
			settings.show_value ? OSTR ( "Remove Value Text" ) : OSTR ( "Add Value Text" ),
			settings.esp_name.enabled ? OSTR ( "Remove Nametag" ) : OSTR ( "Add Nametag" ),
			settings.esp_weapon.enabled ? OSTR ( "Remove Weapon Name" ) : OSTR ( "Add Weapon Name" )
		};

		/* background of the list */
		rect list_pos ( rclick_pos.x, rclick_pos.y, 150, theme.spacing );

		pos mouse_pos;
		binds::mouse_pos ( mouse_pos );

		hovered_index = 0;

		auto index = 0;
		auto selected = -1;

		for ( const auto& it : rclick_menu_items ) {
			const auto backup_input_clip_area = g_oxui_input_clip_area;

			/* ignore clipping */
			g_oxui_input_clip_area = false;

			/* check if we are clicking the thingy */
			if ( utils::key_state ( VK_LBUTTON ) && mouse_pos.x >= list_pos.x && mouse_pos.y >= list_pos.y && mouse_pos.x <= list_pos.x + list_pos.w && mouse_pos.y <= list_pos.y + list_pos.h ) {
				selected = index;

				/* remove if you want to close it manually instead of automatically (snek) */ {
					opened_shortcut_menu = false;
				}

				shapes::finished_input_frame = true;
				shapes::click_start = pos ( 0, 0 );
				g_input = true;
			}
			else if ( mouse_pos.x >= list_pos.x && mouse_pos.y >= list_pos.y && mouse_pos.x <= list_pos.x + list_pos.w && mouse_pos.y <= list_pos.y + list_pos.h ) {
				hovered_index = index;
			}

			/* ignore clipping */
			g_oxui_input_clip_area = backup_input_clip_area;

			list_pos.y += theme.spacing;
			index++;
		}

		/* set the shit here */
		if ( selected != -1 ) {
			g_input = false;

			switch ( selected ) {
			case 0:
				settings.model_type = model_type_t::model_default;
				break;
			case 1:
				settings.model_type = model_type_t::model_textured;
				break;
			case 2:
				settings.model_type = model_type_t::model_flat;
				break;
			case 3:
				settings.xqz = !settings.xqz;
				break;
			case 4:
				settings.backtrack = !settings.backtrack;
				break;
			case 5:
				settings.hit_matrix = !settings.hit_matrix;
				break;
			case 6:
				settings.glow = !settings.glow;
				break;
			case 7:
				settings.rimlight = !settings.rimlight;
				break;
			case 8:
				settings.esp_box.enabled = !settings.esp_box.enabled;
				break;
			case 9:
				settings.health_bar.enabled = !settings.health_bar.enabled;
				break;
			case 10:
				settings.ammo_bar.enabled = !settings.ammo_bar.enabled;
				break;
			case 11:
				settings.desync_bar.enabled = !settings.desync_bar.enabled;
				break;
			case 12:
				settings.show_value = !settings.show_value;
				break;
			case 13:
				settings.esp_name.enabled = !settings.esp_name.enabled;
				break;
			case 14:
				settings.esp_weapon.enabled = !settings.esp_weapon.enabled;
				break;
			}
		}

		/* we clicked outside the right click menu, close */
		if ( utils::key_state ( VK_LBUTTON ) ) {
			pos mouse_pos;
			binds::mouse_pos ( mouse_pos );

			const auto hovered_area = mouse_pos.x >= list_pos.x && mouse_pos.y >= rclick_pos.y && mouse_pos.x <= list_pos.x + list_pos.w && mouse_pos.y <= list_pos.y;

			if ( !hovered_area ) {
				shapes::finished_input_frame = true;
				shapes::click_start = pos ( 0, 0 );
				g_input = true;
				opened_shortcut_menu = false;
			}
		}
	}
}

void oxui::visual_editor::draw( ) {
	think( );

	auto& parent_panel = find_parent< panel >( object_panel );
	auto& parent_window = find_parent< window >( object_window );
	auto& font = parent_panel.fonts [ OSTR( "object" ) ];
	auto& cursor_pos = parent_window.cursor_pos;

	const auto center_x = cursor_pos.x + area.w / 2;

	switch ( settings.model_type ) {
	case model_type_t::model_default:
		render::texture ( parent_panel.player_img.normal.sprite, parent_panel.player_img.normal.tex, center_x - player_image_res_w / 2, cursor_pos.y, player_image_res_w * 0.7f, player_image_res_h * 0.7f, 0.7f, 0.7f );
		break;
	case model_type_t::model_textured:
		render::texture ( parent_panel.player_img.lighting.sprite, parent_panel.player_img.lighting.tex, center_x - player_image_res_w / 2, cursor_pos.y, player_image_res_w * 0.7f, player_image_res_h * 0.7f, 0.7f, 0.7f, D3DCOLOR_RGBA( settings.cham_picker->clr.r, settings.cham_picker->clr.g, settings.cham_picker->clr.b, settings.cham_picker->clr.a ) );
		break;
	case model_type_t::model_flat:
		render::texture ( parent_panel.player_img.flat.sprite, parent_panel.player_img.flat.tex, center_x - player_image_res_w / 2, cursor_pos.y, player_image_res_w * 0.7f, player_image_res_h * 0.7f, 0.7f, 0.7f, D3DCOLOR_RGBA ( settings.cham_picker->clr.r, settings.cham_picker->clr.g, settings.cham_picker->clr.b, settings.cham_picker->clr.a ) );
		break;
	}

	if ( settings.rimlight )
		render::texture ( parent_panel.player_img.rimlight.sprite, parent_panel.player_img.rimlight.tex, center_x - player_image_res_w / 2, cursor_pos.y, player_image_res_w * 0.7f, player_image_res_h * 0.7f, 0.7f, 0.7f, D3DCOLOR_RGBA ( settings.rimlight_picker->clr.r, settings.rimlight_picker->clr.g, settings.rimlight_picker->clr.b, settings.rimlight_picker->clr.a ) );

	if ( settings.glow )
		render::texture ( parent_panel.player_img.glow.sprite, parent_panel.player_img.glow.tex, center_x - player_image_res_w / 2, cursor_pos.y, player_image_res_w * 0.7f, player_image_res_h * 0.7f, 0.7f, 0.7f, D3DCOLOR_RGBA ( settings.glow_picker->clr.r, settings.glow_picker->clr.g, settings.glow_picker->clr.b, settings.glow_picker->clr.a ) );

	if ( settings.esp_box.enabled ) {
		render::outline ( cursor_pos.x + 30, cursor_pos.y + 20, player_image_res_w * 0.7f, player_image_res_h * 0.7f, D3DCOLOR_RGBA ( 0, 0, 0, std::clamp< int > ( static_cast< float >( settings.esp_box.picker->clr.a ) * 60.0f, 0, 60 ) ) );
		render::outline ( cursor_pos.x + 30, cursor_pos.y + 20, player_image_res_w * 0.7f, player_image_res_h * 0.7f, D3DCOLOR_RGBA( settings.esp_box.picker->clr.r, settings.esp_box.picker->clr.g, settings.esp_box.picker->clr.b, settings.esp_box.picker->clr.a ) );
	}
	
	auto cur_offset_left_height = 0;
	auto cur_offset_right_height = 0;
	auto cur_offset_left = 4;
	auto cur_offset_right = 4;
	auto cur_offset_bottom = 4;
	auto cur_offset_top = 4;

	auto render_bar_preview = [ & ] ( esp_widget_t& bar ) {
		if ( !bar.enabled )
			return;

		auto dragging = false;
		pos mouse_pos = pos ( 0, 0 );

		if ( utils::key_state ( VK_LBUTTON ) && ( ( shapes::hovering ( { cursor_pos.x + 30, cursor_pos.y + 20 + static_cast< int >( player_image_res_h * 0.7f ) + cur_offset_bottom, static_cast< int >( player_image_res_w * 0.7f ), 4 }, true ) && bar.location == esp_widget_pos_t::pos_bottom )
			|| ( shapes::hovering ( { cursor_pos.x + 30, cursor_pos.y + 20 - cur_offset_top - 4, static_cast< int >( player_image_res_w * 0.7f ), 4 }, true ) && bar.location == esp_widget_pos_t::pos_top )
			|| ( shapes::hovering ( { cursor_pos.x + 30 - cur_offset_left - 4, cursor_pos.y + 20, 4, static_cast< int > ( player_image_res_h * 0.7f ) }, true ) && bar.location == esp_widget_pos_t::pos_left )
			|| ( shapes::hovering ( { cursor_pos.x + 30 + static_cast< int > ( player_image_res_w * 0.7f ) + cur_offset_right , cursor_pos.y + 20, 4, static_cast< int > ( player_image_res_h * 0.7f ) }, true ) && bar.location == esp_widget_pos_t::pos_right ) ) ) {
			if ( shapes::hovering ( { cursor_pos.x + 30, cursor_pos.y + 20 + static_cast< int >( player_image_res_h * 0.7f ), static_cast< int >( player_image_res_w * 0.7f ), 50 } ) )
				bar.queued_location = esp_widget_pos_t::pos_bottom;
			else if ( shapes::hovering ( { cursor_pos.x + 30, cursor_pos.y + 20 - 50, static_cast< int >( player_image_res_w * 0.7f ), 50 } ) )
				bar.queued_location = esp_widget_pos_t::pos_top;
			else if ( shapes::hovering ( { cursor_pos.x + 30 - 50, cursor_pos.y + 20, 50, static_cast< int >( player_image_res_h * 0.7f ) } ) )
				bar.queued_location = esp_widget_pos_t::pos_left;
			else if ( shapes::hovering ( { cursor_pos.x + 30 + static_cast< int >( player_image_res_w * 0.7f ), cursor_pos.y + 20, 50, static_cast< int >( player_image_res_h * 0.7f ) } ) )
				bar.queued_location = esp_widget_pos_t::pos_right;

			binds::mouse_pos ( mouse_pos );
			dragging = true;
		}

		if ( !utils::key_state ( VK_LBUTTON ) && bar.queued_location != bar.location )
			bar.location = bar.queued_location;

		const auto calc_height = static_cast< float >( player_image_res_h * 0.7f )* ( static_cast< float >( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ) / 100.0f );

		switch ( bar.location ) {
		case esp_widget_pos_t::pos_left:
			render::outline ( cursor_pos.x + 30 - cur_offset_left - 4, cursor_pos.y + 20, 4, player_image_res_h * 0.7f, D3DCOLOR_RGBA ( 0, 0, 0, std::clamp< int > ( static_cast< float >( bar.picker->clr.a ) / 2, 0, 60 ) ) );
			render::rectangle ( cursor_pos.x + 30 - cur_offset_left - 4 + 1, cursor_pos.y + 20 + calc_height + 1, 4 - 1, static_cast< int >( player_image_res_h * 0.7f ) - calc_height, D3DCOLOR_RGBA ( bar.picker->clr.r, bar.picker->clr.g, bar.picker->clr.b, bar.picker->clr.a ) );
			
			if ( settings.show_value )
				render::text ( cursor_pos.x + 30 - cur_offset_left - 4 + 1, cursor_pos.y + 20 + calc_height + 1, D3DCOLOR_RGBA(255, 255, 255, 255), features::esp::dbg_font, std::to_wstring( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ), false, true );

			cur_offset_left += 6;
			break;
		case esp_widget_pos_t::pos_right:
			render::outline ( cursor_pos.x + 30 + player_image_res_w * 0.7f + cur_offset_right , cursor_pos.y + 20, 4, player_image_res_h * 0.7f, D3DCOLOR_RGBA ( 0, 0, 0, std::clamp< int > ( static_cast< float >( bar.picker->clr.a ) / 2, 0, 60 ) ) );
			render::rectangle ( cursor_pos.x + 30 + player_image_res_w * 0.7f + cur_offset_right + 1, cursor_pos.y + 20 + calc_height + 1, 4 - 1, static_cast< int >( player_image_res_h * 0.7f ) - calc_height, D3DCOLOR_RGBA ( bar.picker->clr.r, bar.picker->clr.g, bar.picker->clr.b, bar.picker->clr.a ) );
			
			if ( settings.show_value )
				render::text ( cursor_pos.x + 30 + player_image_res_w * 0.7f + cur_offset_right + 1, cursor_pos.y + 20 + calc_height + 1, D3DCOLOR_RGBA ( 255, 255, 255, 255 ), features::esp::dbg_font, std::to_wstring ( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ), false, true );

			cur_offset_right += 6;
			break;
		case esp_widget_pos_t::pos_bottom:
			render::outline ( cursor_pos.x + 30, cursor_pos.y + 20 + player_image_res_h * 0.7f + cur_offset_bottom, player_image_res_w * 0.7f, 4, D3DCOLOR_RGBA ( 0, 0, 0, std::clamp< int > ( static_cast< float >( bar.picker->clr.a ) / 2, 0, 60 ) ) );
			render::rectangle ( cursor_pos.x + 1 + 30, cursor_pos.y + 20 + player_image_res_h * 0.7f + cur_offset_bottom + 1, static_cast< float >( player_image_res_w * 0.7f )* ( static_cast< float >( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ) / 100.0f ) + 1, 4 - 1, D3DCOLOR_RGBA ( bar.picker->clr.r, bar.picker->clr.g, bar.picker->clr.b, bar.picker->clr.a ) );
			cur_offset_bottom += 6;

			if ( settings.show_value )
				render::text ( cursor_pos.x + 1 + 30 + static_cast< float >( player_image_res_w * 0.7f )* ( static_cast< float >( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ) / 100.0f ) + 1, cursor_pos.y + 20 + player_image_res_h * 0.7f + cur_offset_bottom + 1, D3DCOLOR_RGBA ( 255, 255, 255, 255 ), features::esp::dbg_font, std::to_wstring ( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ), false, true );

			break;
		case esp_widget_pos_t::pos_top:
			render::outline ( cursor_pos.x + 30, cursor_pos.y + 20 - cur_offset_top - 4, player_image_res_w * 0.7f, 4, D3DCOLOR_RGBA ( 0, 0, 0, std::clamp< int > ( static_cast< float >( bar.picker->clr.a ) / 2, 0, 60 ) ) );
			render::rectangle ( cursor_pos.x + 1 + 30, cursor_pos.y + 20 - cur_offset_top - 4 + 1, static_cast< float >( player_image_res_w * 0.7f )* ( static_cast< float >( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ) / 100.0f ) + 1, 4 - 1, D3DCOLOR_RGBA ( bar.picker->clr.r, bar.picker->clr.g, bar.picker->clr.b, bar.picker->clr.a ) );
			cur_offset_top += 6;

			if ( settings.show_value )
				render::text ( cursor_pos.x + 1 + 30 + static_cast< float >( player_image_res_w * 0.7f )* ( static_cast< float >( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ) / 100.0f ) + 1, cursor_pos.y + 20 - cur_offset_top - 4 + 1, D3DCOLOR_RGBA ( 255, 255, 255, 255 ), features::esp::dbg_font, std::to_wstring ( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ), false, true );

			break;
		}

		if ( dragging ) {
			parent_window.draw_overlay ( [ = ] ( ) {
				switch ( bar.queued_location ) {
				case esp_widget_pos_t::pos_left:
				case esp_widget_pos_t::pos_right:
					render::outline ( mouse_pos.x - cur_offset_left, mouse_pos.y - static_cast< float >( player_image_res_h * 0.7f ) / 2, 4, player_image_res_h * 0.7f, D3DCOLOR_RGBA ( 0, 0, 0, std::clamp< int > ( static_cast< float >( bar.picker->clr.a ) / 2, 0, 60 ) ) );
					render::rectangle ( mouse_pos.x - cur_offset_left + 1, mouse_pos.y + calc_height + 1 - static_cast< float >( player_image_res_h * 0.7f ) / 2, 4 - 1, static_cast< int >( player_image_res_h * 0.7f ) - calc_height, D3DCOLOR_RGBA ( bar.picker->clr.r, bar.picker->clr.g, bar.picker->clr.b, bar.picker->clr.a ) );
					render::text ( mouse_pos.x - cur_offset_left + 1, mouse_pos.y + calc_height + 1 - static_cast< float >( player_image_res_h * 0.7f ) / 2 + static_cast< int >( player_image_res_h * 0.7f ) - calc_height, D3DCOLOR_RGBA ( 255, 255, 255, 255 ), features::esp::dbg_font, std::to_wstring ( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ), false, true );
					break;
				case esp_widget_pos_t::pos_bottom:
				case esp_widget_pos_t::pos_top:
					render::outline ( mouse_pos.x - static_cast< float >( player_image_res_w * 0.7f ) / 2, mouse_pos.y, player_image_res_w * 0.7f, 4, D3DCOLOR_RGBA ( 0, 0, 0, std::clamp< int > ( static_cast< float >( bar.picker->clr.a ) / 2, 0, 60 ) / 2 ) );
					render::rectangle ( mouse_pos.x - static_cast< float >( player_image_res_w * 0.7f ) / 2 + 1, mouse_pos.y + 1, static_cast< float >( player_image_res_w * 0.7f )* ( static_cast< float >( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ) / 100.0f ) + 1, 4 - 1, D3DCOLOR_RGBA ( bar.picker->clr.r, bar.picker->clr.g, bar.picker->clr.b, bar.picker->clr.a / 2 ) );
					render::text ( mouse_pos.x - static_cast< float >( player_image_res_w * 0.7f ) / 2 + 1 + static_cast< float >( player_image_res_w * 0.7f )* ( static_cast< float >( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ) / 100.0f ) + 1, mouse_pos.y + 1, D3DCOLOR_RGBA ( 255, 255, 255, 255 ), features::esp::dbg_font, std::to_wstring ( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ), false, true );
					break;
				}
			} );
		}
	};

	auto render_text_preview = [ & ] ( esp_widget_t& bar, const str& sample_text ) {
		if ( !bar.enabled )
			return;

		rect text_dim;
		binds::text_bounds ( reinterpret_cast< oxui::font >( features::esp::esp_font ), sample_text, text_dim );

		auto dragging = false;
		pos mouse_pos = pos ( 0, 0 );

		if ( utils::key_state ( VK_LBUTTON ) && ( ( shapes::hovering ( { cursor_pos.x + 30, cursor_pos.y + 20 + static_cast< int >( player_image_res_h * 0.7f ) + cur_offset_bottom, static_cast< int >( player_image_res_w * 0.7f ), text_dim.h }, true ) && bar.location == esp_widget_pos_t::pos_bottom )
			|| ( shapes::hovering ( { cursor_pos.x + 30, cursor_pos.y + 20 - cur_offset_top - text_dim.h, static_cast< int >( player_image_res_w * 0.7f ), text_dim.h }, true ) && bar.location == esp_widget_pos_t::pos_top )
			|| ( shapes::hovering ( { cursor_pos.x + 30 - cur_offset_left - text_dim.w, cursor_pos.y + 20 + cur_offset_left_height, text_dim.w, static_cast< int > ( player_image_res_h * 0.7f ) }, true ) && bar.location == esp_widget_pos_t::pos_left )
			|| ( shapes::hovering ( { cursor_pos.x + 30 + static_cast< int > ( player_image_res_w * 0.7f ) + cur_offset_right , cursor_pos.y + 20 + cur_offset_right_height, text_dim.w, static_cast< int > ( player_image_res_h * 0.7f ) }, true ) && bar.location == esp_widget_pos_t::pos_right ) ) ) {
			if ( shapes::hovering ( { cursor_pos.x + 30, cursor_pos.y + 20 + static_cast< int >( player_image_res_h * 0.7f ), static_cast< int >( player_image_res_w * 0.7f ), 50 } ) )
				bar.queued_location = esp_widget_pos_t::pos_bottom;
			else if ( shapes::hovering ( { cursor_pos.x + 30, cursor_pos.y + 20 - 50, static_cast< int >( player_image_res_w * 0.7f ), 50 } ) )
				bar.queued_location = esp_widget_pos_t::pos_top;
			else if ( shapes::hovering ( { cursor_pos.x + 30 - 50, cursor_pos.y + 20, 50, static_cast< int >( player_image_res_h * 0.7f ) } ) )
				bar.queued_location = esp_widget_pos_t::pos_left;
			else if ( shapes::hovering ( { cursor_pos.x + 30 + static_cast< int >( player_image_res_w * 0.7f ), cursor_pos.y + 20, 50, static_cast< int >( player_image_res_h * 0.7f ) } ) )
				bar.queued_location = esp_widget_pos_t::pos_right;

			binds::mouse_pos ( mouse_pos );
			dragging = true;
		}

		if ( !utils::key_state ( VK_LBUTTON ) && bar.queued_location != bar.location )
			bar.location = bar.queued_location;

		const auto calc_height = static_cast< float >( player_image_res_h * 0.7f )* ( static_cast< float >( static_cast< int >( std::sin ( parent_panel.time * 0.2 * 3.141f ) * 50.0 + 50.0 ) ) / 100.0f );

		switch ( bar.location ) {
		case esp_widget_pos_t::pos_left:
			binds::text ( { cursor_pos.x + 30 - cur_offset_left - text_dim.w, cursor_pos.y + 20 + cur_offset_left_height }, reinterpret_cast< oxui::font >( features::esp::esp_font ), sample_text, bar.picker->clr, true );
			cur_offset_left_height += text_dim.h + 4;
			break;
		case esp_widget_pos_t::pos_right:
			binds::text ( { cursor_pos.x + 30 + cur_offset_right + static_cast< int >( player_image_res_w * 0.7f ), cursor_pos.y + 20 + cur_offset_right_height }, reinterpret_cast< oxui::font >( features::esp::esp_font ), sample_text, bar.picker->clr, true );
			cur_offset_right_height += text_dim.h + 4;
			break;
		case esp_widget_pos_t::pos_bottom:
			binds::text ( { cursor_pos.x + 30 + static_cast< int >( player_image_res_w * 0.7f ) / 2 - text_dim.w / 2, cursor_pos.y + 20 + static_cast< int >( player_image_res_h * 0.7f ) + cur_offset_bottom }, reinterpret_cast< oxui::font >( features::esp::esp_font ), sample_text, bar.picker->clr, true );
			cur_offset_bottom += text_dim.h + 4;
			break;
		case esp_widget_pos_t::pos_top:
			binds::text ( { cursor_pos.x + 30 + static_cast< int >( player_image_res_w * 0.7f ) / 2 - text_dim.w / 2, cursor_pos.y + 20 - cur_offset_top - text_dim.h }, reinterpret_cast< oxui::font >( features::esp::esp_font ), sample_text, bar.picker->clr, true );
			cur_offset_top += text_dim.h + 4;
			break;
		}

		if ( dragging ) {
			parent_window.draw_overlay ( [ = ] ( ) {
				binds::text ( { mouse_pos.x - text_dim.w / 2, mouse_pos.y - text_dim.h / 2 }, reinterpret_cast< oxui::font >( features::esp::esp_font ), sample_text, { bar.picker->clr.r, bar.picker->clr.g, bar.picker->clr.b, std::clamp< int > ( static_cast< float >( bar.picker->clr.a ) / 2, 0, 60 ) }, true );
			} );
		}
	};

	/* esp widget previews */
	render_bar_preview ( settings.health_bar );
	render_bar_preview ( settings.ammo_bar );
	render_bar_preview ( settings.desync_bar );
	render_text_preview ( settings.esp_name, OSTR ( "Sesame" ) );
	render_text_preview ( settings.esp_weapon, OSTR ( "AWP" ) );

	/* draw rclick menu items */
	if ( opened_shortcut_menu ) {
		parent_window.draw_overlay ( [ = ] ( ) {
			std::vector< str > rclick_menu_items {
			OSTR ( "Default Model" ),
			OSTR ( "Textured Model" ),
			OSTR ( "Flat Model" ),
			settings.xqz ? OSTR ( "Disable XQZ" ) : OSTR ( "Enable XQZ" ),
			settings.backtrack ? OSTR ( "Disable Backtrack Chams" ) : OSTR ( "Enable Backtrack Chams" ),
			settings.hit_matrix ? OSTR ( "Disable Hit Matrix" ) : OSTR ( "Enable Hit Matrix" ),
			settings.glow ? OSTR ( "Remove Glow" ) : OSTR ( "Add Glow" ),
			settings.rimlight ? OSTR ( "Remove Rimlight" ) : OSTR ( "Add Rimlight" ),
			settings.esp_box.enabled ? OSTR ( "Remove ESP Box" ) : OSTR ( "Add ESP Box" ),
			settings.health_bar.enabled ? OSTR ( "Remove Health Bar" ) : OSTR ( "Add Health Bar" ),
			settings.ammo_bar.enabled ? OSTR ( "Remove Ammo Bar" ) : OSTR ( "Add Ammo Bar" ),
			settings.desync_bar.enabled ? OSTR ( "Remove Desync Bar" ) : OSTR ( "Add Desync Bar" ),
			settings.show_value ? OSTR ( "Remove Value Text" ) : OSTR ( "Add Value Text" ),
			settings.esp_name.enabled ? OSTR ( "Remove Nametag" ) : OSTR ( "Add Nametag" ),
			settings.esp_weapon.enabled ? OSTR ( "Remove Weapon Name" ) : OSTR ( "Add Weapon Name" )
			};

			/* render the items name */
			auto index = 0;

			/* background of the list */
			binds::rounded_rect ( { rclick_pos.x, rclick_pos.y, 150, theme.spacing + 8 }, 8, 16, hovered_index == index ? theme.bg : theme.container_bg, false );
			binds::rounded_rect ( { rclick_pos.x, rclick_pos.y + theme.spacing * ( static_cast< int > ( rclick_menu_items.size ( ) ) - 1 ) - 8, 150, theme.spacing + 8 }, 8, 16, hovered_index == ( rclick_menu_items.size ( ) - 1 ) ? theme.bg : theme.container_bg, false );

			for ( const auto& it : rclick_menu_items ) {
				/* get the text size */
				rect item_text_size;
				binds::text_bounds ( font, it, item_text_size );

				/* render the square background if not first or last (middle of rounded rectangles) */
				if ( index && index != rclick_menu_items.size ( ) - 1 )
					binds::fill_rect ( { rclick_pos.x, rclick_pos.y + theme.spacing * index, 150, theme.spacing }, hovered_index == index ? theme.bg : theme.container_bg );

				/* render the name */
				binds::text ( { rclick_pos.x + 150 / 2 - item_text_size.w / 2 - 1, rclick_pos.y + theme.spacing * index + theme.spacing / 2 - item_text_size.h / 2 }, font, it, hovered_index == index ? theme.main : theme.text, hovered_index == index );

				index++;
			}

			/* outline of the list */
			binds::rounded_rect ( { rclick_pos.x, rclick_pos.y, 150, theme.spacing * static_cast< int > ( rclick_menu_items.size ( ) ) }, 8, 16, { 0, 0, 0, 90 }, true );
		} );
	}
}