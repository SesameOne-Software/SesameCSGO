#include "prediction_error_handler.hpp"

decltype( &hooks::prediction_error_handler ) hooks::old::prediction_error_handler = nullptr;

int __fastcall hooks::prediction_error_handler ( REG, int a2, int a3, int a4 ) {
	const auto HandlePredictionErrors_a1 = reinterpret_cast<uintptr_t*>( ecx );
	const auto HandlePredictionErrors_a2 = a4;
	const auto HandlePredictionErrors_a3 = 0;

	const auto v4 = ( int* ) ( *( uintptr_t* ) ( HandlePredictionErrors_a2 + 20 ) + 48 * HandlePredictionErrors_a3 );
    
    auto v6 = *v4;
	auto result = v4 [ 3 ];
    auto v3 = 0;
    auto v138 = 0;
    auto v8 = 0;
    auto v142 = 0;
    auto v136 = result;

	if ( result > 0 ) {
        while ( true ) {
            result = 15 * v3;
            v8 = v6 + 60 * v3;
            v142 = v8;

            if ( !( *( uint16_t* ) ( v8 + 14 ) & 0x400 ) )
                break;

            v3 = v138 + 1;
            v138 = v3;
            if ( v3 >= v136 )
                return result;
        }

        auto v9 = ( float* ) ( HandlePredictionErrors_a1 [ 2 ] + *( uintptr_t* ) ( v8 + 4 * HandlePredictionErrors_a1 [ 4 ] + 48 ) );
        auto v10 = *( uint16_t* ) ( v8 + 12 );
        auto v146 = ( uint8_t* ) v9;
        auto v11 = ( const char* ) ( *( uintptr_t* ) ( v8 + 4 * HandlePredictionErrors_a1 [ 5 ] + 48 ) + HandlePredictionErrors_a1 [ 3 ] );
        result = *( uint32_t* ) v8 - 1;
        auto v144 = ( char* ) v11;

        if ( v10 ) {
            auto start = v9;
            auto end = v144 - ( char* ) v9;
            auto i = 0;

            switch ( result ) {
            case 0: /* float */
                break;
            case 1: /* string */
                break;
            case 2: /* vec[] */
                break;
            case 3: /* quaternion[] */
                break;
            case 4: /* int */
                do {
                    if ( *( int* ) start != *( int* ) ( ( char* ) start + end ) ) {
                        dbg_print ( _("PREDICTION ERROR INT\n") );
                    }
                    ++i;
                    ++start;
                } while ( i < v10 );
                break;
            case 5: /* bool */
                break;
            case 6: /* short */
                break;
            case 7: /* byte */
                break;
            case 8: /* color */
                break;
            case 12: /* ehandle */
                break;
            default:
                break;
            }
        }
	}

	return old::prediction_error_handler ( REG_OUT, a2, a3, a4 );
}