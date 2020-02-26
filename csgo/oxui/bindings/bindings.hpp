#ifndef OXUI_BINDINGS_HPP
#define OXUI_BINDINGS_HPP

#include <memory>
#include <string_view>
#include <functional>
#include <d3d9.h>
#include <d3dx9.h>
#include <map>
#include "../types/types.hpp"
#include "../../renderer/d3d9.hpp"
#include "../../sdk/sdk.hpp"

namespace oxui {
	namespace binds {
		static std::add_pointer_t< void( const oxui::rect&, std::function< void( ) > ) > clip = [ ] ( const oxui::rect& area, std::function< void( ) > func ) {
			RECT backup_scissor_rect;
			csgo::i::dev->GetScissorRect( &backup_scissor_rect );

			RECT rect { area.x - 0.5f, area.y - 0.5f, area.x + area.w - 0.5f, area.y + area.h - 0.5f };
			csgo::i::dev->SetRenderState( D3DRS_SCISSORTESTENABLE, true );
			csgo::i::dev->SetScissorRect( &rect );

			func( );

			csgo::i::dev->SetScissorRect( &backup_scissor_rect );
			csgo::i::dev->SetRenderState( D3DRS_SCISSORTESTENABLE, false );
		};

		static std::add_pointer_t< void( const str&, int, bool, font& ) > create_font = [ ] ( const str& font_name, int size, bool bold, font& fout ) {
			font font_out;
			render::create_font( (void**) &font_out, font_name, size, bold );
			fout = font_out;
		};

		static std::add_pointer_t< void( oxui::rect& ) > screen_size = [ ] ( oxui::rect& p ) {
			int w, h;
			render::screen_size( w, h ); p = oxui::rect( 0, 0, w, h );
		};

		static std::add_pointer_t< void( pos& ) > mouse_pos = [ ] ( oxui::pos& p ) {
			render::pos pos;
			render::mouse_pos( pos );
			p.x = pos.x;
			p.y = pos.y;
		};

		static std::add_pointer_t< void( const oxui::rect&, const color& ) > rect = [ ] ( const oxui::rect& r, const oxui::color& c ) {
			render::outline( r.x, r.y, r.w, r.h, D3DCOLOR_RGBA( c.r, c.g, c.b, c.a ) );
		};

		static std::add_pointer_t< void( const oxui::rect&, const color& ) > fill_rect = [ ] ( const oxui::rect& r, const oxui::color& c ) {
			render::rectangle( r.x, r.y, r.w, r.h, D3DCOLOR_RGBA( c.r, c.g, c.b, c.a ) );
		};

		static std::add_pointer_t< void( const oxui::rect&, const color&, const color&, bool ) > gradient_rect = [ ] ( const oxui::rect& r, const oxui::color& c, const oxui::color& c1, bool horizontal ) {
			render::gradient( r.x, r.y, r.w, r.h, D3DCOLOR_RGBA( c.r, c.g, c.b, c.a ), D3DCOLOR_RGBA( c1.r, c1.g, c1.b, c1.a ), horizontal );
		};
		
		static std::add_pointer_t< void( const oxui::pos&, const oxui::pos&, const color& ) > line = [ ] ( const oxui::pos& p1, const oxui::pos& p2, const color& c ) {
			render::line( p1.x, p1.y, p2.x, p2.y, D3DCOLOR_RGBA( c.r, c.g, c.b, c.a ) );
		};

		static std::add_pointer_t< void( const font&, const str&, oxui::rect& ) > text_bounds = [ ] ( const font& font, const str& text, oxui::rect& bounds ) {
			render::dim size;
			render::text_size( (void*)font, text, size );
			bounds.w = size.w;
			bounds.h = size.h;
		};

		static std::add_pointer_t< void( const pos&, const font&, const str&, const color&, bool ) > text = [ ] ( const pos& point, const font& font, const str& text, const color& c, bool shadow ) {
			if ( shadow && c.a == 255 )
				render::text( point.x + 1, point.y + 1, D3DCOLOR_RGBA( 0, 0, 0, 255 ), ( void* ) font, text );

			render::text( point.x, point.y, D3DCOLOR_RGBA( c.r, c.g, c.b, c.a ), ( void* ) font, text );
		};
	}
}

#endif // OXUI_BINDINGS_HPP