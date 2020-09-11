#include "get_viewmodel_fov.hpp"

#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../menu/options.hpp"

decltype( &hooks::get_viewmodel_fov ) hooks::old::get_viewmodel_fov = nullptr;

float __fastcall hooks::get_viewmodel_fov( REG ) {
	if ( !csgo::i::engine->is_in_game( ) || !csgo::i::engine->is_connected( ) )
		return old::get_viewmodel_fov( REG_OUT );

	static auto& viewmodel_fov = options::vars [ _( "visuals.other.viewmodel_fov" ) ].val.f;

	return static_cast < float > ( viewmodel_fov );
}