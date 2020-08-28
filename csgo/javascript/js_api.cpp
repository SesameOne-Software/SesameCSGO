#include "../sdk/sdk.hpp"
#include <shlobj.h>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include "js_api.hpp"
#include "duktape.hpp"
#include "../menu/menu.hpp"
#include "../renderer/d3d9.hpp"
#include "../menu/options.hpp"

std::mutex duktape_mutex;
time_t last_script_options_save_time = 0;
wchar_t appdata [ MAX_PATH ];

struct font_data_t {
	std::string family;
	int size;
	bool bold;
	ID3DXFont* font;
};

std::map < std::string, font_data_t > cached_fonts = {

};

std::map < std::wstring, duk_context* > script_ctx = {

};

enum control_type_t {
	control_button = 0,
	control_checkbox,
	control_slider_int,
	control_slider_float,
	control_colorpicker,
	control_combobox,
	control_multiselect,
	control_keybind,
	control_textbox
};

struct option_data_t {
	control_type_t control_type;
	options::option* option_ptr;
	float min, max;
	std::vector< sesui::ses_string > names;
};

/* [option id, option name], option */
std::map < std::pair< std::string, std::string >, option_data_t > name_to_option = {

};

void js::destroy_fonts( ) {
	for ( auto& font : cached_fonts )
		font.second.font->Release( );
}

void js::reset_fonts( ) {
	for ( auto& font : cached_fonts )
		LI_FN( D3DXCreateFontA )( csgo::i::dev, font.second.size, 0, font.second.bold ? FW_BOLD : FW_NORMAL, 0, false, OEM_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font.second.family.c_str( ), &font.second.font );
}

template < typename ...args_t >
void log_console( const sesui::color& clr, const char* fmt, args_t ...args ) {
	if ( !fmt )
		return;

	struct {
		uint8_t r, g, b, a;
	} s_clr;

	s_clr = { static_cast < uint8_t > ( clr.r ), static_cast < uint8_t > ( clr.g ), static_cast < uint8_t > ( clr.b ), static_cast < uint8_t > ( clr.a ) };

	static auto con_color_msg = reinterpret_cast< void ( * )( const decltype( s_clr )&, const char*, ... ) >( LI_FN( GetProcAddress ) ( LI_FN( GetModuleHandleA ) ( _( "tier0.dll" ) ), _( "?ConColorMsg@@YAXABVColor@@PBDZZ" ) ) );

	con_color_msg( s_clr, fmt, args... );
}

void js::process_render_callbacks( ) {
	std::lock_guard< std::mutex > guard( duktape_mutex );

	for ( auto& script : script_ctx ) {
		const auto has_callback = duk_get_global_string( script.second, _( "_render_cb" ) );

		duk_get_global_string( script.second, _( "_script_id" ) );
		const auto enabled = options::script_vars [ duk_require_string( script.second, -1 ) ].val.b;
		duk_pop( script.second );

		if ( enabled && has_callback && duk_pcall( script.second, 0 ) ) {
			char u8_script_name [ 128 ] { '\0' };

			// file deepcode ignore InternationalStringConversion: <please specify a reason of ignoring this>
   if ( WideCharToMultiByte( CP_UTF8, 0, script.first.c_str( ), -1, u8_script_name, 128, nullptr, nullptr ) <= 0 )
				continue;

			log_console( { 0.85f, 0.31f, 0.83f, 1.0f }, _( "[ sesame ] " ) );
			log_console( { 1.0f, 0.2f, 0.2f, 1.0f }, ( char* )_( u8"Error running \"%s\" script render callback: %s\n" ), u8_script_name, duk_safe_to_string( script.second, -1 ) );
		}

		//if ( has_callback )
		duk_pop( script.second );
	}
}

void js::process_net_update_callbacks( ) {
	std::lock_guard< std::mutex > guard( duktape_mutex );

	for ( auto& script : script_ctx ) {
		const auto has_callback = duk_get_global_string( script.second, _( "_net_update_cb" ) );

		duk_get_global_string( script.second, _( "_script_id" ) );
		const auto enabled = options::script_vars [ duk_require_string( script.second, -1 ) ].val.b;
		duk_pop( script.second );

		if ( enabled && has_callback && duk_pcall( script.second, 0 ) ) {
			char u8_script_name [ 128 ] { '\0' };

			if ( WideCharToMultiByte( CP_UTF8, 0, script.first.c_str( ), -1, u8_script_name, 128, nullptr, nullptr ) <= 0 )
				continue;

			log_console( { 0.85f, 0.31f, 0.83f, 1.0f }, _( "[ sesame ] " ) );
			log_console( { 1.0f, 0.2f, 0.2f, 1.0f }, ( char* )_( u8"Error running \"%s\" script network update callback: %s\n" ), u8_script_name, duk_safe_to_string( script.second, -1 ) );
		}

		//if ( has_callback )
		duk_pop( script.second );
	}
}

void js::process_net_update_end_callbacks( ) {
	std::lock_guard< std::mutex > guard( duktape_mutex );

	for ( auto& script : script_ctx ) {
		const auto has_callback = duk_get_global_string( script.second, _( "_net_update_end_cb" ) );

		duk_get_global_string( script.second, _( "_script_id" ) );
		const auto enabled = options::script_vars [ duk_require_string( script.second, -1 ) ].val.b;
		duk_pop( script.second );

		if ( enabled && has_callback && duk_pcall( script.second, 0 ) ) {
			char u8_script_name [ 128 ] { '\0' };

			if ( WideCharToMultiByte( CP_UTF8, 0, script.first.c_str( ), -1, u8_script_name, 128, nullptr, nullptr ) <= 0 )
				continue;

			log_console( { 0.85f, 0.31f, 0.83f, 1.0f }, _( "[ sesame ] " ) );
			log_console( { 1.0f, 0.2f, 0.2f, 1.0f }, ( char* )_( u8"Error running \"%s\" script network update end callback: %s\n" ), u8_script_name, duk_safe_to_string( script.second, -1 ) );
		}

		//if ( has_callback )
		duk_pop( script.second );
	}
}

