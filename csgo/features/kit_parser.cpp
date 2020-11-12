/* This file is part of nSkinz by namazso, licensed under the MIT license:
*
* MIT License
*
* Copyright (c) namazso 2018
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include "kit_parser.hpp"
#include "../sdk/sdk.hpp"

#include <windows.h>
#include <algorithm>

class CCStrike15ItemSchema;
class CCStrike15ItemSystem;

template <typename Key, typename Value>
struct Node_t
{
	int previous_id;		//0x0000
	int next_id;			//0x0004
	void* _unknown_ptr;		//0x0008
	int _unknown;			//0x000C
	Key key;				//0x0010
	Value value;			//0x0014
};

template <typename Key, typename Value>
struct Head_t
{
	Node_t<Key, Value>* memory;		//0x0000
	int allocation_count;			//0x0004
	int grow_size;					//0x0008
	int start_element;				//0x000C
	int next_available;				//0x0010
	int _unknown;					//0x0014
	int last_element;				//0x0018
}; //Size=0x001C

// could use CUtlString but this is just easier and CUtlString isn't needed anywhere else
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

struct CStickerKit
{
	int id;

	int item_rarity;

	String_t name;
	String_t description;
	String_t item_name;
	String_t material_name;
	String_t image_inventory;

	int tournament_event_id;
	int tournament_team_id;
	int tournament_player_id;
	bool is_custom_sticker_material;

	float rotate_end;
	float rotate_start;

	float scale_min;
	float scale_max;

	float wear_min;
	float wear_max;

	String_t image_inventory2;
	String_t image_inventory_large;

	uint32_t pad0 [ 4 ];
};

template < typename t >
t create_interface ( const char* module, const char* iname ) {
	using createinterface_fn = void* ( __cdecl* )( const char*, int );
	const auto createinterface_export = LI_FN ( GetProcAddress )( LI_FN ( GetModuleHandleA )( module ), _ ( "CreateInterface" ) );
	const auto fn = ( createinterface_fn ) createinterface_export;

	return reinterpret_cast< t >( fn ( iname, 0 ) );
}

void features::skinchanger::dump_kits ( )
{
	const auto localize = create_interface< void* > ( _("localize.dll"), _("Localize_001") );
	const auto V_UCS2ToUTF8 = reinterpret_cast< int( * )( const wchar_t*, char*, int ) >( LI_FN( GetProcAddress )( LI_FN( GetModuleHandleA ) ( _ ( "vstdlib.dll" ) ),_( "V_UCS2ToUTF8" )) );
	
	auto pattern = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? FF 76 0C 8D 48 04 E8" ) );
	
	const auto item_system_fn = pattern.resolve_rip ( ).get<CCStrike15ItemSystem * ( * )( )> ( );
	const auto item_schema = reinterpret_cast< CCStrike15ItemSchema* >( uintptr_t ( item_system_fn ( ) ) + sizeof ( void* ) );

	// Dump paint kits
	{
		const auto get_paint_kit_definition_fn = pattern.add(11).resolve_rip ( ).get<CPaintKit * ( __thiscall* )( CCStrike15ItemSchema*, int )> ( );

		const auto start_element_offset = *reinterpret_cast< intptr_t* >( uintptr_t ( get_paint_kit_definition_fn ) + 8 + 2 );

		const auto head_offset = start_element_offset - 12;

		const auto map_head = reinterpret_cast< Head_t<int, CPaintKit*>* >( uintptr_t ( item_schema ) + head_offset );

		for ( auto i = 0; i <= map_head->last_element; ++i ) {
			const auto paint_kit = map_head->memory [ i ].value;

			if ( paint_kit->id == 9001 )
				continue;

			const auto wide_name = vfunc< wchar_t* ( __thiscall* )( void*, const char* ) > ( localize, 11 )( localize, paint_kit->item_name.buffer + 1 );
			char name [ 256 ];
			V_UCS2ToUTF8 ( wide_name, name, sizeof ( name ) );

			if ( paint_kit->id < 10000 ) {
				skin_kits.push_back ( { paint_kit->id, name, std::string ( _ ( "resource/flash/econ/default_generated/weapon_" ) ).append ( paint_kit->name.buffer ).append ( _ ( "_light_large.png" ) ), *reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( paint_kit ) + 101 ),* reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( paint_kit ) + 105 ) } );
			}
			else {
				std::string base_dir;

				//DRIVER GLOVES MISSING

				if ( strstr ( paint_kit->name.buffer, _ ( "sporty_" ) ) )
					base_dir = _ ( "resource/flash/econ/default_generated/sporty_gloves_" );
				else if ( strstr ( paint_kit->name.buffer, _ ( "hydra_" ) ) )
					base_dir = _ ( "resource/flash/econ/default_generated/studded_hydra_gloves_" );
				else if ( strstr ( paint_kit->name.buffer, _ ( "bloodhound_" ) ) )
					base_dir = _ ( "resource/flash/econ/default_generated/studded_bloodhound_gloves_" );
				else if ( strstr ( paint_kit->name.buffer, _ ( "handwrap_" ) ) )
					base_dir = _ ( "resource/flash/econ/default_generated/leather_handwraps_" );
				else if ( strstr ( paint_kit->name.buffer, _ ( "motorcycle_" ) ) )
					base_dir = _ ( "resource/flash/econ/default_generated/motorcycle_gloves_" );
				else if ( strstr ( paint_kit->name.buffer, _ ( "specialist_" ) ) )
					base_dir = _ ( "resource/flash/econ/default_generated/specialist_gloves_" );
				else if ( strstr ( paint_kit->name.buffer, _ ( "slick_" ) ) /* driver gloves???? */)
					base_dir = _ ( "resource/flash/econ/default_generated/slick_gloves_" );

				glove_kits.push_back ( { paint_kit->id, name, base_dir.append ( paint_kit->name.buffer ).append ( _ ( "_light_large.png" ) ), *reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( paint_kit ) + 101 ),* reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( paint_kit ) + 105 ) } );
			}
		}
		
		std::sort ( skin_kits.begin ( ), skin_kits.end ( ) );
		std::sort ( glove_kits.begin ( ), glove_kits.end ( ) );
	}

	// Dump sticker kits
	{
		auto sticker_sig = pattern::search ( _ ( "client.dll" ),_( "53 8D 48 04 E8 ? ? ? ? 8B 4D 10") );

		const auto get_sticker_kit_definition_fn = sticker_sig.add(4).resolve_rip().get<CPaintKit * ( __thiscall* )( CCStrike15ItemSchema*, int )>();

		const auto start_element_offset = *reinterpret_cast< intptr_t* >( uintptr_t ( get_sticker_kit_definition_fn ) + 8 + 2 );

		// Calculate head base from start_element's offset
		const auto head_offset = start_element_offset - 12;

		const auto map_head = reinterpret_cast< Head_t<int, CStickerKit*>* >( uintptr_t ( item_schema ) + head_offset );

		for ( auto i = 0; i <= map_head->last_element; ++i ) {
			const auto sticker_kit = map_head->memory [ i ].value;

			char sticker_name_if_valve_fucked_up_their_translations [ 64 ];

			auto sticker_name_ptr = sticker_kit->item_name.buffer + 1;

			if ( sticker_name_ptr [ 0 ] == 'S'
				&& sticker_name_ptr [ 7 ] == 'K'
				&& sticker_name_ptr [ 11 ] == 'd'
				&& sticker_name_ptr [ 19 ] == 'd'
				&& sticker_name_ptr [ 26 ] == 's'
				&& sticker_name_ptr [ 22 ] == 'n'
				/* strstr(sticker_name_ptr, _("StickerKit_dhw2014_dignitas")) */ ) {
				strcpy_s ( sticker_name_if_valve_fucked_up_their_translations, _("StickerKit_dhw2014_teamdignitas") );
				strcat_s ( sticker_name_if_valve_fucked_up_their_translations, sticker_name_ptr + 27 );
				sticker_name_ptr = sticker_name_if_valve_fucked_up_their_translations;
			}

			const auto wide_name = vfunc< wchar_t* ( __thiscall* )( void*, const char* ) > ( localize, 11 )( localize, sticker_name_ptr );
			char name [ 256 ];
			V_UCS2ToUTF8 ( wide_name, name, sizeof ( name ) );

			sticker_kits.push_back ( { sticker_kit->id, name, std::string( _ ( "resource/flash/econ/stickers/" ) ).append(sticker_kit->material_name.buffer).append(_("_large.png")), sticker_kit->item_rarity, 0 } );
		}

		std::sort ( sticker_kits.begin ( ), sticker_kits.end ( ));

		sticker_kits.insert ( sticker_kits.begin ( ), { 0,_( "None" )} );
	}
}