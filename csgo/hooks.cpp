#include <intrin.h>
#include <oxui.h>
#include "hooks.h"
#include "globals.h"
#include "minhook/minhook.h"

/* menu */
#include "menu/menu.h"
#include "oxui/d3d9/impl.h"

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

/* fix event delays */
bool __fastcall hooks::fire_event_hk( void* ecx, void* edx ) {
	auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

	if ( !local->valid( ) )
		return fire_event( ecx, edx );

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
		return fire_event( ecx, edx );

	do {
		next = ei->next;
		ei->delay = 0.f;
		ei = next;
	} while ( next );

	return fire_event( ecx, edx );
}

void __stdcall hooks::update_clientside_animations_hk( ) {
	return;

	static auto g_ClientSideAnimationList_Count = pattern::search( "client_panorama.dll", "8B 0D ? ? ? ? 8B 3D ? ? ? ? 33 F6" ).add( 8 ).deref( ).get< std::uint32_t* >( );
	static auto ClientSideAnimationList = pattern::search( "client_panorama.dll", "7E 27 0F 1F 44 00 ? A1" ).add( 8 ).deref( ).get< anim_list_t* >( );

	auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

	if ( *g_ClientSideAnimationList_Count > 0 ) {
		for ( auto i = 0; i < *g_ClientSideAnimationList_Count; i++ ) {
			auto rec = ClientSideAnimationList->m_data [ i ];

			if ( rec.m_ent == local && rec.m_flags & 1 && local->alive( ) )
				rec.m_ent->update( );
		}
	}

	const auto backup_animlist_count = *g_ClientSideAnimationList_Count;

	*g_ClientSideAnimationList_Count = 0;
	update_clientside_animations( );
	*g_ClientSideAnimationList_Count = backup_animlist_count;
}

void __stdcall hooks::cl_sendmove_hk( ) {
	cl_sendmove( );
}

void __fastcall hooks::override_view_hk( void* ecx, void* edx, void* setup ) {
	
	override_view( ecx, edx, setup );
}

void __fastcall hooks::sceneend_hk( void* ecx, void* edx ) {
	sceneend( ecx, edx );
}

void __fastcall hooks::drawmodelexecute_hk( void* ecx, void* edx, void* ctx, void* state, const mdlrender_info_t& info, matrix3x4_t* bone_to_world ) {

}

bool __fastcall hooks::createmove_hk( class c_clientmode* thisptr, void* edx, float sampletime, ucmd_t* ucmd ) {
	auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

	if ( !ucmd || !ucmd->m_cmdnum || !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) || !local )
		return false;

	auto ret = createmove( thisptr, edx, sampletime, ucmd );

	g::ucmd = ucmd;

	csgo::clamp( ucmd->m_angs );

	/*
	if ( ragebot::overwrite_tickcount ) {
		ucmd->m_tickcount = ragebot::tickcount;
		ragebot::overwrite_tickcount = false;
	}
	*/

	void* _ebp;
	__asm mov _ebp, ebp;
	g::psend_packet = ( bool* ) ( *( std::uintptr_t* ) _ebp - 0x1C );
	*( bool* ) ( *( std::uintptr_t* ) _ebp - 0x1C ) = g::send_packet;

	g::last_real.x = g::last_fake.x;

	/*
	if ( !g::send_packet ) {
		const auto current_choke = csgo::i::client_state->choked( );

		csgo::i::client_state->choked( ) = 0;

		( *( int( __stdcall** )( void* ) )( **( std::uintptr_t** ) ( std::uintptr_t( csgo::i::client_state ) + 0x9C ) + 184 ) )( nullptr );

		--csgo::i::client_state->out_seq_num( );
		csgo::i::client_state->choked( ) = current_choke;
	}
	*/

	// g::send_packet = true;

	std::memcpy( &g::raw_ucmd, ucmd, sizeof( ucmd_t ) );

	return false;
}

void __fastcall hooks::framestagenotify_hk( c_client* thisptr, void* edx, int stage ) {
	

	framestagenotify( thisptr, edx, stage );
}

