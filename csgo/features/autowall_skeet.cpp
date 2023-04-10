#include "autowall_skeet.hpp"

#undef min
#undef max

__forceinline void clip_trace_to_player ( const vec3_t& start, const vec3_t& end, float range, uint32_t mask, player_t* player, trace_t& tr, float& smallest_fraction ) {
	const auto dir = ( end - start ).normalized ( );
	const auto world_pos = player->world_space ( );
	const auto to = world_pos - start;
	const auto range_along = dir.dot_product ( to );

	if ( range_along < 0.0f )
		range = -to.length ( );
	else if ( range_along > dir.length ( ) )
		range = -( world_pos - end ).length ( );
	else
		range = ( world_pos - ( dir * range_along + start ) ).length ( );

	if ( range >= 0.0f && range <= 60.0f ) {
		trace_t trace;
		ray_t ray;
		ray.init ( start, end );

		cs::i::trace->clip_ray_to_entity ( ray, mask | contents_hitbox, player, &trace );

		if ( smallest_fraction > trace.m_fraction ) {
			smallest_fraction = trace.m_fraction;
			tr = trace;
		}
	}
}

__forceinline void clip_trace_to_players ( player_t* dst_player /* can be nullptr */, const vec3_t& start, const vec3_t& end, uint32_t mask, _trace_filter_t* filter, trace_t& tr, float fraction_in ) {
	auto shortest_fraction = fraction_in;

	if ( fraction_in < 0.0f )
		shortest_fraction = tr.m_fraction;

	if ( dst_player ) {
		clip_trace_to_player ( start, end, 0.0f, mask, dst_player, tr, shortest_fraction );
	}
	else {
		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
			auto player = cs::i::ent_list->get<player_t*> ( i );

			if ( !player || !player->alive ( ) || !player->is_player ( ) || player->dormant ( ) )
				continue;

			if ( filter && filter->should_hit_ent ( player, mask ) )
				continue;

			clip_trace_to_player ( start, end, 0.0f, mask, player, tr, shortest_fraction );
		}
	}
}

/* not skeet -- pls change */
__forceinline bool is_breakable ( entity_t* ent ) {
	/* XREF: "func_breakable_surf" */
	static auto _is_breakable = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 84 C0 75 A1" ) ).resolve_rip ( );

	/*
	...
	.text:102F1934
	.text:102F1934                               loc_102F1934:                           ; CODE XREF: IsBreakable+12↑j
	.text:102F1934 80 BE 80 02 00 00 02                          cmp     byte ptr [esi+280h], 2					<--------------------
	...
	*/ 

	static auto m_takedamage_offset = _is_breakable.add ( 38 ).deref ( ).get< uint32_t > ( );

	if ( !ent || !ent->idx ( ) )
		return false;

	auto& takedmg = *reinterpret_cast< uint8_t* > ( reinterpret_cast< uintptr_t >( ent ) + m_takedamage_offset );
	
	const auto backup_takedmg = takedmg;
	takedmg = 2;

	const auto ret = _is_breakable.get<bool ( __thiscall* )( entity_t* )> ( ) ( ent );

	takedmg = backup_takedmg;

	return ret;
}

