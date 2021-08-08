#include "../menu/menu.hpp"
#include "clantag.hpp"
#include "../menu/options.hpp"
#include "prediction.hpp"

int iter = 0;
std::string last_tag = _( "" );

uint64_t current_plist_player = 0;
uint64_t player_to_steal_tag_from = 0;

void features::clantag::run( ucmd_t* ucmd ) {
	static auto& clantag = options::vars [ _ ( "misc.effects.clantag" ) ].val.b;
	static auto& clantag_animation = options::vars [ _ ( "misc.effects.clantag_animation" ) ].val.i;
	static auto& clantag_text = options::vars [ _("misc.effects.clantag_text")].val.s;

	static auto player_rsc_ptr = pattern::search ( _ ( "client.dll" ), _ ( "8B 3D ? ? ? ? 85 FF 0F 84 ? ? ? ? 81 C7" ) ).add ( 2 ).deref ( ).get< uintptr_t > ( );
	static auto off_clan = netvars::get_offset ( _ ( "DT_CSPlayerResource->m_szClan" ) );

	using fn = int( __fastcall* )( const char*, const char* );
	static auto change_clantag = pattern::search( _( "engine.dll" ), _( "53 56 57 8B DA 8B F9 FF 15" ) ).get< fn >( );

	/* remove players if they leave the game or do no longer exist */
	bool found_current_plist_player = false;
	bool found_player_to_steal_tag_from = false;
	int player_to_steal_tag_from_idx = 0;

	for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
		const auto ent = cs::i::ent_list->get<player_t*> ( i );

		if ( !ent || !ent->is_player ( ) )
			continue;

		player_info_t player_info { };

		if ( !cs::i::engine->get_player_info ( i, &player_info ) || player_info.m_fake_player )
			continue;

		if ( current_plist_player == player_info.m_steam_id )
			found_current_plist_player = true;

		if ( player_to_steal_tag_from == player_info.m_steam_id ) {
			player_to_steal_tag_from_idx = ent->idx ( );
			found_player_to_steal_tag_from = true;
		}
	}

	if ( !found_current_plist_player )
		current_plist_player = 0;

	if ( !found_player_to_steal_tag_from )
		player_to_steal_tag_from = 0;

	/* start clantag changer/stealer */
	if ( !clantag && !player_to_steal_tag_from_idx ) {
		if ( last_tag != _ ( "" ) ) {
			change_clantag ( _ ( "" ), _ ( "" ) );
		}

		last_tag = _( "" );
		return;
	}

	auto iter = g::server_tick * 4;
	const std::string tag = clantag_text;

	if ( player_to_steal_tag_from_idx ) {
		const auto player_rsc = *reinterpret_cast< uintptr_t* > ( player_rsc_ptr );

		if ( player_rsc ) {
			char pl_tag [ 16 ] { '\0' };
			strcpy_s ( pl_tag, reinterpret_cast< char* > ( player_rsc + off_clan + player_to_steal_tag_from_idx * sizeof( pl_tag ) ) );
			const auto tag = std::string ( pl_tag );

			if ( last_tag != tag ) {
				change_clantag ( tag.c_str ( ), tag.c_str ( ) );
			}

			last_tag = tag;
		}
		else {
			if ( last_tag != tag )
				change_clantag ( "", "" );

			last_tag = "";
		}
	}
	else {
		switch ( clantag_animation ) {
		case 0: {
			if ( last_tag != tag ) { change_clantag ( tag.c_str ( ), tag.c_str ( ) ); }
			last_tag = tag;
		} break;
		case 1: {
			auto marquee_tag = tag + _ ( "    " );
			std::rotate ( marquee_tag.rbegin ( ), marquee_tag.rbegin ( ) + ( iter % ( tag.length ( ) - 1 + 4 ) ), marquee_tag.rend ( ) );
			if ( last_tag != marquee_tag ) { change_clantag ( marquee_tag.c_str ( ), marquee_tag.c_str ( ) ); }
			last_tag = marquee_tag;
		} break;
		case 2: {
			auto dynamic_tag = tag;
			iter %= tag.length ( ) + 4;
			if ( iter < tag.length ( ) ) dynamic_tag [ iter ] = toupper ( dynamic_tag [ iter ] );
			if ( last_tag != dynamic_tag ) { change_clantag ( dynamic_tag.c_str ( ), dynamic_tag.c_str ( ) ); }
			last_tag = dynamic_tag;
		} break;
		case 3: {
			const std::string tag = !( iter % 4 ) ? _ ( "❤" ) : _ ( "♡" );
			if ( tag != last_tag ) { change_clantag ( tag.c_str ( ), tag.c_str ( ) ); }
			last_tag = tag;
		} break;
		case 4: {
			static int prev_array_stage = 0;
			static char tag_stages [ ] = {
				0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0b,
				0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
				0x12, 0x13, 0x14, 0x15, 0x16
			};

			auto time = 0.3f / cs::i::globals->m_ipt;
			auto iter = g::server_tick / static_cast<int> ( time + 0.5f ) % 30;
			auto new_array_stage = tag_stages [ iter ];

			if ( prev_array_stage != new_array_stage ) {
				char gamesense [ 40 ] = { 0 };
				strcpy ( gamesense, _("             gamesense                ") );

				char* current_tag = &gamesense [ new_array_stage ];
				current_tag [ 15 ] = '\0';
				change_clantag ( current_tag, current_tag );
				prev_array_stage = new_array_stage;

				last_tag = current_tag;
			}
		} break;
		}
	}
}