#include "send_datagram.hpp"

#include "../menu/options.hpp"
#include "../features/exploits.hpp"

decltype( &hooks::send_datagram ) hooks::old::send_datagram = nullptr;

extern bool in_cm;

int __fastcall hooks::send_datagram ( REG, void* datagram ) {
	static auto& extended_lagcomp_enabled = options::vars [ _ ( "ragebot.extended_lagcomp_enabled" ) ].val.b;
	static auto& extended_lagcomp_time = options::vars [ _ ( "ragebot.extended_lagcomp_ms" ) ].val.i;

	if ( !cs::i::engine->is_in_game ( ) || !cs::i::engine->is_connected ( ) || datagram || !extended_lagcomp_enabled )
		return old::send_datagram ( REG_OUT, datagram );

	const auto nc = cs::i::client_state->net_channel ( );

	if ( nc ) {
		const auto instate = nc->in_reliable_state;
		const auto insequencenr = nc->in_seq_nr;

		for ( auto& seq : exploits::incoming_seqs ) {
			if ( cs::i::globals->m_realtime - seq.curtime >= static_cast<float>( extended_lagcomp_time ) / 1000.0f ) {
				nc->in_reliable_state = seq.in_reliable_state;
				nc->in_seq_nr = seq.seq_nr;
				break;
			}
		}

		const auto ret = old::send_datagram ( REG_OUT, datagram );

		nc->in_reliable_state = instate;
		nc->in_seq_nr = insequencenr;

		return ret;
	}

	return old::send_datagram ( REG_OUT, datagram );
}