__forceinline bool trace_to_exit ( vec3_t start, vec3_t dir, vec3_t& end, trace_t& tr, trace_t& exit_tr, float step_size, float max_distance ) {
	auto dist = 0.0f;
	auto first_contents = 0;

	while ( dist <= max_distance ) {
		dist += step_size;

		end = start + dir * dist;

		vec3_t tr_end = end - ( dir * step_size );

		if ( !first_contents )
			first_contents = cs::i::trace->get_point_contents ( end, mask_shot_hull | contents_hitbox );

		const auto point_contents = cs::i::trace->get_point_contents ( end, mask_shot_hull | contents_hitbox, nullptr );

		if ( !( point_contents & mask_shot_hull ) || ( point_contents & contents_hitbox && first_contents != point_contents ) ) {
			ray_t ray;
			ray.init ( end, tr_end );

			cs::i::trace->trace_ray ( ray, mask_shot_hull | contents_hitbox, nullptr, &exit_tr );

			if ( exit_tr.m_startsolid && exit_tr.m_surface.m_flags & surf_hitbox ) {
				trace_filter_world_and_props_only_t filter;

				ray_t ray;
				ray.init ( end, start );

				cs::i::trace->trace_ray ( ray, mask_shot_hull, &filter, &exit_tr );

				if ( exit_tr.did_hit ( ) && !exit_tr.m_startsolid ) {
					end = exit_tr.m_endpos;
					return true;
				}
			}
			else if ( exit_tr.did_hit ( ) && !exit_tr.m_startsolid ) {
				const auto start_is_nodraw = tr.m_surface.m_flags & surf_nodraw;
				const auto exit_is_nodraw =  exit_tr.m_surface.m_flags & surf_nodraw;

				if ( exit_is_nodraw && is_breakable ( exit_tr.m_hit_entity ) && is_breakable ( tr.m_hit_entity ) ) {
					end = exit_tr.m_endpos;
					return true;
				}
				else if ( ( !exit_is_nodraw || ( start_is_nodraw && exit_is_nodraw ) ) && dir.dot_product ( exit_tr.m_plane.m_normal ) <= 1.0f ) {
					end = end - ( ( step_size * exit_tr.m_fraction ) * dir );
					return true;
				}
			}
			else if ( tr.m_hit_entity && tr.m_hit_entity != cs::i::ent_list->get< entity_t* > ( 0 ) && is_breakable ( tr.m_hit_entity ) ) {
				exit_tr = tr;
				exit_tr.m_endpos = start + ( 1.0f * dir );
				return true;
			}
		}
	}

	return false;
}

