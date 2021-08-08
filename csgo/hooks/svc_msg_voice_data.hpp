#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	bool __fastcall svc_msg_voice_data ( REG, void* voice_data );

	namespace old {
		extern decltype( &hooks::svc_msg_voice_data ) svc_msg_voice_data;
	}
}