#pragma once
#include <cstdint>
#include <string_view>
#include <functional>
#include "../sdk/sdk.hpp"

namespace render {
	struct vtx_t {
		float x, y, z, rhw;
		std::uint32_t color;
	};

	struct custom_vtx_t {
		float x, y, z, rhw;
		std::uint32_t color;
		float tu, tv;
	};

	struct pos {
		int x, y;
	};

	struct dim {
		int w, h;
	};

	struct rect {
		int x, y, w, h;
	};

	void create_font( void** font, const std::wstring_view& family, int size, bool bold );
	void screen_size( int& width, int& height );
	void text_size( void* font, const std::wstring_view& text, dim& dimentions );
	void rectangle( int x, int y, int width, int height, std::uint32_t color );
	void gradient( int x, int y, int width, int height, std::uint32_t color1, std::uint32_t color2, bool is_horizontal );
	void outline( int x, int y, int width, int height, std::uint32_t color );
	void line( int x, int y, int x2, int y2, std::uint32_t color );
	void text( int x, int y, std::uint32_t color, void* font, const std::wstring_view& text, bool shadow = false, bool outline = false );
	void circle( int x, int y, int radius, int segments, std::uint32_t color, int fraction = 0, float rotation = 0.0f, bool outline = false );
	void texture( ID3DXSprite* sprite, IDirect3DTexture9* tex, int x, int y, int width, int height, float scalex, float scaley, uint32_t clr = 0xffffffff );
	void clip ( int x, int y, int width, int height, const std::function< void ( ) >& func );
	bool key_pressed( const std::uint32_t key );
	void circle3d ( const vec3_t& pos, int rad, int segments, std::uint32_t color, bool outline = false );
	void cube ( const vec3_t& pos, int size, std::uint32_t color );
	void polygon ( const std::vector< std::pair< float, float > >& verticies, std::uint32_t color, bool outline );
	void gradient_rounded_rect ( int x, int y, int width, int height, int rad, int resolution, std::uint32_t color, std::uint32_t color1, bool outline );
	void rounded_alpha_rect ( int x, int y, int width, int height, int rad, int resolution );
	void rounded_rect ( int x, int y, int width, int height, int rad, int resolution, std::uint32_t color, bool outline );

	void mouse_pos( pos& position );
}