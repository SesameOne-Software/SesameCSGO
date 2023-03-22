#pragma once
#include "entity.hpp"
#include "../utils/padding.hpp"

// Generated using ReClass 2016

enum class weapons_t : int {
	none = 0,
	deagle = 1,
	elite = 2,
	fiveseven = 3,
	glock = 4,
	ak47 = 7,
	aug = 8,
	awp = 9,
	famas = 10,
	g3sg1 = 11,
	galil = 13,
	m249 = 14,
	m4a4 = 16,
	mac10 = 17,
	p90 = 19,
	mp5_sd = 23,
	ump45 = 24,
	xm1014 = 25,
	bizon = 26,
	mag7 = 27,
	negev = 28,
	sawedoff = 29,
	tec9 = 30,
	taser = 31,
	p2000 = 32,
	mp7 = 33,
	mp9 = 34,
	nova = 35,
	p250 = 36,
	scar20 = 38,
	sg553 = 39,
	ssg08 = 40,
	knife_ct = 42,
	flashbang = 43,
	hegrenade = 44,
	smoke = 45,
	molotov = 46,
	decoy = 47,
	firebomb = 48,
	c4 = 49,
	musickit = 58,
	knife_t = 59,
	m4a1s = 60,
	usps = 61,
	tradeupcontract = 62,
	cz75a = 63,
	revolver = 64,
	knife_bayonet = 500,
	knife_css = 503,
	knife_flip = 505,
	knife_gut = 506,
	knife_karambit = 507,
	knife_m9_bayonet = 508,
	knife_huntsman = 509,
	knife_falchion = 512,
	knife_bowie = 514,
	knife_butterfly = 515,
	knife_shadow_daggers = 516,
	knife_cord = 517,
	knife_canis = 518,
	knife_ursus = 519,
	knife_gypsy_jackknife = 520,
	knife_outdoor = 521,
	knife_stiletto = 522,
	knife_widowmaker = 523,
	knife_skeleton = 525,
	glove_studded_bloodhound = 5027,
	glove_t_side = 5028,
	glove_ct_side = 5029,
	glove_sporty = 5030,
	glove_slick = 5031,
	glove_leather_wrap = 5032,
	glove_motorcycle = 5033,
	glove_specialist = 5034,
	glove_studded_hydra = 5035
};

enum weapon_type_t : uint32_t {
	knife = 0,
	pistol,
	smg,
	rifle,
	shotgun,
	sniper,
	lmg,
	c4,
	placeholder,
	grenade,
	unknown
};

class weapon_info_t;

class weapon_info_t
{
public:
	PAD ( 20 );
	int m_max_clip;
	PAD ( 12 );
	int m_max_reserved_ammo;
	PAD ( 0x4 );												// 0x0028
	const char* m_world_model;						// 0x002C
	const char* m_view_model;							// 0x0030
	const char* m_world_dropped_model;				// 0x0034
	PAD ( 0x48 );											// 0x0038
	const char* m_ammo_type;							// 0x0080
	uint8_t           pad_0084 [ 4 ];
	char* m_hud_name;
	char* m_weapon_name;
	PAD ( 56 );
	weapon_type_t m_type;
	int m_price;
	int m_reward;
	PAD ( 4 );
	float m_fire_rate;
	float m_fire_rate_alt;
	PAD ( 8 );
	bool m_full_auto;
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
	bool m_silencer;
	PAD ( 3 );
	const char* m_silencer_model;						// 0x0120
	int				  m_crosshair_min_distance;				// 0x0124
	int				  m_crosshair_delta_distance;			// 0x0128
	float m_max_speed;
	float m_max_speed_alt;
	float m_speed_modifier;
	float m_spread;
	float m_spread_alt;
	float m_inaccuracy_crouch;
	float m_inaccuracy_crouch_alt;
	float m_inaccuracy_stand;
	float m_inaccuracy_stand_alt;
	float m_inaccuracy_jump_start;
	float m_inaccuracy_jump;
	float m_inaccuracy_jump_alt;
	float m_inaccuracy_land;
	float m_inaccuracy_land_alt;
	float m_inaccuracy_ladder;
	float m_inaccuracy_ladder_alt;
	float m_inaccuracy_fire;
	float m_inaccuracy_fire_alt;
	float m_inaccuracy_move;
	float m_inaccuracy_move_alt;
	float m_inaccuracy_reload;
	int m_recoil_seed;
	PAD ( 36 );
};

