#pragma once
#include <d3d9.h>
#include <d3dx9.h>

namespace menu {
	extern IDirect3DTexture9* desync_none;
	extern IDirect3DTexture9* desync_add;
	extern IDirect3DTexture9* desync_sub;

	enum class desync_types {
		none,
		subtractive,
		additive,
		inward,
		outward
	};

	void draw_desync_model( desync_types desync_type, float real, float x, float y );
	void save( const std::string& file );
	void load( const std::string& file );
	void init( );
	void render( );
}