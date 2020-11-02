#pragma once
#include <windows.h>
#include <memory>

#define SESAME_VERSION "Sesame v3.2.0"

struct ucmd_t;
class vec3_t;
class player_t;
class cvar_t;

enum class round_t : int {
	starting = 0,
	in_progress,
	ending,
};

typedef struct _Loader_Info {
	HMODULE hMod;
	size_t hMod_sz;
	void* section;
	size_t section_sz;
	const char* init;
	unsigned char* key;
	unsigned char* iv;
	const char* username;
	const char* avatar;
	size_t avatar_sz;
	char padding [ 24 ];
}Loader_Info, * PLoader_Info;

namespace g {
	extern bool unload;

	extern player_t* local;
	extern ucmd_t* ucmd;
	extern ucmd_t sent_cmd;
	extern vec3_t angles;
	extern bool hold_aim;
	extern bool next_tickbase_shot;
	extern bool send_packet;
	extern int shifted_tickbase;
	extern int shifted_amount;
	extern int dt_ticks_to_shift;
	extern int dt_recharge_time;
	extern int cock_ticks;
	extern int tickbase_at_shift;
	extern bool can_fire_revolver;
	extern round_t round;
	extern PLoader_Info loader_data;

	namespace resources {
#include "base85.hpp"
#include "resources/sesame_icons.hpp"
#include "resources/sesame_ui.hpp"

		inline uint8_t* sesame_icons;
		inline uint8_t* sesame_ui;

		void init ( );
	}

	namespace cvars {
		inline cvar_t* r_drawstaticprops = nullptr;
		inline cvar_t* r_aspectratio = nullptr;
		inline cvar_t* cl_updaterate = nullptr;
		inline cvar_t* cl_cmdrate = nullptr;
		inline cvar_t* sv_gravity = nullptr;
		inline cvar_t* sv_jump_impulse = nullptr;
		inline cvar_t* sv_maxusrcmdprocessticks = nullptr;
		inline cvar_t* sv_maxusrcmdprocessticks_holdaim = nullptr;
		inline cvar_t* molotov_throw_detonate_time = nullptr;
		inline cvar_t* weapon_molotov_maxdetonateslope = nullptr;
		inline cvar_t* weapon_accuracy_nospread = nullptr;
		inline cvar_t* cl_sidespeed = nullptr;
		inline cvar_t* cl_forwardspeed = nullptr;
		inline cvar_t* mp_solid_teammates = nullptr;
		inline cvar_t* sv_clockcorrection_msecs = nullptr;
		inline cvar_t* weapon_recoil_scale = nullptr;
		inline cvar_t* mp_friendlyfire = nullptr;
		inline cvar_t* sv_maxunlag = nullptr;
		inline cvar_t* sv_minupdaterate = nullptr;
		inline cvar_t* sv_maxupdaterate = nullptr;
		inline cvar_t* cl_interp_ratio = nullptr;
		inline cvar_t* cl_interp = nullptr;
		inline cvar_t* sv_client_min_interp_ratio = nullptr;
		inline cvar_t* sv_client_max_interp_ratio = nullptr;
		inline cvar_t* sv_friction = nullptr;
		inline cvar_t* sv_stopspeed = nullptr;

		void init ( );
	}
}