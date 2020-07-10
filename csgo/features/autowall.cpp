#include "autowall.hpp"

void clip_trace_to_players_fast ( player_t* pl, const vec3_t& start, const vec3_t& end, std::uint32_t mask, trace_filter_t* filter, trace_t* tr ) {
	trace_t trace;
	ray_t ray;

	ray.init ( start, end );

	if ( !pl || !pl->alive ( ) || pl->dormant ( ) )
		return;

	if ( filter && !filter->should_hit_ent ( pl, mask ) )
		return;

	csgo::i::trace->clip_ray_to_entity ( ray, mask_shot_hull | contents_hitbox, reinterpret_cast < entity_t* > ( pl ), &trace );

	if ( trace.m_fraction < tr->m_fraction )
		*tr = trace;
}

void autowall::clip_trace_to_players( const vec3_t& start, const vec3_t& end, std::uint32_t mask, trace_filter_t* filter, trace_t* trace ) {
	static auto clip_trace_to_players_add = pattern::search( "client.dll", "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC D8 ? ? ? 0F 57 C9" ).get<void*>( );

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

	//auto trace_len = weapon_data->m_range;
	auto trace_len = ( hitgroup != -1 ) ? data.src.dist_to ( end_pos ) : weapon_data->m_range;
	auto enter_surface_data = csgo::i::phys->surface( data.enter_trace.m_surface.m_surface_props );
	auto enter_surface_penetration_modifier = enter_surface_data->m_game.m_penetration_modifier;

	data.penetrate_count = 4;
	data.trace_length = 0.0f;

	data.current_damage = ( float ) weapon_data->m_dmg;

	if ( data.penetrate_count > 0 ) {
		while ( data.current_damage > 1.0f ) {
			data.trace_length_remaining = trace_len - data.trace_length;

			auto end = data.direction * data.trace_length_remaining + data.src;

			ray_t ray;
			ray.init ( data.src, end );
			csgo::i::trace->trace_ray ( ray, mask_shot_hull | contents_hitbox, &data.filter, &data.enter_trace );
			clip_trace_to_players_fast ( dst_entity, data.src, end + data.direction * 40.0f, 0x4600400B, &data.filter, &data.enter_trace );

			if ( data.enter_trace.m_fraction >= 1.0f && hitgroup != -1 ) {
				scale_dmg( dst_entity, weapon_data, hitgroup, data.current_damage );
				return true;
			}

			data.trace_length += data.enter_trace.m_fraction * data.trace_length_remaining;
			data.current_damage *= weapon_data->m_range_modifier;

			if ( data.enter_trace.m_hitgroup > 0 && data.enter_trace.m_hitgroup <= 9 && data.enter_trace.m_hit_entity && data.enter_trace.m_hit_entity == dst_entity ) {
				scale_dmg( dst_entity, weapon_data, data.enter_trace.m_hitgroup, data.current_damage );
				return true;
			}

			if ( data.trace_length > 3000.0f || enter_surface_penetration_modifier < 0.1f || !hbp( entity, dst_entity, weapon_data, data ) )
				break;
		}
	}

	return false;
}

