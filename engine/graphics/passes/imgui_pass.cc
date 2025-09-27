#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <graphics/scene_renderer.h>
#include <core/hash_map.h>

#include <graphics/passes/imgui_pass.h>

static uint32 vertex_buffer_size_ = 665536;
static uint32 index_buffer_size_ = 665536;

void
crude_gfx_imgui_pass_initialize
(
  _In_ crude_gfx_imgui_pass                               *pass,
  crude_gfx_scene_renderer                                *scene_renderer
)
{
  crude_gfx_device                                        *gpu;
  ImGuiIO                                                 *imgui_io;
  uint8                                                   *font_pixels;
  crude_gfx_buffer_creation                                cb_creation;
  crude_gfx_buffer_creation                                vertex_buffer_creation;
  crude_gfx_buffer_creation                                index_buffer_creation;
  int32                                                    font_width, font_height;

  pass->scene_renderer = scene_renderer;
  gpu = scene_renderer->renderer->gpu;

  ImGui::SetCurrentContext( ( ImGuiContext* )pass->scene_renderer->imgui_context );
  
  /* Setup Platform/Renderer bindings */
  ImGui_ImplSDL3_InitForVulkan( gpu->sdl_window );
  
  imgui_io = &ImGui::GetIO();
  imgui_io->BackendRendererName = "Raptor_ImGui";
  imgui_io->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
  
  /* Load font texture atlas */
  imgui_io->Fonts->GetTexDataAsRGBA32( &font_pixels, &font_width, &font_height );
  
  crude_gfx_texture_creation texture_creation = crude_gfx_texture_creation_empty();
  texture_creation.format = VK_FORMAT_R8G8B8A8_UNORM;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.initial_data = font_pixels;
  texture_creation.width = font_width;
  texture_creation.height = font_height;
  texture_creation.depth = 1;
  texture_creation.name = "imgui_font";
  pass->font_texture = crude_gfx_create_texture( gpu, &texture_creation );
  
  /* Store our identifier */
  imgui_io->Fonts->TexID = CRUDE_CAST( ImTextureID, &pass->font_texture );
  
  /* Create constant buffer */
  cb_creation;
  cb_creation.type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  cb_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  cb_creation.size = 64;
  cb_creation.name = "cb_imgui";
  pass->ui_cb = crude_gfx_create_buffer( gpu, &cb_creation );
  
  /* Create vertex and index buffers */
  vertex_buffer_creation = crude_gfx_buffer_creation_empty();
  vertex_buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  vertex_buffer_creation.type_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  vertex_buffer_creation.size = vertex_buffer_size_;
  vertex_buffer_creation.name = "vertex_buffer_imgui";
  pass->vertex_buffer = crude_gfx_create_buffer( gpu, &vertex_buffer_creation );
  
  index_buffer_creation = crude_gfx_buffer_creation_empty();
  index_buffer_creation.type_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  index_buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  index_buffer_creation.size = index_buffer_size_;
  index_buffer_creation.name = "index_buffer_imgui";
  pass->index_buffer = crude_gfx_create_buffer( gpu, &index_buffer_creation );

  pass->imgui_ds = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
  crude_gfx_imgui_pass_on_techniques_reloaded( pass );
}

void
crude_gfx_imgui_pass_deinitialize
(
  _In_ crude_gfx_imgui_pass                               *pass
)
{

  ImGui::SetCurrentContext( ( ImGuiContext* )pass->scene_renderer->imgui_context );
  ImGui_ImplSDL3_Shutdown( );

  crude_gfx_destroy_descriptor_set( pass->scene_renderer->renderer->gpu, pass->imgui_ds );

  crude_gfx_destroy_buffer( pass->scene_renderer->renderer->gpu, pass->vertex_buffer );
  crude_gfx_destroy_buffer( pass->scene_renderer->renderer->gpu, pass->index_buffer );
  crude_gfx_destroy_buffer( pass->scene_renderer->renderer->gpu, pass->ui_cb );
  
  crude_gfx_destroy_texture( pass->scene_renderer->renderer->gpu, pass->font_texture );
}

void
crude_gfx_imgui_pass_pre_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_imgui_pass                                    *pass;
  crude_gfx_device                                        *gpu;
  ImDrawData                                              *imgui_draw_data;
  int32                                                    draw_counts;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_imgui_pass*, ctx );
  
  gpu = pass->scene_renderer->renderer->gpu;
  
  ImGui::SetCurrentContext( ( ImGuiContext* )pass->scene_renderer->imgui_context );
  
  ImGui::Render();
  imgui_draw_data = ImGui::GetDrawData();
  
  draw_counts = imgui_draw_data->CmdListsCount;
  for ( int32 n = 0; n < draw_counts; ++n )
  {
    ImDrawList const *cmd_list = imgui_draw_data->CmdLists[ n ];
    for ( int32 cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i )
    {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[ cmd_i ];
      crude_gfx_texture_handle texture_handle = *( crude_gfx_texture_handle* )( pcmd->TexRef.GetTexID() );
      if ( texture_handle.index != pass->font_texture.index )
      {
        crude_gfx_texture *texture = crude_gfx_access_texture( gpu, texture_handle );
        crude_gfx_cmd_add_image_barrier( primary_cmd, texture, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0u, 1u, crude_gfx_has_depth( texture->vk_format ) );
      }
    }
  }
}

