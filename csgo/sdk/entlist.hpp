#pragma once
#include "../utils/vfunc.hpp"
#include "../utils/padding.hpp"

class c_entlist {
public:
	template < typename t >
	__forceinline t get( int i ) {
		using getcliententity_fn = t( __thiscall* )( void*, int );
		return vfunc< getcliententity_fn >( this, 3 )( this, i );
	}

	template < typename t >
	__forceinline t get_by_handle( std::uint32_t h ) {
		using getcliententityfromhandle_fn = t( __thiscall* )( void*, std::uint32_t );
		return vfunc< getcliententityfromhandle_fn >( this, 4 )( this, h );
	}

	__forceinline int get_highest_index ( ) {
		using gethighestentityindex_fn = int ( __thiscall* )( void* );
		return vfunc< gethighestentityindex_fn > ( this, 6 )( this );
	}
};