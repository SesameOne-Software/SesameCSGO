#pragma once
#include "../sdk/sdk.hpp"

#include <optional>

#include "kit_parser.hpp"

enum weapons_t : int {
	deagle = 1,
	elite = 2,
	fiveseven = 3,
	glock = 4,
	ak47 = 7,
	aug = 8,
	awp = 9,
	famas = 10,
	g3sg1 = 11,
	galil = 13,
	m249 = 14,
	m4a4 = 16,
	mac10 = 17,
	p90 = 19,
	mp5_sd = 23,
	ump45 = 24,
	xm1014 = 25,
	bizon = 26,
	mag7 = 27,
	negev = 28,
	sawedoff = 29,
	tec9 = 30,
	zeus = 31,
	p2000 = 32,
	mp7 = 33,
	mp9 = 34,
	nova = 35,
	p250 = 36,
	scar20 = 38,
	sg553 = 39,
	ssg08 = 40,
	knife_ct = 42,
	flashbang = 43,
	hegrenade = 44,
	smoke = 45,
	molotov = 46,
	decoy = 47,
	firebomb = 48,
	c4 = 49,
	musickit = 58,
	knife_t = 59,
	m4a1s = 60,
	usps = 61,
	tradeupcontract = 62,
	cz75a = 63,
	revolver = 64,
	knife_bayonet = 500,
	knife_css = 503,
	knife_flip = 505,
	knife_gut = 506,
	knife_karambit = 507,
	knife_m9_bayonet = 508,
	knife_huntsman = 509,
	knife_falchion = 512,
	knife_bowie = 514,
	knife_butterfly = 515,
	knife_shadow_daggers = 516,
	knife_cord = 517,
	knife_canis = 518,
	knife_ursus = 519,
	knife_gypsy_jackknife = 520,
	knife_outdoor = 521,
	knife_stiletto = 522,
	knife_widowmaker = 523,
	knife_skeleton = 525,
};

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
		std::vector<uint8_t>& skin_preview ( const std::string& file );

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
			/* main stuff */
			bool m_stattrak = false;
			uint16_t m_item_definition_index = 0;
			int m_stattrak_counter = 0, m_paintkit = 0, m_seed = 0;
			float m_wear = 0.0f;
			std::vector<c_sticker> m_stickers {};

		private:
			std::vector<uint8_t>* m_cached_preview = nullptr;
			paint_kit* m_cached_kit = nullptr;

		public:

			/* misc stuff (only used internally *NOT TO BE USED*) */
			bool m_equipped_t = false, m_equipped_ct = false;
			c_econ_item* m_item = nullptr;

			void remove ( );
			void equip ( );

			void add ( ) {
				add_item ( *this );
				equip ( );
				cache ( );
			}

			void cache ( ) {
				/* paintkit < 10000 is weapon skin, else is glove skin */
				for ( auto& paintkit : ( m_paintkit < 10000 ? skin_kits : glove_kits ) ) {
					if ( paintkit.id == m_paintkit ) {
						m_cached_kit = &paintkit;
						m_cached_preview = skin_preview ( paintkit.image_name );
						return;
					}
				}
			}

			std::optional<std::vector<uint8_t>&> get_preview ( ) {
				if ( !m_cached_preview )
					cache ( );

				return m_cached_preview;
			}

			std::optional<paint_kit&> get_kit ( ) {
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

			if ( skin.m_item_definition_index >= knife_bayonet && skin.m_item_definition_index <= knife_skeleton )
				item->set_quality ( 3 );
			else
				item->set_rarity ( 6 );

			item->clean_inventory_image_cache_dir ( );

			inventory->add_econ_item ( item );

			skins.push_back ( skin );

			last_skin_index++;
		}
		
		struct sequence_mapping {
			int m_num;
			std::string m_name;
		};

		inline std::unordered_map<int, std::vector<sequence_mapping>> knife_sequences;

		void dump_sequences ( );
		int remap_sequence ( int from_item, int to_item, int from_sequence );
		void update_equipped ( );
		void run ( );
		void init ( );
	}
}