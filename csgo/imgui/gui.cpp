#include "gui.hpp"

#include <unordered_map>
#include <functional>
#include <string>

ImFont* g_small_font = nullptr;
int* g_cur_tab_ptr = nullptr;
int g_cur_tab_idx = 0;
float g_window_fade = 0.0f;
float g_sidebar = 0.0f;
ImVec2 g_window_dim = ImVec2( 520, 400 );
std::string g_window_name;

constexpr auto animation_time = 0.100f;

std::unordered_map<int/*tab*/, std::string/*open_node*/> g_nodes_opened { };

template <typename type>
constexpr auto animate( float& t, float change_dir, const type& min, const type& max ) {
    t = ImClamp( t + ImGui::GetIO( ).DeltaTime * ( change_dir * ( 1.0f / animation_time ) ), 0.0f, 1.0f );
    return ImLerp( min, max, t );
}

void ImGui::custom::TextOutlined( const ImVec2& pos, ImU32 color, const char* text ) {
    const auto window = GetCurrentWindow( );
    const auto draw_list = window->DrawList;

    draw_list->AddText( ImVec2( pos.x - 1.0f, pos.y - 1.0f ), color & IM_COL32_A_MASK, text );
    draw_list->AddText( ImVec2( pos.x - 1.0f, pos.y + 1.0f ), color & IM_COL32_A_MASK, text );
    draw_list->AddText( ImVec2( pos.x + 1.0f, pos.y + 1.0f ), color & IM_COL32_A_MASK, text );
    draw_list->AddText( ImVec2( pos.x + 1.0f, pos.y - 1.0f ), color & IM_COL32_A_MASK, text );

    draw_list->AddText( pos, color, text );
}

bool ImGui::custom::BeginTabs( int* cur_tab, ImFont* icon_font ) {
    if ( !cur_tab || !icon_font )
        return false;

    g_cur_tab_ptr = cur_tab;
    g_cur_tab_idx = 0;

    PushFont( icon_font );

    return true;
}

extern std::unordered_map< uintptr_t/*option_address*/, animation_data_t/*animation_data*/ > animation_list;

void ImGui::custom::AddTab( const char* icon ) {
    const auto& style = GetStyle( );
    const auto window = GetCurrentWindow( );
    const auto draw_list = window->DrawList;
    const auto window_pos = ImGui::GetWindowPos( );
    const auto window_size = ImGui::GetWindowSize( );
    const auto menu_bar_width = window_size.x * ( 1.0f / 12.0f );
    const auto icon_size = CalcTextSize( icon );
    const auto this_tab_rect = ImRect( window_pos.x, window_pos.y + menu_bar_width * ( 1 + g_cur_tab_idx ), window_pos.x + menu_bar_width, window_pos.y + menu_bar_width * ( 2 + g_cur_tab_idx ) );
    const auto tab_id = GetID( std::string( icon ).append( "##tab_icon" ).c_str( ) );

    bool hovered, held;
    bool pressed = ButtonBehavior( this_tab_rect, tab_id, &hovered, &held );

    auto& animations = animation_list [ reinterpret_cast< uintptr_t >( g_cur_tab_ptr ) ];

    if ( pressed )
        *g_cur_tab_ptr = g_cur_tab_idx;

    TextOutlined( ImVec2( this_tab_rect.Min.x + ( menu_bar_width - icon_size.x ) * 0.5f, this_tab_rect.Min.y + ( menu_bar_width - icon_size.y ) * 0.5f ), GetColorU32( style.Colors [ ImGuiCol_Text ] ), icon );

    g_cur_tab_idx++;
}

