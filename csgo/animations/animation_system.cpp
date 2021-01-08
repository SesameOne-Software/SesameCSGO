#include "animation_system.hpp"
#include "resolver.hpp"

#include "../features/lagcomp.hpp"
#include "../features/ragebot.hpp"

#undef max
#undef min

float anims::angle_diff ( float dst, float src ) {
	auto delta = fmodf ( dst - src, 360.0f );

	if ( dst > src ) {
		if ( delta >= 180.0f )
			delta -= 360.0f;
	}
	else {
		if ( delta <= -180.0f )
			delta += 360.0f;
	}

	return delta;
}

// credits cbrs
bool anims::build_bones ( player_t* target, matrix3x4_t* mat, int mask, vec3_t rotation, vec3_t origin, float time, std::array<float, 24>& poses ) {
	/* vfunc indices */
	static auto standard_blending_rules_vfunc_idx = pattern::search ( _ ( "client.dll" ), _ ( "FF 90 ? ? ? ? 8B 47 FC" ) ).add ( 2 ).deref ( ).get< uint32_t > ( ) / 4;
	static auto build_transformations_vfunc_idx = pattern::search ( _ ( "client.dll" ), _ ( "FF 90 ? ? ? ? A1 ? ? ? ? B9 ? ? ? ? 8B 40 34 FF D0 85 C0 74 41" ) ).add ( 2 ).deref ( ).get< uint32_t > ( ) / 4;
	static auto update_ik_locks_vfunc_idx = pattern::search ( _ ( "client.dll" ), _ ( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50" ) ).add ( 2 ).deref ( ).get< uint32_t > ( ) / 4;
	static auto calculate_ik_locks_vfunc_idx = pattern::search ( _ ( "client.dll" ), _ ( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50 FF B7 ? ? ? ? 8D 84 24 ? ? ? ? 50 8D 84 24 ? ? ? ? 50 E8 ? ? ? ? 8B 47 FC 8D 4F FC 8B 80" ) ).add ( 2 ).deref ( ).get< uint32_t > ( ) / 4;

	/* func sigs */
	static auto init_iks = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D" ) ).get< void ( __thiscall* )( void*, void*, vec3_t&, vec3_t&, float, int, int ) > ( );
	static auto update_targets = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F0 81 EC 18 01 00 00 33" ) ).get< void ( __thiscall* )( void*, vec3_t*, void*, void*, uint8_t* ) > ( );
	static auto solve_dependencies = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F0 81 EC 98 05 00 00 8B 81" ) ).get< void ( __thiscall* )( void*, vec3_t*, void*, void*, uint8_t* ) > ( );

	/* offset sigs */
	static auto iks_off = pattern::search ( _ ( "client.dll" ), _ ( "8D 47 FC 8B 8F" ) ).add ( 5 ).deref ( ).add ( 4 ).get< uint32_t > ( );
	static auto effects_off = pattern::search ( _ ( "client.dll" ), _ ( "75 0D 8B 87" ) ).add ( 4 ).deref ( ).add ( 4 ).get< uint32_t > ( );

	const auto backup_poses = target->poses ( );

	auto cstudio = *reinterpret_cast< void** >( uintptr_t ( target ) + 0x294C );

	if ( !cstudio )
		return false;

	target->poses ( ) = poses;

	//  we need an aligned matrix in the bone accessor, so do this :) bad performance cause memcpy but that's ok
	matrix3x4a_t used [ 128 ];

	//	output shit
	uint8_t bone_computed [ 0x100 ] = { 0 };

	//	needs to be aligned
	matrix3x4a_t base_matrix;

	cs::angle_matrix ( rotation, origin, base_matrix );

	//	store shit
	const auto old_bones = target->bones ( );

	auto iks = *reinterpret_cast< void** >( uintptr_t ( target ) + iks_off );
	*reinterpret_cast< int* >( uintptr_t ( target ) + effects_off ) |= 8;

	//	clear iks & re-create them
	if ( iks ) {
		if ( *( uint32_t* ) ( uintptr_t ( iks ) + 0xFF0 ) > 0 ) {
			int v1 = 0;
			auto v62 = ( uint32_t* ) ( uintptr_t ( iks ) + 0xD0 );

			do {
				*v62 = -9999;
				v62 += 0x55;
				++v1;
			} while ( v1 < *( uint32_t* ) ( uintptr_t ( iks ) + 0xFF0 ) );
		}

		init_iks ( iks, cstudio, rotation, origin, time, cs::i::globals->m_framecount, 0x1000 );
	}

	vec3_t pos [ 128 ];
	memset ( pos, 0, sizeof ( vec3_t [ 128 ] ) );

	void* q = malloc ( /* sizeof quaternion_t */ 48 * 128 );
	memset ( q, 0, /* sizeof quaternion_t */ 48 * 128 );

	//	set flags and bones
	target->bones ( ) = used;

	//	build some shit
	// IDX = 205 ( 2020.7.10 )
	vfunc< void ( __thiscall* )( player_t*, void*, vec3_t*, void*, float, uint32_t ) > ( target, standard_blending_rules_vfunc_idx ) ( target, cstudio, pos, q, time, mask );

	//	set iks
	if ( iks ) {
		vfunc< void ( __thiscall* )( player_t*, float ) > ( target, update_ik_locks_vfunc_idx ) ( target, time );
		update_targets ( iks, pos, q, target->bones ( ), bone_computed );
		vfunc< void ( __thiscall* )( player_t*, float ) > ( target, calculate_ik_locks_vfunc_idx ) ( target, time );
		solve_dependencies ( iks, pos, q, target->bones ( ), bone_computed );
	}

	//	build the matrix
	// IDX = 189 ( 2020.7.10 )
	vfunc< void ( __thiscall* )( player_t*, void*, vec3_t*, void*, matrix3x4a_t const&, uint32_t, void* ) > ( target, build_transformations_vfunc_idx ) ( target, cstudio, pos, q, base_matrix, mask, bone_computed );

	free ( q );

	//	restore flags and bones
	*reinterpret_cast< int* >( uintptr_t ( target ) + effects_off ) &= ~8;
	target->bones ( ) = old_bones;

	//  and pop out our new matrix
	memcpy ( mat, used, sizeof ( matrix3x4_t ) * 128 );

	target->poses ( ) = backup_poses;

	return true;
}

void anims::calc_poses ( player_t* ent, std::array<float, 24>& poses, float feet_yaw ) {
	/* copy existing poses*/
	poses = ent->poses ( );

	auto jump_fall = [ ] ( float air_time ) {
		const float recalc_air_time = ( air_time - 0.72f ) * 1.25f;
		const float clamped = recalc_air_time >= 0.0f ? std::min< float > ( recalc_air_time, 1.0f ) : 0.0f;
		float out = ( 3.0f - ( clamped + clamped ) ) * ( clamped * clamped );

		if ( out >= 0.0f )
			out = std::min< float > ( out, 1.0f );

		return out;
	};

	/* calculate *some* new ones */
	poses [ 11 /* body_yaw */ ] = std::clamp ( cs::normalize ( angle_diff ( cs::normalize ( ent->animstate()->m_eye_yaw ), cs::normalize ( feet_yaw ) ) ), -60.0f, 60.0f ) / 120.0f + 0.5f;

	auto move_yaw = cs::normalize ( angle_diff ( cs::normalize ( cs::vec_angle ( ent->vel ( ) ).y ), cs::normalize ( feet_yaw ) ) );
	
	if ( move_yaw < 0.0f )
		move_yaw += 360.0f;

	poses [ 7 /* move_yaw */ ] = move_yaw / 360.0f;
}

void anims::predict_movement ( player_t* ent, flags_t& old_flags, vec3_t& abs_origin, vec3_t& abs_vel, vec3_t& abs_accel ) {
	/* predict velocity (only on x and y axes) */
	abs_vel.x += abs_accel.x * cs::ticks2time ( 1 );
	abs_vel.y += abs_accel.y * cs::ticks2time ( 1 );

	/* predict gravity's effect on velocity */
	if(!( ent->flags() & flags_t::on_ground))
		abs_vel.z -= g::cvars::sv_gravity->get_float() * cs::ticks2time ( 1 );
	//else if ( !( old_flags & flags_t::on_ground ) )
	//	abs_vel.z = g::cvars::sv_jump_impulse->get_float ( );
	
	/* ideal position */
	const auto src = abs_origin;
	const auto dst = abs_origin + abs_vel * cs::ticks2time ( 1 );

	trace_t tr {};
	cs::util_tracehull ( abs_origin + vec3_t ( 0.0f, 0.0f, 2.0f ) /* add little offset so we dont fail on uneven ground*/, dst, ent->mins(), ent->maxs(), mask_playersolid, ent, &tr );

	/* predict position */
	abs_origin = (tr.m_fraction == 1.0f) ? dst : tr.m_endpos;

	tr = trace_t {};
	cs::util_tracehull ( abs_origin, abs_origin - vec3_t ( 0.0f, 0.0f, 2.0f ) /* add little offset so we dont fail on uneven ground*/, ent->mins ( ), ent->maxs ( ), mask_playersolid, ent, &tr );
	
	/* predict flags */
	if ( tr.did_hit ( ) && tr.m_plane.m_normal.z > 0.7f )
		ent->flags ( ) |= flags_t::on_ground;
	else
		ent->flags ( ) &= ~flags_t::on_ground;

	/* calculate new velocity */
	abs_vel = ( abs_origin - src ) / cs::ticks2time ( 1 );
}

void anims::manage_fake ( ) {
	if ( !g::local || !g::local->layers ( ) )
		return;

	static animstate_t fake_anim_state { };
	static auto handle = g::local->handle ( );
	static auto spawn_time = g::local->spawn_time ( );

	if ( !g::local->alive ( ) ) {
		spawn_time = 0.0f;
		return;
	}

	bool reset = g::local->spawn_time ( ) != spawn_time || g::local->handle ( ) != handle;
	
	if ( reset ) {
		g::local->create_animstate ( &fake_anim_state );

		handle = g::local->handle ( );
		spawn_time = g::local->spawn_time ( );
	}
	
	//if ( g::send_packet ) {
		std::array<animlayer_t, 13> networked_layers;
		memcpy ( networked_layers.data(), g::local->layers ( ), sizeof( networked_layers ) );

		auto backup_abs_angles = g::local->abs_angles ( );
		auto backup_poses = g::local->poses ( );
		auto origin = g::local->origin ( );

		local::simulating_fake = true;
		fake_anim_state.update ( g::angles );
		local::simulating_fake = false;

		g::local->abs_angles ( ).y = fake_anim_state.m_abs_yaw;

		build_bones ( g::local, local::fake_matrix.data(), 0x7FF00, vec3_t( 0.0f, fake_anim_state.m_abs_yaw, 0.0f ), origin, cs::i::globals->m_curtime, g::local->poses ( ) );

		memcpy ( g::local->layers ( ), networked_layers.data ( ), sizeof ( networked_layers ) );

		g::local->poses ( ) = backup_poses;
		g::local->abs_angles ( ) = backup_abs_angles;

		for ( auto& iter : local::fake_matrix )
			iter.set_origin ( iter.origin ( ) - origin );
	//}
}

float backup_curtime = 0.0f;
float backup_framtime = 0.0f;
bool updated_this_tick = false;
int backup_flags1 = 0;
int backup_flags2 = 0;
vec3_t backup_abs_origin;
vec3_t backup_abs_vel;
flags_t backup_flags;
std::array<animlayer_t, 13> backup_anim_layers;

void anims::pre_update_callback ( animstate_t* anim_state ) {
	const auto ent = anim_state->m_entity;
	const auto idx = ent->idx ( );
	
	bool choking = ent != g::local;

	updated_this_tick = true;
	backup_curtime = cs::i::globals->m_curtime;
	backup_framtime = cs::i::globals->m_frametime;
	backup_flags1 = *reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( ent ) + 0xE8 );
	backup_flags2 = *reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( ent ) + 0xF0 );
	backup_abs_origin = ent->abs_origin ( );
	backup_abs_vel = *reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0x94 );
	backup_flags = ent->flags ( );
	memcpy ( backup_anim_layers.data ( ), ent->layers ( ), sizeof ( backup_anim_layers ) );
	
	*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( ent ) + 0xA30 ) = cs::i::globals->m_framecount;
	*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( ent ) + 0xA28 ) = 0;
	*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( ent ) + 0xA68 ) = cs::i::globals->m_framecount;

	//if ( ent != g::local )
	//	cs::i::globals->m_curtime = ent->simtime ( );

	if ( ent != g::local && ent->simtime ( ) > players::anim_times [ idx ] ) {
		if ( players::anim_times [ idx ] )
			players::choked_commands [ idx ] = std::clamp<int> ( cs::time2ticks ( ent->simtime ( ) - players::anim_times [ idx ] ) - 1, 0, g::cvars::sv_maxusrcmdprocessticks->get_int ( ) );
		
		if( players::anim_times [ idx ] )
			players::accelerations [ idx ] = ( ent->vel ( ) - players::last_velocities [ idx ] ) / abs ( ent->simtime ( ) - players::anim_times [ idx ] );
		else
			players::accelerations [ idx ] = {};

		players::update_time [ idx ] = cs::i::globals->m_curtime;
		players::last_velocities [ idx ] = ent->vel ( );
		players::anim_times [ idx ] = ent->simtime ( );
		players::last_origins [ idx ] = ent->origin ( );
		players::last_flags [ idx ] = players::flags [ idx ];
		players::flags [ idx ] = ent->flags ( );
		players::updates_since_dormant [ idx ]++;

		memcpy ( players::anim_layers [ idx ].data(), ent->layers(), sizeof( players::anim_layers [ idx ] ) );

		choking = false;
	}

	/* set updated data */
	anim_state->m_feet_yaw_rate = 0.0f;
	
	if ( ent != g::local ) {
		/* fix angles */
		anim_state->m_pitch = ent->angles ( ).x;
		anim_state->m_eye_yaw = ent->angles ( ).y;

		/* simulate player movement if they are choking */
		if ( choking && players::updates_since_dormant [ idx ] > 1 ) {
			predict_movement ( ent, players::last_flags [ idx ], players::last_origins [ idx ], players::last_velocities [ idx ], players::accelerations [ idx ] );

			*reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0x94 ) = players::last_velocities [ idx ];
			*reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0xE8 ) &= ~0x1000;
			ent->set_abs_origin ( players::last_origins [ idx ] );
		}
		else {
			*reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0x94 ) = ent->vel ( );
			*reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0xE8 ) &= ~0x1000;
			ent->set_abs_origin ( ent->origin ( ) );
		}

		memcpy ( ent->layers ( ), players::anim_layers [ idx ].data ( ), sizeof ( players::anim_layers [ idx ] ) );
	}
}

