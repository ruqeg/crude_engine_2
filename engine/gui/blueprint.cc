#include <engine/core/hash_map.h>
#include <engine/gui/hardcoded_icon.h>
#include <engine/gui/blueprint_node_builder.h>

#include <engine/gui/blueprint.h>

static inline ImRect
crude_imgui_get_item_rect_
(
);

static inline ImRect
crude_imgui_expanded_
(
  _In_ ImRect const                                        rect,
  _In_ float32                                             x,
  _In_ float32                                             y
);

static bool
crude_imgui_splitter_
(
  _In_ bool                                                split_vertically,
  _In_ float32                                             thickness,
  _In_ float32                                            *size1,
  _In_ float32                                            *size2,
  _In_ float32                                             min_size1,
  _In_ float32                                             min_size2,
  _In_ float32                                             splitter_long_axis_size
);

static int32
crude_gui_blueprint_get_next_id_
(
  _In_ crude_gui_blueprint                                *blueprint
);

static ax::NodeEditor::LinkId
crude_gui_blueprint_get_next_link_id_
(
  _In_ crude_gui_blueprint                                *blueprint
);

static void
crude_gui_blueprint_touch_node_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ ax::NodeEditor::NodeId                              id
);

static float32
crude_gui_blueprint_get_touch_progress_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ ax::NodeEditor::NodeId                              id
);

static void
crude_gui_blueprint_update_touch_
(
  _In_ crude_gui_blueprint                                *blueprint
);

static crude_gui_blueprint_node*
crude_gui_blueprint_find_node_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ ax::NodeEditor::NodeId                              id
);

static crude_gui_blueprint_link*
crude_gui_blueprint_find_link_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ ax::NodeEditor::LinkId                              id
);

static crude_gui_blueprint_pin*
crude_gui_blueprint_find_pin_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ ax::NodeEditor::PinId                               id
);

static bool
crude_gui_blueprint_is_pin_linked_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ ax::NodeEditor::PinId                              id
);

static bool
crude_gui_blueprint_can_create_link_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ crude_gui_blueprint_pin                            *a,
  _In_ crude_gui_blueprint_pin                            *b
);

static bool
crude_gui_blueprint_config_save_node_settings_
(
  _In_ ax::NodeEditor::NodeId                              node_id,
  _In_ char const*                                         data,
  _In_ size_t                                              size,
  _In_ ax::NodeEditor::SaveReasonFlags                     reason,
  _In_ void                                               *user_data
);

static size_t
crude_gui_blueprint_config_load_node_settings_
(
  _In_ ax::NodeEditor::NodeId                              node_id, 
  _In_ char                                               *data,
  _In_ void                                               *user_data
);

static XMFLOAT4
crude_gui_blueprint_pin_type_to_color_
(
  _In_ crude_gui_blueprint_pin_type                     type
);

static void
crude_gui_blueprint_pin_queue_draw_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ crude_gui_blueprint_pin const                      *pin,
  _In_ bool                                                connected,
  _In_ float32                                             alpha
);

static void
crude_gui_blueprint_queue_draw_style_editor_
(
  _In_opt_ bool                                           *queue_draw
);

static void
crude_gui_blueprint_queue_draw_left_pane_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ float32                                             pane_width
);

static void
crude_gui_blueprint_build_node_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ crude_gui_blueprint_node                           *node
);

void
crude_gui_blueprint_link_initialize
(
  _In_ crude_gui_blueprint_link                           *link
)
{
  *link = CRUDE_COMPOUNT_EMPTY( crude_gui_blueprint_link );
  link->color = CRUDE_COMPOUNT( XMFLOAT4, { 1, 1, 1, 1 } );
}

void
crude_gui_blueprint_link_deinitialize
(
  _In_ crude_gui_blueprint_link                           *link
)
{
}

void
crude_gui_blueprint_node_initialize
(
  _In_ crude_gui_blueprint_node                           *node,
  _In_ crude_heap_allocator                               *allocator
)
{
  *node = CRUDE_COMPOUNT_EMPTY( crude_gui_blueprint_node );
  node->color = CRUDE_COMPOUNT_EMPTY( XMFLOAT4, { 1, 1, 1, 1 } );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->inputs, 0, crude_heap_allocator_pack( allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->outputs, 0, crude_heap_allocator_pack( allocator ) );
}

void
crude_gui_blueprint_node_deinitialize
(
  _In_ crude_gui_blueprint_node                           *node
)
{
  CRUDE_ARRAY_DEINITIALIZE( node->inputs );
  CRUDE_ARRAY_DEINITIALIZE( node->outputs );
}

void
crude_gui_blueprint_initialize
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ crude_gui_blueprint_create_node_callback_container  create_node_callback_container,
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_heap_allocator                               *allocator,
  _In_ crude_stack_allocator                              *temporary_allocator,
  _In_ char const                                         *settings_absolute_filepath
)
{
  ax::NodeEditor::Config                                   config;

  *blueprint = CRUDE_COMPOUNT_EMPTY( crude_gui_blueprint );

  blueprint->next_id = 1;
  blueprint->pin_icon_size = 24;
  blueprint->touch_time = 1.f;
  blueprint->show_ordinals = false;

  blueprint->header_background_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  blueprint->restore_icon_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  blueprint->save_icon_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  blueprint->temporary_allocator = temporary_allocator;
  blueprint->gpu = gpu;
  blueprint->create_node_callback_container = create_node_callback_container;
  blueprint->allocator = allocator;
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( blueprint->nodes, 0, crude_heap_allocator_pack( allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( blueprint->links, 0, crude_heap_allocator_pack( allocator ) );
  CRUDE_HASHMAP_INITIALIZE( blueprint->node_id_to_touch_time, crude_heap_allocator_pack( allocator ) );

  config.SettingsFile = "Blueprints.json";
  config.UserPointer = blueprint;
  config.LoadNodeSettings = crude_gui_blueprint_config_load_node_settings_;
  config.SaveNodeSettings = crude_gui_blueprint_config_save_node_settings_;

  blueprint->ax_context = ax::NodeEditor::CreateEditor(&config);
  ax::NodeEditor::SetCurrentEditor( blueprint->ax_context );
  ax::NodeEditor::NavigateToContent();
}

void
crude_gui_blueprint_deinitialize
(
  _In_ crude_gui_blueprint                                *blueprint
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( blueprint->nodes ); ++i )
  {
    crude_gui_blueprint_node_deinitialize( &blueprint->nodes[ i ] );
  }

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( blueprint->links ); ++i )
  {
    crude_gui_blueprint_link_deinitialize( &blueprint->links[ i ] );
  }

  CRUDE_ARRAY_DEINITIALIZE( blueprint->nodes );
  CRUDE_ARRAY_DEINITIALIZE( blueprint->links );
  CRUDE_HASHMAP_DEINITIALIZE( blueprint->node_id_to_touch_time );

  ax::NodeEditor::DestroyEditor( blueprint->ax_context );
}

