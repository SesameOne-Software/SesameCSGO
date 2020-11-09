﻿#include "nade_prediction.hpp"
#include "../menu/menu.hpp"
#include "../menu/options.hpp"
#include "../globals.hpp"
#include <deque>
#include <mutex>
#include "prediction.hpp"

#include "../renderer/render.hpp"

/*
nade trajectory prediction
nade esp
*/

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

void features::nade_prediction::predict( ucmd_t* ucmd ) {
	constexpr float restitution = 0.45f;
	constexpr float velocity = 403.0f * 0.9f;

	float step, gravity, new_velocity, unk01;
	int index {}, grenade_act { 1 };
	vec3_t pos, thrown_direction, start, eye_origin;
	vec3_t angles, thrown;

	//	calculate step and actual gravity value
	gravity = 800.0f / 8.0f;
	step = csgo::i::globals->m_ipt;

	//	get local view and eye origin
	eye_origin = g::local->eyes( );
	angles = ucmd->m_angs;

	//	copy current angles and normalise pitch
	thrown = angles;

	if ( thrown.x < 0 ) {
		thrown.x = -10 + thrown.x * ( ( 90 - 10 ) / 90.0f );
	}
	else {
		thrown.x = -10 + thrown.x * ( ( 90 + 10 ) / 90.0f );
	}

	//	find out how we're throwing the grenade
	auto primary_attack = ucmd->m_buttons & 1;
	auto secondary_attack = ucmd->m_buttons & 2048;

	if ( primary_attack && secondary_attack ) {
		grenade_act = ACT_LOB;
	}
	else if ( secondary_attack ) {
		grenade_act = ACT_DROP;
	}

	//	apply 'magic' and modulate by velocity
	unk01 = g::local->weapon( )->throw_strength( );

	if ( !primary_attack && !secondary_attack ) {
		unk01 = old_throw_strength;
	}
	else {
		old_throw_strength = unk01;
	}

	const auto throw_strength = unk01;

	unk01 = unk01 * 0.7f;
	unk01 = unk01 + 0.3f;

	new_velocity = velocity * unk01;

	//	here's where the fun begins
	thrown_direction = csgo::angle_vec( thrown );

	start = eye_origin + vec3_t( 0.0f, 0.0f, ( throw_strength * 12.f ) - 12.f );
	thrown_direction = ( thrown_direction * new_velocity ) + ( g::local->vel( ).length_2d( ) < 5.0f ? vec3_t( 0.0f, 0.0f, 0.0f ) : g::local->vel( ) );

	cur_nade_track.clear( );

	const auto item_def_index = g::local->weapon( )->item_definition_index( );
	auto nade_radius = 0.0f;

	switch ( item_def_index ) {
		case 43:
		case 44:
		case 47:
		case 48:
		case 45:
			nade_radius = 115.0f;
			break;
		case 46:
			nade_radius = 166.0f;
			break;
	}

	//	let's go ahead and predict
	for ( auto time = 0.0f; index < 500; time += step ) {
		pos = start + thrown_direction * step;

		//	setup trace
		trace_t trace;
		csgo::util_tracehull( start, pos, vec3_t( -2.0f, -2.0f, -2.0f ), vec3_t( 2.0f, 2.0f, 2.0f ), 0x46004003, g::local, &trace );

		//	modify path if we have hit something
		if ( trace.m_fraction != 1.0f ) {
			thrown_direction = trace.m_plane.m_normal * -2.0f * thrown_direction.dot_product( trace.m_plane.m_normal ) + thrown_direction;

			thrown_direction *= restitution;

			pos = start + thrown_direction * trace.m_fraction * step;

			time += ( step * ( 1.0f - trace.m_fraction ) );
		}

		//	check for detonation
		auto detonate = detonated( g::local->weapon( ), time, trace );

		//	emplace nade point
		const auto nade_record = nade_record_t( start, pos, trace.m_fraction != 1.0f, true, trace.m_plane.m_normal, detonate, prediction::curtime( ) + time * 2.0f, nade_radius );

		cur_nade_track.push_back( nade_record );
		cur_nade_track_renderable.at( index++ ) = nade_record;
		start = pos;

		//	apply gravity modifier
		thrown_direction.z -= gravity * trace.m_fraction * step;

		if ( detonate ) {
			break;
		}
	}

	for ( auto n = index; n < 500; ++n ) {
		cur_nade_track_renderable.at( n ).m_valid = false;
	}

	nade_predicted = true;
}

