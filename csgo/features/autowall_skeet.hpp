#pragma once
#include "../sdk/sdk.hpp"

namespace awall_skeet {
	/* stripped down struct from skeet */
	struct bullet_data_t {
		player_t* player;
		float damage_out;
		bool penetrate_fail;
		int penetrate_count;
		trace_t enter_trace;
		float distance;
		int wanted_hitgroup;
	};

	__forceinline float scale_dmg ( int hitgroup, player_t* player, float dmg, float armor_ratio, weapons_t item_id ) {
		static float hitgroup_damage_mult [ ] = {
			1.0f,
			4.0f,
			1.0f,
			1.25f,
			1.0f,
			1.0f,
			0.75f,
			0.75f,
			1.0f,
			1.0f,
			1.0f,
			1.0f,
			1.0f,
			1.0f,
			1.0f,
			1.0f,
		};

		if ( item_id == weapons_t::taser )
			dmg *= 1.0f;
		else
			dmg *= hitgroup_damage_mult [ hitgroup ];

		if ( player && player->armor ( ) > 0 && ( hitgroup != hitgroup_head || ( hitgroup == hitgroup_head && player->has_helmet ( ) ) ) )
			dmg *= armor_ratio * 0.5f;

		return dmg;
	}

	bool fire_bullet ( player_t* local, vec3_t src, const vec3_t& dir, float min_dmg, weapon_t* weapon, bullet_data_t& data, uint32_t mask = mask_shot_hull | contents_hitbox );

	__forceinline float dmg ( player_t* local, const vec3_t& src, const vec3_t& dst, weapon_t* weapon, player_t* dst_player = nullptr, float min_dmg = 1.0f, bool floating_point = false, int wanted_hitgroup = -1 ) {
		bullet_data_t bullet_data = { dst_player, 0.0f, false, 4, trace_t ( ), src.dist_to ( dst ), wanted_hitgroup };

		if ( !fire_bullet ( local, src, ( dst - src ).normalized ( ), min_dmg, weapon, bullet_data, floating_point ? mask_shot_hull : ( mask_shot_hull | contents_hitbox ) ) )
			return 0.0f;

		return bullet_data.damage_out;
	}
}