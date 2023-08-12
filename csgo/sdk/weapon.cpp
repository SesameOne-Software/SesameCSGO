#include "weapon.hpp"
#include "../utils/utils.hpp"
#include "sdk.hpp"
#include "../features/skinchanger.hpp"

std::string c_econ_item::build_inventory_image ( ) {
	struct String_t
	{
		char* buffer;	//0x0000
		int capacity;	//0x0004
		int grow_size;	//0x0008
		int length;		//0x000C
	}; //Size=0x0010

	struct CPaintKit
	{
		int id;						//0x0000

		String_t name;				//0x0004
		String_t description;		//0x0014
		String_t item_name;			//0x0024
		String_t material_name;		//0x0034
		String_t image_inventory;	//0x0044

		char pad_0x0054 [ 0x8C ];		//0x0054
	}; //Size=0x00E0

	using fn = void ( * )( char*, int, const char*, const char*, bool, float, bool );
	using fn1 = int ( __thiscall* )( c_econ_item* );
	using fn2 = float ( __thiscall* )( c_econ_item* );
	static auto build_inventory_image_path = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 8B 5D 08 8B CB 56 57 E8" ) ).get<fn> ( );
	static auto get_custom_paintkit_index = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F8 A1 ? ? ? ? 83 EC 18 56 57 8B F9 A8 01" ) ).get<fn1> ( );
	static auto get_paint_kit_definition_fn = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 8B F0 8B 4E 6C" ) ).resolve_rip ( ).get<CPaintKit* ( __thiscall* )( void*, int )> ( );
	static auto item_schema =*reinterpret_cast<void**>( pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? FF 76 0C 8D 48 04 E8" ) ).resolve_rip ( ).get<uintptr_t (* )( )> ( )()+4);
	char outfile [ MAX_PATH ];

	const auto definition_name = static_data ( )->get_definition_name ( );
	const auto custom_paintkit_index = vfunc<int(__thiscall*)(void*)> ( this, 1 )( this );

	std::string paintkit_name;

	for ( auto& skin : features::skinchanger::skin_kits ) {
		if ( skin.id == custom_paintkit_index ) {
			paintkit_name = skin.image_name;
			break;
		}
	}

	float wear = 0.0f;
	const auto has_wear = vfunc<bool ( __thiscall* )( void*, float& )> ( this, 3 )( this, wear );
	
	build_inventory_image_path ( outfile, sizeof( outfile ), definition_name,
		custom_paintkit_index == 0 ? nullptr : paintkit_name.c_str(),
		has_wear, wear,
		false );

	return outfile;
}

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
	//auto v15 = reinterpret_cast< uint32_t* >( features::inventory::get_item_schema ( ) );
	//auto v16 = *reinterpret_cast< uint32_t* > ( v15 [ 72 ] + 4 * index );
	//
	//using fn = int ( __thiscall* )( c_econ_item*, uint32_t, void* );
	//static auto set_dynamic_attribute_val = pattern::search (_( "client.dll"),_( "55 8B EC 83 E4 F8 83 EC 3C 53 8B 5D 08 56 57 6A 00") ).get< fn > ( );
	//
	//set_dynamic_attribute_val ( this, v16, &val );
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
	static auto get_soc_data = pattern::search ( _("client.dll"), _("55 8B EC 83 E4 F0 83 EC 18 56 8B F1 57 8B 86") ).get<fn> ( );
	return get_soc_data ( this );
}

weapon_info_t* weapon_t::data( ) {
	static auto weapon_system = pattern::search( _( "client.dll" ), _( "8B 35 ? ? ? ? FF 10 0F B7 C0" ) ).add( 2 ).deref( ).get< void* >( );
	using ffn = weapon_info_t * ( __thiscall* )( void*, weapons_t );
	return vfunc< ffn >( weapon_system, 2 )( weapon_system, item_definition_index( ) );
}

weapon_t* weapon_t::world_mdl ( ) {
	return cs::i::ent_list->get_by_handle<weapon_t*>( world_model_handle() );
}

float weapon_t::inaccuracy( ) {
	using fn = float( __thiscall* )( void* );
	return vfunc< fn >( this, 483 )( this );
}

float weapon_t::spread( ) {
	using fn = float( __thiscall* )( void* );
	return vfunc< fn >( this, 453 )( this );
}

float weapon_t::max_speed( ) {
	using fn = float( __thiscall* )( void* );
	return vfunc< fn >( this, 442 )( this );
}