#include "skinchanger.hpp"

#include "kit_parser.hpp"
#include "../vpk/vpkparser.hpp"

/* XREF: "gc_connected"; function called in first do-while loop */
bool features::inventory::c_inventory::add_econ_item ( c_econ_item* econ_item ) {
	using fn = c_econ_item* ( __thiscall* )( c_inventory*, c_econ_item*, int, int, bool );
	static auto _add_econ_item = pattern::search ( _("client.dll"),_( "55 8B EC 83 E4 F8 A1 ? ? ? ? 83 EC 14 53 56 57 8B F9 8B 08" )).get< fn > ( );
	
	const auto cache = get_so_cache ( );

	cache->add_obj ( econ_item );

	const auto ret = _add_econ_item ( this, econ_item, 1, 0, true );

	//if ( ret ) {
	//	auto i = GetInventoryItemByItemID ( *item->GetItemID ( ) );
	//
	//	*reinterpret_cast< bool* >( ( uintptr_t ) i + 0xA1 ) = 1;
	//}

	return ret;
}

void features::inventory::c_inventory::remove_item ( c_econ_item* econ_item ) {
	using fn = int ( __thiscall* )( void*, uint64_t );
	static auto inv_remove_item = pattern::search ( _("client.dll"),_( "55 8B EC 83 E4 F8 51 53 56 57 FF 75 0C 8B F1") ).get<fn> ( );

	inv_remove_item ( this, econ_item->item_id() );

	get_so_cache ( )->rem_obj ( econ_item );
}

/* REF: "set item texture prefab"; second-to-last function call */
features::inventory::c_inventory* features::inventory::get_local_inventory ( ) {
	static auto local_inventory = pattern::search ( _("client.dll"),_( "8B 35 ? ? ? ? 8B 3D ? ? ? ? 85 F6 0F 84" )).add ( 2 ).deref ( ).get< c_inventory** > ( );
	return *local_inventory;
}

/* XREF: "BuildCacheSubscribed(CEconItem)"; string is argument after function passed in before it */
c_econ_item* features::inventory::create_econ_item ( ) {
	using fn = c_econ_item * ( __stdcall* )( );
	static auto create_shared_object_subclass_econ_item = pattern::search ( _("client.dll"), _("C7 45 EC ? ? ? ? C7 45 E8 00 00 00 00 C7 45 F0 ? ? ? ? C7 45 F4 ? ? ? ? C7 45 FC") ).add ( 3 ).deref ( ).get< fn > ( );
	return create_shared_object_subclass_econ_item ( );
}

/* XREF: "decals/sprays/%s"; first call in function */
void* features::inventory::get_item_schema ( ) {
	using fn = void* ( __stdcall* )( );
	static auto get_item_schema = pattern::search ( _("client.dll"), _("A1 ? ? ? ? 85 C0 75 53" )).get< fn > ( );
	return get_item_schema ( );
}

features::inventory::c_socache* features::inventory::get_so_cache ( ) {
	const auto inventory = get_local_inventory ( );

	using fn = void* ( __thiscall* )( void*, uint64_t, uint64_t, bool );

	static auto find_so_cache = pattern::search ( _("client.dll"),_( "55 8B EC 83 E4 F8 83 EC 1C 0F 10 45 08" )).get< fn > ( );
	static auto client_system = pattern::search ( _("client.dll"), _("8B 0D ? ? ? ? 6A 00 83 EC 10") ).add ( 2 ).deref ( ).deref ( ).get< void* > ( );

	auto so_cache = find_so_cache (
		reinterpret_cast< void* >( reinterpret_cast< uintptr_t > ( client_system ) + 0x70 ),
		*reinterpret_cast< uint64_t* >( reinterpret_cast< uintptr_t > ( get_local_inventory ( ) ) + 0x8 ),
		*reinterpret_cast< uint64_t* >( reinterpret_cast< uintptr_t > ( get_local_inventory ( ) ) + 0x10 ), 0
	);

	auto unk1 = *reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t > ( so_cache ) + 0x1C );
	auto unk2 = *reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t > ( so_cache ) + 0x10 );
	auto type = 1;

	static auto find_base_type_cache = pattern::search ( _("client.dll"), _("55 8B EC 56 57 8B F2 8B F9") ).get< void* > ( );

	auto unk3 = unk2 + 4 * unk1;
	auto t = 1;
	c_socache** _ret = nullptr;

	__asm {
		push inventory
		lea eax, [ t ]
		push eax
		mov ecx, unk2
		mov edx, unk3
		call find_base_type_cache
		mov _ret, eax
		add esp, 8
	}

	return *_ret;
}

