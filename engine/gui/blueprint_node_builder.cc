#include <engine/graphics/gpu_device.h>

#include <engine/gui/blueprint_node_builder.h>

CRUDE_API bool
crude_gui_blueprint_node_builder_set_stage_
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ crude_gui_blueprint_node_builder_stage              stage
);

CRUDE_API void
crude_gui_blueprint_node_builder_pin_
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ ax::NodeEditor::PinId                               id,
  _In_ ax::NodeEditor::PinKind                             kind
);

CRUDE_API void
crude_gui_blueprint_node_builder_end_pin_
(
  _In_ crude_gui_blueprint_node_builder                   *builder
);

void
crude_gui_blueprint_node_builder_initialize
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ crude_gfx_device                                   *gpu
)
{
  *builder = CRUDE_COMPOUNT_EMPTY( crude_gui_blueprint_node_builder );
  builder->header_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  builder->gpu = gpu;
}

void
crude_gui_blueprint_node_builder_begin
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ ax::NodeEditor::NodeId                              id
)
{
  builder->has_header = false;
  builder->header_min = builder->header_max = ImVec2();

  ax::NodeEditor::PushStyleVar( ax::NodeEditor::StyleVar_NodePadding, ImVec4( 8, 4, 8, 8 ) );
  ax::NodeEditor::BeginNode( id );

  ImGui::PushID( id.AsPointer( ) );
  builder->current_node_id = id;

  crude_gui_blueprint_node_builder_set_stage_( builder, CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_BEGIN );
}

void
crude_gui_blueprint_node_builder_end
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ ax::NodeEditor::NodeId                              id
)
{
  crude_gui_blueprint_node_builder_set_stage_( builder, CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_END );

  ax::NodeEditor::EndNode();

  if ( ImGui::IsItemVisible( ) )
  {
    int32 alpha = 255 * ImGui::GetStyle().Alpha;

    ImDrawList *draw_list = ax::NodeEditor::GetNodeBackgroundDrawList( builder->current_node_id );

    float32 half_border_width = ax::NodeEditor::GetStyle( ).NodeBorderWidth * 0.5f;

    ImU32 header_color = IM_COL32( 0, 0, 0, alpha ) | ( builder->header_color & IM_COL32( 255, 255, 255, 0 ) );
    if ( ( builder->header_max.x > builder->header_min.x ) && ( builder->header_max.y > builder->header_min.y ) && CRUDE_RESOURCE_HANDLE_IS_VALID( builder->header_texture_handle ) )
    {
      crude_gfx_texture *texture = crude_gfx_access_texture( builder->gpu, builder->header_texture_handle );

      ImVec2 uv = ImVec2(
        ( builder->header_max.x - builder->header_min.x ) / ( float32 )( 4.0f * texture->width ),
        ( builder->header_max.y - builder->header_min.y ) / ( float32 )( 4.0f * texture->height ) );

      ImTextureRef im_texture_ref = ImTextureRef { &builder->header_texture_handle.index };

      draw_list->AddImageRounded( im_texture_ref,
        builder->header_min - ImVec2(8 - half_border_width, 4 - half_border_width),
        builder->header_max + ImVec2(8 - half_border_width, 0),
        ImVec2(0.0f, 0.0f), uv,
        builder->header_color, ax::NodeEditor::GetStyle().NodeRounding, ImDrawFlags_RoundCornersTop);

      if ( builder->content_min.y > builder->header_max.y)
      {
        draw_list->AddLine(
          ImVec2(builder->header_min.x - (8 - half_border_width), builder->header_max.y - 0.5f),
          ImVec2(builder->header_max.x + (8 - half_border_width), builder->header_max.y - 0.5f),
          ImColor(255, 255, 255, 96 * alpha / (3 * 255)), 1.0f);
      }
    }

    builder->current_node_id = 0;

    ImGui::PopID();

    ax::NodeEditor::PopStyleVar();

    crude_gui_blueprint_node_builder_set_stage_( builder, CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_INVALID );
  }
}

void
crude_gui_blueprint_node_builder_header
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ XMFLOAT4                                           color
)
{
  builder->header_color = ImColor{ color.x, color.y, color.z, color.w };
  crude_gui_blueprint_node_builder_set_stage_( builder, CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_HEADER );
}

void
crude_gui_blueprint_node_builder_end_header
(
  _In_ crude_gui_blueprint_node_builder                   *builder
)
{
  crude_gui_blueprint_node_builder_set_stage_( builder, CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_CONTENT );
}

void
crude_gui_blueprint_node_builder_input
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ ax::NodeEditor::PinId                               id
)
{
  if ( builder->current_stage == CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_BEGIN )
  {
    crude_gui_blueprint_node_builder_set_stage_( builder, CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_CONTENT );
  }

  bool apply_padding = ( builder->current_stage == CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_INPUT );
  
  crude_gui_blueprint_node_builder_set_stage_( builder, CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_INPUT );

  if ( apply_padding )
  {
    ImGui::Spring(0);
  }

  crude_gui_blueprint_node_builder_pin_( builder, id, ax::NodeEditor::PinKind::Input );

  ImGui::BeginHorizontal( id.AsPointer( ) );
}

void
crude_gui_blueprint_node_builder_end_input
(
  _In_ crude_gui_blueprint_node_builder                   *builder
)
{
  ImGui::EndHorizontal();
  
  crude_gui_blueprint_node_builder_end_pin_( builder );
}

