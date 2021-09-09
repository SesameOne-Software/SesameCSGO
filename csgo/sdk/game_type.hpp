#pragma once
#include "../utils/vfunc.hpp"
#include "../utils/padding.hpp"

enum class game_type_t : int {
	inavlid = -1,
	casual,
	competitive,
	progressive,
	bomb,
	danger_zone = 6,
	max
};

class c_game_type {
public:
	game_type_t get_game_type ( ) {
		using get_game_type_fn = game_type_t ( __thiscall* )( c_game_type* );
		return vfunc< get_game_type_fn > ( this, 8 )( this );
	}

	int get_game_mode ( ) {
		using get_game_mode_fn = int ( __thiscall* )( c_game_type* );
		return vfunc< get_game_mode_fn > ( this, 9 )( this );
	}
};