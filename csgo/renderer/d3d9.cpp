#include "d3d9.hpp"
#include "../sdk/sdk.hpp"
#include "../security/security_handler.hpp"

struct vtx_t {
	float x, y, z, rhw;
	std::uint32_t color;
};

struct custom_vtx_t {
	float x, y, z, rhw;
	std::uint32_t color;
	float tu, tv;
};

void render::create_font( void** font, const std::wstring_view& family, int size, bool bold ) {
	ID3DXFont* d3d_font = nullptr;
	LI_FN( D3DXCreateFontW )( csgo::i::dev, size, 0, bold ? FW_BOLD : FW_NORMAL, 0, false, OEM_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, family.data( ), &d3d_font );
	*font = d3d_font;
}

void render::screen_size( int& width, int& height ) {
	csgo::i::engine->get_screen_size( width, height );
}

void render::text_size( void* font, const std::wstring_view& text, dim& dimentions ) {
	RECT rect = { 0, 0, 0, 0 };
	reinterpret_cast< ID3DXFont* >( font )->DrawTextW( nullptr, text.data( ), text.length( ), &rect, DT_CALCRECT, D3DCOLOR_RGBA( 0, 0, 0, 0 ) );
	dimentions = dim { rect.right - rect.left, rect.bottom - rect.top };
}

void render::rectangle( int x, int y, int width, int height, std::uint32_t color ) {
	vtx_t vert [ 4 ] = {
		{ x - 0.5f, y - 0.5f, 0.0f, 1.0f, color },
		{ x + width - 0.5f, y - 0.5f, 0.0f, 1.0f, color },
		{ x - 0.5f, y + height - 0.5f, 0.0f, 1.0f, color },
		{ x + width - 0.5f, y + height - 0.5f, 0.0f, 1.0f, color }
	};

	csgo::i::dev->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, &vert, sizeof( vtx_t ) );
}

void render::gradient( int x, int y, int width, int height, std::uint32_t color1, std::uint32_t color2, bool is_horizontal ) {
	const auto d3d_clr1 = color1;
	const auto d3d_clr2 = color2;
	std::uint32_t c1, c2, c3, c4;

	if ( is_horizontal ) {
		c1 = d3d_clr1;
		c2 = d3d_clr2;
		c3 = d3d_clr1;
		c4 = d3d_clr2;
	}
	else {
		c1 = d3d_clr1;
		c2 = d3d_clr1;
		c3 = d3d_clr2;
		c4 = d3d_clr2;
	}

	vtx_t vert [ 4 ] = {
		{ x, y, 0.0f, 1.0f, c1 },
		{ x + width, y, 0.0f, 1.0f, c2 },
		{ x, y + height, 0.0f, 1.0f, c3 },
		{ x + width, y + height, 0.0f, 1.0f, c4 }
	};

	csgo::i::dev->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, &vert, sizeof( vtx_t ) );
}

void render::outline( int x, int y, int width, int height, std::uint32_t color ) {
	rectangle( x, y, width, 1, color );
	rectangle( x, y + height, width, 1, color );
	rectangle( x, y, 1, height, color );
	rectangle( x + width, y, 1, height + 1, color );
}

void render::line( int x, int y, int x2, int y2, std::uint32_t color ) {
	vtx_t vert [ 2 ] = {
		{ x - 0.5f, y - 0.5f, 0.0f, 1.0f, color },
		{ x2 - 0.5f, y2 - 0.5f, 0.0f, 1.0f, color }
	};

	csgo::i::dev->DrawPrimitiveUP( D3DPT_LINELIST, 1, &vert, sizeof( vtx_t ) );
}