bool handle_bullet_penetration ( const vec3_t& dir, vec3_t& src, weapon_info_t* weapon_data, trace_t& tr, int& penetrate_count, float& cur_dmg, float dmg_fail, player_t* player ) {
	auto surface_data = cs::i::phys->surface ( tr.m_surface.m_surface_props );
	auto enter_material = surface_data->m_game.m_material;
	auto hit_grate = ( tr.m_contents & contents_grate ) != 0;
	auto is_nodraw = ( tr.m_surface.m_flags & surf_nodraw ) != 0;

	if ( penetrate_count == 0 && !hit_grate && !is_nodraw && enter_material != 'Y' && enter_material != 'G' )
		return true;

	if ( weapon_data->m_penetration <= 0 || penetrate_count <= 0 )
		return true;

	trace_t exit_trace;
	vec3_t exit_end;

	auto max_trace_to_exit_dist = 90.0f;

	//if ( dmg_fail > 10.0f ) {
	//	auto enter_penetration_modifier = 3.0f;
	//	auto enter_dmg_lost_percent = 0.16f;
	//
	//	switch ( enter_material ) {
	//	case 'G':
	//	case 'Y':
	//		enter_penetration_modifier = 3.0f;
	//		enter_dmg_lost_percent = 0.05f;
	//		break;
	//	case 'U':
	//	case 'W':
	//		enter_penetration_modifier = 3.0f;
	//		enter_dmg_lost_percent = 0.16f;
	//		break;
	//	case 'L':
	//		enter_penetration_modifier = 2.0f;
	//		enter_dmg_lost_percent = 0.16f;
	//		break;
	//	default:
	//		if ( is_nodraw || hit_grate ) {
	//			enter_dmg_lost_percent = 0.16f;
	//			enter_penetration_modifier = 1.0f;
	//		}
	//		else {
	//			enter_dmg_lost_percent = 0.16f;
	//			enter_penetration_modifier = ( surface_data->m_game.m_penetration_modifier + 1.0f ) * 0.5f;
	//		}
	//		break;
	//	}
	//
	//	const auto enter_pen_mod = std::max ( 0.0f, 1.0f / enter_penetration_modifier );
	//	const auto enter_pen_wep_mod = std::max ( 0.0f, ( 3.0f / weapon_data->m_penetration ) * 1.25f ) * ( enter_pen_mod * 3.0f ) + ( cur_dmg * enter_dmg_lost_percent );
	//
	//	max_trace_to_exit_dist = std::min ( ( abs ( sqrt ( ( cur_dmg - ( dmg_fail - 1.0f ) ) - enter_pen_wep_mod ) * 4.8989797f ) / sqrt ( enter_pen_mod ) + 4.0f ) + 1.0f, 90.0f );
	//}

	if ( !trace_to_exit ( tr.m_endpos, dir, exit_end, tr, exit_trace, 4.0f, max_trace_to_exit_dist )
		&& ( cs::i::trace->get_point_contents ( tr.m_endpos, mask_shot_hull ) & mask_shot_hull ) == 0 )
		return true;

	auto exit_surface_data = cs::i::phys->surface ( exit_trace.m_surface.m_surface_props );
	auto exit_material = exit_surface_data->m_game.m_material;

	static auto sv_penetration_type = cs::i::cvar->find ( _ ( "sv_penetration_type" ) );
	static auto ff_damage_reduction_bullets = cs::i::cvar->find ( _ ( "ff_damage_reduction_bullets" ) );
	static auto ff_damage_bullet_penetration = cs::i::cvar->find ( _ ( "ff_damage_bullet_penetration" ) );
	static auto mp_teammates_are_enemies = cs::i::cvar->find ( _ ( "mp_teammates_are_enemies" ) );

	auto damage_lost_percent = 0.16f;
	auto penetration_modifier = 1.0f;

	if ( hit_grate || is_nodraw || enter_material == 'Y' || enter_material == 'G' ) {
		if ( enter_material == 'Y' || enter_material == 'G' ) {
			penetration_modifier = 3.0f;
			damage_lost_percent = 0.05f;
		}
		else
			penetration_modifier = 1.0f;
	}
	else if ( enter_material == 'F' && ff_damage_reduction_bullets->get_float ( ) == 0
		&& tr.m_hit_entity && tr.m_hit_entity->is_player ( ) && player && tr.m_hit_entity->team ( ) == player->team ( ) ) {
		if ( ff_damage_bullet_penetration->get_float ( ) == 0 ) {
			penetration_modifier = 0.0f;
			return true;
		}

		penetration_modifier = ff_damage_bullet_penetration->get_float ( );
	}
	else {
		penetration_modifier = ( penetration_modifier + exit_surface_data->m_game.m_penetration_modifier ) * 0.5f;
	}

	if ( enter_material == exit_material ) {
		if ( exit_material == 'W' || exit_material == 'U' )
			penetration_modifier = 3.0f;
		else if ( exit_material == 'L' )
			penetration_modifier = 2.0f;
	}
	
	const auto trace_distance = ( exit_trace.m_endpos - tr.m_endpos ).length ( );
	const auto pen_mod = std::max ( 0.0f, 1.0f / penetration_modifier );
	const auto pen_wep_mod = std::max ( 0.0f, ( 3.0f / surface_data->m_game.m_penetration_modifier ) * 1.25f ) * ( pen_mod * 3.0f ) + ( cur_dmg * damage_lost_percent );
	const auto lost_damage = pen_wep_mod + ( ( pen_mod * ( trace_distance * trace_distance ) ) / 24.0f );

	if ( lost_damage != std::numeric_limits<float>::infinity ( ) ) {
		cur_dmg -= std::max( 0.0f, lost_damage );

		if ( cur_dmg >= 1.0f ) {
			src = exit_trace.m_endpos;
			penetrate_count--;
			return false;
		}
	}

	cur_dmg = 0.0f;
	
	return true;
}