void
crude_gfx_imgui_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  ImDrawVert                                              *vtx_dst;
  ImDrawIdx                                               *idx_dst;
  crude_gfx_imgui_pass                                    *pass;
  crude_gfx_device                                        *gpu;
  ImDrawData                                              *imgui_draw_data;
  float32                                                 *cb_data;
  crude_gfx_pipeline_handle                                imgui_pipeline;
  crude_gfx_viewport                                       dev_viewport;
  crude_gfx_map_buffer_parameters                          map_parameters;
  XMFLOAT4X4                                               ortho_projection;
  ImVec2                                                   clip_off, clip_scale;
  int32                                                    draw_counts, framebuffer_width, framebuffer_height;
  float32                                                  l, r, t, b;
  size_t                                                   vertex_size, index_size;
  bool                                                     clip_origin_lower_left;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_imgui_pass*, ctx );
  
  gpu = pass->scene_renderer->renderer->gpu;

  ImGui::SetCurrentContext( ( ImGuiContext* )pass->scene_renderer->imgui_context );
  
  imgui_draw_data = ImGui::GetDrawData();
  
  /* Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates) */
  framebuffer_width = (int)( imgui_draw_data->DisplaySize.x * imgui_draw_data->FramebufferScale.x );
  framebuffer_height = (int)( imgui_draw_data->DisplaySize.y * imgui_draw_data->FramebufferScale.y );
  if ( framebuffer_width <= 0 || framebuffer_height <= 0 )
  {
    return;
  }
  
  /* Vulkan backend has a different origin than OpenGL. */
  clip_origin_lower_left = false;
  vertex_size = imgui_draw_data->TotalVtxCount * sizeof( ImDrawVert );
  index_size = imgui_draw_data->TotalIdxCount * sizeof( ImDrawIdx );
  
  if ( vertex_size >= vertex_buffer_size_ || index_size >= index_buffer_size_ )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "ImGui Backend Error: vertex/index overflow!" );
    return;
  }
  
  if ( vertex_size == 0 && index_size == 0 )
  {
    return;
  }
  
  /* Upload data */
  vtx_dst = NULL;
  idx_dst = NULL;
  
  map_parameters = CRUDE_COMPOUNT( crude_gfx_map_buffer_parameters, { pass->vertex_buffer, 0, ( uint32 )vertex_size } );
  vtx_dst = CRUDE_REINTERPRET_CAST( ImDrawVert*, crude_gfx_map_buffer( gpu, &map_parameters ) );
  
  if ( vtx_dst )
  {
    for ( int i = 0; i < imgui_draw_data->CmdListsCount; i++ )
    {
      ImDrawList const* cmd_list = imgui_draw_data->CmdLists[ i ];
      memcpy( vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof( ImDrawVert ) );
      vtx_dst += cmd_list->VtxBuffer.Size;
    }
    
    crude_gfx_unmap_buffer( gpu, map_parameters.buffer );
  }
  
  map_parameters = CRUDE_COMPOUNT( crude_gfx_map_buffer_parameters, { pass->index_buffer, 0, ( uint32 )index_size } );
  idx_dst = CRUDE_REINTERPRET_CAST( ImDrawIdx*, crude_gfx_map_buffer( gpu, &map_parameters ) );
  
  if ( idx_dst )
  {
    for ( int i = 0; i < imgui_draw_data->CmdListsCount; i++ )
    {
      ImDrawList const* cmd_list = imgui_draw_data->CmdLists[ i ];
      memcpy( idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof( ImDrawIdx ) );
      idx_dst += cmd_list->IdxBuffer.Size;
    }
    
    crude_gfx_unmap_buffer( gpu, map_parameters.buffer );
  }
  
  // !TODO add the sorting
  imgui_pipeline = crude_gfx_renderer_access_technique_pass_by_name(pass->scene_renderer->renderer, "imgui", "imgui" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, imgui_pipeline );
  crude_gfx_cmd_bind_vertex_buffer( primary_cmd, pass->vertex_buffer, 0u, 0u );
  crude_gfx_cmd_bind_index_buffer( primary_cmd, pass->index_buffer, 0u );
  
  dev_viewport = { 0, 0, ( uint16 )framebuffer_width, ( uint16 )framebuffer_height, 0.0f, 1.0f };
  crude_gfx_cmd_set_viewport( primary_cmd, &dev_viewport ); 
  
  /* Setup dev_viewport, orthographic projection matrix */
  /* Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is typically (0,0) for single dev_viewport apps. */
  l = imgui_draw_data->DisplayPos.x;
  r = imgui_draw_data->DisplayPos.x + imgui_draw_data->DisplaySize.x;
  t = imgui_draw_data->DisplayPos.y;
  b = imgui_draw_data->DisplayPos.y + imgui_draw_data->DisplaySize.y;
  
  XMStoreFloat4x4( &ortho_projection, XMMatrixSet(
    2.0f / ( r - l ), 0.0f, 0.0f, 0.0f,
    0.0f, 2.0f / ( t - b ), 0.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 0.0f,
    ( r + l ) / ( l - r ), ( t + b ) / ( b - t ), 0.0f, 1.0f
  ) );
  
  map_parameters = CRUDE_COMPOUNT( crude_gfx_map_buffer_parameters, { pass->ui_cb, 0, 0 } );
  cb_data = ( float* )crude_gfx_map_buffer( gpu, &map_parameters );
  if ( cb_data )
  {
    memcpy( cb_data, &ortho_projection._11, sizeof( XMFLOAT4X4 ) );
    crude_gfx_unmap_buffer( gpu, map_parameters.buffer );
  }
  
  /* Will project scissor/clipping rectangles into framebuffer space */
  clip_off = imgui_draw_data->DisplayPos;         /* (0,0) unless using multi-viewports */
  clip_scale = imgui_draw_data->FramebufferScale; /* (1,1) unless using retina display which are often (2,2) */
  
  /* Render command lists */
  draw_counts = imgui_draw_data->CmdListsCount;

  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->imgui_ds );
  
  uint32_t vtx_buffer_offset = 0, index_buffer_offset = 0;
  for ( int32 n = 0; n < draw_counts; ++n )
  {
    ImDrawList const *cmd_list = imgui_draw_data->CmdLists[ n ];
    
    for ( int32 cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i )
    {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[ cmd_i ];
      if ( pcmd->UserCallback )
      {
        /* User callback (registered via ImDrawList::AddCallback) */
        pcmd->UserCallback( cmd_list, pcmd );
      }
      else
      {
        /* Project scissor/clipping rectangles into framebuffer space */
        ImVec4 clip_rect;
        clip_rect.x = ( pcmd->ClipRect.x - clip_off.x ) * clip_scale.x;
        clip_rect.y = ( pcmd->ClipRect.y - clip_off.y ) * clip_scale.y;
        clip_rect.z = ( pcmd->ClipRect.z - clip_off.x ) * clip_scale.x;
        clip_rect.w = ( pcmd->ClipRect.w - clip_off.y ) * clip_scale.y;
        
        if ( clip_rect.x < framebuffer_width && clip_rect.y < framebuffer_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f )
        {
          /* Apply scissor/clipping rectangle */
          if ( clip_origin_lower_left )
          {
            crude_gfx_rect2d_int scissor_rect = { (int16_t)clip_rect.x, (int16_t)( framebuffer_height - clip_rect.w ), (uint16_t)( clip_rect.z - clip_rect.x ), (uint16_t)( clip_rect.w - clip_rect.y ) };
            crude_gfx_cmd_set_scissor( primary_cmd, &scissor_rect );
          }
          else
          {
            crude_gfx_rect2d_int scissor_rect = { (int16_t)clip_rect.x, (int16_t)clip_rect.y, (uint16_t)( clip_rect.z - clip_rect.x ), (uint16_t)( clip_rect.w - clip_rect.y ) };
            crude_gfx_cmd_set_scissor( primary_cmd, &scissor_rect );
          }
          
          /* Retrieve */
          crude_gfx_texture_handle new_texture = *( crude_gfx_texture_handle* )( pcmd->TexRef.GetTexID() );
          crude_gfx_cmd_draw_indexed( primary_cmd, pcmd->ElemCount, 1, index_buffer_offset + pcmd->IdxOffset, vtx_buffer_offset + pcmd->VtxOffset, new_texture.index );
        }
      }
    }
    index_buffer_offset += cmd_list->IdxBuffer.Size;
    vtx_buffer_offset += cmd_list->VtxBuffer.Size;
  }
}

void
crude_gfx_imgui_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
)
{
  crude_gfx_imgui_pass                                    *pass;
  crude_gfx_pipeline_handle                                imgui_pipeline;
  crude_gfx_descriptor_set_creation                        ds_creation;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_imgui_pass*, ctx );

  imgui_pipeline = crude_gfx_renderer_access_technique_pass_by_name(pass->scene_renderer->renderer, "imgui", "imgui" )->pipeline;

  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->imgui_ds) )
  {
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->renderer->gpu, pass->imgui_ds );
  }

  ds_creation = crude_gfx_descriptor_set_creation_empty();
  ds_creation.layout = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->renderer->gpu, imgui_pipeline, 1u );
  ds_creation.name = "rl_imgui";
  crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->ui_cb, 0u );
  pass->imgui_ds = crude_gfx_create_descriptor_set( pass->scene_renderer->renderer->gpu, &ds_creation );
}

crude_gfx_render_graph_pass_container
crude_gfx_imgui_pass_pack
(
  _In_ crude_gfx_imgui_pass                               *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_imgui_pass_render;
  container.pre_render = crude_gfx_imgui_pass_pre_render;
  container.on_techniques_reloaded = crude_gfx_imgui_pass_on_techniques_reloaded;
  return container;
}