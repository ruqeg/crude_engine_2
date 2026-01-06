#include <TaskScheduler_c.h>
#include <cgltf.h>
#include <stb_image.h>
#include <meshoptimizer.h>

#include <engine/core/profiler.h>
#include <engine/core/array.h>
#include <engine/core/file.h>
#include <engine/core/hash_map.h>

#include <engine/scene/scene_ecs.h>
#include <engine/physics/physics_ecs.h>
#include <engine/scene/scene_debug_ecs.h>

#include <engine/graphics/gpu_profiler.h>
#include <engine/graphics/scene_renderer.h>

/**
 * Scene Renderer Other
 */
static void
update_dynamic_buffers_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

/**
 * Scene Renderer Utils
 */
static void
crude_scene_renderer_register_nodes_instances_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _Out_opt_ bool                                          *model_initialized
);

/**
 *
 * Renderer Scene Function
 * 
 */
void
crude_gfx_scene_renderer_initialize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_scene_renderer_creation                  *creation
)
{
  crude_gfx_buffer_creation                                buffer_creation;
 
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Initialize scene renderer." );

  /* Context */
  scene_renderer->allocator = creation->allocator;
  scene_renderer->temporary_allocator = creation->temporary_allocator;
  scene_renderer->gpu = creation->async_loader->gpu;
  scene_renderer->async_loader = creation->async_loader;
  scene_renderer->imgui_context = creation->imgui_context;
  scene_renderer->model_renderer_resources_manager = creation->model_renderer_resources_manager;
  scene_renderer->imgui_pass_enalbed = creation->imgui_pass_enalbed;
  scene_renderer->total_visible_meshes_instances_count = 0u;

  scene_renderer->options = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene_renderer_options );

  scene_renderer->options.compose_pass.gbuffer_albedo = "gbuffer_albedo";
  scene_renderer->options.compose_pass.gbuffer_normal = "gbuffer_normal";
  scene_renderer->options.compose_pass.gbuffer_roughness_metalness = "gbuffer_roughness_metalness";
  scene_renderer->options.compose_pass.gbuffer_depth = "depth";

  scene_renderer->options.depth_pyramid_pass.depth = "depth";

  scene_renderer->options.postprocessing_pass.hdr_pre_tonemapping = "pbr_with_ssr";
  scene_renderer->options.postprocessing_pass.gamma = 2.2;
  
  scene_renderer->options.ssr_pass.max_steps = 100;
  scene_renderer->options.ssr_pass.max_distance = 100;
  
  scene_renderer->options.ssr_pass.stride_zcutoff = 1.0 / 200.0;
  scene_renderer->options.ssr_pass.stride = 20;
  scene_renderer->options.ssr_pass.z_thickness = 0;
  scene_renderer->options.ssr_pass.depth_texture = "depth";
  scene_renderer->options.ssr_pass.normal_texture = "gbuffer_normal";
  scene_renderer->options.ssr_pass.pbr_without_ssr_texture = "pbr";
  scene_renderer->options.ssr_pass.pbr_with_ssr_texture = "pbr_with_ssr";

  scene_renderer->options.scene.background_color = CRUDE_COMPOUNT( XMFLOAT3, { 0.529, 0.807, 0.921 } );
  scene_renderer->options.scene.background_intensity = 1.f;
  scene_renderer->options.scene.ambient_color = CRUDE_COMPOUNT( XMFLOAT3, { 0, 0, 0 } );
  scene_renderer->options.scene.ambient_intensity = 1.f;

  scene_renderer->options.debug.debug_mode = CRUDE_SHADER_DEBUG_MODE_NONE;

  scene_renderer->total_meshes_instances_buffer_capacity = CRUDE_GRAPHICS_SCENE_RENDERER_MESH_INSTANCES_BUFFER_CAPACITY;
  
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->lights, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->model_renderer_resoruces_instances, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );
  
  scene_renderer->lights_hga = crude_gfx_memory_allocation_empty( );
  scene_renderer->lights_world_to_clip_hga = crude_gfx_memory_allocation_empty( );
  scene_renderer->lights_indices_hga = crude_gfx_memory_allocation_empty( );
  scene_renderer->lights_bins_hga = crude_gfx_memory_allocation_empty( );
  scene_renderer->lights_tiles_hga = crude_gfx_memory_allocation_empty( );

  scene_renderer->meshes_instances_draws_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_mesh_instance_draw_gpu ) * scene_renderer->total_meshes_instances_buffer_capacity, CRUDE_GFX_MEMORY_TYPE_GPU, "meshes_instances_draws" );
  scene_renderer->scene_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_scene_constant_gpu ), CRUDE_GFX_MEMORY_TYPE_GPU, "scene" );
  scene_renderer->mesh_task_indirect_commands_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, scene_renderer->total_meshes_instances_buffer_capacity * sizeof( crude_gfx_mesh_draw_command_gpu ), CRUDE_GFX_MEMORY_TYPE_GPU, "mesh_task_indirect_commands" );
  scene_renderer->mesh_task_indirect_commands_culled_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, scene_renderer->total_meshes_instances_buffer_capacity * sizeof( crude_gfx_mesh_draw_command_gpu ), CRUDE_GFX_MEMORY_TYPE_GPU, "mesh_task_indirect_commands_culled_hga" );
  scene_renderer->mesh_task_indirect_count_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_mesh_draw_counts_gpu ), CRUDE_GFX_MEMORY_TYPE_GPU, "mesh_task_indirect_count_hga" );

  scene_renderer->debug_commands_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_debug_draw_command_gpu ), CRUDE_GFX_MEMORY_TYPE_GPU, "debug_commands_hga" );
  scene_renderer->debug_line_vertices_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_debug_line_vertex_gpu ) * CRUDE_GRAPHICS_SCENE_RENDERER_MAX_DEBUG_LINES * 2u, CRUDE_GFX_MEMORY_TYPE_GPU, "debug_line_vertices_hga" );
  scene_renderer->debug_cubes_instances_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_debug_cube_instance_gpu ) * CRUDE_GRAPHICS_SCENE_RENDERER_MAX_DEBUG_CUBES, CRUDE_GFX_MEMORY_TYPE_GPU, "debug_cubes_instances_hga" );

  crude_gfx_scene_renderer_on_resize( scene_renderer );

  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Initialize scene renderer passes." );

  if ( scene_renderer->imgui_pass_enalbed )
  {
    crude_gfx_imgui_pass_initialize( &scene_renderer->imgui_pass, scene_renderer );
  }
  crude_gfx_gbuffer_early_pass_initialize( &scene_renderer->gbuffer_early_pass, scene_renderer );
  crude_gfx_gbuffer_late_pass_initialize( &scene_renderer->gbuffer_late_pass, scene_renderer );
  crude_gfx_depth_pyramid_pass_initialize( &scene_renderer->depth_pyramid_pass, scene_renderer );
  //crude_gfx_pointlight_shadow_pass_initialize( &scene_renderer->pointlight_shadow_pass, scene_renderer );
  crude_gfx_culling_early_pass_initialize( &scene_renderer->culling_early_pass, scene_renderer );
  crude_gfx_culling_late_pass_initialize( &scene_renderer->culling_late_pass, scene_renderer );
  crude_gfx_debug_pass_initialize( &scene_renderer->debug_pass, scene_renderer );
  crude_gfx_compose_pass_initialize( &scene_renderer->compose_pass, scene_renderer );
  crude_gfx_postprocessing_pass_initialize( &scene_renderer->postprocessing_pass, scene_renderer );
  crude_gfx_transparent_pass_initialize( &scene_renderer->transparent_pass, scene_renderer );
  crude_gfx_light_lut_pass_initialize( &scene_renderer->light_lut_pass, scene_renderer );
  crude_gfx_ssr_pass_initialize( &scene_renderer->ssr_pass, scene_renderer );
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
#if CRUDE_DEBUG_RAY_TRACING_SOLID_PASS
  crude_gfx_ray_tracing_solid_pass_initialize( &scene_renderer->ray_tracing_solid_pass, scene_renderer );
