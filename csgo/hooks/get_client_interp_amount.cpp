#include "get_client_interp_amount.hpp"

#include "../menu/options.hpp"

decltype( &hooks::get_client_interp_amount ) hooks::old::get_client_interp_amount = nullptr;

float __cdecl hooks::get_client_interp_amount ( ) {
    static int& interp_ticks = options::vars [ _ ( "misc.effects.view_interpolation" ) ].val.i;

    if ( interp_ticks == -1 )
        return old::get_client_interp_amount ( );

    return cs::ticks2time ( interp_ticks );
}