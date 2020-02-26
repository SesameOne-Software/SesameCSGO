#pragma once
#include "../utils/padding.hpp"
#include "vec3.hpp"

struct ucmd_t {
	PAD( 4 );
	int	m_cmdnum;
	int m_tickcount;
	vec3_t m_angs;
	vec3_t m_aimdir;
	float m_fmove;
	float m_smove;
	float m_umove;
	int m_buttons;
	std::uint8_t m_impulse;
	int m_weaponselect;
	int m_weaponsubtype;
	int m_randseed;
	short m_mousedx;
	short m_mousedy;
	bool m_hasbeenpredicted;
	PAD( 24 );

	void reset( ) {
		this->m_cmdnum = 0;
		this->m_tickcount = 0;
		this->m_angs.init( );
		this->m_fmove = 0.0f;
		this->m_smove = 0.0f;
		this->m_umove = 0.0f;
		this->m_buttons = 0;
		this->m_impulse = 0;
		this->m_weaponselect = 0;
		this->m_weaponsubtype = 0;
		this->m_randseed = 0;
		this->m_mousedx = 0;
		this->m_mousedy = 0;
		this->m_hasbeenpredicted = false;
	}
};