#pragma once
#include <vector>
#include <utility>
#include "../security/security_handler.hpp"

namespace threads {
	using thread_t = void*;
	using thread_fn_t = unsigned ( __cdecl* )( void* );

	inline thread_t create_thread ( thread_fn_t start_addr, void* arg ) {
		using create_simple_thread_t = thread_t ( __cdecl* )( thread_fn_t, void*, size_t );
		static auto create_simple_thread = reinterpret_cast< create_simple_thread_t >( GetProcAddress ( GetModuleHandleA ( _ ( "tier0.dll" ) ), _ ( "CreateSimpleThread" ) ) );
		return create_simple_thread ( start_addr, arg, 0 );
	}

	inline bool release_thread ( thread_t thread ) {
		using release_thread_handle_t = bool ( __cdecl* )( thread_t );
		static auto release_thread_handle = reinterpret_cast< release_thread_handle_t >( GetProcAddress ( GetModuleHandleA ( _ ( "tier0.dll" ) ), _ ( "ReleaseThreadHandle" ) ) );
		return release_thread_handle ( thread );
	}

	inline bool join_thread ( thread_t thread, unsigned timeout = 0xFFFFFFFF ) {
		using thread_join_t = bool ( __cdecl* )( thread_t, unsigned );
		static auto thread_join = reinterpret_cast< thread_join_t >( GetProcAddress ( GetModuleHandleA ( _ ( "tier0.dll" ) ), _ ( "ThreadJoin" ) ) );
		return thread_join ( thread, timeout );
	}
	
	template < typename thread_arg_t >
	class thread_mgr_t {
		std::vector< std::pair< thread_t, thread_arg_t* > > threads_and_data { };

		inline void release_all ( ) {
			for ( auto& thread : threads_and_data ) {
				release_thread ( thread.first );
				delete thread.second;
			}

			if ( !threads_and_data.empty ( ) )
				threads_and_data.clear ( );
		}

	public:
		inline ~thread_mgr_t ( ) {
			release_all ( );
		}

		template < typename custom_thread_fn_t >
		inline void create_thread ( custom_thread_fn_t fn, const thread_arg_t& arg ) {
			thread_arg_t* thread_data = new thread_arg_t { arg };
			threads_and_data.push_back ( { threads::create_thread ( reinterpret_cast< thread_fn_t  >( fn ), thread_data ), thread_data } );
		}

		inline void join_all ( ) {
			for ( auto& thread : threads_and_data )
				join_thread ( thread.first );
		}

		inline std::vector< thread_arg_t* > get_data ( ) {
			std::vector< thread_arg_t* > ret { };

			for ( auto& thread : threads_and_data )
				ret.push_back ( thread.second );

			return ret;
		}

		inline void reset ( ) {
			release_all ( );
		}
	};
}