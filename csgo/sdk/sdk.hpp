#pragma once
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <functional>
#include "../utils/utils.hpp"
#include "../globals.hpp"

// sdk classes
#include "engine.hpp"
#include "surface.hpp"
#include "client.hpp"
#include "netvar.hpp"
#include "entlist.hpp"
#include "mat_system.hpp"
#include "mdl_info.hpp"
#include "mdl_render.hpp"
#include "render_view.hpp"
#include "ucmd.hpp"
#include "player.hpp"
#include "globals.hpp"
#include "trace.hpp"
#include "physics.hpp"
#include "mem_alloc.hpp"
#include "mdl_cache.hpp"
#include "prediction.hpp"
#include "input.hpp"
#include "events.hpp"
#include "effects.hpp"
#include "beams.hpp"

namespace csgo {
	constexpr auto pi = 3.14159265358979f;

	namespace i {
		extern c_surface* surface;
		extern c_panel* panel;
		extern c_engine* engine;
		extern c_client* client;
		extern c_entlist* ent_list;
		extern c_matsys* mat_sys;
		extern c_mdlinfo* mdl_info;
		extern c_mdlrender* mdl_render;
		extern c_renderview* render_view;
		extern c_globals* globals;
		extern c_phys* phys;
		extern c_engine_trace* trace;
		extern c_clientstate* client_state;
		extern c_mem_alloc* mem_alloc;
		extern c_prediction* pred;
		extern c_move_helper* move_helper;
		extern c_movement* move;
		extern mdl_cache_t* mdl_cache;
		extern c_input* input;
		extern void* cvar;
		extern c_game_event_mgr* events;
		extern c_view_render_beams* beams;
		extern IDirect3DDevice9* dev;
	}

	namespace render {
		bool screen_transform( vec3_t& screen, vec3_t& origin );
		bool world_to_screen( vec3_t& screen, vec3_t& origin );
	}

	namespace util {
		void trace_line( const vec3_t& start, const vec3_t& end, std::uint32_t mask, const entity_t* ignore, trace_t* ptr );
		void clip_trace_to_players( const vec3_t& start, const vec3_t& end, std::uint32_t mask, trace_filter_t* filter, trace_t* trace_ptr );
	}

	constexpr int time2ticks( float t ) {
		return static_cast< int >( t / i::globals->m_ipt + 0.5f );
	}

	constexpr float ticks2time( int t ) {
		return static_cast< float >( t )* i::globals->m_ipt;
	}

	__forceinline void for_each_player ( const std::function< void ( player_t* ) >& fn ) {
		for ( auto i = 1; i <= i::globals->m_max_clients; i++ ) {
			auto entity = i::ent_list->get< player_t* > ( i );

			if ( !entity->valid ( ) )
				continue;

			fn ( entity );
		}
	}

	__forceinline float normalize ( float ang ) {
		while ( ang < -180.0f ) ang += 360.0f;
		while ( ang > 180.0f ) ang -= 360.0f;
		return ang;
	}

	__forceinline void clamp ( vec3_t& ang ) {
		auto flt_valid = [ ] ( float val ) {
			return std::isfinite ( val ) && !std::isnan ( val );
		};

		for ( auto i = 0; i < 3; i++ )
			if ( !flt_valid ( ang [ i ] ) )
				return;

		ang.x = std::clamp ( normalize ( ang.x ), -89.0f, 89.0f );
		ang.y = std::clamp ( normalize ( ang.y ), -180.0f, 180.0f );
		ang.z = 0.0f;
	}

	__forceinline float rad2deg ( float rad ) {
		float result = rad * ( 180.0f / pi );
		return result;
	}

	__forceinline float deg2rad ( float deg ) {
		float result = deg * ( pi / 180.0f );
		return result;
	}

	__forceinline void sin_cos ( float radians, float* sine, float* cosine ) {
		*sine = std::sinf ( radians );
		*cosine = std::cosf ( radians );
	}

	__forceinline vec3_t vec_angle ( vec3_t vec ) {
		vec3_t ret;

		if ( vec.x == 0.0f && vec.y == 0.0f ) {
			ret.x = ( vec.z > 0.0f ) ? 270.0f : 90.0f;
			ret.y = 0.0f;
		}
		else {
			ret.x = rad2deg ( std::atan2f ( -vec.z, vec.length_2d ( ) ) );
			ret.y = rad2deg ( std::atan2f ( vec.y, vec.x ) );

			if ( ret.y < 0.0f )
				ret.y += 360.0f;

			if ( ret.x < 0.0f )
				ret.x += 360.0f;
		}

		ret.z = 0.0f;

		return ret;
	}

	__forceinline vec3_t calc_angle ( const vec3_t& from, const vec3_t& to ) {
		return vec_angle ( to - from );
	}

	__forceinline vec3_t angle_vec ( vec3_t angle ) {
		vec3_t ret;

		float sp, sy, cp, cy;

		sin_cos ( deg2rad ( angle.y ), &sy, &cy );
		sin_cos ( deg2rad ( angle.x ), &sp, &cp );

		ret.x = cp * cy;
		ret.y = cp * sy;
		ret.z = -sp;

		return ret;
	}

	__forceinline float calc_fov ( const vec3_t& src, const vec3_t& dst ) {
		vec3_t ang = angle_vec ( src ), aim = angle_vec ( dst );
		return rad2deg ( acosf ( aim.dot_product ( ang ) / aim.length_sqr ( ) ) );
	}