void js::process_create_move_callbacks( ) {
	std::lock_guard< std::mutex > guard( duktape_mutex );

	for ( auto& script : script_ctx ) {
		const auto has_callback = duk_get_global_string( script.second, _( "_create_move_cb" ) );

		duk_get_global_string( script.second, _( "_script_id" ) );
		const auto enabled = options::script_vars [ duk_require_string( script.second, -1 ) ].val.b;
		duk_pop( script.second );

		if ( enabled && has_callback && duk_pcall( script.second, 0 ) ) {
			char u8_script_name [ 128 ] { '\0' };

			if ( WideCharToMultiByte( CP_UTF8, 0, script.first.c_str( ), -1, u8_script_name, 128, nullptr, nullptr ) <= 0 )
				continue;

			log_console( { 0.85f, 0.31f, 0.83f, 1.0f }, _( "[ sesame ] " ) );
			log_console( { 1.0f, 0.2f, 0.2f, 1.0f }, ( char* )_( u8"Error running \"%s\" script create move callback: %s\n" ), u8_script_name, duk_safe_to_string( script.second, -1 ) );
		}

		//if ( has_callback )
		duk_pop( script.second );
	}
}

std::wstring utf8_to_utf16( const std::string& utf8 ) {
	std::vector<unsigned long> unicode;
	size_t i = 0;
	while ( i < utf8.size( ) ) {
		unsigned long uni;
		size_t todo;
		bool error = false;
		unsigned char ch = utf8 [ i++ ];
		if ( ch <= 0x7F ) {
			uni = ch;
			todo = 0;
		}
		else if ( ch <= 0xBF ) {
			throw std::logic_error( _( "not a UTF-8 string" ) );
		}
		else if ( ch <= 0xDF ) {
			uni = ch & 0x1F;
			todo = 1;
		}
		else if ( ch <= 0xEF ) {
			uni = ch & 0x0F;
			todo = 2;
		}
		else if ( ch <= 0xF7 ) {
			uni = ch & 0x07;
			todo = 3;
		}
		else {
			throw std::logic_error( _( "not a UTF-8 string" ) );
		}
		for ( size_t j = 0; j < todo; ++j ) {
			if ( i == utf8.size( ) )
				throw std::logic_error( _( "not a UTF-8 string" ) );
			unsigned char ch = utf8 [ i++ ];
			if ( ch < 0x80 || ch > 0xBF )
				throw std::logic_error( _( "not a UTF-8 string" ) );
			uni <<= 6;
			uni += ch & 0x3F;
		}
		if ( uni >= 0xD800 && uni <= 0xDFFF )
			throw std::logic_error( _( "not a UTF-8 string" ) );
		if ( uni > 0x10FFFF )
			throw std::logic_error( _( "not a UTF-8 string" ) );
		unicode.push_back( uni );
	}
	std::wstring utf16;
	for ( size_t i = 0; i < unicode.size( ); ++i ) {
		unsigned long uni = unicode [ i ];
		if ( uni <= 0xFFFF ) {
			utf16 += ( wchar_t )uni;
		}
		else {
			uni -= 0x10000;
			utf16 += ( wchar_t )( ( uni >> 10 ) + 0xD800 );
			utf16 += ( wchar_t )( ( uni & 0x3FF ) + 0xDC00 );
		}
	}
	return utf16;
}

namespace bindings {
	namespace console {
		static duk_ret_t log( duk_context* ctx ) {
			duk_push_string( ctx, " " );
			duk_insert( ctx, 0 );
			duk_join( ctx, duk_get_top( ctx ) - 1 );
			log_console( { 0.85f, 0.31f, 0.83f, 1.0f }, _( "[ sesame ] " ) );
			log_console( { 1.0f, 1.0f, 1.0f, 1.0f }, _( "%s\n" ), duk_safe_to_string( ctx, -1 ) );
			return 0;
		}

		static duk_ret_t print( duk_context* ctx ) {
			duk_push_string( ctx, " " );
			duk_insert( ctx, 0 );
			duk_join( ctx, duk_get_top( ctx ) - 1 );
			log_console( { 1.0f, 1.0f, 1.0f, 1.0f }, _( "%s\n" ), duk_safe_to_string( ctx, -1 ) );
			return 0;
		}

		static duk_ret_t error( duk_context* ctx ) {
			duk_push_string( ctx, " " );
			duk_insert( ctx, 0 );
			duk_join( ctx, duk_get_top( ctx ) - 1 );
			log_console( { 0.85f, 0.31f, 0.83f, 1.0f }, _( "[ sesame ] " ) );
			log_console( { 1.0f, 0.3f, 0.3f, 1.0f }, _( "%s\n" ), duk_safe_to_string( ctx, -1 ) );
			return 0;
		}

		static duk_ret_t warning( duk_context* ctx ) {
			duk_push_string( ctx, " " );
			duk_insert( ctx, 0 );
			duk_join( ctx, duk_get_top( ctx ) - 1 );
			log_console( { 0.85f, 0.31f, 0.83f, 1.0f }, _( "[ sesame ] " ) );
			log_console( { 1.0f, 1.0f, 0.35f, 1.0f }, _( "%s\n" ), duk_safe_to_string( ctx, -1 ) );
			return 0;
		}
	}

