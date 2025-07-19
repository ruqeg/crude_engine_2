#include <core/profiler.h>
#include <core/hash_map.h>
#include <scene/scene_components.h>
#include <graphics/scene_renderer.h>

#include <graphics/passes/meshlet_pass.h>

void
crude_gfx_meshlet_pass_initialize
(
  _In_ crude_gfx_meshlet_pass                             *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  bool                                                     early_pass
)
{ 
  pass->scene_renderer = scene_renderer;
  pass->early_pass = early_pass;
}

void
crude_gfx_meshlet_pass_deinitialize
(
  _In_ crude_gfx_meshlet_pass                             *pass
)
{
}

void
crude_gfx_meshlet_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_meshlet_pass                                  *pass;
  crude_gfx_renderer                                      *renderer;
  crude_gfx_mesh_material_gpu                             *mesh_materials;
  crude_gfx_mesh_draw_counts_gpu                          *mesh_draw_counts;
  crude_gfx_mesh_draw_command_gpu                         *mesh_draw_commands;
  crude_gfx_map_buffer_parameters                          buffer_map;
  crude_gfx_pipeline_handle                                pipeline;
 
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_meshlet_pass*, ctx );

  renderer = pass->scene_renderer->renderer;

  buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  buffer_map.buffer = pass->scene_renderer->mesh_task_indirect_commands_sb[ renderer->gpu->current_frame ];
  buffer_map.offset = 0;
  buffer_map.size = sizeof( crude_gfx_mesh_draw_command_gpu ) * CRUDE_ARRAY_LENGTH( pass->scene_renderer->mesh_instances );
  mesh_draw_commands = CRUDE_CAST( crude_gfx_mesh_draw_command_gpu*, crude_gfx_map_buffer( renderer->gpu, &buffer_map ) );
  
  buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  buffer_map.buffer = pass->scene_renderer->mesh_task_indirect_count_sb[ renderer->gpu->current_frame ];
  buffer_map.offset = 0;
  buffer_map.size = sizeof( crude_gfx_mesh_draw_counts_gpu );
  mesh_draw_counts = CRUDE_CAST( crude_gfx_mesh_draw_counts_gpu*, crude_gfx_map_buffer( renderer->gpu, &buffer_map ) );
  
  buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  buffer_map.buffer = pass->scene_renderer->meshes_materials_sb;
  buffer_map.offset = 0;
  buffer_map.size = 0;
  mesh_materials = CRUDE_CAST( crude_gfx_mesh_material_gpu*, crude_gfx_map_buffer( renderer->gpu, &buffer_map ) );
  
  if ( mesh_materials && mesh_draw_counts && mesh_draw_commands )
  {
    uint32 visible_meshlet_index = 0u;
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( pass->scene_renderer->mesh_instances ); ++i )
    {
      crude_gfx_device *gpu = pass->scene_renderer->renderer->gpu;
      crude_gfx_mesh_cpu *mesh_cpu = pass->scene_renderer->mesh_instances[ i ].mesh;
    
      bool mesh_textures_ready = ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->albedo_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->albedo_texture_handle ) )
        && ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->normal_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->normal_texture_handle ) )
        && ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->occlusion_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->occlusion_texture_handle ) )
        && ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->roughness_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->roughness_texture_handle ) );

      if ( !mesh_textures_ready )
      {
        continue;
      }
      
      mesh_draw_commands[ visible_meshlet_index ].indirect_meshlet.groupCountX = crude_ceil( pass->scene_renderer->mesh_instances[ i ].mesh->meshlets_count / 32.0 );
      mesh_draw_commands[ visible_meshlet_index ].indirect_meshlet.groupCountY = 1;
      mesh_draw_commands[ visible_meshlet_index ].indirect_meshlet.groupCountZ = 1;

      crude_gfx_mesh_cpu_to_mesh_material_gpu( pass->scene_renderer->mesh_instances[ i ].mesh, &mesh_materials[ i ] );

      ++visible_meshlet_index;
    }

    mesh_draw_counts->opaque_mesh_visible_count = visible_meshlet_index;
    mesh_draw_counts->depth_pyramid_texture_index = pass->early_pass ? pass->scene_renderer->depth_pyramid_late_pass.depth_pyramid_texture_handle.index : pass->scene_renderer->depth_pyramid_early_pass.depth_pyramid_texture_handle.index;
    mesh_draw_counts->occlusion_culling_late_flag = pass->scene_renderer->occlusion_culling_late_flag = !pass->early_pass;
  }

  if ( mesh_draw_commands )
  {
    crude_gfx_unmap_buffer( renderer->gpu, pass->scene_renderer->mesh_task_indirect_commands_sb[ renderer->gpu->current_frame ] );
  }
  if ( mesh_draw_counts )
  {
    crude_gfx_unmap_buffer( renderer->gpu, pass->scene_renderer->mesh_task_indirect_count_sb[ renderer->gpu->current_frame ] );
  }
  if ( mesh_materials )
  {
    crude_gfx_unmap_buffer( renderer->gpu, pass->scene_renderer->meshes_materials_sb );
  }
  
  pipeline = crude_gfx_renderer_access_technique_pass_by_name( renderer, "meshlet", "meshlet" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->scene_renderer->mesh_shader_ds[ renderer->gpu->current_frame ] );
  crude_gfx_cmd_draw_mesh_task_indirect_count(
    primary_cmd,
    pass->scene_renderer->mesh_task_indirect_commands_sb[ renderer->gpu->current_frame ],
    CRUDE_OFFSETOF( crude_gfx_mesh_draw_command_gpu, indirect_meshlet ),
    pass->scene_renderer->mesh_task_indirect_count_sb[ renderer->gpu->current_frame ],
    0,
    CRUDE_ARRAY_LENGTH( pass->scene_renderer->mesh_instances ),
    sizeof( crude_gfx_mesh_draw_command_gpu )
  );
}

static void
crude_gfx_meshlet_pass_on_resize
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
}

crude_gfx_render_graph_pass_container
crude_gfx_meshlet_pass_pack
(
  _In_ crude_gfx_meshlet_pass                             *pass
)
{
  crude_gfx_render_graph_pass_container container;
  container.ctx = pass;
  container.on_resize = crude_gfx_meshlet_pass_on_resize;
  container.render = crude_gfx_meshlet_pass_render;
  return container;
}