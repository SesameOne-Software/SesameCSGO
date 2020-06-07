#include "ragebot.hpp"
#include <deque>
#include "autowall.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../animations/animations.hpp"
#include "prediction.hpp"

player_t* last_target = nullptr;
int target_idx = 0;
std::array< features::ragebot::misses_t, 65 > misses { 0 };
std::array< vec3_t, 65 > target_pos { vec3_t( ) };
std::array< vec3_t, 65 > shot_pos { vec3_t( ) };
std::array< int, 65 > hits { 0 };
std::array< int, 65 > shots { 0 };
std::array< int, 65 > hitbox { 0 };
std::array< features::lagcomp::lag_record_t, 65 > cur_lag_rec { 0 };

int& features::ragebot::get_target_idx ( ) {
	return target_idx;
}

features::lagcomp::lag_record_t& features::ragebot::get_lag_rec ( int pl ) {
	return cur_lag_rec [ pl ];
}

player_t* &features::ragebot::get_target( ) {
	return last_target;
}

features::ragebot::misses_t& features::ragebot::get_misses( int pl ) {
	return misses [ pl ];
}

vec3_t& features::ragebot::get_target_pos( int pl ) {
	return target_pos [ pl ];
}

vec3_t& features::ragebot::get_shot_pos( int pl ) {
	return shot_pos [ pl ];
}

int& features::ragebot::get_hits( int pl ) {
	return hits [ pl ];
}

int& features::ragebot::get_shots ( int pl ) {
	return shots [ pl ];
}

int& features::ragebot::get_hitbox ( int pl ) {
	return hitbox [ pl ];
}