void features::skinchanger::c_skin::remove ( ) {
	/* unequip weapon first */
	m_equipped_ct = m_equipped_t = false;
	equip ( );

	/* remove weapon from inventory */
	inventory::get_local_inventory ( )->remove_item ( m_item );

	/* remove this item from skins list */
	auto iter = 0;

	for ( auto& skin : skins ) {
		if ( skin.m_item == m_item )
			skins.erase ( skins.begin ( ) + iter );

		iter++;
	}
}

void features::skinchanger::c_skin::equip ( ) {
	using fn = bool ( __thiscall* )( void*, int, int, uint64_t, bool );
	using fn1 = c_econ_item * ( __thiscall* )( void*, uint64_t );
	using fn2 = c_econ_item * ( __thiscall* )( void*, uint64_t );

	static auto csinventory_mgr = pattern::search ( _("client.dll"),_( "B9 ? ? ? ? 8D 44 24 10 89 54 24 14") ).add ( 1 ).deref ( ).get< void* > ( );
	static auto equip_item_in_loadout = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F8 83 EC 24 83 3D ? ? ? ? ? 53 56 57 8B F9" ) ).get<fn> ( );
	static auto get_inventory_item_by_item_id = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 8B 55 08 83 EC 10 8B C2" ) ).get<fn1> ( );
	static auto find_or_create_reference_econ_item = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 51 8B 55 0C 53 56" ) ).get<fn2> ( );

	auto item_view = get_inventory_item_by_item_id ( csinventory_mgr, m_item->item_id() );

	if ( !item_view ) {
		item_view = find_or_create_reference_econ_item ( csinventory_mgr, m_item->item_id ( ) );

		if ( !item_view )
			return;
	}

	const auto static_data = item_view->static_data ( );

	if ( !static_data )
		return;
	
	equip_item_in_loadout ( csinventory_mgr, 2, static_data->equipped_position ( ), m_equipped_t ? m_item->item_id ( ) : 0, true );
	equip_item_in_loadout ( csinventory_mgr, 3, static_data->equipped_position ( ), m_equipped_ct ? m_item->item_id ( ) : 0, true );
}

void features::skinchanger::update_equipped ( ) {
	const auto inventory = inventory::get_local_inventory ( );

	/* update all weapon equip status (sync with config if we equip in game) */
	for ( auto& skin : skins ) {
		const auto econ_item_definition = skin.m_item->static_data ( );

		if ( !econ_item_definition )
			continue;

		const auto item_t = inventory->item_in_loadout ( 2, econ_item_definition->equipped_position ( ) );
		const auto item_ct = inventory->item_in_loadout ( 3, econ_item_definition->equipped_position ( ) );

		const auto new_equipped_t = item_t != nullptr;
		const auto new_equipped_ct = item_ct != nullptr;

		/* force update if a skin was equipped/unequipped */
		if ( skin.m_equipped_t != new_equipped_t || skin.m_equipped_ct != new_equipped_ct ) {
			cs::i::client_state->delta_tick ( ) = -1;
		}

		skin.m_equipped_t = new_equipped_t;
		skin.m_equipped_ct = new_equipped_ct;
	}
}

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

