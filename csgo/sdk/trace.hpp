#pragma once
#include <xmmintrin.h>
#include "matrix3x4.hpp"
#include "entity.hpp"

enum contents_t : uint32_t {
	contents_empty = 0,
	contents_solid = 0x1,
	contents_window = 0x2,
	contents_aux = 0x4,
	contents_grate = 0x8,
	contents_slime = 0x10,
	contents_water = 0x20,
	contents_blocklos = 0x40,
	contents_opaque = 0x80,
	last_visible_contents = contents_opaque,
	all_visible_contents = ( last_visible_contents | ( last_visible_contents - 1 ) ),
	contents_testfogvolume = 0x100,
	contents_unused = 0x200,
	contents_blocklight = 0x400,
	contents_team1 = 0x800,
	contents_team2 = 0x1000,
	contents_ignore_nodraw_opaque = 0x2000,
	contents_moveable = 0x4000,
	contents_areaportal = 0x8000,
	contents_playerclip = 0x10000,
	contents_monsterclip = 0x20000,
	contents_current_0 = 0x40000,
	contents_current_90 = 0x80000,
	contents_current_180 = 0x100000,
	contents_current_270 = 0x200000,
	contents_current_up = 0x400000,
	contents_current_down = 0x800000,
	contents_origin = 0x1000000,
	contents_monster = 0x2000000,
	contents_debris = 0x4000000,
	contents_detail = 0x8000000,
	contents_translucent = 0x10000000,
	contents_ladder = 0x20000000,
	contents_hitbox = 0x40000000
};

enum surf_t : uint32_t {
	surf_light = 0x0001,
	surf_sky2d = 0x0002,
	surf_sky = 0x0004,
	surf_warp = 0x0008,
	surf_trans = 0x0010,
	surf_noportal = 0x0020,
	surf_trigger = 0x0040,
	surf_nodraw = 0x0080,
	surf_hint = 0x0100,
	surf_skip = 0x0200,
	surf_nolight = 0x0400,
	surf_bumplight = 0x0800,
	surf_noshadows = 0x1000,
	surf_nodecals = 0x2000,
	surf_nopaint = surf_nodecals,
	surf_nochop = 0x4000,
	surf_hitbox = 0x8000
};

enum mask_t : uint32_t {
	mask_all = ( 0xffffffff ),
	mask_solid = ( contents_solid | contents_moveable | contents_window | contents_monster | contents_grate ),
	mask_playersolid = ( contents_solid | contents_moveable | contents_playerclip | contents_window | contents_monster | contents_grate ),
	mask_npcsolid = ( contents_solid | contents_moveable | contents_monsterclip | contents_window | contents_monster | contents_grate ),
	mask_npcfluid = ( contents_solid | contents_moveable | contents_monsterclip | contents_window | contents_monster ),
	mask_water = ( contents_water | contents_moveable | contents_slime ),
	mask_opaque = ( contents_solid | contents_moveable | contents_opaque ),
	mask_opaque_and_npcs = ( mask_opaque | contents_monster ),
	mask_blocklos = ( contents_solid | contents_moveable | contents_blocklos ),
	mask_blocklos_and_npcs = ( mask_blocklos | contents_monster ),
	mask_visible = ( mask_opaque | contents_ignore_nodraw_opaque ),
	mask_visible_and_npcs = ( mask_opaque_and_npcs | contents_ignore_nodraw_opaque ),
	mask_shot = ( contents_solid | contents_moveable | contents_monster | contents_window | contents_debris | contents_hitbox ),
	mask_shot_brushonly = ( contents_solid | contents_moveable | contents_window | contents_debris ),
	mask_shot_hull = ( contents_solid | contents_moveable | contents_monster | contents_window | contents_debris | contents_grate ),
	mask_shot_portal = ( contents_solid | contents_moveable | contents_window | contents_monster ),
	mask_solid_brushonly = ( contents_solid | contents_moveable | contents_window | contents_grate ),
	mask_playersolid_brushonly = ( contents_solid | contents_moveable | contents_window | contents_playerclip | contents_grate ),
	mask_npcsolid_brushonly = ( contents_solid | contents_moveable | contents_window | contents_monsterclip | contents_grate ),
	mask_npcworldstatic = ( contents_solid | contents_window | contents_monsterclip | contents_grate ),
	mask_npcworldstatic_fluid = ( contents_solid | contents_window | contents_monsterclip ),
	mask_splitareaportal = ( contents_water | contents_slime ),
	mask_current = ( contents_current_0 | contents_current_90 | contents_current_180 | contents_current_270 | contents_current_up | contents_current_down ),
	mask_deadsolid = ( contents_solid | contents_playerclip | contents_window | contents_grate )
};

enum hitgroup_t : int {
	hitgroup_generic = 0,
	hitgroup_head = 1,
	hitgroup_chest = 2,
	hitgroup_stomach = 3,
	hitgroup_leftarm = 4,
	hitgroup_rightarm = 5,
	hitgroup_leftleg = 6,
	hitgroup_rightleg = 7,
	hitgroup_gear = 10
};

enum dispsurf_t {
	dispsurf_flag_surface = ( 1 << 0 ),
	dispsurf_flag_walkable = ( 1 << 1 ),
	dispsurf_flag_buildable = ( 1 << 2 ),
	dispsurf_flag_surfprop1 = ( 1 << 3 ),
	dispsurf_flag_surfprop2 = ( 1 << 4 )
};

enum tracetype_t
{
	trace_everything = 0,
	trace_world_only,
	trace_entities_only,
	trace_everything_filter_props
};

