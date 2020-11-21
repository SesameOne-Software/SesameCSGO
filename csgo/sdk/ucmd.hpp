#pragma once
#include "../utils/padding.hpp"
#include "vec3.hpp"

#include "../crc32/crc32.hpp"

enum class buttons_t : uint32_t {
	none = 0,
attack        =        (1 << 0)	,
jump          =        (1 << 1)	,
duck          =        (1 << 2)	,
forward       =        (1 << 3)	,
back          =        (1 << 4)	,
use           =        (1 << 5)	,
cancel        =        (1 << 6)	,
_left          =        (1 << 7)	,
_right         =        (1 << 8)	,
left      =        (1 << 9)	,
right     =        (1 << 10)	,
attack2       =        (1 << 11)	,
run           =        (1 << 12)	,
reload        =        (1 << 13)	,
alt1          =        (1 << 14)	,
alt2          =        (1 << 15)	,
score         =        (1 << 16)  ,
speed         =        (1 << 17)  ,
walk          =        (1 << 18)  ,
zoom          =        (1 << 19)  ,
weapon1       =        (1 << 20)  ,
weapon2       =        (1 << 21)  ,
bullrush      =        (1 << 22)	,
grenade1      =        (1 << 23)  ,
grenade2      =        (1 << 24)  ,
attack3       =        (1 << 25)
};

ENUM_BITMASK ( buttons_t );

struct ucmd_t {
	PAD( 4 );
	int	m_cmdnum;
	int m_tickcount;
	vec3_t m_angs;
	vec3_t m_aimdir;
	float m_fmove;
	float m_smove;
	float m_umove;
	buttons_t m_buttons;
	uint8_t m_impulse;
	int m_weaponselect;
	int m_weaponsubtype;
	int m_randseed;
	short m_mousedx;
	short m_mousedy;
	bool m_hasbeenpredicted;
	PAD( 24 );

	uint32_t get_checksum ( ) {
		uint32_t crc;
		crc32::init ( crc );
	
		crc32::process ( crc, m_cmdnum );
		crc32::process ( crc, m_tickcount );
		crc32::process ( crc, m_angs );
		crc32::process ( crc, m_aimdir );
		crc32::process ( crc, m_fmove );
		crc32::process ( crc, m_smove );
		crc32::process ( crc, m_umove );
		crc32::process ( crc, m_buttons );
		crc32::process ( crc, m_impulse );
		crc32::process ( crc, m_weaponselect );
		crc32::process ( crc, m_weaponsubtype );
		crc32::process ( crc, m_randseed );
		crc32::process ( crc, m_mousedx );
		crc32::process ( crc, m_mousedy );
	
		crc32::final ( crc );

		return crc;
	}

	void reset( ) {
		this->m_cmdnum = 0;
		this->m_tickcount = 0;
		this->m_angs.init( );
		this->m_fmove = 0.0f;
		this->m_smove = 0.0f;
		this->m_umove = 0.0f;
		this->m_buttons = buttons_t::none;
		this->m_impulse = 0;
		this->m_weaponselect = 0;
		this->m_weaponsubtype = 0;
		this->m_randseed = 0;
		this->m_mousedx = 0;
		this->m_mousedy = 0;
		this->m_hasbeenpredicted = false;
	}
};

struct verified_ucmd_t {
public:
	ucmd_t m_cmd;
	uint32_t m_crc;
};