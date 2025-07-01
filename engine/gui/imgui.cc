#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <core/hash_map.h>

#include <gui/imgui.h>

static crude_gfx_texture_handle font_texture_;

static uint32 vertex_buffer_size_ = 665536;
static uint32 index_buffer_size_ = 665536;

static char const *vertex_shader_code_bindless_ =
{
  "#version 450\n"
  "layout( location = 0 ) in vec2 Position;\n"
  "layout( location = 1 ) in vec2 UV;\n"
  "layout( location = 2 ) in uvec4 Color;\n"
  "layout( location = 0 ) out vec2 Frag_UV;\n"
  "layout( location = 1 ) out vec4 Frag_Color;\n"
  "layout (location = 2) flat out uint texture_id;\n"
  "layout( std140, set = 1, binding = 0 ) uniform LocalConstants { mat4 ProjMtx; };\n"
  "void main()\n"
  "{\n"
  " Frag_UV = UV;\n"
  " Frag_Color = Color / 255.0f;\n"
  " texture_id = gl_InstanceIndex;\n"
  " gl_Position = ProjMtx * vec4( Position.xy,0,1 );\n"
  "}\n"
};

static char const *fragment_shader_code_bindless_ =
{
  "#version 450\n"
  "#extension GL_EXT_nonuniform_qualifier : enable\n"
  "layout (location = 0) in vec2 Frag_UV;\n"
  "layout (location = 1) in vec4 Frag_Color;\n"
  "layout (location = 2) flat in uint texture_id;\n"
  "layout (location = 0) out vec4 Out_Color;\n"
  "layout (set = 0, binding = 10) uniform sampler2D textures[];\n"
  "void main()\n"
  "{\n"
  " Out_Color = Frag_Color * texture(textures[nonuniformEXT(texture_id)], Frag_UV.st);\n"
  "}\n"
};

void
crude_imgui_initialize
(
  _In_ crude_imgui                                        *imgui,
  _In_ crude_gfx_device                                   *gpu,
  _In_ void                                               *window_handle
)
{
  imgui->gpu = gpu;
  
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  
  /* Setup Platform/Renderer bindings */
  ImGui_ImplSDL3_InitForVulkan( CRUDE_CAST( SDL_Window*, window_handle ) );
  
  ImGuiIO& io = ImGui::GetIO();
  io.BackendRendererName = "Raptor_ImGui";
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
  
  /* Load font texture atlas */
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );
  
  crude_gfx_texture_creation texture_creation = crude_gfx_texture_creation_empty();
  texture_creation.format = VK_FORMAT_R8G8B8A8_UNORM;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.initial_data = pixels;
  texture_creation.width = width;
  texture_creation.height = height;
  texture_creation.depth = 1;
  texture_creation.name = "imgui_font";
  font_texture_ = crude_gfx_create_texture( gpu, &texture_creation );
  
  /* Store our identifier */
  io.Fonts->TexID = CRUDE_CAST( ImTextureID, &font_texture_ );
  
  /* Used to remove dependency from that. */
  crude_gfx_shader_state_creation shader_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_shader_state_creation );
  shader_creation.name = "imgui";
  crude_gfx_shader_state_creation_add_stage( &shader_creation, vertex_shader_code_bindless_, crude_string_length( vertex_shader_code_bindless_ ), VK_SHADER_STAGE_VERTEX_BIT );
  crude_gfx_shader_state_creation_add_stage( &shader_creation, fragment_shader_code_bindless_, crude_string_length( fragment_shader_code_bindless_ ), VK_SHADER_STAGE_FRAGMENT_BIT );
    
  crude_gfx_pipeline_creation pipeline_creation = crude_gfx_pipeline_creation_empty();
  pipeline_creation.name = "pipeline_imgui";
  pipeline_creation.shaders = shader_creation;
  
  crude_gfx_blend_state blend_state = CRUDE_COMPOUNT_EMPTY( crude_gfx_blend_state );
  blend_state.source_color = VK_BLEND_FACTOR_SRC_ALPHA;
  blend_state.destination_color = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blend_state.color_operation = VK_BLEND_OP_ADD;
  blend_state.blend_enabled = true;
  pipeline_creation.blend_state.blend_states[ pipeline_creation.blend_state.active_states++ ] = blend_state;
  
  pipeline_creation.rasterization.front = VK_FRONT_FACE_CLOCKWISE;
  pipeline_creation.rasterization.cull_mode = VK_CULL_MODE_BACK_BIT;
  pipeline_creation.render_pass_output = gpu->swapchain_output;
  
  imgui->pipeline = crude_gfx_create_pipeline( gpu, &pipeline_creation );

  /* Create constant buffer */
  crude_gfx_buffer_creation cb_creation;
  cb_creation.type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  cb_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  cb_creation.size = 64;
  cb_creation.name = "cb_imgui";
  imgui->ui_cb = crude_gfx_create_buffer( gpu, &cb_creation );
  
  /* Create vertex and index buffers */
  crude_gfx_buffer_creation vertex_buffer_creation = crude_gfx_buffer_creation_empty();
  vertex_buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  vertex_buffer_creation.type_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  vertex_buffer_creation.size = vertex_buffer_size_;
  vertex_buffer_creation.name = "vertex_buffer_imgui";
  imgui->vertex_buffer = crude_gfx_create_buffer( gpu, &vertex_buffer_creation );
  
  crude_gfx_buffer_creation index_buffer_creation = crude_gfx_buffer_creation_empty();
  index_buffer_creation.type_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  index_buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  index_buffer_creation.size = index_buffer_size_;
  index_buffer_creation.name = "index_buffer_imgui";
  imgui->index_buffer = crude_gfx_create_buffer( gpu, &index_buffer_creation );
}

