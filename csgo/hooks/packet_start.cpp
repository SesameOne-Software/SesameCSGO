#include "packet_start.hpp"

decltype( &hooks::packet_start ) hooks::old::packet_start = nullptr;

void __fastcall hooks::packet_start ( REG, int incoming, int outgoing ) {
    if ( !g::local || !g::local->alive ( ) )
        return old::packet_start ( REG_OUT, incoming, outgoing );

    int ack = 0;

    if ( cs::is_valve_server ( ) ) {
        ack = outgoing;
    }
    else {
        network_data_t* net_data = nullptr;

        if ( g::outgoing_cmds [ outgoing % 150 ].sequence == outgoing )
            net_data = &g::outgoing_cmds [ outgoing % 150 ];

        if ( net_data )
            ack = net_data->sequence;
    }

    return old::packet_start ( REG_OUT, incoming, ack );
}