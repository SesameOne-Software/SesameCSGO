#pragma once

struct ucmd_t;
class vec3_t;
class player_t;

enum class round_t : int {
	starting = 0,
	in_progress,
	ending,
};

namespace g {
	extern bool unload;

	extern player_t* local;
	extern ucmd_t* ucmd;
	extern ucmd_t sent_cmd;
	extern vec3_t angles;
	extern bool hold_aim;
	extern bool next_tickbase_shot;
	extern bool send_packet;
	extern int shifted_tickbase;
	extern int shifted_amount;
	extern int dt_ticks_to_shift;
	extern int dt_recharge_time;
	extern round_t round;
}