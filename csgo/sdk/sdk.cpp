#include "sdk.hpp"
#include "netvar.hpp"
#include "../globals.hpp"

c_entlist* csgo::i::ent_list = nullptr;
c_matsys* csgo::i::mat_sys = nullptr;
c_mdlinfo* csgo::i::mdl_info = nullptr;
c_mdlrender* csgo::i::mdl_render = nullptr;
c_renderview* csgo::i::render_view = nullptr;
c_client* csgo::i::client = nullptr;
c_surface* csgo::i::surface = nullptr;
c_panel* csgo::i::panel = nullptr;
c_engine* csgo::i::engine = nullptr;
c_globals* csgo::i::globals = nullptr;
c_phys* csgo::i::phys = nullptr;
c_engine_trace* csgo::i::trace = nullptr;
c_clientstate* csgo::i::client_state = nullptr;
c_mem_alloc* csgo::i::mem_alloc = nullptr;
c_prediction* csgo::i::pred = nullptr;
c_move_helper* csgo::i::move_helper = nullptr;
c_movement* csgo::i::move = nullptr;
mdl_cache_t* csgo::i::mdl_cache = nullptr;
c_input* csgo::i::input = nullptr;
c_cvar* csgo::i::cvar = nullptr;
c_game_event_mgr* csgo::i::events = nullptr;
c_view_render_beams* csgo::i::beams = nullptr;
IDirect3DDevice9* csgo::i::dev = nullptr;

c_mdl_cache_critical_section::c_mdl_cache_critical_section( ) {
	csgo::i::mdl_cache->begin_lock( );
}

c_mdl_cache_critical_section::~c_mdl_cache_critical_section( ) {
	csgo::i::mdl_cache->end_lock( );
}

bool csgo::render::screen_transform( vec3_t& screen, vec3_t& origin ) {
	static auto view_matrix = pattern::search( _( "client.dll" ), _( "0F 10 05 ? ? ? ? 8D 85 ? ? ? ? B9" ) ).add( 3 ).deref( ).add( 176 ).get< std::uintptr_t >( );

	const auto& world_matrix = *( matrix3x4_t* ) view_matrix;

	screen.x = world_matrix [ 0 ][ 0 ] * origin.x + world_matrix [ 0 ][ 1 ] * origin.y + world_matrix [ 0 ][ 2 ] * origin.z + world_matrix [ 0 ][ 3 ];
	screen.y = world_matrix [ 1 ][ 0 ] * origin.x + world_matrix [ 1 ][ 1 ] * origin.y + world_matrix [ 1 ][ 2 ] * origin.z + world_matrix [ 1 ][ 3 ];
	screen.z = 0.0f;

	const auto w = world_matrix [ 3 ][ 0 ] * origin.x + world_matrix [ 3 ][ 1 ] * origin.y + world_matrix [ 3 ][ 2 ] * origin.z + world_matrix [ 3 ][ 3 ];

	if ( w < 0.001f ) {
		screen.x *= -1.0f / w;
		screen.y *= -1.0f / w;

		return true;
	}

	screen.x *= 1.0f / w;
	screen.y *= 1.0f / w;

	return false;
}

bool csgo::render::world_to_screen( vec3_t& screen, vec3_t& origin ) {
	const auto find_point = [ ] ( vec3_t& point, int screen_w, int screen_h, float deg ) {
		const auto x2 = screen_w / 2.0f;
		const auto y2 = screen_h / 2.0f;
		const auto one = point.x - x2;
		const auto two = point.y - y2;
		const auto d = std::sqrt ( one * one + two * two );
		const auto r = deg / d;

		point.x = r * point.x + ( 1.0f - r ) * x2;
		point.y = r * point.y + ( 1.0f - r ) * y2;
	};

	const auto transform = screen_transform ( screen, origin );

	int width, height;
	csgo::i::engine->get_screen_size ( width, height );

	screen.x = ( width * 0.5f ) + ( screen.x * width ) * 0.5f;
	screen.y = ( height * 0.5f ) - ( screen.y * height ) * 0.5f;

	if ( screen.x > width || screen.x < 0 || screen.y > height || screen.y < 0 || transform ) {
		find_point ( screen, width, height, std::sqrt ( width * width + height * height ) );
		return false;
	}

	return true;
}

void csgo::util::trace_line( const vec3_t& start, const vec3_t& end, std::uint32_t mask, const entity_t* ignore, trace_t* ptr ) {
	trace_filter_t filter( ( void* ) ignore );

	ray_t ray;
	ray.init( start, end );

	csgo::i::trace->trace_ray( ray, mask, &filter, ptr );

	/*
	using fn = void( __fastcall* )( const vec3_t&, const vec3_t&, std::uint32_t, const entity_t*, std::uint32_t, trace_t* );
	static auto util_traceline = pattern::search( STR( "client.dll" ), STR( "55 8B EC 83 E4 F0 83 EC 7C 56 52" ) ).get< fn >( );
	util_traceline( start, end, mask, ignore, 0, ptr );
	*/
}

