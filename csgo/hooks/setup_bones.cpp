#include "setup_bones.hpp"

#include "../features/ragebot.hpp"

bool hooks::bone_setup::allow = false;

decltype( &hooks::setup_bones ) hooks::old::setup_bones = nullptr;

bool __fastcall hooks::setup_bones( REG, matrix3x4_t* out, int max_bones, int mask, float curtime ) {
	static auto setup_bones_ret = pattern::search( _( "client.dll" ), _( "E8 ? ? ? ? 5E 5D C2 10 00 32 C0" ) ).add( 5 ).get< int* >( );

	const auto pl = reinterpret_cast< player_t* > ( uintptr_t ( ecx ) - 4 );

	auto call_original = [ & ] ( ) -> bool {
		const auto backup_flags = *reinterpret_cast< int* >( uintptr_t ( pl ) + 0xe8 );

		*reinterpret_cast< int* >( uintptr_t ( pl ) + 0xA30 ) = cs::i::globals->m_framecount;
		*reinterpret_cast< int* >( uintptr_t ( pl ) + 0xA28 ) = 0;
		*reinterpret_cast< int* >( uintptr_t ( pl ) + 0xA68 ) = 0;

		*reinterpret_cast< int* >( uintptr_t ( pl ) + 0xE8 ) |= 8;

		const auto backup_frametime = cs::i::globals->m_frametime;

		cs::i::globals->m_frametime = 666.0f;

		const auto ret = old::setup_bones ( REG_OUT, out, max_bones, mask, curtime );

		cs::i::globals->m_frametime = backup_frametime;

		*reinterpret_cast< int* >( uintptr_t ( pl ) + 0xe8 ) = backup_flags;

		return ret;
	};

	return call_original ( );

	if ( pl && pl->is_player ( ) ) {
		if ( pl == g::local ) {
			return call_original ( );
		}
		else {
			if ( anims::frames [ pl->idx ( ) ].empty ( ) )
				return call_original ( );

			const auto target_frame = std::find_if ( anims::frames [ pl->idx ( ) ].begin ( ), anims::frames [ pl->idx ( ) ].end ( ), [ & ] ( const anims::animation_frame_t& frame ) { return frame.m_anim_update; } );

			if ( target_frame == anims::frames [ pl->idx ( ) ].end ( ) )
				return call_original ( );

			if ( out ) {
				if ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 3 == 0 )
					memcpy ( out, target_frame->m_matrix1.data ( ), sizeof ( target_frame->m_matrix1 ) );
				else if ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 3 == 1 )
					memcpy ( out, target_frame->m_matrix2.data ( ), sizeof ( target_frame->m_matrix2 ) );
				else
					memcpy ( out, target_frame->m_matrix3.data ( ), sizeof ( target_frame->m_matrix3 ) );
			}
		}

		return true;
	}

	return old::setup_bones( REG_OUT, out, max_bones, mask, curtime );
}