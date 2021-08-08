#include "globals.hpp"

#include "base85.hpp"
#include "resources/sesame_icons.hpp"

#include "sdk/sdk.hpp"

void g::resources::init ( ) {
	sesame_icons = ( uint8_t* ) sesame_icons_data;
}

void g::cvars::init ( ) {
	r_drawstaticprops = cs::i::cvar->find ( _ ( "r_drawstaticprops" ) );
	r_aspectratio = cs::i::cvar->find ( _ ( "r_aspectratio" ) );
	cl_updaterate = cs::i::cvar->find ( _ ( "cl_updaterate" ) );
	cl_cmdrate = cs::i::cvar->find ( _ ( "cl_cmdrate" ) );
	sv_gravity = cs::i::cvar->find ( _ ( "sv_gravity" ) );
	sv_jump_impulse = cs::i::cvar->find ( _ ( "sv_jump_impulse" ) );
	sv_maxusrcmdprocessticks = cs::i::cvar->find ( _ ( "sv_maxusrcmdprocessticks" ) );
	sv_maxusrcmdprocessticks_holdaim = cs::i::cvar->find ( _ ( "sv_maxusrcmdprocessticks_holdaim" ) );
	molotov_throw_detonate_time = cs::i::cvar->find ( _ ( "molotov_throw_detonate_time" ) );
	weapon_molotov_maxdetonateslope = cs::i::cvar->find ( _ ( "weapon_molotov_maxdetonateslope" ) );
	weapon_accuracy_nospread = cs::i::cvar->find ( _ ( "weapon_accuracy_nospread" ) );
	cl_sidespeed = cs::i::cvar->find ( _ ( "cl_sidespeed" ) );
	cl_upspeed = cs::i::cvar->find ( _ ( "cl_upspeed" ) );
	cl_forwardspeed = cs::i::cvar->find ( _ ( "cl_forwardspeed" ) );
	mp_solid_teammates = cs::i::cvar->find ( _ ( "mp_solid_teammates" ) );
	sv_clockcorrection_msecs = cs::i::cvar->find ( _ ( "sv_clockcorrection_msecs" ) );
	weapon_recoil_scale = cs::i::cvar->find ( _ ( "weapon_recoil_scale" ) );
	mp_friendlyfire = cs::i::cvar->find ( _ ( "mp_friendlyfire" ) );
	sv_maxunlag = cs::i::cvar->find ( _ ( "sv_maxunlag" ) );
	sv_minupdaterate = cs::i::cvar->find ( _ ( "sv_minupdaterate" ) );
	sv_maxupdaterate = cs::i::cvar->find ( _ ( "sv_maxupdaterate" ) );
	cl_interp_ratio = cs::i::cvar->find ( _ ( "cl_interp_ratio" ) );
	cl_interp = cs::i::cvar->find ( _ ( "cl_interp" ) );
	sv_client_min_interp_ratio = cs::i::cvar->find ( _ ( "sv_client_min_interp_ratio" ) );
	sv_client_max_interp_ratio = cs::i::cvar->find ( _ ( "sv_client_max_interp_ratio" ) );
	sv_friction = cs::i::cvar->find ( _ ( "sv_friction" ) );
	sv_stopspeed = cs::i::cvar->find ( _ ( "sv_stopspeed" ) );
	viewmodel_offset_x = cs::i::cvar->find ( _ ( "viewmodel_offset_x" ) );
	viewmodel_offset_y = cs::i::cvar->find ( _ ( "viewmodel_offset_y" ) );
	viewmodel_offset_z = cs::i::cvar->find ( _ ( "viewmodel_offset_z" ) );
	sv_maxspeed = cs::i::cvar->find ( _ ( "sv_maxspeed" ) );
	sv_accelerate = cs::i::cvar->find ( _ ( "sv_accelerate" ) );
}