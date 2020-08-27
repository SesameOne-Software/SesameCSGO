#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	int __fastcall emit_sound ( REG, void* filter, int ent_idx, int chan, const char* sound_entry, unsigned int sound_entry_hash, const char* sample, float volume, float attenuation, int seed, int flags, int pitch, const vec3_t* origin, const vec3_t* dir, vec3_t* vec_origins, bool update_positions, float sound_time, int speaker_ent, void* sound_params );

	namespace old {
		extern decltype( &hooks::emit_sound ) emit_sound;
	}
}