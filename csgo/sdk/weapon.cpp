#include "weapon.hpp"
#include "../utils/utils.hpp"
#include "sdk.hpp"
#include "../features/skinchanger.hpp"

c_econ_item* c_econ_item::static_data ( ) {
	using fn = c_econ_item * ( __thiscall* )( c_econ_item* );
	static auto get_static_data = pattern::search ( _("client.dll"), _("55 8B EC 51 53 8B D9 8B 0D ? ? ? ? 56 57 8B B9" )).get<fn> ( );
	return get_static_data ( this );
}

void c_econ_item::update_equipped_state ( bool state ) {
	using fn = int ( __thiscall* )( c_econ_item*, uint32_t );
	static auto update_equipped_state = pattern::search ( _("client.dll"), _("55 8B EC 8B 45 08 8B D0 C1 EA 10" )).get<fn> ( );
	update_equipped_state ( this, static_cast< uint32_t >( state ) );
}

/* XREF: "resource/flash/econ/weapons/cached/*.iic" string is inside the function */
void c_econ_item::clean_inventory_image_cache_dir ( ) {
	using fn = void ( __thiscall* )( c_econ_item* );
	static auto clean_inventory_image_cache_dir = pattern::search ( _("client.dll"), _("55 8B EC 81 EC ? ? ? ? 80 3D ? ? ? ? ? 56 0F 85") ).get< fn > ( );
	clean_inventory_image_cache_dir ( this );
}

template< typename t >
void c_econ_item::set_or_add_attribute_by_name ( t val, const char* attribute_name ) {
	static auto _set_or_add_attribute_by_name = pattern::search ( _("client.dll"),_( "55 8B EC 83 EC 30 53 56 8B F1 F3" )).get< void* > ( );

	const auto as_float = float ( val );

	__asm {
		mov ecx, this
		movss xmm2, as_float
		push attribute_name
		call _set_or_add_attribute_by_name
	}
}

void c_econ_item::set_custom_name ( const char* name ) {
	using fn = c_econ_item * ( __thiscall* )( c_econ_item*, const char* );
	static auto set_custom_name_fn = pattern::search ( _("client.dll"), _("E8 ? ? ? ? 8B 46 78 C1 E8 0A A8 01 74 13 8B 46 34") ).resolve_rip().get<fn>();
	set_custom_name_fn ( this, name );
}

void c_econ_item::set_custom_desc ( const char* name ) {
	using fn = c_econ_item * ( __thiscall* )( c_econ_item*, const char* );
	static auto set_custom_name_fn = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 33 DB 39 5E 3C 7E 5E" ) ).resolve_rip ( ).get<fn> ( );
	set_custom_name_fn ( this, name );
}

void c_econ_item::set_attribute ( int index, int val ) {
	auto v15 = reinterpret_cast< uint32_t* >( features::inventory::get_item_schema ( ) );
	auto v16 = *reinterpret_cast< uint32_t* > ( v15 [ 72 ] + 4 * index );

	using fn = int ( __thiscall* )( c_econ_item*, uint32_t, void* );
	static auto set_dynamic_attribute_val = pattern::search (_( "client.dll"),_( "55 8B EC 83 E4 F8 83 EC 3C 53 8B 5D 08 56 57 6A 00") ).get< fn > ( );

	set_dynamic_attribute_val ( this, v16, &val );
}

void c_econ_item::set_attribute ( int index, float val ) {
	auto as_int = *reinterpret_cast< int* >(&val );
	set_attribute ( index, as_int );
}

c_econ_item* weapon_t::econ_item ( ) {
	static auto offset = netvars::get_offset ( _ ( "DT_BaseAttributableItem->m_Item" ) );
	return reinterpret_cast< c_econ_item* >( reinterpret_cast<uintptr_t>(this)+ offset );
}

c_econ_item* c_econ_item::soc_data ( ) {
	using fn = c_econ_item * ( __thiscall* )( c_econ_item* );
	static auto get_soc_data = pattern::search ( "client.dll", "55 8B EC 83 E4 F0 83 EC 18 56 8B F1 57 8B 86" ).get<fn> ( );
	return get_soc_data ( this );
}

weapon_info_t* weapon_t::data( ) {
	static auto weapon_system = pattern::search( _( "client.dll" ), _( "8B 35 ? ? ? ? FF 10 0F B7 C0" ) ).add( 2 ).deref( ).get< void* >( );
	using ffn = weapon_info_t * ( __thiscall* )( void*, std::uint16_t );
	return vfunc< ffn >( weapon_system, 2 )( weapon_system, item_definition_index( ) );
}

weapon_t* weapon_t::world_mdl ( ) {
	return csgo::i::ent_list->get_by_handle<weapon_t*>( world_model_handle() );
}

float weapon_t::inaccuracy( ) {
	using fn = float( __thiscall* )( void* );
	return vfunc< fn >( this, 482 )( this );
}

float weapon_t::spread( ) {
	using fn = float( __thiscall* )( void* );
	return vfunc< fn >( this, 452 )( this );
}

float weapon_t::max_speed( ) {
	using fn = float( __thiscall* )( void* );
	return vfunc< fn >( this, 441 )( this );
}