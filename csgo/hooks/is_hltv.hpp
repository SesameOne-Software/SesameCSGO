#pragma once
#include <sdk.hpp>

namespace hooks {
	bool __fastcall is_hltv ( REG );

	namespace old {
		extern decltype( &is_hltv ) is_hltv;
	}
}