void
crude_gui_blueprint_queue_draw
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ char const                                         *title
)
{
  static ax::NodeEditor::NodeId                            ax_context_node_id = 0;
  static ax::NodeEditor::LinkId                            ax_context_link_id = 0;
  static ax::NodeEditor::PinId                             ax_context_pin_id = 0;
  static bool                                              create_new_node = false;
  static crude_gui_blueprint_pin                          *new_node_link_pin = NULL;
  static crude_gui_blueprint_pin                          *new_link_pin = NULL;
  static float32                                           left_pane_width = 400.0f;
  static float32                                           right_pane_width = 800.0f;
  static char                                              node_output_string_buffer[ 128 ] = "Edit Me\nMultiline!";
  static bool                                              node_output_string_was_active = false;

  ImGuiIO                                                 *imgui_io;
  float32                                                  window_border_size, window_rounding;

  imgui_io = &ImGui::GetIO();
  ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
  ImGui::SetNextWindowSize( imgui_io->DisplaySize);
  window_border_size = ImGui::GetStyle( ).WindowBorderSize;
  window_rounding = ImGui::GetStyle( ).WindowRounding;
  ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
  ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
  ImGui::Begin( title, nullptr, ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoScrollbar |
    ImGuiWindowFlags_NoScrollWithMouse |
    ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoBringToFrontOnFocus );
  ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, window_border_size );
  ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, window_rounding );
  
  ax::NodeEditor::SetCurrentEditor( blueprint->ax_context );

  crude_imgui_splitter_( true, 4.0f, &left_pane_width, &right_pane_width, 50.0f, 50.0f, false );

  crude_gui_blueprint_queue_draw_left_pane_( blueprint, left_pane_width - 4.0f );

  ImGui::SameLine( 0.0f, 12.0f );

  ax::NodeEditor::Begin( "Node editor" );
  {
    crude_gui_blueprint_node_builder                       node_builder;
    ImVec2                                                 im_cursor_top_left;

    im_cursor_top_left = ImGui::GetCursorScreenPos( );

    crude_gui_blueprint_node_builder_initialize( &node_builder, blueprint->gpu );
    node_builder.header_texture_handle = blueprint->header_background_texture_handle;
    
    for ( uint32 node_index = 0u; node_index < CRUDE_ARRAY_LENGTH( blueprint->nodes ); ++node_index )
    {
      crude_gui_blueprint_node                            *node;
      bool                                                 is_node_simple, has_output_delegates;

      node = &blueprint->nodes[ node_index ];

      if ( node->type != CRUDE_GUI_BLUEPRINT_NODE_TYPE_BLUEPRINT && node->type != CRUDE_GUI_BLUEPRINT_NODE_TYPE_SIMPLE )
      {
        continue;
      }

      is_node_simple = node->type == CRUDE_GUI_BLUEPRINT_NODE_TYPE_SIMPLE;

      has_output_delegates = false;
      for ( uint32 output_index = 0u; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
      {
        if ( node->outputs[ output_index ].type == CRUDE_GUI_BLUEPRINT_PIN_TYPE_DELEGATE )
        {
          has_output_delegates = true;
        }
      }
        
      crude_gui_blueprint_node_builder_begin( &node_builder, node->id );

      if ( !is_node_simple )
      {
        crude_gui_blueprint_node_builder_header( &node_builder, node->color );

        ImGui::Spring( 0 );
        ImGui::TextUnformatted( node->name );
        ImGui::Spring( 1 );
        ImGui::Dummy( ImVec2( 0, 28 ) );
        if ( has_output_delegates )
        {
          ImGui::BeginVertical( "delegates", ImVec2(0, 28) );
          ImGui::Spring( 1, 0 );
          for ( uint32 output_index = 0u; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
          {
            crude_gui_blueprint_pin                       *output;
            float32                                        alpha;

            output = &node->outputs[ output_index ];
            if ( output->type != CRUDE_GUI_BLUEPRINT_PIN_TYPE_DELEGATE )
            {
              continue;
            }

            alpha = ImGui::GetStyle( ).Alpha;
            if ( new_link_pin && ! crude_gui_blueprint_can_create_link_( blueprint, new_link_pin, output ) && output != new_link_pin )
            {
              alpha = alpha * ( 48.0f / 255.0f );
            }

            ax::NodeEditor::BeginPin( output->id, ax::NodeEditor::PinKind::Output );
            ax::NodeEditor::PinPivotAlignment( ImVec2( 1.0f, 0.5f ) );
            ax::NodeEditor::PinPivotSize( ImVec2(0, 0 ) );
            ImGui::BeginHorizontal( output->id.AsPointer( ) );
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
            if ( output->name[ 0 ] )
            {
              ImGui::TextUnformatted( output->name );
              ImGui::Spring(0);
            }

            crude_gui_blueprint_pin_queue_draw_( blueprint, output, crude_gui_blueprint_is_pin_linked_( blueprint, output->id ), alpha );
            ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
            ImGui::EndHorizontal();
            ImGui::PopStyleVar();
            ax::NodeEditor::EndPin();
          }
          ImGui::Spring(1, 0);
          ImGui::EndVertical();
          ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
        }
        else
        {
          ImGui::Spring(0);
        }
        crude_gui_blueprint_node_builder_end_header( &node_builder );
      }
      
      for ( uint32 input_index = 0u; input_index < CRUDE_ARRAY_LENGTH( node->inputs ); ++input_index )
      {
        crude_gui_blueprint_pin                           *input;
        float32                                            alpha;

        input = &node->inputs[ input_index ];

        alpha = ImGui::GetStyle( ).Alpha;
        if ( new_link_pin && !crude_gui_blueprint_can_create_link_( blueprint, new_link_pin, input ) && input != new_link_pin )
        {
          alpha = alpha * ( 48.0f / 255.0f );
        }

        crude_gui_blueprint_node_builder_input( &node_builder, input->id );
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        crude_gui_blueprint_pin_queue_draw_( blueprint, input, crude_gui_blueprint_is_pin_linked_( blueprint, input->id ), alpha );
        ImGui::Spring( 0 );
        if ( input->name[ 0 ] )
        {
          ImGui::TextUnformatted( input->name );
          ImGui::Spring( 0 );
        }
        
        ImGui::PopStyleVar();
        crude_gui_blueprint_node_builder_end_input( &node_builder );
      }

      if ( is_node_simple )
      {
        crude_gui_blueprint_node_builder_middle( &node_builder );

        ImGui::Spring(1, 0);
        ImGui::TextUnformatted( node->name );
        ImGui::Spring(1, 0);
      }
      
      for ( uint32 output_index = 0u; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
      {
        crude_gui_blueprint_pin                           *output;
        float32                                            alpha;

        output = &node->outputs[ output_index ];
        
        if ( !is_node_simple && output->type == CRUDE_GUI_BLUEPRINT_PIN_TYPE_DELEGATE )
        {
          continue;
        }

        alpha = ImGui::GetStyle( ).Alpha;
        if ( new_link_pin && !crude_gui_blueprint_can_create_link_( blueprint, new_link_pin, output ) && output != new_link_pin )
        {
          alpha = alpha * (48.0f / 255.0f);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        crude_gui_blueprint_node_builder_output( &node_builder, output->id );
        if ( output->type == CRUDE_GUI_BLUEPRINT_PIN_TYPE_STRING )
        {
          ImGui::PushItemWidth(100.0f);
          ImGui::InputText("##edit", node_output_string_buffer, 127);
          ImGui::PopItemWidth();
          if ( ImGui::IsItemActive( ) && !node_output_string_was_active )
          {
            ax::NodeEditor::EnableShortcuts( false );
            node_output_string_was_active = true;
          }
          else if ( !ImGui::IsItemActive( ) && node_output_string_was_active )
          {
            ax::NodeEditor::EnableShortcuts( true );
            node_output_string_was_active = false;
          }
          ImGui::Spring( 0 );
        }
        
        if ( output->name )
        {
          ImGui::Spring( 0 );
          ImGui::TextUnformatted( output->name );
        }
        ImGui::Spring( 0 );
        crude_gui_blueprint_pin_queue_draw_( blueprint, output, crude_gui_blueprint_is_pin_linked_( blueprint, output->id ), alpha );
        ImGui::PopStyleVar();
        crude_gui_blueprint_node_builder_end_output( &node_builder );
      }

      crude_gui_blueprint_node_builder_end( &node_builder );
    }

    for ( uint32 node_index = 0u; node_index < CRUDE_ARRAY_LENGTH( blueprint->nodes ); ++node_index )
    {
      crude_gui_blueprint_node                            *node;
      ImDrawList                                          *draw_list;
      ImVec4                                               im_pin_background;
      ImRect                                               inputs_rect, content_rect, outputs_rect;
      ImDrawFlags_                                         top_round_corners_flags, bottom_round_corners_flags;
      float32                                              rounding, padding, input_alpha, output_alpha;

      node = &blueprint->nodes[ node_index ];

      if ( node->type != CRUDE_GUI_BLUEPRINT_NODE_TYPE_TREE )
      {
        continue;
      }
      
      rounding = 5.0f;
      padding  = 12.0f;
      im_pin_background = ax::NodeEditor::GetStyle( ).Colors[ ax::NodeEditor::StyleColor_NodeBg ];

      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBg,        ImColor(128, 128, 128, 200));
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBorder,    ImColor( 32,  32,  32, 200));
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_PinRect,       ImColor( 60, 180, 255, 150));
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_PinRectBorder, ImColor( 60, 180, 255, 150));

      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_NodePadding,  ImVec4(0, 0, 0, 0));
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_NodeRounding, rounding);
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_SourceDirection, ImVec2(0.0f,  1.0f));
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_LinkStrength, 0.0f);
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinBorderWidth, 1.0f);
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinRadius, 5.0f);
      
      ax::NodeEditor::BeginNode( node->id );

      ImGui::BeginVertical( node->id.AsPointer( ) );
      ImGui::BeginHorizontal("inputs");
      ImGui::Spring(0, padding * 2);

      input_alpha = 200 / 255.f;
      if ( CRUDE_ARRAY_LENGTH( node->inputs ) )
      {
        crude_gui_blueprint_pin                           *pin;

        pin = &node->inputs[ 0 ];
        
        ImGui::Dummy( ImVec2( 0, padding ) );
        ImGui::Spring( 1, 0 );
        inputs_rect = crude_imgui_get_item_rect_( );
      
        ax::NodeEditor::PushStyleVar( ax::NodeEditor::StyleVar_PinArrowSize, 10.0f );
        ax::NodeEditor::PushStyleVar( ax::NodeEditor::StyleVar_PinArrowWidth, 10.0f );
        ax::NodeEditor::PushStyleVar( ax::NodeEditor::StyleVar_PinCorners, ImDrawFlags_RoundCornersBottom );
        ax::NodeEditor::BeginPin( pin->id, ax::NodeEditor::PinKind::Input );
        ax::NodeEditor::PinPivotRect( inputs_rect.GetTL( ), inputs_rect.GetBR( ) );
        ax::NodeEditor::PinRect( inputs_rect.GetTL( ), inputs_rect.GetBR( ) );
        ax::NodeEditor::EndPin();
        ax::NodeEditor::PopStyleVar(3);
      
        if ( new_link_pin && !crude_gui_blueprint_can_create_link_( blueprint, new_link_pin, pin ) && pin != new_link_pin )
        {
          input_alpha = ImGui::GetStyle().Alpha * ( 48.0f / 255.0f );
        }
      }
      else
      {
        ImGui::Dummy( ImVec2( 0, padding ) );
      }

      ImGui::Spring( 0, padding * 2 );
      ImGui::EndHorizontal( );

      ImGui::BeginHorizontal( "content_frame" );
      ImGui::Spring( 1, padding );

      ImGui::BeginVertical( "content", ImVec2( 0.0f, 0.0f ) );
      ImGui::Dummy( ImVec2( 160, 0 ) );
      ImGui::Spring( 1 );
      ImGui::TextUnformatted( node->name );
      ImGui::Spring(1);
      ImGui::EndVertical();
      content_rect = crude_imgui_get_item_rect_( );

      ImGui::Spring(1, padding);
      ImGui::EndHorizontal();

      ImGui::BeginHorizontal("outputs");
      ImGui::Spring(0, padding * 2);

      if ( CRUDE_ARRAY_LENGTH( node->outputs ) )
      {
        crude_gui_blueprint_pin *pin = &node->outputs[ 0 ];

        ImGui::Dummy( ImVec2( 0, padding ) );
        ImGui::Spring( 1, 0 );
        outputs_rect = crude_imgui_get_item_rect_( );
      
        ax::NodeEditor::PushStyleVar( ax::NodeEditor::StyleVar_PinCorners, ImDrawFlags_RoundCornersTop );
        ax::NodeEditor::BeginPin( pin->id, ax::NodeEditor::PinKind::Output );
        ax::NodeEditor::PinPivotRect( outputs_rect.GetTL( ), outputs_rect.GetBR( ) );
        ax::NodeEditor::PinRect( outputs_rect.GetTL( ), outputs_rect.GetBR( ) );
        ax::NodeEditor::EndPin( );
        ax::NodeEditor::PopStyleVar( );
      
        if (new_link_pin && !crude_gui_blueprint_can_create_link_( blueprint, new_link_pin, pin ) && pin != new_link_pin)
        {
          output_alpha = ImGui::GetStyle( ).Alpha * ( 48.0f / 255.0f );
        }
      }
      else
      {
        ImGui::Dummy(ImVec2(0, padding));
      }

      ImGui::Spring(0, padding * 2);
      ImGui::EndHorizontal();

      ImGui::EndVertical();

      ax::NodeEditor::EndNode();
      ax::NodeEditor::PopStyleVar(7);
      ax::NodeEditor::PopStyleColor(4);

      draw_list = ax::NodeEditor::GetNodeBackgroundDrawList( node->id );

      top_round_corners_flags = ImDrawFlags_RoundCornersTop;
      bottom_round_corners_flags = ImDrawFlags_RoundCornersBottom;

      draw_list->AddRectFilled( inputs_rect.GetTL( ) + ImVec2(0, 1), inputs_rect.GetBR( ),
        IM_COL32((int)(255 * im_pin_background.x), (int)(255 * im_pin_background.y), (int)(255 * im_pin_background.z), CRUDE_CAST( int32, 255 * input_alpha ) ), 4.0f, bottom_round_corners_flags);
      //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
      draw_list->AddRect( inputs_rect.GetTL( ) + ImVec2(0, 1), inputs_rect.GetBR( ),
        IM_COL32((int)(255 * im_pin_background.x), (int)(255 * im_pin_background.y), (int)(255 * im_pin_background.z), CRUDE_CAST( int32, 255 * input_alpha ) ), 4.0f, bottom_round_corners_flags);
      //ImGui::PopStyleVar();
      draw_list->AddRectFilled(outputs_rect.GetTL(), outputs_rect.GetBR() - ImVec2(0, 1),
        IM_COL32((int)(255 * im_pin_background.x), (int)(255 * im_pin_background.y), (int)(255 * im_pin_background.z), CRUDE_CAST( int32, 255 * input_alpha ) ), 4.0f, top_round_corners_flags);
      //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
      draw_list->AddRect(outputs_rect.GetTL(), outputs_rect.GetBR() - ImVec2(0, 1),
        IM_COL32((int)(255 * im_pin_background.x), (int)(255 * im_pin_background.y), (int)(255 * im_pin_background.z), CRUDE_CAST( int32, 255 * input_alpha ) ), 4.0f, top_round_corners_flags);
      //ImGui::PopStyleVar();
      draw_list->AddRectFilled(content_rect.GetTL(), content_rect.GetBR(), IM_COL32(24, 64, 128, 200), 0.0f);
      //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
      draw_list->AddRect(
        content_rect.GetTL(),
        content_rect.GetBR(),
        IM_COL32(48, 128, 255, 100), 0.0f);
      //ImGui::PopStyleVar();
    }
  
    for ( uint32 node_index = 0u; node_index < CRUDE_ARRAY_LENGTH( blueprint->nodes ); ++node_index )
    {
      crude_gui_blueprint_node                            *node;
      ImVec4                                               pin_background;
      ImRect                                               content_rect;
      float32                                              rounding, padding;

      node = &blueprint->nodes[ node_index ];

      if ( node->type != CRUDE_GUI_BLUEPRINT_NODE_TYPE_HOUDINI )
      {
        continue;
      }

      rounding = 10.0f;
      padding  = 12.0f;

      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBg,        ImColor(229, 229, 229, 200));
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBorder,    ImColor(125, 125, 125, 200));
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_PinRect,       ImColor(229, 229, 229, 60));
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_PinRectBorder, ImColor(125, 125, 125, 60));

      pin_background = ax::NodeEditor::GetStyle( ).Colors[ ax::NodeEditor::StyleColor_NodeBg ];

      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_NodePadding,  ImVec4(0, 0, 0, 0));
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_NodeRounding, rounding);
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_SourceDirection, ImVec2(0.0f,  1.0f));
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_LinkStrength, 0.0f);
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinBorderWidth, 1.0f);
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinRadius, 6.0f);
      ax::NodeEditor::BeginNode( node->id );

      ImGui::BeginVertical( node->id.AsPointer( ) );

      if ( CRUDE_ARRAY_LENGTH( node->inputs ) )
      {
        ImRect                                             inputs_rect;
        float32                                            input_alpha;

        ImGui::BeginHorizontal("inputs");
        ImGui::Spring(1, 0);

        input_alpha = 200 / 255.f;
        
        for ( uint32 input_index = 0u; input_index < CRUDE_ARRAY_LENGTH( node->inputs ); ++input_index )
        {
          crude_gui_blueprint_pin                         *pin;
          ImDrawFlags_                                     im_all_round_corners_flags;

          pin = &node->inputs[ input_index ];

          ImGui::Dummy(ImVec2(padding, padding));
          inputs_rect = crude_imgui_get_item_rect_( );
          ImGui::Spring(1, 0);
          inputs_rect.Min.y -= padding;
          inputs_rect.Max.y -= padding;

          im_all_round_corners_flags = ImDrawFlags_RoundCornersAll;
          //ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
          //ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
          ax::NodeEditor::PushStyleVar( ax::NodeEditor::StyleVar_PinCorners, im_all_round_corners_flags );

          ax::NodeEditor::BeginPin( pin->id, ax::NodeEditor::PinKind::Input);
          ax::NodeEditor::PinPivotRect(inputs_rect.GetCenter(), inputs_rect.GetCenter());
          ax::NodeEditor::PinRect(inputs_rect.GetTL(), inputs_rect.GetBR());
          ax::NodeEditor::EndPin();
          //ed::PopStyleVar(3);
          ax::NodeEditor::PopStyleVar(1);

          ImDrawList *drawList = ImGui::GetWindowDrawList();
          drawList->AddRectFilled(inputs_rect.GetTL(), inputs_rect.GetBR(),
            IM_COL32((int)(255 * pin_background.x), (int)(255 * pin_background.y), (int)(255 * pin_background.z), (int)(255 * input_alpha ) ), 4.0f, im_all_round_corners_flags );
          drawList->AddRect(inputs_rect.GetTL(), inputs_rect.GetBR(),
            IM_COL32((int)(255 * pin_background.x), (int)(255 * pin_background.y), (int)(255 * pin_background.z), (int)(255 * input_alpha )), 4.0f, im_all_round_corners_flags );

          if ( new_link_pin && !crude_gui_blueprint_can_create_link_( blueprint, new_link_pin, pin ) && pin != new_link_pin )
          {
            input_alpha = ImGui::GetStyle().Alpha * ( 48.0f / 255.0f );
          }
        }

        //ImGui::Spring(1, 0);
        ImGui::EndHorizontal();
      }

      ImGui::BeginHorizontal("content_frame");
      ImGui::Spring(1, padding);

      ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
      ImGui::Dummy(ImVec2(160, 0));
      ImGui::Spring(1);
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
      ImGui::TextUnformatted( node->name );
      ImGui::PopStyleColor();
      ImGui::Spring(1);
      ImGui::EndVertical();

      content_rect = crude_imgui_get_item_rect_();

      ImGui::Spring(1, padding);
      ImGui::EndHorizontal();

      if ( CRUDE_ARRAY_LENGTH( node->outputs ) )
      {
        ImRect                                             outputs_rect;
        float32                                            output_alpha;

        ImGui::BeginHorizontal("outputs");
        ImGui::Spring(1, 0);

        output_alpha = 200 / 255.f;
        for ( uint32 output_index = 0u; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
        {
          crude_gui_blueprint_pin                         *pin;
          ImDrawFlags_                                     all_round_corners_flags, top_round_corners_flags;

          pin = &node->outputs[ output_index ];

          ImGui::Dummy(ImVec2(padding, padding));
          outputs_rect = crude_imgui_get_item_rect_( );
          ImGui::Spring(1, 0);
          outputs_rect.Min.y += padding;
          outputs_rect.Max.y += padding;

          all_round_corners_flags = ImDrawFlags_RoundCornersAll;
          top_round_corners_flags = ImDrawFlags_RoundCornersTop;

          ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinCorners, top_round_corners_flags);
          ax::NodeEditor::BeginPin( pin->id, ax::NodeEditor::PinKind::Output );
          ax::NodeEditor::PinPivotRect(outputs_rect.GetCenter(), outputs_rect.GetCenter());
          ax::NodeEditor::PinRect(outputs_rect.GetTL(), outputs_rect.GetBR());
          ax::NodeEditor::EndPin();
          ax::NodeEditor::PopStyleVar();

          ImDrawList *drawList = ImGui::GetWindowDrawList();
          drawList->AddRectFilled(outputs_rect.GetTL(), outputs_rect.GetBR(),
            IM_COL32((int)(255 * pin_background.x), (int)(255 * pin_background.y), (int)(255 * pin_background.z),  (int)(255 * output_alpha ) ), 4.0f, all_round_corners_flags );
          drawList->AddRect(outputs_rect.GetTL(), outputs_rect.GetBR(),
            IM_COL32((int)(255 * pin_background.x), (int)(255 * pin_background.y), (int)(255 * pin_background.z), (int)(255 * output_alpha ) ), 4.0f, all_round_corners_flags );


          if ( new_link_pin && !crude_gui_blueprint_can_create_link_( blueprint, new_link_pin, pin ) && pin != new_link_pin )
          {
            output_alpha = ImGui::GetStyle( ).Alpha * ( 48.0f / 255.0f );
          }
        }

        ImGui::EndHorizontal();
      }

      ImGui::EndVertical();

      ax::NodeEditor::EndNode();
      ax::NodeEditor::PopStyleVar(7);
      ax::NodeEditor::PopStyleColor(4);
    }
    
    for ( uint32 node_index = 0u; node_index < CRUDE_ARRAY_LENGTH( blueprint->nodes ); ++node_index )
    {
      crude_gui_blueprint_node                            *node;
      float32                                              comment_alpha;

      node = &blueprint->nodes[ node_index ];

      if ( node->type != CRUDE_GUI_BLUEPRINT_NODE_TYPE_COMMENT )
      {
        continue;
      }

      comment_alpha = 0.75f;
      ImGui::PushStyleVar( ImGuiStyleVar_Alpha, comment_alpha );
      ax::NodeEditor::PushStyleColor( ax::NodeEditor::StyleColor_NodeBg, ImColor( 255, 255, 255, 64 ) );
      ax::NodeEditor::PushStyleColor( ax::NodeEditor::StyleColor_NodeBorder, ImColor( 255, 255, 255, 64 ) );
      ax::NodeEditor::BeginNode( node->id );
      ImGui::PushID( node->id.AsPointer( ) );
      ImGui::BeginVertical( "content" );
      ImGui::BeginHorizontal( "horizontal" );
      ImGui::Spring( 1 );
      ImGui::TextUnformatted( node->name );
      ImGui::Spring( 1 );
      ImGui::EndHorizontal( );
      ax::NodeEditor::Group( ImVec2( node->size.x, node->size.y ) );
      ImGui::EndVertical( );
      ImGui::PopID( );
      ax::NodeEditor::EndNode( );
      ax::NodeEditor::PopStyleColor( 2 );
      ImGui::PopStyleVar( );

      if ( ax::NodeEditor::BeginGroupHint( node->id ) )
      {
        ImDrawList                                        *im_draw_list;
        ImRect                                             hint_bounds, hint_frame_bounds;
        ImVec2                                             min;
        int32                                              bg_alpha;
        
        bg_alpha = ImGui::GetStyle( ).Alpha * 255;
      
        min = ax::NodeEditor::GetGroupMin();
      
        ImGui::SetCursorScreenPos( min - ImVec2( -8, ImGui::GetTextLineHeightWithSpacing( ) + 4 ) );
        ImGui::BeginGroup( );
        ImGui::TextUnformatted( node->name );
        ImGui::EndGroup( );
      
        im_draw_list = ax::NodeEditor::GetHintBackgroundDrawList( );
      
        hint_bounds = crude_imgui_get_item_rect_();
        hint_frame_bounds = crude_imgui_expanded_( hint_bounds, 8, 4 );
      
        im_draw_list->AddRectFilled(
            hint_frame_bounds.GetTL(),
            hint_frame_bounds.GetBR(),
            IM_COL32(255, 255, 255, CRUDE_CAST( int32, 64 * bg_alpha ) ), 4.0f);
      
        im_draw_list->AddRect(
            hint_frame_bounds.GetTL( ),
            hint_frame_bounds.GetBR( ),
            IM_COL32(255, 255, 255, CRUDE_CAST( int32, 128 * bg_alpha ) ), 4.0f );
      }
      ax::NodeEditor::EndGroupHint();
    }

    for ( uint32 link_index = 0; link_index < CRUDE_ARRAY_LENGTH( blueprint->links ); ++link_index )
    {
      crude_gui_blueprint_link *link = &blueprint->links[ link_index ];
      ax::NodeEditor::Link( link->id, link->start_pin_id, link->end_pin_id, ImColor( link->color.x, link->color.y, link->color.z, link->color.w ), 2.0f);
    }

    if ( !create_new_node )
    {
      if (ax::NodeEditor::BeginCreate( ImColor( 255, 255, 255 ), 2.0f ) )
      {
        ax::NodeEditor::PinId                              ax_start_pin_id, ax_end_pin_id, ax_pin_id;

        auto showLabel = [](const char* label, ImColor color)
        {
          ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
          auto size = ImGui::CalcTextSize(label);
        
          auto padding = ImGui::GetStyle().FramePadding;
          auto spacing = ImGui::GetStyle().ItemSpacing;
        
          ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));
        
          auto rectMin = ImGui::GetCursorScreenPos() - padding;
          auto rectMax = ImGui::GetCursorScreenPos() + size + padding;
        
          auto drawList = ImGui::GetWindowDrawList();
          drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
          ImGui::TextUnformatted(label);
        };
      
        ax_start_pin_id = ax_end_pin_id = 0;

        if ( ax::NodeEditor::QueryNewLink( &ax_start_pin_id, &ax_end_pin_id ) )
        {
          crude_gui_blueprint_pin                         *start_pin;
          crude_gui_blueprint_pin                         *end_pin;

          start_pin = crude_gui_blueprint_find_pin_( blueprint, ax_start_pin_id );
          end_pin = crude_gui_blueprint_find_pin_( blueprint, ax_end_pin_id );
        
          new_link_pin = start_pin ? start_pin : end_pin;
        
          if ( start_pin->kind == CRUDE_GUI_BLUEPRINT_PIN_KIND_INPUT )
          {
            
            CRUDE_SWAP( start_pin, end_pin );
            CRUDE_SWAP( ax_start_pin_id, ax_end_pin_id );
          }
        
          if ( start_pin && end_pin )
          {
            if ( end_pin == start_pin )
            {
              ax::NodeEditor::RejectNewItem(ImColor(255, 0, 0), 2.0f);
            }
            else if ( end_pin->kind == start_pin->kind)
            {
              showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
              ax::NodeEditor::RejectNewItem(ImColor(255, 0, 0), 2.0f);
            }
            else if (end_pin->node == start_pin->node )
            {
              showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
              ax::NodeEditor::RejectNewItem(ImColor(255, 0, 0), 1.0f);
            }
            else if ( end_pin->type != start_pin->type )
            {
              showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
              ax::NodeEditor::RejectNewItem(ImColor(255, 128, 128), 1.0f);
            }
            else
            {
              showLabel("+ Create Link", ImColor(32, 45, 32, 180));
              if (ax::NodeEditor::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
              {
                crude_gui_blueprint_link link;
                crude_gui_blueprint_link_initialize( &link );

                link.id = crude_gui_blueprint_get_next_id_( blueprint );
                link.start_pin_id = ax_start_pin_id;
                link.end_pin_id = ax_end_pin_id;
                link.color = crude_gui_blueprint_pin_type_to_color_( start_pin->type );
                CRUDE_ARRAY_PUSH( blueprint->links, link );
              }
            }
          }
        }
      
        ax_pin_id = 0;
        if ( ax::NodeEditor::QueryNewNode( &ax_pin_id ) )
        {
          new_link_pin = crude_gui_blueprint_find_pin_( blueprint, ax_pin_id );
          if (new_link_pin)
          {
            showLabel("+ Create Node", ImColor(32, 45, 32, 180));
          }
      
          if (ax::NodeEditor::AcceptNewItem())
          {
            create_new_node = true;
            new_node_link_pin = crude_gui_blueprint_find_pin_( blueprint, ax_pin_id );
            new_link_pin = nullptr;
            ax::NodeEditor::Suspend();
            ImGui::OpenPopup("Create New Node");
            ax::NodeEditor::Resume();
          }
        }
      }
      else
      {
        new_link_pin = nullptr;
      }

      ax::NodeEditor::EndCreate();

      if (ax::NodeEditor::BeginDelete())
      {
        ax::NodeEditor::NodeId node_id = 0;
        while ( ax::NodeEditor::QueryDeletedNode( &node_id ) )
        {
          if ( ax::NodeEditor::AcceptDeletedItem( ) )
          {
            uint32 i; 
            for ( i = 0; i < CRUDE_ARRAY_LENGTH( blueprint->nodes ); ++i )
            {
              crude_gui_blueprint_node *node = &blueprint->nodes[ i ];
              if ( node->id == node_id )
              {
                break;
              }
            }
            if ( i != CRUDE_ARRAY_LENGTH( blueprint->nodes ) )
            {
              CRUDE_ARRAY_DELSWAP( blueprint->nodes, i );
            }
          }
        }

        ax::NodeEditor::LinkId linkId = 0;
        while (ax::NodeEditor::QueryDeletedLink(&linkId))
        {
          if (ax::NodeEditor::AcceptDeletedItem())
          {
            uint32 i; 
            for ( i = 0; i < CRUDE_ARRAY_LENGTH( blueprint->links ); ++i )
            {
              if ( blueprint->links[ i ].id == linkId )
              {
                break;
              }
            }
            if ( i != CRUDE_ARRAY_LENGTH( blueprint->links ) )
            {
              CRUDE_ARRAY_DELSWAP( blueprint->links, i );
            }
          }
        }
      }
      
      ax::NodeEditor::EndDelete();
    }

    ImGui::SetCursorScreenPos( im_cursor_top_left );
  }

  {
    ImVec2                                                 open_popup_position;
    
    open_popup_position = ImGui::GetMousePos();

    ax::NodeEditor::Suspend();

    if ( ax::NodeEditor::ShowNodeContextMenu( &ax_context_node_id ))
    {
      ImGui::OpenPopup( "Node Context Menu" );
    }
    else if ( ax::NodeEditor::ShowPinContextMenu( &ax_context_pin_id ) )
    {
      ImGui::OpenPopup( "Pin Context Menu" );
    }
    else if ( ax::NodeEditor::ShowLinkContextMenu( &ax_context_link_id ) )
    {
      ImGui::OpenPopup( "Link Context Menu" );
    }
    else if ( ax::NodeEditor::ShowBackgroundContextMenu( ) )
    {
      ImGui::OpenPopup( "Create New Node" );
      new_node_link_pin = NULL;
    }

    ax::NodeEditor::Resume();

    ax::NodeEditor::Suspend();
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 8, 8 ) );
    
    if ( ImGui::BeginPopup( "Node Context Menu" ) )
    {
      crude_gui_blueprint_node *node = crude_gui_blueprint_find_node_( blueprint, ax_context_node_id );

      ImGui::TextUnformatted("Node Context Menu");
      ImGui::Separator();
      if ( node )
      {
        ImGui::Text( "ID: %p", node->id.AsPointer( ) );
        ImGui::Text( "Type: %s", node->type == CRUDE_GUI_BLUEPRINT_NODE_TYPE_BLUEPRINT ? "Blueprint" : (node->type == CRUDE_GUI_BLUEPRINT_NODE_TYPE_TREE ? "Tree" : "Comment") );
        ImGui::Text( "Inputs: %d", CRUDE_ARRAY_LENGTH( node->inputs )  );
        ImGui::Text( "Outputs: %d", CRUDE_ARRAY_LENGTH( node->outputs )  );
      }
      else
      {
        ImGui::Text( "Unknown node: %p", ax_context_node_id.AsPointer() );
      }
      
      ImGui::Separator( );
      if ( ImGui::MenuItem( "Delete" ) )
      {
        ax::NodeEditor::DeleteNode( ax_context_node_id );
      }
      ImGui::EndPopup( );
    }

    if ( ImGui::BeginPopup( "Pin Context Menu" ) )
    {
      crude_gui_blueprint_pin *pin = crude_gui_blueprint_find_pin_( blueprint, ax_context_pin_id );

      ImGui::TextUnformatted("Pin Context Menu");
      ImGui::Separator();
      if ( pin )
      {
        ImGui::Text( "ID: %p", pin->id.AsPointer( ) );
        if (pin->node )
        {
          ImGui::Text( "Node: %p", pin->node->id.AsPointer( ) );
        }
        else
        {
          ImGui::Text( "Node: %s", "<none>" );
        }
      }
      else
      {
        ImGui::Text( "Unknown pin: %p", ax_context_pin_id.AsPointer( ) );
      }

      ImGui::EndPopup( );
    }

    if ( ImGui::BeginPopup( "Link Context Menu" ) )
    {
      crude_gui_blueprint_link *link = crude_gui_blueprint_find_link_( blueprint, ax_context_link_id );

      ImGui::TextUnformatted("Link Context Menu");
      ImGui::Separator();
      if ( link )
      {
        ImGui::Text( "ID: %p", link->id.AsPointer( ) );
        ImGui::Text( "From: %p", link->start_pin_id.AsPointer( ) );
        ImGui::Text( "To: %p", link->end_pin_id.AsPointer( ) );
      }
      else
      {
        ImGui::Text( "Unknown link: %p", ax_context_link_id.AsPointer( ) );
      }

      ImGui::Separator( );
      
      if ( ImGui::MenuItem( "Delete" ) )
      {
        ax::NodeEditor::DeleteLink( ax_context_link_id );
      }
      
      ImGui::EndPopup( );
    }

    if (ImGui::BeginPopup("Create New Node"))
    {
      ImVec2                                               new_node_postion;
      crude_gui_blueprint_node                            *node;
      
      node = NULL;
      new_node_postion = open_popup_position;

      blueprint->create_node_callback_container.fun( blueprint->create_node_callback_container.ctx, &node );
      
      if ( node )
      {
        crude_gui_blueprint_pin                           *start_pin;

        crude_gui_blueprint_build_nodes( blueprint );

        create_new_node = false;

        ax::NodeEditor::SetNodePosition(node->id, new_node_postion );

        start_pin = new_node_link_pin;
        if ( start_pin )
        {
          crude_gui_blueprint_pin *pins = start_pin->kind == CRUDE_GUI_BLUEPRINT_PIN_KIND_INPUT ? node->outputs : node->inputs;
          
          for ( uint32 pin_index = 0u; pin_index < CRUDE_ARRAY_LENGTH( pins ); ++pin_index )
          {
            crude_gui_blueprint_pin *pin = &pins[ pin_index ];

            if ( crude_gui_blueprint_can_create_link_( blueprint, start_pin, pin ) )
            {
              crude_gui_blueprint_pin *end_pin = pin;
              if ( start_pin->kind == CRUDE_GUI_BLUEPRINT_PIN_KIND_INPUT )
              {
                CRUDE_SWAP( start_pin, end_pin );
              }
        
              crude_gui_blueprint_link link;
              crude_gui_blueprint_link_initialize( &link );
              link.id = crude_gui_blueprint_get_next_id_( blueprint );
              link.start_pin_id = start_pin->id;
              link.end_pin_id = end_pin->id;
              link.color = crude_gui_blueprint_pin_type_to_color_( start_pin->type );
              CRUDE_ARRAY_PUSH( blueprint->links, link );
              break;
            }
          }
        }
      }

      ImGui::EndPopup();
    }
    else
    {
      create_new_node = false;
    }
    ImGui::PopStyleVar();
    ax::NodeEditor::Resume();
  }

  ax::NodeEditor::End();

  if ( blueprint->show_ordinals )
  {
    ImDrawList                                            *im_draw_list;
    ax::NodeEditor::NodeId                                *ax_ordered_node_ids;
    ImVec2                                                 editor_min, editor_max;
    uint64                                                 ordinal, temporary_allocator_marker, nodeCount;

    temporary_allocator_marker = crude_stack_allocator_get_marker( blueprint->temporary_allocator );
    nodeCount = ax::NodeEditor::GetNodeCount();
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( ax_ordered_node_ids, nodeCount, crude_stack_allocator_pack( blueprint->temporary_allocator ) );
    ax::NodeEditor::GetOrderedNodeIds( ax_ordered_node_ids, nodeCount );
  
    editor_min = ImGui::GetItemRectMin();
    editor_max = ImGui::GetItemRectMax();

    im_draw_list = ImGui::GetWindowDrawList();
    im_draw_list->PushClipRect( editor_min, editor_max );
  
    ordinal = 0;
    for ( uint32 i = 0; i < nodeCount; ++i )
    {
      ImGuiTextBuffer                                      im_text_builder;
      ImVec2                                               im_text_size, im_padding, im_widget_size, im_widget_position, p0, p1;
      ax::NodeEditor::NodeId                               ax_node_id;

      ax_node_id = ax_ordered_node_ids[ i ];
      p0 = ax::NodeEditor::GetNodePosition( ax_node_id );
      p1 = p0 + ax::NodeEditor::GetNodeSize( ax_node_id );
      p0 = ax::NodeEditor::CanvasToScreen( p0 );
      p1 = ax::NodeEditor::CanvasToScreen( p1 );

      im_text_builder.appendf( "#%d", ordinal++ );
    
      im_text_size = ImGui::CalcTextSize( im_text_builder.c_str( ) );
      im_padding = ImVec2( 2.0f, 2.0f);
      im_widget_size = im_text_size + im_padding * 2;
    
      im_widget_position = ImVec2(p1.x, p0.y) + ImVec2(0.0f, -im_widget_size.y);
    
      im_draw_list->AddRectFilled( im_widget_position, im_widget_position + im_widget_size, IM_COL32(100, 80, 80, 190), 3.0f, ImDrawFlags_RoundCornersAll );
      im_draw_list->AddRect( im_widget_position, im_widget_position + im_widget_size, IM_COL32(200, 160, 160, 190), 3.0f, ImDrawFlags_RoundCornersAll );
      im_draw_list->AddText( im_widget_position + im_padding, IM_COL32(255, 255, 255, 255), im_text_builder.c_str() );
    }

    im_draw_list->PopClipRect();
    crude_stack_allocator_free_marker( blueprint->temporary_allocator, temporary_allocator_marker );
  }

  ImGui::PopStyleVar( 2 );
  ImGui::End( );
  ImGui::PopStyleVar( 2 );
}

