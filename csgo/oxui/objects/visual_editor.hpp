#ifndef OXUI_VISUAL_EDITOR_HPP
#define OXUI_VISUAL_EDITOR_HPP

#include <memory>
#include <vector>
#include <functional>
#include "object.hpp"
#include "../types/types.hpp"
#include "colorpicker.hpp"
#include "slider.hpp"

namespace oxui {
	enum class model_type_t : int {
		model_default = 0,
		model_textured,
		model_flat
	};

	enum class esp_widget_pos_t : int {
		pos_center = 0,
		pos_bottom,
		pos_left,
		pos_top,
		pos_right
	};

	enum class esp_widget_type_t : int {
		info_type_box = 0,
		info_type_bar,
		info_type_text,
		info_type_number,
		info_type_flag,
	};

	struct esp_widget_t {
		esp_widget_t ( esp_widget_type_t widget_type, esp_widget_pos_t _location, color _clr ) {
			type = widget_type;
			location = _location;
			queued_location = _location;
		}

		~esp_widget_t ( ) {

		}

		std::shared_ptr< color_picker > picker;
		esp_widget_pos_t queued_location;
		esp_widget_pos_t location;
		esp_widget_type_t type;
		bool enabled = false;
	};

	class visual_editor : public obj {
		pos rclick_pos = pos( 0, 0 );
		bool opened_shortcut_menu = false;
		bool last_rkey = false;
		int hovered_index = 0;

	public:
		struct settings_t {
			model_type_t model_type = model_type_t::model_default;
			int font_size = 18;
			std::shared_ptr< color_picker > cham_picker;
			std::shared_ptr< color_picker > xqz_picker;
			bool xqz = false;
			bool glow = false;
			std::shared_ptr< color_picker > glow_picker;
			bool rimlight = false;
			bool backtrack = false;
			bool hit_matrix = false;
			bool show_value = false;
			std::shared_ptr< slider > reflectivity;
			std::shared_ptr< slider > phong;
			std::shared_ptr< color_picker > backtrack_picker;
			std::shared_ptr< color_picker > hit_matrix_picker;
			std::shared_ptr< color_picker > rimlight_picker;
			esp_widget_t esp_box = esp_widget_t ( esp_widget_type_t::info_type_box, esp_widget_pos_t::pos_center, color ( 255, 255, 255, 255 ) );
			esp_widget_t health_bar = esp_widget_t ( esp_widget_type_t::info_type_bar, esp_widget_pos_t::pos_bottom, color ( 129, 255, 56, 255 ) );
			esp_widget_t ammo_bar = esp_widget_t ( esp_widget_type_t::info_type_bar, esp_widget_pos_t::pos_bottom, color ( 125, 233, 255, 255 ) );
			esp_widget_t desync_bar = esp_widget_t ( esp_widget_type_t::info_type_bar, esp_widget_pos_t::pos_bottom, color ( 221, 110, 255, 255 ) );
			esp_widget_t esp_name = esp_widget_t ( esp_widget_type_t::info_type_text, esp_widget_pos_t::pos_top, color ( 255, 255, 255, 255 ) );
			esp_widget_t esp_weapon = esp_widget_t ( esp_widget_type_t::info_type_text, esp_widget_pos_t::pos_bottom, color ( 255, 255, 255, 255 ) );
		} settings;

		visual_editor ( std::shared_ptr< color_picker > cham_picker,
			std::shared_ptr< slider > reflectivity_slider,
			std::shared_ptr< slider > phong_slider,
			std::shared_ptr< color_picker > backtrack_picker,
			std::shared_ptr< color_picker > hit_matrix_picker,
			std::shared_ptr< color_picker > xqz_picker,
			std::shared_ptr< color_picker > glow_picker,
			std::shared_ptr< color_picker > rimlight_picker,
			std::shared_ptr< color_picker > esp_box_picker,
			std::shared_ptr< color_picker > health_bar_picker,
			std::shared_ptr< color_picker > ammo_bar_picker,
			std::shared_ptr< color_picker > desync_bar_picker,
			std::shared_ptr< color_picker > esp_name_picker,
			std::shared_ptr< color_picker > esp_weapon_picker ) {
			settings.reflectivity = reflectivity_slider;
			settings.phong = phong_slider;
			settings.backtrack_picker = backtrack_picker;
			settings.hit_matrix_picker = hit_matrix_picker;
			settings.cham_picker = cham_picker;
			settings.xqz_picker = xqz_picker;
			settings.glow_picker = glow_picker;
			settings.rimlight_picker = rimlight_picker;
			settings.esp_box.picker = esp_box_picker;
			settings.health_bar.picker = health_bar_picker;
			settings.ammo_bar.picker = ammo_bar_picker;
			settings.desync_bar.picker = desync_bar_picker;
			settings.esp_name.picker = esp_name_picker;
			settings.esp_weapon.picker = esp_weapon_picker;

			type = object_visual_editor;
		}

		~visual_editor ( ) {}

		void think( );
		void draw( ) override;
	};
}

#endif // OXUI_VISUAL_EDITOR_HPP