long __fastcall hooks::endscene_hk( IDirect3DDevice9* thisptr, void* edx, IDirect3DDevice9* device ) {
	static auto ret = _ReturnAddress( );

	if ( ret != _ReturnAddress( ) )
		return endscene( thisptr, edx, device );

	IDirect3DStateBlock9* pixel_state = nullptr;
	IDirect3DVertexDeclaration9* vertex_decleration = nullptr;
	IDirect3DVertexShader9* vertex_shader = nullptr;

	device->CreateStateBlock( D3DSBT_PIXELSTATE, &pixel_state );
	device->GetVertexDeclaration( &vertex_decleration );
	device->GetVertexShader( &vertex_shader );

	/* create fonts if needed */
	if ( oxui::vars::fonts.empty( ) ) {
		oxui::vars::fonts [ "esp" ] = oxui::impl::create_font( "Segoe UI", 14, false );
		oxui::vars::fonts [ "window" ] = oxui::impl::create_font( "Segoe UI", 14, false );
		oxui::vars::fonts [ "tab" ] = oxui::impl::create_font( "Segoe UI", 14, true );
		oxui::vars::fonts [ "control_selected" ] = oxui::impl::create_font( "Segoe UI", 12, false );
		oxui::vars::fonts [ "control" ] = oxui::impl::create_font( "Segoe UI", 15, false );
	}

	menu::render( );

	pixel_state->Apply( );
	pixel_state->Release( );

	device->SetVertexDeclaration( vertex_decleration );
	device->SetVertexShader( vertex_shader );

	return endscene( thisptr, edx, device );
}

long __fastcall hooks::reset_hk( IDirect3DDevice9* thisptr, void* edx, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* presentation_params ) {
	if ( menu::init ) {
		oxui::destroy( );

		auto hr = reset( thisptr, edx, device, presentation_params );

		if ( SUCCEEDED( hr ) ) {
			oxui::vars::fonts [ "esp" ] = oxui::impl::create_font( "Segoe UI", 14, false );
			oxui::vars::fonts [ "window" ] = oxui::impl::create_font( "Segoe UI", 14, false );
			oxui::vars::fonts [ "tab" ] = oxui::impl::create_font( "Segoe UI", 26, false );
			oxui::vars::fonts [ "control_selected" ] = oxui::impl::create_font( "Segoe UI", 12, false );
			oxui::vars::fonts [ "control" ] = oxui::impl::create_font( "Segoe UI", 15, false );
		}

		return hr;
	}
	
	return reset( thisptr, edx, device, presentation_params );
}

void __fastcall hooks::lockcursor_hk( c_surface* thisptr, void* edx ) {
	if ( oxui::vars::open ) {
		csgo::i::surface->unlock_cursor( );
		return;
	}

	lockcursor( thisptr, edx );
}

void __fastcall hooks::doextraboneprocessing_hk( void* ecx, void* edx, int a2, int a3, int a4, int a5, int a6, int a7 ) {
	auto e = reinterpret_cast< player_t* >( ecx );

	if ( !e ) {
		doextraboneprocessing( ecx, edx, a2, a3, a4, a5, a6, a7 );
		return;
	}

	auto animstate = e->animstate( );

	if ( !animstate || !animstate->m_entity ) {
		doextraboneprocessing( ecx, edx, a2, a3, a4, a5, a6, a7 );
		return;
	}

	// prevent call to do_procedural_foot_plant
	auto o_on_ground = animstate->m_on_ground;
	animstate->m_on_ground = false;
	doextraboneprocessing( ecx, edx, a2, a3, a4, a5, a6, a7 );
	animstate->m_on_ground = o_on_ground;
}

bool __fastcall hooks::setupbones_hk( void* ecx, void* edx, matrix3x4_t* out, int max_bones, std::uint32_t mask, float curtime ) {
	static auto setupbones_ret_addr_1 = pattern::search( "client_panorama.dll", "E8 ? ? ? ? 5E 5D C2 10 00 32 C0" ).add( 5 ).get< std::uintptr_t >( ); // sets up other entities
	static auto setupbones_ret_addr_2 = pattern::search( "client_panorama.dll", "FF 75 08 E8 ? ? ? ? 5F 5E 5D C2 10 00" ).add( 8 ).get< std::uintptr_t >( ); // sets up players

	// only if animfixing!
	if ( std::uintptr_t( _ReturnAddress( ) ) == setupbones_ret_addr_2 )
		return true;

	return setupbones( ecx, edx, out, max_bones, mask, curtime );
}

vec3_t* __fastcall hooks::get_eye_angles_hk( void* ecx, void* edx ) {
	auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

	if ( ecx != local )
		return get_eye_angles( ecx, edx );

	static auto ret_to_thirdperson_pitch = pattern::search( "client_panorama.dll", "8B CE F3 0F 10 00 8B 06 F3 0F 11 45 ? FF 90 ? ? ? ? F3 0F 10 55 ?" ).get< std::uintptr_t >( );
	static auto ret_to_thirdperson_yaw = pattern::search( "client_panorama.dll", "F3 0F 10 55 ? 51 8B 8E ? ? ? ?" ).get< std::uintptr_t >( );

	/*
	if ( !ctx::client.in_thirdperson )
		return get_eye_angles( ecx, edx );
	*/

	if ( ( std::uintptr_t( _ReturnAddress( ) ) == ret_to_thirdperson_pitch
		|| std::uintptr_t( _ReturnAddress( ) ) == ret_to_thirdperson_yaw )
		&& !no_update )
		return g::ucmd ? &g::ucmd->m_angs : &local->angles( );

	return get_eye_angles( ecx, edx );
}

