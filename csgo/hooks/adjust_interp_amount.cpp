#include "adjust_interp_amount.hpp"

#include "../menu/options.hpp"

decltype( &hooks::adjust_interp_amount ) hooks::old::adjust_interp_amount = nullptr;

float __fastcall hooks::adjust_interp_amount ( REG, int type ) {
    static int& interp_ticks = options::vars [ _ ( "misc.effects.view_interpolation" ) ].val.i;

    if ( interp_ticks == -1 )
        return old::adjust_interp_amount ( REG_OUT, type );

    return cs::ticks2time ( interp_ticks );
}