#endif
  crude_gfx_indirect_light_pass_initialize( &scene_renderer->indirect_light_pass, scene_renderer );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
}

void
crude_gfx_scene_renderer_deinitialize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_render_graph_builder_unregister_all_render_passes( scene_renderer->render_graph->builder );
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
#if CRUDE_DEBUG_RAY_TRACING_SOLID_PASS
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "ray_tracing_solid_pass" );
#endif
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "indirect_light_pass" );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
  
  if ( scene_renderer->imgui_pass_enalbed )
  {
    crude_gfx_imgui_pass_deinitialize( &scene_renderer->imgui_pass );
  }
  crude_gfx_gbuffer_early_pass_deinitialize( &scene_renderer->gbuffer_early_pass );
  crude_gfx_gbuffer_late_pass_deinitialize( &scene_renderer->gbuffer_late_pass );
  crude_gfx_depth_pyramid_pass_deinitialize( &scene_renderer->depth_pyramid_pass );
  //crude_gfx_pointlight_shadow_pass_deinitialize( &scene_renderer->pointlight_shadow_pass );
  crude_gfx_culling_early_pass_deinitialize( &scene_renderer->culling_early_pass );
  crude_gfx_culling_late_pass_deinitialize( &scene_renderer->culling_late_pass );
  crude_gfx_debug_pass_deinitialize( &scene_renderer->debug_pass );
  crude_gfx_compose_pass_deinitialize( &scene_renderer->compose_pass );
  crude_gfx_postprocessing_pass_deinitialize( &scene_renderer->postprocessing_pass );
  crude_gfx_transparent_pass_deinitialize( &scene_renderer->transparent_pass );
  crude_gfx_light_lut_pass_deinitialize( &scene_renderer->light_lut_pass );
  crude_gfx_ssr_pass_deinitialize( &scene_renderer->ssr_pass );
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
#if CRUDE_DEBUG_RAY_TRACING_SOLID_PASS
  crude_gfx_ray_tracing_solid_pass_deinitialize( &scene_renderer->ray_tracing_solid_pass );
