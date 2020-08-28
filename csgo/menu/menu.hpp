#pragma once
#include "sesui.hpp"
#include "../sdk/sdk.hpp"
#include "../renderer/d3d9.hpp"

namespace gui {
	extern bool opened;

	void init( );
	void load_cfg_list( );
	void weapon_controls( const std::string& weapon_name );
	void antiaim_controls( const std::string& antiaim_name );
	void player_visuals_controls( const std::string& visual_name );
	void draw( );
}