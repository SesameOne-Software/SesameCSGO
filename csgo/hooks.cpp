#include <intrin.h>
#include <oxui.hpp>
#include "hooks.hpp"
#include "globals.hpp"
#include "minhook/minhook.h"
#include "renderer/d3d9.hpp"
#include "menu/menu.hpp"

/* features */
#include "animations/animations.hpp"
#include "features/chams.hpp"
#include "features/esp.hpp"
#include "features/movement.hpp"
#include "features/ragebot.hpp"
#include "features/antiaim.hpp"
#include "features/prediction.hpp"

bool hooks::in_autowall = false;
int hooks::scroll_delta = 0;
bool hooks::in_setupbones = false;
WNDPROC hooks::o_wndproc = nullptr;
bool hooks::no_update = false;

decltype( &hooks::fire_event_hk ) hooks::fire_event = nullptr;
decltype( &hooks::override_view_hk ) hooks::override_view = nullptr;
decltype( &hooks::get_bool_hk ) hooks::get_bool = nullptr;
decltype( &hooks::createmove_hk ) hooks::createmove = nullptr;
decltype( &hooks::framestagenotify_hk ) hooks::framestagenotify = nullptr;
decltype( &hooks::endscene_hk ) hooks::endscene = nullptr;
decltype( &hooks::reset_hk ) hooks::reset = nullptr;
decltype( &hooks::lockcursor_hk ) hooks::lockcursor = nullptr;
decltype( &hooks::sceneend_hk ) hooks::sceneend = nullptr;
decltype( &hooks::drawmodelexecute_hk ) hooks::drawmodelexecute = nullptr;
decltype( &hooks::doextraboneprocessing_hk ) hooks::doextraboneprocessing = nullptr;
decltype( &hooks::setupbones_hk ) hooks::setupbones = nullptr;
decltype( &hooks::get_eye_angles_hk ) hooks::get_eye_angles = nullptr;
decltype( &hooks::setupvelocity_hk ) hooks::setupvelocity = nullptr;
decltype( &hooks::draw_bullet_impacts_hk ) hooks::drawbulletimpacts = nullptr;
decltype( &hooks::trace_ray_hk ) hooks::traceray = nullptr;
decltype( &hooks::cl_sendmove_hk ) hooks::cl_sendmove = nullptr;
decltype( &hooks::update_clientside_animations_hk ) hooks::update_clientside_animations = nullptr;
decltype( &hooks::send_datagram_hk ) hooks::send_datagram = nullptr;
decltype( &hooks::should_skip_animation_frame_hk ) hooks::should_skip_animation_frame = nullptr;

/* fix event delays */
bool __fastcall hooks::fire_event_hk( REG ) {
	auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

	if ( !local->valid( ) )
		return fire_event( REG_OUT );

	struct event_t {
	public:
		PAD( 4 );
		float delay;
		PAD( 48 );
		event_t* next;
	};

	auto ei = *( event_t** ) ( std::uintptr_t( csgo::i::client_state ) + 0x4E64 );

	event_t* next = nullptr;

	if ( !ei )
		return fire_event( REG_OUT );

	do {
		next = ei->next;
		ei->delay = 0.f;
		ei = next;
	} while ( next );

	return fire_event( REG_OUT );
}

void __stdcall hooks::update_clientside_animations_hk( ) {
	return;
}

void __stdcall hooks::cl_sendmove_hk( ) {
	cl_sendmove( );
}

