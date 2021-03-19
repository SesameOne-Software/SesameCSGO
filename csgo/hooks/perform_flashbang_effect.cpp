#include "perform_flashbang_effect.hpp"
#include "../menu/options.hpp"

decltype( &hooks::perform_flashbang_effect ) hooks::old::perform_flashbang_effect = nullptr;

/* hook for proper removal of flashbang effect (without modifying flash alpha or duration cuz it messes stuff up) */
void __fastcall hooks::perform_flashbang_effect ( REG, void* view_setup ) {
	static auto& removals = options::vars [ _ ( "visuals.other.removals" ) ].val.l;

	if ( removals [ 1 ] )
		return;

	old::perform_flashbang_effect ( REG_OUT, view_setup );
}