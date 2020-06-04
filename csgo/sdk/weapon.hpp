#pragma once
#include "entity.hpp"
#include "../utils/padding.hpp"

// Generated using ReClass 2016

class weapon_info_t;

class weapon_info_t
{
public:
	PAD ( 20 );
	int m_max_clip;
	PAD ( 12 );
	int m_max_reserved_ammo;
	PAD ( 96 );
	char* m_hud_name;
	char* m_weapon_name;
	PAD ( 56 );
	int m_type;
	PAD ( 4 );
	int m_price;
	int m_reward;
	PAD ( 4 );
	float m_fire_rate;
	PAD ( 12 );
	uint8_t m_full_auto;
	PAD ( 3 );
	int m_dmg;
	float m_armor_ratio;
	int m_bullets;
	float m_penetration;
	PAD ( 8 );
	float m_range;
	float m_range_modifier;
	float m_throw_velocity;
	PAD ( 12 );
	uint8_t m_silencer;
	PAD ( 15 );
	float m_max_speed;
	float m_max_speed_alt;
	PAD ( 76 );
	int m_recoil_seed;
	PAD ( 32 );
};

class weapon_t : public entity_t {
public:
	NETVAR( short, item_definition_index, "DT_BaseAttributableItem->m_iItemDefinitionIndex" );
	NETVAR( float, next_primary_attack, "DT_BaseCombatWeapon->m_flNextPrimaryAttack" );
	NETVAR( float, next_secondary_attack, "DT_BaseCombatWeapon->m_flNextSecondaryAttack" );
	NETVAR( int, ammo, "DT_BaseCombatWeapon->m_iClip1" );
	NETVAR( int, ammo2, "DT_BaseCombatWeapon->m_iClip2" );
	NETVAR( float, postpone_fire_time, "DT_BaseCombatWeapon->m_flPostponeFireReadyTime" );
	NETVAR( std::uint64_t, world_model_handle, "DT_BaseCombatWeapon->m_hWeaponWorldModel" );
	NETVAR( float, throw_time, "DT_BaseCSGrenade->m_fThrowTime" );
	NETVAR( bool, pin_pulled, "DT_BaseCSGrenade->m_bPinPulled" );
	NETVAR( std::uint64_t, original_owner_xuid, "DT_BaseAttributableItem->m_OriginalOwnerXuidLow" );
	NETVAR( std::uint32_t, original_owner_xuid_low, "DT_BaseAttributableItem->m_OriginalOwnerXuidLow" );
	NETVAR( std::uint32_t, original_owner_xuid_high, "DT_BaseAttributableItem->m_OriginalOwnerXuidHigh" );
	NETVAR( std::uint32_t, fallback_stattrak, "DT_BaseAttributableItem->m_nFallbackStatTrak" );
	NETVAR( std::uint32_t, fallback_paintkit, "DT_BaseAttributableItem->m_nFallbackPaintKit" );
	NETVAR( std::uint32_t, fallback_seed, "DT_BaseAttributableItem->m_nFallbackSeed" );
	NETVAR( float, fallback_wear, "DT_BaseAttributableItem->m_flFallbackWear" );
	NETVAR( std::uint32_t, fallback_quality, "DT_BaseAttributableItem->m_iEntityQuality" );
	NETVAR( std::uint32_t, item_id_low, "DT_BaseAttributableItem->m_iItemIDLow" );
	NETVAR( std::uint32_t, item_id_high, "DT_BaseAttributableItem->m_iItemIDHigh" );
	NETVAR( std::uint32_t, account, "DT_BaseAttributableItem->m_iAccountID" );
	NETVAR( const char*, name, "DT_BaseAttributableItem->m_szCustomName" );
	NETVAR( float, last_shot_time, "DT_WeaponCSBase->m_fLastShotTime" );

	weapon_info_t* data( );

	void update_accuracy( ) {
		using fn = void( __thiscall* )( void* );
		vfunc< fn >( this, 479 )( this );
	}

	float inaccuracy( );
	float spread( );
	float max_speed( );
};