crude_gui_blueprint_node*
crude_gui_blueprint_create_node_unsafe_ptr
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ char const                                         *name,
  _In_ XMFLOAT4                                            color
)
{
  crude_gui_blueprint_node new_node;
  crude_gui_blueprint_node_initialize( &new_node, blueprint->allocator );
  new_node.id = crude_gui_blueprint_get_next_id_( blueprint );
  new_node.name = name;
  new_node.color = color;
  CRUDE_ARRAY_PUSH( blueprint->nodes, new_node );
  return &CRUDE_ARRAY_BACK( blueprint->nodes );
}

void
crude_gui_blueprint_add_input_pin_to_node
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ crude_gui_blueprint_node                           *node,
  _In_ char const                                         *name,
  _In_ crude_gui_blueprint_pin_type                        type
)
{
  crude_gui_blueprint_pin new_pin = CRUDE_COMPOUNT_EMPTY( crude_gui_blueprint_pin );
  new_pin.id = crude_gui_blueprint_get_next_id_( blueprint );
  new_pin.name = name;
  new_pin.type = type;
  CRUDE_ARRAY_PUSH( node->inputs, new_pin );
}

void
crude_gui_blueprint_add_output_pin_to_node
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ crude_gui_blueprint_node                           *node,
  _In_ char const                                         *name,
  _In_ crude_gui_blueprint_pin_type                        type
)
{
  crude_gui_blueprint_pin new_pin = CRUDE_COMPOUNT_EMPTY( crude_gui_blueprint_pin );
  new_pin.id = crude_gui_blueprint_get_next_id_( blueprint );
  new_pin.name = name;
  new_pin.type = type;
  CRUDE_ARRAY_PUSH( node->outputs, new_pin );
}

