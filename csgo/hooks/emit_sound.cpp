#include "emit_sound.hpp"
#include "../menu/menu.hpp"

decltype( &hooks::emit_sound ) hooks::old::emit_sound = nullptr;

int __fastcall hooks::emit_sound ( REG, void* filter, int ent_idx, int chan, const char* sound_entry, unsigned int sound_entry_hash, const char* sample, float volume, float attenuation, int seed, int flags, int pitch, const vec3_t* origin, const vec3_t* dir, vec3_t* vec_origins, bool update_positions, float sound_time, int speaker_ent, void* sound_params ) {
	OPTION ( double, revolver_cock_volume, "Sesame->E->Effects->Main->Revolver Cock Volume", oxui::object_slider );
	OPTION ( double, weapon_volume, "Sesame->E->Effects->Main->Weapon Volume", oxui::object_slider );

	if ( !strcmp ( sound_entry, _ ( "Weapon_Revolver.Prepare" ) ) && revolver_cock_volume != 1.0 )
		volume *= revolver_cock_volume;
	else if ( strstr ( sound_entry, _ ( "Weapon_" ) ) )
		volume *= weapon_volume;

	return old::emit_sound ( REG_OUT, filter, ent_idx, chan, sound_entry, sound_entry_hash, sample, volume, attenuation, seed, flags, pitch, origin, dir, vec_origins, update_positions, sound_time, speaker_ent, sound_params );
}