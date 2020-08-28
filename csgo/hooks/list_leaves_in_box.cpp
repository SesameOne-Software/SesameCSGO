#include "list_leaves_in_box.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../menu/options.hpp"

decltype( &hooks::list_leaves_in_box ) hooks::old::list_leaves_in_box = nullptr;

int __fastcall hooks::list_leaves_in_box( REG, vec3_t& mins, vec3_t& maxs, uint16_t* list, int list_max ) {
	static auto& removals = options::vars [ _( "visuals.other.removals" ) ].val.l;

	if ( !removals [ 6 ] || !g::local )
		return old::list_leaves_in_box( REG_OUT, mins, maxs, list, list_max );

	struct render_info_t {
		void* m_renderable;
		void* m_alpha_property;
		int m_enum_count;
		int m_render_frame;
		unsigned short m_first_shadow;
		unsigned short m_leaf_list;
		short m_area;
		uint16_t m_flags;
		uint16_t m_flags2;
		vec3_t m_bloated_abs_mins;
		vec3_t m_bloated_abs_maxs;
		vec3_t m_abs_mins;
		vec3_t m_abs_maxs;
		PAD( 4 );
	};

	static auto ret_addr = pattern::search( _( "client.dll" ), _( "56 52 FF 50 18" ) ).add( 5 ).get< void* >( );

	if ( _ReturnAddress( ) != ret_addr )
		return old::list_leaves_in_box( REG_OUT, mins, maxs, list, list_max );

	auto info = *( render_info_t** )( ( uintptr_t )_AddressOfReturnAddress( ) + 0x14 );

	if ( !info || !info->m_renderable )
		return old::list_leaves_in_box( REG_OUT, mins, maxs, list, list_max );

	auto get_client_unknown = [ ] ( void* renderable ) {
		typedef void* ( __thiscall* o_fn )( void* );
		return ( *( o_fn** )renderable ) [ 0 ]( renderable );
	};

	auto get_base_entity = [ ] ( void* c_unk ) {
		typedef player_t* ( __thiscall* o_fn )( void* );
		return ( *( o_fn** )c_unk ) [ 7 ]( c_unk );
	};

	auto base_entity = get_base_entity( get_client_unknown( info->m_renderable ) );

	if ( !base_entity || base_entity->idx( ) > 64 || !base_entity->idx( ) )
		return old::list_leaves_in_box( REG_OUT, mins, maxs, list, list_max );

	info->m_flags &= ~0x100;
	info->m_flags2 |= 0x40;

	auto mins_out = vec3_t ( -16384.0f, -16384.0f, -16384.0f );
	auto maxs_out = vec3_t ( 16384.0f, 16384.0f, 16384.0f );

	return old::list_leaves_in_box( REG_OUT, mins_out, maxs_out, list, list_max );
}