#include "nade_prediction.hpp"
#include "../menu/menu.hpp"
#include "../menu/options.hpp"
#include "../globals.hpp"

#include <deque>
#include <mutex>

#include "prediction.hpp"
#include "esp.hpp"

#include "../renderer/render.hpp"

/* TODO: https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/shared/cstrike15/weapon_basecsgrenade.cpp */

class nade_record_t {
public:
	nade_record_t( ) {
		m_valid = false;
	}

	nade_record_t( vec3_t start, vec3_t end, bool plane, bool valid, vec3_t normal, bool detonate, float time, float radius ) {
		m_start = start;
		m_end = end;
		m_plane = plane;
		m_valid = valid;
		m_normal = normal;
		m_detonate = detonate;
		m_time = time;
		m_radius = radius;
	}

	vec3_t m_start, m_end, m_normal;
	bool m_valid, m_plane, m_detonate;
	float m_time, m_radius;
};

enum nade_throw_act {
	ACT_NONE,
	ACT_THROW,
	ACT_LOB,
	ACT_DROP
};

float old_throw_strength = 1.0f;
std::array< nade_record_t, 500 > cur_nade_track_renderable { };
std::deque< nade_record_t > cur_nade_track { };
std::deque< std::deque< nade_record_t > > nade_points { };
bool nade_predicted = false;
bool will_hit = false;

void features::nade_prediction::predict( ucmd_t* ucmd ) {
	if ( !g::local->weapon ( ) || !g::local->weapon ( )->data( ) ) {
		nade_predicted = false;
		will_hit = false;
		return;
	}

	float step, gravity;
	int index = 0, grenade_act = 1;
	vec3_t pos, thrown_direction, start, eye_origin;
	vec3_t angles, thrown;

	//	calculate step and actual gravity value
	gravity = g::cvars::sv_gravity->get_float() * 0.4f;
	step = cs::i::globals->m_ipt;

	//	get local view and eye origin
	eye_origin = g::local->eyes( );
	angles = ucmd->m_angs;

	//	copy current angles and normalise pitch
	thrown = angles;

	if ( thrown.x < -90.0f )
		thrown.x += 360.f;
	else if ( thrown.x > 90.0f )
		thrown.x -= 360.0f;

	thrown.x -= ( 90.0f - abs ( thrown.x ) ) * 10.0f / 90.0f;

	auto throw_strength = std::clamp ( g::local->weapon ( )->throw_strength ( ), 0.0f, 1.0f );

	auto speed = std::clamp ( g::local->weapon ( )->data ( )->m_throw_velocity * 0.9f, 15.0f, 750.0f );
	speed *= throw_strength * 0.7f + 0.3f;

	thrown_direction = cs::angle_vec ( thrown );
	eye_origin.z += throw_strength * 12.0f - 12.0f;

	trace_t trace;
	cs::util_tracehull ( eye_origin, eye_origin + thrown_direction * 22.0f, vec3_t ( -2.0f, -2.0f, -2.0f ), vec3_t ( 2.0f, 2.0f, 2.0f ), 0x46004003, g::local, &trace );
	start = trace.m_endpos - thrown_direction * 6.0f;

	thrown_direction = thrown_direction * speed + features::prediction::vel * 1.25f;

	cur_nade_track.clear( );

	const auto item_def_index = g::local->weapon( )->item_definition_index( );
	auto nade_radius = 0.0f;

	switch ( item_def_index ) {
	case weapons_t::flashbang:
	case weapons_t::decoy:
		nade_radius = 0.0f;
		break;
	case weapons_t::hegrenade:
	case weapons_t::smoke:
		nade_radius = 115.0f;
		break;
	case weapons_t::molotov:
	case weapons_t::firebomb:
		nade_radius = 166.0f;
		break;
	}

	//	let's go ahead and predict
	vec3_t last_pos = start + thrown_direction * step;

	for ( auto time = 0.0f; index < 500; time += step ) {
		pos = start + thrown_direction * step;

		//	setup trace
		trace_t trace;
		cs::util_tracehull( start, pos, vec3_t( -2.0f, -2.0f, -2.0f ), vec3_t( 2.0f, 2.0f, 2.0f ), 0x46004003, g::local, &trace );

		//	modify path if we have hit something
		if ( trace.m_fraction != 1.0f ) {
			thrown_direction = trace.m_plane.m_normal * -2.0f * thrown_direction.dot_product( trace.m_plane.m_normal ) + thrown_direction;
			thrown_direction *= 0.45f;

			pos = start + thrown_direction * trace.m_fraction * step;

			time += step * ( 1.0f - trace.m_fraction );
		}

		//	check for detonation
		auto detonate = detonated( g::local->weapon( ), time, trace, !index ? vec3_t( 1.0f, 1.0f, 1.0f ) : ( pos - last_pos ) );

		/* check for players in detonation range */
		if ( detonate ) {
			will_hit = false;

			for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
				auto player = cs::i::ent_list->get<player_t*> ( i );

				if ( !player || !player->is_player ( ) || !player->alive ( ) || player->team ( ) == g::local->team ( ) )
					continue;
				
				auto calc_alpha = [ & ] ( float time, float fade_time, bool add = false ) {
					auto dormant_time = std::max< float > ( 9.0f/*esp_fade_time*/, 0.1f );
					return ( std::clamp< float > ( dormant_time - ( std::clamp< float > ( add ? ( dormant_time - std::clamp< float > ( std::fabsf ( ( g::local ? cs::ticks2time ( g::local->tick_base ( ) ) : cs::i::globals->m_curtime ) - time ), 0.0f, dormant_time ) ) : std::fabsf ( cs::i::globals->m_curtime - time ), std::max< float > ( dormant_time - fade_time, 0.0f ), dormant_time ) ), 0.0f, fade_time ) / fade_time );
				};

				int box_alpha = 0;
				if ( !features::esp::esp_data [ i ].m_dormant )
					box_alpha = calc_alpha ( features::esp::esp_data [ i ].m_first_seen, 0.6f, true );
				else
					box_alpha = calc_alpha ( features::esp::esp_data [ i ].m_last_seen, 2.0f );

				if ( box_alpha > 0 && features::esp::esp_data [ i ].m_pos.dist_to ( pos ) <= nade_radius ) {
					will_hit = true;
					break;
				}
			}
		}

		//	emplace nade point
		const auto nade_record = nade_record_t( start, pos, trace.m_fraction != 1.0f, true, trace.m_plane.m_normal, detonate, cs::i::globals->m_curtime + time * 2.0f, nade_radius );

		cur_nade_track.push_back( nade_record );
		cur_nade_track_renderable.at( index++ ) = nade_record;
		start = pos;

		//	apply gravity modifier
		thrown_direction.z -= gravity * trace.m_fraction * step;

		last_pos = pos;

		if ( detonate )
			break;
	}

	//index = 0;
	for ( auto n = index; n < 500; ++n )
		cur_nade_track_renderable.at( n ).m_valid = false;

	nade_predicted = true;
}