void
crude_gui_blueprint_build_nodes
(
  _In_ crude_gui_blueprint                                *blueprint
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( blueprint->nodes ); ++i )
  {
    crude_gui_blueprint_build_node_( blueprint, &blueprint->nodes[ i ] );
  }
}

ImRect
crude_imgui_get_item_rect_
(
)
{
  return ImRect( ImGui::GetItemRectMin( ), ImGui::GetItemRectMax( ) );
}

ImRect
crude_imgui_expanded_
(
  _In_ ImRect const                                        rect,
  _In_ float32                                             x,
  _In_ float32                                             y
)
{
  ImRect result = rect;
  result.Min.x -= x;
  result.Min.y -= y;
  result.Max.x += x;
  result.Max.y += y;
  return result;
}

bool
crude_imgui_splitter_
(
  _In_ bool                                                split_vertically,
  _In_ float32                                             thickness,
  _In_ float32                                            *size1,
  _In_ float32                                            *size2,
  _In_ float32                                             min_size1,
  _In_ float32                                             min_size2,
  _In_ float32                                             splitter_long_axis_size
)
{
  ImGuiContext& g = *GImGui;
  ImGuiWindow* window = g.CurrentWindow;
  ImGuiID id = window->GetID("##Splitter");
  ImRect bb;
  bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
  bb.Max = bb.Min + ImGui::CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
  return ImGui::SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
}

