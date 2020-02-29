#include "weapon.hpp"
#include "../utils/utils.hpp"

weapon_info_t* weapon_t::data( ) {
	static auto weapon_system = pattern::search( _( "client_panorama.dll"), _( "8B 35 ? ? ? ? FF 10 0F B7 C0" )).add( 2 ).deref( ).get< void* >( );
	using ffn = weapon_info_t * ( __thiscall* )( void*, std::uint16_t );
	return vfunc< ffn >( weapon_system, 2 )( weapon_system, item_definition_index( ) );
}

float weapon_t::inaccuracy( ) {
	using fn = float( __thiscall* )( void* );
	return vfunc< fn >( this, 481 )( this );
}

float weapon_t::spread( ) {
	using fn = float( __thiscall* )( void* );
	return vfunc< fn >( this, 451 )( this );
}

float weapon_t::max_speed( ) {
	using fn = float( __thiscall* )( void* );
	return vfunc< fn >( this, 438 )( this );
}