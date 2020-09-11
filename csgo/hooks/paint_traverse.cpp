#include "paint_traverse.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../menu/options.hpp"

decltype( &hooks::paint_traverse ) hooks::old::paint_traverse = nullptr;

void __fastcall hooks::paint_traverse( REG, int ipanel, bool force_repaint, bool allow_force ) {
	if ( !csgo::i::engine->is_in_game( ) || !csgo::i::engine->is_connected( ) )
		return old::paint_traverse( REG_OUT, ipanel, force_repaint, allow_force );

	static auto& removals = options::vars [ _( "visuals.other.removals" ) ].val.l;

	static auto override_post_processing_disable = pattern::search( _( "client.dll" ), _( "80 3D ? ? ? ? ? 53 56 57 0F 85" ) ).add( 2 ).deref( ).get< bool* >( );

	if ( removals [ 2 ] && !strcmp( _( "HudZoom" ), csgo::i::panel->get_name( ipanel ) ) )
		return;

	old::paint_traverse( REG_OUT, ipanel, force_repaint, allow_force );

	*override_post_processing_disable = g::local && g::local->scoped( ) && removals [ 2 ];

	if ( removals [ 8 ] )
		*override_post_processing_disable = true;
}