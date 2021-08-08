#include "svc_msg_voice_data.hpp"

#include "../features/esp.hpp"

decltype( &hooks::svc_msg_voice_data ) hooks::old::svc_msg_voice_data = nullptr;

bool __fastcall hooks::svc_msg_voice_data ( REG, void* voice_data ) {
	const auto ret = old::svc_msg_voice_data(REG_OUT, voice_data );

	features::esp::recieve_voice_data ( reinterpret_cast< features::esp::voice_data* >( voice_data ) );

	return ret;
}