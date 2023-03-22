#include "skinchanger.hpp"

#include "kit_parser.hpp"
#include "../vpk/vpkparser.hpp"
#include "../menu/options.hpp"

int features::skinchanger::remap_sequence ( weapons_t from_item, weapons_t to_item, int from_sequence ) {
	auto clean_str = [ ] ( std::string& str ) -> void {
		/* search for 2char number, ex: 06 */
		if ( isdigit ( str [ str.size ( ) - 2 ] ) )
			str.erase ( str.size ( ) - 2 );
		/* search for 1char number, ex: 6 */
		else if ( isdigit ( str [ str.size ( ) - 1 ] ) )
			str.erase ( str.size ( ) - 1 );
	};

	auto rand_int = [ ] ( int min, int max ) -> int {
		return rand ( ) % ( ( max - min ) + 1 ) + min;
	};

	std::string cur_sequence_generic;

	/* get current sequence name, map onto corresponding sequence of new weapon */
	for ( auto& sequence : knife_sequences [ from_item ] ) {
		if ( sequence.m_num == from_sequence ) {
			cur_sequence_generic = sequence.m_name;
			break;
		}
	}

	if ( cur_sequence_generic.empty ( ) )
		return from_sequence;

	clean_str ( cur_sequence_generic );

	std::vector<int> possible_remapped_sequences { };

	for ( auto& sequence : knife_sequences [ to_item ] ) {
		if ( sequence.m_name.find ( cur_sequence_generic ) != std::string::npos )
			possible_remapped_sequences.push_back ( sequence.m_num );
	}

	/* randomly remap to one of the possible sequences */
	if ( possible_remapped_sequences.empty ( ) )
		return from_sequence;
	
	return possible_remapped_sequences [ rand_int( 0,possible_remapped_sequences.size() - 1) ];
}

bool precache_model ( const char* mdl_name ) {
	const auto mdl_precache_table = cs::i::client_string_table_container->find_table ( _ ( "modelprecache" ) );

	if ( mdl_precache_table ) {
		cs::i::mdl_info->find_or_load_mdl ( mdl_name );

		if ( mdl_precache_table->add_string ( false, mdl_name ) == -1 )
			return false;
	}

	return true;
}

void* find_hud_element_skinchanger ( const char* name ) {
	static auto hud = pattern::search ( _ ( "client.dll" ), _ ( "B9 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89 46 24" ) ).add ( 1 ).deref ( ).get< void* > ( );
	static auto find_hud_element_func = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28" ) ).get< void* ( __thiscall* )( void*, const char* ) > ( );
	return ( void* ) find_hud_element_func ( hud, name );
}

