#pragma once
#include "../sdk/sdk.hpp"

#include <optional>

#include "kit_parser.hpp"

namespace features {
	namespace skinchanger {
		std::vector< uint8_t > skin_preview ( const std::string& file );

		inline IDirect3DTexture9* get_preview ( weapons_t item_id, int paintkit ) {
			std::vector< uint8_t > raw_preview {};

			std::string weapon_identifier;

			/* fix gloves */
			if ( item_id >= weapons_t::glove_studded_bloodhound ) {
				switch ( item_id ) {
				case weapons_t::glove_ct_side:
				case weapons_t::glove_t_side: weapon_identifier = _ ( "<GLOVES_UNKNOWN>" ); break;
				case weapons_t::glove_studded_bloodhound: weapon_identifier = _ ( "studded_bloodhound_gloves" ); break;
				case weapons_t::glove_sporty: weapon_identifier = _ ( "sporty_gloves" ); break;
				case weapons_t::glove_slick: weapon_identifier = _ ( "slick_gloves" ); break;
				case weapons_t::glove_leather_wrap: weapon_identifier = _ ( "leather_handwraps" ); break;
				case weapons_t::glove_motorcycle: weapon_identifier = _ ( "motorcycle_gloves" ); break;
				case weapons_t::glove_specialist: weapon_identifier = _ ( "specialist_gloves" ); break;
				case weapons_t::glove_studded_hydra: weapon_identifier = _ ( "studded_hydra_gloves" ); break;
				default: weapon_identifier = _ ( "<GLOVES_UNKNOWN>" ); break;
				}
			}
			else {
				static auto weapon_system = pattern::search ( _ ( "client.dll" ), _ ( "8B 35 ? ? ? ? FF 10 0F B7 C0" ) ).add ( 2 ).deref ( ).get< void* > ( );
				using fn = weapon_info_t * ( __thiscall* )( void*, weapons_t );
				const auto weapon_info = vfunc< fn > ( weapon_system, 2 )( weapon_system, item_id );

				weapon_identifier = weapon_info->m_weapon_name;

				/* fix revolver */
				if ( item_id == weapons_t::revolver ) {
					weapon_identifier = _ ( "weapon_revolver" );
				}
				/* fix silenced weapons */
				else if ( weapon_info->m_silencer ) {
					switch ( item_id ) {
					case weapons_t::m4a1s: weapon_identifier = _ ( "weapon_m4a1_silencer" ); break;
					case weapons_t::usps: weapon_identifier = _ ( "weapon_usp_silencer" ); break;
					default: weapon_identifier = _ ( "<SILENCED_UNKNOWN>" ); break;
					}
				}
				/* fix knives */
				else if ( item_id >= weapons_t::knife_bayonet ) {
					switch ( item_id ) {
					case weapons_t::knife_ct:
					case weapons_t::knife_t: break;
					case weapons_t::knife_bayonet: weapon_identifier = _ ( "weapon_bayonet" ); break;
					case weapons_t::knife_bowie: weapon_identifier.append ( _ ( "_bowie" ) ); break;
					case weapons_t::knife_butterfly: weapon_identifier.append ( _ ( "_butterfly" ) ); break;
					case weapons_t::knife_canis: weapon_identifier.append ( _ ( "_canis" ) ); break;
					case weapons_t::knife_cord: weapon_identifier.append ( _ ( "_cord" ) ); break;
					case weapons_t::knife_css: weapon_identifier.append ( _ ( "_css" ) ); break;
					case weapons_t::knife_falchion: weapon_identifier.append ( _ ( "_falchion" ) ); break;
					case weapons_t::knife_flip: weapon_identifier.append ( _ ( "_flip" ) ); break;
					case weapons_t::knife_gut: weapon_identifier.append ( _ ( "_gut" ) ); break;
					case weapons_t::knife_gypsy_jackknife: weapon_identifier.append ( _ ( "_gypsy_jackknife" ) ); break;
					case weapons_t::knife_huntsman: weapon_identifier.append ( _ ( "_tactical" ) ); break;
					case weapons_t::knife_karambit:weapon_identifier.append ( _ ( "_karambit" ) ); break;
					case weapons_t::knife_m9_bayonet: weapon_identifier.append ( _ ( "_m9_bayonet" ) ); break;
					case weapons_t::knife_outdoor: weapon_identifier.append ( _ ( "_outdoor" ) ); break;
					case weapons_t::knife_shadow_daggers:weapon_identifier.append ( _ ( "_push" ) ); break;
					case weapons_t::knife_skeleton: weapon_identifier.append ( _ ( "_skeleton" ) ); break;
					case weapons_t::knife_stiletto: weapon_identifier.append ( _ ( "_stiletto" ) ); break;
					case weapons_t::knife_ursus: weapon_identifier.append ( _ ( "_ursus" ) ); break;
					case weapons_t::knife_widowmaker: weapon_identifier.append ( _ ( "_widowmaker" ) ); break;
					default: weapon_identifier = _ ( "<KNIFE_UNKNOWN>" ); break;
					}
				}
			}

			/* paintkit < 10000 is weapon skin, else is glove skin */
			for ( auto& iter : ( ( paintkit < 10000 ) ? skin_kits : glove_kits ) ) {
				if ( iter.id == paintkit ) {
					raw_preview = skin_preview ( std::string ( _ ( "resource/flash/econ/default_generated/" ) ).append ( weapon_identifier ).append ( _ ( "_" ) ).append ( iter.image_name ).append ( _ ( "_light_large.png" ) ) );
					break;
				}
			}

			if ( raw_preview.empty ( ) )
				return nullptr;

			IDirect3DTexture9* ret = nullptr;
			D3DXCreateTextureFromFileInMemoryEx ( cs::i::dev, raw_preview.data ( ), raw_preview.size ( ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &ret );

			return ret;
		}

		struct sequence_mapping {
			int m_num;
			std::string m_name;
		};

		inline std::unordered_map< weapons_t, std::vector< sequence_mapping > > knife_sequences;

		void dump_sequences ( );
		int remap_sequence ( weapons_t from_item, weapons_t to_item, int from_sequence );
		void run ( );
		void init ( );
		void process_death ( event_t* event );
	}
}