class c_econ_item {
public:
	__forceinline uint32_t& account_id ( ) {
		return *reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( this ) + 0x1C );
	}

	__forceinline const char* icon_name ( ) {
		return *reinterpret_cast< const char** >( reinterpret_cast< uintptr_t >( this ) + 0x198 );
	}

	__forceinline const char* mdl_name ( ) {
		return *reinterpret_cast< const char** >( reinterpret_cast< uintptr_t >(this) + 0x94 );
	}

	__forceinline const char* world_mdl_name ( ) {
		return *reinterpret_cast< const char** >( reinterpret_cast< uintptr_t >( this ) + 0x9C );
	}

	__forceinline const char* inventory_image ( ) {
		using fn = const char* ( __thiscall* )( void* );
		return vfunc<fn> ( this, 5 )( this );
	}

	__forceinline uint64_t& item_id ( ) {
		return *reinterpret_cast< uint64_t* >( reinterpret_cast< uintptr_t >( this ) + 0x8 );
	}

	__forceinline uint64_t& original_id ( ) {
		return *reinterpret_cast< uint64_t* >( reinterpret_cast< uintptr_t >( this ) + 0x10 );
	}

	__forceinline weapons_t& item_definition_index ( ) {
		return *reinterpret_cast< weapons_t* >( reinterpret_cast< uintptr_t >( this ) + 0x24 );
	}

	__forceinline uint32_t& inventory ( ) {
		return *reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( this ) + 0x20 );
	}

	__forceinline uint8_t& flags ( ) {
		return *reinterpret_cast< uint8_t* >( reinterpret_cast< uintptr_t >( this ) + 0x30 );
	}

	__forceinline uint16_t& econ_item_data ( ) {
		return *reinterpret_cast< uint16_t* > ( reinterpret_cast< uintptr_t >( this ) + 0x26 );
	}

	__forceinline void set_origin ( int origin ) {
		auto data = econ_item_data ( );
		econ_item_data ( ) = data ^ ( static_cast< uint8_t > ( data ) ^ static_cast< uint8_t > ( origin ) ) & 0x1F;
	}

	__forceinline void set_level ( int level ) {
		auto data = econ_item_data ( );
		econ_item_data ( ) = data ^ ( data ^ ( level << 9 ) ) & 0x600;
	}

	__forceinline void set_in_use ( bool in_use ) {
		auto data = econ_item_data ( );
		econ_item_data ( ) = data & 0x7FFF | ( in_use << 15 );
	}

	__forceinline void set_rarity ( int rarity ) {
		auto data = econ_item_data ( );
		econ_item_data ( ) = ( data ^ ( rarity << 11 ) ) & 0x7800 ^ data;
	}

	__forceinline void set_quality ( int quality ) {
		auto data = econ_item_data ( );
		econ_item_data ( ) = data ^ ( data ^ 32 * quality ) & 0x1E0;
	}

	__forceinline void add_sticker ( int index, int kit, float wear, float scale, float rotation ) {
		set_attribute( 113 + 4 * index, kit );
		set_attribute ( 114 + 4 * index, wear );
		set_attribute ( 115 + 4 * index, scale );
		set_attribute ( 116 + 4 * index, rotation );
	}

	__forceinline void set_stattrak ( int val ) {
		set_attribute ( 80, val );
		set_attribute ( 81, 0 );
		set_quality ( 9 );
	}

	__forceinline void set_paintkit ( float kit ) {
		set_attribute( 6, kit );
	}

	__forceinline void set_paint_seed ( float seed ) {
		set_attribute( 7, seed );
	}

	__forceinline void set_paint_wear ( float wear ) {
		set_attribute( 8, wear );
	}
	
	__forceinline int& equipped_position ( ) {
		return *reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( this ) + 0x248 );
	}

	/*
	*	XREF: "Error Parsing PaintData in %s! \n" offset is in same function call
	*	https://github.com/perilouswithadollarsign/cstrike15_src/blob/29e4c1fda9698d5cebcdaf1a0de4b829fa149bf8/game/shared/cstrike15/cstrike15_item_schema.cpp#L188
	*/
	const char* get_definition_name ( ) {
		return *reinterpret_cast< const char** >( reinterpret_cast< uintptr_t >( this ) + 0x1BC );
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

	std::string build_inventory_image ( );
};

class weapon_t : public entity_t {
public:
	NETVAR( weapons_t, item_definition_index, "DT_BaseAttributableItem->m_iItemDefinitionIndex" );
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
	NETVAR ( char* , name, "DT_BaseAttributableItem->m_szCustomName" );
	NETVAR ( float, last_shot_time, "DT_WeaponCSBase->m_fLastShotTime" );
	NETVAR ( int, sequence, "DT_BaseViewModel->m_nSequence" );
	NETVAR ( bool, initialized, "DT_BaseAttributableItem->m_bInitialized" );
	NETVAR ( int, zoom_level, "DT_WeaponCSBaseGun->m_zoomLevel" );
	NETVAR( float , accuracy_penalty , "DT_WeaponCSBase->m_fAccuracyPenalty" );
	NETVAR( float , recoil_index , "DT_WeaponCSBase->m_flRecoilIndex" );
	NETVAR ( float, cycle, "DT_BaseAnimating->m_flCycle" );
	NETVAR ( float, anim_time, "DT_BaseEntity->m_flAnimTime" );

	weapon_t* world_mdl ( );
	c_econ_item* econ_item ( );

	weapon_info_t* data( );

	__forceinline void update_accuracy( ) {
		using fn = void( __thiscall* )( void* );
		vfunc< fn >( this, 471 )( this );
	}

	float inaccuracy( );
	float spread( );
	float max_speed( );
};