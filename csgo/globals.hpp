#pragma once
#include <windows.h>
#include <memory>
#include <deque>
#include <array>
#include <vector>

#include "sdk/ucmd.hpp"
#include "sdk/vec3.hpp"

#ifdef VMPROTECT
#include "VMProtect/Include/C/VMProtectSDK.h"

#define VMP_STRINGIFY_(x) #x
#define VMP_STRINGIFY(x) VMP_STRINGIFY_(x)
#define VMP_DEFAULT_LABEL __FILE__ ":" __FUNCTION__ "(" VMP_STRINGIFY(__LINE__) ")"

#define VMP_BEGIN() VMProtectBegin(VMP_DEFAULT_LABEL);
#define VMP_BEGINVIRTUALIZATION() VMProtectBeginVirtualization(VMP_DEFAULT_LABEL);
#define VMP_BEGINMUTATION() VMProtectBeginMutation(VMP_DEFAULT_LABEL);
#define VMP_BEGINULTRA() VMProtectBeginUltra(VMP_DEFAULT_LABEL);
#define VMP_BEGINVIRTUALIZATIONLOCKBYKEY() VMProtectBeginVirtualizationLockByKey(VMP_DEFAULT_LABEL);
#define VMP_BEGINULTRALOCKBYKEY() VMProtectBeginUltraLockByKey(VMP_DEFAULT_LABEL);
#define VMP_END() VMProtectEnd()

#define VMP_ISPROTECTED VMProtectIsProtected()
#define VMP_ISDEBUGGERPRESENT(bcheckkernel) VMProtectIsDebuggerPresent(bcheckkernel)
#define VMP_ISVIRTUALMACHINEPRESENT() VMProtectIsVirtualMachinePresent()
#define VMP_ISVALIDIMAGECRC() VMProtectIsValidImageCRC()
#define VMP_ENCDECSTRINGA(value) VMProtectDecryptStringA(value)
#define VMP_ENCDECSTRINGW(value) VMProtectDecryptStringW(value)
#define VMP_FREESTRING(value) VMProtectFreeString(value)
#else
#define VMP_BEGIN()
#define VMP_BEGINVIRTUALIZATION()
#define VMP_BEGINMUTATION()
#define VMP_BEGINULTRA()
#define VMP_BEGINVIRTUALIZATIONLOCKBYKEY()
#define VMP_BEGINULTRALOCKBYKEY()
#define VMP_END()

#define VMP_ISPROTECTED false
#define VMP_ISDEBUGGERPRESENT(bcheckkernel) false
#define VMP_ISVIRTUALMACHINEPRESENT() false
#define VMP_ISVALIDIMAGECRC true
#define VMP_ENCDECSTRINGA(value) value
#define VMP_ENCDECSTRINGW(value) value
#define VMP_FREESTRING(value) true
#endif

/* semantic versioning */
#define SESAME_VERSION "Sesame v4.5.0"

template < typename type >
constexpr uint32_t rgba ( type r, type g, type b, type a ) {
	return ( ( static_cast< uint32_t >( r ) & 0xFF ) << 0 ) | ( ( static_cast< uint32_t >( g ) & 0xFF ) << 8 ) | ( ( static_cast<uint32_t>( b ) & 0xFF ) << 16 ) | ( ( static_cast< uint32_t >( a ) & 0xFF ) << 24 );
}

class player_t;
class cvar_t;

enum class round_t : int {
	starting = 0,
	in_progress,
	ending,
};

struct network_data_t {
	int sequence;
	int cmd;
};

namespace g {
	inline bool unload = false;

	inline player_t* local = nullptr;
	inline ucmd_t* ucmd = nullptr;
	inline ucmd_t sent_cmd;
	inline vec3_t angles {};
	inline bool hold_aim = false;
	inline bool send_packet = false;
	inline float cock_time = 0.0f;
	inline bool can_fire_revolver = false;
	inline round_t round = round_t::in_progress;
	inline uint32_t server_tick = 0;
	inline std::array< network_data_t, 150 > outgoing_cmds { };
	inline int choked_cmds = 0;
	inline int send_cmds = 0;
	inline bool just_shot = false;
	inline bool is_legacy = false;

	namespace resources {
#include "base85.hpp"
#include "resources/sesame_icons.hpp"

		inline uint8_t* sesame_icons;

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
		inline cvar_t* cl_upspeed = nullptr;
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
		inline cvar_t* viewmodel_offset_x = nullptr;
		inline cvar_t* viewmodel_offset_y = nullptr;
		inline cvar_t* viewmodel_offset_z = nullptr;
		inline cvar_t* sv_maxspeed = nullptr;
		inline cvar_t* sv_accelerate = nullptr;

		void init ( );
	}
}