bool awall_skeet::fire_bullet ( player_t* local, vec3_t src, const vec3_t& dir, float min_dmg, weapon_t* weapon, bullet_data_t& data, uint32_t mask ) {
	if ( !local || !weapon )
		return false;

	const auto weapon_id = weapon->item_definition_index ( );

	if ( ( weapon_id >= weapons_t::flashbang && weapon_id <= weapons_t::knife_t ) || weapon_id >= weapons_t::knife_bayonet )
		return false;

	const auto weapon_data = weapon->data ( );

	if ( !weapon_data )
		return false;
	
	trace_filter_skip_two_entities_t filter_skip2;

	filter_skip2.m_skip1 = local;
	filter_skip2.m_skip2 = nullptr;
	
	float trace_len = 0.0f;
	float cur_dmg = weapon_data->m_dmg;
	float cur_dmg_scaled = data.wanted_hitgroup == -1 ? cur_dmg : scale_dmg ( data.wanted_hitgroup, data.player, cur_dmg, weapon_data->m_armor_ratio, weapon_id );
	float min_dmg_clamped = std::max ( 1.0f, min_dmg - 5.0f );

	float range = ( mask & contents_hitbox ) ? weapon_data->m_range : data.distance;

	while ( cur_dmg_scaled > min_dmg_clamped && cur_dmg > 1.0f ) {
		float trace_len_remaining = range - trace_len;

		vec3_t pos = src + dir * trace_len_remaining;

		ray_t ray;
		ray.init ( src, pos );

		cs::i::trace->trace_ray ( ray, mask, &filter_skip2, &data.enter_trace );

		if ( mask & contents_hitbox ) {
			vec3_t delta_endpos = data.enter_trace.m_endpos - src;
			vec3_t clip_trace_end = pos + dir * 40.0f;
			vec3_t delta_clip_trace_end = src - clip_trace_end;

			clip_trace_to_players ( data.player, src, clip_trace_end, mask, &filter_skip2, data.enter_trace, delta_endpos.length ( ) / delta_clip_trace_end.length ( ) );

			if ( data.enter_trace.m_fraction == 1.0f )
				break;
		}

		auto enter_surface_data = cs::i::phys->surface ( data.enter_trace.m_surface.m_surface_props );
		auto enter_surface_penetration_modifier = enter_surface_data->m_game.m_penetration_modifier;

		trace_len += data.enter_trace.m_fraction * trace_len_remaining;
		cur_dmg *= pow ( weapon_data->m_range_modifier, trace_len / 500.0f );

		cur_dmg_scaled = data.wanted_hitgroup == -1 ? cur_dmg : scale_dmg ( data.wanted_hitgroup, data.player, cur_dmg, weapon_data->m_armor_ratio, weapon_id );

		if ( cur_dmg_scaled < min_dmg_clamped || cur_dmg <= 1.0f )
			break;

		if ( ( trace_len > 3000.0f && weapon_data->m_penetration > 0.0f ) || enter_surface_penetration_modifier < 0.1f )
			data.penetrate_count = 0;

		if ( !( mask & contents_hitbox ) ) {
			data.enter_trace.m_hitgroup = data.wanted_hitgroup == -1 ? hitgroup_head : data.wanted_hitgroup;

			if ( data.enter_trace.m_fraction == 1.0f ) {
				data.damage_out = scale_dmg ( data.enter_trace.m_hitgroup, data.player, cur_dmg, weapon_data->m_armor_ratio, weapon_id );
				return true;
			}
		}
		else if ( data.enter_trace.m_hit_entity ) {
			const auto as_player = reinterpret_cast< player_t* >( data.enter_trace.m_hit_entity );

			if ( as_player->is_player ( ) && data.enter_trace.m_hitgroup > hitgroup_generic && data.enter_trace.m_hitgroup <= 8 /*&& !as_player->is_ghost ( )*/ ) {
				const auto is_enemy = local->is_enemy ( as_player );

				if ( !is_enemy && g::cvars::mp_friendlyfire->get_bool ( ) )
					return false;

				if ( is_enemy ) {
					data.damage_out = scale_dmg ( data.enter_trace.m_hitgroup, as_player, cur_dmg, weapon_data->m_armor_ratio, weapon_id );
					return true;
				}
			}
		}

		if ( data.penetrate_count <= 0 )
			break;

		if ( handle_bullet_penetration ( dir, src, weapon_data, data.enter_trace, data.penetrate_count, cur_dmg, min_dmg_clamped, local ) ) {
			data.penetrate_fail = true;
			break;
		}
	}

	data.damage_out = 0.0f;

	return false;
}