	namespace globals {
		static duk_ret_t realtime( duk_context* ctx ) {
			duk_push_number( ctx, csgo::i::globals->m_realtime );
			return 1;
		}

		static duk_ret_t curtime( duk_context* ctx ) {
			duk_push_number( ctx, csgo::i::globals->m_curtime );
			return 1;
		}

		static duk_ret_t frametime( duk_context* ctx ) {
			duk_push_number( ctx, csgo::i::globals->m_frametime );
			return 1;
		}

		static duk_ret_t absframetime( duk_context* ctx ) {
			duk_push_number( ctx, csgo::i::globals->m_abs_frametime );
			return 1;
		}

		static duk_ret_t max_players( duk_context* ctx ) {
			duk_push_int( ctx, csgo::i::globals->m_max_clients );
			return 1;
		}

		static duk_ret_t tickcount( duk_context* ctx ) {
			duk_push_int( ctx, csgo::i::globals->m_tickcount );
			return 1;
		}

		static duk_ret_t interval_per_tick( duk_context* ctx ) {
			duk_push_number( ctx, csgo::i::globals->m_ipt );
			return 1;
		}

		static duk_ret_t framecount( duk_context* ctx ) {
			duk_push_int( ctx, csgo::i::globals->m_framecount );
			return 1;
		}
	}

	namespace renderer {
		uint32_t duk_require_color( duk_context* ctx, int arg ) {
			const auto success1 = duk_get_prop_string( ctx, arg, _( "r" ) );
			const auto success2 = duk_get_prop_string( ctx, arg, _( "g" ) );
			const auto success3 = duk_get_prop_string( ctx, arg, _( "b" ) );
			const auto success4 = duk_get_prop_string( ctx, arg, _( "a" ) );

			uint32_t clr = 0;

			if ( success1 && success2 && success3 && success4 )
				clr = D3DCOLOR_RGBA( duk_require_int( ctx, -4 ), duk_require_int( ctx, -3 ), duk_require_int( ctx, -2 ), duk_require_int( ctx, -1 ) );

			if ( success1 )
				duk_pop( ctx );
			if ( success2 )
				duk_pop( ctx );
			if ( success3 )
				duk_pop( ctx );
			if ( success4 )
				duk_pop( ctx );

			return clr;
		}

		static duk_ret_t rect( duk_context* ctx ) {
			if ( duk_require_boolean( ctx, 5 ) )
				::render::rectangle( duk_require_int( ctx, 0 ), duk_require_int( ctx, 1 ), duk_require_int( ctx, 2 ), duk_require_int( ctx, 3 ), duk_require_color( ctx, 4 ) );
			else
				::render::outline( duk_require_int( ctx, 0 ), duk_require_int( ctx, 1 ), duk_require_int( ctx, 2 ), duk_require_int( ctx, 3 ), duk_require_color( ctx, 4 ) );

			return 0;
		}

		static duk_ret_t rounded_rect( duk_context* ctx ) {
			::render::rounded_rect( duk_require_int( ctx, 0 ), duk_require_int( ctx, 1 ), duk_require_int( ctx, 2 ), duk_require_int( ctx, 3 ), duk_require_int( ctx, 4 ), duk_require_int( ctx, 4 ), duk_require_color( ctx, 5 ), !duk_require_boolean( ctx, 6 ) );

			return 0;
		}

		static duk_ret_t line( duk_context* ctx ) {
			::render::line( duk_require_int( ctx, 0 ), duk_require_int( ctx, 1 ), duk_require_int( ctx, 2 ), duk_require_int( ctx, 3 ), duk_require_color( ctx, 4 ) );

			return 0;
		}

		static duk_ret_t circle( duk_context* ctx ) {
			::render::circle( duk_require_int( ctx, 0 ), duk_require_int( ctx, 1 ), duk_require_int( ctx, 2 ), duk_require_int( ctx, 2 ) * 2, duk_require_color( ctx, 3 ), 0, 0.0f, !duk_require_boolean( ctx, 4 ) );

			return 0;
		}

		static duk_ret_t add_font( duk_context* ctx ) {
			font_data_t font_data;
			font_data.family = duk_require_string( ctx, 0 );
			font_data.size = duk_require_int( ctx, 1 );
			font_data.bold = duk_require_boolean( ctx, 2 );

			std::string font_id;

			if ( duk_get_global_string( ctx, _( "_script_id" ) ) ) {
				font_id = std::string( duk_require_string( ctx, -1 ) ) + _( "_" ) + font_data.family + _( "_" ) + std::to_string( font_data.size ) + _( "_" ) + std::to_string( static_cast< int > ( font_data.bold ) );
				duk_pop( ctx );
			}

			ID3DXFont* font_out = nullptr;
			LI_FN( D3DXCreateFontA )( csgo::i::dev, font_data.size, 0, font_data.bold ? FW_BOLD : FW_NORMAL, 0, false, OEM_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font_data.family.c_str( ), &font_data.font );

			if ( !font_id.empty( ) )
				cached_fonts [ font_id ] = font_data;

			/* add font returns font id */
			duk_push_string( ctx, font_id.empty( ) ? _( "default" ) : font_id.c_str( ) );

			return 1;
		}

		static duk_ret_t text( duk_context* ctx ) {
			const auto font = cached_fonts.find( duk_require_string( ctx, 4 ) );

			/* couldn't find target font */
			if ( font == cached_fonts.end( ) )
				return 0;

			::render::text( duk_require_int( ctx, 0 ), duk_require_int( ctx, 1 ), duk_require_color( ctx, 3 ), font->second.font, utf8_to_utf16( duk_require_string( ctx, 2 ) ) );

			return 0;
		}