void features::ragebot::hitscan( player_t* pl, vec3_t& point, float& dmg, lagcomp::lag_record_t& rec_out, int& hitbox_out ) {
	OPTION( bool, head, "Sesame->A->Rage Aimbot->Hitboxes->Head", oxui::object_checkbox );
	OPTION( bool, neck, "Sesame->A->Rage Aimbot->Hitboxes->Neck", oxui::object_checkbox );
	OPTION( bool, chest, "Sesame->A->Rage Aimbot->Hitboxes->Chest", oxui::object_checkbox );
	OPTION( bool, pelvis, "Sesame->A->Rage Aimbot->Hitboxes->Pelvis", oxui::object_checkbox );
	OPTION( bool, arms, "Sesame->A->Rage Aimbot->Hitboxes->Arms", oxui::object_checkbox );
	OPTION( bool, legs, "Sesame->A->Rage Aimbot->Hitboxes->Legs", oxui::object_checkbox );
	OPTION( bool, feet, "Sesame->A->Rage Aimbot->Hitboxes->Feet", oxui::object_checkbox );
	OPTION( bool, baim_lethal, "Sesame->A->Rage Aimbot->Hitscan->Baim If Lethal", oxui::object_checkbox );
	OPTION( bool, baim_air, "Sesame->A->Rage Aimbot->Hitscan->Baim If In Lethal", oxui::object_checkbox );
	OPTION( double, damage, "Sesame->A->Rage Aimbot->Main->Minimum Damage", oxui::object_slider );
	OPTION( double, head_ps, "Sesame->A->Rage Aimbot->Hitscan->Head Pointscale", oxui::object_slider );
	OPTION( double, body_ps, "Sesame->A->Rage Aimbot->Hitscan->Body Pointscale", oxui::object_slider );
	OPTION ( double, baim_after_misses, "Sesame->A->Rage Aimbot->Hitscan->Baim After X Misses", oxui::object_slider );
	OPTION ( bool, safe_point, "Sesame->A->Rage Aimbot->Accuracy->Safe Point", oxui::object_dropdown );
	OPTION ( bool, predict_fakelag, "Sesame->A->Rage Aimbot->Accuracy->Predict Fakelag", oxui::object_checkbox );

	const auto recs = lagcomp::get( pl );
	//auto extrapolated = lagcomp::get_extrapolated( pl );
	auto shot = lagcomp::get_shot( pl );

	dmg = 0.0f;

	if ( !g::local || !g::local->weapon ( ) || !pl->valid( ) || !pl->weapon( ) || !pl->weapon( )->data( ) || !pl->bone_cache( ) || !pl->layers ( ) || !pl->animstate( ) || ( !recs.second && !shot.second ) )
		return;

	auto build_record_bones = [ pl ] ( lagcomp::lag_record_t& rec ) {
		if ( rec.m_needs_matrix_construction ) {
			const auto backup_angles = pl->angles ( );
			const auto backup_origin = pl->origin ( );
			auto backup_abs_origin = pl->abs_origin ( );
			const auto backup_min = pl->mins ( );
			const auto backup_max = pl->maxs ( );
			const auto backup_flags = pl->flags ( );
			animlayer_t backup_overlays [ 15 ];
			float backup_poses [ 24 ];
			animstate_t backup_animstate = *pl->animstate ( );

			std::memcpy ( backup_overlays, pl->layers ( ), sizeof backup_overlays );
			std::memcpy ( backup_poses, &pl->poses ( ), sizeof backup_poses );

			pl->mins ( ) = rec.m_min;
			pl->maxs ( ) = rec.m_max;
			pl->origin ( ) = rec.m_origin;
			pl->flags ( ) = rec.m_flags;
			pl->abs_origin ( ) = rec.m_origin;
			*pl->animstate ( ) = rec.m_state;
			pl->angles ( ) = rec.m_ang;

			pl->poses ( ) [ 11 ] = animations::data::body_yaw [ pl->idx ( ) ];

			std::memcpy ( pl->layers ( ), rec.m_layers, sizeof backup_overlays );
			std::memcpy ( &pl->poses ( ), rec.m_poses, sizeof backup_poses );

			animations::build_matrix ( pl, rec.m_bones, N ( 128 ), N ( 256 ), rec.m_simtime );

			pl->mins ( ) = backup_min;
			pl->maxs ( ) = backup_max;
			pl->origin ( ) = backup_origin;
			pl->flags ( ) = backup_flags;
			pl->abs_origin ( ) = backup_abs_origin;
			*pl->animstate ( ) = backup_animstate;
			pl->angles ( ) = backup_angles;

			std::memcpy ( pl->layers ( ), backup_overlays, sizeof backup_overlays );
			std::memcpy ( &pl->poses ( ), backup_poses, sizeof backup_poses );

			rec.m_needs_matrix_construction = false;
		}
	};

	//build_record_bones ( extrapolated.first );
	//build_record_bones ( shot.first );

	const auto backup_origin = pl->origin( );
	auto backup_abs_origin = pl->abs_origin( );
	const auto backup_min = pl->mins( );
	const auto backup_max = pl->maxs( );
	matrix3x4_t backup_bones [ 128 ];
	std::memcpy( backup_bones, pl->bone_cache( ), sizeof matrix3x4_t * pl->bone_count( ) );

	auto newest_moving_tick = 0;
	std::deque < lagcomp::lag_record_t > best_recs { };

	auto head_only = false;
	
	///* added a ton of tiny optimization features to imcrease speed and performance */
	///* if we have a shot, we don't have to look at other records... tap their head off if it's visible */
	//if ( shot.second && csgo::is_visible( shot.first.m_bones [ 8 ].origin( ) ) ) {
	//	shot.first.m_priority = 2;
	//	best_recs.push_back( shot.first );
	//	head_only = true;
	//}
	//else {
	//	if ( shot.second ) {
	//		shot.first.m_priority = 2;
	//		best_recs.push_back( shot.first );
	//	}
	//
	//	/* if extrapolated rec is visible, negate all other records */
	//	/*if ( extrapolated.second && mode >= 2
	//		&& ( csgo::is_visible( extrapolated.first.m_bones [ 3 ].origin( ) )
	//			|| csgo::is_visible( extrapolated.first.m_bones [ 8 ].origin( ) ) ) ) {
	//		shot.first.m_priority = 0;
	//		best_recs.push_back( extrapolated.first );
	//	}
	//	else */{
	//		//if ( extrapolated.second && mode >= 2 ) {
	//		//	shot.first.m_priority = 0;
	//		//	best_recs.push_back( extrapolated.first );
	//		//}
	//
	//		/* get records by priority */
	//		std::for_each( recs.first.begin( ), recs.first.end( ), [ & ] ( lagcomp::lag_record_t& rec ) {
	//			//build_record_bones ( rec );
	//
	//			rec.m_priority = 1;
	//
	//			/* newest (might be exposed) */
	//			if ( rec.m_tick == recs.first.front( ).m_tick )
	//				best_recs.push_back( rec );
	//
	//			/* oldest (hit old position) */
	//			if ( recs.first.size( ) >= 2 && rec.m_tick == recs.first.back( ).m_tick )
	//				best_recs.push_back( rec );
	//
	//			/* moving on ground at speed2d > max_speed * 0.34f (lower desync range) */
	//			if ( rec.m_tick != recs.first.begin( )->m_tick && rec.m_state.m_speed2d > pl->weapon( )->data( )->m_max_speed * 0.5f && rec.m_state.m_hit_ground && rec.m_tick > newest_moving_tick ) {
	//				if ( !best_recs.empty( ) && newest_moving_tick ) {
	//					auto rec_num = best_recs.begin( );
	//
	//					std::for_each( best_recs.begin( ), best_recs.end( ), [ & ] ( const lagcomp::lag_record_t& replaceable ) {
	//						if ( replaceable.m_tick == newest_moving_tick )
	//							best_recs.erase( rec_num );
	//
	//						rec_num++;
	//					} );
	//				}
	//
	//				/* prioritize definite records, but aim at extrapolated data if it's our only option */
	//				best_recs.push_back( rec );
	//				newest_moving_tick = rec.m_tick;
	//			}
	//		} );
	//	}
	//}
	
	if ( shot.second ) {
		best_recs.push_back ( shot.first );
		best_recs.push_back ( recs.first.back ( ) );
		head_only = true;
	}
	else if ( recs.second ) {
		if ( predict_fakelag && !lagcomp::data::extrapolated_records [ pl->idx ( ) ].empty ( ) && csgo::time2ticks( csgo::i::globals->m_curtime - pl->simtime( ) ) > 2 && csgo::time2ticks ( csgo::i::globals->m_curtime - pl->simtime ( ) ) <= 5 )
			best_recs.push_back ( lagcomp::data::extrapolated_records [ pl->idx ( ) ].front ( ) );
		else
			best_recs.push_back ( recs.first.front ( ) );

		best_recs.push_back ( recs.first.back ( ) );
	
		//const auto dmg_old = std::max< float > ( autowall::dmg ( g::local, pl, g::local->eyes ( ), best_recs [ 1 ].m_bones [ 0 ].origin ( ), 0 ), autowall::dmg ( g::local, pl, g::local->eyes ( ), best_recs [ 1 ].m_bones [ 8 ].origin ( ), 0 ) );
		//const auto dmg_new = std::max< float > ( autowall::dmg ( g::local, pl, g::local->eyes ( ), best_recs [ 0 ].m_bones [ 0 ].origin ( ), 0 ), autowall::dmg ( g::local, pl, g::local->eyes ( ), best_recs [ 0 ].m_bones [ 8 ].origin ( ), 0 ) );
		//
		///* ghetto optimization */
		//if ( !dmg_new && !dmg_old )
		//	best_recs.clear ( );
		//else if ( dmg_new && !dmg_old )
		//	best_recs.pop_back ( );
		//else if ( !dmg_new && dmg_old )
		//	best_recs.pop_front ( );
	}

	if ( best_recs.empty( ) )
		return;

	lagcomp::lag_record_t now_rec;

	///* execute selected lagfix */ {
	//	/* allow all records */
	//	if ( mode == 2 ) {
	//
	//	}
	//	/* remove records and only allow latest, without lagfix */
	//	else if ( mode == 0 ) {
	//		best_recs.clear( );
	//
	//		now_rec.store( pl, pl->origin( ) );
	//		now_rec.m_tick = csgo::time2ticks( csgo::i::globals->m_curtime );
	//		best_recs.push_back( now_rec );
	//
	//		if ( shot.second ) {
	//			shot.first.m_priority = 2;
	//			best_recs.push_back( shot.first );
	//		}
	//	}
	//	/* allow only delayed lag records*/
	//	else if ( mode == 1 ) {
	//		auto backup_best_recs = best_recs;
	//		best_recs.clear( );
	//
	//		std::for_each( backup_best_recs.begin( ), backup_best_recs.end( ), [ & ] ( lagcomp::lag_record_t& rec ) {
	//			if ( rec.m_priority == 2 || rec.m_priority == 1 )
	//				best_recs.push_back( rec );
	//		} );
	//	}
	//}

	std::deque< int > hitboxes { };

	if ( pelvis )
		hitboxes.push_back ( 2 ); // pelvis

	if ( head )
		hitboxes.push_back( 0 ); // head

	if ( neck )
		hitboxes.push_back( 1 ); // neck

	if ( feet ) {
		hitboxes.push_back( 11 ); // right foot
		hitboxes.push_back( 12 ); // left foot
	}

	if ( chest ) {
		hitboxes.push_back( 5 ); // chest
	}

	if ( legs ) {
		hitboxes.push_back( 7 ); // right thigh
		hitboxes.push_back( 8 ); // left thigh
	}

	if ( arms ) {
		hitboxes.push_back( 18 ); // right forearm
		hitboxes.push_back( 16 ); // left forearm
	}

	/* skip all hitboxes but head if we have their onshot */
	if ( head_only ) {
		hitboxes.clear( );
		hitboxes.push_back ( 2 ); // pelvis
		hitboxes.push_front( 0 );
	}

	auto should_baim = false;

	/* override to baim if we can doubletap */ {
		/* tickbase manip controller */
		OPTION ( double, tickbase_shift_amount, "Sesame->A->Rage Aimbot->Main->Maximum Doubletap Ticks", oxui::object_slider );
		KEYBIND ( tickbase_key, "Sesame->A->Rage Aimbot->Main->Doubletap Key" );

		auto can_shoot = [ & ] ( ) {
			if ( !g::local->weapon ( ) || !g::local->weapon ( )->ammo ( ) )
				return false;

			//if ( g::shifted_tickbase )
			//	return csgo::ticks2time ( g::local->tick_base ( ) - ( ( g::shifted_tickbase > 0 ) ? 1 + g::shifted_tickbase : 0 ) ) <= g::local->weapon ( )->next_primary_attack ( );

			return g::local->weapon ( )->next_primary_attack ( ) <= csgo::i::globals->m_curtime;
		};

		const auto weapon_data = ( g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) ) ? g::local->weapon ( )->data ( ) : nullptr;
		const auto fire_rate = weapon_data ? weapon_data->m_fire_rate : 0.0f;
		auto tickbase_as_int = std::clamp< int > ( csgo::time2ticks ( fire_rate ), 0, static_cast< int >( tickbase_shift_amount ) );

		if ( !tickbase_key )
			tickbase_as_int = 0;

		if ( tickbase_as_int && ( ( can_shoot ( ) && ( std::abs ( g::ucmd->m_tickcount - g::dt_recharge_time ) >= tickbase_as_int && !g::dt_ticks_to_shift ) ) || g::next_tickbase_shot ) && g::local->weapon ( )->item_definition_index ( ) != 64 ) {
			should_baim = true;
		}
	}

	auto scan_safe_points = true;

	/* retry with safe points if they aren't valid */
