#include "process_interp_list.hpp"

decltype( &hooks::process_interp_list ) hooks::old::process_interp_list = nullptr;

int hooks::process_interp_list( ) {
	static auto& extrapolate = *pattern::search( _( "client.dll" ) , _( "A2 ? ? ? ? 8B 45 E8" ) ).add( 1 ).deref( ).get<bool*>( );

	extrapolate = false;

	return old::process_interp_list();
}