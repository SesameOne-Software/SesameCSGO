#pragma once
#include <sdk.hpp>

namespace hooks {
	class c_event_handler : c_event_listener {
	public:
		c_event_handler ( );

		virtual ~c_event_handler ( );
		virtual void fire_game_event ( event_t* event );
		int get_event_debug_id ( ) override;
	};

	extern std::unique_ptr< c_event_handler > event_handler;
}