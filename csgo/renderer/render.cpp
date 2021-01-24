#include "render.hpp"

#include <unordered_map>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"

std::unordered_map<std::string, void*> font_list {};

void render::create_font ( const uint8_t* data, size_t data_size, std::string_view family_name, float size, const uint16_t* ranges, void* font_config ) {
	ImGuiIO& io = ImGui::GetIO ( );

	font_list[ family_name.data ( ) ] = io.Fonts->AddFontFromMemoryTTF ( (void*) data, data_size, size, reinterpret_cast< ImFontConfig *>( font_config), ranges ? ranges : io.Fonts->GetGlyphRangesCyrillic() );
}

void render::screen_size ( float& width, float& height ) {
	int temp_w, temp_h;
	cs::i::engine->get_screen_size ( temp_w, temp_h );
	width = temp_w;
	height = temp_h;
}

void render::text_size ( std::string_view text, std::string_view font, vec3_t& dimentions ) {
	RECT rect = { 0, 0, 0, 0 };
	ImGui::PushFont ( reinterpret_cast<ImFont*>( font_list [ font.data() ] ) );
	const auto text_size = ImGui::CalcTextSize ( text.data ( ) );
	ImGui::PopFont ( );
	dimentions = { text_size.x, text_size.y };
}

void render::rect ( float x, float y, float width, float height, uint32_t color ) {
	ImGui::GetWindowDrawList ( )->AddRectFilled ( { round(x),round(y) }, { round(x + width), round(y + height)}, color );
}

void render::gradient ( float x, float y, float width, float height, uint32_t color1, uint32_t color2, bool is_horizontal ) {
	ImGui::GetWindowDrawList ( )->AddRectFilledMultiColor ( { round (x),round (y) }, { round(x + width), round (y + height) }, color1, is_horizontal ? color2 : color1, color2, is_horizontal ? color1 : color2 );
}

void render::outline ( float x, float y, float width, float height, uint32_t color ) {
	ImGui::GetWindowDrawList ( )->AddRect ( { round (x),round (y) }, { round (x + width), round (y + height) }, color );
}

void render::line ( float x, float y, float x2, float y2, uint32_t color, float thickness ) {
	ImGui::GetWindowDrawList ( )->AddLine ( { round (x),round (y) }, { round (x2),round (y2) }, color, thickness );
}

void render::text ( float x, float y, std::string_view text, std::string_view font, uint32_t color, bool outline ) {
	const auto draw_list = ImGui::GetWindowDrawList ( );

	ImGui::PushFont ( reinterpret_cast< ImFont* >( font_list [ font.data ( ) ] ) );

	if ( outline ) {
		//draw_list->AddText ( { round(x - 1.0f), round(y - 1.0f) }, color & IM_COL32_A_MASK, text.data ( ) );
		//draw_list->AddText ( { round(x - 1.0f), round(y + 1.0f) }, color & IM_COL32_A_MASK, text.data ( ) );
		draw_list->AddText ( { round(x + 1.0f), round(y + 1.0f) }, color & IM_COL32_A_MASK, text.data ( ) );
		//draw_list->AddText ( { round(x + 1.0f), round(y - 1.0f) }, color & IM_COL32_A_MASK, text.data ( ) );
	}

	draw_list->AddText ( { round (x),round (y)}, color, text.data ( ) );

	ImGui::PopFont ( );
}

void render::circle ( float x, float y, float radius, int segments, uint32_t color, bool outline ) {
	if(!outline )
		ImGui::GetWindowDrawList ( )->AddCircleFilled ( { round (x),round (y) }, radius, color, segments );
	else
		ImGui::GetWindowDrawList ( )->AddCircle ( { round (x),(y) }, radius, color, segments );
}

void render::polygon ( const std::vector< vec3_t >& verticies, uint32_t color, bool outline, float thickness ) {
	std::vector< ImVec2 > points_2d {};

	for ( auto& point : verticies )
		points_2d.push_back ( { round (point.x), round (point.y)} );
		
	if ( outline )
		ImGui::GetWindowDrawList ( )->AddPolyline ( points_2d.data ( ), points_2d.size ( ), color, true, thickness );
	else
		ImGui::GetWindowDrawList ( )->AddConvexPolyFilled ( points_2d.data(), points_2d.size(), color );
}

void render::clip ( float x, float y, float width, float height, const std::function< void ( ) >& func ) {
	ImGui::PushClipRect ( { round (x), round (y) }, { round (x + width),round (y + height)},true );

	func ( );

	ImGui::PopClipRect ( );
}