	__forceinline void util_traceline ( const vec3_t& start, const vec3_t& end, unsigned int mask, const void* ignore, trace_t* tr ) {
		using fn = void ( __fastcall* )( const vec3_t&, const vec3_t&, std::uint32_t, const void*, std::uint32_t, trace_t* );
		static auto utl = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F0 83 EC 7C 56 52" ) ).get< fn > ( );
		utl ( start, end, mask, ignore, 0, tr );
	}

	__forceinline void util_tracehull ( const vec3_t& start, const vec3_t& end, const vec3_t& mins, const vec3_t& maxs, unsigned int mask, const void* ignore, trace_t* tr ) {
		using fn = void ( __fastcall* )( const vec3_t&, const vec3_t&, const vec3_t&, const vec3_t&, unsigned int, const void*, std::uint32_t, trace_t* );
		static auto utl = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 8B 07 83 C4 20" ) ).resolve_rip ( ).get< fn > ( );
		utl ( start, end, mins, maxs, mask, ignore, 0, tr );
	}

	__forceinline bool is_visible ( const vec3_t& point ) {
		if ( !g::local->valid ( ) )
			return false;

		trace_t tr;
		util_traceline ( g::local->eyes ( ), point, 0x46004003, g::local, &tr );

		return tr.m_fraction > 0.97f || ( reinterpret_cast< player_t* >( tr.m_hit_entity )->valid ( ) && reinterpret_cast< player_t* >( tr.m_hit_entity )->team ( ) != g::local->team ( ) );
	}

	__forceinline void rotate_movement ( ucmd_t* ucmd, float old_smove, float old_fmove, const vec3_t& old_angs ) {
		auto dv = 0.0f;
		auto f1 = 0.0f;
		auto f2 = 0.0f;

		if ( old_angs.y < 0.f )
			f1 = 360.0f + old_angs.y;
		else
			f1 = old_angs.y;

		if ( ucmd->m_angs.y < 0.0f )
			f2 = 360.0f + ucmd->m_angs.y;
		else
			f2 = ucmd->m_angs.y;

		if ( f2 < f1 )
			dv = abs ( f2 - f1 );
		else
			dv = 360.0f - abs ( f1 - f2 );

		dv = 360.0f - dv;

		ucmd->m_fmove = std::cosf ( csgo::deg2rad ( dv ) ) * old_fmove + std::cosf ( csgo::deg2rad ( dv + 90.0f ) ) * old_smove;
		ucmd->m_smove = std::sinf ( csgo::deg2rad ( dv ) ) * old_fmove + std::sinf ( csgo::deg2rad ( dv + 90.0f ) ) * old_smove;
	}

	__forceinline void angle_matrix ( const vec3_t& angles, matrix3x4_t& matrix ) {
		float sr, sp, sy, cr, cp, cy;

		sin_cos ( deg2rad ( angles.y ), &sy, &cy );
		sin_cos ( deg2rad ( angles.x ), &sp, &cp );
		sin_cos ( deg2rad ( angles.z ), &sr, &cr );

		// matrix = (YAW * PITCH) * ROLL
		matrix [ 0 ][ 0 ] = cp * cy;
		matrix [ 1 ][ 0 ] = cp * sy;
		matrix [ 2 ][ 0 ] = -sp;

		float crcy = cr * cy;
		float crsy = cr * sy;
		float srcy = sr * cy;
		float srsy = sr * sy;

		matrix [ 0 ][ 1 ] = sp * srcy - crsy;
		matrix [ 1 ][ 1 ] = sp * srsy + crcy;
		matrix [ 2 ][ 1 ] = sr * cp;

		matrix [ 0 ][ 2 ] = ( sp * crcy + srsy );
		matrix [ 1 ][ 2 ] = ( sp * crsy - srcy );
		matrix [ 2 ][ 2 ] = cr * cp;

		matrix [ 0 ][ 3 ] = 0.0f;
		matrix [ 1 ][ 3 ] = 0.0f;
		matrix [ 2 ][ 3 ] = 0.0f;
	}

	__forceinline void angle_matrix ( const vec3_t& angles, const vec3_t& position, matrix3x4_t& matrix ) {
		angle_matrix ( angles, matrix );
		matrix.set_origin ( position );
	}

	__forceinline bool is_valve_server ( ) {
		static auto cs_game_rules = pattern::search ( _ ( "client.dll" ), _ ( "A1 ? ? ? ? 74 38" ) ).add ( 1 ).deref ( ).get< void* > ( );
		return *reinterpret_cast< uintptr_t* > ( cs_game_rules ) && *reinterpret_cast< bool* > ( *reinterpret_cast< uintptr_t* > ( cs_game_rules ) + 0x75 );
	}

#define DOT_PROD( a, b ) ( a.x * b.x + a.y * b.y + a.z * b.z )

#define VEC_TRANSFORM( in1, in2, out )																	\
out.x = DOT_PROD( in1, vec3_t( in2 [ 0 ] [ 0 ], in2 [ 0 ] [ 1 ], in2 [ 0 ] [ 2 ] ) ) + in2 [ 0 ] [ 3 ];	\
out.y = DOT_PROD( in1, vec3_t( in2 [ 1 ] [ 0 ], in2 [ 1 ] [ 1 ], in2 [ 1 ] [ 2 ] ) ) + in2 [ 1 ] [ 3 ];	\
out.z = DOT_PROD( in1, vec3_t( in2 [ 2 ] [ 0 ], in2 [ 2 ] [ 1 ], in2 [ 2 ] [ 2 ] ) ) + in2 [ 2 ] [ 3 ];

	template < typename t >
	t create_interface( const char* module, const char* iname );

	bool init( );
}