bool __fastcall hooks::get_bool_hk( void* ecx, void* edx ) {
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

	return get_bool( ecx, edx );
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

void __fastcall hooks::draw_bullet_impacts_hk( void* ecx, void* edx, int a1, int a2 ) {
	if ( in_autowall )
		return;

	drawbulletimpacts( ecx, edx, a1, a2 );
}

void __fastcall hooks::trace_ray_hk( void* ecx, void* edx, const ray_t& ray, unsigned int mask, trace_filter_t* filter, trace_t* trace ) {
	traceray( ecx, edx, ray, mask, filter, trace );

	if ( in_autowall )
		trace->m_surface.m_flags |= 4;
}

int __fastcall hooks::send_datagram_hk( void* ecx, void* edx, void* datagram ) {
	// return tickbase::send_datagram( ecx, edx, datagram );
}

long __stdcall hooks::wndproc( HWND hwnd, std::uint32_t msg, std::uintptr_t wparam, std::uint32_t lparam ) {
	if ( oxui::vars::open ) {
		switch ( msg ) {
		case WM_MOUSEWHEEL:
			oxui::vars::scroll_amount += GET_WHEEL_DELTA_WPARAM( wparam ) > 0 ? -1.0f : 1.0f;
			return 0;
		}

		return true;
	}

	return CallWindowProcA( o_wndproc, hwnd, msg, wparam, lparam );
}

bool hooks::init( ) {
	oxui::impl::init( );

	/* initialize menu objects */
	menu::init( );

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
	// utils::log( L"drawbulletimpact hooked\t\t\t=\t\t0x%x", draw_bullet_impacts );
	MH_CreateHook( c_baseanimating_setupbones, setupbones_hk, ( void** ) &setupbones );
	// MH_CreateHook( c_csgoplayeranimstate_setupvelocity, setupvelocity_hk, ( void** ) &setupvelocity );
	// utils::log( L"setupvelocity hooked\t\t\t=\t\t0x%x", c_csgoplayeranimstate_setupvelocity );
	MH_CreateHook( convar_getbool, get_bool_hk, ( void** ) &get_bool );
	MH_CreateHook( overrideview, override_view_hk, ( void** ) &override_view );
	MH_CreateHook( c_baseanimating_updateclientsideanimations, update_clientside_animations_hk, ( void** ) &update_clientside_animations );
	// MH_CreateHook( nc_send_datagram, send_datagram_hk, ( void** ) &send_datagram );
	// utils::log( L"send_datagram hooked\t\t\t\t=\t\t0x%x", nc_send_datagram );
	// MH_CreateHook( clsendmove, cl_sendmove_hk, ( void** ) &cl_sendmove );
	// utils::log( L"cl_move hooked\t\t\t\t=\t\t0x%x", clsendmove );

	/*
	unsigned long o_prot = 0;
	VirtualProtect( ( void* ) ( std::uintptr_t( c_csgoplayeranimstate_setupvelocity_call ) + 1 ), sizeof( std::uint32_t ), PAGE_EXECUTE_READWRITE, &o_prot );
	*( std::uint32_t* ) ( std::uintptr_t( c_csgoplayeranimstate_setupvelocity_call ) + 1 ) = std::uintptr_t( naked_setupvelocity_hk ) - std::uintptr_t( c_csgoplayeranimstate_setupvelocity_call ) - 5;
	VirtualProtect( ( void* ) ( std::uintptr_t( c_csgoplayeranimstate_setupvelocity_call ) + 1 ), sizeof( std::uint32_t ), o_prot, &o_prot );
	setupvelocity = ( void( __fastcall* )( animstate_t*, void* ) ) ( std::uintptr_t( c_csgoplayeranimstate_setupvelocity_call ) + 5 );
	*/

	MH_EnableHook( MH_ALL_HOOKS );

	// inventory_mgr::test( );

	o_wndproc = ( WNDPROC ) SetWindowLongA( FindWindowA( "Valve001", nullptr ), GWLP_WNDPROC, ( long ) wndproc );

	return true;
}