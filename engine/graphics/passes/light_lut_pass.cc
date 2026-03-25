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
  crude_gfx_device                                        *gpu;
  crude_gfx_light_lut_pass                                *pass;

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_light_lut_pass*, ctx );

  gpu = pass->scene_renderer->gpu;

  if ( CRUDE_ARRAY_LENGTH( pass->scene_renderer->culled_lights ) == 0 )
  {
    return;
  }
  
  /* Upload light to gpu */
  {
    crude_gfx_cmd_add_buffer_barrier( primary_cmd, pass->scene_renderer->lights_hga.buffer_handle, CRUDE_GFX_RESOURCE_STATE_SHADER_RESOURCE, CRUDE_GFX_RESOURCE_STATE_COPY_DEST );
  
    crude_gfx_light                                       *lights_gpu;
    crude_gfx_memory_allocation                            lights_tca;

    lights_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, sizeof( crude_gfx_light ) * CRUDE_ARRAY_LENGTH( pass->scene_renderer->culled_lights ) );
    lights_gpu = CRUDE_CAST( crude_gfx_light*, lights_tca.cpu_address );

    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( pass->scene_renderer->culled_lights ); ++i )
    {
      crude_gfx_culled_light_cpu const                    *culled_light_cpu;
      crude_gfx_light_cpu const                           *light_cpu;
  
      culled_light_cpu = &pass->scene_renderer->culled_lights[ i ];
      light_cpu = &pass->scene_renderer->lights[ culled_light_cpu->light_index ];
      
      lights_gpu[ i ] = CRUDE_COMPOUNT_EMPTY( crude_gfx_light );
      lights_gpu[ i ].color = light_cpu->light.color;
      lights_gpu[ i ].intensity = light_cpu->light.intensity;
      lights_gpu[ i ].world_position = light_cpu->translation;
      lights_gpu[ i ].radius = light_cpu->light.radius;
    }

    crude_gfx_cmd_memory_copy( primary_cmd, lights_tca, pass->scene_renderer->lights_hga, 0, 0 );
  }
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