bool features::nade_prediction::detonated( weapon_t* weapon, float time, trace_t& trace ) {
	if ( !weapon ) {
		return true;
	}

	const auto index = weapon->item_definition_index( );

	switch ( index ) {
		case 43:
		case 44:
			if ( time > 2.5f )
				return true;
			break;
		case 46:
		case 48:
			if ( trace.m_fraction != 1.0f && trace.m_plane.m_normal.z > cosf(csgo::deg2rad( g::cvars::weapon_molotov_maxdetonateslope->get_float() )) || time > g::cvars::molotov_throw_detonate_time->get_float() )
				return true;
			break;
		case 47:
		case 45:
			if ( time > 2.5f )
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
		was_attacking_last_tick = false;
		return;
	}

	if ( !g::local || !g::local->alive( ) || !g::local->weapon( ) ) {
		cur_nade_track.clear( );
		nade_predicted = false;
		was_attacking_last_tick = false;
		return;
	}

	auto weapon = g::local->weapon( );

	const static std::vector< int > nades {
		43,
		45,
		44,
		46,
		47,
		48
	};

	const auto has_nade = std::find( nades.begin( ), nades.end( ), weapon->item_definition_index( ) ) != nades.end( );

	if ( nade_predicted && !g::local->weapon( )->pin_pulled( ) && g::local->weapon( )->throw_time( ) > 0.0f ) {
		nade_points.push_back( cur_nade_track );
		cur_nade_track.clear( );
		nade_predicted = false;
		was_attacking_last_tick = false;
		return;
	}

	cur_nade_track.clear( );

	if ( has_nade && !( !g::local->weapon( )->pin_pulled( ) && g::local->weapon( )->throw_time( ) > 0.0f ) ) {
		was_attacking_last_tick = ucmd->m_buttons & 1 || ucmd->m_buttons & 2048;
		return predict( ucmd );
	}

	nade_predicted = false;
}

