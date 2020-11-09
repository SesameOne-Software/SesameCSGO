#include "write_usercmd_delta_to_buffer.hpp"

#include "../features/ragebot.hpp"

decltype( &hooks::write_usercmd_delta_to_buffer ) hooks::old::write_usercmd_delta_to_buffer = nullptr;

class nc_t {
public:
	PAD( 23 );
	bool should_delete;
	int out_seq_nr;
	int in_seq_nr;
	int out_seq_nr_ack;
	int out_reliable_state;
	int in_reliable_state;
	int choked_packets;
};

bool __fastcall hooks::write_usercmd_delta_to_buffer( REG, int slot, void* buf, int from, int to, bool new_cmd ) {
	return old::write_usercmd_delta_to_buffer ( REG_OUT, slot, buf, from, to, new_cmd );
}