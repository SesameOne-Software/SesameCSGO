#pragma once
#include <xmmintrin.h>
#include "matrix3x4.h"
#include "entity.h"

struct plane_t {
	vec3_t m_normal;
	float m_dist;
	std::uint8_t m_type;
	std::uint8_t m_sign_bits;
	PAD( 2 );
};

struct surface_t {
	const char* m_name;
	short m_surface_props;
	std::uint8_t m_flags;
};

class base_trace_t {
public:
	vec3_t m_startpos;
	vec3_t m_endpos;
	plane_t m_plane;

	float m_fraction;

	std::uint32_t m_contents;
	std::uint16_t m_disp_flags;

	bool m_allsolid;
	bool m_startsolid;
};

class trace_t : public base_trace_t {
public:
	int get_entity_index( ) const { }

	bool did_hit( ) const {
		return m_fraction < 1 || m_allsolid || m_startsolid;
	}

	bool is_visible( ) const {
		return m_fraction > 0.97f;
	}

public:
	float m_fractionleftsolid;
	surface_t m_surface;
	int m_hitgroup;
	short m_physicsbone;
	std::uint16_t m_world_surface_index;
	entity_t* m_hit_entity;
	int m_hitbox;

	trace_t( ) { }

private:
	trace_t( const trace_t& other ) :
		m_fractionleftsolid( other.m_fractionleftsolid ),
		m_surface( other.m_surface ),
		m_hitgroup( other.m_hitgroup ),
		m_physicsbone( other.m_physicsbone ),
		m_world_surface_index( other.m_world_surface_index ),
		m_hit_entity( other.m_hit_entity ),
		m_hitbox( other.m_hitbox ) {
		m_startpos = other.m_startpos;
		m_endpos = other.m_endpos;
		m_plane = other.m_plane;
		m_fraction = other.m_fraction;
		m_contents = other.m_contents;
		m_disp_flags = other.m_disp_flags;
		m_allsolid = other.m_allsolid;
		m_startsolid = other.m_startsolid;
	}
};

class __trace_filter_t {
public:
	virtual bool should_hit_ent( entity_t* ent, std::uint32_t mask ) = 0;
	virtual std::uint32_t get_trace_type( ) const = 0;
};

class _trace_filter_t : public __trace_filter_t {
public:
	void* m_skip;

	bool should_hit_ent( entity_t* ent, std::uint32_t ) override {
		return ent != m_skip;
	}

	std::uint32_t get_trace_type( ) const override {
		return 0;
	}
};

class trace_filter_t : public _trace_filter_t {
public:
	void* m_skip;

	trace_filter_t( void* ent ) {
		m_skip = ent;
	}

	bool should_hit_ent( entity_t* ent, std::uint32_t ) override {
		return !( ent == m_skip );
	}

	std::uint32_t get_trace_type( ) const override {
		return 0;
	}
};

struct ray_t {
	vec3_t start;
	vec3_t delta;
	vec3_t start_offset;
	vec3_t extents;

	const matrix3x4_t* world_axis_transform{};

	bool is_ray{};
	bool is_swept{};

	void init(const vec3_t src, const vec3_t end) {
		delta = end - src;
		is_swept = delta.length() != 0.f;
		extents.x = extents.y = extents.z = 0.0f;
		world_axis_transform = nullptr;
		is_ray = true;
		start_offset.x = start_offset.y = start_offset.z = 0.0f;
		start = src;
	}

	void init(vec3_t const& start, vec3_t const& end, vec3_t const& mins, vec3_t const& maxs) {
		world_axis_transform = nullptr;
		delta = end - start;
		extents = maxs - mins;
		extents *= 0.5f;

		is_swept = delta.length_sqr( ) != 0.0f;
		is_ray = extents.length_sqr( ) < 1e-6f;

		start_offset = maxs + mins;
		start_offset *= 0.5f;
		this->start = start + start_offset;
		start_offset *= -1.0f;
	}
};

class c_engine_trace {
public:
	virtual int get_point_contents(const vec3_t &pos, int mask = 0xffffffff /* mask_all */, entity_t *ent = nullptr) = 0; // 0
	virtual int get_point_contents_world_only(const vec3_t &pos, int mask = 0xffffffff) = 0; // 1
	virtual int get_point_contents_collideable(void *collide /* collidable_t */, const vec3_t &pos) = 0; // 2
	virtual void clip_ray_to_entity(const ray_t &ray, unsigned int mask, void *ent, trace_t *trace) = 0; // 3
	virtual void clip_ray_to_collideable(const ray_t &ray, unsigned int mask, void *collide /* collidable_t */, trace_t *trace) = 0; // 4
	virtual void trace_ray(const ray_t &ray, unsigned int mask, trace_filter_t *filter, trace_t *trace) = 0; // 5
};