#pragma once
#include "../sdk/sdk.hpp"

#include <optional>

#include "kit_parser.hpp"

namespace features {
	namespace inventory {
		class c_socache {
		public:
			virtual void vt_pad ( ) = 0;
			virtual void add_obj ( void* obj ) = 0;
			virtual void vt_pad1 ( ) = 0;
			virtual void rem_obj ( void* obj ) = 0;
		};

		class c_inventory {
		public:
			uint32_t& steam_id ( ) {
				return *reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( this ) + 0x8 );
			}

			c_econ_item* item_in_loadout ( int team, int slot ) {
				using fn = c_econ_item * ( __thiscall* )( c_inventory*, uint32_t, int );
				return vfunc<fn> ( this, 8 )( this, team, slot );
			}

			bool add_econ_item ( c_econ_item* econ_item );
			void remove_item ( c_econ_item* econ_item );
		};

		c_socache* get_so_cache ( );
		c_econ_item* create_econ_item ( );
		void* get_item_schema ( );
		c_inventory* get_local_inventory ( );
	}

	namespace skinchanger {
		std::vector<uint8_t> skin_preview ( const std::string& file );

		class c_sticker {
		public:
			int m_paintkit;
			float m_wear;
		};

		class c_skin;

		inline void erase_items ( );
		inline void add_item ( c_skin skin );

		class c_skin {
		public:
			~c_skin ( ) {
				//release_preview ( );
			}

			/* main stuff */
			bool m_stattrak = false;
			weapons_t m_item_definition_index = weapons_t::none;
			int m_stattrak_counter = 0, m_paintkit = 0, m_seed = 0;
			float m_wear = 0.0f;
			std::vector<c_sticker> m_stickers {};

		//private:
			IDirect3DTexture9* m_cached_preview = nullptr;
			std::vector<uint8_t> m_preview_data {};
			paint_kit* m_cached_kit = nullptr;

		public:
			/* misc stuff (only used internally *NOT TO BE USED*) */
			bool m_equipped_t = false, m_equipped_ct = false;
			c_econ_item* m_item = nullptr;
			
			void remove ( );
			void equip ( );

			void cache ( ) {
				std::string weapon_identifier;

				/* fix gloves */
				if ( m_item_definition_index >= weapons_t::glove_studded_bloodhound ) {
					switch ( m_item_definition_index ) {
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
					const auto weapon_info = vfunc< fn > ( weapon_system, 2 )( weapon_system, m_item_definition_index );

					weapon_identifier = weapon_info->m_weapon_name;

					/* fix revolver */
					if ( m_item_definition_index == weapons_t::revolver ) {
						weapon_identifier = _ ( "weapon_revolver" );
					}
					/* fix silenced weapons */
					else if ( weapon_info->m_silencer ) {
						switch ( m_item_definition_index ) {
						case weapons_t::m4a1s: weapon_identifier = _ ( "weapon_m4a1_silencer" ); break;
						case weapons_t::usps: weapon_identifier = _ ( "weapon_usp_silencer" ); break;
						default: weapon_identifier = _ ( "<SILENCED_UNKNOWN>" ); break;
						}
					}
					/* fix knives */
					else if ( m_item_definition_index >= weapons_t::knife_bayonet ) {
						switch ( m_item_definition_index ) {
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
				for ( auto& paintkit : ( (m_paintkit < 10000) ? skin_kits : glove_kits ) ) {
					if ( paintkit.id == m_paintkit ) {
						m_cached_kit = &paintkit;
						m_preview_data = skin_preview ( std::string( _ ( "resource/flash/econ/default_generated/" )).append( weapon_identifier ).append(_("_")).append( paintkit.image_name ).append(_("_light_large.png")) );
						
						return;
					}
				}
			}

			IDirect3DTexture9* get_preview ( ) {
				if ( !m_cached_preview ) {
					cache ( );
					build_preview ( );
				}

				return m_cached_preview;
			}

			void release_preview ( ) {
				if ( m_cached_preview ) {
					m_cached_preview->Release ( );
					m_cached_preview = nullptr;
				}
			}

			void build_preview ( ) {
				D3DXCreateTextureFromFileInMemoryEx ( cs::i::dev, m_preview_data.data ( ), m_preview_data.size ( ), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &m_cached_preview );
				//D3DXCreateTextureFromFileInMemory ( csgo::i::dev, m_preview_data.data ( ), m_preview_data.size ( ), &m_cached_preview );
			}

			paint_kit* get_kit ( ) {
				if ( !m_cached_kit )
					cache ( );

				return m_cached_kit;
			}
		};

		/* list of all current items */
		inline std::vector<c_skin> skins;

		inline void erase_items ( ) {
			/* unequip and remove all skins from inventory/skin list */
			for ( auto& skin : skins ) {
				/* unequip weapon first */
				skin.m_equipped_ct = skin.m_equipped_t = false;
				skin.equip ( );

				/* remove weapon from inventory */
				inventory::get_local_inventory ( )->remove_item ( skin.m_item );

				skin.release_preview ( );
			}

			/* clear skins list */
			skins.clear ( );
		}

		inline void add_item ( c_skin skin ) {
			static uint32_t last_skin_index = 0;

			const auto inventory = inventory::get_local_inventory ( );
			const auto item = inventory::create_econ_item ( );

			skin.m_item = item;
			skin.m_stattrak_counter = skin.m_stattrak ? skin.m_stattrak_counter : -1;
			skin.m_wear = std::clamp ( skin.m_wear, 0.0001f, 1.0f );
			skin.m_equipped_t = skin.m_equipped_ct = false;

			/* karambit as test item */
			item->account_id ( ) = inventory->steam_id ( );
			item->item_definition_index ( ) = skin.m_item_definition_index;
			item->item_id ( ) = 20000 + static_cast< uint64_t >( last_skin_index );
			item->inventory ( ) = 1;
			item->flags ( ) = 0;
			item->original_id ( ) = 0;

			if( skin.m_stattrak_counter >=0)
				item->set_stattrak ( skin.m_stattrak_counter );

			item->set_paintkit ( skin.m_paintkit );
			item->set_paint_seed ( skin.m_seed );
			item->set_paint_wear ( skin.m_wear );

			auto sticker_index = 0;

			for ( auto& sticker : skin.m_stickers )
				item->add_sticker ( sticker_index++, sticker.m_paintkit, sticker.m_wear, 1.0f, 1.0f );

			item->set_origin ( 8 );
			item->set_level ( 1 );
			item->set_in_use ( false );

			if ( skin.m_item_definition_index >= weapons_t::knife_bayonet && skin.m_item_definition_index <= weapons_t::knife_skeleton )
				item->set_quality ( 3 );
			else
				item->set_rarity ( 6 );

			item->set_custom_name ( (char*)_(u8"✨ sesame.one ✨") );

			item->clean_inventory_image_cache_dir ( );

			inventory->add_econ_item ( item );
			
			//skin.equip ( );
			skin.cache ( );
			skin.build_preview ( );

			skins.push_back ( skin );

			last_skin_index++;
		}
		
		struct sequence_mapping {
			int m_num;
			std::string m_name;
		};

		inline std::unordered_map<weapons_t, std::vector<sequence_mapping>> knife_sequences;

		inline void release_previews ( ) {
			for ( auto& item : skins )
				item.release_preview ( );
		}

		inline void rebuild_previews ( ) {
			for ( auto& item : skins )
				item.build_preview ( );
		}

		void dump_sequences ( );
		int remap_sequence ( weapons_t from_item, weapons_t to_item, int from_sequence );
		void update_equipped ( );
		void run ( );
		void init ( );
		void process_death ( event_t* event );
	}
}