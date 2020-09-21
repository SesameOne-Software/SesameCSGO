#include "../menu/menu.hpp"
#include "clantag.hpp"
#include "../menu/options.hpp"
#include "prediction.hpp"

int iter = 0;
std::string last_tag = _( "" );

extern int current_plist_player;
int player_to_steal_tag_from = 0;

void features::clantag::run( ucmd_t* ucmd ) {
	static auto& clantag = options::vars [ _ ( "misc.effects.clantag" ) ].val.b;
	static auto& clantag_animation = options::vars [ _ ( "misc.effects.clantag_animation" ) ].val.i;
	static auto& clantag_text = options::vars [ _("misc.effects.clantag_text")].val.s;

	static auto player_rsc_ptr = pattern::search ( _ ( "client.dll" ), _ ( "8B 3D ? ? ? ? 85 FF 0F 84 ? ? ? ? 81 C7" ) ).add ( 2 ).deref ( ).get< uintptr_t > ( );
	static std::ptrdiff_t off_clan = netvars::get_offset ( _ ( "DT_CSPlayerResource->m_szClan" ) );

	using fn = int( __fastcall* )( const char*, const char* );
	static auto change_clantag = pattern::search( _( "engine.dll" ), _( "53 56 57 8B DA 8B F9 FF 15" ) ).get< fn >( );

	if ( !clantag && !player_to_steal_tag_from ) {
		if ( last_tag != _ ( "" ) ) {
			change_clantag ( _ ( "" ), _ ( "" ) );
		}

		last_tag = _( "" );
		return;
	}

	auto iter = static_cast< int >( prediction::curtime() * 4.0f );
	const std::string tag = clantag_text;

	if ( player_to_steal_tag_from ) {
		const auto player_rsc = *reinterpret_cast< uintptr_t* > ( player_rsc_ptr );

		if ( player_rsc ) {
			char pl_tag [ 16 ] { '\0' };
			strcpy_s ( pl_tag, reinterpret_cast< char* > ( player_rsc + off_clan + player_to_steal_tag_from * sizeof pl_tag ) );
			const auto tag = std::string ( pl_tag );

			if ( last_tag != tag ) {
				change_clantag ( tag.data ( ), tag.data ( ) );
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
			if ( last_tag != tag ) { change_clantag ( tag.data ( ), tag.data ( ) ); }
			last_tag = tag;
		} break;
		case 1: {
			auto marquee_tag = tag + _ ( "    " );
			std::rotate ( marquee_tag.rbegin ( ), marquee_tag.rbegin ( ) + ( iter % ( tag.length ( ) - 1 + 4 ) ), marquee_tag.rend ( ) );
			if ( last_tag != marquee_tag ) { change_clantag ( marquee_tag.data ( ), marquee_tag.data ( ) ); }
			last_tag = marquee_tag;
		} break;
		case 2: {
			auto dynamic_tag = tag;
			iter %= tag.length ( ) - 1 + 4;
			if ( iter < tag.length ( ) ) dynamic_tag [ iter ] = std::toupper ( dynamic_tag [ iter ] );
			if ( last_tag != dynamic_tag ) { change_clantag ( dynamic_tag.data ( ), dynamic_tag.data ( ) ); }
			last_tag = dynamic_tag;
		} break;
		case 3: {
			const std::string tag = !( iter % 4 ) ? _ ( "\xE2\x9D\xA4" ) : _ ( "\xE2\x99\xA1" );
			if ( tag != last_tag ) { change_clantag ( tag.data ( ), tag.data ( ) ); }
			last_tag = tag;
		} break;
		}
	}
}