#endif
  crude_gfx_indirect_light_pass_deinitialize( &scene_renderer->indirect_light_pass );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->blases_buffers ); ++i )
  {
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->blases_buffers[ i ] );
    scene_renderer->renderer->gpu->vkDestroyAccelerationStructureKHR( scene_renderer->renderer->gpu->vk_device, scene_renderer->vk_blases[ i ], scene_renderer->renderer->gpu->vk_allocation_callbacks );
  }
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->blases_buffers );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->vk_blases );

  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->tlas_buffer );
  scene_renderer->renderer->gpu->vkDestroyAccelerationStructureKHR( scene_renderer->renderer->gpu->vk_device, scene_renderer->vk_tlas, scene_renderer->renderer->gpu->vk_allocation_callbacks );

  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->tlas_scratch_buffer_handle );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->tlas_instances_buffer_handle );
#endif

  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->lights_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->scene_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->meshes_instances_draws_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->mesh_task_indirect_commands_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->mesh_task_indirect_commands_culled_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->mesh_task_indirect_count_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->lights_world_to_clip_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->lights_indices_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->lights_bins_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->lights_tiles_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->debug_commands_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->debug_cubes_instances_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->debug_line_vertices_hga );
  
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->model_renderer_resoruces_instances );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->lights );
}

