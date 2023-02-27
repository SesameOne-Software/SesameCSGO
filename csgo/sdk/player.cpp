#include <array>
#include "player.hpp"
#include "sdk.hpp"
#include "../hooks/modify_eye_pos.hpp"
#include "../hooks/setup_bones.hpp"

bool player_t::is_enemy ( player_t* other ) {
	static auto mp_teammates_are_enemies = cs::i::cvar->find ( _ ( "mp_teammates_are_enemies" ) );

	if ( !this || !other || this == other )
		return false;

	if ( cs::i::game_type->get_game_type ( ) == game_type_t::danger_zone ) {
		const auto my_team = survival_team ( );

		if ( my_team == -1 )
			return true;

		return my_team != other->survival_team ( );
	}
	else {
		const auto my_team = static_cast< int >( team ( ) ) - 2;
		auto v11 = false;

		if ( my_team ) {
			if ( my_team != 1 )
				return false;

			v11 = other->team ( ) == 2;
		}
		else
			v11 = other->team ( ) == 3;

		if ( v11 )
			return true;

		return mp_teammates_are_enemies->get_bool ( );
	}

	return false;
}

void* planted_c4_t::get_defuser( ) {
	return cs::i::ent_list->get_by_handle<player_t*>( defuser_handle( ) );
}

void animstate_pose_param_cache_t::set_value( player_t* e, float val ) {
	static auto CCSPlayer__GetModelPtr = pattern::search( _( "client.dll" ) , _( "E8 ? ? ? ? 83 C4 04 8B C8 E8 ? ? ? ? 83 B8 C4 00 00 00 00" ) ).resolve_rip( ).get<void* ( __thiscall* )( void* )>( );
	static auto Studio_SetPoseParameter = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 08 F3 0F 11 54 24 ? 85" ) ).get<float( __stdcall* )( void*, int, float, float& )>( );
	static auto CCSPlayer__LookupPoseParameter = pattern::search( _( "client.dll" ) , _( "55 8B EC 57 8B 7D 08 85 FF 75 08 83 C8 FF 5F 5D C2 08 00" ) ).get<int( __stdcall* )( void* , const char* )>( );

	if ( !m_init && e ) {
		MDLCACHE_CRITICAL_SECTION( );

		const auto hdr = CCSPlayer__GetModelPtr( e );

		if ( hdr ) {
			m_idx = CCSPlayer__LookupPoseParameter( hdr , m_name );

			if ( m_idx != -1 )
				m_init = true;
		}
	}

	if ( m_init && e ) {
		MDLCACHE_CRITICAL_SECTION( );

		const auto hdr = CCSPlayer__GetModelPtr( e );

		if ( hdr && m_idx >= 0 ) {
			float new_val = 0.0f;

			__asm {
				mov ecx, hdr
				mov edx, m_idx
				movss xmm2, val
				lea eax, new_val
				push eax
				call Studio_SetPoseParameter
				add esp, 4
			}

			e->poses( ) [ m_idx ] = new_val;
		}
	}
}

void animstate_t::reset( ) {
	static auto reset_animstate = pattern::search( _( "client.dll" ), _( "56 6A 01 68 ? ? ? ? 8B F1" ) ).get< void( __thiscall* )( void* ) >( );
	reset_animstate( this );
}

void animstate_t::update( vec3_t& ang ) {
	static auto update_animstate = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 F3 0F 11 54" ) ).get< void( __vectorcall* )( void*, void*, float, float, float, void* ) >( );
	update_animstate( this, nullptr, ang.z, ang.y, ang.x, nullptr );
}

const char* animstate_t::get_weapon_move_animation ( ) {
	static auto addr = pattern::search ( _ ( "client.dll" ),_( "E8 ? ? ? ? 50 8D 44 24 54") ).resolve_rip().get<const char* ( __thiscall* )( animstate_t* )> ( );
	return addr ( this );
}

void player_t::get_sequence_linear_motion ( void* studio_hdr, int sequence, float* poses, vec3_t* vec ) {
	static auto addr = pattern::search ( _ ( "client.dll" ), _("55 8B EC 83 EC 0C 56 8B F1 57 8B FA 85 F6 75 14") ).get<void ( __fastcall* )( void*, int, float*, vec3_t& )> ( );

	__asm {
		mov edx, sequence
		mov ecx, studio_hdr
		push vec
		push poses
		call addr
		add esp, 8
	}
}

float player_t::get_sequence_move_distance ( void* studio_hdr, int sequence ) {
	vec3_t ret;
	get_sequence_linear_motion ( studio_hdr, sequence, poses().data(), &ret );
	return ret.length ( );
}

