#pragma once
#include <mutex>
#include "../sdk/sdk.hpp"
#include "../cjson/cJSON.h"

namespace gui {
	extern bool opened;

	extern std::mutex gui_mutex;
	extern int config_access;
	extern std::string config_code;
	extern std::string config_description;
	extern std::string config_user;
	extern uint64_t last_update_time;

	void init( );
	void load_cfg_list( );
	void weapon_controls( const std::string& weapon_name );
	void antiaim_controls( const std::string& antiaim_name );
	void player_visuals_controls( const std::string& visual_name );
	void draw( );

	namespace watermark {
		void draw( );
	}

	namespace keybinds {
		void draw( );
	}
}