bool
crude_gfx_scene_renderer_update_instances_from_node
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        main_node
)
{
  bool                                                     buffers_recrteated, model_initialized;
  crude_gfx_buffer_creation                                buffer_creation;
 
  CRUDE_PROFILER_ZONE_NAME( "crude_gfx_scene_renderer_update_instances_from_node" );

  model_initialized = false;

  CRUDE_ARRAY_SET_LENGTH( scene_renderer->model_renderer_resoruces_instances, 0u );
  CRUDE_ARRAY_SET_LENGTH( scene_renderer->lights, 0u );
  crude_scene_renderer_register_nodes_instances_( scene_renderer, world, main_node, &model_initialized );
  
  scene_renderer->total_meshes_instances_count = 0u;
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances ); ++i )
  {
    scene_renderer->total_meshes_instances_count += CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances[ i ].model_renderer_resources.meshes_instances );
  }

  buffers_recrteated = false;
  
  if ( 2.f * scene_renderer->total_meshes_instances_count > scene_renderer->total_meshes_instances_buffer_capacity )
  {
    scene_renderer->total_meshes_instances_buffer_capacity = 4 * scene_renderer->total_meshes_instances_count; /* we need at least 2x because of transparency objects, so do 4x for extensions idk */

    if ( crude_gfx_memory_allocation_valid( &scene_renderer->meshes_instances_draws_hga ) )
    {
      crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->meshes_instances_draws_hga );
    }
    
    if ( crude_gfx_memory_allocation_valid( &scene_renderer->mesh_task_indirect_commands_hga ) )
    {
      crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->mesh_task_indirect_commands_hga );
    }
    
    if ( crude_gfx_memory_allocation_valid( &scene_renderer->mesh_task_indirect_commands_culled_hga ) )
    {
      crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->mesh_task_indirect_commands_culled_hga );
    }

    scene_renderer->meshes_instances_draws_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_mesh_instance_draw_gpu ) * scene_renderer->total_meshes_instances_buffer_capacity, CRUDE_GFX_MEMORY_TYPE_GPU, "meshes_instances_draws" );
    scene_renderer->mesh_task_indirect_commands_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, scene_renderer->total_meshes_instances_buffer_capacity * sizeof( crude_gfx_mesh_draw_command_gpu ), CRUDE_GFX_MEMORY_TYPE_GPU, "mesh_task_indirect_commands" );
    scene_renderer->mesh_task_indirect_commands_culled_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, scene_renderer->total_meshes_instances_buffer_capacity * sizeof( crude_gfx_mesh_draw_command_gpu ), CRUDE_GFX_MEMORY_TYPE_GPU, "mesh_task_indirect_commands_culled_hga" );

    buffers_recrteated = true;
  }
  
  CRUDE_PROFILER_ZONE_END;
  return buffers_recrteated | model_initialized;
}

void
crude_gfx_scene_renderer_rebuild_light_gpu_buffers
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Rebuild light GPU buffers" );

  if ( crude_gfx_memory_allocation_valid( &scene_renderer->lights_hga ) )
  {
    crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->lights_hga );
  }
  
  if ( crude_gfx_memory_allocation_valid( &scene_renderer->lights_world_to_clip_hga ) )
  {
    crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->lights_world_to_clip_hga );
  }
  
  if ( crude_gfx_memory_allocation_valid( &scene_renderer->lights_indices_hga ) )
  {
    crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->lights_indices_hga );
  }
  
  if ( crude_gfx_memory_allocation_valid( &scene_renderer->lights_bins_hga ) )
  {
    crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->lights_bins_hga );
  }

  scene_renderer->lights_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_light_gpu ) * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHTS_MAX_COUNT, CRUDE_GFX_MEMORY_TYPE_GPU, "lights" );
  scene_renderer->lights_world_to_clip_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( XMFLOAT4X4 ) * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHTS_MAX_COUNT * 4u, CRUDE_GFX_MEMORY_TYPE_GPU, "lights_world_to_clip" );
  scene_renderer->lights_indices_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( uint32 ) * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHTS_MAX_COUNT, CRUDE_GFX_MEMORY_TYPE_GPU, "lights_indices" );
  scene_renderer->lights_bins_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( uint32 ) * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_Z_BINS, CRUDE_GFX_MEMORY_TYPE_GPU, "lights_bins" );
}