int32
crude_gui_blueprint_get_next_id_
(
  _In_ crude_gui_blueprint                                *blueprint
)
{
  return blueprint->next_id++;
}

ax::NodeEditor::LinkId
crude_gui_blueprint_get_next_link_id_
(
  _In_ crude_gui_blueprint                                *blueprint
)
{
  return ax::NodeEditor::LinkId( crude_gui_blueprint_get_next_id_( blueprint ) );
}

void
crude_gui_blueprint_touch_node_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ ax::NodeEditor::NodeId                              id
)
{
  CRUDE_HASHMAP_SET( blueprint->node_id_to_touch_time, CRUDE_CAST( uint64, id.AsPointer( ) ), blueprint->touch_time );
}

float32
crude_gui_blueprint_get_touch_progress_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ ax::NodeEditor::NodeId                              id
)
{
  int64 index = CRUDE_HASHMAP_GET_INDEX( blueprint->node_id_to_touch_time, CRUDE_CAST( uint64, id.AsPointer( ) ) );
  
  if ( index != -1 && blueprint->node_id_to_touch_time[ index ].value > 0.0f)
  {
      return ( blueprint->touch_time - blueprint->node_id_to_touch_time[ index ].value ) / blueprint->touch_time;
  }

  return 0.0f;
}