		static duk_ret_t text_shadow( duk_context* ctx ) {
			const auto font = cached_fonts.find( duk_require_string( ctx, 4 ) );

			/* couldn't find target font */
			if ( font == cached_fonts.end( ) )
				return 0;

			::render::text( duk_require_int( ctx, 0 ), duk_require_int( ctx, 1 ), duk_require_color( ctx, 3 ), font->second.font, utf8_to_utf16( duk_require_string( ctx, 2 ) ), true );

			return 0;
		}

		static duk_ret_t text_outline( duk_context* ctx ) {
			const auto font = cached_fonts.find( duk_require_string( ctx, 4 ) );

			/* couldn't find target font */
			if ( font == cached_fonts.end( ) )
				return 0;

			::render::text( duk_require_int( ctx, 0 ), duk_require_int( ctx, 1 ), duk_require_color( ctx, 3 ), font->second.font, utf8_to_utf16( duk_require_string( ctx, 2 ) ), false, true );

			return 0;
		}
	}

	namespace types {
		/* color object */
		namespace color {
			static duk_ret_t constructor( duk_context* ctx ) {
				if ( !duk_is_constructor_call( ctx ) ) {
					return DUK_RET_TYPE_ERROR;
				}

				duk_push_this( ctx );

				duk_dup( ctx, 0 );
				duk_put_prop_string( ctx, -2, _( "r" ) );
				duk_dup( ctx, 1 );
				duk_put_prop_string( ctx, -2, _( "g" ) );
				duk_dup( ctx, 2 );
				duk_put_prop_string( ctx, -2, _( "b" ) );
				duk_dup( ctx, 3 );
				duk_put_prop_string( ctx, -2, _( "a" ) );

				return 0;
			}

			static void init( duk_context* ctx ) {
				duk_push_c_function( ctx, constructor, 4 /* nargs */ );
				duk_put_global_string( ctx, _( "color" ) );
			}
		}

		namespace ui_element {
			static duk_ret_t constructor( duk_context* ctx ) {
				if ( !duk_is_constructor_call( ctx ) ) {
					return DUK_RET_TYPE_ERROR;
				}

				duk_push_this( ctx );

				duk_dup( ctx, 0 );
				duk_put_prop_string( ctx, -2, _( "ptr" ) );
				duk_dup( ctx, 1 );
				duk_put_prop_string( ctx, -2, _( "type" ) );

				return 0;
			}

			static duk_ret_t get( duk_context* ctx ) {
				duk_push_this( ctx );

				duk_get_prop_string( ctx, -1, _( "ptr" ) );
				const auto element_ptr = reinterpret_cast< options::option* >( duk_require_pointer( ctx, -1 ) );
				duk_pop( ctx );

				if ( !element_ptr ) {
					duk_push_int( ctx, 0 );
					return 1;
				}

				duk_get_prop_string( ctx, -1, _( "type" ) );
				const auto element_type = duk_require_string( ctx, -1 );
				duk_pop( ctx );

				if ( !strcmp( element_type, _( "bool" ) ) && element_ptr->type == options::option_type_t::boolean ) {
					duk_push_boolean( ctx, element_ptr->val.b );
				}
				else if ( !strcmp( element_type, _( "list" ) ) && element_ptr->type == options::option_type_t::list ) {
					duk_push_array( ctx );

					for ( auto i = 0; i < element_ptr->list_size; i++ ) {
						duk_push_boolean( ctx, element_ptr->val.l [ i ] );
						duk_put_prop_index( ctx, -2, i );
					}
				}
				else if ( !strcmp( element_type, _( "string" ) ) && element_ptr->type == options::option_type_t::string ) {
					duk_push_string( ctx, ( char* )element_ptr->val.s );
				}
				else if ( !strcmp( element_type, _( "color" ) ) && element_ptr->type == options::option_type_t::color ) {
					duk_get_global_string( ctx, _( "color" ) );
					duk_push_int( ctx, element_ptr->val.c.r * 255.0f );
					duk_push_int( ctx, element_ptr->val.c.g * 255.0f );
					duk_push_int( ctx, element_ptr->val.c.b * 255.0f );
					duk_push_int( ctx, element_ptr->val.c.a * 255.0f );
					duk_new( ctx, 4 );
				}
				else if ( !strcmp( element_type, _( "int" ) ) && element_ptr->type == options::option_type_t::integer ) {
					duk_push_int( ctx, element_ptr->val.i );
				}
				else if ( !strcmp( element_type, _( "float" ) ) && element_ptr->type == options::option_type_t::floating_point ) {
					duk_push_number( ctx, element_ptr->val.f );
				}
				else {
					duk_push_int( ctx, 0 );
				}

				return 1;
			}

