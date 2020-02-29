#include "autowall.hpp"

void autowall::clip_trace_to_players( const vec3_t& start, const vec3_t& end, std::uint32_t mask, trace_filter_t* filter, trace_t* trace ) {
	static auto clip_trace_to_players_add = pattern::search( "client_panorama.dll", "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC D8 ? ? ? 0F 57 C9" ).get<void*>();

	__asm {
		mov eax, filter
		lea ecx, trace
		push ecx
		push eax
		push mask
		lea edx, end
		lea ecx, start
		call clip_trace_to_players_add
		add esp, 0xC
	}
};

void autowall::scale_dmg( player_t* entity, weapon_info_t* weapon_info, int hitgroup, float& current_damage ) {
	if ( !entity->valid( ) )
		return;

	bool has_heavy_armor = false;

	static auto armored = [ & ] ( void ) {
		switch ( hitgroup ) {
		case ( int ) 1: return entity->has_helmet( );
		case ( int ) 0:
		case ( int ) 2:
		case ( int ) 3:
		case ( int ) 4:
		case ( int ) 5: return true;
		default: return false;
		}
	};

	switch ( hitgroup ) {
	case ( int ) 1: current_damage *= has_heavy_armor ? 2.0f : 4.0f; break;
	case ( int ) 3: current_damage *= 1.25f; break;
	case ( int ) 6:
	case ( int ) 7: current_damage *= 0.75f; break;
	default: break;
	}

	if ( entity->armor( ) > 0 && armored( ) ) {
		auto bonus_val = 1.0f;
		auto armor_bonus_rat = 0.5f;
		auto armor_rat = weapon_info->m_armor_ratio / 2.0f;

		if ( has_heavy_armor ) {
			armor_bonus_rat = 0.33f;
			armor_rat *= 0.5f;
			bonus_val = 0.33f;
		}

		auto dmg = current_damage * armor_rat;

		if ( has_heavy_armor )
			dmg *= 0.85f;

		if ( ( ( current_damage - ( current_damage * armor_rat ) ) * ( bonus_val * armor_bonus_rat ) ) > entity->armor( ) )
			dmg = current_damage - ( entity->armor( ) / armor_bonus_rat );

		current_damage = dmg;
	}
}

bool autowall::simulate_fire_bullet( player_t* entity, player_t* dst_entity, fire_bullet_data_t& data, int hitgroup, vec3_t end_pos ) {
	if ( !entity->valid( ) || !dst_entity->valid( ) || !entity->weapon( ) )
		return false;

	auto weapon_data = entity->weapon( )->data( );

	if ( !weapon_data )
		return false;

	auto enter_surface_data = csgo::i::phys->surface( data.enter_trace.m_surface.m_surface_props );
	auto enter_surface_penetration_modifier = enter_surface_data->m_game.m_penetration_modifier;

	data.penetrate_count = 4;
	data.trace_length = 0.0f;

	data.current_damage = ( float ) weapon_data->m_dmg;

	if ( data.penetrate_count > 0 ) {
		while ( data.current_damage > 1.0f ) {
			data.trace_length_remaining = data.src.dist_to( end_pos ) - data.trace_length;

			auto end = data.direction * data.trace_length_remaining + data.src;

			csgo::util_traceline( data.src, end, 0x4600400B, entity, &data.enter_trace );

			clip_trace_to_players( data.src, end + data.direction * 40.0f, 0x4600400B, &data.filter, &data.enter_trace );

			if ( data.enter_trace.m_fraction >= 0.97f || ( data.enter_trace.m_hit_entity && ( data.enter_trace.m_hit_entity->team( ) == 3 || data.enter_trace.m_hit_entity->team( ) == 2 ) && data.enter_trace.m_hit_entity->team( ) != entity->team( ) ) ) {
				scale_dmg( dst_entity, weapon_data, hitgroup, data.current_damage );

				data.enter_trace.m_hitgroup = hitgroup;
				data.enter_trace.m_endpos = end_pos;
				data.enter_trace.m_hit_entity = dst_entity;

				return true;
			}

			data.trace_length += data.enter_trace.m_fraction * data.trace_length_remaining;
			data.current_damage *= weapon_data->m_range_modifier;

			if ( data.trace_length > 3000.0f || enter_surface_penetration_modifier < 0.1f || !hbp( entity, dst_entity, weapon_data, data ) )
				break;
		}
	}

	return false;
}