void ImGui::custom::EndTabs( ) {
    const auto& style = GetStyle( );
    const auto window = GetCurrentWindow( );
    const auto draw_list = window->DrawList;

    const auto window_pos = ImGui::GetWindowPos( );
    const auto window_size = ImGui::GetWindowSize( );
    const auto menu_bar_width = window_size.x * ( 1.0f / 12.0f );
    auto& animations = animation_list [ reinterpret_cast< uintptr_t >( g_cur_tab_ptr ) ];

    /* selector animation */
    const auto old_main_fraction = animations.main_fraction;
    const auto tab_idx_flt = static_cast< float >( *g_cur_tab_ptr );

    if ( animations.main_fraction != tab_idx_flt )
        animations.main_fraction += ImGui::GetIO( ).DeltaTime * ( ( animations.main_fraction > tab_idx_flt ? -1.0f : 1.0f ) * ImClamp( abs( animations.main_fraction - tab_idx_flt ), 0.01f, static_cast< float >( g_cur_tab_idx ) ) * ( 1.0f / animation_time ) );

    if ( ( animations.main_fraction > tab_idx_flt && old_main_fraction < tab_idx_flt )
        || ( animations.main_fraction < tab_idx_flt && old_main_fraction > tab_idx_flt ) )
        animations.main_fraction = tab_idx_flt;

    const auto target_color = style.Colors [ ImGuiCol_FrameBgActive ];
    const auto color_to = GetColorU32( ImVec4( target_color.x, target_color.y, target_color.z, 1.0f ) );
    const auto color_from = GetColorU32( ImVec4( target_color.x, target_color.y, target_color.z, 0.0f ) );
    const auto this_tab_rect = ImRect( window_pos.x + menu_bar_width - menu_bar_width * 0.1f, window_pos.y + menu_bar_width * ( 1.0f + animations.main_fraction ), window_pos.x + menu_bar_width, window_pos.y + menu_bar_width * ( 2.0f + animations.main_fraction ) );

    draw_list->AddRectFilled( this_tab_rect.Min, this_tab_rect.Max, color_to );
    //draw_list->AddRectFilledMultiColor( this_tab_rect.Min, this_tab_rect.Max, color_from, color_to, color_to, color_from );

    g_cur_tab_idx = 0;
    //g_cur_tab_ptr = nullptr;

    PopFont( );

    const auto title_size = CalcTextSize( g_window_name.c_str( ) );

    SetNextWindowPos( ImVec2( window_pos.x + menu_bar_width, window_pos.y + menu_bar_width ) );

    ImGui::BeginChildFrame( GetID( "##main_space" ), ImVec2( window_size.x - menu_bar_width, window_size.y - menu_bar_width - title_size.y - style.FramePadding.y * 2.0f ), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove );

    draw_list->AddRectFilled( ImVec2( ImGui::GetWindowPos( ).x, ImGui::GetWindowPos( ).y ), ImVec2( ImGui::GetWindowPos( ).x + ImGui::GetWindowSize( ).x + 1.0f, ImGui::GetWindowPos( ).y + ImGui::GetWindowSize( ).y + 1.0f ), GetColorU32( style.Colors [ ImGuiCol_WindowBg ] ) );
}