void features::nade_prediction::draw( ) {
	static auto& grenade_trajectories = options::vars [ _( "visuals.other.grenade_trajectories" ) ].val.b;
	static auto& grenade_bounces = options::vars [ _( "visuals.other.grenade_bounces" ) ].val.b;
	static auto& grenade_blast_radii = options::vars [ _( "visuals.other.grenade_blast_radii" ) ].val.b;
	static auto& grenade_path_fade_time = options::vars [ _( "visuals.other.grenade_path_fade_time" ) ].val.f;
	static auto& grenade_trajectory_color = options::vars [ _( "visuals.other.grenade_trajectory_color" ) ].val.c;
	static auto& grenade_bounce_color = options::vars [ _( "visuals.other.grenade_bounce_color" ) ].val.c;
	static auto& grenade_radii_color = options::vars [ _( "visuals.other.grenade_radii_color" ) ].val.c;

	//if ( !g_vars.visuals.grenade_pred )
	//	return;

	if ( !csgo::i::engine->is_in_game( ) || !g::local || !g::local->alive( ) )
		return;

	vec3_t start, end;

	auto calc_alpha = [ & ] ( float time, float fade_time, float base_alpha, bool add = false ) {
		const auto dormant_time = grenade_path_fade_time;
		return static_cast< int >( ( std::clamp< float >( dormant_time - ( std::clamp< float >( add ? ( dormant_time - std::clamp< float >( std::fabsf( prediction::curtime( ) - time ), 0.0f, dormant_time ) ) : std::fabsf( prediction::curtime( ) - time ), std::max< float >( dormant_time - fade_time, 0.0f ), dormant_time ) ), 0.0f, fade_time ) / fade_time ) * base_alpha );
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

				if ( !p.m_valid ) {
					break;
				}

				if ( csgo::render::world_to_screen( start, p.m_start ) && csgo::render::world_to_screen( end, p.m_end ) ) {
					if ( grenade_trajectories ) {
						render::line( start.x, start.y, end.x, end.y, rgba ( static_cast< int > ( grenade_trajectory_color.r * 255.0f ), static_cast< int > ( grenade_trajectory_color.g * 255.0f ), static_cast< int > ( grenade_trajectory_color.b * 255.0f ), alpha ), 2.5f );
					}

					if ( p.m_detonate && grenade_blast_radii ) {
						render::circle3d( p.m_end, p.m_radius, 64, rgba ( static_cast< int > ( grenade_radii_color.r * 255.0f ), static_cast< int > ( grenade_radii_color.g * 255.0f ), static_cast< int > ( grenade_radii_color.b * 255.0f ), alpha2 ), false );
						render::circle3d( p.m_end, p.m_radius, 64, rgba ( static_cast< int > ( grenade_radii_color.r * 255.0f ), static_cast< int > ( grenade_radii_color.g * 255.0f ), static_cast< int > ( grenade_radii_color.b * 255.0f ), alpha2 ), true, 2.5f );
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
		auto base_time = csgo::i::globals->m_curtime;

		auto calc_alpha1 = std::clamp<int>( static_cast< int >( std::sinf( csgo::i::globals->m_curtime * 3.141f ) * 25.0f + grenade_radii_color.a * 255.0f ), 0, 255 );

		for ( auto& p : cur_nade_track_renderable ) {
			auto calc_alpha = std::clamp<int> ( static_cast< int >( std::sinf( base_time * 2.0f * 3.141f ) * 25.0f + grenade_trajectory_color.a * 255.0f ), 0, 255 );
			auto calc_alpha2 = std::clamp<int> ( static_cast< int >( std::sinf( base_time * 2.0f * 3.141f ) * 25.0f + grenade_bounce_color.a * 255.0f ), 0, 255 );

			if ( !p.m_valid ) {
				break;
			}

			if ( csgo::render::world_to_screen( start, p.m_start ) && csgo::render::world_to_screen( end, p.m_end ) ) {
				if ( grenade_trajectories ) {
					render::line( start.x, start.y, end.x, end.y, rgba ( static_cast< int > ( grenade_trajectory_color.r * 255.0f ), static_cast< int > ( grenade_trajectory_color.g * 255.0f ), static_cast< int > ( grenade_trajectory_color.b * 255.0f ), calc_alpha ), 2.5f );
				}

				if ( p.m_detonate && grenade_blast_radii ) {
					render::circle3d( p.m_end, p.m_radius, 64, rgba ( static_cast< int > ( grenade_radii_color.r * 255.0f ), static_cast< int > ( grenade_radii_color.g * 255.0f ), static_cast< int > ( grenade_radii_color.b * 255.0f ), calc_alpha1 ), false );
					render::circle3d( p.m_end, p.m_radius, 64, rgba ( static_cast< int > ( grenade_radii_color.r * 255.0f ), static_cast< int > ( grenade_radii_color.g * 255.0f ), static_cast< int > ( grenade_radii_color.b * 255.0f ), calc_alpha1 ), true, 2.5f );
				}
				else if ( p.m_plane && grenade_bounces ) {
					render::cube( p.m_start, 4, rgba ( static_cast< int > ( grenade_bounce_color.r * 255.0f ), static_cast< int > ( grenade_bounce_color.g * 255.0f ), static_cast< int > ( grenade_bounce_color.b * 255.0f ), calc_alpha2 ) );
				}
			}

			base_time += csgo::i::globals->m_ipt * 3.0f;
		}
	}
}

void features::nade_prediction::draw_beam( ) {
	static auto& grenade_trajectories = options::vars [ _( "visuals.other.grenade_trajectories" ) ].val.b;
	static auto& grenade_bounces = options::vars [ _( "visuals.other.grenade_bounces" ) ].val.b;
	static auto& grenade_blast_radii = options::vars [ _( "visuals.other.grenade_blast_radii" ) ].val.b;
	static auto& grenade_path_fade_time = options::vars [ _( "visuals.other.grenade_path_fade_time" ) ].val.f;
	static auto& grenade_trajectory_color = options::vars [ _( "visuals.other.grenade_trajectory_color" ) ].val.c;
	static auto& grenade_bounce_color = options::vars [ _( "visuals.other.grenade_bounce_color" ) ].val.c;
	static auto& grenade_radii_color = options::vars [ _( "visuals.other.grenade_radii_color" ) ].val.c;

	//if ( !g_vars.visuals.grenade_pred )
	//	return;

	if ( !csgo::i::engine->is_in_game( ) || !g::local || !g::local->alive( ) )
		return;

	vec3_t start, end;

	auto calc_alpha = [ & ] ( float time, float fade_time, float base_alpha, bool add = false ) {
		const auto dormant_time = grenade_path_fade_time;
		return static_cast< int >( ( std::clamp< float >( dormant_time - ( std::clamp< float >( add ? ( dormant_time - std::clamp< float >( std::fabsf( csgo::i::globals->m_curtime - time ), 0.0f, dormant_time ) ) : std::fabsf( csgo::i::globals->m_curtime - time ), std::max< float >( dormant_time - fade_time, 0.0f ), dormant_time ) ), 0.0f, fade_time ) / fade_time ) * base_alpha );
	};

	auto cur_track = 0;

	beam_info_t beam_info;

	beam_info.m_type = 0;
	beam_info.m_model_name = _("sprites/physbeam.vmt");
	beam_info.m_model_idx = -1;
	beam_info.m_halo_scale = 0.0f;
	beam_info.m_life = csgo::i::globals->m_frametime;
	beam_info.m_fade_len = 2.0f;
	beam_info.m_amplitude = 2.3f;
	beam_info.m_segments = 2;
	beam_info.m_renderable = true;
	beam_info.m_speed = 0.0f;
	beam_info.m_start_frame = 0;
	beam_info.m_frame_rate = 0.0f;
	beam_info.m_width = 1.5f;
	beam_info.m_end_width = 1.5f;
	beam_info.m_flags = 0;

	beam_info.m_red = static_cast< float > ( grenade_trajectory_color.r * 255.0f );
	beam_info.m_green = static_cast< float > ( grenade_trajectory_color.g * 255.0f );
	beam_info.m_blue = static_cast< float > ( grenade_trajectory_color.b * 255.0f );
	beam_info.m_brightness = static_cast< float > ( grenade_trajectory_color.a * 255.0f );

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

				if ( !p.m_valid ) {
					break;
				}

				if ( grenade_trajectories ) {
					beam_info.m_start = p.m_start;
					beam_info.m_end = p.m_end;

					auto beam = csgo::i::beams->create_beam_points( beam_info );

					if ( beam )
						csgo::i::beams->draw_beam( beam );
				}

				cur_point++;
			}

			cur_track++;
		}
	}

	if ( nade_predicted ) {
		auto base_time = csgo::i::globals->m_curtime;

		auto calc_alpha1 = std::clamp<int> ( static_cast< int >( std::sinf( csgo::i::globals->m_curtime * 3.141f ) * 25.0f + grenade_radii_color.a * 255.0f ), 0, 255 );

		for ( auto& p : cur_nade_track_renderable ) {
			auto calc_alpha = std::clamp<int> ( static_cast< int >( std::sinf( base_time * 2.0f * 3.141f ) * 25.0f + grenade_radii_color.a * 255.0f ), 0, 255 );
			auto calc_alpha2 = std::clamp<int> ( static_cast< int >( std::sinf( base_time * 2.0f * 3.141f ) * 25.0f + grenade_radii_color.a * 255.0f ), 0, 255 );

			if ( !p.m_valid ) {
				break;
			}

			if ( grenade_trajectories ) {
				beam_info.m_start = p.m_start;
				beam_info.m_end = p.m_end;

				auto beam = csgo::i::beams->create_beam_points( beam_info );

				if ( beam )
					csgo::i::beams->draw_beam( beam );
			}

			base_time += csgo::i::globals->m_ipt * 3.0f;
		}
	}
}