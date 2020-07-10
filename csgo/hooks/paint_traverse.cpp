#include "paint_traverse.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"

decltype( &hooks::paint_traverse ) hooks::old::paint_traverse = nullptr;

void __fastcall hooks::paint_traverse ( REG, int ipanel, bool force_repaint, bool allow_force ) {
	OPTION ( bool, no_scope, "Sesame->C->Other->Removals->No Scope", oxui::object_checkbox );

	static auto override_post_processing_disable = pattern::search ( _ ( "client.dll" ), _ ( "80 3D ? ? ? ? ? 53 56 57 0F 85" ) ).add ( 2 ).deref ( ).get< bool* > ( );

	if ( no_scope && !strcmp ( _ ( "HudZoom" ), csgo::i::panel->get_name ( ipanel ) ) )
		return;

	old::paint_traverse ( REG_OUT, ipanel, force_repaint, allow_force );

	*override_post_processing_disable = g::local && g::local->scoped ( ) && no_scope;
}