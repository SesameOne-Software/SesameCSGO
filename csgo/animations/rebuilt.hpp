#pragma once
#include <deque>
#include <array>
#include <optional>
#include "../sdk/sdk.hpp"

namespace anims::rebuilt {
	/* anim utils */
	void invalidate_physics_recursive( animstate_t* anim_state , int flags );
	void reset_layer( animstate_t* anim_state , int layer );
	void set_sequence( animstate_t* anim_state , int layer, int sequence );
	void set_cycle( animstate_t* anim_state , int layer , float cycle );
	void increment_layer_cycle_weight_rate_generic( animstate_t* anim_state , int layer );
	bool is_sequence_completed( animstate_t* anim_state , int layer );
	void set_order( animstate_t* anim_state , int layer , int order );
	void set_weight( animstate_t* anim_state , int layer , float weight );
	void set_rate( animstate_t* anim_state , int layer , float rate );
	void increment_layer_cycle( animstate_t* anim_state , int layer, bool allow_loop );
	void increment_layer_weight( animstate_t* anim_state , int layer );
	void set_weight_rate( animstate_t* anim_state , int layer, float previous );
	int get_layer_activity( animstate_t* anim_state , int layer );
	int select_sequence_from_act_mods( animstate_t* anim_state , int act );
	float get_layer_ideal_weight_from_seq_cycle( animstate_t* anim_state , int layer );
	float get_first_sequence_anim_tag( animstate_t* anim_state, int seq, int tag, float start, float end );
	void bone_shapshot_should_capture( void* bone_snapshot , float a2 );
	void* get_model_ptr( player_t* player );
	void* seq_desc( void* mdl, int seq );

	/* anim update funcs */
	void setup_velocity( animstate_t* anim_state );
	void setup_aim_matrix( animstate_t* anim_state );
	void setup_weapon_action( animstate_t* anim_state );
	void setup_movement( animstate_t* anim_state );
	void setup_alive_loop( animstate_t* anim_state );
	void setup_whole_body_action( animstate_t* anim_state );
	void setup_flashed_reaction( animstate_t* anim_state );
	void setup_flinch( animstate_t* anim_state );
	void setup_lean( animstate_t* anim_state );

	/* anim update */
	void update( animstate_t* anim_state , vec3_t angles, vec3_t origin );
}