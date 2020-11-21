#include "lock_cursor.hpp"
#include "../menu/menu.hpp"

decltype( &hooks::lock_cursor ) hooks::old::lock_cursor = nullptr;

void __fastcall hooks::lock_cursor ( REG ) {
	if ( gui::opened ) {
		cs::i::surface->unlock_cursor ( );
		return;
	}

	old::lock_cursor ( REG_OUT );
}