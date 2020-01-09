#pragma once

struct ucmd_t;
class vec3_t;

namespace g {
	extern vec3_t last_fake;
	extern vec3_t last_real;
	extern ucmd_t* ucmd;
	extern ucmd_t raw_ucmd;
	extern bool send_packet;
	extern bool* psend_packet;
}