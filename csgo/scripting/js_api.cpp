#include "js_api.hpp"

#include "../sdk/sdk.hpp"
#include "../v7/v7.h"

#include "../menu/options.hpp"

#include <ShlObj.h>

v7* v7_ctx = nullptr;

template < typename ...args_t >
void print_console ( const options::option::colorf& clr, const char* fmt, args_t ...args ) {
	if ( !fmt )
		return;

	struct {
		uint8_t r, g, b, a;
	} s_clr;

	s_clr = { static_cast < uint8_t > ( clr.r * 255.0f ), static_cast < uint8_t > ( clr.g * 255.0f ), static_cast < uint8_t > ( clr.b * 255.0f ), static_cast < uint8_t > ( clr.a * 255.0f ) };

	static auto con_color_msg = reinterpret_cast< void ( * )( const decltype( s_clr )&, const char*, ... ) >( LI_FN ( GetProcAddress ) ( LI_FN ( GetModuleHandleA ) ( _ ( "tier0.dll" ) ), _ ( "?ConColorMsg@@YAXABVColor@@PBDZZ" ) ) );

	con_color_msg ( s_clr, fmt, args... );
}

namespace bindings {
	namespace util {
		v7_val_t util_obj = 0;

		v7_err clear ( v7* v7, v7_val_t* res ) {
			cs::i::engine->client_cmd_unrestricted ( _ ( "clear" ) );
			return v7_err::V7_OK;
		}

		v7_err play_sound ( v7* v7, v7_val_t* res ) {
			auto arg0 = v7_arg ( v7, 0 );

			if ( v7_is_string ( arg0 ) ) {
				size_t str_len = 0;
				const auto str = v7_get_string ( v7, &arg0, &str_len );
				cs::i::engine->client_cmd_unrestricted ( std::string( _ ( "play " ) ).append( str ).c_str() );
				return v7_err::V7_OK;
			}

			return v7_err::V7_SYNTAX_ERROR;
		}

		v7_err print ( v7* v7, v7_val_t* res ) {
			auto arg0 = v7_arg ( v7, 0 );

			if ( v7_is_string ( arg0 ) ) {
				size_t str_len = 0;
				const auto str = v7_get_string ( v7, &arg0, &str_len );

				dbg_print ( str );
				return v7_err::V7_OK;
			}

			return v7_err::V7_SYNTAX_ERROR;
		}

		v7_err println ( v7* v7, v7_val_t* res ) {
			auto arg0 = v7_arg ( v7, 0 );

			if ( v7_is_string ( arg0 ) ) {
				size_t str_len = 0;
				const auto str = v7_get_string ( v7, &arg0, &str_len );

				dbg_print ( str );
				dbg_print ( _("\n") );
				return v7_err::V7_OK;
			}

			return v7_err::V7_SYNTAX_ERROR;
		}
	}
	
	namespace player {
		v7_err ctor ( v7* v7, v7_val_t* res ) {
			v7_val_t this_obj = v7_get_this ( v7 );
			v7_val_t arg0 = v7_arg ( v7, 0 );
			
			v7_def ( v7, this_obj, _("__idx"), ~0 /* = strlen */, V7_DESC_ENUMERABLE ( 0 ), arg0 );

			return v7_err::V7_OK;
		}

		v7_err is_valid ( v7* v7, v7_val_t* res ) {
			v7_val_t this_obj = v7_get_this ( v7 );

			const auto ent = cs::i::ent_list->get<player_t*> ( v7_get_int ( v7, v7_get ( v7, this_obj, _ ( "__idx" ), ~0 ) ) );
			*res = v7_mk_boolean ( v7, ent && ent->is_player ( ) );

			return v7_err::V7_OK;
		}

		v7_err is_alive ( v7* v7, v7_val_t* res ) {
			v7_val_t this_obj = v7_get_this ( v7 );

			const auto ent = cs::i::ent_list->get<player_t*> ( v7_get_int(v7, v7_get ( v7, this_obj, _ ( "__idx" ), ~0 )) );
			*res = v7_mk_boolean ( v7, ent && ent->is_player() && ent->alive() );

			return v7_err::V7_OK;
		}