bool autowall::hbp( player_t* entity, player_t* dst_entity, weapon_info_t* wpn_data, fire_bullet_data_t& data ) {
	if ( !entity->valid( ) || !wpn_data )
		return false;

	trace_t trace_exit;
	const auto enter_m_surface_data = csgo::i::phys->surface( data.enter_trace.m_surface.m_surface_props );

	const bool is_solid_m_surface = ( ( data.enter_trace.m_contents >> 3 ) & 1 );
	const bool is_light_m_surface = ( ( data.enter_trace.m_surface.m_flags >> 7 ) & 1 );

	float final_damage_modifier = 0.16f;
	float combined_penetration_modifier = 0.0f;

	if ( !data.penetrate_count && !is_light_m_surface && !is_solid_m_surface && enter_m_surface_data->m_game.m_material != 89 ) {
		if ( enter_m_surface_data->m_game.m_material != 71 )
			return false;
	}

	if ( data.penetrate_count <= 0 || wpn_data->m_penetration <= 0.0f )
		return false;

	if ( !trace_to_exit( &data.enter_trace, dst_entity, data.enter_trace.m_endpos, data.direction, &trace_exit ) ) {
		if ( !( csgo::i::trace->get_point_contents( data.enter_trace.m_endpos, 0x600400B, nullptr ) & 0x600400B ) )
			return false;
	}

	const auto exit_m_surface_data = csgo::i::phys->surface( trace_exit.m_surface.m_surface_props );

	if ( enter_m_surface_data->m_game.m_material == 'Y' || enter_m_surface_data->m_game.m_material == 'G' ) {
		combined_penetration_modifier = 3.0f;
		final_damage_modifier = 0.05f;
	}
	else if ( is_light_m_surface || is_solid_m_surface ) {
		combined_penetration_modifier = 1.0f;
		final_damage_modifier = 0.16f;
	}
	else {
		combined_penetration_modifier = ( enter_m_surface_data->m_game.m_penetration_modifier + exit_m_surface_data->m_game.m_penetration_modifier ) * 0.5f;
		final_damage_modifier = 0.16f;
	}

	if ( enter_m_surface_data->m_game.m_material == exit_m_surface_data->m_game.m_material ) {
		if ( exit_m_surface_data->m_game.m_material == 'U' || exit_m_surface_data->m_game.m_material == 'W' )
			combined_penetration_modifier = 3.0f;
		else if ( exit_m_surface_data->m_game.m_material == 'L' )
			combined_penetration_modifier = 2.0f;
	}

	const auto modifier = std::max< float >( 0.0f, 1.0f / combined_penetration_modifier );
	auto lost_damage = std::max< float >( ( modifier * ( trace_exit.m_endpos - data.enter_trace.m_endpos ).length_sqr( ) * 0.041666f ) + ( ( data.current_damage * final_damage_modifier ) + ( std::max< float >( 3.75f / wpn_data->m_penetration, 0.0f ) * 3.0f * modifier ) ), 0.0f );

	if ( lost_damage > data.current_damage )
		return false;

	if ( lost_damage > 0.0f )
		data.current_damage -= lost_damage;

	if ( data.current_damage < 1.0f )
		return false;

	data.src = trace_exit.m_endpos;
	data.penetrate_count--;

	return true;
}

bool autowall::classname_is( player_t* entity, const char* class_name ) {
	return !std::strcmp( entity->client_class( )->m_network_name, class_name );
}

bool autowall::is_breakable_entity( player_t* entity ) {
	static auto __rtdynamiccast_fn = pattern::search( _( "client_panorama.dll"), _( "6A 18 68 ? ? ? ? E8 ? ? ? ? 8B 7D 08" )).get<void*>();
	static auto is_breakable_entity_fn = pattern::search( _( "client_panorama.dll"), _( "55 8B EC 51 56 8B F1 85 F6 74 68 83 BE") ).get<void*>( );
	static auto multiplayer_phys_rtti_desc = *( uintptr_t* ) ( ( uintptr_t ) is_breakable_entity_fn + 0x50 );
	static auto baseentity_rtti_desc = *( uintptr_t* ) ( ( uintptr_t ) is_breakable_entity_fn + 0x55 );
	static auto breakablewithpropdata_rtti_desc = *( uintptr_t* ) ( ( uintptr_t ) is_breakable_entity_fn + 0xD5 );

	int( __thiscall ***v4 )( player_t* );
	int v5;

	if ( entity && ( *( std::uint32_t * ) ( entity + 256 ) >= 0 || ( *( int( ** )( void ) )( *( std::uint32_t* ) entity + 488 ) )( ) <= 0 ) && *( std::uint8_t * ) ( *( std::uint32_t* ) entity + 640 ) == 2 ) {
		auto v3 = *( std::uint32_t* ) ( *( std::uint32_t* ) entity + 1140 );

		if ( v3 != 17 && v3 != 6 && v3 )
			return false;

		if ( *( std::uint32_t* ) ( *( std::uint32_t* ) entity + 256 ) > 200 )
			return false;

		__asm {
			push 0
			push multiplayer_phys_rtti_desc
			push baseentity_rtti_desc
			push 0
			push entity
			call __rtdynamiccast_fn
			add esp, 20
			mov v4, eax
		}

		if ( v4 ) {
			if ( ( **v4 )( entity ) != 1 )
				return false;

			goto label_18;
		}

		if ( !classname_is( entity, _( "func_breakable" )) && !classname_is( entity, _( "func_breakable_surf") ) ) {
			if ( ( *( ( int( __thiscall** )( player_t* ) )*( std::uint32_t * ) entity + 604 ) )( entity ) & 0x10000 )
				return false;

			goto label_18;
		}

		if ( !classname_is( entity, _( "func_breakable_surf") ) || !*( ( uint8_t* ) entity + 2564 ) ) {
		label_18:
			__asm {
				push 0
				push breakablewithpropdata_rtti_desc
				push baseentity_rtti_desc
				push 0
				push entity
				call __rtdynamiccast_fn
				add esp, 20
				mov v5, eax
			}

			if ( v5 && ( ( float( __thiscall* )( uintptr_t ) ) *( uintptr_t* ) ( *( uintptr_t* ) v5 + 12 ) )( v5 ) <= 0.0f )
				return true;
		}
	}

	return false;
}

