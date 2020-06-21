#pragma once
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <functional>
#include "vec3.hpp"

class effect_data_t {
public:
	vec3_t m_origin;
	vec3_t m_start;
	vec3_t m_normal;
	vec3_t m_angles;
	int	m_flags;
	int m_ent_handle;
	float m_scale;
	float m_magnitude;
	float m_radius;
	int m_attachment_idx;
	short m_surface_prop;
	int m_material;
	int m_damage_type;
	int m_hitbox;
	int	m_other_ent_idx;
	unsigned char m_color;
	bool m_positions_are_relative_to_entity;
	int m_effect_name;
};