int player_t::lookup_sequence ( const char* seq ) {
	static auto addr = pattern::search ( _ ( "client.dll" ), _("E8 ? ? ? ? 5E 83 F8 FF" )).resolve_rip().get<int ( __thiscall* )( player_t*, const char* )> ( );
	return addr ( this, seq );
}

float player_t::sequence_duration ( int sequence ) {
	static auto addr = pattern::search ( _ ( "client.dll" ), _("55 8B EC 56 8B F1 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 83 C4 04 5E 5D C2 04 00 52 68 ? ? ? ? 6A 02" )).get<float ( __thiscall* )( player_t*, int )> ( );

	float retval;
	addr ( this, sequence );
	__asm movss retval, xmm0;
	return retval;
}

float player_t::get_sequence_cycle_rate_server ( int sequence ) {
	float t = sequence_duration ( sequence );
	if ( t > 0.0f )
		return 1.0f / t;
	else
		return 1.0f / 0.1f;
}

bool player_t::physics_run_think( int unk01 ) {
	static auto func = pattern::search( _( "client.dll" ), _( "55 8B EC 83 EC 10 53 56 57 8B F9 8B 87 ? ? ? ? C1 E8 16" ) ).get< bool( __thiscall* )( void*, int ) >( );
	return func( this, unk01 );
}

std::uint32_t player_t::handle( ) {
	using fn = std::uint32_t( __thiscall* )( void* );
	static auto get_ehandle = pattern::search( _( "client.dll" ), _( "8B 51 3C 83 FA FF" ) ).get< fn >( );
	return get_ehandle( this );
}

bool player_t::setup_bones( matrix3x4_t* m, std::uint32_t max, std::uint32_t mask, float seed ) {
	using setupbones_fn = bool( __thiscall* )( void*, matrix3x4_t*, std::uint32_t, std::uint32_t, float );
	hooks::bone_setup::allow = true;
	const auto ret = vfunc< setupbones_fn >( renderable( ), 13 )( renderable( ), m, max, mask, seed );
	hooks::bone_setup::allow = false;
	return ret;
}

void player_t::create_animstate( animstate_t* state ) {
	static auto create_animstate = pattern::search( _( "client.dll" ), _( "55 8B EC 56 8B F1 B9 ? ? ? ? C7 46" ) ).get< void( __thiscall* )( void*, void* ) >( );
	create_animstate( state, this );
}

void player_t::inval_bone_cache( ) {
	static auto invalidate_bone_cache = pattern::search( _( "client.dll" ), _( "80 3D ? ? ? ? ? 74 16 A1 ? ? ? ? 48 C7 81" ) ).add( 10 ).get< uintptr_t >( );

	*( uint32_t* ) ( ( uintptr_t ) this + N( 0x2914 ) ) = N( 0xFF7FFFFF );
	*( uint32_t* ) ( ( uintptr_t ) this + N( 0x2680 ) ) = **( uintptr_t** ) invalidate_bone_cache - 1;
}

void player_t::set_abs_angles( const vec3_t& ang ) {
	using setabsangles_fn = void( __thiscall* )( void*, const vec3_t& );
	static auto set_abs_angles = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 83 EC 64 53 56 57 8B F1 E8" ) ).get< setabsangles_fn >( );
	set_abs_angles( this, ang );
}

void player_t::update( ) {
	using update_clientside_animation_fn = void( __thiscall* )( void* );
	static auto update_clientside_animation = pattern::search( _( "client.dll" ), _( "8B F1 80 BE ? ? ? ? 00 74 ? 8B 06 FF 90 ? ? ? ? 8B CE" ) ).sub( 5 ).get< update_clientside_animation_fn >( );
	update_clientside_animation( this );
}

vec3_t& player_t::abs_vel( ) {
	// enable changing of abs velocities
	// *( std::uint32_t* ) ( std::uintptr_t( this ) + 0xe8 ) |= 0x1000;
	// old abs velocity
	// *( vec3_t* ) ( std::uintptr_t( this ) + 0x94 ) = vel;
	// current abs velocity
	return *( vec3_t* )( std::uintptr_t( this ) + 0x94 );
}

void player_t::set_abs_vel( vec3_t& vel ) {
	using set_abs_vel_fn = void( __thiscall* )( void*, vec3_t& );
	static auto fn = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 83 EC 0C 53 56 57 8B 7D 08 8B F1" ) ).get< set_abs_vel_fn >( );
	fn( this, vel );
}