bool features::nade_prediction::detonated( weapon_t* weapon, float time, trace_t& trace, const vec3_t& vel ) {
	if ( !weapon )
		return true;

	const auto index = weapon->item_definition_index( );

	switch ( index ) {
	case weapons_t::smoke:
		if ( vel.length ( ) <= 0.1f )
			return true;
		break;
	case weapons_t::decoy:
		if ( vel.length ( ) <= 0.2f )
			return true;
		break;
	case weapons_t::flashbang:
	case weapons_t::hegrenade:
			if ( time > 1.5f )
				return true;
			break;
	case weapons_t::molotov:
	case weapons_t::firebomb:
			if ( trace.m_fraction != 1.0f && trace.m_plane.m_normal.z > cosf ( cs::deg2rad ( g::cvars::weapon_molotov_maxdetonateslope->get_float ( ) ) ) || time > g::cvars::molotov_throw_detonate_time->get_float ( ) )
				return true;
			break;
	}

	return false;
}

auto was_attacking_last_tick = false;

void features::nade_prediction::trace( ucmd_t* ucmd ) {
	static auto& grenade_trajectories = options::vars [ _( "visuals.other.grenade_trajectories" ) ].val.b;
	static auto& grenade_bounces = options::vars [ _( "visuals.other.grenade_bounces" ) ].val.b;
	static auto& grenade_blast_radii = options::vars [ _( "visuals.other.grenade_blast_radii" ) ].val.b;

	if ( ( g::local && !g::local->alive( ) ) || ( !grenade_trajectories && !grenade_bounces && !grenade_blast_radii ) ) {
		cur_nade_track.clear( );
		nade_points.clear( );
		nade_predicted = false;
		will_hit = false;
		was_attacking_last_tick = false;
		return;
	}

	if ( !g::local || !g::local->alive( ) || !g::local->weapon( ) ) {
		cur_nade_track.clear( );
		nade_predicted = false;
		will_hit = false;
		was_attacking_last_tick = false;
		return;
	}

	auto weapon = g::local->weapon( );

	const static std::vector< weapons_t > nades {
		weapons_t::flashbang,
		weapons_t::hegrenade,
		weapons_t::smoke,
		weapons_t::molotov,
		weapons_t::decoy,
		weapons_t::firebomb
	};

	const auto has_nade = std::find( nades.begin( ), nades.end( ), weapon->item_definition_index( ) ) != nades.end( );

	if ( nade_predicted && !g::local->weapon( )->pin_pulled( ) && g::local->weapon( )->throw_time( ) > 0.0f ) {
		nade_points.push_back( cur_nade_track );
		cur_nade_track.clear( );
		nade_predicted = false;
		will_hit = false;
		was_attacking_last_tick = false;
		return;
	}

	cur_nade_track.clear( );

	if ( has_nade && !( !g::local->weapon( )->pin_pulled( ) && g::local->weapon( )->throw_time( ) > 0.0f ) ) {
		was_attacking_last_tick = !!(ucmd->m_buttons & buttons_t::attack) || !!(ucmd->m_buttons & buttons_t::attack2);
		return predict( ucmd );
	}

	nade_predicted = false;
	will_hit = false;
}

