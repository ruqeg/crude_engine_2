#include <engine/gui/viewport.h>

static char const* crude_gui_viewport_ratios_names_[ ] =
{
  "16/9",
  "16/10",
  "4/3"
};

static float32 crude_gui_viewport_ratios_[ ] =
{
  16 / 9.f,
  16 / 10.f,
  4 / 3.f
};

void
crude_gui_viewport_initialize
(
  _In_ crude_gui_viewport                                 *viewport,
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_texture_handle                            viewport_texture_handle,
  _In_ int32                                               viewport_ratio_index
)
{
  viewport->gpu = gpu;
  viewport->viewport_texture_handle = viewport_texture_handle;
  viewport->viewport_ratio_index = viewport_ratio_index;
  viewport->selected_gizmo_operation = ImGuizmo::TRANSLATE;
  viewport->should_draw_grid = false;
}

void
crude_gui_viewport_deinitialize
(
  _In_ crude_gui_viewport                                 *viewport
)
{
}

void
crude_gui_viewport_update
(
  _In_ crude_gui_viewport                                 *viewport
)
{
}

void
crude_gui_viewport_queue_draw
(
  _In_ crude_gui_viewport                                 *viewport,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        selected_node,
  _In_ crude_entity                                        camera_node
)
{
  crude_gfx_texture                                       *viewport_texture;
  ImVec2                                                   start_cursor_pos, viewport_pos, viewport_size;
  float32                                                  viewport_ratio;

  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( viewport->viewport_texture_handle ) )
  {
    return;
  }

  /* Viewport Texture */
  {
    start_cursor_pos = ImGui::GetCursorPos();

    viewport_texture = crude_gfx_access_texture( viewport->gpu, viewport->viewport_texture_handle );
    
    viewport_ratio = crude_gui_viewport_ratios_[ viewport->viewport_ratio_index ];
    
    viewport_size.x = ImGui::GetContentRegionAvail( ).x;
    viewport_size.y = viewport_size.x / viewport_ratio;

    viewport_pos.x = start_cursor_pos.x;
    viewport_pos.y = start_cursor_pos.y + ( ImGui::GetContentRegionAvail( ).y - viewport_size.y ) * 0.5f;

    ImGui::SetCursorPos( viewport_pos );
    ImGui::Image( CRUDE_CAST( ImTextureRef, &viewport->viewport_texture_handle.index ), viewport_size );
    
    ImGui::SetCursorPos( start_cursor_pos );
    if ( ImGui::BeginCombo( "Ratio", crude_gui_viewport_ratios_names_[ viewport->viewport_ratio_index ], ImGuiComboFlags_WidthFitPreview ) )
    {
      for ( uint32 i = 0; i < CRUDE_COUNTOF( crude_gui_viewport_ratios_names_ ); i++ )
      {
        bool is_selected = ( viewport->viewport_ratio_index == i );
        if ( ImGui::Selectable( crude_gui_viewport_ratios_names_[ i ], is_selected ) )
        {
          viewport->viewport_ratio_index = i;
        }
        
        if ( is_selected )
        {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
  }
  
  /* Viewport Imguizmo */
  if ( crude_entity_valid( world, selected_node ) && crude_entity_valid( world, camera_node ) && CRUDE_ENTITY_HAS_COMPONENT( world, selected_node, crude_transform ) )
  {
    crude_camera                                            *camera;
    crude_transform                                         *camera_transform;
    crude_transform                                         *selected_node_transform;
    crude_transform                                         *selected_node_parent_transform;
    crude_entity                                             selected_node_parent;
    XMFLOAT4X4                                               camera_world_to_view, camera_view_to_clip, selected_node_to_parent, selected_parent_to_camera_view;
    XMVECTOR                                                 new_scale, new_translation, new_rotation_quat;
    
    selected_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_transform );
    camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, camera_node, crude_camera  );
    camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, camera_node, crude_transform  );
    selected_node_parent = crude_entity_get_parent( world, selected_node );
    
    if ( ImGui::IsKeyPressed( ImGuiKey_Z ) )
    {
      viewport->selected_gizmo_operation = ImGuizmo::TRANSLATE;
    }
    
    if ( ImGui::IsKeyPressed( ImGuiKey_X ) )
    {
      viewport->selected_gizmo_operation = ImGuizmo::ROTATE;
    }
    if ( ImGui::IsKeyPressed( ImGuiKey_C ) )
    {
      viewport->selected_gizmo_operation = ImGuizmo::SCALE;
    }
    
    if ( ImGui::RadioButton( "Translate", viewport->selected_gizmo_operation == ImGuizmo::TRANSLATE ) )
    {
      viewport->selected_gizmo_operation = ImGuizmo::TRANSLATE;
    }
    
    ImGui::SameLine();
    
    if ( ImGui::RadioButton( "Rotate", viewport->selected_gizmo_operation == ImGuizmo::ROTATE ) )
    {
      viewport->selected_gizmo_operation = ImGuizmo::ROTATE;
    }
    ImGui::SameLine();
    if ( ImGui::RadioButton( "Scale", viewport->selected_gizmo_operation == ImGuizmo::SCALE ) )
    {
      viewport->selected_gizmo_operation = ImGuizmo::SCALE;
    }
    
    ImGui::SameLine();

    ImGui::Checkbox( "Grid", &viewport->should_draw_grid );
    
    ImGui::SetCursorPos( viewport_pos );
    ImGuizmo::SetDrawlist( );
    ImGuizmo::SetRect(ImGui::GetWindowPos().x + viewport_pos.x, ImGui::GetWindowPos().y + viewport_pos.y, viewport_size.x, viewport_size.y);
    
    if ( crude_entity_valid( world, selected_node_parent ) )
    {
      selected_node_parent_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node_parent, crude_transform  );
    }
    else
    {
      selected_node_parent_transform = NULL;
    }
    
    if ( selected_node_parent_transform )
    {
      XMStoreFloat4x4( &selected_parent_to_camera_view, DirectX::XMMatrixMultiply( crude_transform_node_to_world( world, selected_node_parent, selected_node_parent_transform ), XMMatrixInverse( NULL, crude_transform_node_to_world( world, camera_node, camera_transform ) ) ) );
    }
    else
    {
      XMStoreFloat4x4( &selected_parent_to_camera_view, XMMatrixIdentity( ) );
    }
    
    XMStoreFloat4x4( &camera_view_to_clip, crude_camera_view_to_clip( camera ) );
    XMStoreFloat4x4( &selected_node_to_parent, crude_transform_node_to_parent( selected_node_transform ) );
    
    ImGuizmo::SetID( 0 );
    if ( ImGuizmo::Manipulate( &selected_parent_to_camera_view._11, &camera_view_to_clip._11, viewport->selected_gizmo_operation, ImGuizmo::MODE::WORLD, &selected_node_to_parent._11, NULL, NULL ) )
    {
      XMMatrixDecompose( &new_scale, &new_rotation_quat, &new_translation, XMLoadFloat4x4( &selected_node_to_parent ) );
    
      XMStoreFloat4( &selected_node_transform->rotation, new_rotation_quat );
      XMStoreFloat3( &selected_node_transform->scale, new_scale );
      XMStoreFloat3( &selected_node_transform->translation, new_translation );
    
      CRUDE_ENTITY_COMPONENT_MODIFIED( world, selected_node, crude_transform );
    }

    if ( viewport->should_draw_grid )
    {
      XMFLOAT4X4 identity_matrix;
      XMStoreFloat4x4( &camera_world_to_view, XMMatrixInverse( NULL, crude_transform_node_to_world( world, camera_node, camera_transform ) ) );
      XMStoreFloat4x4( &identity_matrix, XMMatrixIdentity( ) );
      
      ImGuizmo::DrawGrid( &camera_world_to_view._11, &camera_view_to_clip._11, &identity_matrix._11, 100.f );
    }
  }

}