void
crude_gfx_scene_renderer_submit_draw_task
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ bool                                                use_secondary
)
{
  crude_gfx_cmd_buffer                                    *primary_cmd;
  
  CRUDE_PROFILER_ZONE_NAME( "crude_gfx_scene_renderer_submit_draw_task" );
  primary_cmd = crude_gfx_get_primary_cmd( scene_renderer->gpu, 0, true );
  crude_gfx_cmd_push_marker( primary_cmd, "render_graph" );
  update_dynamic_buffers_( scene_renderer, primary_cmd );
  crude_gfx_render_graph_render( scene_renderer->render_graph, primary_cmd );
  crude_gfx_cmd_pop_marker( primary_cmd );
  crude_gfx_queue_cmd( primary_cmd );
  CRUDE_PROFILER_ZONE_END;
}

void
crude_gfx_scene_renderer_register_passes
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_render_graph                             *render_graph
)
{
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Register scene renderer passes." );

  scene_renderer->render_graph = render_graph;
  
  if ( scene_renderer->imgui_pass_enalbed )
  {
    crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "imgui_pass", crude_gfx_imgui_pass_pack( &scene_renderer->imgui_pass ) );
  }

  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "gbuffer_early_pass", crude_gfx_gbuffer_early_pass_pack( &scene_renderer->gbuffer_early_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "gbuffer_late_pass", crude_gfx_gbuffer_late_pass_pack( &scene_renderer->gbuffer_late_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "depth_pyramid_pass", crude_gfx_depth_pyramid_pass_pack( &scene_renderer->depth_pyramid_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "culling_early_pass", crude_gfx_culling_early_pass_pack( &scene_renderer->culling_early_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "culling_late_pass", crude_gfx_culling_late_pass_pack( &scene_renderer->culling_late_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "debug_pass", crude_gfx_debug_pass_pack( &scene_renderer->debug_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "compose_pass", crude_gfx_compose_pass_pack( &scene_renderer->compose_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "postprocessing_pass", crude_gfx_postprocessing_pass_pack( &scene_renderer->postprocessing_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "transparent_pass", crude_gfx_transparent_pass_pack( &scene_renderer->transparent_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "light_lut_pass", crude_gfx_light_lut_pass_pack( &scene_renderer->light_lut_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "ssr_pass", crude_gfx_ssr_pass_pack( &scene_renderer->ssr_pass ) );
  //crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "point_shadows_pass", crude_gfx_pointlight_shadow_pass_pack( &scene_renderer->pointlight_shadow_pass ) );
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
#if CRUDE_DEBUG_RAY_TRACING_SOLID_PASS
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "ray_tracing_solid_pass", crude_gfx_ray_tracing_solid_pass_pack( &scene_renderer->ray_tracing_solid_pass ) );
#endif
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "indirect_light_pass", crude_gfx_indirect_light_pass_pack( &scene_renderer->indirect_light_pass ) );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
}

void
crude_gfx_scene_renderer_on_resize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_gfx_scene_renderer_on_resize" );
  /* Reinitialize light gpu data */
  if ( crude_gfx_memory_allocation_valid( &scene_renderer->lights_tiles_hga ) )
  {
    crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->lights_tiles_hga );
  }
  
  uint32 tile_x_count = scene_renderer->gpu->renderer_size.x / CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_TILE_SIZE;
  uint32 tile_y_count = scene_renderer->gpu->renderer_size.y / CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_TILE_SIZE;
  uint32 tiles_entry_count = tile_x_count * tile_y_count * CRUDE_GRAPHICS_SCENE_RENDERER_LIGHT_WORDS_COUNT;
  scene_renderer->lights_tiles_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( uint32 ) * tiles_entry_count, CRUDE_GFX_MEMORY_TYPE_GPU, "lights_tiles" );

  CRUDE_PROFILER_ZONE_END;
}

/**
 * Scene Renderer Other
 */
static void
update_dynamic_buffers_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_device                                        *gpu;

  gpu = scene_renderer->gpu;
  
  /* Update scene constant buffer*/
  {
    crude_gfx_scene_constant_gpu                          *scene;
    crude_gfx_memory_allocation                            scene_tca;

    scene_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, sizeof( crude_gfx_scene_constant_gpu ) );
    scene = CRUDE_CAST( crude_gfx_scene_constant_gpu*, scene_tca.cpu_address );

    *scene = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene_constant_gpu );
    scene->flags = 0u;
    scene->camera_previous = scene->camera;
    scene->resolution.x = scene_renderer->gpu->renderer_size.x;
    scene->resolution.y = scene_renderer->gpu->renderer_size.y;
    scene->resolution_ratio = CRUDE_CAST( float32, scene_renderer->gpu->renderer_size.x ) / scene_renderer->gpu->renderer_size.y;
    crude_gfx_camera_to_camera_gpu( &scene_renderer->options.scene.camera, scene_renderer->options.scene.camera_view_to_world, &scene->camera );
    scene->meshes_instances_count = scene_renderer->total_visible_meshes_instances_count;
    scene->active_lights_count = CRUDE_ARRAY_LENGTH( scene_renderer->lights );
    //scene->tiled_shadowmap_texture_index = scene_renderer->pointlight_shadow_pass.tetrahedron_shadow_texture.index;
    scene->inv_shadow_map_size.x = 1.f / CRUDE_GRAPHICS_TETRAHEDRON_SHADOWMAP_SIZE;
    scene->inv_shadow_map_size.y = 1.f / CRUDE_GRAPHICS_TETRAHEDRON_SHADOWMAP_SIZE;
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
    scene->indirect_light_texture_index = scene_renderer->indirect_light_pass.indirect_texture_handle.index;
#else
    scene->indirect_light_texture_index = -1;
#endif
    scene->background_color = scene_renderer->options.scene.background_color;
    scene->background_intensity = scene_renderer->options.scene.background_intensity;
    scene->ambient_color = scene_renderer->options.scene.ambient_color;
    scene->ambient_intensity = scene_renderer->options.scene.ambient_intensity;
    scene->absolute_time = scene_renderer->options.absolute_time;
    scene->absolute_frame = scene_renderer->gpu->absolute_frame;
    scene->debug_mode = scene_renderer->options.debug.debug_mode;

    crude_gfx_cmd_memory_copy( primary_cmd, scene_tca, scene_renderer->scene_hga, 0, 0 );
  }
  
  /* Update meshes instanse draws buffers*/
  {
    crude_gfx_mesh_instance_draw_gpu                      *meshes_instances_draws;
    crude_gfx_memory_allocation                            meshes_instances_draws_tca;

    meshes_instances_draws_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, sizeof( crude_gfx_mesh_instance_draw_gpu ) * scene_renderer->total_meshes_instances_count );
    meshes_instances_draws = CRUDE_CAST( crude_gfx_mesh_instance_draw_gpu*, meshes_instances_draws_tca.cpu_address );
  
    scene_renderer->total_visible_meshes_instances_count = 0u;
    for ( uint32 model_instance_index = 0; model_instance_index < CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances ); ++model_instance_index )
    {
      crude_gfx_model_renderer_resources_instance       *model_renderer_resources_instance;

      model_renderer_resources_instance = &scene_renderer->model_renderer_resoruces_instances[ model_instance_index ];
      
      for ( uint32 model_mesh_instance_index = 0; model_mesh_instance_index < CRUDE_ARRAY_LENGTH( model_renderer_resources_instance->model_renderer_resources.meshes_instances ); ++model_mesh_instance_index )
      {
        XMMATRIX                                           mesh_to_model, model_to_world, mesh_to_world;
        crude_gfx_mesh_instance_cpu                       *mesh_instance_cpu;
        
        mesh_instance_cpu = &model_renderer_resources_instance->model_renderer_resources.meshes_instances[ model_mesh_instance_index ];
        
        mesh_to_model = XMLoadFloat4x4( &mesh_instance_cpu->mesh_to_model );
        model_to_world = XMLoadFloat4x4( &model_renderer_resources_instance->model_to_world );
        
        mesh_to_world = XMMatrixMultiply( mesh_to_model, model_to_world );

        XMStoreFloat4x4( &meshes_instances_draws[ scene_renderer->total_visible_meshes_instances_count ].mesh_to_world, mesh_to_world );
        XMStoreFloat4x4( &meshes_instances_draws[ scene_renderer->total_visible_meshes_instances_count ].world_to_mesh, XMMatrixInverse( NULL, mesh_to_world ) );
        meshes_instances_draws[ scene_renderer->total_visible_meshes_instances_count ].mesh_draw_index = mesh_instance_cpu->mesh_gpu_index;

        ++scene_renderer->total_visible_meshes_instances_count;
      }
    }

    crude_gfx_cmd_memory_copy( primary_cmd, meshes_instances_draws_tca, scene_renderer->meshes_instances_draws_hga, 0, 0 );
  }
  
  /* Update meshlets counes storage buffers*/
  {
    crude_gfx_mesh_draw_counts_gpu                        *mesh_draw_counts;
    crude_gfx_memory_allocation                            mesh_task_indirect_count_tca;

    mesh_task_indirect_count_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, sizeof( crude_gfx_mesh_draw_counts_gpu ) );
    mesh_draw_counts = CRUDE_CAST( crude_gfx_mesh_draw_counts_gpu*, mesh_task_indirect_count_tca.cpu_address );

    *mesh_draw_counts = CRUDE_COMPOUNT_EMPTY( crude_gfx_mesh_draw_counts_gpu );
    mesh_draw_counts->opaque_mesh_visible_early_count = 0u;
    mesh_draw_counts->opaque_mesh_culled_count = 0u;
    mesh_draw_counts->opaque_mesh_visible_late_count = 0u;
    mesh_draw_counts->transparent_mesh_visible_count = 0u;
    mesh_draw_counts->transparent_mesh_culled_count = 0u;
    mesh_draw_counts->total_mesh_count = scene_renderer->total_visible_meshes_instances_count;
    mesh_draw_counts->depth_pyramid_texture_index = scene_renderer->depth_pyramid_pass.depth_pyramid_texture_handle.index;

    crude_gfx_cmd_memory_copy( primary_cmd, mesh_task_indirect_count_tca, scene_renderer->mesh_task_indirect_count_hga, 0, 0 );
  }
  
  /* Update debug draw commands */
  {
    crude_gfx_debug_draw_command_gpu                      *debug_draw_command;
    crude_gfx_memory_allocation                            debug_draw_command_tca;

    debug_draw_command_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, sizeof( crude_gfx_debug_draw_command_gpu ) );
    debug_draw_command = CRUDE_CAST( crude_gfx_debug_draw_command_gpu*, debug_draw_command_tca.cpu_address );

    *debug_draw_command = CRUDE_COMPOUNT_EMPTY( crude_gfx_debug_draw_command_gpu );
    debug_draw_command->draw_indirect_2dline.instanceCount = 1u;
    debug_draw_command->draw_indirect_3dline.instanceCount = 1u;
    debug_draw_command->draw_indirect_cube.vertexCount = 36u;
    
    crude_gfx_cmd_memory_copy( primary_cmd, debug_draw_command_tca, scene_renderer->debug_commands_hga, 0, 0 );
  }

  scene_renderer->total_visible_lights_count = CRUDE_ARRAY_LENGTH( scene_renderer->lights );
}

