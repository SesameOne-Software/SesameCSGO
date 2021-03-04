#include "do_extra_bone_processing.hpp"

decltype( &hooks::do_extra_bone_processing ) hooks::old::do_extra_bone_processing = nullptr;

void __fastcall hooks::do_extra_bone_processing ( REG, int a2, int a3, int a4, int a5, int a6, int a7 ) {
	const auto ent = reinterpret_cast< player_t* >( ecx );

	if (!ent || !ent->animstate())
		return old::do_extra_bone_processing( REG_OUT , a2 , a3 , a4 , a5 , a6 , a7 );

	const auto backup_on_ground = ent->animstate( )->m_on_ground;
	ent->animstate( )->m_on_ground = false;
	old::do_extra_bone_processing( REG_OUT , a2 , a3 , a4 , a5 , a6 , a7 );
	ent->animstate( )->m_on_ground = backup_on_ground;
}