void render::circle3d ( const vec3_t& pos, float rad, int segments, uint32_t color, bool outline, float thickness ) {
	std::vector< ImVec2 > points { };

	auto src_point = pos;
	src_point.y += static_cast< float > ( rad );

	auto rotate_point = [ ] ( vec3_t pivot, vec3_t point, float ang ) -> vec3_t {
		const auto s = std::sinf ( ang );
		const auto c = std::cosf ( ang );

		point.x -= pivot.x;
		point.y -= pivot.y;

		const auto new_x = point.x * c - point.y * s;
		const auto new_y = point.x * s + point.y * c;

		point.x = new_x + pivot.x;
		point.y = new_y + pivot.y;

		return point;
	};

	auto segment_num = 0;

	for ( auto i = 0.0f; i < cs::pi * 2.0f; i += ( cs::pi * 2.0f ) / static_cast< float > ( segments ) ) {
		auto new_point = rotate_point ( pos, src_point, i );
		vec3_t screen;

		cs::render::world_to_screen ( screen, new_point );

		points.push_back ( { round(screen.x), round (screen.y )} );
	}

	if ( outline )
		ImGui::GetWindowDrawList ( )->AddPolyline ( points.data ( ), points.size ( ), color, true, thickness );
	else
		ImGui::GetWindowDrawList ( )->AddConvexPolyFilled ( points.data ( ), points.size ( ), color );
}

void render::cube ( const vec3_t& pos, float size, uint32_t color, float thickness ) {
	static vec3_t scrn_one;
	static vec3_t scrn_two;
	static vec3_t scrn_three;
	static vec3_t scrn_four;
	static vec3_t scrn_five;
	static vec3_t scrn_six;
	static vec3_t scrn_seven;
	static vec3_t scrn_eight;

	static vec3_t one;
	static vec3_t two;
	static vec3_t three;
	static vec3_t four;
	static vec3_t five;
	static vec3_t six;
	static vec3_t seven;
	static vec3_t eight;

	const auto mdist = static_cast< float >( size ) / 2.0f;

	one = vec3_t ( pos.x - mdist, pos.y - mdist, pos.z - mdist );
	two = vec3_t ( pos.x - mdist, pos.y - mdist, pos.z + mdist );
	three = vec3_t ( pos.x - mdist, pos.y + mdist, pos.z + mdist );
	four = vec3_t ( pos.x - mdist, pos.y + mdist, pos.z - mdist );
	five = vec3_t ( pos.x + mdist, pos.y - mdist, pos.z - mdist );
	six = vec3_t ( pos.x + mdist, pos.y - mdist, pos.z + mdist );
	seven = vec3_t ( pos.x + mdist, pos.y + mdist, pos.z + mdist );
	eight = vec3_t ( pos.x + mdist, pos.y + mdist, pos.z - mdist );

	if ( !cs::render::world_to_screen ( scrn_one, one )
		|| !cs::render::world_to_screen ( scrn_two, two )
		|| !cs::render::world_to_screen ( scrn_three, three )
		|| !cs::render::world_to_screen ( scrn_four, four )
		|| !cs::render::world_to_screen ( scrn_five, five )
		|| !cs::render::world_to_screen ( scrn_six, six )
		|| !cs::render::world_to_screen ( scrn_seven, seven )
		|| !cs::render::world_to_screen ( scrn_eight, eight ) )
		return;

	/* back */
	line ( scrn_one.x, scrn_one.y, scrn_two.x, scrn_two.y, color, thickness );
	line ( scrn_two.x, scrn_two.y, scrn_three.x, scrn_three.y, color, thickness );
	line ( scrn_three.x, scrn_three.y, scrn_four.x, scrn_four.y, color, thickness );
	line ( scrn_four.x, scrn_four.y, scrn_one.x, scrn_one.y, color, thickness );

	/* front */
	line ( scrn_five.x, scrn_five.y, scrn_six.x, scrn_six.y, color, thickness );
	line ( scrn_six.x, scrn_six.y, scrn_seven.x, scrn_seven.y, color, thickness );
	line ( scrn_seven.x, scrn_seven.y, scrn_eight.x, scrn_eight.y, color, thickness );
	line ( scrn_eight.x, scrn_eight.y, scrn_five.x, scrn_five.y, color, thickness );

	/* connect sides */
	line ( scrn_one.x, scrn_one.y, scrn_five.x, scrn_five.y, color, thickness );
	line ( scrn_two.x, scrn_two.y, scrn_six.x, scrn_six.y, color, thickness );
	line ( scrn_three.x, scrn_three.y, scrn_seven.x, scrn_seven.y, color, thickness );
	line ( scrn_four.x, scrn_four.y, scrn_eight.x, scrn_eight.y, color, thickness );
}