void
crude_scene_renderer_register_nodes_instances_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _Out_opt_ bool                                          *model_initialized
)
{
  ecs_iter_t                                               children_it;
  bool                                                     local_model_initialized;
  XMMATRIX                                                 model_to_custom_model;

  children_it = crude_ecs_children( world, node );
  local_model_initialized = false;

  model_to_custom_model = XMMatrixIdentity( );
  
  if ( CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_gltf ) )
  {
    crude_gltf const                                  *child_gltf;
    crude_gfx_model_renderer_resources_instance        model_renderer_resources_instant;
  
    child_gltf = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_gltf );
  
    if ( !child_gltf->hidden )
    {
      model_renderer_resources_instant = CRUDE_COMPOUNT_EMPTY( crude_gfx_model_renderer_resources_instance );
      model_renderer_resources_instant.model_renderer_resources = crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, child_gltf->path, &local_model_initialized );
      model_renderer_resources_instant.type = CRUDE_GFX_MODEL_RENDERER_RESOURCES_INSTANCE_TYPE_GLTF;
      XMStoreFloat4x4( &model_renderer_resources_instant.model_to_world, XMMatrixMultiply( model_to_custom_model, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform ) ) ) );
      CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, model_renderer_resources_instant );
    }
  }

#if CRUDE_DEVELOP
  if ( CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_debug_collision ) && CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_physics_collision_shape ) )
  {
    crude_debug_collision const                       *child_debug_collision;
    crude_physics_collision_shape const               *collision_shape;
    crude_gfx_model_renderer_resources_instance        model_renderer_resources_instant;
    
    child_debug_collision = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_debug_collision );
    collision_shape = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, node, crude_physics_collision_shape );
        
    if ( collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
    {
      model_to_custom_model = XMMatrixScalingFromVector( XMLoadFloat3( &collision_shape->box.half_extent ) );
    }
    else if ( collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
    {
      model_to_custom_model = XMMatrixScaling( collision_shape->sphere.radius, collision_shape->sphere.radius, collision_shape->sphere.radius );
    }
    else if ( collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_MESH )
    {
      model_to_custom_model = XMMatrixIdentity( );
    }
    else
    {
      CRUDE_ASSERT( false );
    }
    
    if ( !scene_renderer->options.debug.hide_collision && child_debug_collision->visible )
    {
      model_renderer_resources_instant = CRUDE_COMPOUNT_EMPTY( crude_gfx_model_renderer_resources_instance );
      model_renderer_resources_instant.model_renderer_resources = crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, child_debug_collision->absolute_filepath, &local_model_initialized );
      model_renderer_resources_instant.type = CRUDE_GFX_MODEL_RENDERER_RESOURCES_INSTANCE_TYPE_DUBUG_COLLISION;
      XMStoreFloat4x4( &model_renderer_resources_instant.model_to_world, XMMatrixMultiply( model_to_custom_model, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform ) ) ) );
      CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, model_renderer_resources_instant );
    }
  }
  if ( CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_debug_gltf ) )
  {
    crude_debug_gltf const                            *child_gltf;
    crude_gfx_model_renderer_resources_instance        model_renderer_resources_instant;
    
    child_gltf = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_debug_gltf );
    if ( !scene_renderer->options.debug.hide_debug_gltf && child_gltf->visible )
    {
      model_renderer_resources_instant = CRUDE_COMPOUNT_EMPTY( crude_gfx_model_renderer_resources_instance );
      model_renderer_resources_instant.model_renderer_resources = crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, child_gltf->absolute_filepath, &local_model_initialized );
      model_renderer_resources_instant.type = CRUDE_GFX_MODEL_RENDERER_RESOURCES_INSTANCE_TYPE_DUBUG_GLTF;
      XMStoreFloat4x4( &model_renderer_resources_instant.model_to_world, XMMatrixMultiply( model_to_custom_model, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform ) ) ) );
      CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, model_renderer_resources_instant );
    }
  }
#endif

  if ( CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_light ) )
  {
    crude_gfx_light_cpu light_gpu = CRUDE_COMPOUNT_EMPTY( crude_gfx_light_cpu );
    light_gpu.light = *CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, node, crude_light );
    XMStoreFloat3( &light_gpu.translation, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, node, crude_transform ) ).r[ 3 ] );
    CRUDE_ARRAY_PUSH( scene_renderer->lights, light_gpu );
  }
  
  *model_initialized |= local_model_initialized;

  while ( ecs_children_next( &children_it ) )
  {
    for ( size_t i = 0; i < children_it.count; ++i )
    {
      crude_entity child = crude_entity_from_iterator( &children_it, i );
      crude_scene_renderer_register_nodes_instances_( scene_renderer, world, child, &local_model_initialized );
      *model_initialized |= local_model_initialized;
    }
  }
}