void features::skinchanger::run ( ) {
	VMP_BEGINMUTATION ( );
	static auto& override_knife = options::skin_vars [ _ ( "skins.skin.override_knife" ) ].val.b;
	static auto& override_weapon = options::skin_vars [ _ ( "skins.skin.override_weapon" ) ].val.b;
	static auto& selected_knife = options::skin_vars [ _ ( "skins.skin.knife" ) ].val.i;
	//static auto& player_model_t = options::skin_vars [ _ ( "skins.models.player_model_t" ) ].val.i;
	//static auto& player_model_ct = options::skin_vars [ _ ( "skins.models.player_model_ct" ) ].val.i;

	/* precache custom models */
	static bool was_in_game = false;
	static bool was_dead = true;
	//static int backup_player_mdl_idx = 0;
	static int backup_knife_model = selected_knife;
	//static auto last_player_model_t = player_model_t;
	//static auto last_player_model_ct = player_model_ct;

	if ( cs::i::engine->is_in_game ( ) && !was_in_game ) {
		//precache_model ( _ ( "models/player/custom_player/legacy/tm_jumpsuit_variantb.mdl" ) );
		was_dead = true;
	}

	was_in_game = cs::i::engine->is_in_game ( );

	/* skinchanger */
	static auto C_BaseViewModel__SendViewModelMatchingSequence = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 56 FF 75 08 8B F1 8B 06 FF 90 ? ? ? ? 8B 86" ) ).get<void ( __thiscall* )( void*, int )> ( );
	static auto CBaseEntity__GetSequenceActivity = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 53 8B 5D 08 56 8B F1 83" ) ).get<int ( __thiscall* )( void*, int )> ( );

	static auto fnEquip = pattern::search (_( "client.dll"),_( "55 8B EC 83 EC 10 53 8B 5D 08 57 8B F9") ).get<int ( __thiscall* )( void*, void* )>();
	static auto fnInitializeAttributes = pattern::search ( _("client.dll"),_( "55 8B EC 83 E4 F8 83 EC 0C 53 56 8B F1 8B 86") ).get<int ( __thiscall* )( void* )>();

	static auto g_ClientLeafSystem = pattern::search ( "client.dll", "A1 ? ? ? ? FF 50 14 8B 86 ? ? ? ? B9 ? ? ? ? C1 E8 0E 24 01 0F B6 C0 50 0F B7 86 ? ? ? ? 50 A1 ? ? ? ? FF 50 18 8B 86 ? ? ? ? B9 ? ? ? ? C1 E8 0C 24 01 0F B6 C0 50 0F B7 86 ? ? ? ? 50 A1 ? ? ? ? FF 90 ? ? ? ? 8B 0D" ).add(1).deref().get<void*>();
	
	static auto clear_hud_weapon_icon = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 51 53 56 8B 75 08 8B D9 57 6B FE 34" ) ).get<int ( __thiscall* )( void*, int )> ( );

	if ( !g::local || !g::local->alive ( ) ) {
		//backup_player_mdl_idx = 0;

		//last_player_model_t = player_model_t;
		//last_player_model_ct = player_model_ct;
		backup_knife_model = selected_knife;

		was_dead = false;
		skin_changed = false;

		//was_dead = true;
		return;
	}

	/* start skinchanger */
	player_info_t player_info;
	cs::i::engine->get_player_info ( g::local->idx( ), &player_info );

	const auto cur_weapon = g::local->weapon ( );
	const auto weapons = g::local->weapons ( );

	for ( auto& weapon : weapons ) {
		if ( !weapon || !weapon->data ( ) )
			continue;
		
		const auto item_definition_index = weapon->item_definition_index ( );
		const auto is_knife = item_definition_index == weapons_t::knife_t || item_definition_index == weapons_t::knife_ct || ( item_definition_index >= weapons_t::knife_bayonet && item_definition_index <= weapons_t::knife_skeleton );

		auto cur_gun_skin = options::skin_vars.find ( std::string ( _ ( "skins.skin." ) ).append ( weapon->data ( )->m_weapon_name ) );

		/* knife changer */
		if ( override_knife && is_knife ) {
			const auto item_definition_index = weapon->item_definition_index ( );
			const auto wanted_item_definition_index = knife_ids [ selected_knife ];

			weapon->item_id_high ( ) = -1;
			weapon->account ( ) = player_info.m_xuid_low;

			if ( cur_gun_skin != options::skin_vars.end ( ) ) {
				auto& skin = cur_gun_skin->second.val.skin;

				weapon->fallback_stattrak ( ) = skin.is_stattrak ? skin.stattrak : -1;
				weapon->fallback_paintkit ( ) = skin.paintkit;
				weapon->fallback_seed ( ) = skin.seed;
				weapon->fallback_wear ( ) = std::clamp ( ( 100.0f - skin.wear ) / 100.0f, 0.0000001f, 1.0f );

				//if ( skin.nametag [ 0 ] )
				//	memcpy ( weapon->name ( ), skin.nametag, sizeof ( skin.nametag ) );
			}

			/* do extra stuff if it's a knife */
			weapon->item_definition_index ( ) = wanted_item_definition_index;

			const auto new_mdl_idx = cs::i::mdl_info->mdl_idx ( knife_models [ selected_knife ] );

			weapon->mdl_idx ( ) = new_mdl_idx;

			const auto world_mdl = weapon->world_mdl ( );

			if ( world_mdl )
				world_mdl->mdl_idx ( ) = new_mdl_idx + 1;

			/* if viewmodel is a knife */
			const auto view_mdl = cs::i::ent_list->get_by_handle< weapon_t* > ( g::local->viewmodel_handle ( ) );

			if ( view_mdl && cur_weapon ) {
				const auto item_id = cur_weapon->item_definition_index ( );

				if ( item_id == weapons_t::knife_t || item_id == weapons_t::knife_ct || ( item_id >= weapons_t::knife_bayonet && item_id <= weapons_t::knife_skeleton ) ) {
					/* set current viewmodel model index if we are on knife viewmodel */
					view_mdl->mdl_idx ( ) = new_mdl_idx;

					static int last_sequence = 0;

					/* replace knife animations with different ones right after update */
					if ( last_sequence != view_mdl->sequence ( ) )
						/* grab animations from non-default knife with same animation sequences */
						last_sequence = view_mdl->sequence ( ) = remap_sequence ( weapons_t::knife_bayonet, knife_ids [ selected_knife ], view_mdl->sequence ( ) );
				}
			}
		}
		/* weapon skin changer */
		else if ( override_weapon && !is_knife ) {
			if ( cur_gun_skin != options::skin_vars.end( ) ) {
				auto& skin = cur_gun_skin->second.val.skin;

				weapon->item_id_high ( ) = -1;
				weapon->account ( ) = player_info.m_xuid_low;
				weapon->fallback_stattrak ( ) = skin.is_stattrak ? skin.stattrak : -1;
				weapon->fallback_paintkit ( ) = skin.paintkit;
				weapon->fallback_seed ( ) = skin.seed;
				weapon->fallback_wear ( ) = std::clamp( ( 100.0f - skin.wear ) / 100.0f, 0.0000001f, 1.0f );
				
				//if ( skin.nametag [ 0 ] )
				//	memcpy ( weapon->name ( ), skin.nametag, sizeof ( skin.nametag ) );
			}
		}
	}

	//
	///* glove changer */
	//auto get_wearable_create_fn = [ ] ( ) -> create_client_class_t {
	//	auto clazz = cs::i::client->get_all_classes ( );
	//
	//	while ( strcmp ( clazz->m_network_name, _ ( "CEconWearable" ) ) )
	//		clazz = clazz->m_next;
	//
	//	return clazz->m_create_fn;
	//};
	//
	//static auto wearables_offset = netvars::get_offset ( _ ( "DT_BaseCombatCharacter->m_hMyWearables" ) );
	//
	//auto wearables = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(g::local) + wearables_offset );
	//
	//if ( !cs::i::ent_list->get_by_handle<weapon_t*> ( wearables [ 0 ] ) ) {
	//	auto skin_data = inventory->item_in_loadout ( g::local->team ( ), 41 );
	//
	//	if ( skin_data ) {
	//		auto entry = cs::i::ent_list->get_highest_index ( ) + 1;
	//
	//		for ( int i = 0; i < cs::i::ent_list->get_highest_index ( ); i++ ) {
	//			auto temp_ent = cs::i::ent_list->get<entity_t*> ( i );
	//
	//			if ( temp_ent && temp_ent->client_class ( ) && !strcmp ( temp_ent->client_class ( )->m_network_name, _ ( "CRopeKeyframe" ) ) ) {
	//				entry = i;
	//				break;
	//			}
	//		}
	//
	//		const auto serial = ( rand ( ) % 4096 ) + 1;
	//		const auto create_wearable_fn = get_wearable_create_fn ( );
	//		create_wearable_fn ( entry, serial );
	//		auto wearable = cs::i::ent_list->get<weapon_t*> ( entry );
	//
	//		if ( wearable ) {
	//			wearables [ 0 ] = entry | serial << 16;
	//
	//			auto econ_data = skin_data->soc_data ( );
	//			auto item_definition = skin_data->static_data ( );
	//
	//			if ( !econ_data || !item_definition )
	//				return;
	//
	//			const auto id = econ_data->item_id ( );
	//
	//			*reinterpret_cast< uint64_t* >( reinterpret_cast< uintptr_t >( &wearable->item_id_high ( ) ) - sizeof ( uint64_t ) ) = id;
	//			wearable->item_id_low ( ) = id & 0xFFFFFFFF;
	//			wearable->item_id_high ( ) = id >> 32;
	//			wearable->account ( ) = player_info.m_xuid_low;
	//
	//			wearable->item_definition_index ( ) = econ_data->item_definition_index ( );
	//			wearable->initialized ( ) = true;
	//			wearable->mdl_idx ( ) = cs::i::mdl_info->mdl_idx ( item_definition->world_mdl_name ( ) );
	//
	//			fnEquip ( wearable, g::local );
	//
	//			g::local->body ( ) = 1;
	//
	//			fnInitializeAttributes ( wearable );
	//
	//			vfunc<void ( __thiscall* )( void*, int, int, bool, int, bool )> ( g_ClientLeafSystem, 0 )( g_ClientLeafSystem, reinterpret_cast< uintptr_t >( wearable ) + 4, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF );
	//		}
	//	}
	//}

	///* model changer */
	//std::vector<std::string> models {
	//	_ ( "Default" ),
	//	_ ( "models/player/custom_player/legacy/ctm_fbi_variantb.mdl" ),
	//	_ ( "models/player/custom_player/legacy/ctm_fbi_variantf.mdl" ),
	//	_ ( "models/player/custom_player/legacy/ctm_fbi_variantg.mdl" ),
	//	_ ( "models/player/custom_player/legacy/ctm_fbi_varianth.mdl" ),
	//	_ ( "models/player/custom_player/legacy/ctm_sas_variantf.mdl" ),
	//	_ ( "models/player/custom_player/legacy/ctm_st6_variante.mdl" ),
	//	_ ( "models/player/custom_player/legacy/ctm_st6_variantg.mdl" ),
	//	_ ( "models/player/custom_player/legacy/ctm_st6_varianti.mdl" ),
	//	_ ( "models/player/custom_player/legacy/ctm_st6_variantk.mdl" ),
	//	_ ( "models/player/custom_player/legacy/ctm_st6_variantm.mdl" ),
	//	_ ( "models/player/custom_player/legacy/tm_balkan_variantf.mdl" ),
	//	_ ( "models/player/custom_player/legacy/tm_balkan_variantg.mdl" ),
	//	_ ( "models/player/custom_player/legacy/tm_balkan_varianth.mdl" ),
	//	_ ( "models/player/custom_player/legacy/tm_balkan_varianti.mdl" ),
	//	_ ( "models/player/custom_player/legacy/tm_balkan_variantj.mdl" ),
	//	_ ( "models/player/custom_player/legacy/tm_leet_variantf.mdl" ),
	//	_ ( "models/player/custom_player/legacy/tm_leet_variantg.mdl" ),
	//	_ ( "models/player/custom_player/legacy/tm_leet_varianth.mdl" ),
	//	_ ( "models/player/custom_player/legacy/tm_leet_varianti.mdl" ),
	//	_ ( "models/player/custom_player/legacy/tm_phoenix_variantf.mdl" ),
	//	_ ( "models/player/custom_player/legacy/tm_phoenix_variantg.mdl" ),
	//	_ ( "models/player/custom_player/legacy/tm_phoenix_varianth.mdl" ),
	//	_ ( "models/player/custom_player/legacy/tm_jumpsuit_variantb.mdl" )
	//};
	//
	///* force update if player model changes */
	//if ( !backup_player_mdl_idx )
	//	backup_player_mdl_idx = g::local->mdl_idx ( );
	//
	//std::string& model = models [ g::local->team ( ) == 2 ? player_model_t : player_model_ct ];
	//
	//const auto new_idx = model == _ ( "Default" ) ? backup_player_mdl_idx : cs::i::mdl_info->mdl_idx ( model.c_str ( ) );
	//
	//g::local->set_model_idx ( new_idx );
	//
	//if ( const auto ragdoll = cs::i::ent_list->get_by_handle< entity_t* > ( g::local->ragdoll_handle ( ) ) )
	//	ragdoll->set_model_idx ( new_idx );

	if ( backup_knife_model != selected_knife || skin_changed ) {
		/* clear weapon icons */
		const auto hud_weapon_selection = reinterpret_cast< int* >( find_hud_element_skinchanger ( _ ( "CCSGO_HudWeaponSelection" ) ) );

		auto hud_weapons = hud_weapon_selection - 0x28;

		for ( auto i = 0; i < *( hud_weapons + 32 ); i++ )
			i = clear_hud_weapon_icon ( hud_weapons, i );

		/* force full update */
		cs::i::client_state->delta_tick ( ) = -1;

		backup_knife_model = selected_knife;

		was_dead = false;
		skin_changed = false;
	}
	VMP_END ( );
}