/*
	@CBRS this is public code in the csgo source leak, i would recommend going and taking a look at that. it helps in reversing a more updated autowall immensely
*/
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
	static auto __rtdynamiccast_fn = pattern::search ( "client.dll", "6A 18 68 ? ? ? ? E8 ? ? ? ? 8B 7D 08" ).get < void* > ( );
	static auto is_breakable_entity_fn = pattern::search ( "client.dll", "55 8B EC 51 56 8B F1 85 F6 74 68 83 BE" ).get < void* > ( );
	static auto multiplayerphysics_rtti_desc = *( uintptr_t* ) ( ( uintptr_t ) is_breakable_entity_fn + 0x4F );
	static auto baseentity_rtti_desc = *( uintptr_t* ) ( ( uintptr_t ) is_breakable_entity_fn + 0x54 );
	static auto breakablewithpropdata_rtti_desc = *( uintptr_t* ) ( ( uintptr_t ) is_breakable_entity_fn + 0xD4 );

	if ( !entity )
		return false;

	const auto& take_damage = *reinterpret_cast < uint8_t* > ( uintptr_t ( entity ) + 0x280 );

	int ( __thiscall*** v4 )( player_t* );
	int v5;

	if ( ( entity->health ( ) >= 0 || vfunc< int ( __thiscall* ) ( ) > ( entity, 122 )( ) /* GetMaxHealth */ > 0 ) && take_damage == 2 ) {
		const auto& collision_group = *reinterpret_cast < int* > ( uintptr_t ( entity ) + 0x474 );

		if ( ( collision_group == 17 || collision_group == 6 || !collision_group ) && entity->health ( ) <= 200 ) {
			__asm {
				push 0
				push multiplayerphysics_rtti_desc
				push baseentity_rtti_desc
				push 0
				push entity
				call __rtdynamiccast_fn
				add esp, 20
				mov v4, eax
			}

			if ( v4 ) {
				if ( ( **v4 )( reinterpret_cast < player_t* > ( v4 ) ) != 1 )
					return false;

				goto label_18;
			}

			if ( !classname_is ( entity, _("func_breakable" )) && !classname_is ( entity, _("func_breakable_surf") ) ) {
				if ( ( *( ( int ( __thiscall** )( player_t* ) ) * ( uint32_t* ) entity + 604 ) )( entity ) & 0x10000 /* PhysicsSolidMaskForEntity */ )
					return false;

				goto label_18;
			}

			if ( !classname_is ( entity, _("func_breakable_surf") ) || !*( ( uint8_t* ) entity + 0xA04 ) ) {
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

				if ( v5 && ( ( float ( __thiscall* )( uintptr_t ) ) * ( uintptr_t* ) ( *( uintptr_t* ) v5 + 12 ) )( v5 ) <= 0.0f )
					return true;
			}
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

		const auto point_contents = csgo::i::trace->get_point_contents ( start, mask_shot_hull | contents_hitbox, nullptr );

		if ( point_contents & mask_shot_hull && !( point_contents & contents_hitbox ) )
			continue;

		end = start - ( dir * 4.0f );

		ray_t ray;
		ray.init ( start, end );
		csgo::i::trace->trace_ray ( ray, mask_shot_hull | contents_hitbox, nullptr, exit_tr );

		if ( exit_tr->m_startsolid && exit_tr->m_surface.m_flags & surf_hitbox ) {
			ray_t ray;
			ray.init ( start, end );
			trace_filter_t filter ( exit_tr->m_hit_entity );
			csgo::i::trace->trace_ray ( ray, mask_shot_hull, nullptr, exit_tr );

			if ( exit_tr->did_hit( ) && !exit_tr->m_startsolid ) {
				start = exit_tr->m_endpos;
				return true;
			}

			continue;
		}

		if ( !exit_tr->did_hit ( ) || exit_tr->m_startsolid ) {
			if ( tr->m_hit_entity && ( tr->m_hit_entity != nullptr && tr->m_hit_entity != csgo::i::ent_list->get< void* > ( 0 ) ) ) {
				*exit_tr = *tr;
				exit_tr->m_endpos = start + dir;
				return true;
			}

			continue;
		}

		if ( !exit_tr->did_hit ( ) || exit_tr->m_startsolid ) {
			if ( tr->m_hit_entity != nullptr && !tr->m_hit_entity->idx ( ) == 0 && is_breakable_entity ( static_cast< player_t* >( tr->m_hit_entity ) ) ) {
				*exit_tr = *tr;
				exit_tr->m_endpos = start + dir;
				return true;
			}

			continue;
		}

		if ( exit_tr->m_surface.m_flags >> 7 & surf_light && !( tr->m_surface.m_flags >> 7 & surf_light ) )
			continue;

		if ( exit_tr->m_plane.m_normal.dot_product ( dir ) <= 1.0f ) {
			end = end - dir * ( exit_tr->m_fraction * 4.0f );
			return true;
		}
	}

	return false;
}

bool autowall::is_armored( player_t* player, int armor, int hitgroup ) {
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

float autowall::dmg( player_t* entity, player_t* dst_entity, vec3_t src, vec3_t dst, int hitbox, vec3_t* impact_out, int* hitgroup_out ) {
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

	if ( simulate_fire_bullet ( entity, dst_entity, data, hitbox_to_hitgroup ( hitbox ), dst ) ) {
		if ( hitgroup_out )
			*hitgroup_out = data.enter_trace.m_hitgroup;

		if ( impact_out )
			*impact_out = data.enter_trace.m_endpos;

		return data.current_damage;
	}

	return 0.0f;
}

int autowall::hitbox_to_hitgroup( int hitbox ) {
	int result = 0; // eax

	switch ( hitbox ) {
	case 0:
		result = 1;
		break;
	case 2: case 4: case 1:
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