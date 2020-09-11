#include "get_eye_angles.hpp"
#include "../globals.hpp"

namespace lby {
	extern bool in_update;
}

decltype( &hooks::get_eye_angles ) hooks::old::get_eye_angles = nullptr;

vec3_t& __fastcall hooks::get_eye_angles( REG ) {
	if ( !ecx || ecx != g::local )
		return old::get_eye_angles( REG_OUT );

	static auto ret_to_thirdperson_pitch = pattern::search( _( "client.dll" ), _( "8B CE F3 0F 10 00 8B 06 F3 0F 11 45 ? FF 90 ? ? ? ? F3 0F 10 55" ) ).get< std::uintptr_t >( );
	static auto ret_to_thirdperson_yaw = pattern::search( _( "client.dll" ), _( "F3 0F 10 55 ? 51 8B 8E" ) ).get< std::uintptr_t >( );

	if ( std::uintptr_t( _ReturnAddress( ) ) == ret_to_thirdperson_pitch
		|| std::uintptr_t( _ReturnAddress( ) ) == ret_to_thirdperson_yaw )
		return lby::in_update ? g::sent_cmd.m_angs : g::angles;

	return old::get_eye_angles( REG_OUT );
}