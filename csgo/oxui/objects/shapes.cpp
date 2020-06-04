#include "shapes.hpp"
#include "../themes/purple.hpp"
#include "../panels/panel.hpp"

bool g_oxui_input_clip_area = false;
oxui::rect g_oxui_input_clip_rect = oxui::rect( );

bool oxui::shapes::finished_input_frame = false;
bool oxui::shapes::click_switch = false;
bool oxui::shapes::old_click_switch = false;
oxui::pos oxui::shapes::click_start;

bool hits_clip ( oxui::pos& pos ) {
	if ( !g_oxui_input_clip_area )
		return false;

	return !( pos.x >= g_oxui_input_clip_rect.x && pos.y >= g_oxui_input_clip_rect.y && pos.x <= g_oxui_input_clip_rect.x + g_oxui_input_clip_rect.w && pos.y <= g_oxui_input_clip_rect.y + g_oxui_input_clip_rect.h );
}

bool oxui::shapes::hovering( const rect& area, bool from_start, bool override ) {
	if ( ( !g_input && !override ) || finished_input_frame )
		return false;

	pos mouse_pos;
	binds::mouse_pos( mouse_pos );

	if ( from_start ) {
		if ( click_start.x && click_start.y )
			return click_start.x >= area.x && click_start.y >= area.y && click_start.x <= area.x + area.w && click_start.y <= area.y + area.h && !hits_clip ( click_start );

		return false;
	}

	return mouse_pos.x >= area.x && mouse_pos.y >= area.y && mouse_pos.x <= area.x + area.w && mouse_pos.y <= area.y + area.h && !hits_clip ( mouse_pos );
}

bool oxui::shapes::clicking( const rect& area, bool from_start, bool override ) {
	if ( ( !g_input && !override ) || finished_input_frame )
		return false;

	pos mouse_pos;
	binds::mouse_pos ( mouse_pos );

	if ( !click_switch && GetAsyncKeyState( VK_LBUTTON ) && !hits_clip( mouse_pos ) ) { /* press key */
		click_start = mouse_pos;

		if ( from_start ) {
			if ( hovering ( area, true, override ) ) {
				click_switch = true;
				return true;
			}
		}
		else if ( hovering ( area, false, override ) ) {
			click_switch = true;
			finished_input_frame = true;
			return true;
		}
	}
	else if ( click_switch && !GetAsyncKeyState( VK_LBUTTON ) ) { /* release key */
		click_start = pos ( 0, 0 );
	}

	return false;
}

void oxui::shapes::box( const rect& area, const double& hover_time, bool highlight_on_hover, bool top, bool bottom, bool left, bool right, bool background ) {
	if ( background )
		binds::fill_rect( area, theme.container_bg );

	auto hover_highlight_time = theme.animation_speed;
	auto border_max_alpha = 255;
	auto bloom_max_alpha = 62;
	auto bloom_min_alpha = 15;

	if ( highlight_on_hover ) {
		bloom_max_alpha = hover_time > hover_highlight_time ? 62 : int( hover_time * ( 1.0 / hover_highlight_time ) * bloom_max_alpha );
		bloom_min_alpha = hover_time > hover_highlight_time ? 15 : int( hover_time * ( 1.0 / hover_highlight_time ) * bloom_min_alpha );
		border_max_alpha = hover_time > hover_highlight_time ? 255 : int( hover_time * ( 1.0 / hover_highlight_time ) * border_max_alpha );
	}

	if ( top ) {
		binds::line( pos( area.x, area.y ), pos( area.x + area.w, area.y ), color( theme.main.r, theme.main.g, theme.main.b, border_max_alpha ) );
		binds::gradient_rect( rect( area.x, area.y + 1, area.w, 4 ), color( theme.main.r, theme.main.g, theme.main.b, bloom_max_alpha ), color( theme.main.r, theme.main.g, theme.main.b, bloom_min_alpha ), false );
	}

	if ( bottom ) {
		binds::line( pos( area.x, area.y + area.h ), pos( area.x + area.w, area.y + area.h ), color( theme.main.r, theme.main.g, theme.main.b, border_max_alpha ) );
		binds::gradient_rect( rect( area.x, area.y + area.h - 1 - 4, area.w, 4 ), color( theme.main.r, theme.main.g, theme.main.b, bloom_min_alpha ), color( theme.main.r, theme.main.g, theme.main.b, bloom_max_alpha ), false );
	}

	if ( left ) {
		binds::line( pos( area.x, area.y ), pos( area.x, area.y + area.h ), color( theme.main.r, theme.main.g, theme.main.b, border_max_alpha ) );
		binds::gradient_rect( rect( area.x + 1, area.y, 4, area.h ), color( theme.main.r, theme.main.g, theme.main.b, bloom_min_alpha ), color( theme.main.r, theme.main.g, theme.main.b, bloom_max_alpha ), true );
	}

	if ( right ) {
		binds::line( pos( area.x + area.w, area.y ), pos( area.x + area.w, area.y + area.h ), color( theme.main.r, theme.main.g, theme.main.b, border_max_alpha ) );
		binds::gradient_rect( rect( area.x + area.w - 1 - 5, area.y, 4, area.h ), color( theme.main.r, theme.main.g, theme.main.b, bloom_min_alpha ), color( theme.main.r, theme.main.g, theme.main.b, bloom_max_alpha ), true );
	}
}