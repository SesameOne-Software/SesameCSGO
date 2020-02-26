#pragma once

struct ucmd_t;
class vec3_t;
class player_t;

namespace g {
	extern player_t* local;
	extern ucmd_t* ucmd;
	extern bool send_packet;
}