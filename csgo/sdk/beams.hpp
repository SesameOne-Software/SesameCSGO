#pragma once
#include "../utils/vfunc.hpp"
#include "vec3.hpp"
#include "entity.hpp"
#include "trace.hpp"

struct c_beam;
struct beam_t;

struct beam_info_t {
	int m_type;
	entity_t* m_start_ent;
	int m_start_attachment;
	entity_t* m_end_ent;
	int m_end_attachment;
	vec3_t m_start;
	vec3_t m_end;
	int m_model_idx;
	const char* m_model_name;
	int m_halo_idx;
	const char* m_halo_name;
	float m_halo_scale;
	float m_life;
	float m_width;
	float m_end_width;
	float m_fade_len;
	float m_amplitude;
	float m_brightness;
	float m_speed;
	int m_start_frame;
	float m_frame_rate;
	float m_red;
	float m_green;
	float m_blue;
	bool m_renderable;
	int m_segments;
	int m_flags;
	vec3_t m_center;
	float m_start_radius;
	float m_end_radius;

	beam_info_t ( ) {
		m_type = 0;
		m_segments = -1;
		m_model_name = nullptr;
		m_halo_name = nullptr;
		m_model_idx = -1;
		m_halo_idx = -1;
		m_renderable = true;
		m_flags = 0;
	}
};

class c_view_render_beams {
public:
	c_view_render_beams ( void );
	virtual ~c_view_render_beams ( void ) = 0;

	virtual	void init_beams ( void ) = 0;
	virtual	void shutdown_beam ( void ) = 0;
	virtual	void clear_beams ( void ) = 0;

	virtual void update_temp_entity_beams ( ) = 0;

	virtual void draw_beam ( beam_t* pbeam ) = 0;
	virtual void draw_beam ( c_beam* pbeam, trace_filter_t* pentitybeamtracefilter = nullptr ) = 0;

	virtual	void kill_dead_beams ( entity_t* pdeadentity ) = 0;

	virtual	void create_beam_ents ( int startent, int endent, int modelindex, int haloindex, float haloscale,
		float life, float width, float endwidth, float fadelength, float amplitude,
		float brightness, float speed, int startframe,
		float framerate, float r, float g, float b, int type = -1 ) = 0;
	virtual beam_t* create_beam_ents ( beam_info_t& beaminfo ) = 0;

	virtual	void create_beam_ent_point ( int	nstartentity,  vec3_t* pstart, int nendentity,  vec3_t* pend,
		int modelindex, int haloindex, float haloscale,
		float life, float width, float endwidth, float fadelength, float amplitude,
		float brightness, float speed, int startframe,
		float framerate, float r, float g, float b ) = 0;
	virtual beam_t* create_beam_ent_point ( beam_info_t& beaminfo ) = 0;

	virtual	void create_beam_points ( vec3_t& start, vec3_t& end, int modelindex, int haloindex, float haloscale,
		float life, float width, float endwidth, float fadelength, float amplitude,
		float brightness, float speed, int startframe,
		float framerate, float r, float g, float b ) = 0;
	virtual	beam_t* create_beam_points ( beam_info_t& beaminfo ) = 0;
};