void anims::post_update_callback ( animstate_t* anim_state ) {
	anim_state->m_last_clientside_anim_framecount = cs::i::globals->m_tickcount;
	
	const auto ent = anim_state->m_entity;
	const auto idx = ent->idx ( );

	/* display only poses and layers for local on send */
	if ( ent == g::local ) {
		if ( !local::simulating_fake ) {
			if ( g::send_packet ) {
				local::feet_yaw = anim_state->m_abs_yaw;
				local::poses = ent->poses ( );

				memcpy ( local::anim_layers.data ( ), ent->layers ( ), sizeof ( local::anim_layers ) );

				manage_fake ( );
			}

			//memcpy ( ent->layers ( ), local::anim_layers.data ( ), sizeof ( local::anim_layers ) );
			ent->poses ( ) = local::poses;
			ent->set_abs_angles ( vec3_t ( 0.0f, local::feet_yaw, 0.0f ) );

			build_bones ( ent, local::real_matrix.data ( ), 0x7FF00, vec3_t ( 0.0f, local::feet_yaw, 0.0f ), ent->abs_origin(), cs::i::globals->m_curtime, local::poses );

			if ( ent->bone_cache ( ) )
				memcpy ( ent->bone_cache ( ), local::real_matrix.data ( ), sizeof(matrix3x4_t) * ent->bone_count() );
		}

		return;
	}

	/* for other players */

	/* resolve player and recalculate poses on update */
	if ( updated_this_tick ) {
		const auto time_since_update = abs ( cs::i::globals->m_curtime - players::update_time [ idx ] );
		
		if ( ent->team ( ) != g::local->team ( ) ) {
			animations::resolver::resolve ( ent, players::resolved_feet_yaws [ idx ][ 0 ], players::resolved_feet_yaws [ idx ][ 1 ], players::resolved_feet_yaws [ idx ][ 2 ] );

			for ( auto i = 0; i < 3; i++ ) {
				calc_poses ( ent, players::poses [ idx ][ i ], players::resolved_feet_yaws [ idx ][ i ] );
				build_bones ( ent, players::matricies [ idx ][ i ].data ( ), 0x7FF00, vec3_t ( 0.0f, players::resolved_feet_yaws [ idx ][ i ], 0.0f ), ent->abs_origin ( ), ent->simtime ( ) + time_since_update, players::poses [ idx ][ i ] );
			}
		}
		else {
			players::resolved_feet_yaws [ idx ][ 0 ] = players::resolved_feet_yaws [ idx ][ 1 ] = players::resolved_feet_yaws [ idx ][ 2 ] = anim_state->m_abs_yaw;
			calc_poses ( ent, players::poses [ idx ][ 0 ], players::resolved_feet_yaws [ idx ][ 0 ] );
			build_bones ( ent, players::matricies [ idx ][ 0 ].data ( ), 0x7FF00, vec3_t ( 0.0f, players::resolved_feet_yaws [ idx ][ 0 ], 0.0f ), ent->abs_origin ( ), ent->simtime ( ) + time_since_update, players::poses [ idx ][ 0 ] );

			for ( auto i = 0; i < 3; i++ ) {
				players::resolved_feet_yaws [ idx ][ i ] = players::resolved_feet_yaws [ idx ][ 0 ];
				players::poses [ idx ][ i ] = players::poses [ idx ][ 0 ];
				players::matricies [ idx ][ i ] = players::matricies [ idx ][ 0 ];
			}
		}
		
		/* only cache animation updates */
		//if ( !cs::time2ticks ( time_since_update ) )
		/* cache lag record */
		features::lagcomp::cache ( ent, players::update_time [ idx ] && cs::time2ticks ( time_since_update ) > 0 );
		
		memcpy ( players::anim_layers [ idx ].data ( ), ent->layers ( ), sizeof ( players::anim_layers [ idx ] ) );
		memcpy ( ent->layers ( ), backup_anim_layers.data(), sizeof ( backup_anim_layers ) );

		/* restore stuff */
		*reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0x94 ) = backup_abs_vel;
		*reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0xE8 ) &= ~0x1000;
		ent->set_abs_origin ( backup_abs_origin );
		ent->flags ( ) = backup_flags;
		cs::i::globals->m_curtime = backup_curtime;
		cs::i::globals->m_frametime = backup_framtime;

		updated_this_tick = false;
	}

	/* apply resolved data depending on current resolver mode (visual resolver) */
	const auto misses = features::ragebot::get_misses ( idx ).bad_resolve % 3;

	ent->set_abs_angles ( vec3_t ( 0.0f, players::resolved_feet_yaws [ idx ][ misses ], 0.0f ) );
	ent->poses ( ) = players::poses [ idx ][ misses ];

	if ( ent->bone_cache ( ) )
		memcpy ( ent->bone_cache ( ), anims::players::matricies [ idx ][ misses ].data ( ), sizeof ( matrix3x4_t ) * ent->bone_count ( ) );
}

