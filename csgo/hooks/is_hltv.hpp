#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	bool __fastcall is_hltv ( REG );

	namespace old {
		extern decltype( &hooks::is_hltv ) is_hltv;
	}
}