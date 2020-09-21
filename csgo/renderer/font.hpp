#pragma once
#include <cstdint>
#include <string_view>
#include <functional>
#include <memory>
#include <optional>
#include <d3d9.h>
#include <d3dx9.h>
#include "../sdk/sdk.hpp"

namespace font {
	struct glyph_vec {
		float x, y;
	};

	struct glyph_info {
		glyph_vec size;
		glyph_vec bearing;
		uintptr_t advance;
		IDirect3DTexture9* texture;
		bool colored;
	};

	struct font {
		std::string name;
		int size;
		IDirect3DVertexBuffer9* vertex_buffer;
		std::map<int, glyph_info> glyph_map;

		void on_lost_device ( ) {
			if ( vertex_buffer )
				vertex_buffer->Release ( );

			vertex_buffer = nullptr;
		}

		void on_reset_device ( ) {
			if (!vertex_buffer )
				csgo::i::dev->CreateVertexBuffer ( sizeof ( font_vertex_t ) * 12, NULL, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DPOOL_MANAGED, &vertex_buffer, nullptr );
		}
	};

	struct font_vertex_t {
		float x, y, z, rhw;
		uint32_t color;
		float tx, ty;
	};

	std::optional<std::unique_ptr<font>> add_font_from_memory( const std::vector<uint8_t>& data, std::string_view name, int size );
}