#pragma once
#include <cstdint>
#include <intrin.h>

uint32_t constexpr vfunc_xorkey_hash ( char const* input ) {
    return *input ? static_cast< uint32_t >( *input ) + 33 * vfunc_xorkey_hash ( input + 1 ) : 5381;
}

/* they key will be different for every file the vfunc is used in, and the key will also change every day the hack is built */
constexpr uint32_t vfunc_xorkey = vfunc_xorkey_hash ( __DATE__ );

#define IDX(x) ( ( x ) ^ vfunc_xorkey )

template<typename function_type, /* Should be same offset as the macro */ unsigned int xored_index = vfunc_xorkey>
__declspec( noinline ) function_type __fastcall hidden_vcall_helper ( void* base, int idx ) {
    ++* ( uint8_t* ) _AddressOfReturnAddress ( );
    return ( function_type ) ( ( *( int** ) ( base ) ) [ idx ^ xored_index ] );
}

template<typename function_type>
__forceinline function_type hidden_vcall ( void* base, int idx ) {
    hidden_vcall_helper<function_type> ( base, idx );
    __asm _emit 0xC3 /* Spoof return */
    function_type result = nullptr;
    __asm mov result, eax
    return result;
}

//template < typename t >
//__forceinline t vfunc( void* thisptr, uintptr_t idx ) {
//	return hidden_vcall<t>(thisptr, IDX( idx ) );
//}

template < typename t >
__forceinline t vfunc ( void* thisptr, uintptr_t idx ) {
    return reinterpret_cast< t >( ( *reinterpret_cast< uintptr_t** >( thisptr ) ) [ idx ] );
}