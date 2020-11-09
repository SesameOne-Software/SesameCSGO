#include "globals.hpp"

#include "base85.hpp"
#include "resources/sesame_icons.hpp"
#include "resources/sesame_ui.hpp"

#include "sdk/sdk.hpp"

bool g::unload = false;

round_t g::round = round_t::in_progress;
player_t* g::local = nullptr;
ucmd_t* g::ucmd = nullptr;
ucmd_t g::sent_cmd { };
vec3_t g::angles = vec3_t ( 0.0f, 0.0f, 0.0f );
bool g::hold_aim = false;
bool g::send_packet = true;
int g::cock_ticks = 0;
bool g::can_fire_revolver = false;
PLoader_Info g::loader_data;

void g::resources::init ( ) {
	sesame_icons = ( uint8_t* ) sesame_icons_data;
	sesame_ui = ( uint8_t* ) sesame_ui_data;
}

void g::cvars::init ( ) {
	r_drawstaticprops = csgo::i::cvar->find ( _ ( "r_drawstaticprops" ) );
	r_aspectratio = csgo::i::cvar->find ( _ ( "r_aspectratio" ) );
	cl_updaterate = csgo::i::cvar->find ( _ ( "cl_updaterate" ) );
	cl_cmdrate = csgo::i::cvar->find ( _ ( "cl_cmdrate" ) );
	sv_gravity = csgo::i::cvar->find ( _ ( "sv_gravity" ) );
	sv_jump_impulse = csgo::i::cvar->find ( _ ( "sv_jump_impulse" ) );
	sv_maxusrcmdprocessticks = csgo::i::cvar->find ( _ ( "sv_maxusrcmdprocessticks" ) );
	sv_maxusrcmdprocessticks_holdaim = csgo::i::cvar->find ( _ ( "sv_maxusrcmdprocessticks_holdaim" ) );
	molotov_throw_detonate_time = csgo::i::cvar->find ( _ ( "molotov_throw_detonate_time" ) );
	weapon_molotov_maxdetonateslope = csgo::i::cvar->find ( _ ( "weapon_molotov_maxdetonateslope" ) );
	weapon_accuracy_nospread = csgo::i::cvar->find ( _ ( "weapon_accuracy_nospread" ) );
	cl_sidespeed = csgo::i::cvar->find ( _ ( "cl_sidespeed" ) );
	cl_forwardspeed = csgo::i::cvar->find ( _ ( "cl_forwardspeed" ) );
	mp_solid_teammates = csgo::i::cvar->find ( _ ( "mp_solid_teammates" ) );
	sv_clockcorrection_msecs = csgo::i::cvar->find ( _ ( "sv_clockcorrection_msecs" ) );
	weapon_recoil_scale = csgo::i::cvar->find ( _ ( "weapon_recoil_scale" ) );
	mp_friendlyfire = csgo::i::cvar->find ( _ ( "mp_friendlyfire" ) );
	sv_maxunlag = csgo::i::cvar->find ( _ ( "sv_maxunlag" ) );
	sv_minupdaterate = csgo::i::cvar->find ( _ ( "sv_minupdaterate" ) );
	sv_maxupdaterate = csgo::i::cvar->find ( _ ( "sv_maxupdaterate" ) );
	cl_interp_ratio = csgo::i::cvar->find ( _ ( "cl_interp_ratio" ) );
	cl_interp = csgo::i::cvar->find ( _ ( "cl_interp" ) );
	sv_client_min_interp_ratio = csgo::i::cvar->find ( _ ( "sv_client_min_interp_ratio" ) );
	sv_client_max_interp_ratio = csgo::i::cvar->find ( _ ( "sv_client_max_interp_ratio" ) );
	sv_friction = csgo::i::cvar->find ( _ ( "sv_friction" ) );
	sv_stopspeed = csgo::i::cvar->find ( _ ( "sv_stopspeed" ) );
}