void render::text( int x, int y, std::uint32_t color, void* font, const std::wstring_view& text, bool shadow, bool outline ) {
	RECT rect;
	SetRect ( &rect, x - 0.5f, y - 0.5f, x - 0.5f, y - 0.5f );
	
	if ( shadow ) {
		SetRect ( &rect, x - 0.5f + 1, y - 0.5f + 1, x - 0.5f + 1, y - 0.5f + 1 );
		reinterpret_cast< ID3DXFont* >( font )->DrawTextW ( nullptr, text.data ( ), text.length ( ), &rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
	}
	else if ( outline ) {
		SetRect ( &rect, x - 0.5f, y - 0.5f + 1, x - 0.5f, y - 0.5f + 1 );
		reinterpret_cast< ID3DXFont* >( font )->DrawTextW ( nullptr, text.data ( ), text.length ( ), &rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
		SetRect ( &rect, x - 0.5f + 1, y - 0.5f, x - 0.5f + 1, y - 0.5f );
		reinterpret_cast< ID3DXFont* >( font )->DrawTextW ( nullptr, text.data ( ), text.length ( ), &rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
		SetRect ( &rect, x - 0.5f - 1, y - 0.5f, x - 0.5f - 1, y - 0.5f );
		reinterpret_cast< ID3DXFont* >( font )->DrawTextW ( nullptr, text.data ( ), text.length ( ), &rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
		SetRect ( &rect, x - 0.5f, y - 0.5f - 1, x - 0.5f, y - 0.5f - 1 );
		reinterpret_cast< ID3DXFont* >( font )->DrawTextW ( nullptr, text.data ( ), text.length ( ), &rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
	}

	SetRect ( &rect, x - 0.5f, y - 0.5f, x - 0.5f, y - 0.5f );
	reinterpret_cast< ID3DXFont* >( font )->DrawTextW ( nullptr, text.data ( ), text.length ( ), &rect, DT_LEFT | DT_NOCLIP, color );
}

void render::circle( int x, int y, int radius, int segments, std::uint32_t color, int fraction, float rotation, bool outline ) {
	std::vector< vtx_t > circle( segments + 2 );

	auto pi = D3DX_PI;
	const auto angle = rotation * D3DX_PI / 180.0f;

	if ( fraction == 0 ) pi = D3DX_PI;     
	if ( fraction == 2 ) pi = D3DX_PI / 2.0f; 
	if ( fraction == 4 ) pi = D3DX_PI / 4.0f; 

	if ( !outline ) {
		circle [ 0 ].x = static_cast< float > ( x ) - 0.5f;
		circle [ 0 ].y = static_cast< float > ( y ) - 0.5f;
		circle [ 0 ].z = 0;
		circle [ 0 ].rhw = 1;
		circle [ 0 ].color = color;
	}

	for ( auto i = outline ? 0 : 1; i < segments + 2; i++ ) {
		circle [ i ].x = ( float ) ( x - radius * std::cosf( pi * ( ( i - 1 ) / ( segments / 2.0f ) ) ) ) - 0.5f;
		circle [ i ].y = ( float ) ( y - radius * std::sinf( pi * ( ( i - 1 ) / ( segments / 2.0f ) ) ) ) - 0.5f;
		circle [ i ].z = 0;
		circle [ i ].rhw = 1;
		circle [ i ].color = color;
	}

	const auto _res = segments + 2;

	for ( auto i = 0; i < _res; i++ ) {
		circle [ i ].x = x + std::cosf( angle ) * ( circle [ i ].x - x ) - std::sinf( angle ) * ( circle [ i ].y - y ) - 0.5f;
		circle [ i ].y = y + std::sinf( angle ) * ( circle [ i ].x - x ) + std::cosf( angle ) * ( circle [ i ].y - y ) - 0.5f;
	}

	IDirect3DVertexBuffer9* vb = nullptr;

	csgo::i::dev->CreateVertexBuffer( ( segments + 2 ) * sizeof( vtx_t ), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vb, nullptr );

	void* verticies;
	vb->Lock( 0, ( segments + 2 ) * sizeof( vtx_t ), ( void** ) &verticies, 0 );
	std::memcpy( verticies, &circle [ 0 ], ( segments + 2 ) * sizeof( vtx_t ) );
	vb->Unlock( );

	csgo::i::dev->SetStreamSource( 0, vb, 0, sizeof( vtx_t ) );
	csgo::i::dev->DrawPrimitive( outline ? D3DPT_LINESTRIP : D3DPT_TRIANGLEFAN, 0, segments );

	if ( vb )
		vb->Release( );
}

void render::texture( ID3DXSprite* sprite, IDirect3DTexture9* tex, int x, int y, int width, int height, float scalex, float scaley, uint32_t clr ) {
	D3DXVECTOR2 center = D3DXVECTOR2( width / 2, height / 2 );
	D3DXVECTOR2 trans = D3DXVECTOR2( x, y );
	D3DXMATRIX mat;
	D3DXVECTOR2 scaling( scalex, scaley );

	D3DXMatrixTransformation2D( &mat, nullptr, 0.0f, &scaling, &center, 0.0f, &trans );

	csgo::i::dev->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	csgo::i::dev->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	csgo::i::dev->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );

	sprite->Begin( 0 );
	sprite->SetTransform( &mat );
	sprite->Draw( tex, nullptr, nullptr, nullptr, clr );
	sprite->End( );
}

void render::polygon( const std::vector< std::pair< float, float > >& verticies, std::uint32_t color, bool outline ) {
	std::vector< vtx_t > vtx( verticies.size( ) + 1 );

	for ( auto i = 0; i < verticies.size( ); i++ ) {
		vtx [ i ].x = verticies [ i ].first;
		vtx [ i ].y = verticies [ i ].second;
		vtx [ i ].z = 0;
		vtx [ i ].rhw = 1;
		vtx [ i ].color = color;
	}

	vtx [ verticies.size( ) ] = vtx [ 0 ];

	IDirect3DVertexBuffer9* vb = nullptr;

	csgo::i::dev->CreateVertexBuffer( ( verticies.size ( ) + 1 ) * sizeof( vtx_t ), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vb, nullptr );

	void* verticies1;
	vb->Lock( 0, ( verticies.size ( ) + 1 ) * sizeof( vtx_t ), ( void** ) &verticies1, 0 );
	std::memcpy( verticies1, &vtx [ 0 ], ( verticies.size ( ) + 1 ) * sizeof( vtx_t ) );
	vb->Unlock( );

	csgo::i::dev->SetStreamSource( 0, vb, 0, sizeof( vtx_t ) );
	csgo::i::dev->DrawPrimitive( outline ? D3DPT_LINESTRIP : D3DPT_TRIANGLEFAN, 0, verticies.size ( ) );

	if ( vb )
		vb->Release( );
}

//void render::rounded_rect( int x, int y, int w, int h, int radius, int vertices, std::uint32_t col ) {
//	if ( vertices < 2 )
//		return;
//
//	std::vector< vtx_t > vtx( 4 * vertices );
//
//	for ( auto i = 0; i < 4; i++ ) {
//		auto _x = x + ( ( i < 2 ) ? ( w - radius ) : radius );
//		auto _y = y + ( ( i % 3 ) ? ( h - radius ) : radius );
//
//		auto a = 90.f * i;
//
//		for ( auto j = 0; j < vertices; j++ ) {
//			auto _a = csgo::deg2rad( a + ( j / ( float ) ( vertices - 1 ) ) * 90.0f );
//			vtx [ ( i * vertices ) + j ] = vtx_t { _x + radius * std::sinf( _a ), _y - radius * std::cosf( _a ), 0, 1, col };
//		}
//	}
//
//	polygon( 4 * vertices, round, col );
//}

void render::clip( int x, int y, int width, int height, const std::function< void ( ) >& func ) {
	RECT backup_scissor_rect;
	csgo::i::dev->GetScissorRect ( &backup_scissor_rect );

	RECT rect { x, y, x + width, y + height };
	csgo::i::dev->SetRenderState ( D3DRS_SCISSORTESTENABLE, true );
	csgo::i::dev->SetScissorRect ( &rect );

	func ( );

	csgo::i::dev->SetScissorRect ( &backup_scissor_rect );
	csgo::i::dev->SetRenderState ( D3DRS_SCISSORTESTENABLE, false );
}

bool render::key_pressed( const std::uint32_t key ) {
	return utils::key_state ( key );
}

void render::mouse_pos( pos& position ) {
	int x, y;
	csgo::i::surface->get_cursor_pos( x, y );
	position = pos { x, y };
}

void render::circle3d ( const vec3_t& pos, int rad, int segments, std::uint32_t color, bool outline ) {
	std::vector< std::pair< float, float > > points { };
	
	auto src_point = pos;
	src_point.y += static_cast< float > ( rad );

	auto rotate_point = [ ] ( vec3_t pivot, vec3_t point, float ang ) -> vec3_t {
		const auto s = std::sinf ( ang );
		const auto c = std::cosf ( ang );

		point.x -= pivot.x;
		point.y -= pivot.y;

		const auto new_x = point.x * c - point.y * s;
		const auto new_y = point.x * s + point.y * c;

		point.x = new_x + pivot.x;
		point.y = new_y + pivot.y;

		return point;
	};

	auto segment_num = 0;

	for ( auto i = 0.0f; i < csgo::pi * 2.0f; i += ( csgo::pi * 2.0f ) / static_cast< float > ( segments ) ) {
		auto new_point = rotate_point ( pos, src_point, i );
		vec3_t screen;

		csgo::render::world_to_screen ( screen, new_point );

		points.push_back ( { screen.x - 0.5f, screen.y - 0.5f } );
	}

	polygon ( points, color, outline );
}

void render::gradient_rounded_rect ( int x, int y, int width, int height, int rad, int resolution, std::uint32_t color, std::uint32_t color1, bool outline ) {
	if ( resolution < 2 )
		return;

	circle ( x + width - rad, y + rad, rad, resolution, color, 4, 90.0f );

	//gradient ( x, y + rad, width, height - rad * 2, color, color1, false );
}

void render::rounded_alpha_rect ( int x, int y, int width, int height, int rad, int resolution ) {
	if ( resolution < 2 )
		return;

	std::vector< std::pair< std::pair< float, float >, uint32_t > > verticies ( 4 * resolution + 4 );

	for ( auto i = 0; i < 4; i++ ) {
		const auto _x = x + ( ( i < 2 ) ? ( width - rad ) : rad );
		const auto _y = y + ( ( i % 3 ) ? ( height - rad ) : rad );
		const auto a = 90.0f * i;

		for ( auto j = 0; j < resolution; j++ ) {
			const auto _a = csgo::deg2rad ( a + ( j / ( float ) ( resolution - 1 ) ) * 90.0f );
			verticies [ i * resolution + j ].first = { _x + rad * std::sinf ( _a ) - 0.5f, _y - rad * std::cosf ( _a ) - 0.5f };
			verticies [ i * resolution + j ].second = ( i % 2 ) ? D3DCOLOR_RGBA ( 255, 255, 255, 255 ) : D3DCOLOR_RGBA ( 112, 112, 112, 255 );
		}

		verticies [ i * resolution + resolution ].first = { ( x + width / 2 ) + width / 2 * std::sinf ( a ) - 0.5f, ( y + height / 2 ) - height / 2 * std::cosf ( a ) - 0.5f };
		verticies [ i * resolution + resolution ].second = ( i % 2 ) ? D3DCOLOR_RGBA ( 255, 255, 255, 255 ) : D3DCOLOR_RGBA ( 112, 112, 112, 255 );
	}

	std::vector< vtx_t > vtx ( verticies.size ( ) + 1 );

	for ( auto i = 0; i < verticies.size ( ); i++ ) {
		vtx [ i ].x = verticies [ i ].first.first;
		vtx [ i ].y = verticies [ i ].first.second;
		vtx [ i ].z = 0;
		vtx [ i ].rhw = 1;
		vtx [ i ].color = verticies [ i ].second;
	}

	vtx [ verticies.size ( ) ] = vtx [ 0 ];

	IDirect3DVertexBuffer9* vb = nullptr;

	csgo::i::dev->CreateVertexBuffer ( ( verticies.size ( ) + 1 ) * sizeof ( vtx_t ), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vb, nullptr );

	void* verticies1;
	vb->Lock ( 0, ( verticies.size ( ) + 1 ) * sizeof ( vtx_t ), ( void** ) &verticies1, 0 );
	std::memcpy ( verticies1, &vtx [ 0 ], ( verticies.size ( ) + 1 ) * sizeof ( vtx_t ) );
	vb->Unlock ( );

	csgo::i::dev->SetStreamSource ( 0, vb, 0, sizeof ( vtx_t ) );
	csgo::i::dev->DrawPrimitive ( D3DPT_TRIANGLEFAN, 0, verticies.size ( ) );

	if ( vb )
		vb->Release ( );
}

void render::rounded_rect ( int x, int y, int width, int height, int rad, int resolution, std::uint32_t color, bool outline ) {
	if ( resolution < 2 )
		return;

	std::vector< std::pair< float, float > > vtx ( 4 * resolution );

	for ( auto i = 0; i < 4; i++ ) {
		const auto _x = x + ( ( i < 2 ) ? ( width - rad ) : rad );
		const auto _y = y + ( ( i % 3 ) ? ( height - rad ) : rad );
		const auto a = 90.0f * i;

		for ( auto j = 0; j < resolution; j++ ) {
			const auto _a = csgo::deg2rad ( a + ( j / ( float ) ( resolution - 1 ) ) * 90.0f );
			vtx [ i * resolution + j ] = { _x + rad * std::sinf ( _a ) - 0.5f, _y - rad * std::cosf ( _a ) - 0.5f };
		}
	}

	polygon ( vtx, color, outline );
}

void render::cube ( const vec3_t& pos, int size, std::uint32_t color ) {
	static vec3_t scrn_one;
	static vec3_t scrn_two;
	static vec3_t scrn_three;
	static vec3_t scrn_four;
	static vec3_t scrn_five;
	static vec3_t scrn_six;
	static vec3_t scrn_seven;
	static vec3_t scrn_eight;

	static vec3_t one;
	static vec3_t two;
	static vec3_t three;
	static vec3_t four;
	static vec3_t five;
	static vec3_t six;
	static vec3_t seven;
	static vec3_t eight;

	const auto mdist = static_cast< float >( size ) / 2.0f;

	one = vec3_t( pos.x - mdist, pos.y - mdist, pos.z - mdist );
	two = vec3_t ( pos.x - mdist, pos.y - mdist, pos.z + mdist );
	three = vec3_t ( pos.x - mdist, pos.y + mdist, pos.z + mdist );
	four = vec3_t ( pos.x - mdist, pos.y + mdist, pos.z - mdist );
	five = vec3_t ( pos.x + mdist, pos.y - mdist, pos.z - mdist );
	six = vec3_t ( pos.x + mdist, pos.y - mdist, pos.z + mdist );
	seven = vec3_t ( pos.x + mdist, pos.y + mdist, pos.z + mdist );
	eight = vec3_t ( pos.x + mdist, pos.y + mdist, pos.z - mdist );

	if ( !csgo::render::world_to_screen ( scrn_one, one )
		|| !csgo::render::world_to_screen ( scrn_two, two )
		|| !csgo::render::world_to_screen ( scrn_three, three )
		|| !csgo::render::world_to_screen ( scrn_four, four )
		|| !csgo::render::world_to_screen ( scrn_five, five )
		|| !csgo::render::world_to_screen ( scrn_six, six )
		|| !csgo::render::world_to_screen ( scrn_seven, seven )
		|| !csgo::render::world_to_screen ( scrn_eight, eight ) )
		return;

	/* back */
	line ( scrn_one.x, scrn_one.y, scrn_two.x, scrn_two.y, color );
	line ( scrn_two.x, scrn_two.y, scrn_three.x, scrn_three.y, color );
	line ( scrn_three.x, scrn_three.y, scrn_four.x, scrn_four.y, color );
	line ( scrn_four.x, scrn_four.y, scrn_one.x, scrn_one.y, color );

	/* front */
	line ( scrn_five.x, scrn_five.y, scrn_six.x, scrn_six.y, color );
	line ( scrn_six.x, scrn_six.y, scrn_seven.x, scrn_seven.y, color );
	line ( scrn_seven.x, scrn_seven.y, scrn_eight.x, scrn_eight.y, color );
	line ( scrn_eight.x, scrn_eight.y, scrn_five.x, scrn_five.y, color );
	
	/* connect sides */
	line ( scrn_one.x, scrn_one.y, scrn_five.x, scrn_five.y, color );
	line ( scrn_two.x, scrn_two.y, scrn_six.x, scrn_six.y, color );
	line ( scrn_three.x, scrn_three.y, scrn_seven.x, scrn_seven.y, color );
	line ( scrn_four.x, scrn_four.y, scrn_eight.x, scrn_eight.y, color );
}