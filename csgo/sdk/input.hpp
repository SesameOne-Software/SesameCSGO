#pragma once
#include "../utils/vfunc.hpp"
#include "../utils/padding.hpp"
#include "vec3.hpp"
#include "ucmd.hpp"

class c_input {
public:
	void* pvftable; //0x00
	PAD ( 0x8 );
	bool m_track_ir_available; //0x04
	bool m_mouse_initialized; //0x05
	bool m_mouse_active; //0x06
	PAD ( 0x9A ); //0x08
	bool m_camera_in_thirdperson; //0x9D
	PAD ( 0x2 );
	vec3_t m_camera_offset; //0xA0
	PAD ( 0x38 );
	ucmd_t* m_cmds;
	verified_ucmd_t* m_verified_cmds;

	ucmd_t* get_cmd ( int sequence_number ) {
		return &m_cmds [ sequence_number % 150 ];
	}

	verified_ucmd_t* get_verified_cmd ( int sequence_number ) {
		return &m_verified_cmds [ sequence_number % 150 ];
	}
};