bool autowall::trace_to_exit( trace_t* tr, player_t* dst_entity, vec3_t start, vec3_t dir, trace_t* exit_tr ) {
	vec3_t end;
	float dist = 0.0f;
	int first_contents = 0;

	while ( dist <= 90.0f ) {
		dist += 4.0f;
		start = start + dir * dist;

		if ( !first_contents )
			first_contents = csgo::i::trace->get_point_contents( start, 0x4600400B, nullptr );

		int point_contents = csgo::i::trace->get_point_contents( start, 0x4600400B, nullptr );

		if ( point_contents & 0x600400B && ( !( point_contents & 0x40000000 ) || first_contents == point_contents ) )
			continue;

		end = start - ( dir * 4.0f );

		csgo::util_traceline( start, end, 0x4600400B, nullptr, exit_tr );

		if ( exit_tr->m_startsolid && exit_tr->m_surface.m_flags & 0x8000 ) {
			csgo::util_traceline( start, end, 0x600400B, dst_entity, exit_tr );

			if ( exit_tr->did_hit( ) && !exit_tr->m_startsolid ) {
				start = exit_tr->m_endpos;
				return true;
			}

			continue;
		}

		if ( exit_tr->did_hit( ) && !exit_tr->m_startsolid ) {
			if ( is_breakable_entity( dst_entity ) && is_breakable_entity( dst_entity ) )
				return true;

			if ( tr->m_surface.m_flags & 0x80 || !( exit_tr->m_surface.m_flags & 0x80 ) && ( exit_tr->m_plane.m_normal.dot_product( dir ) <= 1.0f ) ) {
				start -= dir * ( exit_tr->m_fraction * 4.0f );
				return true;
			}

			continue;
		}

		if ( !exit_tr->did_hit( ) || exit_tr->m_startsolid ) {
			if ( !( exit_tr->m_hit_entity == csgo::i::ent_list->get<void*>( 0 ) ) && is_breakable_entity( dst_entity ) ) {
				exit_tr = tr;
				exit_tr->m_endpos = start + dir;
				return true;
			}
		}
	}

	return false;
}

bool autowall::is_armored( player_t *player, int armor, int hitgroup ) {
	if ( !player )
		return false;

	bool result = false;

	if ( armor > 0 ) {
		switch ( hitgroup ) {
		case ( int ) 0:
		case ( int ) 2:
		case ( int ) 3:
		case ( int ) 4:
		case ( int ) 5:
			result = true;
			break;
		case ( int ) 1:
			result = player->has_helmet( );
			break;
		}
	}

	return result;
}

float autowall::dmg( player_t* entity, player_t* dst_entity, vec3_t src, vec3_t dst, int hitbox ) {
	if ( !entity->valid( ) )
		return 0.0f;

	fire_bullet_data_t data;

	data.src = src;
	data.filter.m_skip = entity;

	vec3_t angle = csgo::calc_angle( data.src, dst );
	data.direction = csgo::angle_vec( angle );

	data.direction.normalize( );

	auto weapon = entity->weapon( );

	if ( !weapon )
		return 0.0f;

	auto weapon_data = weapon->data( );

	if ( !weapon_data )
		return 0.0f;

	if ( simulate_fire_bullet( entity, dst_entity, data, hitbox_to_hitgroup( hitbox ), dst ) )
		return data.current_damage;

	return 0.0f;
}

int autowall::hitbox_to_hitgroup( int hitbox ) {
	int result = 0; // eax

	switch ( hitbox ) {
	case 0: case 1:
		result = 1;
		break;
	case 2: case 4:
		result = 3;
		break;
	case 3: case 5: case 6:
		result = 2;
		break;
	case 7: case 9: case 11:
		result = 7;
		break;
	case 8: case 10: case 12:
		result = 6;
		break;
	case 13: case 15: case 16:
		result = 5;
		break;
	case 14: case 17: case 18:
		result = 4;
		break;
	default:
		result = -1;
		break;
	}

	return result;
}