namespace hooks {
	void* ret_animstate_update = nullptr;
	animstate_t* anim_state = nullptr;
	animlayer_t* anim_layers = nullptr;

	void* ret_animstate_update_time = nullptr;
	__declspec(naked) void animstate_update_time ( ) {
		_asm {
			mov anim_state, edi

			pushad
			pushfd
		}

		if ( anim_state->m_last_clientside_anim_framecount != cs::i::globals->m_tickcount ) {
			if ( anim_state )
				anims::pre_update_callback ( anim_state );
			
			_asm {
				popfd
				popad
				jmp [ret_animstate_update_time]
			}
		}

		_asm {
			popfd
			popad
			jmp [ret_animstate_update]
		}
	}

	void* ret_animstate_update_LOD = nullptr;
	__declspec( naked ) void animstate_update_LOD ( ) {
		_asm jmp [ret_animstate_update_LOD]
	}

	void* ret_animstate_update_IsCrouchPreUpdate = nullptr;
	__declspec( naked ) void animstate_update_IsCrouchPreUpdate ( ) {
		_asm {
			mov eax, 0
			jmp [ret_animstate_update_IsCrouchPreUpdate]
		}
	}

	void* ret_animstate_update_animlayers_pre_Reset = nullptr;
	__declspec( naked ) void animstate_update_animlayers_pre_Reset ( ) {
		_asm {
			mov eax, dword ptr [ esi + 0x298C ]
			mov anim_layers, eax
			mov dword ptr [ esi + 0x298C ], 0

			xor eax, eax
			mov dword ptr [ esp + 0xC ], eax
			xchg ax, ax
			jmp [ret_animstate_update_animlayers_pre_Reset]
		}
	}

