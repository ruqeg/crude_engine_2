#include <engine/core/hash_map.h>
#include <engine/graphics/scene_renderer.h>
#include <engine/graphics/scene_renderer_resources.h>

#include <engine/graphics/passes/light_lut_pass.h>

typedef struct crude_gfx_sorted_light
{
  uint32                                                   light_index;
  float32                                                  projected_z;
  float32                                                  projected_z_min;
  float32                                                  projected_z_max;
} crude_gfx_sorted_light;

static int
crude_gfx_light_lut_pass_sorting_light_fun_
(
  _In_ const void                                         *a,
  _In_ const void                                         *b
);

void
crude_gfx_light_lut_pass_initialize
(
  _In_ crude_gfx_light_lut_pass                           *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->scene_renderer = scene_renderer;
}

void
crude_gfx_light_lut_pass_deinitialize
(
  _In_ crude_gfx_light_lut_pass                           *pass
)
{
}

void
crude_gfx_light_lut_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  CRUDE_ALIGNED_STRUCT( 16 ) push_constant_
  {
    VkDeviceAddress                                        scene;
    VkDeviceAddress                                        mesh_draws;
    VkDeviceAddress                                        mesh_instance_draws;
    VkDeviceAddress                                        mesh_bounds;
    VkDeviceAddress                                        mesh_draw_commands;
    VkDeviceAddress                                        mesh_draw_commands_culled;
    VkDeviceAddress                                        mesh_draw_count;
  };

  crude_gfx_device                                        *gpu;
  crude_gfx_light_lut_pass                                *pass;
  crude_gfx_light_gpu                                     *lights_gpu;
  crude_gfx_sorted_light                                  *sorted_lights;
  uint32                                                  *lights_luts;
  uint32                                                  *bin_range_per_light;
  crude_camera const                                      *camera;
  crude_transform const                                   *camera_transform;
  XMMATRIX                                                 view_to_world, world_to_view, view_to_clip, clip_to_view, world_to_clip;
  float32                                                  bin_size;
  push_constant_                                           push_constant;
  uint32                                                   temporary_allocator_marker;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_light_lut_pass*, ctx );

  gpu = pass->scene_renderer->gpu;

  if ( !pass->scene_renderer->total_visible_lights_count )
  {
    return;
  }

  temporary_allocator_marker = crude_stack_allocator_get_marker( pass->scene_renderer->temporary_allocator );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( sorted_lights, pass->scene_renderer->total_visible_lights_count, crude_stack_allocator_pack( pass->scene_renderer->temporary_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( bin_range_per_light, pass->scene_renderer->total_visible_lights_count, crude_stack_allocator_pack( pass->scene_renderer->temporary_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( lights_luts, CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_Z_BINS, crude_stack_allocator_pack( pass->scene_renderer->temporary_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( lights_gpu, pass->scene_renderer->total_visible_lights_count, crude_stack_allocator_pack( pass->scene_renderer->temporary_allocator ) );
  
  camera = &pass->scene_renderer->options.scene.camera;
  view_to_world = XMLoadFloat4x4( &pass->scene_renderer->options.scene.camera_view_to_world );

  world_to_view = XMMatrixInverse( NULL, view_to_world );
  view_to_clip = crude_camera_view_to_clip( camera );
  clip_to_view = XMMatrixInverse( NULL, view_to_clip );
  world_to_clip = XMMatrixMultiply( world_to_view, view_to_clip );
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( sorted_lights ); ++i )
  {
    crude_gfx_light_cpu const                           *light_cpu;
  
    light_cpu = &pass->scene_renderer->lights[ i ];
    
    lights_gpu[ i ] = CRUDE_COMPOUNT_EMPTY( crude_gfx_light_gpu );
    lights_gpu[ i ].color = light_cpu->light.color;
    lights_gpu[ i ].intensity = light_cpu->light.intensity;
    lights_gpu[ i ].position = light_cpu->translation;
    lights_gpu[ i ].radius = light_cpu->light.radius;
  }

  /* Sort lights based on Z */
  for ( uint32 i = 0; i < pass->scene_renderer->total_visible_lights_count; ++i )
  {
    crude_gfx_sorted_light                              *sorted_light;
    crude_gfx_light_cpu const                           *light_cpu;
    XMVECTOR                                             world_pos, view_pos, view_pos_min, view_pos_max;
  
    light_cpu = &pass->scene_renderer->lights[ i ];
  
    world_pos = XMVectorSet( light_cpu->translation.x, light_cpu->translation.y, light_cpu->translation.z, 1.0f );
  
    view_pos = XMVector4Transform( world_pos, world_to_view );
    view_pos_min = XMVectorAdd( view_pos, XMVectorSet( 0, 0, -light_cpu->light.radius, 0 ) );
    view_pos_max = XMVectorAdd( view_pos, XMVectorSet( 0, 0, light_cpu->light.radius, 0 ) );
  
    sorted_light = &sorted_lights[ i ];
    sorted_light->light_index = i;
    sorted_light->projected_z = ( ( XMVectorGetZ( view_pos ) - camera->near_z ) / ( camera->far_z - camera->near_z ) );
    sorted_light->projected_z_min = ( ( XMVectorGetZ( view_pos_min ) - camera->near_z ) / ( camera->far_z - camera->near_z ) );
    sorted_light->projected_z_max = ( ( XMVectorGetZ( view_pos_max ) - camera->near_z ) / ( camera->far_z - camera->near_z ) );
  }
  
  qsort( sorted_lights, pass->scene_renderer->total_visible_lights_count, sizeof( crude_gfx_sorted_light ), crude_gfx_light_lut_pass_sorting_light_fun_ );
  
  /* Upload light to gpu */
  {
    crude_gfx_memory_allocation                          lights_tca;
    uint32                                               lights_gpu_count;

    lights_gpu_count = CRUDE_ARRAY_LENGTH( lights_gpu );
    lights_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, sizeof( crude_gfx_light_gpu ) * lights_gpu_count );
    crude_memory_copy( lights_tca.cpu_address, lights_gpu, sizeof( crude_gfx_light_gpu ) * lights_gpu_count );
    crude_gfx_cmd_memory_copy( primary_cmd, lights_tca, pass->scene_renderer->lights_hga, 0, 0 );
  }
    
  /* Calculate lights clusters */
  bin_size = 1.f / CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_Z_BINS;
    
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( sorted_lights ); ++i )
  {
    crude_gfx_sorted_light const                        *sorted_light;
    uint32                                               min_bin, max_bin;
  
    sorted_light = &sorted_lights[ i ];
  
    if ( sorted_light->projected_z_min < 0.f && sorted_light->projected_z_max < 0.f )
    {
      bin_range_per_light[ i ] = UINT32_MAX;
      continue;
    }
    min_bin = CRUDE_MAX( 0u, CRUDE_FLOOR( sorted_light->projected_z_min * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_Z_BINS ) );
    max_bin = CRUDE_MAX( 0u, CRUDE_CEIL( sorted_light->projected_z_max * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_Z_BINS ) );
    bin_range_per_light[ i ] = ( min_bin & 0xffff ) | ( ( max_bin & 0xffff ) << 16 );
  }
    
  for ( uint32 bin = 0; bin < CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_Z_BINS; ++bin )
  {
    float32                                              bin_min, bin_max;
    uint32                                               min_light_id, max_light_id;
  
    min_light_id = CRUDE_GRAPHICS_SCENE_RENDERER_LIGHTS_MAX_COUNT + 1;
    max_light_id = 0;
  
    bin_min = bin_size * bin;
    bin_max = bin_min + bin_size;
  
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( sorted_lights ); ++i )
    {
      crude_gfx_sorted_light const                      *light;
      uint32                                             light_bins, min_bin, max_bin;
  
      light = &sorted_lights[ i ];
      light_bins = bin_range_per_light[ i ];
  
      if ( light_bins == UINT32_MAX )
      {
        continue;
      }
  
      min_bin = light_bins & 0xffff;
      max_bin = light_bins >> 16;
  
      if ( bin >= min_bin && bin <= max_bin )
      {
        if ( i < min_light_id )
        {
          min_light_id = i;
        }
        if ( i > max_light_id )
        {
          max_light_id = i;
        }
      }
    }
  
    lights_luts[ bin ] = min_light_id | ( max_light_id << 16 );
  }
  
  /* Upload light indices */
  {
    uint32                                              *lights_indices_mapped;
    crude_gfx_memory_allocation                          lights_indices_tca;

    lights_indices_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, sizeof( uint32 ) * CRUDE_ARRAY_LENGTH( sorted_lights ) );
    lights_indices_mapped = CRUDE_CAST( uint32*, lights_indices_tca.cpu_address );
    
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( sorted_lights ); ++i )
    {
      lights_indices_mapped[ i ] = sorted_lights[ i ].light_index;
    }

    crude_gfx_cmd_memory_copy( primary_cmd, lights_indices_tca, pass->scene_renderer->lights_indices_hga, 0, 0 );
  }
    
  /* Upload lights LUT */
  {
    crude_gfx_memory_allocation                          lights_luts_tca;
    
    lights_luts_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, sizeof( uint32 ) * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_Z_BINS );
    crude_memory_copy( lights_luts_tca.cpu_address, lights_luts, CRUDE_ARRAY_LENGTH( lights_luts ) * sizeof( uint32 ) );
    crude_gfx_cmd_memory_copy( primary_cmd, lights_luts_tca, pass->scene_renderer->lights_bins_hga, 0, 0 );
  }
    
  {
    uint32                                              *light_tiles_bits;
    float32                                              tile_size_inv;
    uint32                                               tile_x_count, tile_y_count, tiles_entry_count, buffer_size, tile_stride;
  
    tile_x_count = pass->scene_renderer->gpu->renderer_size.x / CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_TILE_SIZE;
    tile_y_count = pass->scene_renderer->gpu->renderer_size.y / CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_TILE_SIZE;
    tiles_entry_count = tile_x_count * tile_y_count * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_WORDS_COUNT;
    buffer_size = tiles_entry_count * sizeof( uint32 );
  
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( light_tiles_bits, tiles_entry_count, crude_stack_allocator_pack( pass->scene_renderer->temporary_allocator ) );
    memset( light_tiles_bits, 0, buffer_size );
  
    tile_size_inv = 1.0f / CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_TILE_SIZE;
    tile_stride = tile_x_count * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_WORDS_COUNT;
  
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( lights_gpu ); ++i )
    {
      crude_gfx_light_gpu                               *light_gpu;
      XMVECTOR                                           light_world_position, light_view_position;
      XMVECTOR                                           aabb, minx, maxx, miny, maxy;
      XMVECTOR                                           left, right, top, bottom;
      float32                                            aabb_screen_width, aabb_screen_height;
      float32                                            aabb_screen_min_x, aabb_screen_min_y, aabb_screen_max_x, aabb_screen_max_y;
      float32                                            light_radius;
      uint32                                             light_index;
      bool                                               camera_visible, ty_camera_inside, tx_camera_inside;
  
      light_index = sorted_lights[ i ].light_index;
      light_gpu = &lights_gpu[ light_index ];
  
      /* Transform light in camera space */
      light_world_position = XMVectorSet( light_gpu->position.x, light_gpu->position.y, light_gpu->position.z, 1.0f );
      light_radius = light_gpu->radius;
  
      light_view_position = XMVector4Transform( light_world_position, world_to_view );
      camera_visible = -XMVectorGetZ( light_view_position ) - light_radius < camera->near_z;
  
      if ( !camera_visible )
      {
        continue;
      }
  
      /* Compute projected sphere AABB */
      {
        XMVECTOR                                         aabb_min, aabb_max;
  
        aabb_min = XMVectorSet( FLT_MAX, FLT_MAX, FLT_MAX, 0 );
        aabb_max = XMVectorSet( -FLT_MAX, -FLT_MAX, -FLT_MAX, 0 );
  
        for ( uint32 c = 0; c < 8; ++c )
        {
          XMVECTOR                                       corner, corner_vs, corner_ndc;
  
          corner = XMVectorSet( ( c % 2 ) ? 1.f : -1.f, ( c & 2 ) ? 1.f : -1.f, ( c & 4 ) ? 1.f : -1.f, 1 );
          corner = XMVectorScale( corner, light_radius );
          corner = XMVectorAdd( corner, light_world_position );
          corner = XMVectorSetW( corner, 1.f );
  
          corner_vs = XMVector4Transform( corner, world_to_view );
          corner_vs = XMVectorSetZ( corner_vs, CRUDE_MAX( camera->near_z, XMVectorGetZ( corner_vs ) ) );
          corner_ndc = XMVector4Transform( corner_vs, view_to_clip );
          corner_ndc = XMVectorScale( corner_ndc, 1.f / XMVectorGetW( corner_ndc ) );
  
          aabb_min = XMVectorMin( aabb_min, corner_ndc );
          aabb_max = XMVectorMax( aabb_max, corner_ndc );
        }
  
        aabb = XMVectorSet( XMVectorGetX( aabb_min ), -1.f * XMVectorGetY( aabb_max ), XMVectorGetX( aabb_max ), -1.f * XMVectorGetY( aabb_min ) );
      }
  
      {
        float32                                         light_view_position_length;
        bool                                            camera_inside;
  
        light_view_position_length = XMVectorGetX( XMVector3Length( light_view_position ) );
        camera_inside = ( light_view_position_length - light_radius ) < camera->near_z;
  
        if ( camera_inside )
        {
          aabb = { -1,-1, 1, 1 };
        }
      }
  
      {
        XMVECTOR                                         aabb_screen;
  
        aabb_screen = XMVectorSet(
          ( XMVectorGetX( aabb ) * 0.5f + 0.5f ) * ( gpu->renderer_size.x - 1 ),
          ( XMVectorGetY( aabb ) * 0.5f + 0.5f ) * ( gpu->renderer_size.y - 1 ),
          ( XMVectorGetZ( aabb ) * 0.5f + 0.5f ) * ( gpu->renderer_size.x - 1 ),
          ( XMVectorGetW( aabb ) * 0.5f + 0.5f ) * ( gpu->renderer_size.y - 1 )
        );
  
        aabb_screen_width = XMVectorGetZ( aabb_screen ) - XMVectorGetX( aabb_screen );
        aabb_screen_height = XMVectorGetW( aabb_screen ) - XMVectorGetY( aabb_screen );
  
        if ( aabb_screen_width < 0.0001f || aabb_screen_height < 0.0001f )
        {
          continue;
        }
  
        aabb_screen_min_x = XMVectorGetX( aabb_screen );
        aabb_screen_min_y = XMVectorGetY( aabb_screen );
        
        aabb_screen_max_x = aabb_screen_min_x + aabb_screen_width;
        aabb_screen_max_y = aabb_screen_min_y + aabb_screen_height;
      }
  
      if ( aabb_screen_min_x > gpu->renderer_size.x || aabb_screen_min_y > gpu->renderer_size.y )
      {
        continue;
      }
  
      if ( aabb_screen_max_x < 0.0f || aabb_screen_max_y < 0.0f )
      {
        continue;
      }
  
      aabb_screen_min_x = CRUDE_MAX( aabb_screen_min_x, 0.0f );
      aabb_screen_min_y = CRUDE_MAX( aabb_screen_min_y, 0.0f );
  
      aabb_screen_max_x = CRUDE_MIN( aabb_screen_max_x, gpu->renderer_size.x );
      aabb_screen_max_y = CRUDE_MIN( aabb_screen_max_y, gpu->renderer_size.y );
  
      {
        uint32                                           first_tile_x, last_tile_x, first_tile_y, last_tile_y;
  
        first_tile_x = aabb_screen_min_x * tile_size_inv;
        last_tile_x = CRUDE_MIN( tile_x_count - 1, aabb_screen_max_x * tile_size_inv );
  
        first_tile_y = aabb_screen_min_y * tile_size_inv;
        last_tile_y = CRUDE_MIN( tile_y_count - 1, aabb_screen_max_y * tile_size_inv );
  
        for ( uint32 y = first_tile_y; y <= last_tile_y; ++y )
        {
          for ( uint32 x = first_tile_x; x <= last_tile_x; ++x )
          {
            uint32                                       array_index, word_index, bit_index;
  
            array_index = y * tile_stride + x * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_WORDS_COUNT;
  
            word_index = i / 32;
            bit_index = i % 32;
  
            light_tiles_bits[ array_index + word_index ] |= ( 1 << bit_index );
          }
        }
      }
    }
    
    {
      crude_gfx_memory_allocation                        light_tiles_tca;
      
      light_tiles_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, CRUDE_ARRAY_LENGTH( light_tiles_bits ) * sizeof( uint32 ) );
      crude_memory_copy( light_tiles_tca.cpu_address, light_tiles_bits, CRUDE_ARRAY_LENGTH( light_tiles_bits ) * sizeof( uint32 ) );
      crude_gfx_cmd_memory_copy( primary_cmd, light_tiles_tca, pass->scene_renderer->lights_tiles_hga, 0, 0 );
    }
  }
  
  crude_stack_allocator_free_marker( pass->scene_renderer->temporary_allocator, temporary_allocator_marker );
}

void
crude_gfx_light_lut_pass_register
(
  _In_ crude_gfx_light_lut_pass                           *pass
)
{
}

crude_gfx_render_graph_pass_container
crude_gfx_light_lut_pass_pack
(
  _In_ crude_gfx_light_lut_pass                           *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_light_lut_pass_render;
  return container;
}

int
crude_gfx_light_lut_pass_sorting_light_fun_
(
  _In_ const void                                         *a,
  _In_ const void                                         *b
)
{
  crude_gfx_sorted_light const * la = CRUDE_CAST( crude_gfx_sorted_light const*, a );
  crude_gfx_sorted_light const * lb = CRUDE_CAST( crude_gfx_sorted_light const*, b );

  if ( la->projected_z < lb->projected_z )
  {
    return -1;
  }
  else if ( la->projected_z > lb->projected_z )
  {
    return 1;
  }
  return 0;
}