void __fastcall hooks::override_view_hk( REG, void* setup ) {
	FIND( bool, thirdperson, "misc", "effects", "third-person", oxui::object_checkbox );
	
	auto get_ideal_dist = [ & ] ( float ideal_distance ) {
		vec3_t inverse;
		csgo::i::engine->get_viewangles( inverse );

		inverse.x *= -1.0f, inverse.y += 180.0f;

		vec3_t direction = csgo::angle_vec( inverse );

		ray_t ray;
		trace_t trace;
		trace_filter_t filter( g::local );

		csgo::util_traceline( g::local->eyes( ), g::local->eyes( ) + ( direction * ideal_distance ), 0x600400B, g::local, &trace );

		return ( ideal_distance * trace.m_fraction ) - 10.0f;
	};

	if ( thirdperson && g::local->valid( ) ) {
		vec3_t ang;
		csgo::i::engine->get_viewangles( ang );
		csgo::i::input->m_camera_in_thirdperson = true;
		csgo::i::input->m_camera_offset = vec3_t( ang.x, ang.y, get_ideal_dist( 150.0f ) );

		if ( GetAsyncKeyState( VK_XBUTTON1 ) )
			*reinterpret_cast< float* >( uintptr_t( setup ) + 0xc0 ) = g::local->abs_origin( ).z + 64.0f;
	}
	else {
		csgo::i::input->m_camera_in_thirdperson = false;
	}

	override_view( REG_OUT, setup );
}

void __fastcall hooks::sceneend_hk( REG ) {
	sceneend( REG_OUT );
}

void __fastcall hooks::drawmodelexecute_hk( REG, void* ctx, void* state, const mdlrender_info_t& info, matrix3x4_t* bone_to_world ) {
	features::chams::drawmodelexecute( ctx, state, info, bone_to_world );
}

bool __fastcall hooks::createmove_hk( REG, float sampletime, ucmd_t* ucmd ) {
	if ( !ucmd || !ucmd->m_cmdnum )
		return false;

	auto ret = createmove( REG_OUT, sampletime, ucmd );

	g::ucmd = ucmd;

	features::movement::run( ucmd );

	const auto backup_angle = ucmd->m_angs;
	const auto backup_sidemove = ucmd->m_smove;
	const auto backup_forwardmove = ucmd->m_fmove;

	features::prediction::run( [ & ] ( ) {
		features::antiaim::run( ucmd );
		features::ragebot::run( ucmd );
	} );

	csgo::clamp( ucmd->m_angs );

	void* _ebp = nullptr;
	__asm mov _ebp, ebp;
	*( bool* ) ( *( std::uintptr_t* ) _ebp - 0x1C ) = g::send_packet;

	csgo::rotate_movement( ucmd );

	return false;
}

void __fastcall hooks::framestagenotify_hk( REG, int stage ) {
	g::local = ( !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) ) ? nullptr : csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

	animations::run( stage );

	framestagenotify( REG_OUT, stage );
}

long __fastcall hooks::endscene_hk( REG, IDirect3DDevice9* device ) {
	static auto ret = _ReturnAddress( );

	if ( ret != _ReturnAddress( ) )
		return endscene( REG_OUT, device );

	IDirect3DStateBlock9* pixel_state = nullptr;
	IDirect3DVertexDeclaration9* vertex_decleration = nullptr;
	IDirect3DVertexShader9* vertex_shader = nullptr;

	device->CreateStateBlock( D3DSBT_PIXELSTATE, &pixel_state );
	device->GetVertexDeclaration( &vertex_decleration );
	device->GetVertexShader( &vertex_shader );

	features::esp::render( );
	menu::draw( );

	pixel_state->Apply( );
	pixel_state->Release( );

	device->SetVertexDeclaration( vertex_decleration );
	device->SetVertexShader( vertex_shader );

	return endscene( REG_OUT, device );
}

long __fastcall hooks::reset_hk( REG, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* presentation_params ) {
	features::esp::esp_font->Release( );
	features::esp::indicator_font->Release( );
	menu::destroy( );

	auto hr = reset( REG_OUT, device, presentation_params );

	if ( SUCCEEDED( hr ) ) {
		render::create_font( ( void** ) &features::esp::esp_font, L"Segoe UI", 16, false );
		render::create_font( ( void** ) &features::esp::indicator_font, L"Tahoma", 16, true );
		menu::reset( );
	}

	return hr;
}

void __fastcall hooks::lockcursor_hk( REG ) {
	if ( menu::open( ) ) {
		csgo::i::surface->unlock_cursor( );
		return;
	}

	lockcursor( REG_OUT );
}