struct plane_t {
	vec3_t m_normal;
	float m_dist;
	uint8_t m_type;
	uint8_t m_sign_bits;
	PAD( 2 );
};

struct surface_t {
	const char* m_name;
	short m_surface_props;
	uint8_t m_flags;
};

class base_trace_t {
public:
	bool is_disp_surface( void ) { return ( ( m_disp_flags & dispsurf_flag_surface ) != 0 ); }
	bool is_disp_surface_walkable( void ) { return ( ( m_disp_flags & dispsurf_flag_walkable ) != 0 ); }
	bool is_disp_surface_buildable( void ) { return ( ( m_disp_flags & dispsurf_flag_buildable ) != 0 ); }
	bool is_disp_surface_prop1( void ) { return ( ( m_disp_flags & dispsurf_flag_surfprop1 ) != 0 ); }
	bool is_disp_surface_prop2( void ) { return ( ( m_disp_flags & dispsurf_flag_surfprop2 ) != 0 ); }

public:
	vec3_t m_startpos;
	vec3_t m_endpos;
	plane_t m_plane;

	float m_fraction;

	uint32_t m_contents;
	uint16_t m_disp_flags;

	bool m_allsolid;
	bool m_startsolid;

	base_trace_t( ) {}
};

class trace_t : public base_trace_t {
public:
	bool did_hit_world( ) const;

	bool did_hit_non_world_ent( ) const;

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
	uint16_t m_world_surface_index;
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
	virtual bool should_hit_ent( entity_t* ent, uint32_t mask ) = 0;
	virtual uint32_t get_trace_type( ) const = 0;
};

class _trace_filter_t : public __trace_filter_t {
public:
	void* m_skip;

	bool should_hit_ent( entity_t* ent, uint32_t ) override {
		return ent != m_skip;
	}

	uint32_t get_trace_type( ) const override {
		return 0;
	}
};

class trace_filter_t : public _trace_filter_t {
public:
	void* m_skip;

	trace_filter_t( ) {
		m_skip = nullptr;
	}

	trace_filter_t( void* ent ) {
		m_skip = ent;
	}

	bool should_hit_ent( entity_t* ent, uint32_t ) override {
		return !( ent == m_skip );
	}

	uint32_t get_trace_type( ) const override {
		return 0;
	}
};

class trace_filter_skip_two_entities_t : public _trace_filter_t {
public:
	void* m_skip1;
	void* m_skip2;

	trace_filter_skip_two_entities_t ( ) {
		m_skip2 = m_skip1 = nullptr;
	}

	trace_filter_skip_two_entities_t ( void* ent1, void* ent2 ) {
		m_skip1 = ent1;
		m_skip2 = ent2;
	}

	bool should_hit_ent ( entity_t* ent, uint32_t ) override {
		return !( ent == m_skip1 || ent == m_skip2 );
	}

	uint32_t get_trace_type ( ) const override {
		return 0;
	}
};

class trace_filter_world_and_props_only_t : public _trace_filter_t {
public:
	void* m_skip;

	trace_filter_world_and_props_only_t ( ) {
		m_skip = nullptr;
	}

	trace_filter_world_and_props_only_t ( void* ent ) {
		m_skip = ent;
	}

	bool should_hit_ent ( entity_t* ent, uint32_t ) override {
		return false;
	}

	uint32_t get_trace_type ( ) const override {
		return 0;
	}
};

struct ray_t {
	vec_aligned_t start;
	vec_aligned_t delta;
	vec_aligned_t start_offset;
	vec_aligned_t extents;
	const matrix3x4_t* world_axis_transform;
	bool is_ray;
	bool is_swept;

	ray_t( ) : world_axis_transform( nullptr ) {}

	void init( vec3_t const& src, vec3_t const& end ) {
		delta = end - src;
		is_swept = delta.length_sqr( ) != 0.f;
		extents.init( );
		world_axis_transform = nullptr;
		is_ray = true;
		start_offset.init( );
		start = src;
	}

	void init( vec3_t const& m_start, vec3_t const& end, vec3_t const& mins, vec3_t const& maxs ) {
		delta = end - m_start;
		world_axis_transform = nullptr;
		is_swept = delta.length_sqr( ) != 0.0f;

		extents = maxs - mins;
		extents *= 0.5f;
		is_ray = extents.length_sqr( ) < 1e-6f;

		start_offset = maxs + mins;
		start_offset *= 0.5f;
		start = m_start + start_offset;
		start_offset *= -1.0f;
	}

	vec3_t inv_delta( ) const
	{
		vec3_t ret;
		for ( int m_axis = 0; m_axis < 3; ++m_axis ) {
			if ( delta[ m_axis ] != 0.0f ) {
				ret[ m_axis ] = 1.0f / delta[ m_axis ];
			}
			else {
				ret[ m_axis ] = FLT_MAX;
			}
		}
		return ret;
	}
};

class c_engine_trace {
public:
	virtual int get_point_contents( const vec3_t& pos, int mask = mask_all, entity_t** ent = nullptr ) = 0; // 0
	virtual int get_point_contents_world_only( const vec3_t& pos, int mask = mask_all ) = 0; // 1
	virtual int get_point_contents_collideable( void* collide /* collidable_t */, const vec3_t& pos ) = 0; // 2
	virtual void clip_ray_to_entity( const ray_t& ray, unsigned int mask, entity_t* ent, trace_t* trace ) = 0; // 3
	virtual void clip_ray_to_collideable( const ray_t& ray, unsigned int mask, void* collide /* collidable_t */, trace_t* trace ) = 0; // 4
	virtual void trace_ray( const ray_t& ray, unsigned int mask, __trace_filter_t* filter, trace_t* trace ) = 0; // 5
};

