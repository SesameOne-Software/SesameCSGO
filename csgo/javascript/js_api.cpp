#include <sdk.hpp>
#include <oxui.hpp>
#include <shlobj.h>
#include <fstream>
#include <mutex>
#include "js_api.hpp"
#include "duktape.h"
#include "../menu/menu.hpp"
#include "../oxui/json/cJSON.h"
#include "../renderer/d3d9.hpp"

std::mutex duktape_mutex;
extern std::shared_ptr< oxui::group > option_list;
cJSON* script_options = nullptr;
oxui::str last_selected_script = OSTR ( "" );
time_t last_script_options_save_time = 0;
wchar_t appdata [ MAX_PATH ];

void create_menu_object ( ) {
	// add, remove menu entry (script_options)
}

void js::save_script_options ( ) {
	OPTION ( oxui::str, script_name, "Sesame->Customization->Scripts->Actions->Script Name", oxui::object_textbox );

	if ( !abs ( time ( nullptr ) - last_script_options_save_time ) )
		return;

	last_script_options_save_time = time ( nullptr );

	if ( !SUCCEEDED ( LI_FN ( SHGetFolderPathW )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) )
		return;

	std::ofstream ofile ( std::wstring ( appdata ) + _ ( L"\\sesame\\global\\script_options.json" ) );

	if ( ofile.is_open ( ) && script_options ) {
		// loop thru menu options on current script and save to config
		if ( !script_name.empty ( ) && script_name.length ( ) && !option_list->objects.empty( ) ) {
			char u8_script_name [ 128 ] { '\0' };

			if ( WideCharToMultiByte ( CP_UTF8, 0, script_name.c_str ( ), -1, u8_script_name, 128, nullptr, nullptr ) <= 0 )
				return;

			auto script_entry = cJSON_GetObjectItemCaseSensitive ( script_options, u8_script_name );

			/* add script entry if it doesn't already exist */
			if ( !script_entry || !cJSON_IsObject ( script_entry ) )
				return;

			auto script_enabled = cJSON_GetObjectItemCaseSensitive ( script_entry, _ ( "Enabled" ) );

			if ( !script_enabled || !cJSON_IsBool ( script_enabled ) )
				return;

			cJSON_ReplaceItemInObject ( script_entry, _ ( "Enabled" ), cJSON_CreateBool ( std::dynamic_pointer_cast< oxui::checkbox >( option_list->objects.front ( ) )->checked ) );

			for ( auto& cur_script_options : option_list->objects ) {
				switch ( static_cast < oxui::object_type > ( cur_script_options->type ) ) {
				case oxui::object_type::object_checkbox: {
					const auto as_checkbox = std::dynamic_pointer_cast< oxui::checkbox >( cur_script_options );
					const auto as_string = std::string ( as_checkbox->label.begin( ), as_checkbox->label.end( ) );

					const auto script_option = cJSON_GetObjectItemCaseSensitive ( script_entry, as_string.c_str( ) );

					if ( !script_option || !cJSON_IsArray ( script_option ) )
						break;

					const auto var_type = cJSON_GetArrayItem ( script_option, 0 );
					const auto var_name = cJSON_GetArrayItem ( script_option, 1 );
					const auto var_value = cJSON_GetArrayItem ( script_option, 2 );

					if ( !var_type || !var_name || !var_value )
						break;

					if ( var_type->valueint != as_checkbox->type )
						break;

					cJSON_SetIntValue ( var_type, as_checkbox->type );
					cJSON_SetValuestring ( var_name, as_string.c_str( ) );
					cJSON_ReplaceItemInArray ( script_option, 2, cJSON_CreateBool ( as_checkbox->checked ) );
					break;
				}
				case oxui::object_type::object_slider: {
					const auto as_slider = std::dynamic_pointer_cast< oxui::slider >( cur_script_options );
					const auto as_string = std::string ( as_slider->label.begin ( ), as_slider->label.end ( ) );

					const auto script_option = cJSON_GetObjectItemCaseSensitive ( script_entry, as_string.c_str ( ) );

					if ( !script_option || !cJSON_IsArray ( script_option ) )
						break;
						
					const auto var_type = cJSON_GetArrayItem ( script_option, 0 );
					const auto var_name = cJSON_GetArrayItem ( script_option, 1 );
					const auto var_value = cJSON_GetArrayItem ( script_option, 2 );

					if ( !var_type || !var_name || !var_value )
						break;

					const auto var_value_val = cJSON_GetArrayItem ( var_value, 0 );

					if ( var_type->valueint != as_slider->type )
						break;

					cJSON_SetIntValue ( var_type, as_slider->type );
					cJSON_SetValuestring ( var_name, as_string.c_str ( ) );
					cJSON_SetNumberValue ( var_value_val, as_slider->value );
					break;
				}
				case oxui::object_type::object_dropdown: {
					const auto as_dropdown = std::dynamic_pointer_cast< oxui::dropdown >( cur_script_options );
					const auto as_string = std::string ( as_dropdown->label.begin ( ), as_dropdown->label.end ( ) );

					const auto script_option = cJSON_GetObjectItemCaseSensitive ( script_entry, as_string.c_str ( ) );

					if ( !script_option || !cJSON_IsArray ( script_option ) )
						break;

					const auto var_type = cJSON_GetArrayItem ( script_option, 0 );
					const auto var_name = cJSON_GetArrayItem ( script_option, 1 );
					const auto var_value = cJSON_GetArrayItem ( script_option, 2 );

					if ( !var_type || !var_name || !var_value )
						break;

					const auto var_value_val = cJSON_GetArrayItem ( var_value, 0 );

					if ( var_type->valueint != as_dropdown->type )
						break;

					cJSON_SetIntValue ( var_type, as_dropdown->type );
					cJSON_SetValuestring ( var_name, as_string.c_str ( ) );
					cJSON_SetIntValue ( var_value_val, as_dropdown->value );
					break;
				}
				case oxui::object_type::object_keybind: {
					const auto as_keybind = std::dynamic_pointer_cast< oxui::keybind >( cur_script_options );
					const auto as_string = std::string ( as_keybind->label.begin ( ), as_keybind->label.end ( ) );

					const auto script_option = cJSON_GetObjectItemCaseSensitive ( script_entry, as_string.c_str ( ) );

					if ( !script_option || !cJSON_IsArray ( script_option ) )
						break;

					const auto var_type = cJSON_GetArrayItem ( script_option, 0 );
					const auto var_name = cJSON_GetArrayItem ( script_option, 1 );
					const auto var_value = cJSON_GetArrayItem ( script_option, 2 );

					if ( !var_type || !var_name || !var_value )
						break;

					if ( var_type->valueint != as_keybind->type )
						break;

					cJSON_SetIntValue ( var_type, as_keybind->type );
					cJSON_SetValuestring ( var_name, as_string.c_str ( ) );
					
					const auto value1 = cJSON_GetArrayItem ( var_value, 0 );
					const auto value2 = cJSON_GetArrayItem ( var_value, 1 );

					if ( !value1 || !value2 )
						break;

					cJSON_SetIntValue ( value1, as_keybind->key );
					cJSON_SetIntValue ( value2, static_cast<int>( as_keybind->mode ) );
					break;
				}
				case oxui::object_type::object_colorpicker: {
					const auto as_colorpicker = std::dynamic_pointer_cast< oxui::color_picker >( cur_script_options );
					const auto as_string = std::string ( as_colorpicker->label.begin ( ), as_colorpicker->label.end ( ) );

					const auto script_option = cJSON_GetObjectItemCaseSensitive ( script_entry, as_string.c_str ( ) );

					if ( !script_option || !cJSON_IsArray ( script_option ) )
						break;

					const auto var_type = cJSON_GetArrayItem ( script_option, 0 );
					const auto var_name = cJSON_GetArrayItem ( script_option, 1 );
					const auto var_value = cJSON_GetArrayItem ( script_option, 2 );

					if ( !var_type || !var_name || !var_value )
						break;

					if ( var_type->valueint != as_colorpicker->type )
						break;

					cJSON_SetIntValue ( var_type, as_colorpicker->type );
					cJSON_SetValuestring ( var_name, as_string.c_str ( ) );

					const auto value_r = cJSON_GetArrayItem ( var_value, 0 );
					const auto value_g = cJSON_GetArrayItem ( var_value, 1 );
					const auto value_b = cJSON_GetArrayItem ( var_value, 2 );
					const auto value_a = cJSON_GetArrayItem ( var_value, 3 );

					if ( !value_r || !value_g || !value_b || !value_a )
						break;

					cJSON_SetIntValue ( value_r, as_colorpicker->clr.r );
					cJSON_SetIntValue ( value_g, as_colorpicker->clr.g );
					cJSON_SetIntValue ( value_b, as_colorpicker->clr.b );
					cJSON_SetIntValue ( value_a, as_colorpicker->clr.a );
					break;
				}
				}
			}
		}

		const auto dump = cJSON_Print ( script_options );

		ofile.write ( dump, strlen ( dump ) );
		ofile.close ( );
	}
}

