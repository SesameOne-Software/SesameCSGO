#include "globals.hpp"
#include "sdk/sdk.hpp"

bool g::unload = false;

round_t g::round = round_t::in_progress;
player_t* g::local = nullptr;
ucmd_t* g::ucmd = nullptr;
ucmd_t g::sent_cmd { };
vec3_t g::angles = vec3_t( 0.0f, 0.0f, 0.0f );
bool g::hold_aim = false;
bool g::next_tickbase_shot = false;
int g::shifted_tickbase = 0;
int g::shifted_amount=0;
bool g::send_packet = true;
int g::dt_ticks_to_shift = 0;
int g::dt_recharge_time = 0;
int g::cock_ticks = 0;
bool g::can_fire_revolver = false;
PLoader_Info g::loader_data;