	void* ret_animstate_update_animlayers_post_Reset = nullptr;
	__declspec( naked ) void animstate_update_animlayers_post_Reset ( ) {
		_asm {
			mov eax, anim_layers
			mov dword ptr [ esi + 0x298C ], eax

			movss xmm0, dword ptr [ edi + 0x80 ]
			jmp [ret_animstate_update_animlayers_post_Reset]
		}
	}
	
	void* ret_animstate_update_set_framecount = nullptr;
	__declspec( naked ) void animstate_update_set_framecount ( ) {
		_asm {
			pushad
			pushfd
		}

		if ( anim_state )
			anims::post_update_callback ( anim_state );

		anim_state = nullptr;

		_asm {
			popfd
			popad

			pop edi
			pop esi
			mov esp, ebp
			pop ebp
			jmp [ret_animstate_update_set_framecount]
		}
	}

	void* ret_animstate_SetupVelocity_GetAbsVelocity = nullptr;
	__declspec( naked ) void animstate_SetupVelocity_GetAbsVelocity ( ) {
		_asm {
			pushad
			pushfd
		}

		//*reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t> ( anim_state->m_entity ) + 0x94 ) = anim_state->m_entity->vel ( );

		_asm {
			popfd
			popad
			mov esi, ecx
			mov ecx, edi
			mov dword ptr [ esp + 0x14 ], esi
			jmp [ret_animstate_SetupVelocity_GetAbsVelocity]
		}
	}