bool ImGui::custom::TreeNodeEx( const char* label, const char* desc ) {
    ImGuiWindow* window = GetCurrentWindow( );

    if ( window->SkipItems )
        return false;

    auto flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    const char* label_end = nullptr;
    auto id = GetID( label );

    auto& animations = animation_list [ static_cast< uintptr_t >( id ) ];

    /* more default flags */
    flags |= ImGuiTreeNodeFlags_Framed;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const bool display_frame = ( flags & ImGuiTreeNodeFlags_Framed ) != 0;
    const ImVec2 padding = ( display_frame || ( flags & ImGuiTreeNodeFlags_FramePadding ) ) ? style.FramePadding : ImVec2( style.FramePadding.x, ImMin( window->DC.CurrLineTextBaseOffset, style.FramePadding.y ) );

    if ( !label_end )
        label_end = FindRenderedTextEnd( label );

    auto desc_end = FindRenderedTextEnd( desc );

    const ImVec2 label_size = CalcTextSize( label, label_end, false );

    PushFont( g_small_font );
    const ImVec2 desc_size = CalcTextSize( desc, desc_end, false );
    PopFont( );

    // We vertically grow up to current line height up the typical widget height.
    const float frame_height = ImMax( ImMin( window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2.0f ), label_size.y + padding.y * 2.0f ) + desc_size.y + padding.y;

    ImRect frame_bb;
    frame_bb.Min.x = ( flags & ImGuiTreeNodeFlags_SpanFullWidth ) ? window->WorkRect.Min.x : window->DC.CursorPos.x;
    frame_bb.Min.y = window->DC.CursorPos.y;
    frame_bb.Max.x = window->WorkRect.Max.x;
    frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
    if ( display_frame ) {
        // Framed header expand a little outside the default padding, to the edge of InnerClipRect
        // (FIXME: May remove this at some point and make InnerClipRect align with WindowPadding.x instead of WindowPadding.x*0.5f)
        frame_bb.Min.x -= IM_FLOOR( window->WindowPadding.x * 0.5f - 1.0f );
        frame_bb.Max.x += IM_FLOOR( window->WindowPadding.x * 0.5f );
    }

    const float text_offset_x = g.FontSize + ( display_frame ? padding.x * 3 : padding.x * 2 );           // Collapser arrow width + Spacing
    const float text_offset_y = ImMax( padding.y, window->DC.CurrLineTextBaseOffset );                    // Latch before ItemSize changes it
    const float text_width = g.FontSize + ( label_size.x > 0.0f ? label_size.x + padding.x * 2 : 0.0f );  // Include collapser
    ImVec2 text_pos( window->DC.CursorPos.x + text_offset_x, window->DC.CursorPos.y + text_offset_y );
    ItemSize( ImVec2( text_width, frame_height ), padding.y );

    // For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
    ImRect interact_bb = frame_bb;
    if ( !display_frame && ( flags & ( ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth ) ) == 0 )
        interact_bb.Max.x = frame_bb.Min.x + text_width + style.ItemSpacing.x * 2.0f;

    // Store a flag for the current depth to tell if we will allow closing this node when navigating one of its child.
    // For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between TreeNode() and TreePop().
    // This is currently only support 32 level deep and we are fine with (1 << Depth) overflowing into a zero.
    const bool is_leaf = ( flags & ImGuiTreeNodeFlags_Leaf ) != 0;
    bool is_open = TreeNodeBehaviorIsOpen( id, flags );
    if ( is_open && !g.NavIdIsAlive && ( flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere ) && !( flags & ImGuiTreeNodeFlags_NoTreePushOnOpen ) )
        window->DC.TreeJumpToParentOnPopMask |= ( 1 << window->DC.TreeDepth );

    bool item_add = ItemAdd( interact_bb, id );
    window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
    window->DC.LastItemDisplayRect = frame_bb;

    if ( !item_add ) {
        if ( is_open && !( flags & ImGuiTreeNodeFlags_NoTreePushOnOpen ) )
            TreePushOverrideID( id );
        IMGUI_TEST_ENGINE_ITEM_INFO( window->DC.LastItemId, label, window->DC.ItemFlags | ( is_leaf ? 0 : ImGuiItemStatusFlags_Openable ) | ( is_open ? ImGuiItemStatusFlags_Opened : 0 ) );
        return is_open;
    }

    ImGuiButtonFlags button_flags = ImGuiTreeNodeFlags_None;
    if ( flags & ImGuiTreeNodeFlags_AllowItemOverlap )
        button_flags |= ImGuiButtonFlags_AllowItemOverlap;
    if ( !is_leaf )
        button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;

    // We allow clicking on the arrow section with keyboard modifiers held, in order to easily
    // allow browsing a tree while preserving selection with code implementing multi-selection patterns.
    // When clicking on the rest of the tree node we always disallow keyboard modifiers.
    const float arrow_hit_x1 = ( text_pos.x - text_offset_x ) - style.TouchExtraPadding.x;
    const float arrow_hit_x2 = ( text_pos.x - text_offset_x ) + ( g.FontSize + padding.x * 2.0f ) + style.TouchExtraPadding.x;
    const bool is_mouse_x_over_arrow = ( g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2 );
    if ( window != g.HoveredWindow || !is_mouse_x_over_arrow )
        button_flags |= ImGuiButtonFlags_NoKeyModifiers;

    // Open behaviors can be altered with the _OpenOnArrow and _OnOnDoubleClick flags.
    // Some alteration have subtle effects (e.g. toggle on MouseUp vs MouseDown events) due to requirements for multi-selection and drag and drop support.
    // - Single-click on label = Toggle on MouseUp (default, when _OpenOnArrow=0)
    // - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=0)
    // - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=1)
    // - Double-click on label = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1)
    // - Double-click on arrow = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1 and _OpenOnArrow=0)
    // It is rather standard that arrow click react on Down rather than Up.
    // We set ImGuiButtonFlags_PressedOnClickRelease on OpenOnDoubleClick because we want the item to be active on the initial MouseDown in order for drag and drop to work.
    if ( is_mouse_x_over_arrow )
        button_flags |= ImGuiButtonFlags_PressedOnClick;
    else if ( flags & ImGuiTreeNodeFlags_OpenOnDoubleClick )
        button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
    else
        button_flags |= ImGuiButtonFlags_PressedOnClickRelease;

    bool selected = ( flags & ImGuiTreeNodeFlags_Selected ) != 0;
    const bool was_selected = selected;

    bool hovered, held;
    bool pressed = ButtonBehavior( interact_bb, id, &hovered, &held, button_flags );
    bool toggled = false;

    /* hovering animations */
    const auto hover_color_inner = ColorConvertFloat4ToU32(
        animate( animations.hover_fraction_inner, ( hovered || pressed || is_open ) ? 1.0f : -1.0f, style.Colors [ ImGuiCol_Header ], style.Colors [ ImGuiCol_HeaderHovered ] )
    );

    const auto hover_color_outer = animate( animations.hover_fraction_outer, ( hovered || pressed || is_open ) ? 1.0f : -1.0f, style.Colors [ ImGuiCol_Border ], ImLerp( style.Colors [ ImGuiCol_Border ], style.Colors [ ImGuiCol_FrameBgActive ], 0.5f ) );

    animate( animations.main_fraction, is_open ? 0.5f : -0.5f, 0.0f, 1.0f );

    if ( !is_leaf ) {
        if ( pressed && g.DragDropHoldJustPressedId != id ) {
            if ( ( flags & ( ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick ) ) == 0 || ( g.NavActivateId == id ) )
                toggled = true;
            if ( flags & ImGuiTreeNodeFlags_OpenOnArrow )
                toggled |= is_mouse_x_over_arrow && !g.NavDisableMouseHover; // Lightweight equivalent of IsMouseHoveringRect() since ButtonBehavior() already did the job
            if ( ( flags & ImGuiTreeNodeFlags_OpenOnDoubleClick ) && g.IO.MouseDoubleClicked [ 0 ] )
                toggled = true;
        }
        else if ( pressed && g.DragDropHoldJustPressedId == id ) {
            IM_ASSERT( button_flags & ImGuiButtonFlags_PressedOnDragDropHold );
            if ( !is_open ) // When using Drag and Drop "hold to open" we keep the node highlighted after opening, but never close it again.
                toggled = true;
        }

        if ( g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Left && is_open ) {
            toggled = true;
            NavMoveRequestCancel( );
        }
        if ( g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Right && !is_open ) // If there's something upcoming on the line we may want to give it the priority?
        {
            toggled = true;
            NavMoveRequestCancel( );
        }

        if ( toggled ) {
            is_open = !is_open;
            window->DC.StateStorage->SetInt( id, is_open );
            window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
        }
    }
    if ( flags & ImGuiTreeNodeFlags_AllowItemOverlap )
        SetItemAllowOverlap( );

    // In this branch, TreeNodeBehavior() cannot toggle the selection so this will never trigger.
    if ( selected != was_selected ) //-V547
        window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

    // Render
    const ImU32 text_col = GetColorU32( ImGuiCol_Text );
    ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_TypeThin;

    RenderFrame( frame_bb.Min, ImVec2( frame_bb.Max.x, frame_bb.Max.y + ( frame_bb.Max.y - frame_bb.Min.y ) * 0.1f ), GetColorU32( ImLerp( style.Colors [ ImGuiCol_FrameBg ], ImVec4( 0.0f, 0.0f, 0.0f, 1.0f ), 0.333f ) ), false, style.FrameRounding );

    if ( display_frame ) {
        // Framed type
        const ImU32 bg_col = GetColorU32( ( held && hovered ) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header );
        RenderFrame( frame_bb.Min, frame_bb.Max, bg_col, false, style.FrameRounding );
        RenderNavHighlight( frame_bb, id, nav_highlight_flags );

        //if ( flags & ImGuiTreeNodeFlags_Bullet )
        //    RenderBullet( window->DrawList, ImVec2( text_pos.x - text_offset_x * 0.60f, text_pos.y + g.FontSize * 0.5f ), text_col );
        //else if ( !is_leaf )
        //    RenderArrow( window->DrawList, ImVec2( text_pos.x - text_offset_x + padding.x, text_pos.y ), text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 1.0f );
        //else // Leaf without bullet, left-adjusted text
        text_pos.x -= text_offset_x;

        //PushStyleColor( ImGuiCol_Text, ImLerp( style.Colors [ ImGuiCol_Text ], style.Colors [ ImGuiCol_FrameBgActive ], animations.hover_fraction_outer ) );

        if ( flags & ImGuiTreeNodeFlags_ClipLabelForTrailingButton )
            frame_bb.Max.x -= g.FontSize + style.FramePadding.x;
        if ( g.LogEnabled ) {
            // NB: '##' is normally used to hide text (as a library-wide feature), so we need to specify the text range to make sure the ## aren't stripped out here.
            const char log_prefix [ ] = "\n##";
            const char log_suffix [ ] = "##";
            LogRenderedText( &text_pos, log_prefix, log_prefix + 3 );
            RenderTextClipped( text_pos, frame_bb.Max, label, label_end, &label_size );
            LogRenderedText( &text_pos, log_suffix, log_suffix + 2 );
        }
        else {
            RenderTextClipped( text_pos, frame_bb.Max, label, label_end, &label_size );
        }

        //PopStyleColor( );
    }
    else {
        // Unframed typed for tree nodes
        if ( hovered || selected ) {
            const ImU32 bg_col = GetColorU32( ( held && hovered ) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header );
            RenderFrame( frame_bb.Min, frame_bb.Max, bg_col, false );
            RenderNavHighlight( frame_bb, id, nav_highlight_flags );
        }
        if ( flags & ImGuiTreeNodeFlags_Bullet )
            RenderBullet( window->DrawList, ImVec2( text_pos.x - text_offset_x * 0.5f, text_pos.y + g.FontSize * 0.5f ), text_col );
        else if ( !is_leaf )
            RenderArrow( window->DrawList, ImVec2( text_pos.x - text_offset_x + padding.x, text_pos.y + g.FontSize * 0.15f ), text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 0.70f );
        if ( g.LogEnabled )
            LogRenderedText( &text_pos, ">" );

        //PushStyleColor( ImGuiCol_Text, ImLerp( style.Colors [ ImGuiCol_Text ], style.Colors [ ImGuiCol_FrameBgActive ], animations.hover_fraction_outer ) );
        RenderText( text_pos, label, label_end, false );
        // PopStyleColor( );
    }

    if ( animations.hover_fraction_outer > 0.0f ) {
        PushStyleColor( ImGuiCol_Border, hover_color_outer );
        PushStyleVar( ImGuiStyleVar_ChildBorderSize, ImLerp( style.FrameBorderSize, style.FrameBorderSize * 1.5f, animations.hover_fraction_outer ) );
        RenderFrameBorder( frame_bb.Min, ImVec2( frame_bb.Max.x, frame_bb.Max.y + ( frame_bb.Max.y - frame_bb.Min.y ) * 0.1f ), style.FrameRounding );
        PopStyleVar( );
        PopStyleColor( );
    }

    PushFont( g_small_font );
    PushStyleColor( ImGuiCol_Text, ImVec4( style.Colors [ ImGuiCol_Text ].x, style.Colors [ ImGuiCol_Text ].y, style.Colors [ ImGuiCol_Text ].z, style.Colors [ ImGuiCol_Text ].z * 0.7f ) );
    RenderTextClipped( ImVec2( text_pos.x, text_pos.y + label_size.y + padding.y ), frame_bb.Max, desc, desc_end, &desc_size );
    PopStyleColor( );
    PopFont( );

    static auto render_animated_arrow = [ ] ( ImDrawList* draw_list, ImVec2 pos, ImU32 col, float scale, float animation_fraction ) {
        const float h = draw_list->_Data->FontSize * 1.00f;
        float r = h * 0.40f * scale;

        ImVec2 center = ImVec2( pos.x + h * 0.50f, pos.y );

        const auto a = ImVec2( +0.000f * r, ImLerp( 0.750f, -0.750f, animation_fraction ) * r );
        const auto b = ImVec2( -0.866f * r, ImLerp( -0.750f, 0.750f, animation_fraction ) * r );
        const auto c = ImVec2( +0.866f * r, ImLerp( -0.750f, 0.750f, animation_fraction ) * r );

        draw_list->AddTriangleFilled( ImVec2( center.x + a.x, center.y + a.y ), ImVec2( center.x + b.x, center.y + b.y ), ImVec2( center.x + c.x, center.y + c.y ), col );
    };

    render_animated_arrow( window->DrawList, ImVec2( frame_bb.Max.x - style.FramePadding.x * 5.0f, frame_bb.Min.y + ( frame_bb.Max.y - frame_bb.Min.y ) * 0.5f ), text_col, 0.5f, animations.main_fraction );

    SetCursorPosY( GetCursorPosY( ) + ( frame_bb.Max.y - frame_bb.Min.y ) * 0.1f );

    if ( is_open && !( flags & ImGuiTreeNodeFlags_NoTreePushOnOpen ) )
        TreePushOverrideID( id );
    IMGUI_TEST_ENGINE_ITEM_INFO( id, label, window->DC.ItemFlags | ( is_leaf ? 0 : ImGuiItemStatusFlags_Openable ) | ( is_open ? ImGuiItemStatusFlags_Opened : 0 ) );
    return pressed;
}