retry_without_safe_points:

	auto best_priority = 0;
	auto best_point = vec3_t( );
	auto best_dmg = 0.0f;
	lagcomp::lag_record_t best_rec;
	int best_hitbox = 0;

	/* find best record */
	for ( auto& rec_it : best_recs ) {
		if ( rec_it.m_needs_matrix_construction || rec_it.m_priority < best_priority )
			continue;

		auto safe_point_bones = animations::data::bones [ pl->idx ( ) ];

		/* for safe point scanning */
		/* override aim matrix with safe point matrix so we scan safe points from safe point matrix on current aim matrix */
		if ( safe_point && scan_safe_points ) {
			for ( auto& bone : safe_point_bones )
				bone.set_origin ( bone.origin ( ) + rec_it.m_origin );
		}

		pl->mins ( ) = rec_it.m_min;
		pl->maxs ( ) = rec_it.m_max;
		pl->origin ( ) = rec_it.m_origin;
		pl->set_abs_origin ( rec_it.m_origin );
		std::memcpy ( pl->bone_cache ( ), rec_it.m_bones, sizeof matrix3x4_t * pl->bone_count ( ) );

		/* find best point on best hitbox */
		for ( auto& hb : hitboxes ) {
			auto mdl = pl->mdl( );

			if ( !mdl )
				continue;

			auto studio_mdl = csgo::i::mdl_info->studio_mdl( mdl );

			if ( !studio_mdl )
				continue;

			auto s = studio_mdl->hitbox_set( 0 );

			if ( !s )
				continue;

			auto hitbox = s->hitbox( hb );

			if ( !hitbox )
				continue;

			auto get_hitbox_pos = [ & ] ( bool force_safe_point = false ) {
				vec3_t vmin, vmax;

				auto safe_point_on = true;

				/* override aim matrix with safe point matrix so we scan safe points from safe point matrix on current aim matrix */
				if ( safe_point && scan_safe_points && ( hb != 0 || force_safe_point ) ) {
					VEC_TRANSFORM ( hitbox->m_bbmin, safe_point_bones [ hitbox->m_bone ], vmin );
					VEC_TRANSFORM ( hitbox->m_bbmax, safe_point_bones [ hitbox->m_bone ], vmax );
				}
				else {
					VEC_TRANSFORM ( hitbox->m_bbmin, pl->bone_cache ( ) [ hitbox->m_bone ], vmin );
					VEC_TRANSFORM ( hitbox->m_bbmax, pl->bone_cache ( ) [ hitbox->m_bone ], vmax );
				}

				auto pos = ( vmin + vmax ) * 0.5f;

				return pos;
			};

			auto a1 = csgo::calc_angle( get_hitbox_pos( ), g::local->eyes( ) );
			auto fwd = csgo::angle_vec( a1 );
			auto right = fwd.cross_product( vec3_t( 0.0f, 0.0f, 1.0f ) );
			auto left = -right;
			auto top = vec3_t( 0.0f, 0.0f, 1.0f );

			auto pointscale = 0.5f;

			/* handle pointscale */
			if ( hb == 0 )
				pointscale = static_cast< float >( head_ps ) / 100.0f;
			else
				pointscale = static_cast< float >( body_ps ) / 100.0f;

			auto rad_coeff = pointscale * hitbox->m_radius;

			auto can_tickbase = false;

			//auto can_shoot = [ & ] ( ) {
			//	return g::local && g::local->weapon ( ) && g::local->weapon ( )->next_primary_attack ( ) <= csgo::i::globals->m_curtime && g::local->weapon ( )->ammo ( );
			//};
			//
			///* tickbase manip controller */
			//FIND ( double, tickbase_shift_amount, "rage", "aimbot", "tickbase shift amount", oxui::object_slider );
			//
			//const auto tickbase_as_int = static_cast< int >( tickbase_shift_amount );
			////
			/////* baim if we can tickbase */
			//if ( tickbase_as_int && std::abs ( g::ucmd->m_tickcount - g::dt_recharge_time ) >= tickbase_as_int && !g::dt_ticks_to_shift )
			//	can_tickbase = true;

			//auto tmp_point = get_hitbox_pos ( );
			//auto hitbox_visible = csgo::is_visible ( tmp_point );
			//
			///* scan priority hitbox first, to see if fatal */
			//if ( ( priority && hitbox_visible && ( ( priority == 1 && hb == 0 ) || ( priority == 2 && hb == 1 ) || ( priority == 3 && hb == 1 ) ) ) || ( baim_after_misses != 0 && misses [ pl->idx ( ) ].bad_resolve > baim_after_misses ) ) {
			//	if ( misses [ pl->idx ( ) ].bad_resolve != 0 && misses [ pl->idx ( ) ].bad_resolve > baim_after_misses )
			//		hitbox = s->hitbox ( 2 );
			//	
			//	auto body = get_hitbox_pos( );
			//	
			//	auto body_top = body + top * rad_coeff;
			//	auto body_left = body + left * rad_coeff;
			//	auto body_right = body + right * rad_coeff;
			//
			//	auto new_hb = ( misses [ pl->idx ( ) ].bad_resolve != 0 && misses [ pl->idx ( ) ].bad_resolve > baim_after_misses ) ? 2 : hb;
			//
			//	auto dmg1 = 0.0f;
			//	auto dmg2 = autowall::dmg( g::local, pl, g::local->eyes( ), body_left, new_hb );
			//	auto dmg3 = autowall::dmg( g::local, pl, g::local->eyes( ), body_right, new_hb );
			//
			//	if ( hb == 0 )
			//		autowall::dmg( g::local, pl, g::local->eyes( ), body_top, new_hb );
			//	else
			//		autowall::dmg( g::local, pl, g::local->eyes( ), body, new_hb );
			//
			//	if ( dmg1 > best_dmg || dmg2 > best_dmg || dmg3 > best_dmg ) {
			//		if ( dmg1 > dmg2 && dmg1 > dmg3 || ( dmg1 == dmg2 && dmg1 == dmg3 ) ) {
			//			best_dmg = dmg1;
			//			best_point = body;
			//		}
			//		else if ( dmg2 > dmg1 && dmg2 > dmg3 ) {
			//			best_dmg = dmg2;
			//			best_point = body_left;
			//		}
			//		else if ( dmg3 > dmg1 && dmg3 > dmg2 ) {
			//			best_dmg = dmg3;
			//			best_point = body_right;
			//		}
			//	}
			//
			//	if ( best_dmg > 0.0f && ( ( baim_air && !( rec_it.m_flags & 1 ) ) || ( baim_lethal && best_dmg >= pl->health( ) ) || ( ( hb != 2 && hb != 5 ) && best_dmg > damage ) ) ) {
			//		hitbox_out = new_hb;
			//		point = best_point;
			//		dmg = best_dmg;
			//		rec_out = rec_it;
			//
			//		pl->mins( ) = backup_min;
			//		pl->maxs( ) = backup_max;
			//		pl->origin( ) = backup_origin;
			//		std::memcpy ( pl->bone_cache ( ), backup_bones, sizeof matrix3x4_t * pl->bone_count ( ) );
			//		pl->set_abs_origin( backup_abs_origin );
			//
			//		return;
			//	}
			//}

			/* try to baim */
			if ( hb == 2 ) {
				auto body = get_hitbox_pos ( );
				auto body_left = body + left * rad_coeff;
				auto body_right = body + right * rad_coeff;
				auto dmg1 = autowall::dmg ( g::local, pl, g::local->eyes ( ), body, -1 /* do not scan floating points */ );
				auto dmg2 = autowall::dmg ( g::local, pl, g::local->eyes ( ), body_left, -1 /* do not scan floating points */ );
				auto dmg3 = autowall::dmg ( g::local, pl, g::local->eyes ( ), body_right, -1 /* do not scan floating points */ );

				if ( dmg1 > best_dmg || dmg2 > best_dmg || dmg3 > best_dmg ) {
					if ( dmg1 > dmg2 && dmg1 > dmg3 || ( dmg1 == dmg2 && dmg1 == dmg3 ) ) {
						best_dmg = dmg1;
						best_point = body;
						best_rec = rec_it;
						best_hitbox = hb;
						best_priority = rec_it.m_priority;
					}
					else if ( dmg2 > dmg1 && dmg2 > dmg3 ) {
						best_dmg = dmg2;
						best_point = body_left;
						best_rec = rec_it;
						best_hitbox = hb;
						best_priority = rec_it.m_priority;
					}
					else if ( dmg3 > dmg1 && dmg3 > dmg2 ) {
						best_dmg = dmg3;
						best_point = body_right;
						best_rec = rec_it;
						best_hitbox = hb;
						best_priority = rec_it.m_priority;
					}
				}

				if ( best_dmg >= pl->health ( ) || ( should_baim && ( best_dmg * 1.75f >= damage || best_dmg * 1.75f >= pl->health ( ) ) ) ) {
					pl->mins ( ) = backup_min;
					pl->maxs ( ) = backup_max;
					pl->origin ( ) = backup_origin;
					pl->set_abs_origin ( backup_abs_origin );
					std::memcpy ( pl->bone_cache ( ), backup_bones, sizeof matrix3x4_t * pl->bone_count ( ) );

					point = best_point;
					dmg = best_dmg;
					rec_out = best_rec;
					hitbox_out = best_hitbox;

					return;
				}
			}

			// calculate best points on hitbox
			switch ( hb ) {
				// aim above head to try to hit
			case 0: {
				auto head = get_hitbox_pos( );
				auto head_top = head + top * rad_coeff;
				auto dmg1 = autowall::dmg( g::local, pl, g::local->eyes( ), head, -1 /* do not scan floating points */ );
				auto dmg2 = autowall::dmg( g::local, pl, g::local->eyes( ), head_top, -1 /* do not scan floating points */ );

				if ( dmg1 > best_dmg || dmg2 > best_dmg ) {
					if ( dmg2 > dmg1 ) {
						best_dmg = dmg1;
						best_point = head;
						best_rec = rec_it;
						best_hitbox = hb;
						best_priority = rec_it.m_priority;
					}
					else {
						best_dmg = dmg2;
						best_point = head_top;
						best_rec = rec_it;
						best_hitbox = hb;
						best_priority = rec_it.m_priority;
					}
				}

				/* prefer safe point on head if available, else fall back to normal point */
				if ( safe_point && scan_safe_points ) {
					auto head = get_hitbox_pos ( true );
					auto head_top = head + top * rad_coeff;
					auto dmg1 = autowall::dmg ( g::local, pl, g::local->eyes ( ), head, -1 /* do not scan floating points */ );
					auto dmg2 = autowall::dmg ( g::local, pl, g::local->eyes ( ), head_top, -1 /* do not scan floating points */ );

					if ( dmg1 >= best_dmg || dmg2 >= best_dmg ) {
						if ( dmg2 > dmg1 ) {
							best_dmg = dmg1;
							best_point = head;
							best_rec = rec_it;
							best_hitbox = hb;
							best_priority = rec_it.m_priority;
						}
						else {
							best_dmg = dmg2;
							best_point = head_top;
							best_rec = rec_it;
							best_hitbox = hb;
							best_priority = rec_it.m_priority;
						}
					}
				}
			} break;
				// body stuff (aim outwards)
			case 3:
			case 7:
			case 8:
			case 11:
			case 12: {
				auto body = get_hitbox_pos( );
				auto body_left = body + left * rad_coeff;
				auto body_right = body + right * rad_coeff;
				auto dmg1 = autowall::dmg( g::local, pl, g::local->eyes( ), body, -1 /* do not scan floating points */ );
				auto dmg2 = autowall::dmg( g::local, pl, g::local->eyes( ), body_left, -1 /* do not scan floating points */ );
				auto dmg3 = autowall::dmg( g::local, pl, g::local->eyes( ), body_right, -1 /* do not scan floating points */ );

				if ( dmg1 > best_dmg || dmg2 > best_dmg || dmg3 > best_dmg ) {
					if ( dmg1 > dmg2 && dmg1 > dmg3 || ( dmg1 == dmg2 && dmg1 == dmg3 ) ) {
						best_dmg = dmg1;
						best_point = body;
						best_rec = rec_it;
						best_hitbox = hb;
						best_priority = rec_it.m_priority;
					}
					else if ( dmg2 > dmg1 && dmg2 > dmg3 ) {
						best_dmg = dmg2;
						best_point = body_left;
						best_rec = rec_it;
						best_hitbox = hb;
						best_priority = rec_it.m_priority;
					}
					else if ( dmg3 > dmg1 && dmg3 > dmg2 ) {
						best_dmg = dmg3;
						best_point = body_right;
						best_rec = rec_it;
						best_hitbox = hb;
						best_priority = rec_it.m_priority;
					}
				}
			} break;
				// no need to test multiple points on these
			case 9:
			case 10:
			case 13:
			case 14: {
				auto hitbox_pos = get_hitbox_pos( );
				auto dmg = autowall::dmg( g::local, pl, g::local->eyes( ), hitbox_pos, -1 /* do not scan floating points */ );

				if ( dmg > best_dmg ) {
					best_dmg = dmg;
					best_point = hitbox_pos;
					best_rec = rec_it;
					best_hitbox = hb;
					best_priority = rec_it.m_priority;
				}
			} break;
			}
		}
	}

	pl->mins( ) = backup_min;
	pl->maxs( ) = backup_max;
	pl->origin( ) = backup_origin;
	pl->set_abs_origin ( backup_abs_origin );
	std::memcpy ( pl->bone_cache ( ), backup_bones, sizeof matrix3x4_t* pl->bone_count ( ) );
	
	if ( best_dmg > 0.0f && ( best_dmg >= damage || best_dmg >= pl->health( ) ) ) {
		point = best_point;
		dmg = best_dmg;
		rec_out = best_rec;
		hitbox_out = best_hitbox;
	}
	/* only retry if it's our first pass, or else we will freeze the game by looping indefinitely */
	else if ( safe_point && scan_safe_points ) {
		/* retry, but this time w/out safe points since they weren't valid */
		scan_safe_points = false;
		goto retry_without_safe_points;
	}
}