	void* ret_animstate_SetupVelocity_ClampVelocity = nullptr;
	void* ret_animstate_SetupVelocity_ClampVelocity_to = nullptr;
	__declspec( naked ) void animstate_SetupVelocity_ClampVelocity ( ) {
		_asm {
			jmp [ret_animstate_SetupVelocity_ClampVelocity_to]
		}
	}
}

void anims::init ( ) {
	auto mid_func_hook = [ ] ( void* src, void* dst, uint32_t sz ) -> void* {
		if ( sz < 5 )
			return nullptr;

		DWORD old_prot = 0;
		VirtualProtect ( src, sz, PAGE_EXECUTE_READWRITE, &old_prot );

		memset ( src, 0x90, sz );
		*reinterpret_cast< uint8_t* >( src ) = 0xE9;
		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( src ) + 1 ) = int ( reinterpret_cast< uintptr_t >( dst ) - reinterpret_cast< uintptr_t >( src ) ) - 5;

		VirtualProtect ( src, sz, old_prot, &old_prot );

		return reinterpret_cast< void* >( reinterpret_cast< uintptr_t >( src ) + sz );
	};

	hooks::ret_animstate_update = pattern::search ( _ ( "client.dll" ), _ ( "C6 05 ? ? ? ? ? 89 47 70 5F 5E 8B E5 5D C2 04 00" ) ).add ( 10 ).get<void*> ( );

	/* has to be changed on client since anims update per-frame instead of per-tick (place the first hook here!) */
	/*
	if ( !bForce && ( m_flLastUpdateTime == gpGlobals->curtime || m_nLastUpdateFrame == gpGlobals->framecount ) )
		return;
	*/
	{
		const auto src = pattern::search ( _("client.dll"), _("0F 8B ? ? ? ? 8B 47 70 3B 41 04 0F 84") ).get<void*>();
		hooks::ret_animstate_update_time = mid_func_hook ( src, hooks::animstate_update_time, 18 );
	}

	///*
	//#ifdef CLIENT_DLL
	//	// changing weapons will change the pose of leafy bones like fingers. The next time we
	//	// set up this player's bones, treat it like a clean first setup.
	//	m_pPlayer->m_nComputedLODframe = 0;
	//#endif
	//*/
	{
		const auto src = pattern::search ( _ ( "client.dll" ), _ ( "C7 80 ? ? ? ? ? ? ? ? 66 66 66 0F 1F 84 00" ) ).get<void*> ( );
		hooks::ret_animstate_update_LOD = mid_func_hook ( src, hooks::animstate_update_LOD, 10 );
	}
	//
	///*
	//#ifdef CLIENT_DLL
	//if ( IsPreCrouchUpdateDemo() )
	//{
	//	// compatibility for old demos using old crouch values
	//	float flTargetDuck = (m_pPlayer->GetFlags() & ( FL_ANIMDUCKING )) ? 1.0f : m_flDuckAdditional;
	//	m_flAnimDuckAmount = Approach( flTargetDuck, m_flAnimDuckAmount, m_flLastUpdateIncrement * ( (m_flAnimDuckAmount < flTargetDuck) ? CSGO_ANIM_DUCK_APPROACH_SPEED_DOWN : CSGO_ANIM_DUCK_APPROACH_SPEED_UP )  );
	//	m_flAnimDuckAmount = clamp( m_flAnimDuckAmount, 0, 1 );
	//}
	//else
