#pragma once
#include <unordered_map>

namespace options {
	enum class option_type_t : int {
		boolean = 0,
		list,
		integer,
		floating_point,
		string,
		color,
		skin
	};

	class option {
	public:
		struct colorf {
			float r, g, b, a;
		};

		struct skin_t {
			int paintkit, seed, stattrak;
			float wear;
			bool is_stattrak;
			char nametag [ 32 ];
		};

		option_type_t type;

		union val {
			bool b;
			int i;
			float f;
			char s [ 128 ];
			bool l [ 128 ];
			colorf c;
			skin_t skin;

			val ( ) {}
			~val ( ) {}
		} val;

		int list_size = 0;

		static void add_list ( const std::string& id, int count );
		static void add_bool ( const std::string& id, bool val );
		static void add_int ( const std::string& id, int val );
		static void add_float ( const std::string& id, float val );
		static void add_str ( const std::string& id, const char* val );
		static void add_color ( const std::string& id, const colorf& val );

		static void add_skin ( const std::string& id );
		static void add_skin_int ( const std::string& id, int val );
		static void add_skin_bool ( const std::string& id, bool val );

		static void add_script_list ( const std::string& id, int count );
		static void add_script_bool ( const std::string& id, bool val );
		static void add_script_int ( const std::string& id, int val );
		static void add_script_float ( const std::string& id, float val );
		static void add_script_str ( const std::string& id, const char* val );
		static void add_script_color ( const std::string& id, const colorf& val );
	};

	inline std::unordered_map< std::string, option > skin_vars;
	inline std::unordered_map< std::string, option > vars;
	inline std::unordered_map< std::string, option > script_vars;

	void save ( const std::unordered_map< std::string, option >& options, const std::string& path );
	void load ( std::unordered_map< std::string, option >& options, const std::string& path );
	void init_skins ( );
	void init ( );
}