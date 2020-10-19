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

	//ADD SUPPORT FOR CRC32 SO YOU CAN VERIFY COMMANDS IN THE CIRCULAR BUFFER

	//backend::crc::crc32_t get_checksum ( void ) const {
	//	backend::crc::crc32_t crc;
	//	backend::crc::crc32_init ( &crc );
	//
	//	backend::crc::crc32_process_buf ( &crc, &m_cmd_nr, sizeof ( m_cmd_nr ) );
	//	backend::crc::crc32_process_buf ( &crc, &m_tick_count, sizeof ( m_tick_count ) );
	//	backend::crc::crc32_process_buf ( &crc, &m_view_angles, sizeof ( m_view_angles ) );
	//	backend::crc::crc32_process_buf ( &crc, &m_aim_dir, sizeof ( m_aim_dir ) );
	//	backend::crc::crc32_process_buf ( &crc, &m_move, sizeof ( m_move ) );
	//	backend::crc::crc32_process_buf ( &crc, &m_buttons, sizeof ( m_buttons ) );
	//	backend::crc::crc32_process_buf ( &crc, &m_impulse, sizeof ( m_impulse ) );
	//	backend::crc::crc32_process_buf ( &crc, &m_weapon_select, sizeof ( m_weapon_select ) );
	//	backend::crc::crc32_process_buf ( &crc, &m_weapon_subtype, sizeof ( m_weapon_subtype ) );
	//	backend::crc::crc32_process_buf ( &crc, &m_random_seed, sizeof ( m_random_seed ) );
	//	backend::crc::crc32_process_buf ( &crc, &m_mouse_dx, sizeof ( m_mouse_dx ) );
	//	backend::crc::crc32_process_buf ( &crc, &m_mouse_dy, sizeof ( m_mouse_dy ) );
	//
	//	backend::crc::crc32_final ( &crc );
	//	return crc;
	//}

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

struct verified_ucmd_t {
public:
	ucmd_t m_cmd;
	unsigned long m_crc;
};