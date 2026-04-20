#include <engine/graphics/imgui.h>

#include <engine/gui/texture_inspector.h>

#if CRUDE_DEVELOP

void
crude_gui_texture_inspector_initialize
(
  _In_ crude_gui_texture_inspector                        *inspector,
  _In_ crude_gfx_render_graph_builder                     *render_graph_builder
)
{
  inspector->render_graph_builder = render_graph_builder;
  inspector->gpu = render_graph_builder->gpu;
  inspector->texture_handle = crude_gfx_access_texture( inspector->gpu, crude_gfx_render_graph_builder_access_resource_by_name( inspector->render_graph_builder, "game_final" )->resource_info.texture.handle )->handle;
}

void
crude_gui_texture_inspector_deinitialize
(
  _In_ crude_gui_texture_inspector                        *inspector
)
{
}

void
crude_gui_texture_inspector_update
(
  _In_ crude_gui_texture_inspector                        *inspector
)
{
}

void
crude_gui_texture_inspector_queue_draw
(
  _In_ crude_gui_texture_inspector                        *inspector
)
{
  char const                                              *preview_texture_name;
  uint32                                                   id;
  
  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( inspector->texture_handle ) )
  {
    ImGui::Image( CRUDE_CAST( ImTextureRef, &inspector->texture_handle.index ), ImGui::GetContentRegionAvail( ) );
  }

  ImGui::SetCursorPos( ImGui::GetWindowContentRegionMin( ) );
  
  preview_texture_name = "Unknown";
  id = 0;

  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( inspector->texture_handle ) )
  {
    crude_gfx_texture *selected_texture = crude_gfx_access_texture( inspector->gpu, inspector->texture_handle );
    if ( selected_texture && selected_texture->name[ 0 ] )
    {
      preview_texture_name = selected_texture->name;
    };
  }
  
  if ( ImGui::BeginCombo( "Texture ID", preview_texture_name ) )
  {
    for ( uint32 t = 0; t < inspector->gpu->textures.pool_size; ++t )
    {
      crude_gfx_texture                                   *texture;
      crude_gfx_texture_handle                             texture_handle;
      bool                                                 is_selected;
  
      texture_handle = CRUDE_CAST( crude_gfx_texture_handle, t );
      if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( texture_handle ) )
      {
        continue;
      }
      
      texture = crude_gfx_access_texture( inspector->gpu, texture_handle );
      if ( !texture || !texture->name )
      {
        continue;
      }
      
      ImGui::PushID( id++ );
  
      is_selected = ( inspector->texture_handle.index == texture_handle.index );
      if ( ImGui::Selectable( texture->name ) )
      {
        inspector->texture_handle = texture_handle;
      }
      
      ImGui::PopID( );
      if ( is_selected )
      {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
}

#endif /* CRUDE_DEVELOP */