#include "esp.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "ragebot.hpp"
#include "../animations/resolver.hpp"
#include "other_visuals.hpp"
#include "autowall.hpp"
#include <locale>

#include "../renderer/render.hpp"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"

#undef min
#undef max

float box_alpha = 0.0f;

void draw_esp_box( int x , int y , int w , int h , bool dormant , const options::option::colorf& esp_box_color ) {
	render::outline( x - 1 , y - 1 , w + 2 , h + 2 , rgba( 0 , 0 , 0 , std::clamp< int >( esp_box_color.a * 60.0f * box_alpha , 0 , 60 ) ) );
	render::outline( x , y , w , h , dormant ? rgba( 150 , 150 , 150 , static_cast< int > ( esp_box_color.a * 255.0f * box_alpha ) ) : rgba( static_cast< int > ( esp_box_color.r * 255.0f ) , static_cast< int > ( esp_box_color.g * 255.0f ) , static_cast< int > ( esp_box_color.b * 255.0f ) , static_cast< int >( esp_box_color.a * 255.0f * box_alpha ) ) );
}

auto cur_offset_left_height = 0;
auto cur_offset_right_height = 0;
auto cur_offset_left = 4;
auto cur_offset_right = 4;
auto cur_offset_bottom = 4;
auto cur_offset_top = 4;

enum esp_type_t {
	esp_type_bar = 0 ,
	esp_type_text ,
	esp_type_number ,
	esp_type_flag
};