std::vector<uint8_t> features::skinchanger::skin_preview ( const std::string& file ) {
	static auto vpk_dir = std::string( pattern::search ( _ ( "engine.dll" ), _ ( "68 ? ? ? ? 8D 85 ? ? ? ? 50 68 ? ? ? ? 68" ) ).add ( 1 ).deref ( ).get<const char*> ( )).append(_("\\pak01_dir.vpk"));
	static c_vpk_archive pak01_archive( vpk_dir );

	std::optional<c_vpk_entry> entry = pak01_archive.get_file ( file );

	if ( entry ) {
		std::optional<std::vector<uint8_t>> png_data = entry.value ( ).get_data ( );

		if ( png_data )
			return png_data.value();
	}

	return {};
}

void features::skinchanger::dump_sequences ( ) {
	VMP_BEGINMUTATION ( );
	std::unordered_map< weapons_t, std::string > knife_anim_model_names {
		{ weapons_t::knife_bayonet, "models/weapons/v_knife_bayonet_anim.mdl" },
		{ weapons_t::knife_css,"models/weapons/v_knife_css_anim.mdl" },
		{ weapons_t::knife_flip,"models/weapons/v_knife_flip_anim.mdl" },
		{ weapons_t::knife_gut,"models/weapons/v_knife_gut_anim.mdl" },
		{ weapons_t::knife_karambit,"models/weapons/v_knife_karam_anim.mdl" },
		{ weapons_t::knife_m9_bayonet,"models/weapons/v_knife_m9_bay_anim.mdl" },
		{ weapons_t::knife_falchion,"models/weapons/v_knife_falchion_advanced_anim.mdl" },
		{ weapons_t::knife_bowie,"models/weapons/v_knife_survival_bowie_anim.mdl" },
		{ weapons_t::knife_butterfly,"models/weapons/v_knife_butterfly_anim.mdl" },
		{ weapons_t::knife_shadow_daggers,"models/weapons/v_knife_push_anim.mdl" },
		{ weapons_t::knife_cord,"models/weapons/v_knife_cord_anim.mdl" },
		{ weapons_t::knife_canis,"models/weapons/v_knife_canis_anim.mdl" },
		{ weapons_t::knife_ursus,"models/weapons/v_knife_ursus_anim.mdl" },
		{ weapons_t::knife_gypsy_jackknife,"models/weapons/v_knife_gypsy_jackknife_anim.mdl" },
		{ weapons_t::knife_outdoor,"models/weapons/v_knife_outdoor_anim.mdl" },
		{ weapons_t::knife_stiletto,"models/weapons/v_knife_stiletto_anim.mdl" },
		{ weapons_t::knife_widowmaker,"models/weapons/v_knife_widowmaker_anim.mdl" },
		{ weapons_t::knife_skeleton,"models/weapons/v_knife_skeleton_anim.mdl" },
	};

	// https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/src_main/public/studio.h#L629
	// Just took the required stuff
	struct mstudioanimdesc_t {
		char pad1 [ 0x4 ];
		int32_t sznameindex;
		char pad2 [ 0x5c ];
	};

	// https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/src_main/public/studio.h#L1965
	// Just took the required stuff
	struct studiohdr_t {
		char pad1 [ 0xc ];
		char name [ 64 ];
		char pad2 [ 0x68 ];
		int32_t numlocalanim;
		int32_t localanimindex;
	};

	static auto vpk_dir = std::string ( pattern::search ( _ ( "engine.dll" ), _ ( "68 ? ? ? ? 8D 85 ? ? ? ? 50 68 ? ? ? ? 68" ) ).add ( 1 ).deref ( ).get<const char*> ( ) ).append ( _ ( "\\pak01_dir.vpk" ) );
	static c_vpk_archive pak01_archive ( vpk_dir );
	
	for ( auto& knife_anim_model : knife_anim_model_names ) {
		std::optional< c_vpk_entry > entry = pak01_archive.get_file ( knife_anim_model.second );

		if ( !entry.has_value ( ) )
			continue;

		std::optional< std::vector< uint8_t > > mdl_data = entry.value ( ).get_data ( );

		if ( !mdl_data.has_value ( ) )
			continue;

		studiohdr_t* hdr = ( studiohdr_t* ) mdl_data.value ( ).data ( );
		mstudioanimdesc_t* anim_desc = ( mstudioanimdesc_t* ) ( ( char* ) hdr + hdr->localanimindex );

		for ( auto i = 0; i < hdr->numlocalanim; ++i ) {
			mstudioanimdesc_t* current = &anim_desc [ i ];
			const char* name = ( char* ) current + current->sznameindex;

			knife_sequences [ knife_anim_model.first ].push_back ( {i, name} );
		}
	}
	VMP_END ( );
}

