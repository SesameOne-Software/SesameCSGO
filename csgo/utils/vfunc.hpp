#pragma once
#include <cstdint>

template < typename t >
__forceinline t vfunc( void* thisptr, uintptr_t idx ) {
	return reinterpret_cast< t >( ( *reinterpret_cast< uintptr_t** >( thisptr ) ) [ idx ] );
}