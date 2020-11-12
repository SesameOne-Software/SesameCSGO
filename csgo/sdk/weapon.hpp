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

class c_econ_item {
public:
	uint32_t& account_id ( ) {
		return *reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( this ) + 0x1C );
	}

	const char* mdl_name ( ) {
		return *reinterpret_cast< const char** >( reinterpret_cast< uintptr_t >(this) + 0x94 );
	}

	const char* world_mdl_name ( ) {
		return *reinterpret_cast< const char** >( reinterpret_cast< uintptr_t >( this ) + 0x9C );
	}

	const char* inventory_image ( ) {
		using fn = const char* ( __thiscall* )( void* );
		return vfunc<fn> ( this, 5 )( this );
	}

	uint64_t& item_id ( ) {
		return *reinterpret_cast< uint64_t* >( reinterpret_cast< uintptr_t >( this ) + 0x8 );
	}

	uint64_t& original_id ( ) {
		return *reinterpret_cast< uint64_t* >( reinterpret_cast< uintptr_t >( this ) + 0x10 );
	}

	uint16_t& item_definition_index ( ) {
		return *reinterpret_cast< uint16_t* >( reinterpret_cast< uintptr_t >( this ) + 0x24 );
	}

	uint32_t& inventory ( ) {
		return *reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( this ) + 0x20 );
	}

	uint8_t& flags ( ) {
		return *reinterpret_cast< uint8_t* >( reinterpret_cast< uintptr_t >( this ) + 0x30 );
	}

	uint16_t& econ_item_data ( ) {
		return *reinterpret_cast< uint16_t* > ( reinterpret_cast< uintptr_t >( this ) + 0x26 );
	}

	void set_origin ( int origin ) {
		auto data = econ_item_data ( );
		econ_item_data ( ) = data ^ ( static_cast< uint8_t > ( data ) ^ static_cast< uint8_t > ( origin ) ) & 0x1F;
	}

	void set_level ( int level ) {
		auto data = econ_item_data ( );
		econ_item_data ( ) = data ^ ( data ^ ( level << 9 ) ) & 0x600;
	}

	void set_in_use ( bool in_use ) {
		auto data = econ_item_data ( );
		econ_item_data ( ) = data & 0x7FFF | ( in_use << 15 );
	}

	void set_rarity ( int rarity ) {
		auto data = econ_item_data ( );
		econ_item_data ( ) = ( data ^ ( rarity << 11 ) ) & 0x7800 ^ data;
	}

	void set_quality ( int quality ) {
		auto data = econ_item_data ( );
		econ_item_data ( ) = data ^ ( data ^ 32 * quality ) & 0x1E0;
	}

	void add_sticker ( int index, int kit, float wear, float scale, float rotation ) {
		set_attribute( 113 + 4 * index, kit );
		set_attribute ( 114 + 4 * index, wear );
		set_attribute ( 115 + 4 * index, scale );
		set_attribute ( 116 + 4 * index, rotation );
	}

	void set_stattrak ( int val ) {
		set_attribute ( 80, val );
		set_attribute ( 81, 0 );
		set_quality ( 9 );
	}

	void set_paintkit ( float kit ) {
		set_attribute( 6, kit );
	}

	void set_paint_seed ( float seed ) {
		set_attribute( 7, seed );
	}

	void set_paint_wear ( float wear ) {
		set_attribute( 8, wear );
	}
	
	int& equipped_position ( ) {
		return *reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( this ) + 0x248 );
	}
	
	void set_custom_name ( const char* name );
	void set_custom_desc ( const char* name );

	void set_attribute ( int index, float val );
	void set_attribute ( int index, int val );

	template< typename t >
	void set_or_add_attribute_by_name ( t val, const char* attribute_name );

	void clean_inventory_image_cache_dir ( );

	void update_equipped_state ( bool state );

	c_econ_item* static_data ( );
	c_econ_item* soc_data ( );
};

class weapon_t : public entity_t {
public:
	NETVAR( uint16_t, item_definition_index, "DT_BaseAttributableItem->m_iItemDefinitionIndex" );
	NETVAR ( int, mdl_idx, "DT_BaseAttributableItem->m_nModelIndex" );
	NETVAR( float, next_primary_attack, "DT_BaseCombatWeapon->m_flNextPrimaryAttack" );
	NETVAR( float, next_secondary_attack, "DT_BaseCombatWeapon->m_flNextSecondaryAttack" );
	NETVAR( int, ammo, "DT_BaseCombatWeapon->m_iClip1" );
	NETVAR( int, ammo2, "DT_BaseCombatWeapon->m_iClip2" );
	NETVAR( float, postpone_fire_time, "DT_BaseCombatWeapon->m_flPostponeFireReadyTime" );
	NETVAR( uint32_t, world_model_handle, "DT_BaseCombatWeapon->m_hWeaponWorldModel" );
	NETVAR( float, throw_time, "DT_BaseCSGrenade->m_fThrowTime" );
	NETVAR ( bool, pin_pulled, "DT_BaseCSGrenade->m_bPinPulled" );
	NETVAR( float, throw_strength, "DT_BaseCSGrenade->m_flThrowStrength" );
	NETVAR( uint64_t, original_owner_xuid, "DT_BaseAttributableItem->m_OriginalOwnerXuidLow" );
	NETVAR( uint32_t, original_owner_xuid_low, "DT_BaseAttributableItem->m_OriginalOwnerXuidLow" );
	NETVAR( uint32_t, original_owner_xuid_high, "DT_BaseAttributableItem->m_OriginalOwnerXuidHigh" );
	NETVAR( uint32_t, fallback_stattrak, "DT_BaseAttributableItem->m_nFallbackStatTrak" );
	NETVAR( uint32_t, fallback_paintkit, "DT_BaseAttributableItem->m_nFallbackPaintKit" );
	NETVAR( uint32_t, fallback_seed, "DT_BaseAttributableItem->m_nFallbackSeed" );
	NETVAR( float, fallback_wear, "DT_BaseAttributableItem->m_flFallbackWear" );
	NETVAR( uint32_t, fallback_quality, "DT_BaseAttributableItem->m_iEntityQuality" );
	NETVAR( uint32_t, item_id_low, "DT_BaseAttributableItem->m_iItemIDLow" );
	NETVAR( uint32_t, item_id_high, "DT_BaseAttributableItem->m_iItemIDHigh" );
	NETVAR( uint32_t, account, "DT_BaseAttributableItem->m_iAccountID" );
	NETVAR( const char*, name, "DT_BaseAttributableItem->m_szCustomName" );
	NETVAR ( float, last_shot_time, "DT_WeaponCSBase->m_fLastShotTime" );
	NETVAR ( int, sequence, "DT_BaseViewModel->m_nSequence" );

	weapon_t* world_mdl ( );
	c_econ_item* econ_item ( );

	weapon_info_t* data( );

	void update_accuracy( ) {
		using fn = void( __thiscall* )( void* );
		vfunc< fn >( this, 483 )( this );
	}

	float inaccuracy( );
	float spread( );
	float max_speed( );
};