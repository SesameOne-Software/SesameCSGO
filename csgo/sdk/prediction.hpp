#pragma once
#include "../utils/vfunc.hpp"
#include "vec3.hpp"
#include "ucmd.hpp"

class c_move_helper {
public:
	virtual void pad00( ) = 0;
	virtual void set_host( void* host ) = 0;
	virtual void pad01( ) = 0;
	virtual void pad02( ) = 0;
	virtual void process_impacts( ) = 0;
};

class player_t;

class c_prediction {
public:
	void get_local_viewangles( vec3_t& angle ) {
		using fn = void( __thiscall* )( void*, vec3_t& );
		vfunc< fn >( this, 12 )( this, angle );
	}

	void set_local_viewangles( vec3_t& angle ) {
		using fn = void( __thiscall* )( void*, vec3_t& );
		vfunc< fn >( this, 13 )( this, angle );
	}

	void update( int start_frame, bool valid_frame, int inc_ack, int out_cmd ) {
		using fn = void( __thiscall* )( void*, int, bool, int, int );
		vfunc< fn >( this, 3 )( this, start_frame, valid_frame, inc_ack, out_cmd );
	}

	void check_moving_ground( player_t* player, double frametime ) {
		using fn = void( __thiscall* )( void*, player_t*, double );
		vfunc< fn >( this, 18 )( this, player, frametime );
	}

	void setup_move( player_t* player, ucmd_t* ucmd, c_move_helper* helper, void* movedata ) {
		using fn = void( __thiscall* )( void*, player_t*, ucmd_t*, c_move_helper*, void* );
		vfunc< fn >( this, 20 )( this, player, ucmd, helper, movedata );
	}

	void finish_move( player_t* player, ucmd_t* ucmd, void* movedata ) {
		using fn = void( __thiscall* )( void*, player_t*, ucmd_t*, void* );
		vfunc< fn >( this, 21 )( this, player, ucmd, movedata );
	}

	PAD( 8 );
	bool m_in_prediction;
	PAD( 1 );
	bool m_engine_paused;
	PAD( 1 );
	int m_previous_start_frame;
	PAD( 8 );
	bool m_is_first_time_predicted;
	PAD ( 3 );
	int m_commands_predicted;
};

class c_movement {
public:
public:
	virtual ~c_movement( void ) = 0;
	virtual void process_movement( void* player, void* move ) = 0;
	virtual void reset( void ) = 0;
	virtual void start_track_prediction_errors( void* player ) = 0;
	virtual void finish_track_prediction_errors( void* player ) = 0;
	virtual void diff_print( char const* fmt, ... ) = 0;
	virtual vec3_t const& get_player_mins( bool ducked ) const = 0;
	virtual vec3_t const& get_player_maxs( bool ducked ) const = 0;
	virtual vec3_t const& get_player_view_offset( bool ducked ) const = 0;
	virtual bool is_moving_player_stuck( void ) const = 0;
	virtual void* get_moving_player( void ) const = 0;
	virtual void unblock_pusher( void* player, void* pusher ) = 0;
	virtual void setup_movement_bounds( void* move ) = 0;
};

typedef enum {
	field_void = 0,			// no type or value
	field_float,			// any floating point value
	field_string,			// a string id (return from alloc_string)
	field_vector,			// any vector, qangle, or angularimpulse
	field_quaternion,		// a quaternion
	field_integer,			// any integer or enum
	field_boolean,			// boolean, implemented as an int, i may use this as a hint for compression
	field_short,			// 2 byte integer
	field_character,		// a byte
	field_color32,			// 8-bit per channel r,g,b,a (32bit color)
	field_embedded,			// an embedded object with a datadesc, recursively traverse and embedded class/structure based on an additional typedescription
	field_custom,			// special type that contains function pointers to it's read/write/parse functions

	field_classptr,			// cbaseentity *
	field_ehandle,			// entity handle
	field_edict,			// edict_t *

	field_position_vector,	// a world coordinate (these are fixed up across level transitions automagically)
	field_time,				// a floating point time (these are fixed up automatically too!)
	field_tick,				// an integer tick count( fixed up similarly to time)
	field_modelname,		// engine string that is a model name (needs precache)
	field_soundname,		// engine string that is a sound name (needs precache)

	field_input,			// a list of inputed data fields (all derived from cmultiinputvar)
	field_function,			// a class function pointer (think, use, etc)

	field_vmatrix,			// a vmatrix (output coords are not worldspace)

	// note: use float arrays for local transformations that don't need to be fixed up.
	field_vmatrix_worldspace,// a vmatrix that maps some local space to world space (translation is fixed up on level transitions)
	field_matrix3x4_worldspace,	// matrix3x4_t that maps some local space to world space (translation is fixed up on level transitions)

	field_interval,			// a start and range floating point interval ( e.g., 3.2->3.6 == 3.2 and 0.4 )
	field_modelindex,		// a model index
	field_materialindex,	// a material index (using the material precache string table)

	field_vector2d,			// 2 floats
	field_integer64,		// 64bit integer

	field_vector4d,			// 4 floats

	field_typecount,		// must be last
} fieldtype_t;

enum {
	td_offset_normal = 0,
	td_offset_packed = 1,

	td_offset_count,
};

struct typedescription_t {
	fieldtype_t field_type;
	const char* field_name;
	int field_offset;
	uint16_t field_size;
	short flags;
	const char* external_name;
	void* save_restore_ops;
	void* input_fn;
	struct datamap_t* td;
	int field_size_in_bytes;
	typedescription_t* override_field;
	int override_count;
	float field_tolerance;
	int flat_offset [ td_offset_count ];
	uint16_t flat_group;
};

struct datamap_t {
	typedescription_t* desc;
	int num_fields;
	char const* class_name;
	datamap_t* base_map;
	bool chains_validated;
	bool packed_offsets_computed;
	int packed_size;
};