void meleebot ( ucmd_t* ucmd ) {
	OPTION ( bool, silent, "Sesame->A->Rage Aimbot->Main->Silent", oxui::object_checkbox );
	OPTION ( bool, autoshoot, "Sesame->A->Rage Aimbot->Main->Auto Shoot", oxui::object_checkbox );
	OPTION ( bool, knife_bot, "Sesame->A->Rage Aimbot->Main->Knife Bot", oxui::object_checkbox );
	OPTION ( bool, zeus_bot, "Sesame->A->Rage Aimbot->Main->Zeus Bot", oxui::object_checkbox );

	if ( g::local->weapon ( )->item_definition_index ( ) == 31 && !zeus_bot )
		return;
	else if ( g::local->weapon ( )->data ( )->m_type == 0 && !knife_bot )
		return;

	/*auto can_shoot = [ & ] ( ) {
		return g::local->weapon ( ) && g::local->weapon ( )->next_primary_attack ( ) <= csgo::i::globals->m_curtime && g::local->weapon ( )->ammo ( );
	};

	if ( !can_shoot ( ) )
		return;*/

	vec3_t engine_ang;
	csgo::i::engine->get_viewangles ( engine_ang );

	features::lagcomp::lag_record_t* best_rec = nullptr;
	player_t* best_pl = nullptr;
	float best_fov = 180.0f;
	vec3_t best_point, best_ang;

	csgo::for_each_player ( [ & ] ( player_t* pl ) {
		if ( pl->team ( ) == g::local->team ( ) || pl->immune ( ) )
			return;
		
		auto rec = features::lagcomp::get ( pl );

		if ( !rec.second )
			return;

		auto mdl = pl->mdl ( );

		if ( !mdl )
			return;

		auto studio_mdl = csgo::i::mdl_info->studio_mdl ( mdl );

		if ( !studio_mdl )
			return;

		auto s = studio_mdl->hitbox_set ( 0 );

		if ( !s )
			return;

		auto hitbox = s->hitbox ( 2 );

		if ( !hitbox )
			return;

		auto get_hitbox_pos = [ & ] ( ) {
			vec3_t vmin, vmax;
			VEC_TRANSFORM ( hitbox->m_bbmin, rec.first [ 0 ].m_bones [ hitbox->m_bone ], vmin );
			VEC_TRANSFORM ( hitbox->m_bbmax, rec.first [ 0 ].m_bones [ hitbox->m_bone ], vmax );

			auto pos = ( vmin + vmax ) * 0.5f;

			return pos;
		};

		const auto hitbox_pos = get_hitbox_pos ( );

		auto ang = csgo::calc_angle ( g::local->eyes ( ), hitbox_pos );
		csgo::clamp ( ang );

		const auto fov = ang.dist_to ( engine_ang );

		auto can_use = false;

		if ( g::local->weapon ( )->item_definition_index ( ) == 31 ) {
			can_use = g::local->eyes ( ).dist_to ( hitbox_pos ) < 150.0f;
		}
		else {
			can_use = g::local->origin ( ).dist_to ( rec.first [ 0 ].m_origin ) < 48.0f;
		}

		if ( csgo::is_visible( hitbox_pos ) && fov < best_fov && can_use ) {
			best_pl = pl;
			best_fov = fov;
			best_point = hitbox_pos;
			best_ang = ang;
			best_rec = &rec.first[ 0 ];
		}
	} );

	if ( !autoshoot && ( ( g::local->weapon ( )->data ( )->m_type == 0 ) ? !( ucmd->m_buttons & 1 ) : !( ucmd->m_buttons & 2048 ) ) )
		return;

	if ( !best_pl )
		return;

	csgo::clamp ( best_ang );

	ucmd->m_angs = best_ang;

	if ( !silent )
		csgo::i::engine->set_viewangles ( best_ang );

	(*best_rec).backtrack ( ucmd );

	features::ragebot::get_target_pos ( best_pl->idx ( ) ) = best_point;
	features::ragebot::get_target ( ) = best_pl;
	features::ragebot::get_shots ( best_pl->idx ( ) )++;
	features::ragebot::get_shot_pos ( best_pl->idx ( ) ) = g::local->eyes ( );
	features::ragebot::get_lag_rec ( best_pl->idx ( ) ) = *best_rec;
	features::ragebot::get_target_idx ( ) = best_pl->idx ( );
	features::ragebot::get_hitbox ( best_pl->idx ( ) ) = 5;

	if ( autoshoot && g::local->weapon ( )->item_definition_index ( ) != 31 ) {
		auto back = best_pl->angles ( );
		back.y = csgo::normalize( back.y + 180.0f );
		auto backstab = best_ang.dist_to ( back ) < 45.0f;

		if ( backstab ) {
			ucmd->m_buttons |= 2048;
		}
		else {
			auto hp = best_pl->health ( );
			auto armor = best_pl->armor ( ) > 1;
			auto min_dmg1 = armor ? 34 : 40;
			auto min_dmg2 = armor ? 55 : 65;

			if ( hp <= min_dmg2 )
				ucmd->m_buttons |= 2048;
			else if ( hp <= min_dmg1 )
				ucmd->m_buttons |= 1;
			else
				ucmd->m_buttons |= 1;
		}

		g::send_packet = true;
	}
	else if ( autoshoot ) {
		ucmd->m_buttons |= 1;
		g::send_packet = true;
	}
}