void draw_esp_widget( const ImRect& box , const options::option::colorf& widget_color , esp_type_t type , bool show_value , const int orientation , bool dormant , double value , double max , std::string to_print = _( "" ) ) {
	int esp_bar_thickness = 2;

	uint32_t clr1 = rgba( 0 , 0 , 0 , std::clamp< int >( static_cast< float >( widget_color.a * 255.0f ) / 2.0f , 0 , 125 ) );
	uint32_t clr = rgba( static_cast< int > ( widget_color.r * 255.0f ) , static_cast< int > ( widget_color.g * 255.0f ) , static_cast< int > ( widget_color.b * 255.0f ) , static_cast< int > ( widget_color.a * 255.0f ) );

	if ( dormant ) {
		clr1 = rgba( 0 , 0 , 0 , std::clamp< int >( static_cast< float >( widget_color.a * 255.0f ) / 2.0f * box_alpha , 0 , 125 ) );
		clr = rgba( 150 , 150 , 150 , static_cast< int >( widget_color.a * 255.0f * box_alpha ) );
	}

	switch ( type ) {
	case esp_type_t::esp_type_bar: {
		const auto sval = std::to_string( static_cast< int >( value ) );

		vec3_t text_size;
		render::text_size( sval , _( "esp_font" ) , text_size );

		const auto fraction = std::clamp( value / max , 0.0 , 1.0 );
		const auto calc_height = fraction * box.Max.y;

		switch ( orientation ) {
		case features::esp_placement_t::esp_placement_left:
			render::rect( box.Min.x - cur_offset_left - esp_bar_thickness , box.Min.y + ( box.Max.y - calc_height ) , esp_bar_thickness , calc_height , clr );
			render::outline( box.Min.x - cur_offset_left - esp_bar_thickness - 1, box.Min.y - 1, esp_bar_thickness + 2, box.Max.y + 2, clr1 );

			if ( show_value )
				render::text( box.Min.x - cur_offset_left - esp_bar_thickness + 1 + esp_bar_thickness / 2 - text_size.x / 2 , box.Min.y + ( box.Max.y - calc_height ) + 1 - text_size.y / 2 , sval , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast<int>( 255.0f * box_alpha ) ) , true );
			cur_offset_left += 7;
			break;
		case features::esp_placement_t::esp_placement_right:
			render::rect( box.Min.x + box.Max.x + cur_offset_right , box.Min.y + ( box.Max.y - calc_height ) , esp_bar_thickness , calc_height , clr );
			render::outline( box.Min.x + box.Max.x + cur_offset_right - 1, box.Min.y - 1, esp_bar_thickness + 2, box.Max.y + 2, clr1 );

			if ( show_value )
				render::text( box.Min.x + box.Max.x + cur_offset_right + 1 + esp_bar_thickness / 2 - text_size.x / 2 , box.Min.y + ( box.Max.y - calc_height ) + 1 - text_size.y / 2 , sval , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_right += 7;
			break;
		case features::esp_placement_t::esp_placement_bottom:
			render::rect( box.Min.x + 1 , box.Min.y + box.Max.y + cur_offset_bottom , static_cast< float >( box.Max.x ) * fraction , esp_bar_thickness , clr );
			render::outline( box.Min.x - 1, box.Min.y + box.Max.y + cur_offset_bottom - 1, box.Max.x + 2, esp_bar_thickness + 2, clr1 );

			if ( show_value )
				render::text( box.Min.x + 1 + static_cast< float >( box.Max.x ) * fraction + 1 - text_size.x / 2 , box.Min.y + box.Max.y + cur_offset_bottom + 1 + esp_bar_thickness / 2 - text_size.y / 2 , sval , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_bottom += 7;
			break;
		case features::esp_placement_t::esp_placement_top:
			render::rect( box.Min.x + 1 , box.Min.y - cur_offset_top - esp_bar_thickness , static_cast< float >( box.Max.x ) * fraction , esp_bar_thickness , clr );
			render::outline( box.Min.x - 1, box.Min.y - cur_offset_top - esp_bar_thickness - 1, box.Max.x + 2, esp_bar_thickness + 2, clr1 );

			if ( show_value )
				render::text( box.Min.x + 1 + static_cast< float >( box.Max.x ) * fraction + 1 - text_size.x / 2 , box.Min.y - cur_offset_top - esp_bar_thickness + 1 + esp_bar_thickness / 2 - text_size.y / 2 , sval , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_top += 7;
			break;
		}
	} break;
	case esp_type_t::esp_type_text: {
		std::string as_str = to_print;

		vec3_t text_size;
		render::text_size( as_str , _( "esp_font" ) , text_size );

		switch ( orientation ) {
		case features::esp_placement_t::esp_placement_left:
			render::gradient( box.Min.x - cur_offset_left - text_size.x - 2.0f , box.Min.y + cur_offset_left_height - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , clr , true );
			render::gradient( box.Min.x - cur_offset_left - text_size.x - 2.0f + ( text_size.x + 4.0f ) * 0.5f , box.Min.y + cur_offset_left_height - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , clr , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , true );
			render::rect( box.Min.x - cur_offset_left - text_size.x - 2.0f , box.Min.y + cur_offset_left_height - 2.0f , text_size.x + 4.0f , text_size.y + 3.0f , rgba( 0 , 0 , 0 , static_cast< int >( 72.0f * box_alpha ) ) );
			render::text( box.Min.x - cur_offset_left - text_size.x , box.Min.y + cur_offset_left_height , as_str , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_left_height += text_size.y + 6;
			break;
		case features::esp_placement_t::esp_placement_right:
			render::gradient( box.Min.x + cur_offset_right + box.Max.x - 2.0f , box.Min.y + cur_offset_right_height - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , clr , true );
			render::gradient( box.Min.x + cur_offset_right + box.Max.x - 2.0f + ( text_size.x + 4.0f ) * 0.5f , box.Min.y + cur_offset_right_height - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , clr , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , true );
			render::rect( box.Min.x + cur_offset_right + box.Max.x - 2.0f , box.Min.y + cur_offset_right_height - 2.0f , text_size.x + 4.0f , text_size.y + 3.0f , rgba( 0 , 0 , 0 , static_cast< int >( 72.0f * box_alpha ) ) );
			render::text( box.Min.x + cur_offset_right + box.Max.x , box.Min.y + cur_offset_right_height , as_str , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_right_height += text_size.y + 6;
			break;
		case features::esp_placement_t::esp_placement_bottom:
			render::gradient( box.Min.x + box.Max.x / 2 - text_size.x / 2 - 2.0f , box.Min.y + box.Max.y + cur_offset_bottom - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , clr , true );
			render::gradient( box.Min.x + box.Max.x / 2 - text_size.x / 2 - 2.0f + ( text_size.x + 4.0f ) * 0.5f , box.Min.y + box.Max.y + cur_offset_bottom - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , clr , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , true );
			render::rect( box.Min.x + box.Max.x / 2 - text_size.x / 2 - 2.0f , box.Min.y + box.Max.y + cur_offset_bottom - 2.0f , text_size.x + 4.0f , text_size.y + 3.0f , rgba( 0 , 0 , 0 , static_cast< int >( 72.0f * box_alpha ) ) );
			render::text( box.Min.x + box.Max.x / 2 - text_size.x / 2 , box.Min.y + box.Max.y + cur_offset_bottom , as_str , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_bottom += text_size.y + 6;
			break;
		case features::esp_placement_t::esp_placement_top:
			render::gradient( box.Min.x + box.Max.x / 2 - text_size.x / 2 - 2.0f , box.Min.y - cur_offset_top - text_size.y - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , clr , true );
			render::gradient( box.Min.x + box.Max.x / 2 - text_size.x / 2 - 2.0f + ( text_size.x + 4.0f ) * 0.5f , box.Min.y - cur_offset_top - text_size.y - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , clr , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , true );
			render::rect( box.Min.x + box.Max.x / 2 - text_size.x / 2 - 2.0f , box.Min.y - cur_offset_top - text_size.y - 2.0f , text_size.x + 4.0f , text_size.y + 3.0f , rgba( 0 , 0 , 0 , static_cast< int >( 72.0f * box_alpha ) ) );
			render::text( box.Min.x + box.Max.x / 2 - text_size.x / 2 , box.Min.y - cur_offset_top - text_size.y , as_str , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_top += text_size.y + 6;
			break;
		}
	} break;
	case esp_type_t::esp_type_number: {
	} break;
	case esp_type_t::esp_type_flag: {
	} break;
	default: break;
	}
}

struct snd_info_t {
	int m_guid;
	void* m_hfile;
	int m_sound_src;
	int m_channel;
	int m_speaker_ent;
	float m_vol;
	float m_last_spatialized_vol;
	float m_rad;
	int pitch;
	vec3_t* m_origin;
	vec3_t* m_dir;
	bool m_update_positions;
	bool m_is_sentence;
	bool m_dry_mix;
	bool m_speaker;
	bool m_from_server;
	bool m_unk;
};

struct snd_data_t {
	snd_info_t* m_sounds;
	PAD( 8 );
	int m_count;
	PAD( 4 );
};

snd_data_t cached_data;

struct radar_player_t {
	vec3_t pos;
	vec3_t angle;
	vec3_t spotted_map_angle_related;
	uint32_t tab_related;
	PAD ( 12 );
	float spotted_time;
	float spotted_fraction;
	float time;
	PAD ( 4 );
	int player_index;
	int entity_index;
	PAD ( 4 );
	int health;
	char name [ 32 ];
	PAD ( 117 );
	bool spotted;
	PAD ( 138 );
};

struct hud_radar_t {
	PAD ( 332 );
	radar_player_t radar_info [ 65 ];
};

static void* find_hud_element ( const char* name ) {
	static auto hud = pattern::search ( _ ( "client.dll" ), _ ( "B9 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89 46 24" ) ).add ( 1 ).deref ( ).get< void* > ( );
	static auto find_hud_element_func = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28" ) ).get< void* ( __thiscall* )( void*, const char* ) > ( );
	return ( void* ) find_hud_element_func ( hud, name );
}

void features::esp::handle_dynamic_updates( ) {
	if ( !g::local )
		return;

	/* update with radar */
	//auto radar_base = find_hud_element ( _ ( "CCSGO_HudRadar" ) );
	//
	//if ( radar_base ) {
	//	auto hud_radar = reinterpret_cast< hud_radar_t* > ( reinterpret_cast< uintptr_t >( radar_base ) - 20 );
	//
	//	for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
	//		auto player = cs::i::ent_list->get< player_t* > ( i );
	//
	//		if ( !player || !player->is_player ( ) || !player->alive ( ) || !player->dormant ( ) )
	//			continue;
	//
	//		if ( esp_data [ i ].m_radar_pos != hud_radar->radar_info [ i ].pos ) {
	//			esp_data [ i ].m_dormant = true;
	//			esp_data [ i ].m_radar_pos = esp_data [ i ].m_sound_pos = hud_radar->radar_info [ i ].pos;
	//			esp_data [ i ].m_last_seen = g::local ? cs::ticks2time ( g::local->tick_base ( ) ) : cs::i::globals->m_curtime;
	//		}
	//	}
	//}

	/* update with sounds */
	static auto get_active_sounds = pattern::search( _( "engine.dll" ) , _( "55 8B EC 83 E4 F8 81 EC 44 03 00 00 53 56" ) ).get< void( __thiscall* )( snd_data_t* ) >( );

	memset( &cached_data , 0 , sizeof cached_data );
	get_active_sounds( &cached_data );

	if ( !cached_data.m_count )
		return;

	for ( auto i = 0; i < cached_data.m_count; i++ ) {
		const auto sound = cached_data.m_sounds[ i ];

		if ( !sound.m_from_server || !sound.m_sound_src || sound.m_sound_src > 64 || !sound.m_origin || *sound.m_origin == vec3_t( 0.0f , 0.0f , 0.0f ) )
			continue;

		auto pl = cs::i::ent_list->get< player_t* >( sound.m_sound_src );

		if ( !pl || !pl->dormant( ) )
			continue;

		vec3_t end_pos = *sound.m_origin;

		trace_t tr;
		ray_t ray;

		trace_filter_t trace_filter;
		trace_filter.m_skip = pl;

		ray.init( *sound.m_origin + vec3_t( 0.0f , 0.0f , 2.0f ) , *sound.m_origin - vec3_t( 0.0f , 0.0f , 4096.0f ) );
		cs::i::trace->trace_ray( ray , mask_playersolid , &trace_filter , &tr );

		if ( !tr.is_visible( ) )
			end_pos = tr.m_endpos;

		if ( abs ( esp_data [ pl->idx ( ) ].m_last_seen - ( g::local ? cs::ticks2time ( g::local->tick_base ( ) ) : cs::i::globals->m_curtime ) ) > 1.0f ) {
			esp_data [ pl->idx ( ) ].m_sound_pos = end_pos;
			esp_data [ pl->idx ( ) ].m_dormant = true;
			esp_data [ pl->idx ( ) ].m_last_seen = g::local ? cs::ticks2time ( g::local->tick_base ( ) ) : cs::i::globals->m_curtime;
		}
	}
}

void features::esp::reset_dormancy( event_t* event ) {
	auto attacker = cs::i::ent_list->get< player_t* >( cs::i::engine->get_player_for_userid( event->get_int( _( "attacker" ) ) ) );
	auto victim = cs::i::ent_list->get< player_t* >( cs::i::engine->get_player_for_userid( event->get_int( _( "userid" ) ) ) );

	if ( !attacker || !victim || attacker != g::local || victim->team( ) == g::local->team( ) || ( victim->idx( ) <= 0 || victim->idx( ) > 64 ) )
		return;

	if ( !victim->dormant( ) ) {
		esp_data[ victim->idx( ) ].m_dormant = false;
		esp_data[ victim->idx( ) ].m_sound_pos = vec3_t( 0.f , 0.f , 0.f );
	}
}

void features::esp::render( ) {
	if ( !g::local )
		return;

	static auto spawn_time = 0.0f;

	if ( g::local->spawn_time( ) != spawn_time ) {
		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
			esp_data[ i ].m_pl = nullptr;
			esp_data[ i ].m_dormant = true;
			esp_data[ i ].m_first_seen = esp_data[ i ].m_last_seen = 0.0f;
		}

		spawn_time = g::local->spawn_time( );

		return;
	}

	for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
		auto e = cs::i::ent_list->get< player_t* >( i );

		if ( !e || !e->alive( ) || ( e == g::local && !cs::i::input->m_camera_in_thirdperson ) ) {
			esp_data[ i ].m_pl = nullptr;
			esp_data[ i ].m_health = 100.0f;
			continue;
		}

		features::visual_config_t visuals;
		if ( !get_visuals( e , visuals ) ) {
			esp_data[ i ].m_pl = nullptr;
			continue;
		}

		vec3_t flb , brt , blb , frt , frb , brb , blt , flt;
		float left , top , right , bottom;
		
		auto abs_origin = ( e->bone_cache ( ) && !e->dormant ( ) ) ? e->bone_cache ( ) [ 1 ].origin ( ) : e->abs_origin ( );

		if ( e->dormant ( ) && esp_data[ e->idx( ) ].m_sound_pos != vec3_t( 0.f , 0.f , 0.f ) )
			abs_origin = esp_data[ e->idx( ) ].m_sound_pos;

		auto min = e->mins( ) + abs_origin;
		auto max = e->maxs( ) + abs_origin;

		if ( ( e->crouch_amount ( ) < 0.333f || e->crouch_amount ( ) == 1.0f ) && e->bone_cache ( ) && !e->dormant ( ) )
			max.z = e->bone_cache( )[ 8 ].origin( ).z + 12.0f;

		vec3_t points [ ] = {
			vec3_t( min.x, min.y, min.z ),
			vec3_t( min.x, max.y, min.z ),
			vec3_t( max.x, max.y, min.z ),
			vec3_t( max.x, min.y, min.z ),
			vec3_t( max.x, max.y, max.z ),
			vec3_t( min.x, max.y, max.z ),
			vec3_t( min.x, min.y, max.z ),
			vec3_t( max.x, min.y, max.z )
		};

		if ( !cs::render::world_to_screen( flb , points[ 3 ] )
			|| !cs::render::world_to_screen( brt , points[ 5 ] )
			|| !cs::render::world_to_screen( blb , points[ 0 ] )
			|| !cs::render::world_to_screen( frt , points[ 4 ] )
			|| !cs::render::world_to_screen( frb , points[ 2 ] )
			|| !cs::render::world_to_screen( brb , points[ 1 ] )
			|| !cs::render::world_to_screen( blt , points[ 6 ] )
			|| !cs::render::world_to_screen( flt , points[ 7 ] ) ) {
			continue;
		}

		vec3_t arr [ ] = { flb, brt, blb, frt, frb, brb, blt, flt };

		left = flb.x;
		top = flb.y;
		right = flb.x;
		bottom = flb.y;

		for ( auto i = 1; i < 8; i++ ) {
			if ( left > arr[ i ].x )
				left = arr[ i ].x;

			if ( bottom < arr[ i ].y )
				bottom = arr[ i ].y;

			if ( right < arr[ i ].x )
				right = arr[ i ].x;

			if ( top > arr[ i ].y )
				top = arr[ i ].y;
		}

		const auto subtract_w = ( right - left ) / 5;
		const auto subtract_h = ( right - left ) / 16;

		right -= subtract_w / 2;
		left += subtract_w / 2;
		bottom -= subtract_h / 2;
		top += subtract_h / 2;

		esp_data[ e->idx( ) ].m_pl = e;
		esp_data[ e->idx( ) ].m_box.left = left;
		esp_data[ e->idx( ) ].m_box.right = right;
		esp_data[ e->idx( ) ].m_box.bottom = bottom;
		esp_data[ e->idx( ) ].m_box.top = top;
		esp_data[ e->idx( ) ].m_dormant = e->dormant( );
		esp_data [ e->idx ( ) ].m_scoped = e->scoped ( );
		esp_data [ e->idx ( ) ].m_radar_pos = e->origin ( );

		const auto clamped_health = std::clamp( static_cast< float >( e->health( ) ) , 0.0f , 100.0f );
		esp_data[ e->idx( ) ].m_health += ( clamped_health - esp_data[ e->idx( ) ].m_health ) * 2.0f * cs::i::globals->m_frametime;
		esp_data[ e->idx( ) ].m_health = std::clamp( esp_data[ e->idx( ) ].m_health , clamped_health , 100.0f );

		if ( g::round == round_t::starting )
			esp_data[ e->idx( ) ].m_sound_pos = vec3_t( 0.0f, 0.0f, 0.0f );

		if ( !e->dormant( ) ) {
			esp_data[ e->idx( ) ].m_sound_pos = abs_origin; // reset to our current origin since it will show our last sound origin
			esp_data[ e->idx( ) ].m_pos = abs_origin;

			if ( esp_data[ e->idx( ) ].m_first_seen == 0.0f )
				esp_data[ e->idx( ) ].m_first_seen = g::local ? cs::ticks2time ( g::local->tick_base ( ) ) : cs::i::globals->m_curtime;

			esp_data[ e->idx( ) ].m_last_seen = g::local ? cs::ticks2time ( g::local->tick_base ( ) ) : cs::i::globals->m_curtime;

			if ( e->weapon ( ) )
				esp_data [ e->idx ( ) ].m_weapon_name = cs::get_weapon_name ( e->weapon ( )->item_definition_index ( ) );
		}
		else {
			esp_data[ e->idx( ) ].m_first_seen = 0.0f;
		}

		auto dormant_time = std::max< float >( 9.0f/*esp_fade_time*/ , 0.1f );

		if ( esp_data[ e->idx( ) ].m_pl && std::fabsf( ( g::local ? cs::ticks2time ( g::local->tick_base ( ) ) : cs::i::globals->m_curtime ) - esp_data[ e->idx( ) ].m_last_seen ) < dormant_time ) {
			auto calc_alpha = [ & ] ( float time , float fade_time , bool add = false ) {
				return ( std::clamp< float >( dormant_time - ( std::clamp< float >( add ? ( dormant_time - std::clamp< float >( std::fabsf( ( g::local ? cs::ticks2time ( g::local->tick_base ( ) ) : cs::i::globals->m_curtime ) - time ) , 0.0f , dormant_time ) ) : std::fabsf( cs::i::globals->m_curtime - time ) , std::max< float >( dormant_time - fade_time , 0.0f ) , dormant_time ) ) , 0.0f , fade_time ) / fade_time );
			};

			if ( !esp_data[ e->idx( ) ].m_dormant )
				box_alpha = calc_alpha( esp_data[ e->idx( ) ].m_first_seen , 0.6f , true );
			else
				box_alpha = calc_alpha( esp_data[ e->idx( ) ].m_last_seen , 2.0f );

			std::string name = _( "" );

			player_info_t info;
			if ( cs::i::engine->get_player_info( e->idx( ) , &info ) )
				name = info.m_name;

			if ( name.size( ) > 20 ) {
				name.resize( 19 );
				name += _( "..." );
			}

			if ( visuals.esp_box )
				draw_esp_box( left , top , right - left , bottom - top , esp_data[ e->idx( ) ].m_dormant , visuals.box_color );

			cur_offset_left_height = 0;
			cur_offset_right_height = 0;
			cur_offset_left = 4;
			cur_offset_right = 4;
			cur_offset_bottom = 4;
			cur_offset_top = 4;

			ImVec4 esp_rect { left, top, right - left, bottom - top };

			if ( visuals.health_bar )
				draw_esp_widget( esp_rect , visuals.health_bar_color , esp_type_bar , esp_data [ e->idx ( ) ].m_health >= 100.0f ? false : visuals.value_text , visuals.health_bar_placement , esp_data[ e->idx( ) ].m_dormant , esp_data[ e->idx( ) ].m_health , 100.0 );

			if ( visuals.ammo_bar && e->weapon( ) && e->weapon( )->data( ) && e->weapon( )->ammo( ) != -1 )
				draw_esp_widget( esp_rect , visuals.ammo_bar_color , esp_type_bar , visuals.value_text , visuals.ammo_bar_placement , esp_data[ e->idx( ) ].m_dormant , e->weapon( )->ammo( ) , e->weapon( )->data( )->m_max_clip );

			if ( visuals.desync_bar )
				draw_esp_widget( esp_rect , visuals.desync_bar_color , esp_type_bar , visuals.value_text , visuals.desync_bar_placement , esp_data[ e->idx( ) ].m_dormant , e->desync_amount( ) , 58.0 );

			if ( visuals.nametag )
				draw_esp_widget( esp_rect , visuals.name_color , esp_type_text , visuals.value_text , visuals.nametag_placement , esp_data[ e->idx( ) ].m_dormant , 0.0 , 0.0 , name );

			if ( visuals.weapon_name )
				draw_esp_widget( esp_rect , visuals.weapon_color , esp_type_text , visuals.value_text , visuals.weapon_name_placement , esp_data[ e->idx( ) ].m_dormant , 0.0 , 0.0 , esp_data[ e->idx( ) ].m_weapon_name );

			if ( visuals.fakeduck_flag && esp_data[ e->idx( ) ].m_fakeducking )
				draw_esp_widget( esp_rect , visuals.fakeduck_color , esp_type_text , visuals.value_text , visuals.fakeduck_flag_placement , esp_data[ e->idx( ) ].m_dormant , 0.0 , 0.0 , _( "FD" ) );

			if ( visuals.reloading_flag && esp_data[ e->idx( ) ].m_reloading )
				draw_esp_widget( esp_rect , visuals.reloading_color , esp_type_text , visuals.value_text , visuals.reloading_flag_placement , esp_data[ e->idx( ) ].m_dormant , 0.0 , 0.0 , _( "Reload" ) );

			if ( visuals.fatal_flag && esp_data[ e->idx( ) ].m_fatal )
				draw_esp_widget( esp_rect , visuals.fatal_color , esp_type_text , visuals.value_text , visuals.fatal_flag_placement , esp_data[ e->idx( ) ].m_dormant , 0.0 , 0.0 , _( "Fatal" ) );
			
			if ( visuals.zoom_flag && esp_data [ e->idx ( ) ].m_scoped )
				draw_esp_widget ( esp_rect, visuals.fatal_color, esp_type_text, visuals.value_text, visuals.fatal_flag_placement, esp_data [ e->idx ( ) ].m_dormant, 0.0, 0.0, _ ( "Zoom" ) );
			
			/* DEBUGGING STUFF */
			//if ( e != g::local && e->team() != g::local->team() ) {
			//	for ( auto i = 0; i < 13; i++ ) {
			//		if ( i != 7 )
			//			continue;
			//
			//		std::string output = "animlayer ";
			//		output.append ( std::to_string ( i ) + ":\n" );
			//		output.append ( "weight: " + std::to_string ( anims::resolver::rdata::latest_layers [ e->idx ( ) ][ i ].m_weight ) + ":\n" );
			//		output.append ( "cycle: " + std::to_string ( anims::resolver::rdata::latest_layers [ e->idx ( ) ][ i ].m_cycle ) + ":\n" );
			//		output.append ( "rate: " + std::to_string ( anims::resolver::rdata::latest_layers [ e->idx ( ) ][ i ].m_playback_rate ) + ":\n" );
			//
			//		draw_esp_widget ( esp_rect, visuals.weapon_color, esp_type_text, visuals.value_text, esp_placement_right, esp_data [ e->idx ( ) ].m_dormant, 0.0, 0.0, output );
			//	}
			//}
		}
	}
}