//if
	//*/
	{
		const auto src = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 84 C0 8B 47 60" ) ).get<void*> ( );
		hooks::ret_animstate_update_IsCrouchPreUpdate = mid_func_hook ( src, hooks::animstate_update_IsCrouchPreUpdate, 5 );
	}
	//
	///*
	//// HERE
	//
	//#ifdef  CLIENT_DLL
	//// zero-sequences are un-set and should have zero weight on the client
	//for ( int i=0; i < ANIMATION_LAYER_COUNT; i++ )
	//{
	//	CAnimationLayer *pLayer = m_pPlayer->GetAnimOverlay( i, USE_ANIMLAYER_RAW_INDEX );
	//	if ( pLayer && pLayer->GetSequence() == 0 )
	//		pLayer->SetWeight(0);
	//}
//if
	//*/
	{
		const auto src = pattern::search ( _ ( "client.dll" ), _ ( "33 C0 89 44 24 0C 66 90 8B 77 60 83 BE" ) ).get<void*> ( );
		hooks::ret_animstate_update_animlayers_pre_Reset = mid_func_hook ( src, hooks::animstate_update_animlayers_pre_Reset, 8 );
	}
	//
	///*
	//#ifdef  CLIENT_DLL
	//// zero-sequences are un-set and should have zero weight on the client
	//for ( int i=0; i < ANIMATION_LAYER_COUNT; i++ )
	//{
	//	CAnimationLayer *pLayer = m_pPlayer->GetAnimOverlay( i, USE_ANIMLAYER_RAW_INDEX );
	//	if ( pLayer && pLayer->GetSequence() == 0 )
	//		pLayer->SetWeight(0);
	//}