void
crude_imgui_deinitialize
(
  _In_ crude_imgui                                        *imgui
)
{
  crude_gfx_destroy_buffer( imgui->gpu, imgui->vertex_buffer );
  crude_gfx_destroy_buffer( imgui->gpu, imgui->index_buffer );
  crude_gfx_destroy_buffer( imgui->gpu, imgui->ui_cb );
  
  crude_gfx_destroy_pipeline( imgui->gpu, imgui->pipeline );
  crude_gfx_destroy_texture( imgui->gpu, imgui->font_texture );
  
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

void
crude_imgui_new_frame
(
  _In_ crude_imgui                                        *imgui
)
{
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}

void
crude_imgui_render
(
  _In_ crude_imgui                                        *imgui,
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_render_pass_handle                        render_pass,
  _In_ crude_gfx_framebuffer_handle                        framebuffer,
  _In_ bool                                                use_secondary
)
{
  ImGui::Render();
  
  ImDrawData *draw_data = ImGui::GetDrawData();
  
  /* Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates) */
  int framebuffer_width = (int)( draw_data->DisplaySize.x * draw_data->FramebufferScale.x );
  int framebuffer_height = (int)( draw_data->DisplaySize.y * draw_data->FramebufferScale.y );
  if ( framebuffer_width <= 0 || framebuffer_height <= 0 )
  {
    return;
  }
  
  /* Vulkan backend has a different origin than OpenGL. */
  bool clip_origin_lower_left = false;
  size_t vertex_size = draw_data->TotalVtxCount * sizeof( ImDrawVert );
  size_t index_size = draw_data->TotalIdxCount * sizeof( ImDrawIdx );
  
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
  ImDrawVert* vtx_dst = NULL;
  ImDrawIdx* idx_dst = NULL;
  
  crude_gfx_map_buffer_parameters map_parameters_vb = CRUDE_COMPOUNT( crude_gfx_map_buffer_parameters, { imgui->vertex_buffer, 0, ( uint32 )vertex_size } );
  vtx_dst = CRUDE_REINTERPRET_CAST( ImDrawVert*, crude_gfx_map_buffer( imgui->gpu, &map_parameters_vb ) );
  
  if ( vtx_dst )
  {
    for ( int i = 0; i < draw_data->CmdListsCount; i++ )
    {
      ImDrawList const* cmd_list = draw_data->CmdLists[ i ];
      memcpy( vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof( ImDrawVert ) );
      vtx_dst += cmd_list->VtxBuffer.Size;
    }
    
    crude_gfx_unmap_buffer( imgui->gpu, map_parameters_vb.buffer );
  }
  
  crude_gfx_map_buffer_parameters map_parameters_ib = CRUDE_COMPOUNT( crude_gfx_map_buffer_parameters, { imgui->index_buffer, 0, ( uint32 )index_size } );
  idx_dst = CRUDE_REINTERPRET_CAST( ImDrawIdx*, crude_gfx_map_buffer( imgui->gpu, &map_parameters_ib ) );
  
  if ( idx_dst )
  {
    for ( int i = 0; i < draw_data->CmdListsCount; i++ )
    {
      ImDrawList const* cmd_list = draw_data->CmdLists[ i ];
      memcpy( idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof( ImDrawIdx ) );
      idx_dst += cmd_list->IdxBuffer.Size;
    }
    
    crude_gfx_unmap_buffer( imgui->gpu, map_parameters_ib.buffer );
  }
  
  // TODO: Add the sorting.
  crude_gfx_cmd_bind_render_pass( cmd, render_pass, framebuffer, use_secondary );
  crude_gfx_cmd_bind_pipeline( cmd, imgui->pipeline );
  crude_gfx_cmd_bind_vertex_buffer( cmd, imgui->vertex_buffer, 0u, 0u );
  crude_gfx_cmd_bind_index_buffer( cmd, imgui->index_buffer, 0u );
  
  crude_gfx_viewport const viewport = { 0, 0, ( uint16 )framebuffer_width, ( uint16 )framebuffer_height, 0.0f, 1.0f };
  crude_gfx_cmd_bind_index_buffer( cmd, imgui->index_buffer, 0u );
  crude_gfx_cmd_set_viewport( cmd, &viewport ); 
  
  /* Setup viewport, orthographic projection matrix */
  /* Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is typically (0,0) for single viewport apps. */
  float L = draw_data->DisplayPos.x;
  float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
  float T = draw_data->DisplayPos.y;
  float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
  float const ortho_projection[ 4 ][ 4 ] =
  {
    { 2.0f / ( R - L ), 0.0f, 0.0f, 0.0f },
    { 0.0f, 2.0f / ( T - B ), 0.0f, 0.0f },
    { 0.0f, 0.0f, -1.0f, 0.0f },
    { ( R + L ) / ( L - R ), ( T + B ) / ( B - T ), 0.0f, 1.0f },
  };
  
  crude_gfx_map_buffer_parameters cb_map = { imgui->ui_cb, 0, 0 };
  float *cb_data = ( float* )crude_gfx_map_buffer( imgui->gpu, &cb_map );
  if ( cb_data )
  {
    memcpy( cb_data, &ortho_projection[0][0], 64 );
    crude_gfx_unmap_buffer( imgui->gpu, cb_map.buffer );
  }
  
  /* Will project scissor/clipping rectangles into framebuffer space */
  ImVec2 clip_off = draw_data->DisplayPos;         /* (0,0) unless using multi-viewports */
  ImVec2 clip_scale = draw_data->FramebufferScale; /* (1,1) unless using retina display which are often (2,2) */
  
  /* Render command lists */
  int counts = draw_data->CmdListsCount;
  crude_gfx_texture_handle last_texture = imgui->font_texture;
  crude_gfx_descriptor_set_handle last_descriptor_set = CRUDE_HASHMAP_GET( imgui->texture_to_descriptor_set, last_texture.index )->value;
  crude_gfx_cmd_bind_descriptor_set( cmd, last_descriptor_set );
  
  uint32_t vtx_buffer_offset = 0, index_buffer_offset = 0;
  for ( int n = 0; n < counts; n++ )
  {
    ImDrawList const *cmd_list = draw_data->CmdLists[n];
    
    for ( int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i )
    {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if ( pcmd->UserCallback )
      {
        /* User callback (registered via ImDrawList::AddCallback) */
        pcmd->UserCallback( cmd_list, pcmd );
      }
      else
      {
        // Project scissor/clipping rectangles into framebuffer space
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
            crude_gfx_cmd_set_scissor( cmd, &scissor_rect );
          }
          else
          {
            crude_gfx_rect2d_int scissor_rect = { (int16_t)clip_rect.x, (int16_t)clip_rect.y, (uint16_t)( clip_rect.z - clip_rect.x ), (uint16_t)( clip_rect.w - clip_rect.y ) };
            crude_gfx_cmd_set_scissor( cmd, &scissor_rect );
          }
          
          /* Retrieve */
          crude_gfx_texture_handle new_texture = *( crude_gfx_texture_handle* )( pcmd->TextureId );
          crude_gfx_cmd_draw_indexed( cmd, pcmd->ElemCount, 1, index_buffer_offset + pcmd->IdxOffset, vtx_buffer_offset + pcmd->VtxOffset, new_texture.index );
        }
      }
    }
    index_buffer_offset += cmd_list->IdxBuffer.Size;
    vtx_buffer_offset += cmd_list->VtxBuffer.Size;
  }
}