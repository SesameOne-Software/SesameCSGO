#pragma once
#include <sdk.hpp>
#include "../hooks/setup_bones.hpp"

namespace animations {
	namespace data {
		namespace simulated {
			extern std::array< float, 65 > animtimes;
			extern std::array< int, 65 > flags;
			extern std::array< vec3_t, 65 > origins;
			extern std::array< vec3_t, 65 > velocities;
			extern std::array< animstate_t, 65 > animstates;
			extern std::array< matrix3x4_t [ 128 ], 65 > bones;
			extern std::array< float [ 24 ], 65 > poses;
			extern std::array< animlayer_t [ 15 ], 65 > overlays;
		}

		extern std::array< animstate_t, 65 > fake_states;
		extern std::array< vec3_t, 65 > origin;
		extern std::array< vec3_t, 65 > old_origin;
		extern std::array< std::array< matrix3x4_t, 128 >, 65 > bones;
		extern std::array< std::array< matrix3x4_t, 128 >, 65 > fixed_bones;
		extern std::array< std::array< matrix3x4_t, 128 >, 65 > fixed_bones1;
		extern std::array< std::array< matrix3x4_t, 128 >, 65 > fixed_bones2;
		extern std::array< std::array< float, 24 >, 65 > poses;
		extern std::array< int, 65 > last_animation_frame;
		extern std::array< int, 65 > old_tick;
		extern std::array< int, 65 > choke;
		extern std::array< float, 65 > old_simtime;
		extern std::array< float, 65 > old_eye_yaw;
		extern std::array< float, 65 > old_abs;
		extern std::array< float, 65 > body_yaw;
		extern std::array< std::array< animlayer_t, 15 >, 65 > overlays;
	}

	namespace fake {
		extern animstate_t fake_state;
		std::array< matrix3x4_t, 128 >& matrix( );
		int simulate( );
	}

	__forceinline bool setup_bones ( player_t* target, matrix3x4_t* mat, int mask, vec3_t rotation, vec3_t origin, float time ) {
		const auto backup_mask_1 = *reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0x269C ) );
		const auto backup_mask_2 = *reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0x26B0 ) );
		const auto backup_flags = *reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0xe8 ) );
		const auto backup_effects = *reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0xf0 ) );
		const auto backup_use_pred_time = *reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0x2ee ) );
		auto backup_abs_origin = target->abs_origin ( );
		auto backup_abs_angle = target->abs_angles ( );

		*reinterpret_cast< int* >( uintptr_t ( target ) + 0xA68 ) = 0;

		*reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0x26AC ) ) = 0;
		*reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0xe8 ) ) |= N ( 8 );

		/* disable matrix interpolation */
		*reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0xf0 ) ) |= N ( 8 );

		/* use our setup time */
		//*reinterpret_cast< bool* >( uintptr_t ( target ) + N ( 0x2ee ) ) = true;

		/* use uninterpolated origin */
		if ( target != g::local ) {
			//target->set_abs_angles ( rotation );
			target->set_abs_origin ( origin );
		}

		target->inval_bone_cache ( );

		const auto backup_curtime = csgo::i::globals->m_curtime;
		const auto backup_frametime = csgo::i::globals->m_frametime;
		const auto backup_framecount = csgo::i::globals->m_framecount;

		csgo::i::globals->m_framecount = INT_MAX;
		csgo::i::globals->m_curtime = csgo::i::globals->m_curtime;
		csgo::i::globals->m_frametime = csgo::i::globals->m_ipt;

		hooks::bone_setup::allow = true;
		const auto ret = hooks::old::setup_bones ( target->renderable ( ), nullptr, mat, 128, mask, time );
		hooks::bone_setup::allow = false;

		csgo::i::globals->m_framecount = backup_framecount;
		csgo::i::globals->m_curtime = backup_curtime;
		csgo::i::globals->m_frametime = backup_frametime;

		*reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0xe8 ) ) = backup_flags;
		*reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0xf0 ) ) = backup_effects;

		if ( target != g::local ) {
			//target->set_abs_angles ( backup_abs_angle );
			target->set_abs_origin ( backup_abs_origin );
		}

		return ret;
	}

	void estimate_vel ( player_t* pl, vec3_t& out );
	int store_local ( );
	int restore_local( bool render = false );
	int fix_pl( player_t* pl );
	int run( int stage );
}