void features::skinchanger::run ( ) {
	static auto C_BaseViewModel__SendViewModelMatchingSequence = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 56 FF 75 08 8B F1 8B 06 FF 90 ? ? ? ? 8B 86" ) ).get<void ( __thiscall* )( void*, int )> ( );
	static auto CBaseEntity__GetSequenceActivity = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 53 8B 5D 08 56 8B F1 83" ) ).get<int ( __thiscall* )( void*, int )> ( );

	static auto fnEquip = pattern::search (_( "client.dll"),_( "55 8B EC 83 EC 10 53 8B 5D 08 57 8B F9") ).get<int ( __thiscall* )( void*, void* )>();
	static auto fnInitializeAttributes = pattern::search ( _("client.dll"),_( "55 8B EC 83 E4 F8 83 EC 0C 53 56 8B F1 8B 86") ).get<int ( __thiscall* )( void* )>();

	static auto g_ClientLeafSystem = pattern::search ( "client.dll", "A1 ? ? ? ? FF 50 14 8B 86 ? ? ? ? B9 ? ? ? ? C1 E8 0E 24 01 0F B6 C0 50 0F B7 86 ? ? ? ? 50 A1 ? ? ? ? FF 50 18 8B 86 ? ? ? ? B9 ? ? ? ? C1 E8 0C 24 01 0F B6 C0 50 0F B7 86 ? ? ? ? 50 A1 ? ? ? ? FF 90 ? ? ? ? 8B 0D" ).add(1).deref().get<void*>();

	const auto inventory = inventory::get_local_inventory ( );

	/* update all weapon equip status (sync with config if we equip in game) */
	update_equipped ( );

	/* apply skins in game */
	if ( !g::local || !g::local->alive() )
		return;

	player_info_t player_info;
	cs::i::engine->get_player_info ( g::local->idx(), &player_info );

	const auto weapons = g::local->weapons ( );

	for ( auto& weapon : weapons ) {
		if ( !weapon )
			continue;
		
		if ( player_info.m_xuid_high != weapon->original_owner_xuid_high ( ) )
			continue;

		if ( player_info.m_xuid_low != weapon->original_owner_xuid_low ( ) )
			continue;

		const auto econ_item_view = weapon->econ_item ( );

		if ( !econ_item_view )
			continue;

		const auto econ_item_definition = econ_item_view->static_data ( );

		if ( !econ_item_definition )
			continue;

		const auto new_econ_item_view = inventory->item_in_loadout ( g::local->team ( ), econ_item_definition->equipped_position ( ) );

		if ( !new_econ_item_view )
			continue;
		
		const auto new_econ_skin_data = new_econ_item_view->soc_data ( );

		if ( !new_econ_skin_data )
			continue;

		const auto item_definition_index = weapon->item_definition_index ( );
		const auto econ_item_definition_index = new_econ_skin_data->item_definition_index ( );

		if ( !(item_definition_index == weapons_t::knife_t
			|| item_definition_index == weapons_t::knife_ct
			|| ( item_definition_index >= weapons_t::knife_bayonet && item_definition_index <= weapons_t::knife_skeleton ))
			&& item_definition_index != new_econ_skin_data->item_definition_index ( ) )
			continue;

		const auto id = new_econ_skin_data->item_id ( );

		*reinterpret_cast< uint64_t* >( reinterpret_cast< uintptr_t >( &weapon->item_id_high ( ) ) - sizeof( uint64_t ) ) = id;
		weapon->item_id_low ( ) = id & 0xFFFFFFFF;
		weapon->item_id_high ( ) = id >> 32;
		weapon->account ( ) = player_info.m_xuid_low;

		/* do extra stuff if it's a knife */
		if ( item_definition_index == weapons_t::knife_t || item_definition_index == weapons_t::knife_ct || ( item_definition_index >= weapons_t::knife_bayonet && item_definition_index <= weapons_t::knife_skeleton ) ) {
			weapon->item_definition_index ( ) = econ_item_definition_index;
			
			const auto new_econ_item_definition = new_econ_item_view->static_data ( );

			if ( new_econ_item_definition ) {
				const auto new_mdl_idx = cs::i::mdl_info->mdl_idx ( new_econ_item_definition->mdl_name ( ) );

				weapon->mdl_idx ( ) = new_mdl_idx;

				const auto world_mdl = weapon->world_mdl ( );

				if( world_mdl )
					world_mdl->mdl_idx()= new_mdl_idx;
				
				const auto view_mdl = cs::i::ent_list->get_by_handle< weapon_t* > ( g::local->viewmodel_handle ( ) );

				if ( view_mdl && g::local->weapon ( ) &&
					( g::local->weapon ( )->item_definition_index ( ) == weapons_t::knife_t || g::local->weapon ( )->item_definition_index ( ) == weapons_t::knife_ct || ( g::local->weapon ( )->item_definition_index ( ) >= weapons_t::knife_bayonet && g::local->weapon ( )->item_definition_index ( ) <= weapons_t::knife_skeleton ) ) ) {
					/* set current viewmodel model index if we are on knife viewmodel */
					view_mdl->mdl_idx ( ) = new_mdl_idx;

					static int last_sequence = 0;

					/* replace knife animations with different ones right after update */
					if ( last_sequence != view_mdl->sequence ( ) )
						/* grab animations from non-default knife with same animation sequences */
						last_sequence = view_mdl->sequence ( ) = remap_sequence ( weapons_t::knife_bayonet, econ_item_definition_index, view_mdl->sequence ( ) );
				}
			}
		}
	}

	auto get_wearable_create_fn = [ ] ( ) -> create_client_class_t {
		auto clazz = cs::i::client->get_all_classes ( );

		while ( strcmp ( clazz->m_network_name, _ ( "CEconWearable" ) ) )
			clazz = clazz->m_next;

		return clazz->m_create_fn;
	};

	/* glove changer */
	static auto wearables_offset = netvars::get_offset ( _ ( "DT_BaseCombatCharacter->m_hMyWearables" ) );
	
	auto wearables = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(g::local) + wearables_offset );

	if ( !cs::i::ent_list->get_by_handle<weapon_t*> ( wearables [ 0 ] ) ) {
		auto skin_data = inventory->item_in_loadout ( g::local->team ( ), 41 );

		if ( skin_data ) {
			auto entry = cs::i::ent_list->get_highest_index ( ) + 1;

			for ( int i = 0; i < cs::i::ent_list->get_highest_index ( ); i++ ) {
				auto temp_ent = cs::i::ent_list->get<entity_t*> ( i );

				if ( temp_ent && temp_ent->client_class ( ) && !strcmp ( temp_ent->client_class ( )->m_network_name, _ ( "CRopeKeyframe" ) ) ) {
					entry = i;
					break;
				}
			}

			const auto serial = ( rand ( ) % 4096 ) + 1;
			const auto create_wearable_fn = get_wearable_create_fn ( );
			create_wearable_fn ( entry, serial );
			auto wearable = cs::i::ent_list->get<weapon_t*> ( entry );

			if ( wearable ) {
				wearables [ 0 ] = entry | serial << 16;

				auto econ_data = skin_data->soc_data ( );
				auto item_definition = skin_data->static_data ( );

				if ( !econ_data || !item_definition )
					return;

				const auto id = econ_data->item_id ( );

				*reinterpret_cast< uint64_t* >( reinterpret_cast< uintptr_t >( &wearable->item_id_high ( ) ) - sizeof ( uint64_t ) ) = id;
				wearable->item_id_low ( ) = id & 0xFFFFFFFF;
				wearable->item_id_high ( ) = id >> 32;
				wearable->account ( ) = player_info.m_xuid_low;

				wearable->item_definition_index ( ) = econ_data->item_definition_index ( );
				wearable->initialized ( ) = true;
				wearable->mdl_idx ( ) = cs::i::mdl_info->mdl_idx ( item_definition->world_mdl_name ( ) );

				fnEquip ( wearable, g::local );

				g::local->body ( ) = 1;

				fnInitializeAttributes ( wearable );

				vfunc<void ( __thiscall* )( void*, int, int, bool, int, bool )> ( g_ClientLeafSystem, 0 )( g_ClientLeafSystem, reinterpret_cast< uintptr_t >( wearable ) + 4, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF );
			}
		}
	}
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
	std::unordered_map<weapons_t, std::string> knife_anim_model_names {
		{weapons_t::knife_bayonet, "models/weapons/v_knife_bayonet_anim.mdl"},
		{weapons_t::knife_css,"models/weapons/v_knife_css_anim.mdl"},
		{weapons_t::knife_flip,"models/weapons/v_knife_flip_anim.mdl"},
		{weapons_t::knife_gut,"models/weapons/v_knife_gut_anim.mdl"},
		{weapons_t::knife_karambit,"models/weapons/v_knife_karam_anim.mdl"},
		{weapons_t::knife_m9_bayonet,"models/weapons/v_knife_m9_bay_anim.mdl"},
		{weapons_t::knife_falchion,"models/weapons/v_knife_falchion_advanced_anim.mdl"},
		{weapons_t::knife_bowie,"models/weapons/v_knife_survival_bowie_anim.mdl"},
		{weapons_t::knife_butterfly,"models/weapons/v_knife_butterfly_anim.mdl"},
		{weapons_t::knife_shadow_daggers,"models/weapons/v_knife_push_anim.mdl"},
		{weapons_t::knife_cord,"models/weapons/v_knife_cord_anim.mdl"},
		{weapons_t::knife_canis,"models/weapons/v_knife_canis_anim.mdl"},
		{weapons_t::knife_ursus,"models/weapons/v_knife_ursus_anim.mdl"},
		{weapons_t::knife_gypsy_jackknife,"models/weapons/v_knife_gypsy_jackknife_anim.mdl"},
		{weapons_t::knife_outdoor,"models/weapons/v_knife_outdoor_anim.mdl"},
		{weapons_t::knife_stiletto,"models/weapons/v_knife_stiletto_anim.mdl"},
		{weapons_t::knife_widowmaker,"models/weapons/v_knife_widowmaker_anim.mdl"},
		{weapons_t::knife_skeleton,"models/weapons/v_knife_skeleton_anim.mdl"},
	};

	// https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/src_main/public/studio.h#L629
	// Just took the required stuff
	struct mstudioanimdesc_t
	{
		char pad1 [ 0x4 ];
		int32_t sznameindex;
		char pad2 [ 0x5c ];
	};

	// https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/src_main/public/studio.h#L1965
	// Just took the required stuff
	struct studiohdr_t
	{
		char pad1 [ 0xc ];
		char name [ 64 ];
		char pad2 [ 0x68 ];
		int32_t numlocalanim;
		int32_t localanimindex;
	};

	static auto vpk_dir = std::string ( pattern::search ( _ ( "engine.dll" ), _ ( "68 ? ? ? ? 8D 85 ? ? ? ? 50 68 ? ? ? ? 68" ) ).add ( 1 ).deref ( ).get<const char*> ( ) ).append ( _ ( "\\pak01_dir.vpk" ) );
	static c_vpk_archive pak01_archive ( vpk_dir );
	
	for ( auto& knife_anim_model : knife_anim_model_names ) {
		std::optional<c_vpk_entry> entry = pak01_archive.get_file ( knife_anim_model.second );

		if ( !entry.has_value ( ) )
			continue;

		std::optional<std::vector<uint8_t>> mdl_data = entry.value ( ).get_data ( );

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
	if ( !event || !g::local )
		return;

	const auto weapon = g::local->weapon ( );

	if ( !weapon )
		return;

	const auto attacker = event->get_int ( _("attacker") );

	if ( !attacker || cs::i::engine->get_player_for_userid ( attacker ) != cs::i::engine->get_local_player ( ) )
		return;

	const auto weapon_name = event->get_string ( _("weapon"));

	if (!weapon_name || !strstr( weapon_name, _("knife")))
		return;

	const auto inventory = inventory::get_local_inventory ( );

	if ( !inventory )
		return;

	const auto econ_item_view = weapon->econ_item ( );

	if ( !econ_item_view )
		return;

	const auto econ_item_definition = econ_item_view->static_data ( );

	if ( !econ_item_definition )
		return;

	const auto new_econ_item_view = inventory->item_in_loadout ( g::local->team ( ), econ_item_definition->equipped_position ( ) );

	if ( !new_econ_item_view )
		return;

	auto static_data = new_econ_item_view->static_data ( );
	
	if ( !static_data )
		return;

	event->set_string ( _("weapon"), static_data->icon_name() );
}