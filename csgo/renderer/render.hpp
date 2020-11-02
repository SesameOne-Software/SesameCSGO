#pragma once
#include <cstdint>
#include <vector>
#include <string_view>
#include <string>

#include "../sdk/sdk.hpp"

namespace render {
	void create_font ( const uint8_t* data, size_t data_size, const std::string& family_name, float size, const uint16_t* ranges = nullptr );
	inline void create_font ( const std::vector<uint8_t>& data, const std::string& family_name, float size, const uint16_t* ranges = nullptr ) {
		create_font ( data.data(), data.size(), family_name, size, ranges );
	}
	void screen_size ( float& width, float& height );
	void text_size ( std::string_view text, std::string_view font, vec3_t& dimentions );
	void rect ( float x, float y, float width, float height, uint32_t color );
	void gradient ( float x, float y, float width, float height, uint32_t color1, uint32_t color2, bool is_horizontal );
	void outline ( float x, float y, float width, float height, uint32_t color );
	void line ( float x, float y, float x2, float y2, uint32_t color, float thickness = 1.0f );
	void text ( float x, float y, std::string_view text, std::string_view font, uint32_t color, bool outline = false );
	void circle ( float x, float y, float radius, int segments, uint32_t color, bool outline = false );
	void polygon ( const std::vector< vec3_t >& verticies, uint32_t color, bool outline = false, float thickness = 1.0f );
	void clip ( float x, float y, float width, float height, const std::function< void ( ) >& func );
	void circle3d ( const vec3_t& pos, float rad, int segments, uint32_t color, bool outline = false, float thickness = 1.0f );
	void cube ( const vec3_t& pos, float size, uint32_t color, float thickness = 1.0f );
}