void features::ragebot::run( ucmd_t* ucmd, float& old_smove, float& old_fmove, vec3_t& old_angs ) {
	OPTION( bool, ragebot, "Sesame->A->Rage Aimbot->Main->Main Switch", oxui::object_checkbox );
	OPTION ( double, dt_hitchance, "Sesame->A->Rage Aimbot->Main->Doubletap Hit Chance", oxui::object_slider );
	OPTION( double, hitchance, "Sesame->A->Rage Aimbot->Main->Hit Chance", oxui::object_slider );
	//OPTION( double, hitchance_tolerance, "rage", "aimbot", "hit chance tolerance", oxui::object_slider );
	OPTION( bool, autoshoot, "Sesame->A->Rage Aimbot->Main->Auto Shoot", oxui::object_checkbox );
	OPTION( bool, silent, "Sesame->A->Rage Aimbot->Main->Silent", oxui::object_checkbox );
	OPTION ( bool, autoscope, "Sesame->A->Rage Aimbot->Main->Auto Scope", oxui::object_checkbox );
	OPTION( bool, autoslow, "Sesame->A->Rage Aimbot->Main->Auto Slow", oxui::object_checkbox );
	OPTION ( double, tickbase_shift_amount, "Sesame->A->Rage Aimbot->Main->Maximum Doubletap Ticks", oxui::object_slider );

	security_handler::update ( );

	revolver_firing = false;
	//get_target ( ) = nullptr;

	if ( g::round == round_t::starting )
		return;

	if ( ragebot && g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) && (g::local->weapon()->item_definition_index() == 31 || g::local->weapon ( )->data ( )->m_type == 0 )) {
		meleebot ( ucmd );
		return;
	}

	if ( !ragebot || !g::local || !g::local->weapon( ) || !g::local->weapon( )->data( ) || g::local->weapon( )->data( )->m_type == 9 || g::local->weapon( )->data( )->m_type == 7 || g::local->weapon( )->data( )->m_type == 0 )
		return;

	vec3_t engine_ang;
	csgo::i::engine->get_viewangles( engine_ang );

	auto can_shoot = [ & ] ( ) {
		if ( !g::local->weapon ( ) || !g::local->weapon ( )->ammo ( ) )
			return false;

		//if ( g::shifted_tickbase )
		//	return csgo::ticks2time ( g::local->tick_base ( ) - ( ( g::shifted_tickbase > 0 ) ? 1 + g::shifted_tickbase : 0 ) ) <= g::local->weapon ( )->next_primary_attack ( );

		return g::local->weapon ( )->next_primary_attack ( ) <= csgo::i::globals->m_curtime;
	};

	auto run_hitchance = [ & ] ( vec3_t ang, player_t* pl, vec3_t point ) {
		static int rand_table [ 4 ] { -1 };

		auto weapon = g::local->weapon( );

		if ( !weapon )
			return false;

		auto src = g::local->eyes( );
		csgo::clamp( ang );
		auto forward = csgo::angle_vec( ang );
		auto right = csgo::angle_vec( ang + vec3_t( 0.0f, 90.0f, 0.0f ) );
		auto up = csgo::angle_vec( ang + vec3_t( 90.0f, 0.0f, 0.0f ) );

		auto hits = 0;
		auto needed_hits = static_cast< int >( 150.0f * ( hitchance / 100.0f ) );

		auto weap_spread = weapon->spread( );
		auto weap_inaccuracy = weapon->inaccuracy( );

		for ( auto i = 0; i < 150; i++ ) {
			if ( rand_table [ 0 ] == -1 )
				for ( auto j = 0; j < 4; j++ )
					rand_table [ j ] = rand ( ) % ( ( j % 2 ) ? 360 : 100 );

			auto a = static_cast< float >( rand_table [ 0 ] ) / 100.0f;
			auto b = static_cast< float >( rand_table [ 1 ] ) * ( csgo::pi / 180.0f );
			auto c = static_cast< float >( rand_table [ 2 ] ) / 100.0f;
			auto d = static_cast< float >( rand_table [ 3 ] ) * ( csgo::pi / 180.0f );

			auto inaccuracy = a * weap_inaccuracy;
			auto spread = c * weap_spread;

			if ( weapon->item_definition_index( ) == 64 ) {
				a = 1.0f - a * a;
				a = 1.0f - c * c;
			}

			vec3_t spread_v( ( std::cosf( b ) * inaccuracy ) + ( std::cosf( d ) * spread ), ( std::sinf( b ) * inaccuracy ) + ( std::sinf( d ) * spread ), 0 ), direction;

			direction.x = forward.x + ( spread_v.x * right.x ) + ( spread_v.y * up.x );
			direction.y = forward.y + ( spread_v.x * right.y ) + ( spread_v.y * up.y );
			direction.z = forward.z + ( spread_v.x * right.z ) + ( spread_v.y * up.z );
			direction.normalize( );

			auto va_spread = csgo::vec_angle( direction );
			csgo::clamp( va_spread );

			const auto final_pos = src + ( csgo::angle_vec( va_spread ).normalized( ) * src.dist_to( point ) );
			const auto dst = final_pos.dist_to( point );

			// dbg_print( "%3.f\n", dst );

			if ( dst <= 6.0f )
				hits++;

			if ( ( static_cast< float >( hits ) / 150.0f ) * 100.0f >= ( g::next_tickbase_shot ? dt_hitchance : hitchance ) )
				return true;

			if ( ( 150 - i + hits ) < needed_hits )
				return false;
		}

		return false;
	};

	lagcomp::lag_record_t best_rec;
	player_t* best_pl = nullptr;
	auto best_ang = vec3_t( );
	auto best_point = vec3_t( );
	auto best_fov = 180.0f;
	auto best_dmg = 0.0f;
	auto best_hitbox = 0;

	csgo::for_each_player( [ & ] ( player_t* pl ) {
		if ( pl->team( ) == g::local->team( ) || pl->immune( ) )
			return;

		vec3_t point;
		float dmg = 0.0f;

		hitscan( pl, point, dmg, best_rec, best_hitbox );

		auto ang = csgo::calc_angle( g::local->eyes( ), point );
		csgo::clamp( ang );

		const auto fov = ang.dist_to( engine_ang );

		if ( dmg > 0.0f && dmg > best_dmg && fov < best_fov ) {
			best_pl = pl;
			best_dmg = dmg;
			best_fov = fov;
			best_ang = ang;
			best_point = point;
		}
		} );

	if ( !best_pl )
		return;

	if ( autoshoot && !can_shoot( ) ) {
		ucmd->m_buttons &= ~1;
	}
	else if ( can_shoot( ) ) {
		auto hc = run_hitchance ( best_ang, best_pl, best_point );
		auto should_aim = best_dmg > 0.0f && hc;

		/* TODO: EXTRAPOLATE POSITION TO SLOW DOWN EXACTLY WHEN WE SHOOT */
		if ( autoslow && best_dmg > 0.0f && !hc && g::local->vel ( ).length_2d ( ) > 0.1f ) {
			const auto vec_move = vec3_t ( ucmd->m_fmove, ucmd->m_smove, ucmd->m_umove );
			const auto magnitude = vec_move.length_2d ( );
			const auto max_speed = g::local->weapon ( )->data ( )->m_max_speed;
			const auto move_to_button_ratio = 250.0f / 450.0f;
			const auto speed_ratio = ( max_speed * 0.34f ) * 0.7f;
			const auto move_ratio = speed_ratio * move_to_button_ratio;

			if ( g::local->vel ( ).length_2d ( ) > g::local->weapon ( )->data ( )->m_max_speed * 0.34f ) {
				auto vel_ang = csgo::vec_angle ( vec_move );
				vel_ang.y = csgo::normalize( vel_ang.y + 180.0f );

				const auto normal = csgo::angle_vec( vel_ang ).normalized ( );
				const auto speed_2d = g::local->vel ( ).length_2d ( );

				old_fmove = normal.x * speed_2d;
				old_smove = normal.y * speed_2d;
			}
			else if ( old_fmove != 0.0f || old_smove != 0.0f ) {
				old_fmove = ( old_fmove / magnitude ) * move_ratio;
				old_smove = ( old_smove / magnitude ) * move_ratio;
			}
		}

		if ( !autoshoot )
			should_aim = ucmd->m_buttons & 1 && best_dmg > 0.0f;

		if ( !autoshoot && g::local->weapon ( )->item_definition_index ( ) == 64 )
			should_aim = ucmd->m_buttons & 1 && !revolver_cocking && should_aim;
		else if ( g::local->weapon ( )->item_definition_index ( ) == 64 )
			should_aim = should_aim && !revolver_cocking;

		if ( should_aim ) {
			revolver_firing = true;

			if ( autoshoot ) {
				ucmd->m_buttons |= 1;
				g::send_packet = true;
			}

			if ( autoscope && !g::local->scoped( ) &&
				( g::local->weapon ( )->item_definition_index ( ) == 9
					|| g::local->weapon ( )->item_definition_index ( ) == 40
					|| g::local->weapon ( )->item_definition_index ( ) == 38
					|| g::local->weapon ( )->item_definition_index ( ) == 11 ) )
				ucmd->m_buttons |= 2048;

			auto ang = best_ang - g::local->aim_punch( ) * 2.0f;
			csgo::clamp( ang );

			ucmd->m_angs = ang;

			if ( !silent )
				csgo::i::engine->set_viewangles( ang );

			best_rec.backtrack ( ucmd );

			get_target_pos( best_pl->idx( ) ) = best_point;
			get_target( ) = best_pl;
			get_shots( best_pl->idx( ) )++;
			get_shot_pos( best_pl->idx( ) ) = g::local->eyes( );
			get_lag_rec ( best_pl->idx ( ) ) = best_rec;
			get_target_idx ( ) = best_pl->idx ( );
			get_hitbox ( best_pl->idx ( ) ) = best_hitbox;
		}
		else if ( best_dmg > 0.0f ) {
			if ( g::local->weapon ( )->item_definition_index ( ) == 9
				|| g::local->weapon ( )->item_definition_index ( ) == 40
				|| g::local->weapon ( )->item_definition_index ( ) == 38
				|| g::local->weapon ( )->item_definition_index ( ) == 11 ) {
				if ( autoscope && !g::local->scoped ( ) )
					ucmd->m_buttons |= 2048;
				else if ( autoscope && g::local->scoped ( ) )
					ucmd->m_buttons &= ~2048;
			}
		}
	}
}

