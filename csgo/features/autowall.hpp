#pragma once
#include <sdk.hpp>

struct fire_bullet_data_t {
	vec3_t src;
	trace_t enter_trace;
	vec3_t direction;
	trace_filter_t filter;
	float trace_length;
	float trace_length_remaining;
	float current_damage;
	int penetrate_count;
};

namespace autowall {
	void clip_trace_to_players( const vec3_t& start, const vec3_t& end, std::uint32_t mask, trace_filter_t* filter, trace_t* trace );
	bool classname_is( player_t* entity, const char* class_name );
	void scale_dmg( player_t* entity, weapon_info_t* weapon_info, int hitgroup, float& current_damage );
	bool simulate_fire_bullet( player_t* entity, player_t* dst_entity, fire_bullet_data_t& data, int hitgroup, vec3_t end_pos );
	bool hbp( player_t* entity, player_t* dst_entity, weapon_info_t* wpn_data, fire_bullet_data_t& data );
	bool is_breakable_entity( player_t* entity );
	bool trace_to_exit( trace_t* tr, player_t* dst_entity, vec3_t start, vec3_t dir, trace_t* exit_tr );
	bool is_armored( player_t* player, int armor, int hitgroup );
	float dmg( player_t* entity, player_t* dst_entity, vec3_t src, vec3_t dst, int hitbox );
	int hitbox_to_hitgroup( int hitbox );
}