void features::skinchanger::init ( ) {
	/* dump skins */
	dump_kits ( );
	/* dump knife sequences/animations */
	dump_sequences ( );
	/* dump skin icons */

	/* add items in skin config to inventory */
}

void features::skinchanger::process_death ( event_t* event ) {
	VMP_BEGINMUTATION ( );
	static auto& override_knife = options::skin_vars [ _ ( "skins.skin.override_knife" ) ].val.b;
	static auto& selected_knife = options::skin_vars [ _ ( "skins.skin.knife" ) ].val.i;

	static auto weapon_system = pattern::search ( _ ( "client.dll" ), _ ( "8B 35 ? ? ? ? FF 10 0F B7 C0" ) ).add ( 2 ).deref ( ).get< void* > ( );

	if ( !event || !g::local )
		return;
	
	const auto weapon = g::local->weapon ( );
	
	if ( !weapon )
		return;
	
	const auto attacker = event->get_int ( _("attacker") );
	
	if ( !attacker || cs::i::engine->get_player_for_userid ( attacker ) != cs::i::engine->get_local_player ( ) )
		return;
	
	const auto weapon_name = event->get_string ( _ ( "weapon" ) );
	
	/* increment stattrak here */
	auto cur_gun_skin = options::skin_vars.find ( std::string ( _ ( "skins.skin." ) ).append ( weapon_name ) );

	if ( cur_gun_skin != options::skin_vars.end ( ) )
		cur_gun_skin->second.val.skin.stattrak++;
	
	/* knife icon */
	if ( !weapon_name || !strstr ( weapon_name, _ ( "knife" ) ) )
		return;
	
	if ( override_knife )
		event->set_string ( _ ( "weapon" ), knife_weapon_names [ selected_knife ] );
	VMP_END ( );
}