void csgo::util::clip_trace_to_players( const vec3_t& start, const vec3_t& end, std::uint32_t mask, trace_filter_t* filter, trace_t* trace_ptr ) {
	static auto util_cliptracetoplayers = pattern::search( _( "client.dll" ), _( "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC D8 ? ? ? 0F 57 C9" ) ).get< std::uint32_t >( );

	if ( !util_cliptracetoplayers )
		return;

	__asm {
		mov	eax, filter
		lea ecx, trace_ptr
		push ecx
		push eax
		push mask
		lea edx, end
		lea ecx, start
		call util_cliptracetoplayers
		add esp, 0xC
	}
}

template < typename t >
t csgo::create_interface( const char* module, const char* iname ) {
	using createinterface_fn = void* ( __cdecl* )( const char*, int );
	const auto createinterface_export = GetProcAddress( GetModuleHandleA( module ), _( "CreateInterface" ) );
	const auto fn = ( createinterface_fn ) createinterface_export;

	return reinterpret_cast< t >( fn( iname, 0 ) );
}

bool csgo::init( ) {
	i::globals = pattern::search( _( "client.dll" ), _( "A1 ? ? ? ? F3 0F 10 8F ? ? ? ? F3 0F 10 05 ? ? ? ? ? ? ? ? ? 0F 2F C1 0F 86" ) ).add( 1 ).deref( ).deref( ).get< c_globals* >( );
	i::ent_list = create_interface< c_entlist* >( _( "client.dll" ), _( "VClientEntityList003" ) );
	i::mat_sys = create_interface< c_matsys* >( _( "materialsystem.dll" ), _( "VMaterialSystem080" ) );
	i::mdl_info = create_interface< c_mdlinfo* >( _( "engine.dll" ), _( "VModelInfoClient004" ) );
	i::mdl_render = create_interface< c_mdlrender* >( _( "engine.dll" ), _( "VEngineModel016" ) );
	i::render_view = create_interface< c_renderview* >( _( "engine.dll" ), _( "VEngineRenderView014" ) );
	i::client = create_interface< c_client* >( _( "client.dll" ), _( "VClient018" ) );
	i::surface = create_interface< c_surface* > ( _ ( "vguimatsurface.dll" ), _ ( "VGUI_Surface031" ) );
	i::panel = create_interface< c_panel* >( _( "vgui2.dll" ), _( "VGUI_Panel009" ) );
	i::engine = create_interface< c_engine* >( _( "engine.dll" ), _( "VEngineClient014" ) );
	i::phys = create_interface< c_phys* >( _( "vphysics.dll" ), _( "VPhysicsSurfaceProps001" ) );
	i::trace = create_interface< c_engine_trace* >( _( "engine.dll" ), _( "EngineTraceClient004" ) );
	i::pred = create_interface< c_prediction* >( _( "client.dll" ), _( "VClientPrediction001" ) );
	i::move = create_interface< c_movement* >( _( "client.dll" ), _( "GameMovement001" ) );
	i::mdl_cache = create_interface< mdl_cache_t* >( _( "client.dll" ), _( "MDLCache004" ) );
	i::events = create_interface< c_game_event_mgr* >( _( "engine.dll" ), _( "GAMEEVENTSMANAGER002" ) );
	i::input = pattern::search( _( "client.dll" ), _( "B9 ? ? ? ? FF 60 60" ) ).add( 1 ).deref( ).get< c_input* >( );
	i::cvar = create_interface< c_cvar* >( _( "vstdlib.dll" ), _( "VEngineCvar007" ) );
	i::move_helper = **reinterpret_cast< c_move_helper*** >( pattern::search( _( "client.dll" ), _( "8B 0D ? ? ? ? 8B 45 ? 51 8B D4 89 02 8B 01" ) ).add( 2 ).get< std::uintptr_t >( ) );
	i::client_state = **reinterpret_cast< c_clientstate*** >( reinterpret_cast< std::uintptr_t >( vfunc< void* >( i::engine, 12 ) ) + 16 );
	i::beams = pattern::search ( _ ( "client.dll" ), _ ( "A1 ? ? ? ? 56 8B F1 B9 ? ? ? ? FF 50 08" ) ).add ( 1 ).deref ( ).get< c_view_render_beams* > ( );
	i::mem_alloc = *( c_mem_alloc** ) GetProcAddress( GetModuleHandleA( _( "tier0.dll" ) ), _( "g_pMemAlloc" ) );
	i::dev = pattern::search( _( "shaderapidx9.dll" ), _( "A1 ? ? ? ? 50 8B 08 FF 51 0C" ) ).add( 1 ).deref( ).deref( ).get< IDirect3DDevice9* >( );

	g::cvars::init ( );

	END_FUNC

	return true;
}