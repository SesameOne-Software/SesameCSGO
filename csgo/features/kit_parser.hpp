/* This file is part of nSkinz by namazso, licensed under the MIT license:
*
* MIT License
*
* Copyright (c) namazso 2018
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#pragma once
#include <vector>
#include <string>

#include "../sdk/weapon.hpp"

namespace features {
	namespace skinchanger {
		struct paint_kit {
			int id;
			std::string name, image_name;
			int rarity, style;

			bool operator < ( const paint_kit& other ) const {
				return name < other.name;
			}
		};

		inline bool skin_changed = false;
		inline std::vector<paint_kit> skin_kits;
		inline std::vector<paint_kit> glove_kits;
		inline std::vector<paint_kit> sticker_kits;

		inline std::vector<const char*> knife_names = {
			"Bayonet",
			"Flip",
			"Gut",
			"Karambit",
			"M9 Bayonet",
			"Tactical",
			"Butterfly",
			"Falchion",
			"Shadow Dagger",
			"Survival Bowie",
			"Ursus",
			"Navaja",
			"Stiletto",
			"Talon",
			"Classic Knife",
			"Paracord",
			"Survival",
			"Nomad",
			"Skeleton"
		};

		inline std::vector<weapons_t> knife_ids = {
			weapons_t::knife_bayonet,
			weapons_t::knife_flip,
			weapons_t::knife_gut,
			weapons_t::knife_karambit,
			weapons_t::knife_m9_bayonet,
			weapons_t::knife_huntsman,
			weapons_t::knife_butterfly,
			weapons_t::knife_falchion,
			weapons_t::knife_shadow_daggers,
			weapons_t::knife_bowie,
			weapons_t::knife_ursus,
			weapons_t::knife_gypsy_jackknife,
			weapons_t::knife_stiletto,
			weapons_t::knife_widowmaker,
			weapons_t::knife_css,
			weapons_t::knife_cord,
			weapons_t::knife_canis,
			weapons_t::knife_outdoor,
			weapons_t::knife_skeleton
		};

		inline std::vector<const char*> knife_models = {
			"models/weapons/v_knife_bayonet.mdl",
			"models/weapons/v_knife_flip.mdl",
			"models/weapons/v_knife_gut.mdl",
			"models/weapons/v_knife_karam.mdl",
			"models/weapons/v_knife_m9_bay.mdl",
			"models/weapons/v_knife_tactical.mdl",
			"models/weapons/v_knife_butterfly.mdl",
			"models/weapons/v_knife_falchion_advanced.mdl",
			"models/weapons/v_knife_push.mdl",
			"models/weapons/v_knife_survival_bowie.mdl",
			"models/weapons/v_knife_ursus.mdl",
			"models/weapons/v_knife_gypsy_jackknife.mdl",
			"models/weapons/v_knife_stiletto.mdl",
			"models/weapons/v_knife_widowmaker.mdl",
			"models/weapons/v_knife_css.mdl",
			"models/weapons/v_knife_cord.mdl",
			"models/weapons/v_knife_canis.mdl",
			"models/weapons/v_knife_outdoor.mdl",
			"models/weapons/v_knife_skeleton.mdl"
		};

		inline std::vector<const char*> knife_weapon_names = {
			"bayonet",
			"knife_flip",
			"knife_gut",
			"knife_karambit",
			"knife_m9_bayonet",
			"knife_tactical",
			"knife_butterfly",
			"knife_falchion",
			"knife_push",
			"knife_survival_bowie",
			"knife_ursus",
			"knife_gypsy_jackknife",
			"knife_stiletto",
			"knife_widowmaker",
			"knife_css",
			"knife_cord",
			"knife_canis",
			"knife_outdoor",
			"knife_skeleton"
		};
		
		void dump_kits ( );
	}
}