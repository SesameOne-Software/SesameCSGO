#pragma once
#include <mutex>
#include "sesui.hpp"
#include "../sdk/sdk.hpp"
#include "../renderer/d3d9.hpp"

namespace gui {
	extern bool opened;

	class c_cloud_config_data {
		std::mutex mtx;

		int config_access = 0;
		std::wstring config_code = _ ( L"" );
		std::wstring config_description = _ ( L"" );

		std::wstring config_user = _ ( L"" );
		uint64_t last_update_time = 0;

		cJSON* selected_cloud_config = nullptr;
		cJSON* cloud_config_list = nullptr;

	public:
		int config_access = 0;
		std::wstring config_code = _ ( L"" );
		std::wstring config_description = _ ( L"" );

		std::wstring config_user = _ ( L"" );
		uint64_t last_update_time = 0;

		cJSON* selected_cloud_config = nullptr;
		cJSON* cloud_config_list = nullptr;


	};

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