void features::ragebot::tickbase_controller( ucmd_t* ucmd ) {
	auto can_shoot = [ & ] ( ) {
		if ( !g::local->weapon ( ) || !g::local->weapon ( )->ammo ( ) || !g::local->weapon ( )->data ( ) )
			return false;

		//if ( g::shifted_tickbase )
		//	return csgo::ticks2time ( g::local->tick_base ( ) - ( ( g::shifted_tickbase > 0 ) ? 1 + g::shifted_tickbase : 0 ) ) <= g::local->weapon ( )->next_primary_attack ( );
		
		return g::local->weapon ( )->next_primary_attack ( ) <= csgo::i::globals->m_curtime && g::local->weapon ( )->next_primary_attack ( ) + g::local->weapon ( )->data ( )->m_fire_rate <= csgo::i::globals->m_curtime;
	};

	/* tickbase manip controller */
	OPTION ( double, tickbase_shift_amount, "Sesame->A->Rage Aimbot->Main->Maximum Doubletap Ticks", oxui::object_slider );
	KEYBIND ( tickbase_key, "Sesame->A->Rage Aimbot->Main->Doubletap Key" );
	OPTION ( int, _fd_mode, "Sesame->B->Other->Other->Fakeduck Mode", oxui::object_dropdown );
	KEYBIND ( _fd_key, "Sesame->B->Other->Other->Fakeduck Key" );

	const auto weapon_data = ( g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) ) ? g::local->weapon ( )->data ( ) : nullptr;
	const auto fire_rate = weapon_data ? weapon_data->m_fire_rate : 0.0f;
	auto tickbase_as_int = std::clamp< int > ( csgo::time2ticks( fire_rate ) - 1, 0, static_cast< int >( tickbase_shift_amount ) );

	if ( !tickbase_key )
		tickbase_as_int = 0;

	if ( g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) && tickbase_as_int && ucmd->m_buttons & 1 && can_shoot( ) && std::abs( ucmd->m_cmdnum - g::dt_recharge_time ) > tickbase_as_int && !g::dt_ticks_to_shift && !( g::local->weapon ( )->item_definition_index ( ) == 64 || g::local->weapon ( )->data ( )->m_type == 0 || g::local->weapon ( )->data ( )->m_type >= 7 ) && !_fd_key ) {
		g::dt_ticks_to_shift = tickbase_as_int;
		g::dt_recharge_time = ucmd->m_cmdnum + tickbase_as_int;
	}
}