void
crude_gui_blueprint_node_builder_middle
(
  _In_ crude_gui_blueprint_node_builder                   *builder
)
{
  if ( builder->current_stage == CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_BEGIN )
  {
    crude_gui_blueprint_node_builder_set_stage_( builder, CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_CONTENT );
  }
  
  crude_gui_blueprint_node_builder_set_stage_( builder, CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_MIDDLE );
}

void
crude_gui_blueprint_node_builder_output
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ ax::NodeEditor::PinId                               id
)
{
  if ( builder->current_stage == CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_BEGIN )
  {
    crude_gui_blueprint_node_builder_set_stage_( builder, CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_CONTENT );
  }

  bool apply_padding = ( builder->current_stage == CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_OUTPUT );
    
  crude_gui_blueprint_node_builder_set_stage_( builder, CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_OUTPUT );

  if ( apply_padding )
  {
    ImGui::Spring( 0 );
  }

  crude_gui_blueprint_node_builder_pin_( builder, id, ax::NodeEditor::PinKind::Output );

  ImGui::BeginHorizontal(id.AsPointer());
}

void
crude_gui_blueprint_node_builder_end_output
(
  _In_ crude_gui_blueprint_node_builder                   *builder
)
{
  ImGui::EndHorizontal();

  crude_gui_blueprint_node_builder_end_pin_( builder );
}

bool
crude_gui_blueprint_node_builder_set_stage_
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ crude_gui_blueprint_node_builder_stage              stage
)
{
  if ( stage == builder->current_stage )
  {
    return false;
  }

  crude_gui_blueprint_node_builder_stage old_stage = builder->current_stage;
  builder->current_stage = stage;

  ImVec2 cursor;
  switch ( old_stage )
  {
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_BEGIN:
  {
    break;
  }
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_HEADER:
  {
    ImGui::EndHorizontal();
    builder->header_min = ImGui::GetItemRectMin();
    builder->header_max = ImGui::GetItemRectMax();
    ImGui::Spring( 0, ImGui::GetStyle( ).ItemSpacing.y * 2.0f );
    break;
  }
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_CONTENT:
  {
    break;
  }
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_INPUT:
  {
    ax::NodeEditor::PopStyleVar(2);
    ImGui::Spring(1, 0);
    ImGui::EndVertical();
    break;
  }
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_MIDDLE:
  {
    ImGui::EndVertical();
    break;
  }
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_OUTPUT:
  {
    ax::NodeEditor::PopStyleVar( 2 );
    ImGui::Spring( 1, 0 );
    ImGui::EndVertical( );
    break;
  }
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_END:
  {
    break;
  }
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_INVALID:
  {
    break;
  }
  }

  switch ( stage )
  {
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_BEGIN:
  {
    ImGui::BeginVertical("node");
    break;
  }
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_HEADER:
  {
    builder->has_header = true;
    ImGui::BeginHorizontal("header");
    break;
  }
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_CONTENT:
  {
    if ( old_stage == CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_BEGIN )
    {
      ImGui::Spring( 0 );
    }

    ImGui::BeginHorizontal( "content" );
    ImGui::Spring( 0, 0 );
    break;
  }
  
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_INPUT:
  {
    ImGui::BeginVertical("inputs", ImVec2(0, 0), 0.0f);

    ax::NodeEditor::PushStyleVar( ax::NodeEditor::StyleVar_PivotAlignment, ImVec2( 0, 0.5f ) );
    ax::NodeEditor::PushStyleVar( ax::NodeEditor::StyleVar_PivotSize, ImVec2( 0, 0 ) );

    if ( !builder->has_header )
    {
      ImGui::Spring(1, 0);
    }
    break;
  }
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_MIDDLE:
  {
    ImGui::Spring( 1 );
    ImGui::BeginVertical( "middle", ImVec2(0, 0), 1.0f );
    break;
  }
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_OUTPUT:
  {
    if ( old_stage == CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_MIDDLE || old_stage == CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_INPUT )
    {
      ImGui::Spring(1);
    }
    else
    {
      ImGui::Spring(1, 0);
    }
    ImGui::BeginVertical("outputs", ImVec2(0, 0), 1.0f);

    ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PivotAlignment, ImVec2(1.0f, 0.5f));
    ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PivotSize, ImVec2(0, 0));

    if ( !builder->has_header )
    {
      ImGui::Spring(1, 0);
    }
    break;
  }
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_END:
  {
    if ( old_stage == CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_INPUT )
    {
      ImGui::Spring(1, 0);
    }

    if ( old_stage != CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_BEGIN )
    {
      ImGui::EndHorizontal();
    }

    builder->content_min = ImGui::GetItemRectMin();
    builder->content_max = ImGui::GetItemRectMax();

    //ImGui::Spring(0);
    ImGui::EndVertical();
    builder->node_min = ImGui::GetItemRectMin();
    builder->node_max = ImGui::GetItemRectMax();
    break;
  }
  case CRUDE_GUI_BLUEPRINT_NODE_BUILDER_STAGE_INVALID:
  {
    break;
  }
  }

  return true;
}

void
crude_gui_blueprint_node_builder_pin_
(
  _In_ crude_gui_blueprint_node_builder                   *builder,
  _In_ ax::NodeEditor::PinId                               id,
  _In_ ax::NodeEditor::PinKind                             kind
)
{
  ax::NodeEditor::BeginPin(id, kind);
}

void
crude_gui_blueprint_node_builder_end_pin_
(
  _In_ crude_gui_blueprint_node_builder                   *builder
)
{
  ax::NodeEditor::EndPin();
}
