#include "post_network_data_received.hpp"

decltype( &hooks::post_network_data_received ) hooks::old::post_network_data_received = nullptr;

/* stop game from updating animations */
void __fastcall hooks::post_network_data_received ( REG, int commands_acknowledged ) {
	if ( !g::local || reinterpret_cast< player_t* >( ecx ) != g::local || commands_acknowledged <= 0 )
		return old::post_network_data_received ( REG_OUT, commands_acknowledged );

	old::post_network_data_received ( REG_OUT, commands_acknowledged );


}