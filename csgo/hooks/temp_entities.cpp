#include "temp_entities.hpp"

decltype( &hooks::temp_entities ) hooks::old::temp_entities = nullptr;

bool __fastcall hooks::temp_entities ( REG, void* msg ) {
	const auto ret = old::temp_entities ( REG_OUT, msg );
	
	if ( !g::local || !g::local->alive ( ) )
		return ret;

	for ( auto ei = cs::i::client_state->events ( ); ei; ei = ei->next ) {
		const auto create_event = ei->client_class->m_create_event_fn;

		if ( !create_event || !create_event ( ) )
			continue;

		const auto class_id = ei->class_id - 1;

		ei->fire_delay = 0.0f;

		//if ( ( class_id >= 175 && class_id <= 222 ) || class_id == 225 ) {
		//	ei->fire_delay = 0.0f;
		//	dbg_print ( _ ( "NULLED EVENT DELAY\n" ) );
		//}
	}

	return ret;
}