			static duk_ret_t set( duk_context* ctx ) {
				/* make sure we are only passing in 1 value (we can only set 1 value at a time) */
				if ( duk_get_top( ctx ) != 1 )
					return 0;

				duk_push_this( ctx );

				duk_get_prop_string( ctx, -1, _( "ptr" ) );
				const auto element_ptr = reinterpret_cast< options::option* >( duk_require_pointer( ctx, -1 ) );
				duk_pop( ctx );

				if ( !element_ptr )
					return 0;

				duk_get_prop_string( ctx, -1, _( "type" ) );
				const auto element_type = duk_require_string( ctx, -1 );
				duk_pop( ctx );

				if ( !strcmp( element_type, _( "bool" ) ) && element_ptr->type == options::option_type_t::boolean ) {
					element_ptr->val.b = duk_require_boolean( ctx, 0 );
				}
				else if ( !strcmp( element_type, _( "list" ) ) && element_ptr->type == options::option_type_t::list ) {
					if ( !duk_is_array( ctx, 0 ) )
						return 0;

					const auto len = duk_get_length( ctx, 0 );

					for ( auto i = 0; i < len && i < element_ptr->list_size; i++ ) {
						duk_get_prop_index( ctx, 0, i );
						element_ptr->val.l [ i ] = duk_require_boolean( ctx, -1 );
						duk_pop( ctx );
					}
				}
				else if ( !strcmp( element_type, _( "string" ) ) && element_ptr->type == options::option_type_t::string ) {
					wcscpy_s( element_ptr->val.s, ( wchar_t* )duk_require_string( ctx, 0 ) );
				}
				else if ( !strcmp( element_type, _( "color" ) ) && element_ptr->type == options::option_type_t::color ) {
					const auto clr = renderer::duk_require_color( ctx, 0 );

					element_ptr->val.c.r = ( clr >> 16 ) & 0xff;
					element_ptr->val.c.g = ( clr >> 8 ) & 0xff;
					element_ptr->val.c.b = ( clr >> 0 ) & 0xff;
					element_ptr->val.c.a = ( clr >> 24 ) & 0xff;
				}
				else if ( !strcmp( element_type, _( "int" ) ) && element_ptr->type == options::option_type_t::integer ) {
					element_ptr->val.i = duk_require_int( ctx, 0 );
				}
				else if ( !strcmp( element_type, _( "float" ) ) && element_ptr->type == options::option_type_t::floating_point ) {
					element_ptr->val.f = duk_require_number( ctx, 0 );
				}

				return 0;
			}

			static void init( duk_context* ctx ) {
				duk_push_c_function( ctx, constructor, 2 /* nargs */ );
				duk_push_object( ctx );
				duk_push_c_function( ctx, get, 0 /*nargs*/ );
				duk_put_prop_string( ctx, -2, _( "get" ) );
				duk_push_c_function( ctx, set, 1 /*nargs*/ );
				duk_put_prop_string( ctx, -2, _( "set" ) );
				duk_put_prop_string( ctx, -2, _( "prototype" ) );
				duk_put_global_string( ctx, _( "ui_element" ) );
			}
		}
	}

	namespace sesame {
		static duk_ret_t on_render( duk_context* ctx ) {
			duk_dup( ctx, 0 );
			duk_put_global_string( ctx, _( "_render_cb" ) );

			return 0;
		}

		static duk_ret_t on_net_update( duk_context* ctx ) {
			duk_dup( ctx, 0 );
			duk_put_global_string( ctx, _( "_net_update_cb" ) );

			return 0;
		}

		static duk_ret_t on_net_update_end( duk_context* ctx ) {
			duk_dup( ctx, 0 );
			duk_put_global_string( ctx, _( "_net_update_end_cb" ) );

			return 0;
		}

		static duk_ret_t on_create_move( duk_context* ctx ) {
			duk_dup( ctx, 0 );
			duk_put_global_string( ctx, _( "_create_move_cb" ) );

			return 0;
		}

