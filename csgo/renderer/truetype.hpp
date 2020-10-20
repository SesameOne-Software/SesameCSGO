#pragma once

#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_LARGE_RECTS
#include "stb_rect_pack.h"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <map>
#include <optional>
#include <memory>

namespace truetype {
    enum class text_flags_t {
        text_flags_none = 0,
        text_flags_dropshadow,
        text_flags_outline
    };

    struct font {
        uint32_t id;
        std::string font_name;
        float size;
        stbtt_fontinfo font_info;
        stbtt_pack_context pc;
        stbtt_packedchar* chars;
        uint8_t* font_map;
        unsigned int bitmap_width;
        unsigned int bitmap_height;

        void operator=( const font& f ) {
            font_name = f.font_name;
            size = f.size;
            font_info = f.font_info;
            pc = f.pc;
            chars = f.chars;
            font_map = f.font_map;
            id = f.id;
        }

        void create_texture( );
        void set_font_size( float x );
        void text_size( const std::string& string, float& x_out, float& y_out );
        void draw_text( float x, float y, const std::string& string, uint32_t color, text_flags_t flags );
        void draw_atlas( float x, float y );
    };

    std::optional<font> create_font( const uint8_t* font_data, std::string_view font_name, float size, bool extended_range = false );
    void begin( );
    void end( );
} // namespace truetype