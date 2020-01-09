#include "player.h"
#include "sdk.h"

void animstate_pose_param_cache_t::set_value( player_t* e, float val ) {
	auto v5 = *reinterpret_cast< std::uint32_t* >( std::uintptr_t( e ) + 0x294C );

	if ( !v5 || *reinterpret_cast< std::uint32_t* >( v5 ) )
		v5 = 0;

	if ( v5 && m_idx >= 0 )
		e->poseparam( ) [ m_idx ] = val;
}

void animstate_t::reset( ) {
	static auto reset_animstate = pattern::search( "client_panorama.dll", "56 6A 01 68 ? ? ? ? 8B F1" ).get< void( __thiscall* )( void* ) >( );
	reset_animstate( this );
}

void animstate_t::update( vec3_t& ang ) {
	static auto update_animstate = pattern::search( "client_panorama.dll", "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 F3 0F 11 54" ).get< void( __vectorcall* )( void*, void*, float, float, float, void* ) >( );
	update_animstate( this, nullptr, ang.z, ang.y, ang.x, nullptr );
}

bool player_t::physics_run_think( int unk01 ) {
	static auto func = pattern::search( "client_panorama.dll", "55 8B EC 83 EC 10 53 56 57 8B F9 8B 87 ? ? ? ? C1 E8 16" ).get< bool( __thiscall* )( void*, int ) >( );
	return func( this, unk01 );
}

std::uint32_t player_t::handle( ) {
	using fn = std::uint32_t( __thiscall* )( void* );
	static auto get_ehandle = pattern::search( "client_panorama.dll", "8B 51 3C 83 FA FF" ).get< fn >( );
	return get_ehandle( this );
}

void player_t::create_animstate( animstate_t* state ) {
	static auto create_animstate = pattern::search( "client_panorama.dll", "55 8B EC 56 8B F1 B9 ? ? ? ? C7 46" ).get< void( __thiscall* )( void*, void* ) >( );
	create_animstate( state, this );
}

void player_t::inval_bone_cache( ) {
	static auto invalidate_bone_bache = pattern::search( "client_panorama.dll", "80 3D ? ? ? ? ? 74 16 A1 ? ? ? ? 48 C7 81" ).add( 10 ).get< std::uintptr_t >( );

	*( std::uint32_t* ) ( ( std::uintptr_t ) this + 0x2924 ) = 0xFF7FFFFF;
	*( std::uint32_t* ) ( ( std::uintptr_t ) this + 0x2690 ) = **( std::uintptr_t** ) invalidate_bone_bache - 1;
}

void player_t::set_abs_angles( const vec3_t& ang ) {
	using setabsangles_fn = void( __thiscall* )( void*, const vec3_t& );
	static auto set_abs_angles = pattern::search( "client_panorama.dll", "55 8B EC 83 E4 F8 83 EC 64 53 56 57 8B F1 E8" ).get< setabsangles_fn >( );
	set_abs_angles( this, ang );
}

void player_t::update( ) {
	using update_clientside_animation_fn = void( __thiscall* )( void* );
	static auto update_clientside_animation = pattern::search( "client_panorama.dll", "8B F1 80 BE ? ? ? ? 00 74 ? 8B 06 FF 90 ? ? ? ? 8B CE" ).sub( 5 ).get< update_clientside_animation_fn >( );
	update_clientside_animation( this );
}

vec3_t& player_t::abs_vel( ) {
	// enable changing of abs velocities
	// *( std::uint32_t* ) ( std::uintptr_t( this ) + 0xe8 ) |= 0x1000;
	// old abs velocity
	// *( vec3_t* ) ( std::uintptr_t( this ) + 0x94 ) = vel;
	// current abs velocity
	return *( vec3_t* ) ( std::uintptr_t( this ) + 0x94 );
}

void player_t::set_abs_vel( vec3_t& vel ) {
	using set_abs_vel_fn = void( __thiscall* )( void*, vec3_t& );
	static auto fn = pattern::search( "client_panorama.dll", "55 8B EC 83 E4 F8 83 EC 0C 53 56 57 8B 7D 08 8B F1" ).get< set_abs_vel_fn >( );
	fn( this, vel );
}

void player_t::set_abs_origin( vec3_t& vec ) {
	using set_abs_origin_fn = void( __thiscall* )( void*, vec3_t& );
	static auto fn = pattern::search( "client_panorama.dll", "55 8B EC 83 E4 F8 51 53 56 57 8B F1 E8 ? ? ? ? 8B 7D" ).get< set_abs_origin_fn >( );
	fn( this, vec );
}

animstate_t* player_t::animstate( ) {
	static auto animstate_offset = pattern::search( "client_panorama.dll", "8B 8E ? ? ? ? F3 0F 10 48 04 E8 ? ? ? ? E9" ).add( 2 ).deref( ).get< std::uintptr_t >( );
	return *( animstate_t** ) ( std::uintptr_t( this ) + animstate_offset );
}

vec3_t player_t::eyes( ) {
	using weapon_shootpos_fn = float*( __thiscall* )( void*, vec3_t* );
	static auto fn = pattern::search( "client_panorama.dll", "55 8B EC 56 8B 75 08 57 8B F9 56 8B 07 FF 90" ).get< weapon_shootpos_fn >( );

	vec3_t ret;
	fn( this, &ret );

	return ret;
}

std::uint32_t& player_t::bone_count( ) {
	static auto offset = pattern::search( "client_panorama.dll", "8B 87 ? ? ? ? 8B 4D 0C" ).add( 2 ).deref( ).get< std::uint32_t >( );
	return *( uint32_t* ) ( std::uintptr_t( renderable( ) ) + offset );
}

matrix3x4_t* player_t::bone_cache( ) {
	static auto offset = pattern::search( "client_panorama.dll", "FF B7 ? ? ? ? 52" ).add( 2 ).deref( ).get< std::uint32_t >( );
	return *( matrix3x4_t** ) ( std::uintptr_t( renderable( ) ) + offset );
}

weapon_t* player_t::weapon( ) {
	return csgo::i::ent_list->get_by_handle< weapon_t* >( weapon_handle( ) );
}