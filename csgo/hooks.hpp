#pragma once
#include "sdk/sdk.hpp"
#include "utils/utils.hpp"

namespace hooks {
	extern int scroll_delta;
	extern bool in_setupbones;
	extern WNDPROC o_wndproc;
	extern bool in_autowall;
	extern bool no_update;

	bool __fastcall fire_event_hk( REG );
	int __fastcall send_datagram_hk( REG, void* datagram );
	void __stdcall update_clientside_animations_hk( );
	long __fastcall endscene_hk( REG, IDirect3DDevice9* device );
	long __fastcall reset_hk( REG, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* presentation_params );
	void __fastcall lockcursor_hk( REG );
	bool __fastcall createmove_hk( REG, float sampletime, ucmd_t* ucmd );
	void __fastcall framestagenotify_hk( REG, int stage );
	void __fastcall sceneend_hk( REG );
	void __fastcall drawmodelexecute_hk( REG, void* ctx, void* state, const mdlrender_info_t& info, matrix3x4_t* bone_to_world );
	void __fastcall doextraboneprocessing_hk( REG, int a2, int a3, int a4, int a5, int a6, int a7 );
	bool __fastcall setupbones_hk( REG, matrix3x4_t* out, int max_bones, int mask, float curtime );
	vec3_t* __fastcall get_eye_angles_hk( REG );
	bool __fastcall get_bool_hk( REG );
	void __stdcall naked_setupvelocity_hk( );
	void __fastcall setupvelocity_hk( animstate_t* state, void* edx );
	void __fastcall draw_bullet_impacts_hk( REG, int a1, int a2 );
	void __fastcall trace_ray_hk( REG, const ray_t& ray, unsigned int mask, trace_filter_t* filter, trace_t* trace );
	void __fastcall override_view_hk( REG, void* setup );
	void __stdcall cl_sendmove_hk( );
	bool __fastcall should_skip_animation_frame_hk( REG );
	bool __fastcall is_hltv_hk( REG );
	long __stdcall wndproc( HWND hwnd, std::uint32_t msg, std::uintptr_t wparam, std::uint32_t lparam );

	extern decltype( &fire_event_hk ) fire_event;
	extern decltype( &update_clientside_animations_hk ) update_clientside_animations;
	extern decltype( &cl_sendmove_hk ) cl_sendmove;
	extern decltype( &override_view_hk ) override_view;
	extern decltype( &get_bool_hk ) get_bool;
	extern decltype( &trace_ray_hk ) traceray;
	extern decltype( &createmove_hk ) createmove;
	extern decltype( &framestagenotify_hk ) framestagenotify;
	extern decltype( &endscene_hk ) endscene;
	extern decltype( &reset_hk ) reset;
	extern decltype( &lockcursor_hk ) lockcursor;
	extern decltype( &sceneend_hk ) sceneend;
	extern decltype( &send_datagram_hk ) send_datagram;
	extern decltype( &drawmodelexecute_hk ) drawmodelexecute;
	extern decltype( &doextraboneprocessing_hk ) doextraboneprocessing;
	extern decltype( &setupbones_hk ) setupbones;
	extern decltype( &setupvelocity_hk ) setupvelocity;
	extern decltype( &get_eye_angles_hk ) get_eye_angles;
	extern decltype( &draw_bullet_impacts_hk ) drawbulletimpacts;
	extern decltype( &should_skip_animation_frame_hk ) should_skip_animation_frame;
	extern decltype( &is_hltv_hk ) is_hltv;

	bool init( );
}