#pragma once
#include <optional>
#include "../sdk/sdk.hpp"

struct fire_bullet_data_t {
	vec3_t src;
	trace_t enter_trace;
	vec3_t direction;
	trace_filter_t filter;
	float trace_length;
	float trace_length_remaining;
	float current_damage;
	int penetrate_count;
};

namespace autowall {
#pragma optimize( "2", on )

	inline void scale_dmg( player_t* entity, weapon_info_t* weapon_info, int hitgroup, float& current_damage ) {
		if ( !entity->valid( ) )
			return;

		bool has_heavy_armor = entity->has_heavy_armor();

		switch ( hitgroup ) {
			case ( int )1: current_damage *= has_heavy_armor ? 2.0f : 4.0f; break;
			case ( int )3: current_damage *= 1.25f; break;
			case ( int )6:
			case ( int )7: current_damage *= 0.75f; break;
			default: break;
		}

		if ( entity->armor( ) > 0 && ( hitgroup == 1 ? entity->has_helmet ( ) : true ) ) {
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

	inline void clip_trace_to_players_fast( player_t* pl, const vec3_t& start, const vec3_t& end, uint32_t mask, trace_filter_t* filter, trace_t* tr ) {
		const auto dir = ( end - start ).normalized();
		const auto world_pos = pl->world_space( );
		const auto to = world_pos - start;
		const auto range_along = dir.dot_product( to );

		auto range = 0.0f;

		if ( range_along < 0.0f )
			range = -to.length( );
		else if ( range_along > dir.length( ) )
			range = -( world_pos - end ).length( );
		else
			range = ( world_pos - ( dir * range_along + start ) ).length( );

		if ( range > 60.0f )
			return;

		trace_t trace;
		ray_t ray;
		ray.init( start , end );

		cs::i::trace->clip_ray_to_entity( ray , mask | contents_hitbox , pl , &trace );

		if ( tr->m_fraction > trace.m_fraction )
			*tr = trace;
	}

	inline bool classname_is( player_t* entity, const char* class_name ) {
		return !std::strcmp( entity->client_class( )->m_network_name, class_name );
	}

	inline bool is_breakable ( player_t* ent ) {
		/* XREF: "func_breakable_surf" */
		static auto _is_breakable = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 84 C0 75 A1" ) ).resolve_rip();

		bool ret;
		client_class_t* cc;
		const char* name;
		char* takedmg, old_takedmg;

		/*
		...
		.text:102F1934
		.text:102F1934                               loc_102F1934:                           ; CODE XREF: IsBreakable+12↑j
		.text:102F1934 80 BE 80 02 00 00 02                          cmp     byte ptr [esi+280h], 2					<--------------------
		...
		*/

		static auto m_takedamage_offset = _is_breakable.add ( 38 ).deref ( ).get<uint32_t> ( );

		// skip null ents and the world ent.
		if ( !ent || !ent->idx ( ) )
			return false;

		// get m_takedamage and save old m_takedamage.
		takedmg = ( char* ) ( ( uintptr_t ) ent + m_takedamage_offset );
		old_takedmg = *takedmg;

		// get clientclass.
		cc = ent->client_class ( );

		if ( cc && cc->m_network_name ) {
			// get clientclass network name.
			name = cc->m_network_name;

			const auto name_len = strlen ( name );

			// CBreakableSurface, CBaseDoor, ...
			if ( name_len > 16 && name [ 1 ] == 'B' && name [ 9 ] == 'e' && name [ 10 ] == 'S' && name [ 16 ] == 'e' )
				*takedmg = 2;

			else if ( name_len > 5 && name [ 1 ] != 'B' && name [ 5 ] != 'D' )
				*takedmg = 2;

			else if ( name_len > 9 && name [ 1 ] != 'F' && name [ 4 ] != 'c' && name [ 5 ] != 'B' && name [ 9 ] != 'h' ) // CFuncBrush
				*takedmg = 2;
		}

		ret = _is_breakable.get<bool ( __thiscall* )( player_t* )> ( )(ent);
		*takedmg = old_takedmg;

		return ret;
	}

	inline bool trace_to_exit( trace_t* tr, player_t* src_entity, player_t* dst_entity, vec3_t start, vec3_t dir, trace_t* exit_tr ) {
		vec3_t end;
		float dist = 0.0f;
		int first_contents = 0;

		ray_t ray;
		trace_filter_t filter;
		filter.m_skip = src_entity;

		while ( dist <= 90.0f ) {
			// GHETTO OPTIMIZATION
			dist += 4.0f;
			//dist += 6.0f;
			start = start + dir * dist;

			const auto point_contents = cs::i::trace->get_point_contents( start, mask_shot_hull | contents_hitbox, nullptr );

			if ( point_contents & mask_shot_hull && !( point_contents & contents_hitbox ) )
				continue;

			end = start - ( dir * 4.0f );

			ray.init( start , end );
			cs::i::trace->trace_ray( ray, mask_shot_hull | contents_hitbox, &filter, exit_tr );

			if ( exit_tr->m_startsolid && exit_tr->m_surface.m_flags & surf_hitbox ) {
				trace_filter_t filter1;
				filter1.m_skip = exit_tr->m_hit_entity;

				ray.init( start , end );
				cs::i::trace->trace_ray( ray , mask_shot_hull , &filter1 , exit_tr );

				if ( exit_tr->did_hit( ) && !exit_tr->m_startsolid ) {
					start = exit_tr->m_endpos;
					return true;
				}

				continue;
			}

			if ( !exit_tr->did_hit( ) || exit_tr->m_startsolid ) {
				if ( tr->m_hit_entity && tr->m_hit_entity != cs::i::ent_list->get< void* >( 0 ) && is_breakable ( static_cast< player_t* >( tr->m_hit_entity ) ) ) {
					*exit_tr = *tr;
					exit_tr->m_endpos = start + dir;
					return true;
				}

				continue;
			}

			if ( exit_tr->m_surface.m_flags >> 7 & surf_light && !( tr->m_surface.m_flags >> 7 & surf_light ) )
				continue;

			if ( exit_tr->m_plane.m_normal.dot_product( dir ) <= 1.0f ) {
				end = end - dir * ( exit_tr->m_fraction * 4.0f );
				return true;
			}
		}

		return false;
	}

	inline bool hbp( player_t* entity, player_t* dst_entity, weapon_info_t* wpn_data, fire_bullet_data_t& data ) {
		if ( !entity->valid( ) || !wpn_data )
			return false;

		trace_t trace_exit;
		const auto enter_m_surface_data = cs::i::phys->surface( data.enter_trace.m_surface.m_surface_props );

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

		if ( !trace_to_exit( &data.enter_trace, entity, dst_entity, data.enter_trace.m_endpos, data.direction, &trace_exit ) ) {
			if ( !( cs::i::trace->get_point_contents( data.enter_trace.m_endpos, 0x600400B, nullptr ) & 0x600400B ) )
				return false;
		}

		const auto exit_m_surface_data = cs::i::phys->surface( trace_exit.m_surface.m_surface_props );

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

	inline bool is_armored( player_t* player, int armor, int hitgroup ) {
		if ( !player )
			return false;

		bool result = false;

		if ( armor > 0 ) {
			switch ( hitgroup ) {
				case ( int )0:
				case ( int )2:
				case ( int )3:
				case ( int )4:
				case ( int )5:
					result = true;
					break;
				case ( int )1:
					result = player->has_helmet( );
					break;
			}
		}

		return result;
	}

	inline bool simulate_fire_bullet( player_t* entity, player_t* dst_entity, fire_bullet_data_t& data, int hitgroup, vec3_t end_pos ) {
		if ( !entity->valid( ) || !dst_entity->valid( ) || !entity->weapon( ) )
			return false;

		auto weapon_data = entity->weapon( )->data( );

		if ( !weapon_data )
			return false;

		//auto trace_len = weapon_data->m_range;
		const auto max_ray_dist = data.src.dist_to ( end_pos );
		auto trace_len = ( hitgroup != -1 ) ? max_ray_dist : weapon_data->m_range;
		auto enter_surface_data = cs::i::phys->surface( data.enter_trace.m_surface.m_surface_props );
		auto enter_surface_penetration_modifier = enter_surface_data->m_game.m_penetration_modifier;

		data.penetrate_count = 4;
		data.trace_length = 0.0f;
		data.current_damage = ( float )weapon_data->m_dmg;

		ray_t ray;
		trace_filter_t filter;
		filter.m_skip = entity;

		while ( data.current_damage > 1.0f ) {
			data.trace_length_remaining = trace_len - data.trace_length;

			auto end = data.direction * data.trace_length_remaining + data.src;

			ray.init( data.src , end );

			cs::i::trace->trace_ray( ray, mask_shot_hull | contents_hitbox, &filter, &data.enter_trace );

			//clip_trace_to_players_fast( entity, data.src, end + data.direction * 40.0f, mask_shot_hull | contents_hitbox, &data.filter, &data.enter_trace );
			cs::util::clip_trace_to_players ( data.src, end + data.direction * 40.0f, mask_shot_hull | contents_hitbox, &data.filter, &data.enter_trace );

			if ( data.enter_trace.m_fraction >= 1.0f && hitgroup != -1 ) {
				autowall::scale_dmg( dst_entity, weapon_data, hitgroup, data.current_damage );
				return true;
			}

			data.trace_length += data.enter_trace.m_fraction * data.trace_length_remaining;
			data.current_damage *= pow ( weapon_data->m_range_modifier, data.trace_length / 500.0f );

			if ( data.enter_trace.m_hit_entity
				&& (data.enter_trace.m_hit_entity->team ( ) == 2 || data.enter_trace.m_hit_entity->team ( ) == 3)
				&& hitgroup == -1
				&& data.enter_trace.m_hitgroup <= 8 && data.enter_trace.m_hitgroup > 0 ) {
				if ( reinterpret_cast< player_t* >( data.enter_trace.m_hit_entity )->team ( ) == g::local->team ( ) && g::cvars::mp_friendlyfire->get_bool ( ) )
					return false;

				if ( reinterpret_cast< player_t* >( data.enter_trace.m_hit_entity )->team ( ) != g::local->team ( ) ) {
					autowall::scale_dmg ( dst_entity, weapon_data, data.enter_trace.m_hitgroup, data.current_damage );
					return true;
				}
			}

			if ( data.trace_length > 3000.0f || enter_surface_penetration_modifier < 0.1f || !hbp( entity, dst_entity, weapon_data, data ) )
				return false;
		}

		return false;
	}

	inline int hitbox_to_hitgroup( int hitbox ) {
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

	inline float dmg( player_t* entity, player_t* dst_entity, vec3_t src, vec3_t dst, int hitbox, vec3_t* impact_out = nullptr, int* hitgroup_out = nullptr ) {
		if ( !entity->valid( ) )
			return 0.0f;

		fire_bullet_data_t data;

		data.src = src;
		data.filter.m_skip = entity;
		data.direction = dst - src;
		data.direction.normalize( );

		auto weapon = entity->weapon( );

		if ( !weapon )
			return 0.0f;

		auto weapon_data = weapon->data( );

		if ( !weapon_data )
			return 0.0f;

		if ( simulate_fire_bullet( entity, dst_entity, data, hitbox_to_hitgroup( hitbox ), dst ) ) {
			if ( hitgroup_out )
				*hitgroup_out = data.enter_trace.m_hitgroup;

			if ( impact_out )
				*impact_out = data.enter_trace.m_endpos;

			return data.current_damage;
		}

		return 0.0f;
	}

	inline bool trace_ray( const vec3_t& min, const vec3_t& max, const matrix3x4_t& mat, float r, const vec3_t& src, const vec3_t& dst ) {
		static auto vector_rotate = [ ] ( const vec3_t& in1, const matrix3x4_t& in2, vec3_t& out ) {
			out [ 0 ] = in1 [ 0 ] * in2 [ 0 ][ 0 ] + in1 [ 1 ] * in2 [ 1 ][ 0 ] + in1 [ 2 ] * in2 [ 2 ][ 0 ];
			out [ 1 ] = in1 [ 0 ] * in2 [ 0 ][ 1 ] + in1 [ 1 ] * in2 [ 1 ][ 1 ] + in1 [ 2 ] * in2 [ 2 ][ 1 ];
			out [ 2 ] = in1 [ 0 ] * in2 [ 0 ][ 2 ] + in1 [ 1 ] * in2 [ 1 ][ 2 ] + in1 [ 2 ] * in2 [ 2 ][ 2 ];
		};

		static auto vector_transform = [ ] ( const vec3_t& in1, const matrix3x4_t& in2, vec3_t& out ) {
			vec3_t in1t;

			in1t [ 0 ] = in1 [ 0 ] - in2 [ 0 ][ 3 ];
			in1t [ 1 ] = in1 [ 1 ] - in2 [ 1 ][ 3 ];
			in1t [ 2 ] = in1 [ 2 ] - in2 [ 2 ][ 3 ];

			vector_rotate( in1t, in2, out );
		};

		static auto trace_aabb = [ ] ( const vec3_t& src, const vec3_t& dst, const vec3_t& min, const vec3_t& max ) -> bool {
			auto dir = ( dst - src ).normalized( );

			if ( !dir.is_valid( ) )
				return false;

			float tmin, tmax, tymin, tymax, tzmin, tzmax;

			if ( dir.x >= 0.0f ) {
				tmin = ( min.x - src.x ) / dir.x;
				tmax = ( max.x - src.x ) / dir.x;
			}
			else {
				tmin = ( max.x - src.x ) / dir.x;
				tmax = ( min.x - src.x ) / dir.x;
			}

			if ( dir.y >= 0.0f ) {
				tymin = ( min.y - src.y ) / dir.y;
				tymax = ( max.y - src.y ) / dir.y;
			}
			else {
				tymin = ( max.y - src.y ) / dir.y;
				tymax = ( min.y - src.y ) / dir.y;
			}

			if ( tmin > tymax || tymin > tmax )
				return false;

			if ( tymin > tmin )
				tmin = tymin;

			if ( tymax < tmax )
				tmax = tymax;

			if ( dir.z >= 0.0f ) {
				tzmin = ( min.z - src.z ) / dir.z;
				tzmax = ( max.z - src.z ) / dir.z;
			}
			else {
				tzmin = ( max.z - src.z ) / dir.z;
				tzmax = ( min.z - src.z ) / dir.z;
			}

			if ( tmin > tzmax || tzmin > tmax )
				return false;

			if ( tmin < 0.0f || tmax < 0.0f )
				return false;

			return true;
		};

		static auto trace_obb = [ & ] ( const vec3_t& src, const vec3_t& dst, const vec3_t& min, const vec3_t& max, const matrix3x4_t& mat ) -> bool {
			const auto dir = ( dst - src ).normalized( );

			vec3_t ray_trans, dir_trans;
			vector_transform( src, mat, ray_trans );
			vector_rotate( dir, mat, dir_trans );

			return trace_aabb( ray_trans, dir_trans, min, max );
		};

		static auto trace_sphere = [ ] ( const vec3_t& src, const vec3_t& dst, const vec3_t& sphere, float rad ) -> bool {
			auto delta = ( dst - src ).normalized( );

			if ( !delta.is_valid( ) )
				return false;

			auto q = sphere - src;

			if ( !q.is_valid( ) )
				return false;

			auto v = q.dot_product( delta );
			auto d = ( rad * rad ) - ( q.length_sqr( ) - v * v );

			if ( d < FLT_EPSILON )
				return false;

			return true;
		};

		if ( r == -1.0f ) {
			return trace_obb( src, dst, min, max, mat );
		}
		else {
			auto delta = ( max - min ).normalized( );

			const auto hitbox_delta = floorf( min.dist_to( max ) );

			for ( auto i = 0.0f; i <= hitbox_delta; i += 1.0f ) {
				if ( trace_sphere( src, dst, min + delta * i, r ) )
					return true;
			}
		}

		return false;
	}

#pragma optimize( "2", off )
}