		v7_err get_health ( v7* v7, v7_val_t* res ) {
			v7_val_t this_obj = v7_get_this ( v7 );

			const auto ent = cs::i::ent_list->get<player_t*> ( v7_get_int ( v7, v7_get ( v7, this_obj, _ ( "__idx" ), ~0 ) ) );
			*res = v7_mk_number ( v7, ( ent && ent->is_player ( ) ) ? ent->health ( ) : 0 );

			return v7_err::V7_OK;
		}
	}
}

void js_api::load_script ( std::string_view file_name ) {
	char appdata [ MAX_PATH ];

	if ( SUCCEEDED ( LI_FN ( SHGetFolderPathA )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
		LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame" ) ).c_str ( ), nullptr );
		LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame\\configs" ) ).c_str ( ), nullptr );
		LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame\\scripts" ) ).c_str ( ), nullptr );
	}

	v7_val_t eval_result;
	const auto rcode = v7_exec_file ( v7_ctx, std::string( appdata ).append( _ ( "\\sesame\\scripts\\" ) ).append( file_name ).c_str(), &eval_result );

	if ( rcode != v7_err::V7_OK ) {
		v7_val_t msg;
		if ( v7_is_undefined ( eval_result ) )
			return;

		msg = v7_get ( v7_ctx, eval_result, _ ( "message" ), ~0 );

		if ( v7_is_undefined ( msg ) )
			msg = eval_result;

		/* print error */
		char buf [ 16 ];
		char* s = v7_stringify ( v7_ctx, msg, buf, sizeof ( buf ), v7_stringify_mode::V7_STRINGIFY_DEBUG );

		print_console ( { 0.85f, 0.31f, 0.83f, 1.0f }, _ ( "sesame" ) );
		print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( " ～ " ) );
		print_console ( { 1.0f, 0.34f, 0.34f, 1.0f }, _ ( "error executing %s: %s\n" ), file_name.data ( ), s );

		if ( buf != s )
			free ( s );

		/* print call stack */
		v7_val_t strace_v = v7_get ( v7_ctx, msg, _ ( "stack" ), ~0 );
		const char* strace = NULL;
		if ( v7_is_string ( strace_v ) ) {
			size_t s = 0;
			strace = v7_get_string ( v7_ctx, &strace_v, &s );
			print_console ( { 1.0f, 0.34f, 0.34f, 1.0f }, _ ( "-- CALL STACK --\n" ) );
			print_console ( { 1.0f, 0.34f, 0.34f, 1.0f }, _ ( "%s\n" ), strace );
		}

		return;
	}

	print_console ( { 0.85f, 0.31f, 0.83f, 1.0f }, _ ( "sesame" ) );
	print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( " ～ " ) );
	print_console ( { 0.678f, 1.0f, 0.996f, 1.0f }, _ ( "executed %s\n" ), file_name.data ( ) );
}

void js_api::init ( ) {
	VMP_BEGINULTRA ( );
	/* destroy and reload context if it already exists */
	if ( v7_ctx ) {
		v7_destroy ( v7_ctx );
		v7_ctx = nullptr;
	}

	/* init */
	v7_ctx = v7_create ( );
	
	/* utils */ {
		bindings::util::util_obj = v7_mk_object ( v7_ctx );
		v7_set_method ( v7_ctx, bindings::util::util_obj, _ ( "clear" ), bindings::util::clear );
		v7_set_method ( v7_ctx, bindings::util::util_obj, _ ( "play_sound" ), bindings::util::play_sound );
		v7_set_method ( v7_ctx, bindings::util::util_obj, _ ( "print" ), bindings::util::print );
		v7_set_method ( v7_ctx, bindings::util::util_obj, _ ( "println" ), bindings::util::println );
		v7_set ( v7_ctx, v7_get_global ( v7_ctx ), _ ( "utils" ), ~0, bindings::util::util_obj );
	}

	/* player */ {
		const auto player_obj = v7_mk_object ( v7_ctx );
		const auto ctor_func = v7_mk_function_with_proto ( v7_ctx, bindings::player::ctor, player_obj );
		v7_set_method ( v7_ctx, player_obj, _ ( "is_valid" ), bindings::player::is_valid );
		v7_set_method ( v7_ctx, player_obj, _ ( "is_alive" ), bindings::player::is_alive );
		v7_set_method ( v7_ctx, player_obj, _ ( "get_health" ), bindings::player::get_health );
		v7_set ( v7_ctx, v7_get_global ( v7_ctx ), _("player"), ~0, ctor_func );
	}
	VMP_END ( );
	END_FUNC;
}