//if
	//
// H//ERE
	//*/
	{
		const auto src = pattern::search ( _ ( "client.dll" ), _ ( "F3 0F 10 87 ? ? ? ? 8D 44 24 14 8B 4F 60" ) ).get<void*> ( );
		hooks::ret_animstate_update_animlayers_post_Reset = mid_func_hook ( src, hooks::animstate_update_animlayers_post_Reset, 8 );
	}
	//
	///*
	//m_nLastUpdateFrame = gpGlobals->framecount;
	//*/
	{
		const auto src = pattern::search ( _ ( "client.dll" ), _ ( "C6 05 ? ? ? ? ? 89 47 70 5F 5E" ) ).add(10).get<void*> ( );
		hooks::ret_animstate_update_set_framecount = mid_func_hook ( src, hooks::animstate_update_set_framecount, 5 );
	}
	//
	///*
	//#ifndef  CLIENT_DLL
	//Vector vecAbsVelocity = m_pPlayer->GetAbsVelocity();
//e
	//Vector vecAbsVelocity = m_vecVelocity;
	//
	//if ( engine->IsHLTV() || engine->IsPlayingDemo() )
	//{
	//	// Estimating velocity when playing demos is prone to fail, especially in POVs. Fall back to GetAbsVelocity.
	//	vecAbsVelocity = m_pPlayer->GetAbsVelocity();
	//}
	//else
	//{
	//	m_pPlayer->EstimateAbsVelocity( vecAbsVelocity );	// Using this accessor if the client is starved of information, 
	//														// the player doesn't run on the spot. Note this is unreliable
	//														// and could fail to populate the value if prediction fails.
	//}
	//*/
	//{
	//	const auto src = pattern::search ( _ ( "client.dll" ), _ ( "8B F1 8B CF 89 74 24 14 8B 07 FF 90" ) ).get<void*> ( );
	//	hooks::ret_animstate_SetupVelocity_GetAbsVelocity = mid_func_hook ( src, hooks::animstate_SetupVelocity_GetAbsVelocity, 8 );
	//}
	//
	///*
	//// prevent the client input velocity vector from exceeding a reasonable magnitude
	//#define CSGO_ANIM_MAX_VEL_LIMIT 1.2f
	//if ( vecAbsVelocity.LengthSqr() > Sqr( CS_PLAYER_SPEED_RUN * CSGO_ANIM_MAX_VEL_LIMIT ) )
	//	vecAbsVelocity = vecAbsVelocity.Normalized() * (CS_PLAYER_SPEED_RUN * CSGO_ANIM_MAX_VEL_LIMIT);
	//*/
	{
		hooks::ret_animstate_SetupVelocity_ClampVelocity_to = pattern::search ( _ ( "client.dll" ), _ ( "F3 0F 7E 6C 24 ? F3 0F 11 8E" ) ).get<void*> ( );
	
		const auto src = pattern::search ( _ ( "client.dll" ), _ ( "0F 86 ? ? ? ? F3 0F 7E 4C 24" ) ).get<void*> ( );
		hooks::ret_animstate_SetupVelocity_ClampVelocity = mid_func_hook ( src, hooks::animstate_SetupVelocity_ClampVelocity, 6 );
	}
}