bool ImGui::custom::BeginSubtabs( int* cur_subtab ) {
    return true;
}

void ImGui::custom::AddSubtab( const char* title, const char* desc, const std::function<void( )>& func ) {
    //PushFont( g_small_font );
    //PopFont( );
//
    //PushStyleVar( ImGuiStyleVar_FrameRounding, 0.0f );
    //Button( title );
    //PopStyleVar( );

    const auto node_name = std::string( title ).append( "##node" ).append( std::to_string( *g_cur_tab_ptr ) );
    const auto node_id = GetID( node_name.c_str( ) );

    auto entry = g_nodes_opened.find( *g_cur_tab_ptr );

    if ( entry == g_nodes_opened.end( ) )
        g_nodes_opened [ *g_cur_tab_ptr ] = "";

    SetNextTreeNodeOpen( g_nodes_opened [ *g_cur_tab_ptr ] == node_name );

    if ( ( g_nodes_opened [ *g_cur_tab_ptr ] == "" || g_nodes_opened [ *g_cur_tab_ptr ] == node_name ) && TreeNodeEx( node_name.c_str( ), desc ) ) {
        g_nodes_opened [ *g_cur_tab_ptr ] = !g_nodes_opened [ *g_cur_tab_ptr ].empty( ) ? "" : node_name;
    }

    if ( g_nodes_opened [ *g_cur_tab_ptr ] == node_name )
        func( );
}