void __fastcall hooks::doextraboneprocessing_hk( REG, int a2, int a3, int a4, int a5, int a6, int a7 ) {
	auto e = reinterpret_cast< player_t* >( ecx );

	if ( !e ) {
		doextraboneprocessing( REG_OUT, a2, a3, a4, a5, a6, a7 );
		return;
	}

	auto animstate = e->animstate( );

	if ( !animstate || !animstate->m_entity ) {
		doextraboneprocessing( REG_OUT, a2, a3, a4, a5, a6, a7 );
		return;
	}

	// prevent call to do_procedural_foot_plant
	auto o_on_ground = animstate->m_on_ground;
	animstate->m_on_ground = false;
	doextraboneprocessing( REG_OUT, a2, a3, a4, a5, a6, a7 );
	animstate->m_on_ground = o_on_ground;
}

bool __fastcall hooks::setupbones_hk( REG, matrix3x4_t* out, int max_bones, int mask, float curtime ) {
	const auto pl = reinterpret_cast< player_t* >( uintptr_t( ecx ) - 4 );

	/* do not let game build bone matrix for players, we will build it ourselves with correct information. */
	if ( pl && pl->is_player( ) )
		return animations::build_matrix( pl );

	return setupbones( REG_OUT, out, max_bones, mask, curtime );
}

vec3_t* __fastcall hooks::get_eye_angles_hk( REG ) {
	auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

	if ( ecx != local )
		return get_eye_angles( REG_OUT );

	static auto ret_to_thirdperson_pitch = pattern::search( "client_panorama.dll", "8B CE F3 0F 10 00 8B 06 F3 0F 11 45 ? FF 90 ? ? ? ? F3 0F 10 55 ?" ).get< std::uintptr_t >( );
	static auto ret_to_thirdperson_yaw = pattern::search( "client_panorama.dll", "F3 0F 10 55 ? 51 8B 8E ? ? ? ?" ).get< std::uintptr_t >( );

	if ( ( std::uintptr_t( _ReturnAddress( ) ) == ret_to_thirdperson_pitch
		|| std::uintptr_t( _ReturnAddress( ) ) == ret_to_thirdperson_yaw )
		&& !no_update )
		return g::ucmd ? &g::ucmd->m_angs : &local->angles( );

	return get_eye_angles( REG_OUT );
}

bool __fastcall hooks::get_bool_hk( REG ) {
	static auto cl_interpolate = pattern::search( "client_panorama.dll", "85 C0 BF ? ? ? ? 0F 95 C3" ).get< std::uintptr_t >( );
	static auto cam_think = pattern::search( "client_panorama.dll", "85 C0 75 30 38 86" ).get< std::uintptr_t >( );
	static auto hermite_fix = pattern::search( "client_panorama.dll", "0F B6 15 ? ? ? ? 85 C0" ).get< std::uintptr_t >( );
	static auto cl_extrapolate = pattern::search( "client_panorama.dll", "85 C0 74 22 8B 0D ? ? ? ? 8B 01 8B" ).get< std::uintptr_t >( );

	if ( !ecx )
		return false;

	auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

	if ( /* oxui::vars::items [ "thirdperson" ].val.b */ true ) {
		if ( std::uintptr_t( _ReturnAddress( ) ) == cam_think )
			return true;
	}

	// /* disable client-side extrapolation */
	// if ( std::uintptr_t( _ReturnAddress( ) ) == cl_interpolate )
	// 	return false;
	// 
	// /* disable client-side hermite fix */
	// if ( std::uintptr_t( _ReturnAddress( ) ) == hermite_fix )
	// 	return false;
	// 
	// /* disable client-side interpolation */
	// if ( std::uintptr_t( _ReturnAddress( ) ) == cl_extrapolate )
	// 	return false;

	return get_bool( REG_OUT );
}

void __declspec( naked ) __stdcall hooks::naked_setupvelocity_hk( ) {
	__asm {
		pushad
		push ecx
		call /* anim_system::setup_velocity */ eax
		popad
		ret
	}
}