void player_t::set_abs_origin( const vec3_t& vec ) {
	using set_abs_origin_fn = void( __thiscall* )( void*, const vec3_t& );
	static auto fn = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 51 53 56 57 8B F1 E8 ? ? ? ? 8B 7D" ) ).get< set_abs_origin_fn >( );
	fn( this, vec );
}

animstate_t* player_t::animstate( ) {
	static auto animstate_offset = pattern::search( _( "client.dll" ), _( "8B 8E ? ? ? ? 85 C9 74 3E E8" ) ).add( 2 ).deref( ).get< std::uintptr_t >( );
	return *( animstate_t** )( std::uintptr_t( this ) + animstate_offset );
}

vec3_t player_t::eyes( ) {
	vec3_t pos = origin( ) + view_offset( );

	/* eye position */
	vfunc< void( __thiscall* )( player_t*, vec3_t& ) >( this, 169 ) ( this, pos );

	if ( *reinterpret_cast< bool* > ( uintptr_t ( this ) + 0x39E1 ) && this == g::local && animstate ( ) )
		hooks::modify_eye_pos( animstate( ), nullptr, pos );

	return pos;
}

std::uint32_t& player_t::bone_count( ) {
	static auto offset = pattern::search( _( "client.dll" ), _( "8B 87 ? ? ? ? 8B 4D 0C" ) ).add( 2 ).deref( ).get< std::uint32_t >( );
	return *( uint32_t* )( std::uintptr_t( renderable( ) ) + offset );
}

matrix3x4_t*& player_t::bone_cache( ) {
	static auto offset = pattern::search( _( "client.dll" ), _( "FF B7 ? ? ? ? 52" ) ).add( 2 ).deref( ).get< std::uint32_t >( );
	return *( matrix3x4_t** )( std::uintptr_t( renderable( ) ) + offset );
}

weapon_t* player_t::weapon( ) {
	return cs::i::ent_list->get_by_handle< weapon_t* >( weapon_handle( ) );
}

entity_t* player_t::vehicle ( ) {
	auto handle = vehicle_handle ( );
	int* v14 = nullptr;

	if ( handle != -1
		&& ( v14 = cs::i::ent_list->get_by_handle< int* > ( handle ) ) != 0
		&& v14 [ 1 ] == *( reinterpret_cast< unsigned short int* >( &handle ) + 1 )
		&& *v14 )
		return ( *( entity_t* ( __thiscall** )( int ) )( *( DWORD* ) *v14 + 396 ) )( *v14 );

	return nullptr;
}

std::vector<weapon_t*> player_t::weapons ( ) {
	static auto offset = netvars::get_offset ( _ ( "DT_BaseCombatCharacter->m_hMyWeapons" ) );

	std::vector<weapon_t*> ret {};

	const auto my_weapons = reinterpret_cast<uint32_t*>( reinterpret_cast< uintptr_t >( this ) + offset );

	for ( auto i = 0; my_weapons [ i ] != 0xFFFFFFFF; i++ ) {
		const auto weapon = cs::i::ent_list->get_by_handle<weapon_t*> ( my_weapons [ i ] );
		
		if ( !weapon )
			continue;

		ret.push_back ( weapon );
	}

	return ret;
}

std::vector<weapon_t*> player_t::wearables ( ) {
	static auto offset = netvars::get_offset ( _ ( "DT_BaseCombatCharacter->m_hMyWearables" ) );

	std::vector<weapon_t*> ret {};

	const auto my_weapons = reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( this ) + offset );

	for ( auto i = 0; my_weapons [ i ] != 0xFFFFFFFF; i++ ) {
		const auto weapon = cs::i::ent_list->get_by_handle<weapon_t*> ( my_weapons [ i ] );

		if ( !weapon )
			continue;

		ret.push_back ( weapon );
	}

	return ret;
}

void* player_t::get_original_data ( ) {
	static auto offset = pattern::search ( _ ( "client.dll" ), _ ( "8B 8F ? ? ? ? 33 D2 85 C9 74 0F 48 BE" ) ).add ( 2 ).deref ( ).get< uint32_t > ( );
	return *reinterpret_cast<void**> ( reinterpret_cast< uintptr_t >( this ) + offset );
}

void* player_t::get_predicted_frame ( int frame_num ) {
	static auto offset = pattern::search ( _ ( "client.dll" ), _ ( "8B 94 97 ? ? ? ? 33 C0" ) ).add ( 3 ).deref ( ).get< uint32_t > ( );
	return reinterpret_cast< void** >( this ) [ ( frame_num % 150 ) + ( offset / 4 ) ];
}