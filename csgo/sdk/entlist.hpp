#pragma once
#include "../utils/vfunc.hpp"
#include "../utils/padding.hpp"

class c_entlist {
public:
	template < typename t >
	t get( int i ) {
		using getcliententity_fn = t( __thiscall* )( void*, int );
		return vfunc< getcliententity_fn >( this, 3 )( this, i );
	}

	template < typename t >
	t get_by_handle( std::uint32_t h ) {
		using getcliententityfromhandle_fn = t( __thiscall* )( void*, std::uint32_t );
		return vfunc< getcliententityfromhandle_fn >( this, 4 )( this, h );
	}
};