void __fastcall hooks::setupvelocity_hk( animstate_t* state, void* edx ) {
	__asm pushad
	// anim_system::setup_velocity( state );
	__asm popad
}

void __fastcall hooks::draw_bullet_impacts_hk( REG, int a1, int a2 ) {
	if ( in_autowall )
		return;

	drawbulletimpacts( REG_OUT, a1, a2 );
}

void __fastcall hooks::trace_ray_hk( REG, const ray_t& ray, unsigned int mask, trace_filter_t* filter, trace_t* trace ) {
	traceray( REG_OUT, ray, mask, filter, trace );

	if ( in_autowall )
		trace->m_surface.m_flags |= 4;
}

int __fastcall hooks::send_datagram_hk( REG, void* datagram ) {
	// return tickbase::send_datagram( REG_OUT, datagram );
}

bool __fastcall hooks::should_skip_animation_frame_hk( REG ) {
	return true;
}

long __stdcall hooks::wndproc( HWND hwnd, std::uint32_t msg, std::uintptr_t wparam, std::uint32_t lparam ) {
	if ( menu::wndproc( hwnd, msg, wparam, lparam ) )
		return true;

	return CallWindowProcA( o_wndproc, hwnd, msg, wparam, lparam );
}

bool hooks::init( ) {
	menu::init( );

	/* create fonts */
	render::create_font( ( void** ) &features::esp::esp_font, L"Segoe UI", 12, false );
	render::create_font( ( void** ) &features::esp::indicator_font, L"Tahoma", 16, true );

	/* load default config */
	menu::load_default( );

	const auto clsendmove = pattern::search( "engine.dll", "E8 ? ? ? ? 84 DB 0F 84 ? ? ? ? 8B 8F" ).resolve_rip( ).get< void* >( );
	const auto clientmodeshared_createmove = pattern::search( "client_panorama.dll", "55 8B EC 8B 0D ? ? ? ? 85 C9 75 06 B0" ).get< decltype( &createmove_hk ) >( );
	const auto chlclient_framestagenotify = pattern::search( "client_panorama.dll", "55 8B EC 8B 0D ? ? ? ? 8B 01 8B 80 74 01 00 00 FF D0 A2" ).get< decltype( &framestagenotify_hk ) >( );
	const auto idirect3ddevice9_endscene = vfunc< decltype( &endscene_hk ) >( csgo::i::dev, 42 );
	const auto idirect3ddevice9_reset = vfunc< decltype( &reset_hk ) >( csgo::i::dev, 16 );
	const auto vguimatsurface_lockcursor = pattern::search( "vguimatsurface.dll", "80 3D ? ? ? ? 00 8B 91 A4 02 00 00 8B 0D ? ? ? ? C6 05 ? ? ? ? 01" ).get< decltype( &lockcursor_hk ) >( );
	const auto ivrenderview_sceneend = vfunc< decltype( &sceneend_hk ) >( csgo::i::render_view, 9 );
	const auto modelrender_drawmodelexecute = vfunc< decltype( &drawmodelexecute_hk ) >( csgo::i::mdl_render, 21 );
	const auto c_csplayer_doextraboneprocessing = pattern::search( "client_panorama.dll", "55 8B EC 83 E4 F8 81 EC FC 00 00 00 53 56 8B F1 57" ).get< void* >( );
	const auto c_baseanimating_setupbones = pattern::search( "client_panorama.dll", "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9" ).get< void* >( );
	const auto c_csplayer_get_eye_angles = pattern::search( "client_panorama.dll", "56 8B F1 85 F6 74 32" ).get< void* >( );
	const auto c_csgoplayeranimstate_setupvelocity = pattern::search( "client_panorama.dll", "55 8B EC 83 E4 F8 83 EC 30 56 57 8B 3D" ).get< void* >( );
	const auto c_csgoplayeranimstate_setupvelocity_call = pattern::search( "client_panorama.dll", "E8 ? ? ? ? E9 ? ? ? ? 83 BE" ).resolve_rip( ).add( 0x366 ).get< void* >( );
	const auto draw_bullet_impacts = pattern::search( "client_panorama.dll", "56 8B 71 4C 57" ).get< void* >( );
	const auto convar_getbool = pattern::search( "client_panorama.dll", "8B 51 1C 3B D1 75 06" ).get< void* >( );
	const auto traceray_fn = vfunc< void* >( csgo::i::trace, 5 );
	const auto overrideview = pattern::search( "client_panorama.dll", "55 8B EC 83 E4 F8 83 EC 58 56 57 8B 3D ? ? ? ? 85 FF" ).get< void* >( );
	const auto c_baseanimating_updateclientsideanimations = pattern::search( "client_panorama.dll", "8B 0D ? ? ? ? 53 56 57 8B 99 0C 10 00 00 85 DB 74 ? 6A 04 6A 00" ).get< void* >( );
	const auto nc_send_datagram = pattern::search( "engine.dll", "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 89 7C 24 18" ).get<void*>( );
	const auto engine_fire_event = vfunc< void* >( csgo::i::engine, 59 );
	const auto c_baseanimating_should_skip_animation_frame = pattern::search( "client_panorama.dll", "E8 ? ? ? ? 88 44 24 0B" ).add( 1 ).deref( ).get< void* >( );

	MH_Initialize( );

	MH_CreateHook( engine_fire_event, fire_event_hk, ( void** ) &fire_event );
	MH_CreateHook( clientmodeshared_createmove, createmove_hk, ( void** ) &createmove );
	MH_CreateHook( chlclient_framestagenotify, framestagenotify_hk, ( void** ) &framestagenotify );
	MH_CreateHook( idirect3ddevice9_endscene, endscene_hk, ( void** ) &endscene );
	MH_CreateHook( idirect3ddevice9_reset, reset_hk, ( void** ) &reset );
	MH_CreateHook( vguimatsurface_lockcursor, lockcursor_hk, ( void** ) &lockcursor );
	MH_CreateHook( ivrenderview_sceneend, sceneend_hk, ( void** ) &sceneend );
	MH_CreateHook( modelrender_drawmodelexecute, drawmodelexecute_hk, ( void** ) &drawmodelexecute );
	MH_CreateHook( c_csplayer_doextraboneprocessing, doextraboneprocessing_hk, ( void** ) &doextraboneprocessing );
	MH_CreateHook( c_csplayer_get_eye_angles, get_eye_angles_hk, ( void** ) &get_eye_angles );
	MH_CreateHook( traceray_fn, trace_ray_hk, ( void** ) &traceray );
	// MH_CreateHook( draw_bullet_impacts, draw_bullet_impacts_hk, ( void** ) &drawbulletimpacts );
	MH_CreateHook( c_baseanimating_setupbones, setupbones_hk, ( void** ) &setupbones );
	// MH_CreateHook( c_csgoplayeranimstate_setupvelocity, setupvelocity_hk, ( void** ) &setupvelocity );
	MH_CreateHook( convar_getbool, get_bool_hk, ( void** ) &get_bool );
	MH_CreateHook( overrideview, override_view_hk, ( void** ) &override_view );
	// MH_CreateHook( c_baseanimating_should_skip_animation_frame, should_skip_animation_frame_hk, ( void** ) &should_skip_animation_frame );
	// MH_CreateHook( c_baseanimating_updateclientsideanimations, update_clientside_animations_hk, ( void** ) &update_clientside_animations );
	// MH_CreateHook( nc_send_datagram, send_datagram_hk, ( void** ) &send_datagram );
	// MH_CreateHook( clsendmove, cl_sendmove_hk, ( void** ) &cl_sendmove );

	MH_EnableHook( MH_ALL_HOOKS );

	o_wndproc = ( WNDPROC ) SetWindowLongA( FindWindowA( "Valve001", nullptr ), GWLP_WNDPROC, ( long ) wndproc );

	return true;
}