void ImGui::custom::EndSubtabs( ) {

}

bool ImGui::custom::Begin( const char* name, bool* p_open, ImFont* small_font ) {
    g_small_font = small_font;
    g_window_name = name;

    if ( IsKeyPressed( VK_INSERT, false ) )
        *p_open = !*p_open;

    const auto window_alpha = animate( g_window_fade, *p_open ? 1.0f : -1.0f, 0.0f, 1.0f );

    if ( !window_alpha )
        return false;

    SetNextWindowBgAlpha( window_alpha );
    SetNextWindowSize( g_window_dim );

    if ( !ImGui::Begin(
        name,
        p_open,
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground ) )
        return false;

    const auto title_size = CalcTextSize( name );

    const auto& style = GetStyle( );
    const auto window = GetCurrentWindow( );
    const auto draw_list = window->DrawList;
    const auto window_pos = ImGui::GetWindowPos( );
    const auto window_size = ImGui::GetWindowSize( );
    const auto menu_bar_width = window_size.x * ( 1.0f / 12.0f );

    draw_list->PushClipRect( window_pos, ImVec2( window_pos.x + window_size.x, window_pos.y + window_size.y ) );

    /* window top */
    draw_list->AddRectFilled( ImVec2( window_pos.x + menu_bar_width, window_pos.y ), ImVec2( window_pos.x + window_size.x, window_pos.y + menu_bar_width ), GetColorU32( ImLerp( style.Colors [ ImGuiCol_FrameBg ], ImVec4( style.Colors [ ImGuiCol_FrameBg ].x, style.Colors [ ImGuiCol_FrameBg ].y, style.Colors [ ImGuiCol_FrameBg ].x, 0.0f ), 0.5f ) ) );

    /* menu bar */
    draw_list->AddRectFilled( ImVec2( window_pos.x, window_pos.y + menu_bar_width ), ImVec2( window_pos.x + menu_bar_width, window_pos.y + window_size.y ), GetColorU32( style.Colors [ ImGuiCol_FrameBg ] ) );

    /* menu button */
    draw_list->AddRectFilled( ImVec2( window_pos.x, window_pos.y ), ImVec2( window_pos.x + menu_bar_width, window_pos.y + menu_bar_width ), GetColorU32( style.Colors [ ImGuiCol_FrameBg ] ) );

    return true;
}

