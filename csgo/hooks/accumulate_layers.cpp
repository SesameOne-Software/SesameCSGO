#include "accumulate_layers.hpp"

decltype( &hooks::accumulate_layers ) hooks::old::accumulate_layers = nullptr;

// credits cbrs
void __fastcall hooks::accumulate_layers( REG , void* setup , vec3_t& pos , void* q , float time ) {
    static auto iks_off = pattern::search( _( "client.dll" ) , _( "8D 47 FC 8B 8F" ) ).add( 5 ).deref( ).add( 4 ).get< uint32_t >( );
    static auto accumulate_pose = pattern::search( _( "server.dll" ) , _("E8 ? ? ? ? 83 BF ? ? ? ? ? 0F 84 ? ? ? ? 8D") ).resolve_rip().get<void( __thiscall* )( void* , vec3_t&, void* , int , float , float , float , void* )>();
    
    const auto player = reinterpret_cast<player_t*>( ecx );
    
    if ( !player || !player->is_player( ) || player->health( ) <= 0 || !player->layers( ) || !*reinterpret_cast< void** >( reinterpret_cast< uintptr_t >( player ) + iks_off ) )
        return old::accumulate_layers( REG_OUT , setup , pos , q , time );

    for ( auto animLayerIndex = 0; animLayerIndex < 13; animLayerIndex++ ) {
        auto& layer = player->layers( )[ animLayerIndex ];

        if ( layer.m_weight > 0.0f && layer.m_order >= 0 && layer.m_order < 13 )
            accumulate_pose( *reinterpret_cast<void**>( setup ), pos , q , layer.m_sequence , layer.m_cycle , layer.m_weight , time , *reinterpret_cast< void** >( reinterpret_cast<uintptr_t>( player ) + iks_off ) );
    }
}