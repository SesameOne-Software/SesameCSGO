#include "glow.hpp"
#include "../menu/menu.hpp"
#include "../hooks.hpp"
#include "../oxui/themes/purple.hpp"
#include "../globals.hpp"

enum stencil_operation_t {
	stencil_operation_keep = 1,
	stencil_operation_zero = 2,
	stencil_operation_replace = 3,
	stencil_operation_incrsat = 4,
	stencil_operation_decrsat = 5,
	stencil_operation_invert = 6,
	stencil_operation_incr = 7,
	stencil_operation_decr = 8,
	stencil_operation_force_dword = 0x7fffffff
};

enum stencil_comparison_function_t {
	stencil_comparison_function_never = 1,
	stencil_comparison_function_less = 2,
	stencil_comparison_function_equal = 3,
	stencil_comparison_function_lessequal = 4,
	stencil_comparison_function_greater = 5,
	stencil_comparison_function_notequal = 6,
	stencil_comparison_function_greaterequal = 7,
	stencil_comparison_function_always = 8,
	stencil_comparison_function_force_dword = 0x7fffffff
};

struct stencil_state_t {
	bool m_enable;
	int m_fail_op;
	int m_zfail_op;
	int m_pass_op;
	int m_compare_fn;
	int m_reference_val;
	uint32_t m_test_mask;
	uint32_t m_write_mask;

	stencil_state_t( ) {
		m_enable = false;
		m_pass_op = m_fail_op = m_zfail_op = stencil_operation_keep;
		m_compare_fn = stencil_comparison_function_always;
		m_reference_val = 0;
		m_test_mask = m_write_mask = 0xFFFFFFFF;
	}

	void set_stencil_state( void* ctx ) {
		using set_stencil_state_fn = void( __thiscall* )( void*, stencil_state_t* );

		if ( ctx )
			vfunc< set_stencil_state_fn >( ctx, 128 )( ctx, this );
	}
};

stencil_state_t stencil_state;

class glow_object_definition_t {
public:
	entity_t* m_ent;
	vec3_t m_glow_color;
	float m_glow_alpha;
	PAD( 8 );
	float m_bloom_amount;
	float m_localplayeriszeropoint3;
	bool m_render_when_occluded;
	bool m_render_when_unoccluded;
	bool m_full_bloom_render;
	PAD( 1 );
	int m_full_bloom_stencil_test_value;
	int m_style;
	int m_split_screen_slot;
	int m_next_free_slot;

	void set( float r, float g, float b, float a, int style = 0 ) {
		m_glow_color = vec3_t( r, g, b );
		m_glow_alpha = a;
		m_render_when_occluded = true;
		m_render_when_unoccluded = false;
		m_bloom_amount = 1.0f;
		m_style = style;
	}

	bool should_draw( int slot ) const {
		return get_entity( ) && ( m_split_screen_slot == -1 || m_split_screen_slot == slot ) && ( m_render_when_occluded || m_render_when_unoccluded );
	}

	entity_t* get_entity( ) const {
		return m_ent;
	}

	bool is_unused( ) const {
		return m_next_free_slot != -2;
	}
};

class c_glow_obj_mgr {
public:
	glow_object_definition_t* m_glow_object_definitions;
	int m_max_size;
	int m_pad;
	int m_size;
	glow_object_definition_t* m_more_glow_object_definitions;
	int m_current_objects;
};

c_glow_obj_mgr* glow_obj_mgr = nullptr;

void features::glow::cache_entities( ) {
	OPTION ( bool, glow, "Sesame->C->Glow->Main->Glow", oxui::object_checkbox );
	OPTION ( oxui::color, outline_clr, "Sesame->C->Glow->Colors->Glow", oxui::object_colorpicker );

	OPTION ( bool, team, "Sesame->C->Glow->Targets->Team", oxui::object_checkbox );
	OPTION ( bool, enemy, "Sesame->C->Glow->Targets->Enemy", oxui::object_checkbox );
	OPTION ( bool, local, "Sesame->C->Glow->Targets->Local", oxui::object_checkbox );

	if ( !glow_obj_mgr )
		glow_obj_mgr = pattern::search( _( "client.dll" ), _( "0F 11 05 ? ? ? ? 83 C8 01" ) ).add( 3 ).deref( ).get< c_glow_obj_mgr* >( );

	if ( !g::local || !glow )
		return;

	for ( auto i = 0; i < glow_obj_mgr->m_size; i++ ) {
		if ( glow_obj_mgr->m_glow_object_definitions [ i ].is_unused( ) || !glow_obj_mgr->m_glow_object_definitions [ i ].get_entity( ) )
			continue;

		auto& glow_object = glow_obj_mgr->m_glow_object_definitions [ i ];

		auto entity = reinterpret_cast< player_t* >( glow_object.m_ent );

		if ( !entity )
			continue;

		const auto client_class = entity->client_class( );

		if ( !client_class )
			continue;

		if ( !enemy
			&& entity->team ( ) != g::local->team ( ) )
			continue;

		if ( !team
			&& entity->team ( ) == g::local->team ( )
			&& entity != g::local )
			continue;

		if ( !local
			&& entity == g::local )
			continue;

		if ( ( enemy || team )
			&& !entity->alive ( ) )
			continue;

		/*if ( client_class->m_class_id == 1 || ( client_class->m_class_id >= 231 && client_class->m_class_id <= 272 ) )
			glow_object.set( 0.007f, 0.949f, 0.705f, 1.0f );
		else*/ if ( client_class->m_class_id == 40 ) {
			if ( !entity->valid( ) )
				continue;

			glow_object.set( static_cast< float >( outline_clr.r ) / 255.0f, static_cast< float >( outline_clr.g ) / 255.0f, static_cast< float >( outline_clr.b ) / 255.0f, static_cast< float >( outline_clr.a ) / 255.0f );
		}
		/*else if ( client_class->m_class_id == 8
			|| client_class->m_class_id == 9
			|| client_class->m_class_id == 13 
			|| client_class->m_class_id == 47 
			|| client_class->m_class_id == 96 
			|| client_class->m_class_id == 99 
			|| client_class->m_class_id == 112 
			|| client_class->m_class_id == 151 
			|| client_class->m_class_id == 152 
			|| client_class->m_class_id == 155
			|| client_class->m_class_id == 156 ) {
			glow_object.set( 0.949f, 0.019f, 0.0f, 1.0f );
		}*/
	}
}