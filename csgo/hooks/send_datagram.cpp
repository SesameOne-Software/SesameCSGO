#include "send_datagram.hpp"

class c_nc {
public:
	PAD ( 23 );
	bool should_delete;
	int out_seq_nr;
	int in_seq_nr;
	int out_seq_nr_ack;
	int out_reliable_state;
	int in_reliable_state;
	int choked_packets;
};

decltype( &hooks::send_datagram ) hooks::old::send_datagram = nullptr;

int __fastcall hooks::send_datagram ( REG, void* datagram ) {
	const auto nc = *reinterpret_cast< c_nc** >( uintptr_t ( cs::i::client_state ) + 0x9c );

	if ( nc ) {

	}

	return old::send_datagram ( REG_OUT, datagram );
}