void
crude_gui_blueprint_update_touch_
(
  _In_ crude_gui_blueprint                                *blueprint
)
{
  float32 delta_time = ImGui::GetIO( ).DeltaTime;

  for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( blueprint->node_id_to_touch_time ); ++i )
  {
    if ( !crude_hashmap_backet_key_valid( blueprint->node_id_to_touch_time[ i ].key ) )
    {
      continue;
    }
    
    if ( blueprint->node_id_to_touch_time[ i ].value > 0.f )
    {
      blueprint->node_id_to_touch_time[ i ].value -= delta_time;
    }
  }
}

crude_gui_blueprint_node*
crude_gui_blueprint_find_node_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ ax::NodeEditor::NodeId                              id
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( blueprint->nodes ); ++i )
  {
    crude_gui_blueprint_node *node = &blueprint->nodes[ i ];

    if ( node->id == id )
    {
        return node;
    }
  }

  return NULL;
}

crude_gui_blueprint_link*
crude_gui_blueprint_find_link_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ ax::NodeEditor::LinkId                              id
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( blueprint->links ); ++i )
  {
    if ( blueprint->links[ i ].id == id )
    {
        return &blueprint->links[ i ];
    }
  }

  return NULL;
}

crude_gui_blueprint_pin*
crude_gui_blueprint_find_pin_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ ax::NodeEditor::PinId                               id
)
{
  if ( !id )
  {
    return NULL;
  }
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( blueprint->nodes ); ++i )
  {
    crude_gui_blueprint_node *node = &blueprint->nodes[ i ];

    for ( uint32 k = 0; k < CRUDE_ARRAY_LENGTH( node->inputs ); ++k )
    {
      if ( node->inputs[ k ].id == id )
      {
        return &node->inputs[ k ];
      }
    }
    
    for ( uint32 k = 0; k < CRUDE_ARRAY_LENGTH( node->outputs ); ++k )
    {
      if ( node->outputs[ k ].id == id )
      {
        return &node->outputs[ k ];
      }
    }
  }

  return NULL;
}

bool
crude_gui_blueprint_is_pin_linked_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ ax::NodeEditor::PinId                              id
)
{
  if ( !id )
  {
    return false;
  }
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( blueprint->links ); ++i )
  {
    if ( blueprint->links[ i ].start_pin_id == id || blueprint->links[ i ].end_pin_id == id )
    {
      return true;
    }
  }

  return false;
}

bool
crude_gui_blueprint_can_create_link_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ crude_gui_blueprint_pin                            *a,
  _In_ crude_gui_blueprint_pin                            *b
)
{
  if ( !a || !b || a == b || a->kind == b->kind || a->type != b->type || a->node == b->node )
  {
    return false;
  }
  return true;
}

bool
crude_gui_blueprint_config_save_node_settings_
(
  _In_ ax::NodeEditor::NodeId                              node_id,
  _In_ char const*                                         data,
  _In_ size_t                                              size,
  _In_ ax::NodeEditor::SaveReasonFlags                     reason,
  _In_ void                                               *user_data
)
{
  crude_gui_blueprint                                     *blueprint;
  crude_gui_blueprint_node                                *node;

  blueprint = CRUDE_CAST( crude_gui_blueprint*, user_data );

  node = crude_gui_blueprint_find_node_( blueprint, node_id );
  if ( !node )
  {
    return false;
  }

  crude_string_copy( node->state, data, size );
  crude_gui_blueprint_touch_node_( blueprint, node_id );

  return true;
}

size_t
crude_gui_blueprint_config_load_node_settings_
(
  _In_ ax::NodeEditor::NodeId                              node_id, 
  _In_ char                                               *data,
  _In_ void                                               *user_data
)
{
  crude_gui_blueprint                                     *blueprint;
  crude_gui_blueprint_node                                *node;
  uint64                                                   length;
  
  blueprint = CRUDE_CAST( crude_gui_blueprint*, user_data );

  node = crude_gui_blueprint_find_node_( blueprint, node_id );
  if ( !node )
  {
    return 0;
  }
  
  if ( data != NULL )
  {
    length = crude_string_copy_unknow_length( data, node->state, sizeof( node->state ) );
  }
  return length;
}

XMFLOAT4
crude_gui_blueprint_pin_type_to_color_
(
  _In_ crude_gui_blueprint_pin_type                     type
)
{
  switch ( type )
  {
    default:
    case CRUDE_GUI_BLUEPRINT_PIN_TYPE_FLOW:     return CRUDE_COMPOUNT( XMFLOAT4, { 255 / 255.f, 255 / 255.f, 255 / 255.f, 1 } );
    case CRUDE_GUI_BLUEPRINT_PIN_TYPE_BOOL:     return CRUDE_COMPOUNT( XMFLOAT4, { 220 / 255.f,  48 / 255.f,  48 / 255.f, 1 } );
    case CRUDE_GUI_BLUEPRINT_PIN_TYPE_INT:      return CRUDE_COMPOUNT( XMFLOAT4, {  68 / 255.f, 201 / 255.f, 156 / 255.f, 1 } );
    case CRUDE_GUI_BLUEPRINT_PIN_TYPE_FLOAT:    return CRUDE_COMPOUNT( XMFLOAT4, { 147 / 255.f, 226 / 255.f,  74 / 255.f, 1 } );
    case CRUDE_GUI_BLUEPRINT_PIN_TYPE_STRING:   return CRUDE_COMPOUNT( XMFLOAT4, { 124 / 255.f,  21 / 255.f, 153 / 255.f, 1 } );
    case CRUDE_GUI_BLUEPRINT_PIN_TYPE_OBJECT:   return CRUDE_COMPOUNT( XMFLOAT4, {  51 / 255.f, 150 / 255.f, 215 / 255.f, 1 } );
    case CRUDE_GUI_BLUEPRINT_PIN_TYPE_FUNCTION: return CRUDE_COMPOUNT( XMFLOAT4, { 218 / 255.f,   0 / 255.f, 183 / 255.f, 1 } );
    case CRUDE_GUI_BLUEPRINT_PIN_TYPE_DELEGATE: return CRUDE_COMPOUNT( XMFLOAT4, { 255 / 255.f,  48 / 255.f,  48 / 255.f, 1 } );
  }
}

