#pragma once
#include <cstdint>

template < typename t >
__forceinline t vfunc( void* thisptr, std::uintptr_t idx ) {
	return reinterpret_cast< t >( ( *reinterpret_cast< std::uintptr_t** >( thisptr ) ) [ idx ] );
}