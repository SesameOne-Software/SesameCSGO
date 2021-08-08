#pragma once
#include "netvar.hpp"
#include "vec3.hpp"
#include "../utils/vfunc.hpp"

struct client_class_t;

class entity_t {
public:
	OFFSET( std::uint32_t, idx, 0x64 );
	OFFSET( bool, dormant, 0xED );
	POFFSET( void*, renderable, 0x4 );
	POFFSET( void*, networkable, 0x8 );
	NETVAR( vec3_t, origin, "DT_BaseEntity->m_vecOrigin" );
	NETVAR( std::uint32_t, team, "DT_BaseEntity->m_iTeamNum" );
	NETVAR( std::uint32_t, highlight_r, "DT_BaseAnimating->m_nHighlightColorR" );
	NETVAR( std::uint32_t, highlight_g, "DT_BaseAnimating->m_nHighlightColorG" );
	NETVAR( std::uint32_t, highlight_b, "DT_BaseAnimating->m_nHighlightColorB" );
	NETVAR( std::uint32_t, model_scale, "DT_BaseAnimating->m_flModelScale" );
	NETVAR( bool, did_smoke_effect, "DT_SmokeGrenadeProjectile->m_bDidSmokeEffect" );

	NETVAR ( bool, use_auto_exposure_min, "DT_EnvTonemapController->m_bUseCustomAutoExposureMin" );
	NETVAR ( bool, use_auto_exposure_max, "DT_EnvTonemapController->m_bUseCustomAutoExposureMax" );
	NETVAR ( bool, use_bloom_scale, "DT_EnvTonemapController->m_bUseCustomBloomScale" );
	NETVAR ( float, auto_exposure_min, "DT_EnvTonemapController->m_flCustomAutoExposureMin" );
	NETVAR ( float, auto_exposure_max, "DT_EnvTonemapController->m_flCustomAutoExposureMax" );
	NETVAR ( float, bloom_scale, "DT_EnvTonemapController->m_flCustomBloomScale" );
	NETVAR ( float, bloom_scale_min, "DT_EnvTonemapController->m_flCustomBloomScaleMinimum" );
	NETVAR ( float, bloom_exponent, "DT_EnvTonemapController->m_flBloomExponent" );
	NETVAR ( float, bloom_saturation, "DT_EnvTonemapController->m_flBloomSaturation" );
	NETVAR ( float, tonemap_percent_target, "DT_EnvTonemapController->m_flTonemapPercentTarget" );
	NETVAR ( float, tonemap_percent_bright_pixels, "DT_EnvTonemapController->m_flTonemapPercentBrightPixels" );
	NETVAR ( float, tonemap_min_avg_lum, "DT_EnvTonemapController->m_flTonemapMinAvgLum" );
	NETVAR ( float, tonemap_rate, "DT_EnvTonemapController->m_flTonemapRate" );

	NETVAR ( bool, fog_enable, "DT_FogController->m_fog.enable" );
	NETVAR ( bool, fog_blend, "DT_FogController->m_fog.blend" );
	NETVAR ( uint32_t, fog_color_primary, "DT_FogController->m_fog.colorPrimary" );
	NETVAR ( uint32_t, fog_color_secondary, "DT_FogController->m_fog.colorSecondary" );
	NETVAR ( float, fog_start, "DT_FogController->m_fog.start" );
	NETVAR ( float, fog_end, "DT_FogController->m_fog.end" );
	NETVAR ( float, fog_far_z, "DT_FogController->m_fog.farz" );
	NETVAR ( float, fog_max_density, "DT_FogController->m_fog.maxdensity" );
	NETVAR ( uint32_t, fog_color_primary_lerp_to, "DT_FogController->m_fog.colorPrimaryLerpTo" );
	NETVAR ( uint32_t, fog_color_secondary_lerp_to, "DT_FogController->m_fog.colorSecondaryLerpTo" );
	NETVAR ( float, fog_start_lerp_to, "DT_FogController->m_fog.startLerpTo" );
	NETVAR ( float, fog_end_lerp_to, "DT_FogController->m_fog.endLerpTo" );

	NETVAR ( uint32_t, mdl_idx, "DT_BaseEntity->m_nModelIndex" );

	void set_model_idx ( int i ) {
		vfunc< void ( __thiscall* )( void*, int ) > ( this, 75 )( this, i );
	}

	void draw( ) {
		using drawmodel_fn = int( __thiscall* )( void*, int, std::uint8_t );
		vfunc< drawmodel_fn >( renderable( ), 9 )( renderable( ), 1, 255 );
	}

	client_class_t* client_class( ) {
		using fn = client_class_t * ( __thiscall* )( void* );
		return vfunc< fn >( networkable( ), 2 )( networkable( ) );
	}

	bool is_player ( ) {
		return vfunc< bool ( __thiscall* )( entity_t* ) > ( this, 157 )( this );
	}
};

class planted_c4_t : public entity_t {
public:
	NETVAR( bool, bomb_ticking, "DT_PlantedC4->m_bBombTicking" );
	NETVAR( int, bomb_site, "DT_PlantedC4->m_nBombSite" );
	NETVAR( float, c4_blow, "DT_PlantedC4->m_flC4Blow" );
	NETVAR( float, timer_length, "DT_PlantedC4->m_flTimerLength" );
	NETVAR( float, defuse_length, "DT_PlantedC4->m_flDefuseLength" );
	NETVAR( float, defuse_countdown, "DT_PlantedC4->m_flDefuseCountDown" );
	NETVAR( bool, bomb_defused, "DT_PlantedC4->m_bBombDefused" );
	NETVAR( uint32_t, defuser_handle, "DT_PlantedC4->m_hBombDefuser" );

	void* get_defuser( );
};

class tonemap_controller_t : public entity_t {
public:
	NETVAR( bool, use_custom_auto_exposure_min, "DT_EnvTonemapController->m_bUseCustomAutoExposureMin" );
	NETVAR( bool, use_custom_auto_exposure_max, "DT_EnvTonemapController->m_bUseCustomAutoExposureMax" );
	NETVAR( bool, use_custom_bloom_scale, "DT_EnvTonemapController->m_bUseCustomBloomScale" );
	NETVAR( float, custom_auto_exposure_min, "DT_EnvTonemapController->m_flCustomAutoExposureMin" );
	NETVAR( float, custom_auto_exposure_max, "DT_EnvTonemapController->m_flCustomAutoExposureMax" );
	NETVAR( float, custom_bloom_scale, "DT_EnvTonemapController->m_flCustomBloomScale" );
	NETVAR( float, custom_bloom_scale_min, "DT_EnvTonemapController->m_flCustomBloomScaleMinimum" );
	NETVAR( float, bloom_exponent, "DT_EnvTonemapController->m_flBloomExponent" );
};