void
crude_gui_blueprint_pin_queue_draw_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ crude_gui_blueprint_pin const                      *pin,
  _In_ bool                                                connected,
  _In_ float32                                             alpha
)
{
  
  crude_gui_hardcoded_icon_type                            icon_type;
  XMFLOAT4                                                 icon_color;

  icon_color = crude_gui_blueprint_pin_type_to_color_( pin->type );
  icon_color.w = alpha;

  switch ( pin->type )
  {
  case CRUDE_GUI_BLUEPRINT_PIN_TYPE_FLOW:     icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_FLOW;   break;
  case CRUDE_GUI_BLUEPRINT_PIN_TYPE_BOOL:     icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE; break;
  case CRUDE_GUI_BLUEPRINT_PIN_TYPE_INT:      icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE; break;
  case CRUDE_GUI_BLUEPRINT_PIN_TYPE_FLOAT:    icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE; break;
  case CRUDE_GUI_BLUEPRINT_PIN_TYPE_STRING:   icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE; break;
  case CRUDE_GUI_BLUEPRINT_PIN_TYPE_OBJECT:   icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE; break;
  case CRUDE_GUI_BLUEPRINT_PIN_TYPE_FUNCTION: icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE; break;
  case CRUDE_GUI_BLUEPRINT_PIN_TYPE_DELEGATE: icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_SQUARE; break;
  default:
    return;
  }
  
  crude_gui_queue_draw_icon(
    XMFLOAT2 { CRUDE_CAST( float32, blueprint->pin_icon_size ), CRUDE_CAST( float32, blueprint->pin_icon_size ) },
    icon_type,
    connected,
    icon_color,
    XMFLOAT4 { 32 / 255.f, 32/ 255.f, 32/ 255.f, alpha / 255.0f } );
}

void
crude_gui_blueprint_queue_draw_style_editor_
(
  _In_opt_ bool                                           *queue_draw
)
{
  static ImGuiColorEditFlags                               im_edit_mode = ImGuiColorEditFlags_DisplayRGB;
  static ImGuiTextFilter                                   im_filter;

  ax::NodeEditor::Style                                   *ax_editor_style;
  float32                                                  pane_width;

  if ( !ImGui::Begin( "Style", queue_draw ) )
  {
    ImGui::End();
    return;
  }

  pane_width = ImGui::GetContentRegionAvail().x;

  ax_editor_style = &ax::NodeEditor::GetStyle( );

  ImGui::BeginHorizontal( "Style buttons", CRUDE_COMPOUNT( ImVec2, { pane_width, 0 } ), 1.0f );
  ImGui::TextUnformatted( "Values" );
  ImGui::Spring();
  if ( ImGui::Button( "Reset to defaults" ) )
  {
    *ax_editor_style = ax::NodeEditor::Style( );
  }

  ImGui::EndHorizontal( );
  ImGui::Spacing( );
  ImGui::DragFloat4( "Node Padding", &ax_editor_style->NodePadding.x, 0.1f, 0.0f, 40.0f );
  ImGui::DragFloat( "Node Rounding", &ax_editor_style->NodeRounding, 0.1f, 0.0f, 40.0f );
  ImGui::DragFloat( "Node Border Width", &ax_editor_style->NodeBorderWidth, 0.1f, 0.0f, 15.0f );
  ImGui::DragFloat( "Hovered Node Border Width", &ax_editor_style->HoveredNodeBorderWidth, 0.1f, 0.0f, 15.0f );
  ImGui::DragFloat( "Hovered Node Border Offset", &ax_editor_style->HoverNodeBorderOffset, 0.1f, -40.0f, 40.0f );
  ImGui::DragFloat( "Selected Node Border Width", &ax_editor_style->SelectedNodeBorderWidth, 0.1f, 0.0f, 15.0f );
  ImGui::DragFloat( "Selected Node Border Offset", &ax_editor_style->SelectedNodeBorderOffset, 0.1f, -40.0f, 40.0f );
  ImGui::DragFloat( "Pin Rounding", &ax_editor_style->PinRounding, 0.1f, 0.0f, 40.0f );
  ImGui::DragFloat( "Pin Border Width", &ax_editor_style->PinBorderWidth, 0.1f, 0.0f, 15.0f );
  ImGui::DragFloat( "Link Strength", &ax_editor_style->LinkStrength, 1.0f, 0.0f, 500.0f );
  ImGui::DragFloat( "Scroll Duration", &ax_editor_style->ScrollDuration, 0.001f, 0.0f, 2.0f );
  ImGui::DragFloat( "Flow Marker Distance", &ax_editor_style->FlowMarkerDistance, 1.0f, 1.0f, 200.0f );
  ImGui::DragFloat( "Flow Speed", &ax_editor_style->FlowSpeed, 1.0f, 1.0f, 2000.0f );
  ImGui::DragFloat( "Flow Duration", &ax_editor_style->FlowDuration, 0.001f, 0.0f, 5.0f );
  ImGui::DragFloat( "Group Rounding", &ax_editor_style->GroupRounding, 0.1f, 0.0f, 40.0f );
  ImGui::DragFloat( "Group Border Width", &ax_editor_style->GroupBorderWidth, 0.1f, 0.0f, 15.0f );

  ImGui::Separator( );

  ImGui::BeginHorizontal( "Color Mode", ImVec2(pane_width, 0), 1.0f );
  ImGui::TextUnformatted( "Filter Colors" );
  ImGui::Spring( );
  ImGui::RadioButton( "RGB", &im_edit_mode, ImGuiColorEditFlags_DisplayRGB );
  ImGui::Spring( 0 );
  ImGui::RadioButton( "HSV", &im_edit_mode, ImGuiColorEditFlags_DisplayHSV );
  ImGui::Spring( 0 );
  ImGui::RadioButton( "HEX", &im_edit_mode, ImGuiColorEditFlags_DisplayHex );
  ImGui::EndHorizontal( );

  im_filter.Draw( "##filter", pane_width );

  ImGui::Spacing( );

  ImGui::PushItemWidth( -160 );
  for ( uint32 i = 0; i < ax::NodeEditor::StyleColor_Count; ++i )
  {
    char const *name = ax::NodeEditor::GetStyleColorName( CRUDE_CAST( ax::NodeEditor::StyleColor, i ) );
    if ( !im_filter.PassFilter( name ) )
    {
      continue;
    }

    ImGui::ColorEdit4( name, &ax_editor_style->Colors[ i ].x, im_edit_mode );
  }

  ImGui::PopItemWidth( );

  ImGui::End( );
}

