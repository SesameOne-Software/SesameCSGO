#include "get_eye_angles.hpp"
#include "../globals.hpp"

namespace lby {
	extern bool in_update;
}

decltype( &hooks::get_eye_angles ) hooks::old::get_eye_angles = nullptr;

vec3_t& __fastcall hooks::get_eye_angles( REG ) {
	if ( !ecx || ecx != g::local )
		return old::get_eye_angles( REG_OUT );

	static auto ret_to_thirdperson_pitch = pattern::search ( _ ( "client.dll" ), _ ( "8B CE F3 0F 10 00 8B 06 F3 0F 11 45 ? FF 90 ? ? ? ? F3 0F 10 55" ) ).get< void* > ( );
	static auto ret_to_thirdperson_yaw = pattern::search ( _ ( "client.dll" ), _ ( "F3 0F 10 55 ? 51 8B 8E" ) ).get< void* > ( );
	static auto ret_to_unk_bone_func = pattern::search ( _ ( "client.dll" ), _ ( "8B 55 0C 8B C8 E8 ? ? ? ? 83 C4 08 5E 8B E5" ) ).get< void* > ( );

	if ( _ReturnAddress ( ) == ret_to_unk_bone_func )
		return g::sent_cmd.m_angs;

	if ( _ReturnAddress ( ) == ret_to_thirdperson_pitch || _ReturnAddress ( ) == ret_to_thirdperson_yaw )
		return lby::in_update ? g::sent_cmd.m_angs : g::angles;

	return old::get_eye_angles( REG_OUT );
}