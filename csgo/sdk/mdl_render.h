#pragma once
#include "../utils/vfunc.h"
#include "../utils/padding.h"

class c_mdlrender {
public:
	void force_mat( class material_t* mat ) {
		using forcedmatoverride_fn = void( __thiscall* )( void*, material_t*, void*, void* );
		vfunc< forcedmatoverride_fn >( this, 1 )( this, mat, nullptr, nullptr );
	}
};