void
crude_gui_blueprint_queue_draw_left_pane_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ float32                                             pane_width
)
{
  static int                                               change_count = 0;
  static bool                                              should_queue_draw_style_editor = false;
  
  ax::NodeEditor::NodeId                                  *ax_selected_nodes;
  ax::NodeEditor::LinkId                                  *ax_selected_links;
  ImGuiStyle                                              *im_style;
  ImGuiIO                                                 *im_io;
  crude_gfx_texture                                       *save_icon_texture;
  crude_gfx_texture                                       *restore_icon_texture;
  ImDrawList                                              *im_draw_list;
  int32                                                    node_count, link_count;
  int32                                                    save_icon_width, save_icon_height, restore_icon_width, restore_icon_height;
  uint64                                                   temporary_allocator_marker;

  temporary_allocator_marker = crude_stack_allocator_get_marker( blueprint->temporary_allocator );

  im_io = &ImGui::GetIO();
  im_style = &ImGui::GetStyle( );

  ImGui::BeginChild( "Selection", ImVec2( pane_width, 0 ) );

  pane_width = ImGui::GetContentRegionAvail( ).x;

  ImGui::BeginHorizontal( "Style Editor", ImVec2( pane_width, 0 ) );
  ImGui::Spring( 0.0f, 0.0f );
  if ( ImGui::Button( "Zoom to Content" ) )
  {
    ax::NodeEditor::NavigateToContent( );
  }
  
  ImGui::Spring( 0.0f );
  if ( ImGui::Button( "Show Flow" ) )
  {
    for ( uint64 i = 0; i < CRUDE_ARRAY_LENGTH( blueprint->links ); ++i )
    {
      ax::NodeEditor::Flow( blueprint->links[ i ].id );
    }
  }

  ImGui::Spring( );
  
  if ( ImGui::Button( "Edit Style" ) )
  {
    should_queue_draw_style_editor = true;
  }

  ImGui::EndHorizontal( );
  ImGui::Checkbox( "Show Ordinals", &blueprint->show_ordinals );

  if ( should_queue_draw_style_editor )
  {
    crude_gui_blueprint_queue_draw_style_editor_( &should_queue_draw_style_editor );
  }

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( ax_selected_nodes, ax::NodeEditor::GetSelectedObjectCount( ), crude_stack_allocator_pack( blueprint->temporary_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( ax_selected_links, ax::NodeEditor::GetSelectedObjectCount( ), crude_stack_allocator_pack( blueprint->temporary_allocator ) );

  node_count = ax::NodeEditor::GetSelectedNodes( ax_selected_nodes, ax::NodeEditor::GetSelectedObjectCount( ) );
  link_count = ax::NodeEditor::GetSelectedLinks( ax_selected_links, ax::NodeEditor::GetSelectedObjectCount( ) );

  CRUDE_ARRAY_SET_LENGTH( ax_selected_nodes, node_count );
  CRUDE_ARRAY_SET_LENGTH( ax_selected_links, link_count );

  save_icon_texture = crude_gfx_access_texture( blueprint->gpu, blueprint->save_icon_texture_handle );
  restore_icon_texture = crude_gfx_access_texture( blueprint->gpu, blueprint->restore_icon_texture_handle );
  save_icon_width = save_icon_texture ? save_icon_texture->width : 100;
  save_icon_height = save_icon_texture ? save_icon_texture->width : 100;
  restore_icon_width = restore_icon_texture ? restore_icon_texture->width : 0;
  restore_icon_height = restore_icon_texture ? restore_icon_texture->width : 0;

  ImGui::GetWindowDrawList( )->AddRectFilled(
    ImGui::GetCursorScreenPos( ),
    ImGui::GetCursorScreenPos( ) + ImVec2( pane_width, ImGui::GetTextLineHeight( ) ),
    ImColor( im_style->Colors[ ImGuiCol_HeaderActive ] ), ImGui::GetTextLineHeight( ) * 0.25f );

  ImGui::Spacing( );
  ImGui::SameLine( );
  ImGui::TextUnformatted( "Nodes" );
  ImGui::Indent( );
  
  for ( uint64 node_index = 0; node_index < CRUDE_ARRAY_LENGTH( blueprint->nodes ); ++node_index )
  {
    char                                                   node_id_str[ 128 ];
    char                                                   node_name_with_id[ 512 ];
    crude_gui_blueprint_node                              *node;
    ImVec2                                                 im_icon_panel_pos, im_text_size, im_start_position;
    float32                                                touch_progress;
    bool                                                   is_node_selected;

    node = &blueprint->nodes[ node_index ];
    ImGui::PushID( node->id.AsPointer( ) );

    im_start_position = ImGui::GetCursorScreenPos();

    touch_progress = crude_gui_blueprint_get_touch_progress_( blueprint, node->id );
    if ( touch_progress )
    {
      ImGui::GetWindowDrawList()->AddLine(
        im_start_position + ImVec2( -8, 0 ),
        im_start_position + ImVec2( -8, ImGui::GetTextLineHeight( ) ),
        IM_COL32( 255, 0, 0, 255 - CRUDE_CAST( int32, 255 * touch_progress ) ), 4.0f );
    }

    is_node_selected = false;
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( ax_selected_nodes ); ++i )
    {
      if ( ax_selected_nodes[ i ] == node->id )
      {
        is_node_selected = true;
        break;
      }
    }

    ImGui::SetNextItemAllowOverlap( );
    
    crude_snprintf( node_name_with_id, sizeof( node_name_with_id ), "%s##%u", node->name, CRUDE_CAST( uint64, node->id.AsPointer( ) ) );

    if ( ImGui::Selectable( node_name_with_id, &is_node_selected ) )
    {
      if ( im_io->KeyCtrl )
      {
        if ( is_node_selected )
        {
          ax::NodeEditor::SelectNode( node->id, true );
        }
        else
        {
          ax::NodeEditor::DeselectNode( node->id );
        }
      }
      else
      {
        ax::NodeEditor::SelectNode( node->id, false );
      }

      ax::NodeEditor::NavigateToSelection( );
    }

    if ( ImGui::IsItemHovered( ) && node->state[ 0 ] )
    {
      ImGui::SetTooltip( "State: %s", node->state );
    }

    crude_snprintf( node_id_str, sizeof( node_id_str ), "(%u)", CRUDE_CAST( uint64, node->id.AsPointer( ) ) );
    im_text_size = ImGui::CalcTextSize( node_id_str, NULL );

    im_icon_panel_pos = im_start_position + ImVec2(
      pane_width - im_style->FramePadding.x - im_style->IndentSpacing - save_icon_width - restore_icon_width - im_style->ItemInnerSpacing.x * 1,
      ( ImGui::GetTextLineHeight( ) - save_icon_height ) / 2.f );

    ImGui::GetWindowDrawList()->AddText(
      ImVec2( im_icon_panel_pos.x - im_text_size.x - im_style->ItemInnerSpacing.x, im_start_position.y ),
      IM_COL32( 255, 255, 255, 255 ), node_id_str, NULL );

    im_draw_list = ImGui::GetWindowDrawList( );
    ImGui::SetCursorScreenPos( im_icon_panel_pos );
    ImGui::SetNextItemAllowOverlap( );
    
    if ( !node->saved_state[ 0 ] )
    {
      if ( ImGui::InvisibleButton( "save", ImVec2( save_icon_width, save_icon_height ) ) )
      {
        crude_string_copy_unknow_length( node->saved_state, node->state, sizeof( node->saved_state ) );
      }
      
      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( blueprint->save_icon_texture_handle ) )
      {
        if ( ImGui::IsItemActive( ) )
        {
          im_draw_list->AddImage( CRUDE_CAST( ImTextureRef, &blueprint->save_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
        }
        else if (ImGui::IsItemHovered())
        {
          im_draw_list->AddImage( CRUDE_CAST( ImTextureRef, &blueprint->save_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
        }
        else
        {
          im_draw_list->AddImage( CRUDE_CAST( ImTextureRef, &blueprint->save_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
        }
      }
    }
    else
    {
      ImGui::Dummy( ImVec2( save_icon_width, save_icon_height ) );
      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( blueprint->save_icon_texture_handle ) )
      {
        im_draw_list->AddImage( CRUDE_CAST( ImTextureRef, &blueprint->save_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
      }
    }

    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
    ImGui::SetNextItemAllowOverlap();
    if ( node->saved_state[ 0 ] )
    {
      if ( ImGui::InvisibleButton( "restore", ImVec2( restore_icon_width, restore_icon_height ) ) )
      {
        crude_string_copy_unknow_length( node->state, node->saved_state, sizeof( node->state ) );
        ax::NodeEditor::RestoreNodeState( node->id );
        node->saved_state[ 0 ] = 0;
      }

      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( blueprint->restore_icon_texture_handle ) )
      {
        if (ImGui::IsItemActive())
        {
          im_draw_list->AddImage( CRUDE_CAST( ImTextureRef, &blueprint->restore_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
        }
        else if (ImGui::IsItemHovered())
        {
          im_draw_list->AddImage( CRUDE_CAST( ImTextureRef, &blueprint->restore_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
        }
        else
        {
          im_draw_list->AddImage( CRUDE_CAST( ImTextureRef, &blueprint->restore_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
        }
      }
    }
    else
    {
      ImGui::Dummy( ImVec2( restore_icon_width, restore_icon_height ) );
      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( blueprint->restore_icon_texture_handle ) )
      {
        im_draw_list->AddImage( CRUDE_CAST( ImTextureRef, &blueprint->restore_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
      }
    }

    ImGui::SameLine( 0, 0 );
    ImGui::Dummy( ImVec2( 0, restore_icon_height ) );

    ImGui::PopID();
  }
  
  ImGui::Unindent( );

  ImGui::GetWindowDrawList( )->AddRectFilled(
    ImGui::GetCursorScreenPos( ),
    ImGui::GetCursorScreenPos( ) + ImVec2(pane_width, ImGui::GetTextLineHeight( ) ),
    ImColor( ImGui::GetStyle( ).Colors[ ImGuiCol_HeaderActive ] ), ImGui::GetTextLineHeight( ) * 0.25f);

  ImGui::Spacing( );
  ImGui::SameLine( );
  ImGui::TextUnformatted( "Selection" );

  ImGui::BeginHorizontal( "Selection Stats", ImVec2( pane_width, 0 ) );
  ImGui::Text( "Changed %d time%s", change_count, change_count > 1 ? "s" : "" );
  ImGui::Spring( );
  if ( ImGui::Button( "Deselect All" ) )
  {
    ax::NodeEditor::ClearSelection( );
  }

  ImGui::EndHorizontal( );
  ImGui::Indent( );
  for ( int32 i = 0; i < node_count; ++i )
  {
    ImGui::Text( "Node (%p)", ax_selected_nodes[ i ].AsPointer( ) );
  }
  for ( int32 i = 0; i < link_count; ++i )
  {
    ImGui::Text( "Link (%p)", ax_selected_nodes[ i ].AsPointer( ) );
  }
  ImGui::Unindent( );

  if ( ImGui::IsKeyPressed( ImGuiKey_Z ) )
  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( blueprint->links ); ++i )
    {
      ax::NodeEditor::Flow( blueprint->links[ i ].id );
    }
  }

  if ( ax::NodeEditor::HasSelectionChanged( ) )
  {
    ++change_count;
  }

  ImGui::EndChild();
  
cleanup:
  crude_stack_allocator_free_marker( blueprint->temporary_allocator, temporary_allocator_marker );
}

void
crude_gui_blueprint_build_node_
(
  _In_ crude_gui_blueprint                                *blueprint,
  _In_ crude_gui_blueprint_node                           *node
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( node->inputs ); ++i )
  {
    node->inputs[ i ].node = node;
    node->inputs[ i ].kind = CRUDE_GUI_BLUEPRINT_PIN_KIND_INPUT;
  }
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( node->outputs ); ++i )
  {
    node->outputs[ i ].node = node;
    node->outputs[ i ].kind = CRUDE_GUI_BLUEPRINT_PIN_KIND_OUTPUT;
  }
}