void ImGui::custom::End( ) {
    ImGui::EndChildFrame( );

    const auto title_size = CalcTextSize( g_window_name.c_str( ) );

    const auto& style = GetStyle( );
    const auto window = GetCurrentWindow( );
    const auto draw_list = window->DrawList;
    const auto window_pos = ImGui::GetWindowPos( );
    const auto window_size = ImGui::GetWindowSize( );
    const auto menu_bar_width = window_size.x * ( 1.0f / 12.0f );

    /* window title @ bottom right */
    draw_list->AddRectFilled( ImVec2( window_pos.x + menu_bar_width, window_pos.y + window_size.y - title_size.y - style.FramePadding.y * 2.0f ), ImVec2( window_pos.x + window_size.x, window_pos.y + window_size.y ), GetColorU32( style.Colors [ ImGuiCol_FrameBg ] ) );

    draw_list->AddText( ImVec2( window_pos.x + window_size.x - title_size.x - style.FramePadding.x, window_pos.y + window_size.y - title_size.y - style.FramePadding.y ), GetColorU32( style.Colors [ ImGuiCol_Text ] ), g_window_name.c_str( ) );

    const auto color_to = GetColorU32( ImVec4( 0.0f, 0.0f, 0.0f, 0.33f ) );
    const auto color_from = GetColorU32( ImVec4( 0.0f, 0.0f, 0.0f, 0.0f ) );

    /* tabs shadow */
    draw_list->AddRectFilledMultiColor( ImVec2( window_pos.x + menu_bar_width, window_pos.y ), ImVec2( window_pos.x + menu_bar_width + menu_bar_width * 0.2f, window_pos.y + window_size.y ), color_to, color_from, color_from, color_to );

    draw_list->PopClipRect( );

    ImGui::End( );

    g_small_font = nullptr;
}