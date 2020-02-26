#pragma once
#include "entity.hpp"
#include "../utils/padding.hpp"

// Generated using ReClass 2016

class weapon_info_t;

class weapon_info_t
{
public:
	std::uint8_t pad_0x0000 [ 0x4 ]; //0x0000
	char* m_weapon_name; //0x0004 
	std::uint8_t pad_0x0008 [ 0xC ]; //0x0008
	std::uint32_t m_max_clip; //0x0014 
	std::uint8_t pad_0x0018 [ 0x68 ]; //0x0018
	char* m_ammo_name; //0x0080 
	char* m_ammo_name_2; //0x0084 
	char* m_hud_name; //0x0088 
	char* m_weapon_id; //0x008C 
	std::uint8_t pad_0x0090 [ 0x3C ]; //0x0090
	std::uint32_t m_type; //0x00CC 
	std::uint32_t m_price; //0x00D0 
	std::uint32_t m_reward; //0x00D4 
	std::uint8_t pad_0x00D8 [ 0x14 ]; //0x00D8
	std::uint8_t m_full_auto; //0x00EC 
	std::uint8_t pad_0x00ED [ 0x3 ]; //0x00ED
	std::uint32_t m_dmg; //0x00F0 
	float m_armor_ratio; //0x00F4 
	std::uint32_t m_bullets; //0x00F8 
	float m_penetration; //0x00FC 
	std::uint8_t pad_0x0100 [ 0x8 ]; //0x0100
	float m_range; //0x0108 
	float m_range_modifier; //0x010C 
	std::uint8_t pad_0x0110 [ 0x20 ]; //0x0110
	float m_max_speed; //0x0130 
	float m_max_speed_alt; //0x0134 
	std::uint8_t pad_0x0138 [ 0x108 ]; //0x0138
}; //Size=0x0240

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
	NETVAR( float, last_shot_time, "DT_BaseCombatWeapon->m_fLastShotTime" );

	weapon_info_t* data( );

	void update_accuracy( ) {
		using fn = void( __thiscall*)(void* );
		vfunc< fn >( this, 478 )( this );
	}

	float inaccuracy( );
	float spread( );
	float max_speed( );
};