		namespace ui {
			static duk_ret_t get_element( duk_context* ctx ) {
				duk_get_global_string( ctx, _( "_script_id" ) );
				const auto script_name_ptr = duk_require_string( ctx, -1 );
				duk_pop( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_name_ptr ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto entry = options::vars.find( script_name_ptr );

				if ( entry == options::vars.end( ) ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				duk_get_global_string( ctx, _( "ui_element" ) );
				duk_push_pointer( ctx, &entry->second );

				switch ( entry->second.type ) {
					case options::option_type_t::boolean: duk_push_string( ctx, _( "bool" ) ); break;
					case options::option_type_t::list: duk_push_string( ctx, _( "list" ) ); break;
					case options::option_type_t::integer: duk_push_string( ctx, _( "int" ) ); break;
					case options::option_type_t::floating_point: duk_push_string( ctx, _( "float" ) ); break;
					case options::option_type_t::string: duk_push_string( ctx, _( "string" ) ); break;
					case options::option_type_t::color: duk_push_string( ctx, _( "color" ) ); break;
				}

				duk_new( ctx, 2 );

				return 1;
			}

			static duk_ret_t add_checkbox( duk_context* ctx ) {
				duk_get_global_string( ctx, _( "_script_id" ) );
				auto script_entry = duk_require_string( ctx, -1 );
				duk_pop( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_entry ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto entry = options::script_vars.find( script_entry );

				if ( entry == options::script_vars.end( ) ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto item_id = std::string( script_entry ) + _( "." ) + duk_require_string( ctx, 0 );

				options::option::add_script_bool( item_id, duk_require_boolean( ctx, 2 ) );

				const auto option_ptr = &options::script_vars [ item_id ];

				/* [option id, option name], option */
				name_to_option [ {item_id, duk_require_string( ctx, 1 )} ] = { control_checkbox, option_ptr, 0.0f, 0.0f, {} };

				duk_get_global_string( ctx, _( "ui_element" ) );
				duk_push_pointer( ctx, option_ptr );
				duk_push_string( ctx, _( "bool" ) );
				duk_new( ctx, 2 );

				return 1;
			}

			static duk_ret_t add_slider_int( duk_context* ctx ) {
				duk_get_global_string( ctx, _( "_script_id" ) );
				auto script_entry = duk_require_string( ctx, -1 );
				duk_pop( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_entry ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto entry = options::script_vars.find( script_entry );

				if ( entry == options::script_vars.end( ) ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto item_id = std::string( script_entry ) + _( "." ) + duk_require_string( ctx, 0 );

				options::option::add_script_int( item_id, duk_require_int( ctx, 2 ) );

				const auto option_ptr = &options::script_vars [ item_id ];

				/* [option id, option name], option */
				name_to_option [ {item_id, duk_require_string( ctx, 1 )} ] = { control_slider_int, option_ptr, static_cast< float >( duk_require_int( ctx, 3 ) ), static_cast< float >( duk_require_int( ctx, 4 ) ) , {} };

				duk_get_global_string( ctx, _( "ui_element" ) );
				duk_push_pointer( ctx, option_ptr );
				duk_push_string( ctx, _( "int" ) );
				duk_new( ctx, 2 );

				return 1;
			}

			static duk_ret_t add_slider_float( duk_context* ctx ) {
				duk_get_global_string( ctx, _( "_script_id" ) );
				auto script_entry = duk_require_string( ctx, -1 );
				duk_pop( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_entry ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto entry = options::script_vars.find( script_entry );

				if ( entry == options::script_vars.end( ) ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto item_id = std::string( script_entry ) + _( "." ) + duk_require_string( ctx, 0 );

				options::option::add_script_float( item_id, duk_require_number( ctx, 2 ) );

				const auto option_ptr = &options::script_vars [ item_id ];

				/* [option id, option name], option */
				name_to_option [ {item_id, duk_require_string( ctx, 1 )} ] = { control_slider_float, option_ptr,static_cast< float >( duk_require_number( ctx, 3 ) ),static_cast< float >( duk_require_number( ctx, 4 ) ), {} };

				duk_get_global_string( ctx, _( "ui_element" ) );
				duk_push_pointer( ctx, option_ptr );
				duk_push_string( ctx, _( "float" ) );
				duk_new( ctx, 2 );

				return 1;
			}

			static duk_ret_t add_combobox( duk_context* ctx ) {
				duk_get_global_string( ctx, _( "_script_id" ) );
				auto script_entry = duk_require_string( ctx, -1 );
				duk_pop( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_entry ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto entry = options::script_vars.find( script_entry );

				if ( entry == options::script_vars.end( ) ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto item_id = std::string( script_entry ) + _( "." ) + duk_require_string( ctx, 0 );

				options::option::add_script_int( item_id, duk_require_int( ctx, 2 ) );

				const auto option_ptr = &options::script_vars [ item_id ];

				std::vector< sesui::ses_string > items { };

				if ( !duk_is_array( ctx, 3 ) )
					return 0;

				const auto len = duk_get_length( ctx, 3 );

				for ( auto i = 0; i < len; i++ ) {
					duk_get_prop_index( ctx, 3, i );
					items.push_back( utf8_to_utf16( duk_require_string( ctx, -1 ) ).data( ) );
					duk_pop( ctx );
				}

				/* [option id, option name], option */
				name_to_option [ {item_id, duk_require_string( ctx, 1 )} ] = { control_combobox, option_ptr, 0.0f, 0.0f , items };

				duk_get_global_string( ctx, _( "ui_element" ) );
				duk_push_pointer( ctx, option_ptr );
				duk_push_string( ctx, _( "int" ) );
				duk_new( ctx, 2 );

				return 1;
			}

			static duk_ret_t add_multibox( duk_context* ctx ) {
				duk_get_global_string( ctx, _( "_script_id" ) );
				auto script_entry = duk_require_string( ctx, -1 );
				duk_pop( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_entry ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto entry = options::script_vars.find( script_entry );

				if ( entry == options::script_vars.end( ) ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				if ( !duk_is_array( ctx, 2 ) )
					return 0;

				const auto len = duk_get_length( ctx, 2 );

				const auto item_id = std::string( script_entry ) + _( "." ) + duk_require_string( ctx, 0 );

				options::option::add_script_list( item_id, len );

				const auto option_ptr = &options::script_vars [ item_id ];

				std::vector< sesui::ses_string > items { };

				for ( auto i = 0; i < len; i++ ) {
					duk_get_prop_index( ctx, 2, i );
					items.push_back( utf8_to_utf16( duk_require_string( ctx, -1 ) ).data( ) );
					duk_pop( ctx );
				}

				/* [option id, option name], option */
				name_to_option [ {item_id, duk_require_string( ctx, 1 )} ] = { control_multiselect, option_ptr, 0.0f, 0.0f , items };

				duk_get_global_string( ctx, _( "ui_element" ) );
				duk_push_pointer( ctx, option_ptr );
				duk_push_string( ctx, _( "int" ) );
				duk_new( ctx, 2 );

				return 1;
			}

			static duk_ret_t add_keybind( duk_context* ctx ) {
				duk_get_global_string( ctx, _( "_script_id" ) );
				auto script_entry = duk_require_string( ctx, -1 );
				duk_pop( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_entry ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto entry = options::script_vars.find( script_entry );

				if ( entry == options::script_vars.end( ) ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto item_id = std::string( script_entry ) + _( "." ) + duk_require_string( ctx, 0 );

				options::option::add_script_int( item_id + _( "_key" ), duk_require_int( ctx, 2 ) );
				options::option::add_script_int( item_id + _( "_key_mode" ), duk_require_int( ctx, 3 ) );

				const auto option_ptr = &options::script_vars [ item_id ];

				/* [option id, option name], option */
				name_to_option [ {item_id, duk_require_string( ctx, 1 )} ] = { control_keybind, option_ptr, 0.0f, 0.0f , {} };

				duk_get_global_string( ctx, _( "ui_element" ) );
				duk_push_pointer( ctx, option_ptr );
				duk_push_string( ctx, _( "int" ) );
				duk_new( ctx, 2 );

				return 1;
			}

			static duk_ret_t add_colorpicker( duk_context* ctx ) {
				duk_get_global_string( ctx, _( "_script_id" ) );
				auto script_entry = duk_require_string( ctx, -1 );
				duk_pop( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_entry ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto entry = options::script_vars.find( script_entry );

				if ( entry == options::script_vars.end( ) ) {
					duk_get_global_string( ctx, _( "ui_element" ) );
					duk_push_pointer( ctx, nullptr );
					duk_push_string( ctx, _( "" ) );
					duk_new( ctx, 2 );
					return 1;
				}

				const auto item_id = std::string( script_entry ) + _( "." ) + duk_require_string( ctx, 0 );
				const auto clr = renderer::duk_require_color( ctx, 2 );

				const auto cr = static_cast< float >( ( clr >> 16 ) & 0xff ) / 255.0f;
				const auto cg = static_cast< float >( ( clr >> 8 ) & 0xff ) / 255.0f;
				const auto cb = static_cast< float >( ( clr >> 0 ) & 0xff ) / 255.0f;
				const auto ca = static_cast< float >( ( clr >> 24 ) & 0xff ) / 255.0f;

				options::option::add_script_color( item_id, { cr, cg, cb, ca } );

				const auto option_ptr = &options::script_vars [ item_id ];

				/* [option id, option name], option */
				name_to_option [ {item_id, duk_require_string( ctx, 1 )} ] = { control_colorpicker, option_ptr, 0.0f, 0.0f , {} };

				duk_get_global_string( ctx, _( "ui_element" ) );
				duk_push_pointer( ctx, option_ptr );
				duk_push_string( ctx, _( "int" ) );
				duk_new( ctx, 2 );

				return 1;
			}
		}
	}

	char* str_code = nullptr;

	static duk_ret_t unsafe_code( duk_context* ctx, void* udata ) {
		duk_eval_string( ctx, str_code );
		return 0;
	}
}

#define create_object( name ) \
	const auto name##_idx = duk_push_object ( ctx ); 

#define end_object( name ) \
	duk_put_global_string ( ctx, _( #name ) );

#define end_subobject( name, parent ) \
	duk_put_prop_string ( ctx, parent##_idx, _( #name ) );

#define create_method( object, func, name, args ) \
	duk_push_c_function ( ctx, func, args ); \
	duk_put_prop_string ( ctx, object##_idx, _( name ) );

#define create_number_constant( object, name, val ) \
	duk_push_number( ctx, val ); \
	duk_put_prop_string( ctx, object##_idx, _( name ) );

#define create_integer_constant( object, name, val ) \
	duk_push_int( ctx, val ); \
	duk_put_prop_string( ctx, object##_idx, _( name ) );

#define create_boolean_constant( object, name, val ) \
	duk_push_boolean( ctx, val ); \
	duk_put_prop_string( ctx, object##_idx, _( name ) );

#define create_func( func, name, args ) \
	duk_push_c_function ( ctx, func, args ); \
	duk_put_global_string ( ctx, _( name ) );

duk_context* create_ctx( ) {
	const auto ctx = duk_create_heap_default( );

	/* console class */
	create_object( console ); {
		/* console.log(args) */
		create_method( console, bindings::console::log, "log", DUK_VARARGS );
		/* console.print(args) */
		create_method( console, bindings::console::print, "print", DUK_VARARGS );
		/* console.error(args) */
		create_method( console, bindings::console::error, "error", DUK_VARARGS );
		/* console.warning(args) */
		create_method( console, bindings::console::warning, "warning", DUK_VARARGS );

		end_object( console );
	}

	/* sesame class */
	create_object( sesame ); {
		/* sesame.on_render(fn) */
		create_method( sesame, bindings::sesame::on_render, "on_render", 1 );
		/* sesame.on_net_update(fn) */
		create_method( sesame, bindings::sesame::on_net_update, "on_net_update", 1 );
		/* sesame.on_net_update_end(fn) */
		create_method( sesame, bindings::sesame::on_net_update_end, "on_net_update_end", 1 );
		/* sesame.on_create_move(fn) */
		create_method( sesame, bindings::sesame::on_create_move, "on_create_move", 1 );

		/* sesame.ui class */
		create_object( ui ); {
			/* sesame.ui.add_checkbox(id, name, value) */
			create_method( ui, bindings::sesame::ui::add_checkbox, "add_checkbox", 3 );
			/* sesame.ui.add_slider_int(id, name, value, min, max) */
			create_method( ui, bindings::sesame::ui::add_slider_int, "add_slider_int", 5 );
			/* sesame.ui.add_slider_float(id, name, value, min, max) */
			create_method( ui, bindings::sesame::ui::add_slider_float, "add_slider_float", 5 );
			/* sesame.ui.add_combobox(id, name, value, arr_items) */
			create_method( ui, bindings::sesame::ui::add_combobox, "add_combobox", 4 );
			/* sesame.ui.add_multibox(id, name, arr_items) */
			create_method( ui, bindings::sesame::ui::add_multibox, "add_multibox", 3 );
			/* sesame.ui.add_keybind(id, name, key, mode) */
			create_method( ui, bindings::sesame::ui::add_keybind, "add_keybind", 4 );
			/* sesame.ui.add_colorpicker(id, name, clr) */
			create_method( ui, bindings::sesame::ui::add_colorpicker, "add_colorpicker", 3 );

			/* sesame.ui.get_element(id) */
			create_method( ui, bindings::sesame::ui::get_element, "get_element", 1 );

			end_subobject( ui, sesame );
		}

		end_object( sesame );
	}

	/* renderer class */
	create_object( renderer ); {
		/* renderer.rect(x, y, w, h, clr, filled) */
		create_method( renderer, bindings::renderer::rect, "rect", 6 );
		/* renderer.line(x1, y1, x2, y2, clr) */
		create_method( renderer, bindings::renderer::line, "line", 5 );
		/* renderer.rounded_rect(x, y, w, h, rad, clr, filled) */
		create_method( renderer, bindings::renderer::rounded_rect, "rounded_rect", 7 );
		/* renderer.circle(x, y, r, clr, filled) */
		create_method( renderer, bindings::renderer::circle, "circle", 5 );
		/* var font = renderer.add_font(family, size, bold) */
		create_method( renderer, bindings::renderer::add_font, "add_font", 3 );
		/* renderer.text(x, y, str, clr, font) */
		create_method( renderer, bindings::renderer::text, "text", 5 );
		/* renderer.text_shadow(x, y, str, clr, font) */
		create_method( renderer, bindings::renderer::text_shadow, "text_shadow", 5 );
		/* renderer.text_outline(x, y, str, clr, font) */
		create_method( renderer, bindings::renderer::text_outline, "text_outline", 5 );

		end_object( renderer );
	}

	/* globals class */
	create_object( globals ); {
		/* globals.realtime() */
		create_method( globals, bindings::globals::realtime, "realtime", 0 );
		/* globals.curtime() */
		create_method( globals, bindings::globals::curtime, "curtime", 0 );
		/* globals.frametime() */
		create_method( globals, bindings::globals::frametime, "frametime", 0 );
		/* globals.absframetime() */
		create_method( globals, bindings::globals::absframetime, "absframetime", 0 );
		/* globals.max_players() */
		create_method( globals, bindings::globals::max_players, "max_players", 0 );
		/* globals.tickcount() */
		create_method( globals, bindings::globals::tickcount, "tickcount", 0 );
		/* globals.interval_per_tick() */
		create_method( globals, bindings::globals::interval_per_tick, "interval_per_tick", 0 );
		/* globals.framecount() */
		create_method( globals, bindings::globals::framecount, "framecount", 0 );

		end_object( globals );
	}

	/* create new types */
	bindings::types::color::init( ctx );
	bindings::types::ui_element::init( ctx );

	return ctx;
}

void js::load( const std::wstring& script ) {
	char u8_script_name [ 128 ] { '\0' };

	if ( WideCharToMultiByte( CP_UTF8, 0, script.c_str( ), -1, u8_script_name, 128, nullptr, nullptr ) <= 0 )
		return;

	const auto script_entry = script_ctx.find( script );

	/* create script folder if it doesn't exist already */
	wchar_t appdata [ MAX_PATH ];

	if ( SUCCEEDED( LI_FN( SHGetFolderPathW )( nullptr, N( 5 ), nullptr, N( 0 ), appdata ) ) ) {
		LI_FN( CreateDirectoryW )( ( std::wstring( appdata ) + _( L"\\sesame" ) ).data( ), nullptr );
		LI_FN( CreateDirectoryW )( ( std::wstring( appdata ) + _( L"\\sesame\\configs" ) ).data( ), nullptr );
	}

	const auto path = std::wstring( appdata ) + _( L"\\sesame\\js\\" ) + script + _( L".js" );
	const auto file = _wfopen( path.data( ), _( L"rb" ) );

	/* create default options object if it doesnt exist already */
	options::option::add_script_bool( u8_script_name, false );

	/* parse file and compile script to increase execution speed */
	if ( file ) {
		fseek( file, 0, SEEK_END );
		const auto size = ftell( file );
		rewind( file );

		const auto buffer = ( char* )malloc( size );
		// deepcode ignore NullPtrPassFromMaybeNull: <please specify a reason of ignoring this>
  const auto len = fread( buffer, 1, size, file );
		const auto ctx = create_ctx( );

		/* add script id flag */
		duk_push_string( ctx, u8_script_name );
		duk_put_global_string( ctx, _( "_script_id" ) );

		bindings::str_code = buffer;

		if ( script_entry != script_ctx.end( ) ) {
			if ( script_ctx [ script ] )
				duk_destroy_heap( script_ctx [ script ] );

			script_ctx [ script ] = ctx;

			log_console( { 0.85f, 0.31f, 0.83f, 1.0f }, _( "[ sesame ] " ) );
			log_console( { 1.0f, 1.0f, 0.21f, 1.0f }, ( char* )_( u8"Script \"%s\" is already loaded. Reloading...\n" ), u8_script_name );
		}
		else {
			script_ctx.emplace( script, ctx );

			log_console( { 0.85f, 0.31f, 0.83f, 1.0f }, _( "[ sesame ] " ) );
			log_console( { 0.2f, 1.0f, 0.2f, 1.0f }, ( char* )_( u8"Loaded script \"%s\".\n" ), u8_script_name );
		}

		if ( duk_safe_call( ctx, bindings::unsafe_code, nullptr, 0, 1 ) ) {
			log_console( { 0.85f, 0.31f, 0.83f, 1.0f }, _( "[ sesame ] " ) );
			log_console( { 1.0f, 0.2f, 0.2f, 1.0f }, ( char* )_( u8"Error running \"%s\" script: %s\n" ), u8_script_name, duk_safe_to_string( ctx, -1 ) );
		}

		fclose( file );
		free( buffer );
	}
}

void js::init( ) {
	/* load global script options */
	//load_script_options ( );

	END_FUNC
}

void js::reset( ) {
	log_console( { 0.85f, 0.31f, 0.83f, 1.0f }, _( "[ sesame ] " ) );
	log_console( { 0.2f, 1.0f, 0.2f, 1.0f }, ( char* )_( u8"Reloading all scripts...\n" ) );

	for ( auto& script : script_ctx ) {
		if ( script.second )
			duk_destroy_heap( script.second );
	}

	script_ctx.clear( );
}