void js::load_script_options( ) {
	auto create_default_json_object = [ ] ( ) {
		script_options = cJSON_CreateObject ( );


	};

	wchar_t appdata [ MAX_PATH ];

	if ( !SUCCEEDED ( LI_FN ( SHGetFolderPathW )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
		create_default_json_object ( );
		return;
	}

	std::ifstream ofile ( std::wstring ( appdata ) + _ ( L"\\sesame\\global\\script_options.json" ) );

	if ( !ofile.is_open ( ) ) {
		create_default_json_object ( );
		return;
	}

	std::string dump;
	ofile.seekg ( 0, std::ios::end );
	const auto fsize = ofile.tellg ( );
	ofile.seekg ( 0, std::ios::beg );
	char* str = new char [ fsize ];
	ofile.read ( str, fsize );
	dump = str;
	delete [ ] str;
	ofile.close ( );

	if ( dump.empty ( ) ) {
		create_default_json_object ( );
		return;
	}

	script_options = cJSON_Parse ( dump.c_str ( ) );
}

void js::reload_script_option_list ( bool force_refresh ) {
	OPTION ( oxui::str, script_name, "Sesame->Customization->Scripts->Actions->Script Name", oxui::object_textbox );

	/* no script is selected or selected script didn't chance */
	if ( last_selected_script == script_name && !force_refresh )
		return;

	last_selected_script = script_name;
	option_list->objects.clear ( );

	if ( script_name.empty ( ) || !script_name.length ( ) )
		return;

	char u8_script_name [ 128 ] { '\0' };

	if ( WideCharToMultiByte ( CP_UTF8, 0, script_name.c_str ( ), -1, u8_script_name, 128, nullptr, nullptr ) <= 0 )
		return;

	if ( !script_options )
		return;

	auto script_entry = cJSON_GetObjectItemCaseSensitive ( script_options, u8_script_name );

	/* add script entry if it doesn't already exist */
	if ( !script_entry || !cJSON_IsObject ( script_entry ) )
		return;

	auto script_enabled = cJSON_GetObjectItemCaseSensitive ( script_entry, _ ( "Enabled" ) );

	if ( !script_enabled || !cJSON_IsBool ( script_enabled ) )
		return;

	option_list->add_element ( std::make_shared< oxui::checkbox > ( OSTR ( "Enabled" ) ) );
	std::dynamic_pointer_cast< oxui::checkbox >( option_list->objects.back( ) )->checked = !!script_enabled->valueint;

	cJSON* script_option = nullptr;
	cJSON_ArrayForEach ( script_option, script_entry ) {
		if ( !cJSON_IsArray ( script_option ) )
			continue;

		const auto var_type = cJSON_GetArrayItem ( script_option, 0 );
		const auto var_name = cJSON_GetArrayItem ( script_option, 1 );
		const auto var_value = cJSON_GetArrayItem ( script_option, 2 );

		if ( !var_type || !var_name || !var_value )
			continue;

		const auto wide_name = std::wstring ( var_name->valuestring, var_name->valuestring + std::strlen ( var_name->valuestring ) );

		switch ( static_cast < oxui::object_type > ( var_type->valueint ) ) {
		case oxui::object_type::object_checkbox: {
			option_list->add_element ( std::make_shared< oxui::checkbox > ( wide_name ) );
			std::dynamic_pointer_cast< oxui::checkbox >( option_list->objects.back ( ) )->checked = cJSON_IsTrue ( var_value );
			break;
		}
		case oxui::object_type::object_slider: {
			const auto slider_val = cJSON_GetArrayItem ( var_value, 0 );
			const auto slider_min = cJSON_GetArrayItem ( var_value, 1 );
			const auto slider_max = cJSON_GetArrayItem ( var_value, 2 );

			option_list->add_element ( std::make_shared< oxui::slider > ( wide_name, slider_val->valuedouble, slider_min->valuedouble, slider_max->valuedouble ) );
			break;
		}
		case oxui::object_type::object_dropdown: {
			const auto dropdown_val = cJSON_GetArrayItem ( var_value, 0 );
			const auto dropdown_items = cJSON_GetArrayItem ( var_value, 1 );

			std::vector< oxui::str > items_list { };
			cJSON* dropdown_option = nullptr;
			cJSON_ArrayForEach ( dropdown_option, dropdown_items ) {
				items_list.push_back ( std::wstring( dropdown_option->valuestring, dropdown_option->valuestring + strlen( dropdown_option->valuestring ) ) );
			}

			option_list->add_element ( std::make_shared< oxui::dropdown > ( wide_name, items_list ) );
			std::dynamic_pointer_cast< oxui::dropdown >( option_list->objects.back ( ) )->value = dropdown_val->valueint;
			break;
		}
		case oxui::object_type::object_keybind: {
			const auto keybind_var1 = cJSON_GetArrayItem ( var_value, 0 );
			const auto keybind_var2 = cJSON_GetArrayItem ( var_value, 1 );

			option_list->add_element ( std::make_shared< oxui::keybind > ( wide_name ) );
			std::dynamic_pointer_cast< oxui::keybind >( option_list->objects.back ( ) )->key = keybind_var1->valueint;
			std::dynamic_pointer_cast< oxui::keybind >( option_list->objects.back ( ) )->mode = static_cast<oxui::keybind_mode>( keybind_var2->valueint );
			break;
		}
		case oxui::object_type::object_colorpicker: {
			const auto keybind_var_r = cJSON_GetArrayItem ( var_value, 0 );
			const auto keybind_var_g = cJSON_GetArrayItem ( var_value, 1 );
			const auto keybind_var_b = cJSON_GetArrayItem ( var_value, 2 );
			const auto keybind_var_a = cJSON_GetArrayItem ( var_value, 3 );

			option_list->add_element ( std::make_shared< oxui::color_picker > ( wide_name, oxui::color{ keybind_var_r->valueint, keybind_var_g->valueint, keybind_var_b->valueint, keybind_var_a->valueint } ) );
			break;
		}
		}
	}
}

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

void js::destroy_fonts ( ) {
	for ( auto& font : cached_fonts )
		font.second.font->Release ( );
}

void js::reset_fonts ( ) {
	for ( auto& font : cached_fonts )
		LI_FN ( D3DXCreateFontA )( csgo::i::dev, font.second.size, 0, font.second.bold ? FW_BOLD : FW_NORMAL, 0, false, OEM_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font.second.family.c_str ( ), &font.second.font );
}

template < typename ...args_t >
void log_console ( const oxui::color& clr, const char* fmt, args_t ...args ) {
	if ( !fmt )
		return;

	struct {
		uint8_t r, g, b, a;
	} s_clr;

	s_clr = { static_cast < uint8_t > ( clr.r ), static_cast < uint8_t > ( clr.g ), static_cast < uint8_t > ( clr.b ), static_cast < uint8_t > ( clr.a ) };

	static auto con_color_msg = reinterpret_cast< void ( * )( const decltype( s_clr )&, const char*, ... ) >( LI_FN ( GetProcAddress ) ( LI_FN ( GetModuleHandleA ) ( _ ( "tier0.dll" ) ), _ ( "?ConColorMsg@@YAXABVColor@@PBDZZ" ) ) );

	con_color_msg ( s_clr, fmt, args... );
}

void js::process_render_callbacks ( ) {
	std::lock_guard< std::mutex > guard ( duktape_mutex );

	for ( auto& script : script_ctx ) {
		const auto has_callback = duk_get_global_string ( script.second, _("_render_cb") );
		
		duk_get_global_string ( script.second, _ ( "_script_id" ) );
		const auto script_entry = cJSON_GetObjectItemCaseSensitive ( script_options, duk_require_string ( script.second, -1 ) );
		duk_pop ( script.second );

		const auto is_enabled_ptr = script_entry ? cJSON_GetObjectItemCaseSensitive ( script_entry, _ ( "Enabled" ) ) : nullptr;
		const auto is_enabled = is_enabled_ptr ? cJSON_IsTrue ( is_enabled_ptr ) : false;
		
		if ( is_enabled && has_callback && duk_pcall ( script.second, 0 ) ) {
			char u8_script_name [ 128 ] { '\0' };

			if ( WideCharToMultiByte ( CP_UTF8, 0, script.first.c_str ( ), -1, u8_script_name, 128, nullptr, nullptr ) <= 0 )
				continue;

			log_console ( { 0xd8, 0x50, 0xd4, 255 }, _ ( "[ sesame ] " ) );
			log_console ( { 255, 50, 50, 255 }, ( char* ) _ ( u8"Error running \"%s\" script render callback: %s\n" ), u8_script_name, duk_safe_to_string ( script.second, -1 ) );
		}

		//if ( has_callback )
		duk_pop ( script.second );
	}
}

void js::process_net_update_callbacks ( ) {
	std::lock_guard< std::mutex > guard ( duktape_mutex );

	for ( auto& script : script_ctx ) {
		const auto has_callback = duk_get_global_string ( script.second, _("_net_update_cb") );
		
		duk_get_global_string ( script.second, _ ( "_script_id" ) );
		const auto script_entry = cJSON_GetObjectItemCaseSensitive ( script_options, duk_require_string ( script.second, -1 ) );
		duk_pop ( script.second );

		const auto is_enabled_ptr = script_entry ? cJSON_GetObjectItemCaseSensitive ( script_entry, _ ( "Enabled" ) ) : nullptr;
		const auto is_enabled = is_enabled_ptr ? cJSON_IsTrue ( is_enabled_ptr ) : false;

		if ( is_enabled && has_callback && duk_pcall ( script.second, 0 ) ) {
			char u8_script_name [ 128 ] { '\0' };

			if ( WideCharToMultiByte ( CP_UTF8, 0, script.first.c_str ( ), -1, u8_script_name, 128, nullptr, nullptr ) <= 0 )
				continue;

			log_console ( { 0xd8, 0x50, 0xd4, 255 }, _ ( "[ sesame ] " ) );
			log_console ( { 255, 50, 50, 255 }, ( char* ) _ ( u8"Error running \"%s\" script network update callback: %s\n" ), u8_script_name, duk_safe_to_string ( script.second, -1 ) );
		}

		//if ( has_callback )
		duk_pop ( script.second );
	}
}

void js::process_net_update_end_callbacks ( ) {
	std::lock_guard< std::mutex > guard ( duktape_mutex );

	for ( auto& script : script_ctx ) {
		const auto has_callback = duk_get_global_string ( script.second,_( "_net_update_end_cb" ));
		
		duk_get_global_string ( script.second, _ ( "_script_id" ) );
		const auto script_entry = cJSON_GetObjectItemCaseSensitive ( script_options, duk_require_string ( script.second, -1 ) );
		duk_pop ( script.second );

		const auto is_enabled_ptr = script_entry ? cJSON_GetObjectItemCaseSensitive ( script_entry, _ ( "Enabled" ) ) : nullptr;
		const auto is_enabled = is_enabled_ptr ? cJSON_IsTrue ( is_enabled_ptr ) : false;

		if ( is_enabled && has_callback && duk_pcall ( script.second, 0 ) ) {
			char u8_script_name [ 128 ] { '\0' };

			if ( WideCharToMultiByte ( CP_UTF8, 0, script.first.c_str ( ), -1, u8_script_name, 128, nullptr, nullptr ) <= 0 )
				continue;

			log_console ( { 0xd8, 0x50, 0xd4, 255 }, _ ( "[ sesame ] " ) );
			log_console ( { 255, 50, 50, 255 }, ( char* ) _ ( u8"Error running \"%s\" script network update end callback: %s\n" ), u8_script_name, duk_safe_to_string ( script.second, -1 ) );
		}

		//if ( has_callback )
		duk_pop ( script.second );
	}
}

void js::process_create_move_callbacks ( ) {
	std::lock_guard< std::mutex > guard ( duktape_mutex );

	for ( auto& script : script_ctx ) {
		const auto has_callback = duk_get_global_string ( script.second, _("_create_move_cb") );
		
		duk_get_global_string ( script.second, _ ( "_script_id" ) );
		const auto script_entry = cJSON_GetObjectItemCaseSensitive ( script_options, duk_require_string ( script.second, -1 ) );
		duk_pop ( script.second );

		const auto is_enabled_ptr = script_entry ? cJSON_GetObjectItemCaseSensitive ( script_entry, _ ( "Enabled" ) ) : nullptr;
		const auto is_enabled = is_enabled_ptr ? cJSON_IsTrue ( is_enabled_ptr ) : false;

		if ( is_enabled && has_callback && duk_pcall ( script.second, 0 ) ) {
			char u8_script_name [ 128 ] { '\0' };

			if ( WideCharToMultiByte ( CP_UTF8, 0, script.first.c_str ( ), -1, u8_script_name, 128, nullptr, nullptr ) <= 0 )
				continue;

			log_console ( { 0xd8, 0x50, 0xd4, 255 }, _ ( "[ sesame ] " ) );
			log_console ( { 255, 50, 50, 255 }, ( char* ) _ ( u8"Error running \"%s\" script create move callback: %s\n" ), u8_script_name, duk_safe_to_string ( script.second, -1 ) );
		}

		//if ( has_callback )
		duk_pop ( script.second );
	}
}

std::wstring utf8_to_utf16 ( const std::string& utf8 )
{
	std::vector<unsigned long> unicode;
	size_t i = 0;
	while ( i < utf8.size ( ) )
	{
		unsigned long uni;
		size_t todo;
		bool error = false;
		unsigned char ch = utf8 [ i++ ];
		if ( ch <= 0x7F )
		{
			uni = ch;
			todo = 0;
		}
		else if ( ch <= 0xBF )
		{
			throw std::logic_error ( _("not a UTF-8 string") );
		}
		else if ( ch <= 0xDF )
		{
			uni = ch & 0x1F;
			todo = 1;
		}
		else if ( ch <= 0xEF )
		{
			uni = ch & 0x0F;
			todo = 2;
		}
		else if ( ch <= 0xF7 )
		{
			uni = ch & 0x07;
			todo = 3;
		}
		else
		{
			throw std::logic_error ( _("not a UTF-8 string") );
		}
		for ( size_t j = 0; j < todo; ++j )
		{
			if ( i == utf8.size ( ) )
				throw std::logic_error ( _("not a UTF-8 string") );
			unsigned char ch = utf8 [ i++ ];
			if ( ch < 0x80 || ch > 0xBF )
				throw std::logic_error ( _("not a UTF-8 string") );
			uni <<= 6;
			uni += ch & 0x3F;
		}
		if ( uni >= 0xD800 && uni <= 0xDFFF )
			throw std::logic_error ( _("not a UTF-8 string") );
		if ( uni > 0x10FFFF )
			throw std::logic_error ( _("not a UTF-8 string") );
		unicode.push_back ( uni );
	}
	std::wstring utf16;
	for ( size_t i = 0; i < unicode.size ( ); ++i )
	{
		unsigned long uni = unicode [ i ];
		if ( uni <= 0xFFFF )
		{
			utf16 += ( wchar_t ) uni;
		}
		else
		{
			uni -= 0x10000;
			utf16 += ( wchar_t ) ( ( uni >> 10 ) + 0xD800 );
			utf16 += ( wchar_t ) ( ( uni & 0x3FF ) + 0xDC00 );
		}
	}
	return utf16;
}

__forceinline std::vector< oxui::str > split ( const oxui::str& str, const oxui::str& delim ) {
	std::vector< oxui::str > tokens;
	size_t prev = 0, pos = 0;

	do {
		pos = str.find ( delim, prev );

		if ( pos == oxui::str::npos )
			pos = str.length ( );

		oxui::str token = str.substr ( prev, pos - prev );

		if ( !token.empty ( ) )
			tokens.push_back ( token );

		prev = pos + delim.length ( );
	} while ( pos < str.length ( ) && prev < str.length ( ) );

	return tokens;
}

namespace bindings {
	namespace console {
		static duk_ret_t log ( duk_context* ctx ) {
			duk_push_string ( ctx, " " );
			duk_insert ( ctx, 0 );
			duk_join ( ctx, duk_get_top ( ctx ) - 1 );
			log_console ( { 0xd8, 0x50, 0xd4, 255 }, _ ( "[ sesame ] " ) );
			log_console ( { 255, 255, 255, 255 }, _ ( "%s\n" ), duk_safe_to_string ( ctx, -1 ) );
			return 0;
		}

		static duk_ret_t print ( duk_context* ctx ) {
			duk_push_string ( ctx, " " );
			duk_insert ( ctx, 0 );
			duk_join ( ctx, duk_get_top ( ctx ) - 1 );
			log_console ( { 255, 255, 255, 255 }, _ ( "%s\n" ), duk_safe_to_string ( ctx, -1 ) );
			return 0;
		}

		static duk_ret_t error ( duk_context* ctx ) {
			duk_push_string ( ctx, " " );
			duk_insert ( ctx, 0 );
			duk_join ( ctx, duk_get_top ( ctx ) - 1 );
			log_console ( { 0xd8, 0x50, 0xd4, 255 }, _ ( "[ sesame ] " ) );
			log_console ( { 255, 75, 75, 255 }, _ ( "%s\n" ), duk_safe_to_string ( ctx, -1 ) );
			return 0;
		}

		static duk_ret_t warning ( duk_context* ctx ) {
			duk_push_string ( ctx, " " );
			duk_insert ( ctx, 0 );
			duk_join ( ctx, duk_get_top ( ctx ) - 1 );
			log_console ( { 0xd8, 0x50, 0xd4, 255 }, _ ( "[ sesame ] " ) );
			log_console ( { 255, 255, 90, 255 }, _ ( "%s\n" ), duk_safe_to_string ( ctx, -1 ) );
			return 0;
		}
	}

	namespace globals {
		static duk_ret_t realtime ( duk_context* ctx ) {
			duk_push_number ( ctx, csgo::i::globals->m_realtime );
			return 1;
		}

		static duk_ret_t curtime ( duk_context* ctx ) {
			duk_push_number ( ctx, csgo::i::globals->m_curtime );
			return 1;
		}

		static duk_ret_t frametime ( duk_context* ctx ) {
			duk_push_number ( ctx, csgo::i::globals->m_frametime );
			return 1;
		}

		static duk_ret_t absframetime ( duk_context* ctx ) {
			duk_push_number ( ctx, csgo::i::globals->m_abs_frametime );
			return 1;
		}

		static duk_ret_t max_players ( duk_context* ctx ) {
			duk_push_int ( ctx, csgo::i::globals->m_max_clients );
			return 1;
		}

		static duk_ret_t tickcount ( duk_context* ctx ) {
			duk_push_int ( ctx, csgo::i::globals->m_tickcount );
			return 1;
		}

		static duk_ret_t interval_per_tick ( duk_context* ctx ) {
			duk_push_number ( ctx, csgo::i::globals->m_ipt );
			return 1;
		}

		static duk_ret_t framecount ( duk_context* ctx ) {
			duk_push_int ( ctx, csgo::i::globals->m_framecount );
			return 1;
		}
	}

	namespace renderer {
		uint32_t duk_require_color ( duk_context* ctx, int arg ) {
			const auto success1 = duk_get_prop_string ( ctx, arg, _ ( "r" ) );
			const auto success2 = duk_get_prop_string ( ctx, arg, _ ( "g" ) );
			const auto success3 = duk_get_prop_string ( ctx, arg, _ ( "b" ) );
			const auto success4 = duk_get_prop_string ( ctx, arg, _ ( "a" ) );

			uint32_t clr = 0;

			if ( success1 && success2 && success3 && success4 )
				clr = D3DCOLOR_RGBA ( duk_require_int ( ctx, -4 ), duk_require_int ( ctx, -3 ), duk_require_int ( ctx, -2 ), duk_require_int ( ctx, -1 ) );

			if ( success1 )
			duk_pop ( ctx );
			if( success2 )
			duk_pop ( ctx );
			if( success3 )
			duk_pop ( ctx );
			if( success4 )
			duk_pop ( ctx );
			
			return clr;
		}

		static duk_ret_t rect ( duk_context* ctx ) {
			if ( duk_require_boolean( ctx, 5 ) )
				::render::rectangle ( duk_require_int ( ctx, 0 ), duk_require_int ( ctx, 1 ), duk_require_int ( ctx, 2 ), duk_require_int ( ctx, 3 ), duk_require_color ( ctx, 4 ) );
			else
				::render::outline ( duk_require_int ( ctx, 0 ), duk_require_int ( ctx, 1 ), duk_require_int ( ctx, 2 ), duk_require_int ( ctx, 3 ), duk_require_color ( ctx, 4 ) );

			return 0;
		}

		static duk_ret_t rounded_rect ( duk_context* ctx ) {
			::render::rounded_rect ( duk_require_int ( ctx, 0 ), duk_require_int ( ctx, 1 ), duk_require_int ( ctx, 2 ), duk_require_int ( ctx, 3 ), duk_require_int ( ctx, 4 ), duk_require_int ( ctx, 4 ), duk_require_color ( ctx, 5 ), !duk_require_boolean ( ctx, 6 ) );

			return 0;
		}

		static duk_ret_t line ( duk_context* ctx ) {
			::render::line ( duk_require_int ( ctx, 0 ), duk_require_int ( ctx, 1 ), duk_require_int ( ctx, 2 ), duk_require_int ( ctx, 3 ), duk_require_color ( ctx, 4 ) );

			return 0;
		}

		static duk_ret_t circle ( duk_context* ctx ) {
			::render::circle ( duk_require_int ( ctx, 0 ), duk_require_int ( ctx, 1 ), duk_require_int ( ctx, 2 ), duk_require_int ( ctx, 2 ) * 2, duk_require_color ( ctx, 3 ), 0, 0.0f, !duk_require_boolean ( ctx, 4 ) );

			return 0;
		}

		static duk_ret_t add_font ( duk_context* ctx ) {
			font_data_t font_data;
			font_data.family = duk_require_string ( ctx, 0 );
			font_data.size = duk_require_int ( ctx, 1 );
			font_data.bold = duk_require_boolean ( ctx, 2 );

			std::string font_id;

			if ( duk_get_global_string ( ctx, _ ( "_script_id" ) ) ) {
				font_id = std::string ( duk_require_string ( ctx, -1 ) ) + _ ( "_" ) + font_data.family + _ ( "_" ) + std::to_string ( font_data.size ) + _ ( "_" ) + std::to_string ( static_cast< int > ( font_data.bold ) );
				duk_pop ( ctx );
			}

			ID3DXFont* font_out = nullptr;
			LI_FN ( D3DXCreateFontA )( csgo::i::dev, font_data.size, 0, font_data.bold ? FW_BOLD : FW_NORMAL, 0, false, OEM_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font_data.family.c_str( ), &font_data.font );

			if ( !font_id.empty() )
				cached_fonts [ font_id ] = font_data;

			/* add font returns font id */
			duk_push_string ( ctx, font_id.empty ( ) ? _("default") : font_id.c_str( ) );

			return 1;
		}

		static duk_ret_t text ( duk_context* ctx ) {
			const auto font = cached_fonts.find ( duk_require_string ( ctx, 4 ) );

			/* couldn't find target font */
			if ( font == cached_fonts.end ( ) )
				return 0;

			::render::text ( duk_require_int ( ctx, 0 ), duk_require_int ( ctx, 1 ), duk_require_color ( ctx, 3 ), font->second.font, utf8_to_utf16 ( duk_require_string ( ctx, 2 ) ) );

			return 0;
		}

		static duk_ret_t text_shadow ( duk_context* ctx ) {
			const auto font = cached_fonts.find ( duk_require_string ( ctx, 4 ) );

			/* couldn't find target font */
			if ( font == cached_fonts.end ( ) )
				return 0;

			::render::text ( duk_require_int ( ctx, 0 ), duk_require_int ( ctx, 1 ), duk_require_color ( ctx, 3 ), font->second.font, utf8_to_utf16 ( duk_require_string ( ctx, 2 ) ), true );

			return 0;
		}

		static duk_ret_t text_outline ( duk_context* ctx ) {
			const auto font = cached_fonts.find ( duk_require_string ( ctx, 4 ) );

			/* couldn't find target font */
			if ( font == cached_fonts.end ( ) )
				return 0;

			::render::text ( duk_require_int ( ctx, 0 ), duk_require_int ( ctx, 1 ), duk_require_color ( ctx, 3 ), font->second.font, utf8_to_utf16 ( duk_require_string ( ctx, 2 ) ), false, true );

			return 0;
		}
	}

	namespace types {
		/* color object */
		namespace color {
			static duk_ret_t constructor ( duk_context* ctx ) {
				if ( !duk_is_constructor_call ( ctx ) ) {
					return DUK_RET_TYPE_ERROR;
				}

				duk_push_this ( ctx );

				duk_dup ( ctx, 0 );
				duk_put_prop_string ( ctx, -2, _ ( "r" ) );
				duk_dup ( ctx, 1 );
				duk_put_prop_string ( ctx, -2, _ ( "g" ) );
				duk_dup ( ctx, 2 );
				duk_put_prop_string ( ctx, -2, _ ( "b" ) );
				duk_dup ( ctx, 3 );
				duk_put_prop_string ( ctx, -2, _ ( "a" ) );

				return 0;
			}

			static void init ( duk_context* ctx ) {
				duk_push_c_function ( ctx, constructor, 4 /* nargs */ );
				duk_put_global_string ( ctx, _ ( "color" ) );
			}
		}

		namespace ui_element {
			static duk_ret_t constructor ( duk_context* ctx ) {
				if ( !duk_is_constructor_call ( ctx ) ) {
					return DUK_RET_TYPE_ERROR;
				}

				duk_push_this ( ctx );

				duk_dup ( ctx, 0 );
				duk_put_prop_string ( ctx, -2, _ ( "ptr" ) );
				duk_dup ( ctx, 1 );
				duk_put_prop_string ( ctx, -2, _ ( "type" ) );
				duk_dup ( ctx, 2 );
				duk_put_prop_string ( ctx, -2, _ ( "is_jobj" ) );

				return 0;
			}

			std::array< bool, 512 > toggled { false };
			std::array< bool, 512 > output { false };

			static duk_ret_t get ( duk_context* ctx ) {
				duk_push_this ( ctx );

				duk_get_prop_string ( ctx, -1, _ ( "ptr" ) );
				const auto element_ptr = duk_require_pointer ( ctx, -1 );
				duk_pop ( ctx );

				if ( !element_ptr ) {
					duk_push_int ( ctx, 0 );
					return 1;
				}

				duk_get_prop_string ( ctx, -1, _ ( "type" ) );
				const auto element_type = duk_require_string ( ctx, -1 );
				duk_pop ( ctx );

				duk_get_prop_string ( ctx, -1, _ ( "is_jobj" ) );
				const auto element_is_json = duk_require_boolean ( ctx, -1 );
				duk_pop ( ctx );

				if ( !strcmp ( element_type, _ ( "checkbox" ) ) )
					duk_push_boolean ( ctx, element_is_json ? cJSON_IsTrue ( reinterpret_cast< cJSON* > ( element_ptr ) ) : *reinterpret_cast< bool* > ( element_ptr ) );
				else if ( !strcmp ( element_type, _ ( "slider" ) ) )
					duk_push_number ( ctx, *reinterpret_cast< double* > ( element_ptr ) );
				else if ( !strcmp ( element_type, _ ( "dropdown" ) ) )
					duk_push_int ( ctx, *reinterpret_cast< int* > ( element_ptr ) );
				else if ( !strcmp ( element_type, _ ( "keybind" ) ) ) {
					auto key = 0;
					auto mode = 0;

					if ( element_is_json ) {
						const auto json_key = cJSON_GetArrayItem ( reinterpret_cast< cJSON* > ( element_ptr ), 0 );
						const auto json_mode = cJSON_GetArrayItem ( reinterpret_cast< cJSON* > ( element_ptr ), 1 );

						if ( !json_key || !json_mode ) {
							duk_push_int ( ctx, 0 );
							return 1;
						}

						key = json_key->valueint;
						mode = json_mode->valueint;
					}
					else {
						const auto& keybind_obj = *reinterpret_cast< oxui::keybind* >( element_ptr );

						key = keybind_obj.key;
						mode = static_cast< int >( keybind_obj.mode );
					}

					switch ( static_cast< oxui::keybind_mode >( mode ) ) {
					case oxui::keybind_mode::hold: { output [ key ] = key != -1 && utils::key_state ( key ); } break;
					case oxui::keybind_mode::toggle: {
						if ( key != -1 ) {
							if ( toggled [ key ] && !utils::key_state ( key ) )
								output [ key ] = !output [ key ];
							toggled [ key ] = utils::key_state ( key );
						}
						else output [ key ] = false;
					} break;
					case oxui::keybind_mode::always: { output [ key ] = true; } break;
					}

					duk_push_boolean ( ctx, output [ key ] );
				}
				else if ( !strcmp ( element_type, _ ( "colorpicker" ) ) ) {
					if ( element_is_json ) {
						const auto r = cJSON_GetArrayItem ( reinterpret_cast< cJSON* > ( element_ptr ), 0 );
						const auto g = cJSON_GetArrayItem ( reinterpret_cast< cJSON* > ( element_ptr ), 1 );
						const auto b = cJSON_GetArrayItem ( reinterpret_cast< cJSON* > ( element_ptr ), 2 );
						const auto a = cJSON_GetArrayItem ( reinterpret_cast< cJSON* > ( element_ptr ), 3 );

						if ( !r || !g || !b || !a ) {
							duk_get_global_string ( ctx, _ ( "color" ) );
							duk_push_int ( ctx, 255 );
							duk_push_int ( ctx, 255 );
							duk_push_int ( ctx, 255 );
							duk_push_int ( ctx, 255 );
							duk_new ( ctx, 4 );
						}
						else {
							duk_get_global_string ( ctx, _ ( "color" ) );
							duk_push_int ( ctx, r->valueint );
							duk_push_int ( ctx, g->valueint );
							duk_push_int ( ctx, b->valueint );
							duk_push_int ( ctx, a->valueint );
							duk_new ( ctx, 4 );
						}
					}
					else {
						const auto& as_oclr = *reinterpret_cast< oxui::color* > ( element_ptr );

						duk_get_global_string ( ctx, _ ( "color" ) );
						duk_push_int ( ctx, as_oclr.r );
						duk_push_int ( ctx, as_oclr.g );
						duk_push_int ( ctx, as_oclr.b );
						duk_push_int ( ctx, as_oclr.a );
						duk_new ( ctx, 4 );
					}
				}
				else {
					duk_push_int ( ctx, 0 );
				}

				return 1;
			}

			static duk_ret_t set ( duk_context* ctx ) {
				/* make sure we are only passing in 1 value (we can only set 1 value at a time) */
				if ( duk_get_top ( ctx ) != 1 )
					return 0;

				duk_push_this ( ctx );

				duk_get_prop_string ( ctx, -1, _ ( "ptr" ) );
				const auto element_ptr = duk_require_pointer ( ctx, -1 );
				duk_pop ( ctx );

				if ( !element_ptr )
					return 0;

				duk_get_prop_string ( ctx, -1, _ ( "type" ) );
				const auto element_type = duk_require_string ( ctx, -1 );
				duk_pop ( ctx );

				duk_get_prop_string ( ctx, -1, _ ( "is_jobj" ) );
				const auto element_is_json = duk_require_boolean ( ctx, -1 );
				duk_pop ( ctx );

				if ( !strcmp ( element_type, _ ( "checkbox" ) ) ) {
					if ( element_is_json )
						reinterpret_cast< cJSON* > ( element_ptr )->type = duk_to_boolean ( ctx, 0 ) + 1;
					else
						*reinterpret_cast< bool* > ( element_ptr ) = duk_to_boolean ( ctx, 0 );
				}
				else if ( !strcmp ( element_type, _ ( "slider" ) ) )
					*reinterpret_cast< double* > ( element_ptr ) = duk_to_number( ctx, 0 );
				else if ( !strcmp ( element_type, _ ( "dropdown" ) ) )
					*reinterpret_cast< int* > ( element_ptr ) = duk_to_int( ctx, 0 );
				else if ( !strcmp ( element_type, _ ( "keybind" ) ) ) {
					if ( element_is_json ) {
						auto json_key = cJSON_GetArrayItem ( reinterpret_cast< cJSON* > ( element_ptr ), 0 );
						json_key->valueint = duk_to_int ( ctx, 0 );
					}
					else {
						auto& keybind_obj = *reinterpret_cast< oxui::keybind* >( element_ptr );
						keybind_obj.key = duk_to_int ( ctx, 0 );
					}
				}
				else if ( !strcmp ( element_type, _ ( "colorpicker" ) ) ) {
					const auto clr = renderer::duk_require_color ( ctx, 0 );

					const auto cr = ( clr >> 16 ) & 0xff;
					const auto cg = ( clr >> 8 ) & 0xff;
					const auto cb = ( clr >> 0 ) & 0xff;
					const auto ca = ( clr >> 24 ) & 0xff;

					if ( element_is_json ) {
						const auto r = cJSON_GetArrayItem ( reinterpret_cast< cJSON* > ( element_ptr ), 0 );
						const auto g = cJSON_GetArrayItem ( reinterpret_cast< cJSON* > ( element_ptr ), 1 );
						const auto b = cJSON_GetArrayItem ( reinterpret_cast< cJSON* > ( element_ptr ), 2 );
						const auto a = cJSON_GetArrayItem ( reinterpret_cast< cJSON* > ( element_ptr ), 3 );

						if ( !r || !g || !b || !a ) {
							return 0;
						}

						r->valueint = cr;
						g->valueint = cg;
						b->valueint = cb;
						a->valueint = ca;
					}
					else {
						auto& as_oclr = *reinterpret_cast< oxui::color* > ( element_ptr );
						as_oclr.r = cr;
						as_oclr.g = cg;
						as_oclr.b = cb;
						as_oclr.a = ca;
					}
				}

				return 0;
			}

			static void init ( duk_context* ctx ) {
				duk_push_c_function ( ctx, constructor, 3 /* nargs */ );
				duk_push_object ( ctx );
				duk_push_c_function ( ctx, get, 0 /*nargs*/ );
				duk_put_prop_string ( ctx, -2, _ ( "get" ) );
				duk_push_c_function ( ctx, set, 1 /*nargs*/ );
				duk_put_prop_string ( ctx, -2, _ ( "set" ) );
				duk_put_prop_string ( ctx, -2, _ ( "prototype" ) );
				duk_put_global_string ( ctx, _ ( "ui_element" ) );
			}
		}
	}

	namespace sesame {
		static duk_ret_t on_render ( duk_context* ctx ) {
			duk_dup ( ctx, 0 );
			duk_put_global_string ( ctx, _ ( "_render_cb" ) );

			return 0;
		}

		static duk_ret_t on_net_update ( duk_context* ctx ) {
			duk_dup ( ctx, 0 );
			duk_put_global_string ( ctx, _ ( "_net_update_cb" ) );

			return 0;
		}

		static duk_ret_t on_net_update_end ( duk_context* ctx ) {
			duk_dup ( ctx, 0 );
			duk_put_global_string ( ctx, _ ( "_net_update_end_cb" ) );

			return 0;
		}

		static duk_ret_t on_create_move ( duk_context* ctx ) {
			duk_dup ( ctx, 0 );
			duk_put_global_string ( ctx, _ ( "_create_move_cb" ) );

			return 0;
		}

		namespace ui {
			static duk_ret_t get_element ( duk_context* ctx, const char* obj_name ) {
				duk_get_global_string ( ctx, _ ( "_script_id" ) );
				auto script_entry = cJSON_GetObjectItemCaseSensitive ( script_options, duk_require_string ( ctx, -1 ) );
				duk_pop ( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_entry || !cJSON_IsObject ( script_entry ) ) {
					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, nullptr );
					duk_push_string ( ctx, _ ( "" ) );
					duk_push_boolean ( ctx, false );
					duk_new ( ctx, 3 );
					return 1;
				}

				auto new_object_name = duk_require_string ( ctx, 0 );
				const auto script_option = cJSON_GetObjectItemCaseSensitive ( script_entry, obj_name );

				if ( !script_option ) {
					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, nullptr );
					duk_push_string ( ctx, _ ( "" ) );
					duk_push_boolean ( ctx, false );
					duk_new ( ctx, 3 );
					return 1;
				}

				const auto type = cJSON_GetArrayItem ( script_option, 0 );

				if ( !type || !cJSON_IsNumber ( type ) ) {
					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, nullptr );
					duk_push_string ( ctx, _ ( "" ) );
					duk_push_boolean ( ctx, false );
					duk_new ( ctx, 3 );
					return 1;
				}

				switch ( static_cast < oxui::object_type > ( type->valueint ) ) {
				case oxui::object_checkbox: {
					const auto arr = cJSON_GetArrayItem ( script_option, 2 );

					if ( !arr ) {
						duk_get_global_string ( ctx, _ ( "ui_element" ) );
						duk_push_pointer ( ctx, nullptr );
						duk_push_string ( ctx, _ ( "" ) );
						duk_push_boolean ( ctx, false );
						duk_new ( ctx, 3 );
						return 1;
					}

					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, arr );
					duk_push_string ( ctx, _ ( "checkbox" ) );
					duk_push_boolean ( ctx, true );
					duk_new ( ctx, 3 );
				} break;
				case oxui::object_slider: {
					const auto arr = cJSON_GetArrayItem ( script_option, 2 );

					if ( !arr ) {
						duk_get_global_string ( ctx, _ ( "ui_element" ) );
						duk_push_pointer ( ctx, nullptr );
						duk_push_string ( ctx, _ ( "" ) );
						duk_push_boolean ( ctx, false );
						duk_new ( ctx, 3 );
						return 1;
					}

					const auto val = cJSON_GetArrayItem ( arr, 0 );

					if ( !val ) {
						duk_get_global_string ( ctx, _ ( "ui_element" ) );
						duk_push_pointer ( ctx, nullptr );
						duk_push_string ( ctx, _ ( "" ) );
						duk_push_boolean ( ctx, false );
						duk_new ( ctx, 3 );
						return 1;
					}

					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, &val->valuedouble );
					duk_push_string ( ctx, _ ( "slider" ) );
					duk_push_boolean ( ctx, true );
					duk_new ( ctx, 3 );
				} break;
				case oxui::object_dropdown: {
					const auto arr = cJSON_GetArrayItem ( script_option, 2 );

					if ( !arr ) {
						duk_get_global_string ( ctx, _ ( "ui_element" ) );
						duk_push_pointer ( ctx, nullptr );
						duk_push_string ( ctx, _ ( "" ) );
						duk_push_boolean ( ctx, false );
						duk_new ( ctx, 3 );
						return 1;
					}

					const auto val = cJSON_GetArrayItem ( arr, 0 );

					if ( !val ) {
						duk_get_global_string ( ctx, _ ( "ui_element" ) );
						duk_push_pointer ( ctx, nullptr );
						duk_push_string ( ctx, _ ( "" ) );
						duk_push_boolean ( ctx, false );
						duk_new ( ctx, 3 );
						return 1;
					}

					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, &val->valueint );
					duk_push_string ( ctx, _ ( "dropdown" ) );
					duk_push_boolean ( ctx, true );
					duk_new ( ctx, 3 );
				} break;
				case oxui::object_keybind: {
					const auto arr = cJSON_GetArrayItem ( script_option, 2 );

					if ( !arr ) {
						duk_get_global_string ( ctx, _ ( "ui_element" ) );
						duk_push_pointer ( ctx, nullptr );
						duk_push_string ( ctx, _ ( "" ) );
						duk_push_boolean ( ctx, false );
						duk_new ( ctx, 3 );
						return 1;
					}

					const auto val = cJSON_GetArrayItem ( arr, 0 );

					if ( !val ) {
						duk_get_global_string ( ctx, _ ( "ui_element" ) );
						duk_push_pointer ( ctx, nullptr );
						duk_push_string ( ctx, _ ( "" ) );
						duk_push_boolean ( ctx, false );
						duk_new ( ctx, 3 );
						return 1;
					}

					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, val );
					duk_push_string ( ctx, _ ( "keybind" ) );
					duk_push_boolean ( ctx, true );
					duk_new ( ctx, 3 );
				} break;
				case oxui::object_colorpicker: {
					const auto arr = cJSON_GetArrayItem ( script_option, 2 );

					if ( !arr ) {
						duk_get_global_string ( ctx, _ ( "ui_element" ) );
						duk_push_pointer ( ctx, nullptr );
						duk_push_string ( ctx, _ ( "" ) );
						duk_push_boolean ( ctx, false );
						duk_new ( ctx, 3 );
						return 1;
					}

					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, arr );
					duk_push_string ( ctx, _ ( "colorpicker" ) );
					duk_push_boolean ( ctx, true );
					duk_new ( ctx, 3 );
				} break;
				default: {
					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, nullptr );
					duk_push_string ( ctx, _ ( "" ) );
					duk_push_boolean ( ctx, false );
					duk_new ( ctx, 3 );
				} break;
				}

				return 1;
			}

			static duk_ret_t add_checkbox ( duk_context* ctx ) {
				duk_get_global_string ( ctx, _ ( "_script_id" ) );
				auto script_entry = cJSON_GetObjectItemCaseSensitive ( script_options, duk_require_string ( ctx, -1 ) );
				duk_pop ( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_entry || !cJSON_IsObject ( script_entry ) ) {
					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, nullptr );
					duk_push_string ( ctx, _ ( "" ) );
					duk_push_boolean ( ctx, false );
					duk_new ( ctx, 3 );
					return 1;
				}

				const auto new_object_name = duk_require_string ( ctx, 0 );
				const auto script_option = cJSON_GetObjectItemCaseSensitive ( script_entry, new_object_name );

				if ( !script_option ) {
					const auto script_option_array = cJSON_CreateArray ( );
					cJSON_AddItemToArray ( script_option_array, cJSON_CreateNumber ( oxui::object_checkbox ) );
					cJSON_AddItemToArray ( script_option_array, cJSON_CreateString ( new_object_name ) );
					cJSON_AddItemToArray ( script_option_array, cJSON_CreateBool ( duk_require_boolean ( ctx, 1 ) ) );
					cJSON_AddItemToObject ( script_entry, new_object_name, script_option_array );
				}

				get_element ( ctx, new_object_name );

				return 1;
			}

			static duk_ret_t add_slider ( duk_context* ctx ) {
				duk_get_global_string ( ctx, _ ( "_script_id" ) );
				auto script_entry = cJSON_GetObjectItemCaseSensitive ( script_options, duk_require_string ( ctx, -1 ) );
				duk_pop ( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_entry || !cJSON_IsObject ( script_entry ) ) {
					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, nullptr );
					duk_push_string ( ctx, _ ( "" ) );
					duk_push_boolean ( ctx, false );
					duk_new ( ctx, 3 );
					return 1;
				}

				const auto new_object_name = duk_require_string ( ctx, 0 );
				const auto script_option = cJSON_GetObjectItemCaseSensitive ( script_entry, new_object_name );

				if ( !script_option ) {
					const auto slider_settings = cJSON_CreateArray ( );
					cJSON_AddItemToArray ( slider_settings, cJSON_CreateNumber ( duk_require_number ( ctx, 1 ) ) );
					cJSON_AddItemToArray ( slider_settings, cJSON_CreateNumber ( duk_require_number ( ctx, 2 ) ) );
					cJSON_AddItemToArray ( slider_settings, cJSON_CreateNumber ( duk_require_number ( ctx, 3 ) ) );

					const auto script_option_array = cJSON_CreateArray ( );
					cJSON_AddItemToArray ( script_option_array, cJSON_CreateNumber ( oxui::object_slider ) );
					cJSON_AddItemToArray ( script_option_array, cJSON_CreateString ( new_object_name ) );
					cJSON_AddItemToArray ( script_option_array, slider_settings );
					cJSON_AddItemToObject ( script_entry, new_object_name, script_option_array );
				}

				get_element ( ctx, new_object_name );

				return 1;
			}

			static duk_ret_t add_dropdown ( duk_context* ctx ) {
				duk_get_global_string ( ctx, _ ( "_script_id" ) );
				auto script_entry = cJSON_GetObjectItemCaseSensitive ( script_options, duk_require_string ( ctx, -1 ) );
				duk_pop ( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_entry || !cJSON_IsObject ( script_entry ) ) {
					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, nullptr );
					duk_push_string ( ctx, _ ( "" ) );
					duk_push_boolean ( ctx, false );
					duk_new ( ctx, 3 );
					return 1;
				}

				const auto new_object_name = duk_require_string ( ctx, 0 );
				const auto script_option = cJSON_GetObjectItemCaseSensitive ( script_entry, new_object_name );

				if ( !script_option ) {
					const auto dropdown_items = cJSON_CreateArray ( );
					const auto items_len = duk_get_length ( ctx, 2 );

					for ( auto i = 0; i < items_len; i++ ) {
						duk_get_prop_index ( ctx, 2, i );
						cJSON_AddItemToArray ( dropdown_items, cJSON_CreateString ( duk_require_string ( ctx, -1 ) ) );
						duk_pop ( ctx );
					}

					const auto dropdown_settings = cJSON_CreateArray ( );
					cJSON_AddItemToArray ( dropdown_settings, cJSON_CreateNumber ( duk_require_int ( ctx, 1 ) ) );
					cJSON_AddItemToArray ( dropdown_settings, dropdown_items );

					const auto script_option_array = cJSON_CreateArray ( );
					cJSON_AddItemToArray ( script_option_array, cJSON_CreateNumber ( oxui::object_dropdown ) );
					cJSON_AddItemToArray ( script_option_array, cJSON_CreateString ( new_object_name ) );
					cJSON_AddItemToArray ( script_option_array, dropdown_settings );
					cJSON_AddItemToObject ( script_entry, new_object_name, script_option_array );
				}

				get_element ( ctx, new_object_name );

				return 1;
			}

			static duk_ret_t add_keybind ( duk_context* ctx ) {
				duk_get_global_string ( ctx, _ ( "_script_id" ) );
				auto script_entry = cJSON_GetObjectItemCaseSensitive ( script_options, duk_require_string ( ctx, -1 ) );
				duk_pop ( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_entry || !cJSON_IsObject ( script_entry ) ) {
					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, nullptr );
					duk_push_string ( ctx, _ ( "" ) );
					duk_push_boolean ( ctx, false );
					duk_new ( ctx, 3 );
					return 1;
				}

				const auto new_object_name = duk_require_string ( ctx, 0 );
				const auto script_option = cJSON_GetObjectItemCaseSensitive ( script_entry, new_object_name );

				if ( !script_option ) {
					const auto keybind_settings = cJSON_CreateArray ( );
					cJSON_AddItemToArray ( keybind_settings, cJSON_CreateNumber ( duk_require_int ( ctx, 1 ) ) );
					cJSON_AddItemToArray ( keybind_settings, cJSON_CreateNumber ( duk_require_int ( ctx, 2 ) ) );

					const auto script_option_array = cJSON_CreateArray ( );
					cJSON_AddItemToArray ( script_option_array, cJSON_CreateNumber ( oxui::object_keybind ) );
					cJSON_AddItemToArray ( script_option_array, cJSON_CreateString ( new_object_name ) );
					cJSON_AddItemToArray ( script_option_array, keybind_settings );
					cJSON_AddItemToObject ( script_entry, new_object_name, script_option_array );
				}

				get_element ( ctx, new_object_name );

				return 1;
			}

			static duk_ret_t add_colorpicker ( duk_context* ctx ) {
				duk_get_global_string ( ctx, _ ( "_script_id" ) );
				auto script_entry = cJSON_GetObjectItemCaseSensitive ( script_options, duk_require_string ( ctx, -1 ) );
				duk_pop ( ctx );

				/* add script entry if it doesn't already exist */
				if ( !script_entry || !cJSON_IsObject ( script_entry ) ) {
					duk_get_global_string ( ctx, _ ( "ui_element" ) );
					duk_push_pointer ( ctx, nullptr );
					duk_push_string ( ctx, _ ( "" ) );
					duk_push_boolean ( ctx, false );
					duk_new ( ctx, 3 );
					return 1;
				}

				const auto new_object_name = duk_require_string ( ctx, 0 );
				const auto script_option = cJSON_GetObjectItemCaseSensitive ( script_entry, new_object_name );

				if ( !script_option ) {
					const auto clr = renderer::duk_require_color ( ctx, 1 );

					const auto color_settings = cJSON_CreateArray ( );
					cJSON_AddItemToArray ( color_settings, cJSON_CreateNumber ( ( clr >> 16 ) & 0xff ) );
					cJSON_AddItemToArray ( color_settings, cJSON_CreateNumber ( ( clr >> 8 ) & 0xff ) );
					cJSON_AddItemToArray ( color_settings, cJSON_CreateNumber ( ( clr >> 0 ) & 0xff ) );
					cJSON_AddItemToArray ( color_settings, cJSON_CreateNumber ( ( clr >> 24 ) & 0xff ) );

					const auto script_option_array = cJSON_CreateArray ( );
					cJSON_AddItemToArray ( script_option_array, cJSON_CreateNumber ( oxui::object_colorpicker ) );
					cJSON_AddItemToArray ( script_option_array, cJSON_CreateString ( new_object_name ) );
					cJSON_AddItemToArray ( script_option_array, color_settings );
					cJSON_AddItemToObject ( script_entry, new_object_name, script_option_array );
				}

				get_element ( ctx, new_object_name );

				return 1;
			}

			void* find_obj ( const char* dir, oxui::object_type type ) {
				auto as_utf16 = utf8_to_utf16 ( dir );

				/* replace aimbot temp key */ {
					const std::wstring find_aimbot_key = _ ( L"->Aimbot->" );
					const auto find_aimbot_res = as_utf16.find ( find_aimbot_key );

					if ( find_aimbot_res != std::wstring::npos )
						as_utf16.replace ( find_aimbot_res, find_aimbot_key.size ( ), _ ( L"->A->" ) );
				}

				/* replace antiaim temp key */ {
					const std::wstring find_aimbot_key = _ ( L"->Antiaim->" );
					const auto find_aimbot_res = as_utf16.find ( find_aimbot_key );

					if ( find_aimbot_res != std::wstring::npos )
						as_utf16.replace ( find_aimbot_res, find_aimbot_key.size ( ), _ ( L"->B->" ) );
				}

				/* replace visuals temp key */ {
					const std::wstring find_aimbot_key = _ ( L"->Visuals->" );
					const auto find_aimbot_res = as_utf16.find ( find_aimbot_key );

					if ( find_aimbot_res != std::wstring::npos )
						as_utf16.replace ( find_aimbot_res, find_aimbot_key.size ( ), _ ( L"->C->" ) );
				}

				/* replace skins temp key */ {
					const std::wstring find_aimbot_key = _ ( L"->Skins->" );
					const auto find_aimbot_res = as_utf16.find ( find_aimbot_key );

					if ( find_aimbot_res != std::wstring::npos )
						as_utf16.replace ( find_aimbot_res, find_aimbot_key.size ( ), _ ( L"->D->" ) );
				}

				/* replace misc temp key */ {
					const std::wstring find_aimbot_key = _ ( L"->Misc->" );
					const auto find_aimbot_res = as_utf16.find ( find_aimbot_key );

					if ( find_aimbot_res != std::wstring::npos )
						as_utf16.replace ( find_aimbot_res, find_aimbot_key.size ( ), _ ( L"->E->" ) );
				}

				return menu::find_obj ( as_utf16, type );
			}

			static duk_ret_t get_checkbox ( duk_context* ctx ) {
				const auto obj_ptr = find_obj ( duk_require_string ( ctx, 0 ), oxui::object_checkbox );

				duk_get_global_string ( ctx, _ ( "ui_element" ) );
				duk_push_pointer ( ctx, obj_ptr ? obj_ptr : nullptr );
				duk_push_string ( ctx, obj_ptr ? _ ( "checkbox" ) : _ ( "" ) );
				duk_push_boolean ( ctx, false );
				duk_new ( ctx, 3 );

				return 1;
			}

			static duk_ret_t get_slider ( duk_context* ctx ) {
				const auto obj_ptr = find_obj ( duk_require_string ( ctx, 0 ), oxui::object_slider );

				duk_get_global_string ( ctx, _ ( "ui_element" ) );
				duk_push_pointer ( ctx, obj_ptr ? obj_ptr : nullptr );
				duk_push_string ( ctx, obj_ptr ? _ ( "slider" ) : _ ( "" ) );
				duk_push_boolean ( ctx, false );
				duk_new ( ctx, 3 );

				return 1;
			}

			static duk_ret_t get_dropdown ( duk_context* ctx ) {
				const auto obj_ptr = find_obj ( duk_require_string ( ctx, 0 ), oxui::object_dropdown );

				duk_get_global_string ( ctx, _ ( "ui_element" ) );
				duk_push_pointer ( ctx, obj_ptr ? obj_ptr : nullptr );
				duk_push_string ( ctx, obj_ptr ? _ ( "dropdown" ) : _ ( "" ) );
				duk_push_boolean ( ctx, false );
				duk_new ( ctx, 3 );

				return 1;
			}

			static duk_ret_t get_keybind ( duk_context* ctx ) {
				const auto obj_ptr = find_obj ( duk_require_string ( ctx, 0 ), oxui::object_keybind );

				duk_get_global_string ( ctx, _ ( "ui_element" ) );
				duk_push_pointer ( ctx, obj_ptr ? obj_ptr : nullptr );
				duk_push_string ( ctx, obj_ptr ? _ ( "keybind" ) : _ ( "" ) );
				duk_push_boolean ( ctx, false );
				duk_new ( ctx, 3 );

				return 1;
			}

			static duk_ret_t get_colorpicker ( duk_context* ctx ) {
				const auto obj_ptr = find_obj ( duk_require_string ( ctx, 0 ), oxui::object_colorpicker );

				duk_get_global_string ( ctx, _ ( "ui_element" ) );
				duk_push_pointer ( ctx, obj_ptr ? obj_ptr : nullptr );
				duk_push_string ( ctx, obj_ptr ? _ ( "colorpicker" ) : _ ( "" ) );
				duk_push_boolean ( ctx, false );
				duk_new ( ctx, 3 );

				return 1;
			}
		}
	}

	char* str_code = nullptr;

	static duk_ret_t unsafe_code ( duk_context* ctx, void* udata ) {
		duk_eval_string ( ctx, str_code );
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

duk_context* create_ctx ( ) {
	const auto ctx = duk_create_heap_default ( );

	/* console class */
	create_object ( console ); {
		/* console.log(args) */
		create_method ( console, bindings::console::log, "log", DUK_VARARGS );
		/* console.print(args) */
		create_method ( console, bindings::console::print, "print", DUK_VARARGS );
		/* console.error(args) */
		create_method ( console, bindings::console::error, "error", DUK_VARARGS );
		/* console.warning(args) */
		create_method ( console, bindings::console::warning, "warning", DUK_VARARGS );

		end_object ( console );
	}

	/* sesame class */
	create_object ( sesame ); {
		/* sesame.on_render(fn) */
		create_method ( sesame, bindings::sesame::on_render, "on_render", 1 );
		/* sesame.on_net_update(fn) */
		create_method ( sesame, bindings::sesame::on_net_update, "on_net_update", 1 );
		/* sesame.on_net_update_end(fn) */
		create_method ( sesame, bindings::sesame::on_net_update_end, "on_net_update_end", 1 );
		/* sesame.on_create_move(fn) */
		create_method ( sesame, bindings::sesame::on_create_move, "on_create_move", 1 );
		
		/* sesame.ui class */
		create_object ( ui ); {
			/* sesame.ui.keybind_hold */
			create_integer_constant ( ui, "keybind_hold", static_cast < int > ( oxui::keybind_mode::hold ) );
			/* sesame.ui.keybind_toggle */
			create_integer_constant ( ui, "keybind_toggle", static_cast < int > ( oxui::keybind_mode::toggle ) );
			/* sesame.ui.keybind_always */
			create_integer_constant ( ui, "keybind_always", static_cast < int > ( oxui::keybind_mode::always ) );

			/* sesame.ui.add_checkbox(name, value) */
			create_method ( ui, bindings::sesame::ui::add_checkbox, "add_checkbox", 2 );
			/* sesame.ui.add_slider(name, value, min, max) */
			create_method ( ui, bindings::sesame::ui::add_slider, "add_slider", 4 );
			/* sesame.ui.add_dropdown(name, value, arr_items) */
			create_method ( ui, bindings::sesame::ui::add_dropdown, "add_dropdown", 3 );
			/* sesame.ui.add_keybind(name, key, mode) */
			create_method ( ui, bindings::sesame::ui::add_keybind, "add_keybind", 3 );
			/* sesame.ui.add_colorpicker(name, clr) */
			create_method ( ui, bindings::sesame::ui::add_colorpicker, "add_colorpicker", 2 );

			/* sesame.ui.get_checkbox(name, value) */
			create_method ( ui, bindings::sesame::ui::get_checkbox, "get_checkbox", 1 );
			/* sesame.ui.get_slider(name, value, min, max) */
			create_method ( ui, bindings::sesame::ui::get_slider, "get_slider", 1 );
			/* sesame.ui.get_dropdown(name, value, arr_items) */
			create_method ( ui, bindings::sesame::ui::get_dropdown, "get_dropdown", 1 );
			/* sesame.ui.get_keybind(name, key, mode) */
			create_method ( ui, bindings::sesame::ui::get_keybind, "get_keybind", 1 );
			/* sesame.ui.get_colorpicker(name, clr) */
			create_method ( ui, bindings::sesame::ui::get_colorpicker, "get_colorpicker", 1 );

			///* sesame.ui.get_element(name, clr) */
			//create_method ( ui, bindings::sesame::ui::get_element, "get_element", 1 );

			end_subobject ( ui, sesame );
		}

		end_object ( sesame );
	}

	/* renderer class */
	create_object ( renderer ); {
		/* renderer.rect(x, y, w, h, clr, filled) */
		create_method ( renderer, bindings::renderer::rect, "rect", 6 );
		/* renderer.line(x1, y1, x2, y2, clr) */
		create_method ( renderer, bindings::renderer::line, "line", 5 );
		/* renderer.rounded_rect(x, y, w, h, rad, clr, filled) */
		create_method ( renderer, bindings::renderer::rounded_rect, "rounded_rect", 7 );
		/* renderer.circle(x, y, r, clr, filled) */
		create_method ( renderer, bindings::renderer::circle, "circle", 5 );
		/* var font = renderer.add_font(family, size, bold) */
		create_method ( renderer, bindings::renderer::add_font, "add_font", 3 );
		/* renderer.text(x, y, str, clr, font) */
		create_method ( renderer, bindings::renderer::text, "text", 5 );
		/* renderer.text_shadow(x, y, str, clr, font) */
		create_method ( renderer, bindings::renderer::text_shadow, "text_shadow", 5 );
		/* renderer.text_outline(x, y, str, clr, font) */
		create_method ( renderer, bindings::renderer::text_outline, "text_outline", 5 );

		end_object ( renderer );
	}
	
	/* globals class */
	create_object ( globals ); {
		/* globals.realtime() */
		create_method ( globals, bindings::globals::realtime, "realtime", 0 );
		/* globals.curtime() */
		create_method ( globals, bindings::globals::curtime, "curtime", 0 );
		/* globals.frametime() */
		create_method ( globals, bindings::globals::frametime, "frametime", 0 );
		/* globals.absframetime() */
		create_method ( globals, bindings::globals::absframetime, "absframetime", 0 );
		/* globals.max_players() */
		create_method ( globals, bindings::globals::max_players, "max_players", 0 );
		/* globals.tickcount() */
		create_method ( globals, bindings::globals::tickcount, "tickcount", 0 );
		/* globals.interval_per_tick() */
		create_method ( globals, bindings::globals::interval_per_tick, "interval_per_tick", 0 );
		/* globals.framecount() */
		create_method ( globals, bindings::globals::framecount, "framecount", 0 );

		end_object ( globals );
	}

	/* create new types */
	bindings::types::color::init ( ctx );
	bindings::types::ui_element::init ( ctx );

	return ctx;
}

void js::load ( const std::wstring& script ) {
	char u8_script_name [ 128 ] { '\0' };

	if ( WideCharToMultiByte ( CP_UTF8, 0, script.c_str ( ), -1, u8_script_name, 128, nullptr, nullptr ) <= 0 )
		return;

	const auto script_entry = script_ctx.find ( script );

	/* create script folder if it doesn't exist already */
	wchar_t appdata [ MAX_PATH ];

	if ( SUCCEEDED ( LI_FN ( SHGetFolderPathW )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
		LI_FN ( CreateDirectoryW )( ( std::wstring ( appdata ) + _ ( L"\\sesame" ) ).data ( ), nullptr );
		LI_FN ( CreateDirectoryW )( ( std::wstring ( appdata ) + _ ( L"\\sesame\\configs" ) ).data ( ), nullptr );
	}
	
	const auto path = std::wstring ( appdata ) + _ ( L"\\sesame\\js\\" ) + script + _ ( L".js" );
	const auto file = _wfopen ( path.data( ), _( L"rb" ) );

	/* create default options object if it doesnt exist already */
	const auto script_settings = cJSON_GetObjectItemCaseSensitive ( script_options, u8_script_name );
	if ( !script_settings ) {
		const auto new_script_settings = cJSON_CreateObject ( );
		cJSON_AddItemToObject ( new_script_settings, _ ( "Enabled" ), cJSON_CreateBool ( false ) );
		cJSON_AddItemToObject ( script_options, u8_script_name, new_script_settings );
	}

	/* parse file and compile script to increase execution speed */
	if ( file ) {
		fseek ( file, 0, SEEK_END );
		const auto size = ftell ( file );
		rewind ( file );

		const auto buffer = ( char* ) malloc ( size );
		const auto len = fread ( buffer, 1, size, file );
		const auto ctx = create_ctx ( );

		/* add script id flag */
		duk_push_string ( ctx, u8_script_name );
		duk_put_global_string ( ctx, _ ( "_script_id" ) );

		bindings::str_code = buffer;
		
		if ( script_entry != script_ctx.end ( ) ) {
			if ( script_ctx [ script ] )
				duk_destroy_heap ( script_ctx [ script ] );

			script_ctx [ script ] = ctx;

			log_console ( { 0xd8, 0x50, 0xd4, 255 }, _ ( "[ sesame ] " ) );
			log_console ( { 255, 255, 54, 255 }, ( char* ) _ ( u8"Script \"%s\" is already loaded. Reloading...\n" ), u8_script_name );
		}
		else {
			script_ctx.emplace ( script, ctx );

			log_console ( { 0xd8, 0x50, 0xd4, 255 }, _ ( "[ sesame ] " ) );
			log_console ( { 50, 255, 50, 255 }, ( char* ) _ ( u8"Loaded script \"%s\".\n" ), u8_script_name );
		}

		if ( duk_safe_call ( ctx, bindings::unsafe_code, nullptr, 0, 1 ) ) {
			log_console ( { 0xd8, 0x50, 0xd4, 255 }, _ ( "[ sesame ] " ) );
			log_console ( { 255, 50, 50, 255 }, ( char* ) _ ( u8"Error running \"%s\" script: %s\n" ), u8_script_name, duk_safe_to_string ( ctx, -1 ) );
		}

		fclose ( file );
		free ( buffer );
	}
}

void js::init ( ) {
	/* load global script options */
	load_script_options ( );

	END_FUNC
}

void js::reset ( ) {
	log_console ( { 0xd8, 0x50, 0xd4, 255 }, _ ( "[ sesame ] " ) );
	log_console ( { 50, 255, 50, 255 }, ( char* ) _ ( u8"Reloading all scripts...\n" ) );

	for ( auto& script : script_ctx ) {
		if ( script.second )
			duk_destroy_heap ( script.second );
	}

	script_ctx.clear ( );
}