void features::nade_prediction::draw( ) {
	static auto& grenade_trajectories = options::vars [ _( "visuals.other.grenade_trajectories" ) ].val.b;
	static auto& grenade_bounces = options::vars [ _( "visuals.other.grenade_bounces" ) ].val.b;
	static auto& grenade_blast_radii = options::vars [ _( "visuals.other.grenade_blast_radii" ) ].val.b;
	static auto& grenade_trajectory_color = options::vars [ _( "visuals.other.grenade_trajectory_color" ) ].val.c;
	static auto& grenade_trajectory_color_hit = options::vars [ _ ( "visuals.other.grenade_trajectory_color_hit" ) ].val.c;
	static auto& grenade_bounce_color = options::vars [ _( "visuals.other.grenade_bounce_color" ) ].val.c;
	static auto& grenade_radii_color = options::vars [ _( "visuals.other.grenade_radii_color" ) ].val.c;

	//if ( !g_vars.visuals.grenade_pred )
	//	return;

	if ( !cs::i::engine->is_in_game( ) || !g::local || !g::local->alive( ) )
		return;

	vec3_t start, end;

	auto calc_alpha = [ & ] ( float time, float fade_time, float base_alpha, bool add = false ) {
		const auto dormant_time = 4.0f;
		return static_cast< int >( ( std::clamp< float >( dormant_time - ( std::clamp< float >( add ? ( dormant_time - std::clamp< float >( std::fabsf( cs::i::globals->m_curtime - time ), 0.0f, dormant_time ) ) : std::fabsf( prediction::curtime( ) - time ), std::max< float >( dormant_time - fade_time, 0.0f ), dormant_time ) ), 0.0f, fade_time ) / fade_time ) * base_alpha );
	};

	auto cur_track = 0;

	if ( !nade_points.empty( ) ) {
		for ( auto& nade_path : nade_points ) {
			auto cur_point = 0;

			if ( nade_path.empty( ) ) {
				nade_points.erase( nade_points.begin( ) + cur_track );
				continue;
			}

			for ( auto& p : nade_path ) {
				const auto alpha = calc_alpha( p.m_time, 2.0f, grenade_trajectory_color.a * 255.0f );
				const auto alpha1 = calc_alpha( p.m_time, 2.0f, grenade_bounce_color.a * 255.0f );
				const auto alpha2 = calc_alpha( p.m_time, 2.0f, grenade_radii_color.a * 255.0f );
				
				if ( !alpha && !alpha1 && !alpha2 ) {
					nade_path.erase( nade_path.begin( ) + cur_point );
					continue;
				}

				if ( !p.m_valid )
					break;

				if ( cs::render::world_to_screen( start, p.m_start ) && cs::render::world_to_screen( end, p.m_end ) ) {
					if ( grenade_trajectories ) {
						render::line( start.x, start.y, end.x, end.y, rgba ( static_cast< int > ( grenade_trajectory_color.r * 255.0f ), static_cast< int > ( grenade_trajectory_color.g * 255.0f ), static_cast< int > ( grenade_trajectory_color.b * 255.0f ), alpha ), 2.0f );
					}

					if ( p.m_detonate && grenade_blast_radii && p.m_radius ) {
						render::circle3d( p.m_end, p.m_radius, 64, rgba ( static_cast< int > ( grenade_radii_color.r * 255.0f ), static_cast< int > ( grenade_radii_color.g * 255.0f ), static_cast< int > ( grenade_radii_color.b * 255.0f ), alpha2 ), false );
						render::circle3d( p.m_end, p.m_radius, 64, rgba ( static_cast< int > ( grenade_radii_color.r * 255.0f ), static_cast< int > ( grenade_radii_color.g * 255.0f ), static_cast< int > ( grenade_radii_color.b * 255.0f ), alpha2 ), true, 3.0f );
					}
					else if ( p.m_plane && grenade_bounces ) {
						render::cube( p.m_start, 4, rgba ( static_cast< int > ( grenade_bounce_color.r * 255.0f ), static_cast< int > ( grenade_bounce_color.g * 255.0f ), static_cast< int > ( grenade_bounce_color.b * 255.0f ), alpha1 ) );
					}
				}

				cur_point++;
			}

			cur_track++;
		}
	}

	if ( nade_predicted ) {
		auto base_time = cs::i::globals->m_curtime;

		const auto calc_alpha1 = std::clamp<int> ( static_cast< int >( std::sinf ( cs::i::globals->m_curtime * cs::pi ) * 25.0f + grenade_radii_color.a * 255.0f ), 0, 255 );
		const auto calc_alpha = std::clamp<int> ( static_cast< int >( std::sinf ( base_time * 2.0f * cs::pi ) * 25.0f + grenade_trajectory_color.a * 255.0f ), 0, 255 );
		const auto calc_alpha2 = std::clamp<int> ( static_cast< int >( std::sinf ( base_time * 2.0f * cs::pi ) * 25.0f + grenade_bounce_color.a * 255.0f ), 0, 255 );

		for ( auto& p : cur_nade_track_renderable ) {
			if ( !p.m_valid )
				break;

			if ( cs::render::world_to_screen( start, p.m_start ) && cs::render::world_to_screen( end, p.m_end ) ) {
				if ( grenade_trajectories ) {
					render::line( start.x, start.y, end.x, end.y,
						will_hit
						? rgba ( static_cast< int > ( grenade_trajectory_color_hit.r * 255.0f ), static_cast< int > ( grenade_trajectory_color_hit.g * 255.0f ), static_cast< int > ( grenade_trajectory_color_hit.b * 255.0f ), calc_alpha )
						: rgba ( static_cast< int > ( grenade_trajectory_color.r * 255.0f ), static_cast< int > ( grenade_trajectory_color.g * 255.0f ), static_cast< int > ( grenade_trajectory_color.b * 255.0f ), calc_alpha ), 2.0f
					);
				}

				if ( p.m_detonate && grenade_blast_radii && p.m_radius ) {
					render::circle3d( p.m_end, p.m_radius, 64, will_hit
						? rgba ( static_cast< int > ( grenade_trajectory_color_hit.r * 255.0f ), static_cast< int > ( grenade_trajectory_color_hit.g * 255.0f ), static_cast< int > ( grenade_trajectory_color_hit.b * 255.0f ), calc_alpha1 )
						: rgba ( static_cast< int > ( grenade_radii_color.r * 255.0f ), static_cast< int > ( grenade_radii_color.g * 255.0f ), static_cast< int > ( grenade_radii_color.b * 255.0f ), calc_alpha1 ), false );
					render::circle3d( p.m_end, p.m_radius, 64, will_hit
						? rgba ( static_cast< int > ( grenade_trajectory_color_hit.r * 255.0f ), static_cast< int > ( grenade_trajectory_color_hit.g * 255.0f ), static_cast< int > ( grenade_trajectory_color_hit.b * 255.0f ), calc_alpha1 )
						: rgba ( static_cast< int > ( grenade_radii_color.r * 255.0f ), static_cast< int > ( grenade_radii_color.g * 255.0f ), static_cast< int > ( grenade_radii_color.b * 255.0f ), calc_alpha1 ), true, 3.0f );
				}
				else if ( p.m_plane && grenade_bounces ) {
					render::cube( p.m_start, 4, will_hit
						? rgba ( static_cast< int > ( grenade_trajectory_color_hit.r * 255.0f ), static_cast< int > ( grenade_trajectory_color_hit.g * 255.0f ), static_cast< int > ( grenade_trajectory_color_hit.b * 255.0f ), calc_alpha2 )
						: rgba ( static_cast< int > ( grenade_bounce_color.r * 255.0f ), static_cast< int > ( grenade_bounce_color.g * 255.0f ), static_cast< int > ( grenade_bounce_color.b * 255.0f ), calc_alpha2 ) );
				}
			}

			base_time += cs::i::globals->m_ipt * 3.0f;
		}
	}
}