void anims::update ( int stage ) {
	if ( !cs::i::engine->is_in_game ( ) || !cs::i::engine->is_connected ( ) )
		return;

	switch ( stage ) {
	case 5: {
		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
			animations::resolver::process_event_buffer ( i );

			const auto ent = cs::i::ent_list->get< player_t* > ( i );

			if ( !ent || !ent->is_player ( ) ) {
				players::update_time [ i ] = players::anim_times [ i ] = cs::i::globals->m_curtime;
				players::updates_since_dormant [ i ] = 0;
				players::choked_commands [ i ] = 0;
				continue;
			}

			const auto idx = ent->idx ( );

			*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( ent ) + 0xA30 ) = cs::i::globals->m_framecount;
			*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( ent ) + 0xA28 ) = 0;
			*reinterpret_cast< int* >( reinterpret_cast< uintptr_t > ( ent ) + 0xA68 ) = cs::i::globals->m_framecount;

			if ( ent->dormant ( ) ) {
				players::anim_times [ idx ] = ent->simtime();
				players::choked_commands [ idx ] = 0;
				players::update_time [ idx ] = cs::i::globals->m_curtime;
				players::updates_since_dormant [ idx ] = 0;
			}

			if ( ent == g::local ) {

			}
			else {
				
			}
		}
	} break;
	}
}