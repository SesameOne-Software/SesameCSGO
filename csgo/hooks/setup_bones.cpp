#include "setup_bones.hpp"

#include "../features/ragebot.hpp"

#undef min
#undef max

bool hooks::bone_setup::allow = false;

decltype( &hooks::setup_bones ) hooks::old::setup_bones = nullptr;

int last_anim_framecount = 0;
std::array<matrix3x4_t , 128> temp_mat;

bool __fastcall hooks::setup_bones( REG, matrix3x4_t* out, int max_bones, int mask, float curtime ) {
	static auto& g_model_bone_counter = *pattern::search ( _ ( "client.dll" ), _ ( "3B 05 ? ? ? ? 0F 84 ? ? ? ? 8B 47" ) ).add ( 2 ).deref ( ).get<int*> ( );
	static const auto attachmenthelper_fn = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 EC 48 53 8B 5D" ) ).get<void ( __thiscall* )( void*, void* )> ( );

	const auto pl = reinterpret_cast< player_t* > ( reinterpret_cast< uintptr_t >( ecx ) - 4 );
	
	if ( !bone_setup::allow && pl && pl->is_player ( ) && pl->health() > 0 ) {
		if ( *reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( pl ) + 0x2680 ) != g_model_bone_counter ) {
			memcpy ( pl->bone_cache ( ), anims::usable_bones [ pl->idx ( ) ].data ( ), sizeof ( matrix3x4_t ) * pl->bone_count ( ) );
			
			for ( auto i = 0; i < pl->bone_count(); i++ )
				pl->bone_cache ( ) [ i ].set_origin ( pl->bone_cache ( ) [ i ].origin ( ) - anims::usable_origin [ pl->idx ( ) ] + pl->render_origin ( ) );

			attachmenthelper_fn ( pl, *reinterpret_cast< void** > ( reinterpret_cast< uintptr_t >( pl ) + 0x293C ) );
			*reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( pl ) + 0x2680 ) = g_model_bone_counter;
		}

		if ( out ) {
			if ( max_bones >= pl->bone_count ( ) )
				memcpy ( out, pl->bone_cache ( ), sizeof ( matrix3x4_t ) * pl->bone_count ( ) );
			else
				return false;
		}

		return true;
	}

	return old::setup_bones( REG_OUT, out, max_bones, mask, curtime );
}