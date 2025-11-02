#include <TaskScheduler_c.h>
#include <cgltf.h>
#include <stb_image.h>
#include <meshoptimizer.h>

#include <core/profiler.h>
#include <core/array.h>
#include <core/file.h>
#include <core/hash_map.h>

#include <scene/scene_components.h>
#include <scene/scene.h>

#include <graphics/gpu_profiler.h>
#include <graphics/scene_renderer.h>

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
static int
sorting_light_
(
  _In_ const void                                         *a,
  _In_ const void                                         *b
);

void
crude_scene_renderer_register_nodes_instances_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_entity                                        node
)
{
  ecs_iter_t it = ecs_children( node.world, node.handle );

  while ( ecs_children_next( &it ) )
  {
    for ( size_t i = 0; i < it.count; ++i )
    {
      crude_entity child = CRUDE_COMPOUNT( crude_entity, { .handle = it.entities[ i ], .world = node.world } );
      if ( CRUDE_ENTITY_HAS_COMPONENT( child, crude_gltf ) )
      {
        crude_gltf const                                  *child_gltf;
        crude_gfx_model_renderer_resources_instance        model_renderer_resources_instant;

        child_gltf = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( child, crude_gltf );

        model_renderer_resources_instant = CRUDE_COMPOUNT_EMPTY( crude_gfx_model_renderer_resources_instance );
        model_renderer_resources_instant.model_renderer_resources = crude_gfx_model_renderer_resources_manager_add_gltf_model( scene_renderer->model_renderer_resources_manager, child_gltf->path );
        model_renderer_resources_instant.node = child;
        CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, model_renderer_resources_instant );
      }
      if ( CRUDE_ENTITY_HAS_COMPONENT( child, crude_light ) )
      {
        crude_gfx_light_cpu light_gpu = CRUDE_COMPOUNT_EMPTY( crude_gfx_light_cpu );
        light_gpu.node = child;
        CRUDE_ARRAY_PUSH( scene_renderer->lights, light_gpu );
      }

      crude_scene_renderer_register_nodes_instances_( scene_renderer, child );
    }
  }
}
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
  
  /* Context */
  scene_renderer->scene = creation->scene;
  scene_renderer->allocator = creation->allocator;
  scene_renderer->resources_allocator = creation->resources_allocator;
  scene_renderer->temporary_allocator = creation->temporary_allocator;
  scene_renderer->gpu = creation->gpu;
  scene_renderer->async_loader = creation->async_loader;
  scene_renderer->task_scheduler = creation->task_scheduler;
  scene_renderer->imgui_context = creation->imgui_context;
  scene_renderer->cgltf_temporary_allocator = creation->cgltf_temporary_allocator;

  scene_renderer->options.background_color = CRUDE_COMPOUNT( XMFLOAT3, { 0.529, 0.807, 0.921 } );
  scene_renderer->options.background_intensity = 1.f;
  scene_renderer->options.ambient_color = CRUDE_COMPOUNT( XMFLOAT3, { 0, 0, 0 } );
  scene_renderer->options.ambient_intensity = 1.f;

  /* Common lights arrays initialization */
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->lights, 0u, crude_heap_allocator_pack( scene_renderer->resources_allocator ) );
  

  /* Common gpu data */
  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  buffer_creation.size = sizeof( crude_gfx_scene_constant_gpu );
  buffer_creation.name = "scene_cb";
  scene_renderer->scene_cb = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( crude_gfx_mesh_draw_counts_gpu );
    buffer_creation.name = "mesh_count_early_sb";
    scene_renderer->mesh_task_indirect_count_early_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( crude_gfx_mesh_draw_counts_gpu );
    buffer_creation.name = "mesh_count_late_sb";
    scene_renderer->mesh_task_indirect_count_late_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( crude_gfx_debug_draw_command_gpu );
    buffer_creation.name = "debug_line_commands";
    scene_renderer->debug_commands_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );
    
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof( crude_gfx_debug_line_vertex_gpu ) * CRUDE_GFX_MAX_DEBUG_LINES * 2u; /* 2 vertices per line */
    buffer_creation.device_only = true;
    buffer_creation.name = "debug_line_vertices";
    scene_renderer->debug_line_vertices_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof( crude_gfx_debug_cube_instance_gpu ) * CRUDE_GFX_MAX_DEBUG_CUBES;
    buffer_creation.device_only = true;
    buffer_creation.name = "debug_cubes_sb";
    scene_renderer->debug_cubes_instances_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );
  }

  crude_gfx_scene_renderer_on_resize( scene_renderer );

  scene_renderer->ray_trace_shadows = true;

  /* Initialize render graph passes */
  crude_gfx_imgui_pass_initialize( &scene_renderer->imgui_pass, scene_renderer );
  crude_gfx_gbuffer_early_pass_initialize( &scene_renderer->gbuffer_early_pass, scene_renderer );
  crude_gfx_gbuffer_late_pass_initialize( &scene_renderer->gbuffer_late_pass, scene_renderer );
  crude_gfx_depth_pyramid_pass_initialize( &scene_renderer->depth_pyramid_pass, scene_renderer );
  crude_gfx_pointlight_shadow_pass_initialize( &scene_renderer->pointlight_shadow_pass, scene_renderer );
  crude_gfx_culling_early_pass_initialize( &scene_renderer->culling_early_pass, scene_renderer );
  crude_gfx_culling_late_pass_initialize( &scene_renderer->culling_late_pass, scene_renderer );
  crude_gfx_debug_pass_initialize( &scene_renderer->debug_pass, scene_renderer );
  crude_gfx_light_pass_initialize( &scene_renderer->light_pass, scene_renderer );
  crude_gfx_postprocessing_pass_initialize( &scene_renderer->postprocessing_pass, scene_renderer );
#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
#ifdef CRUDE_DEBUG_RAY_TRACING_SOLID_PASS
  crude_gfx_ray_tracing_solid_pass_initialize( &scene_renderer->ray_tracing_solid_pass, scene_renderer );
#endif
  crude_gfx_indirect_light_pass_initialize( &scene_renderer->indirect_light_pass, scene_renderer );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
}

void
crude_gfx_scene_renderer_rebuild_main_node
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_entity                                        main_node
)
{
  crude_gfx_buffer_creation                                buffer_creation;

  CRUDE_ARRAY_SET_LENGTH( scene_renderer->model_renderer_resoruces_instances, 0u );
  crude_scene_renderer_register_nodes_instances_( scene_renderer, main_node );
  
  scene_renderer->meshes_instances_count = 0u;
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances ); ++i )
  {
    scene_renderer->meshes_instances_count += CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances[ i ].model_renderer_resources.meshes_instances );
  }

  /* Recreate buffers related to meshes */
  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  buffer_creation.size = sizeof( crude_gfx_mesh_instance_draw_gpu ) * scene_renderer->meshes_instances_count;
  buffer_creation.name = "meshes_instances_draws_sb";
  scene_renderer->meshes_instances_draws_sb = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = scene_renderer->meshes_instances_count * sizeof( crude_gfx_mesh_draw_command_gpu );
    buffer_creation.name = "draw_commands_early_sb";
    buffer_creation.device_only = true;
    scene_renderer->mesh_task_indirect_commands_early_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = scene_renderer->meshes_instances_count * sizeof( crude_gfx_mesh_draw_command_gpu );
    buffer_creation.device_only = true;
    buffer_creation.name = "draw_commands_late_sb";
    scene_renderer->mesh_task_indirect_commands_late_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );
  }

  /* Recreate data related to light */
  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  buffer_creation.size = sizeof( crude_gfx_light_gpu ) * CRUDE_ARRAY_LENGTH( scene_renderer->lights );
  buffer_creation.name = "lights_sb";
  scene_renderer->lights_sb = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );
  
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    scene_renderer->lights_tiles_sb[ i ] = CRUDE_GFX_BUFFER_HANDLE_INVALID; /* would be initialized in resize */
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( XMFLOAT4X4 ) * CRUDE_GFX_LIGHTS_MAX_COUNT * 4u;
    buffer_creation.name = "pointlight_world_to_clip_sb";
    scene_renderer->pointlight_world_to_clip_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( uint32 ) * CRUDE_ARRAY_LENGTH( scene_renderer->lights );
    buffer_creation.name = "lights_indices_sb";
    scene_renderer->lights_indices_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( uint32 ) * CRUDE_GFX_LIGHT_Z_BINS;
    buffer_creation.name = "lights_bins_sb";
    scene_renderer->lights_bins_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );
  }
}

void
crude_gfx_scene_renderer_deinitialize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "gbuffer_early_pass" );
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "gbuffer_late_pass" );
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "imgui_pass" );
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "depth_pyramid_pass" );
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "culling_early_pass" );
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "culling_late_pass" );
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "debug_pass" );
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "light_pass" );
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "postprocessing_pass" );
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "point_shadows_pass" );
#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
#ifdef CRUDE_DEBUG_RAY_TRACING_SOLID_PASS
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "ray_tracing_solid_pass" );
#endif
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "indirect_light_pass" );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */

  crude_gfx_imgui_pass_deinitialize( &scene_renderer->imgui_pass );
  crude_gfx_gbuffer_early_pass_deinitialize( &scene_renderer->gbuffer_early_pass );
  crude_gfx_gbuffer_late_pass_deinitialize( &scene_renderer->gbuffer_late_pass );
  crude_gfx_depth_pyramid_pass_deinitialize( &scene_renderer->depth_pyramid_pass );
  crude_gfx_pointlight_shadow_pass_deinitialize( &scene_renderer->pointlight_shadow_pass );
  crude_gfx_culling_early_pass_deinitialize( &scene_renderer->culling_early_pass );
  crude_gfx_culling_late_pass_deinitialize( &scene_renderer->culling_late_pass );
  crude_gfx_debug_pass_deinitialize( &scene_renderer->debug_pass );
  crude_gfx_light_pass_deinitialize( &scene_renderer->light_pass );
  crude_gfx_postprocessing_pass_deinitialize( &scene_renderer->postprocessing_pass );
#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
#ifdef CRUDE_DEBUG_RAY_TRACING_SOLID_PASS
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

  crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->lights_sb );
  crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->scene_cb );
  crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->meshes_instances_draws_sb );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->pointlight_world_to_clip_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->lights_indices_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->lights_bins_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->lights_tiles_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->debug_cubes_instances_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->debug_line_vertices_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->debug_commands_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->mesh_task_indirect_commands_early_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->mesh_task_indirect_count_early_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->mesh_task_indirect_commands_late_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->mesh_task_indirect_count_late_sb[ i ] );
  }
  
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->lights );
}

void
crude_gfx_scene_renderer_submit_draw_task
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ bool                                                use_secondary
)
{
  crude_gfx_cmd_buffer                                    *primary_cmd;

  primary_cmd = crude_gfx_get_primary_cmd( scene_renderer->gpu, 0, true );
  crude_gfx_cmd_push_marker( primary_cmd, "render_graph" );
  update_dynamic_buffers_( scene_renderer, primary_cmd );
  crude_gfx_render_graph_render( scene_renderer->render_graph, primary_cmd );
  crude_gfx_cmd_pop_marker( primary_cmd );
  crude_gfx_queue_cmd( primary_cmd );
}

void
crude_gfx_scene_renderer_register_passes
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_render_graph                             *render_graph
)
{
  scene_renderer->render_graph = render_graph;

  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "gbuffer_early_pass", crude_gfx_gbuffer_early_pass_pack( &scene_renderer->gbuffer_early_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "gbuffer_late_pass", crude_gfx_gbuffer_late_pass_pack( &scene_renderer->gbuffer_late_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "imgui_pass", crude_gfx_imgui_pass_pack( &scene_renderer->imgui_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "depth_pyramid_pass", crude_gfx_depth_pyramid_pass_pack( &scene_renderer->depth_pyramid_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "culling_early_pass", crude_gfx_culling_early_pass_pack( &scene_renderer->culling_early_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "culling_late_pass", crude_gfx_culling_late_pass_pack( &scene_renderer->culling_late_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "debug_pass", crude_gfx_debug_pass_pack( &scene_renderer->debug_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "light_pass", crude_gfx_light_pass_pack( &scene_renderer->light_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "postprocessing_pass", crude_gfx_postprocessing_pass_pack( &scene_renderer->postprocessing_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "point_shadows_pass", crude_gfx_pointlight_shadow_pass_pack( &scene_renderer->pointlight_shadow_pass ) );
#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
#ifdef CRUDE_DEBUG_RAY_TRACING_SOLID_PASS
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "ray_tracing_solid_pass", crude_gfx_ray_tracing_solid_pass_pack( &scene_renderer->ray_tracing_solid_pass ) );
#endif
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "indirect_light_pass", crude_gfx_indirect_light_pass_pack( &scene_renderer->indirect_light_pass ) );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */

  crude_gfx_depth_pyramid_pass_on_render_graph_registered( &scene_renderer->depth_pyramid_pass );
  crude_gfx_light_pass_on_render_graph_registered( &scene_renderer->light_pass );
  crude_gfx_postprocessing_pass_on_render_graph_registered( &scene_renderer->postprocessing_pass );
}

void
crude_gfx_scene_renderer_on_resize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_buffer_creation                                buffer_creation;
  
  /* Reinitialize light gpu data */
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( scene_renderer->lights_tiles_sb[ i ] ) )
    {
      crude_gfx_destroy_buffer( scene_renderer->gpu, scene_renderer->lights_tiles_sb[ i ] );
    }
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    uint32 tile_x_count = scene_renderer->gpu->vk_swapchain_width / CRUDE_GFX_LIGHT_TILE_SIZE;
    uint32 tile_y_count = scene_renderer->gpu->vk_swapchain_height / CRUDE_GFX_LIGHT_TILE_SIZE;
    uint32 tiles_entry_count = tile_x_count * tile_y_count * CRUDE_GFX_LIGHT_WORDS_COUNT;
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( uint32 ) * tiles_entry_count;
    buffer_creation.name = "lights_tiles_sb";
    scene_renderer->lights_tiles_sb[ i ] = crude_gfx_create_buffer( scene_renderer->gpu, &buffer_creation );
  }
}

/**
 *
 * Renderer Scene Utils
 * 
 */
void
crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
)
{
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->debug_commands_sb[ frame ], 50u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->debug_line_vertices_sb[ frame ], 51u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->debug_cubes_instances_sb[ frame ], 52u );
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
  crude_gfx_map_buffer_parameters                          buffer_map;

  gpu = scene_renderer->gpu;

  /* Update scene constant buffer*/
  {
    crude_gfx_scene_constant_gpu                          *scene_constant;

    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = scene_renderer->scene_cb;
    buffer_map.offset = 0;
    buffer_map.size = sizeof( crude_gfx_scene_constant_gpu );
    scene_constant = CRUDE_CAST( crude_gfx_scene_constant_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
    if ( scene_constant )
    {
      *scene_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene_constant_gpu );
      scene_constant->flags = 0u;
      scene_constant->camera_previous = scene_constant->camera;
      scene_constant->resolution.x = scene_renderer->gpu->vk_swapchain_width;
      scene_constant->resolution.y = scene_renderer->gpu->vk_swapchain_height;
      crude_gfx_camera_to_camera_gpu( scene_renderer->options.camera_node, &scene_constant->camera );
      scene_constant->meshes_instances_count = scene_renderer->meshes_instances_count;
      scene_constant->active_lights_count = CRUDE_ARRAY_LENGTH( scene_renderer->lights );
      scene_constant->tiled_shadowmap_texture_index = scene_renderer->pointlight_shadow_pass.tetrahedron_shadow_texture.index;
      scene_constant->inv_shadow_map_size.x = 1.f / CRUDE_GFX_TETRAHEDRON_SHADOWMAP_WIDTH;
      scene_constant->inv_shadow_map_size.y = 1.f / CRUDE_GFX_TETRAHEDRON_SHADOWMAP_HEIGHT;
#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
      scene_constant->indirect_light_texture_index = scene_renderer->indirect_light_pass.indirect_texture_handle.index;
#else
      scene_constant->indirect_light_texture_index = -1;
#endif
      scene_constant->background_color = scene_renderer->options.background_color;
      scene_constant->background_intensity = scene_renderer->options.background_intensity;
      scene_constant->ambient_color = scene_renderer->options.ambient_color;
      scene_constant->ambient_intensity = scene_renderer->options.ambient_intensity;
      crude_gfx_unmap_buffer( gpu, scene_renderer->scene_cb );
    }
  }

  /* Update meshes instanse draws buffers*/
  {
    crude_gfx_mesh_instance_draw_gpu                      *meshes_instances_draws;
  
    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = scene_renderer->meshes_instances_draws_sb;
    buffer_map.offset = 0;
    buffer_map.size = sizeof( crude_gfx_mesh_instance_draw_gpu ) * scene_renderer->meshes_instances_count;
    meshes_instances_draws = CRUDE_CAST( crude_gfx_mesh_instance_draw_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
  
    if ( meshes_instances_draws )
    {
      scene_renderer->total_meshes_instances_count = 0u;
      for ( uint32 model_instance_index = 0; model_instance_index < CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances ); ++model_instance_index )
      {
        crude_gfx_model_renderer_resources_instance       *model_renderer_resources_instance;

        model_renderer_resources_instance = &scene_renderer->model_renderer_resoruces_instances[ model_instance_index ];

        for ( uint32 model_mesh_instance_index = 0; model_mesh_instance_index < CRUDE_ARRAY_LENGTH( model_renderer_resources_instance->model_renderer_resources.meshes_instances ); ++model_mesh_instance_index )
        {
          crude_transform const                             *mesh_transform, *model_transform;
          XMMATRIX                                           mesh_to_model, model_to_world, mesh_to_world;
          crude_gfx_mesh_instance_cpu                       *mesh_instance_cpu;
          
          mesh_instance_cpu = &model_renderer_resources_instance->model_renderer_resources.meshes_instances[ model_mesh_instance_index ];
          
          mesh_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( mesh_instance_cpu->node, crude_transform );
          mesh_to_model = crude_transform_node_to_world( mesh_instance_cpu->node, mesh_transform );

          model_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( model_renderer_resources_instance->node, crude_transform );
          model_to_world = crude_transform_node_to_world( model_renderer_resources_instance->node, model_transform );

          mesh_to_world = XMMatrixMultiply( mesh_to_model, model_to_world );

          XMStoreFloat4x4( &meshes_instances_draws[ scene_renderer->total_meshes_instances_count ].mesh_to_world, mesh_to_world );
          XMStoreFloat4x4( &meshes_instances_draws[ scene_renderer->total_meshes_instances_count ].world_to_mesh, XMMatrixInverse( NULL, mesh_to_world ) );
          meshes_instances_draws[ scene_renderer->total_meshes_instances_count ].mesh_draw_index = mesh_instance_cpu->mesh_gpu_index;

          ++scene_renderer->total_meshes_instances_count;
        }
      }
    }

    if ( meshes_instances_draws )
    {
      crude_gfx_unmap_buffer( gpu, scene_renderer->meshes_instances_draws_sb );
    }
  }
  
  /* Update meshlets counes storage buffers*/
  {
    crude_gfx_mesh_draw_counts_gpu                        *mesh_draw_counts_early;
    crude_gfx_mesh_draw_counts_gpu                        *mesh_draw_counts_late;

    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = scene_renderer->mesh_task_indirect_count_early_sb[ gpu->current_frame ];
    buffer_map.offset = 0;
    buffer_map.size = sizeof( crude_gfx_mesh_draw_counts_gpu );
    mesh_draw_counts_early = CRUDE_CAST( crude_gfx_mesh_draw_counts_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
    if ( mesh_draw_counts_early )
    {
      *mesh_draw_counts_early = CRUDE_COMPOUNT_EMPTY( crude_gfx_mesh_draw_counts_gpu );
      mesh_draw_counts_early->total_count = scene_renderer->total_meshes_instances_count;
      mesh_draw_counts_early->depth_pyramid_texture_index = scene_renderer->depth_pyramid_pass.depth_pyramid_texture_handle.index;
      mesh_draw_counts_early->occlusion_culling_late_flag = false;
      crude_gfx_unmap_buffer( gpu, scene_renderer->mesh_task_indirect_count_early_sb[ gpu->current_frame ] );
    }
  
    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = scene_renderer->mesh_task_indirect_count_late_sb[ gpu->current_frame ];
    buffer_map.offset = 0;
    buffer_map.size = sizeof( crude_gfx_mesh_draw_counts_gpu );
    mesh_draw_counts_late = CRUDE_CAST( crude_gfx_mesh_draw_counts_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
    if ( mesh_draw_counts_late )
    {
      *mesh_draw_counts_late = CRUDE_COMPOUNT_EMPTY( crude_gfx_mesh_draw_counts_gpu );
      mesh_draw_counts_late->total_count = scene_renderer->total_meshes_instances_count;
      mesh_draw_counts_late->depth_pyramid_texture_index = scene_renderer->depth_pyramid_pass.depth_pyramid_texture_handle.index;
      mesh_draw_counts_late->occlusion_culling_late_flag = true;
      crude_gfx_unmap_buffer( gpu, scene_renderer->mesh_task_indirect_count_late_sb[ gpu->current_frame ] );
    }
  }
  
  /* Update debug draw commands */
  {
    crude_gfx_debug_draw_command_gpu                      *debug_draw_command;
  
    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = scene_renderer->debug_commands_sb[ gpu->current_frame ];
    buffer_map.offset = 0;
    buffer_map.size = sizeof( crude_gfx_debug_draw_command_gpu );
    debug_draw_command = CRUDE_CAST( crude_gfx_debug_draw_command_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
    if ( debug_draw_command )
    {
      *debug_draw_command = CRUDE_COMPOUNT_EMPTY( crude_gfx_debug_draw_command_gpu );
      debug_draw_command->draw_indirect_2dline.instanceCount = 1u;
      debug_draw_command->draw_indirect_3dline.instanceCount = 1u;
      debug_draw_command->draw_indirect_cube.vertexCount = 36u;
      crude_gfx_unmap_buffer( gpu, scene_renderer->debug_commands_sb[ gpu->current_frame ] );
    }
  }
  
  /* Update lights buffers */
  {
    crude_camera const                                    *camera;
    crude_transform const                                 *camera_transform;
    crude_gfx_light_gpu                                   *lights_gpu;
    crude_gfx_sorted_light                                *sorted_lights;
    uint32                                                *lights_luts;
    uint32                                                *bin_range_per_light;
    XMMATRIX                                               view_to_world, world_to_view, view_to_clip, clip_to_view, world_to_clip;
    float32                                                zfar, znear, bin_size;
    uint32                                                 temporary_allocator_marker;

    temporary_allocator_marker = crude_stack_allocator_get_marker( scene_renderer->temporary_allocator );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( sorted_lights, CRUDE_ARRAY_LENGTH( scene_renderer->lights ), crude_stack_allocator_pack( scene_renderer->temporary_allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( bin_range_per_light, CRUDE_ARRAY_LENGTH( scene_renderer->lights ), crude_stack_allocator_pack( scene_renderer->temporary_allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( lights_luts, CRUDE_GFX_LIGHT_Z_BINS, crude_stack_allocator_pack( scene_renderer->temporary_allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( lights_gpu, CRUDE_ARRAY_LENGTH( scene_renderer->lights ), crude_stack_allocator_pack( scene_renderer->temporary_allocator ) );

    camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( scene_renderer->options.camera_node, crude_camera );
    camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( scene_renderer->options.camera_node, crude_transform );
    
    view_to_world = crude_transform_node_to_world( scene_renderer->options.camera_node, camera_transform );
    world_to_view = XMMatrixInverse( NULL, view_to_world );
    view_to_clip = crude_camera_view_to_clip( camera );
    clip_to_view = XMMatrixInverse( NULL, view_to_clip );
    world_to_clip = XMMatrixMultiply( world_to_view, view_to_clip );

    /* Sort lights based on Z */
    zfar = camera->far_z;
    znear = camera->near_z;
    
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->lights ); ++i )
    {
      crude_light const                                   *light;
      crude_transform const                               *light_transform;

      light = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( scene_renderer->lights[ i ].node, crude_light );
      light_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( scene_renderer->lights[ i ].node, crude_transform );
      
      lights_gpu[ i ] = CRUDE_COMPOUNT_EMPTY( crude_gfx_light_gpu );
      lights_gpu[ i ].color = light->color;
      lights_gpu[ i ].intensity = light->intensity;
      lights_gpu[ i ].position = light_transform->translation;
      lights_gpu[ i ].radius = light->radius;
    }

    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->lights ); ++i )
    {
      crude_gfx_sorted_light                              *sorted_light;
      crude_gfx_light_gpu                                 *light_gpu;
      XMVECTOR                                             world_pos, view_pos, view_pos_min, view_pos_max;
    
      light_gpu = &lights_gpu[ i ];
    
      world_pos = XMVectorSet( light_gpu->position.x, light_gpu->position.y, light_gpu->position.z, 1.0f );
    
      view_pos = XMVector4Transform( world_pos, world_to_view );
      view_pos_min = XMVectorAdd( view_pos, XMVectorSet( 0, 0, -light_gpu->radius, 0 ) );
      view_pos_max = XMVectorAdd( view_pos, XMVectorSet( 0, 0, light_gpu->radius, 0 ) );
    
      sorted_light = &sorted_lights[ i ];
      sorted_light->light_index = i;
      sorted_light->projected_z = ( ( XMVectorGetZ( view_pos ) - znear ) / ( zfar - znear ) );
      sorted_light->projected_z_min = ( ( XMVectorGetZ( view_pos_min ) - znear ) / ( zfar - znear ) );
      sorted_light->projected_z_max = ( ( XMVectorGetZ( view_pos_max ) - znear ) / ( zfar - znear ) );
    }
    
    qsort( sorted_lights, CRUDE_ARRAY_LENGTH( scene_renderer->lights ), sizeof( crude_gfx_sorted_light ), sorting_light_ );

    /* Upload light to gpu */
    {
      crude_gfx_light_gpu                                 *lights_gpu_mapped;

      buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
      buffer_map.buffer = scene_renderer->lights_sb;
      buffer_map.offset = 0;
      buffer_map.size = sizeof( crude_gfx_light_gpu ) * CRUDE_ARRAY_LENGTH( scene_renderer->lights );
      lights_gpu_mapped = CRUDE_CAST( crude_gfx_light_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
      if ( lights_gpu_mapped )
      {
        memcpy( lights_gpu_mapped, lights_gpu, sizeof( crude_gfx_light_gpu ) * CRUDE_ARRAY_LENGTH( scene_renderer->lights ) );
        crude_gfx_unmap_buffer( gpu, scene_renderer->lights_sb );
      }
    }
    
    /* Calculate lights clusters */
    bin_size = 1.f / CRUDE_GFX_LIGHT_Z_BINS;
    
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->lights ); ++i )
    {
      crude_gfx_sorted_light const                        *light;
      uint32                                               min_bin, max_bin;
  
      light = &sorted_lights[ i ];
  
      if ( light->projected_z_min < 0.f && light->projected_z_max < 0.f )
      {
        bin_range_per_light[ i ] = UINT32_MAX;
        continue;
      }
      min_bin = CRUDE_MAX( 0u, CRUDE_FLOOR( light->projected_z_min * CRUDE_GFX_LIGHT_Z_BINS ) );
      max_bin = CRUDE_MAX( 0u, CRUDE_CEIL( light->projected_z_max * CRUDE_GFX_LIGHT_Z_BINS ) );
      bin_range_per_light[ i ] = ( min_bin & 0xffff ) | ( ( max_bin & 0xffff ) << 16 );
    }
  
    for ( uint32 bin = 0; bin < CRUDE_GFX_LIGHT_Z_BINS; ++bin )
    {
      float32                                              bin_min, bin_max;
      uint32                                               min_light_id, max_light_id;
  
      min_light_id = CRUDE_GFX_LIGHTS_MAX_COUNT + 1;
      max_light_id = 0;
  
      bin_min = bin_size * bin;
      bin_max = bin_min + bin_size;
  
      for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->lights ); ++i )
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

      buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
      buffer_map.buffer = scene_renderer->lights_indices_sb[ gpu->current_frame ];
      buffer_map.offset = 0;
      buffer_map.size = sizeof( uint32 );
      lights_indices_mapped = CRUDE_CAST( uint32*, crude_gfx_map_buffer( gpu, &buffer_map ) );
      if ( lights_indices_mapped )
      {
        for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->lights ); ++i )
        {
          lights_indices_mapped[ i ] = sorted_lights[ i ].light_index;
        }
        crude_gfx_unmap_buffer( gpu, scene_renderer->lights_indices_sb[ gpu->current_frame ] );
      }
    }

    /* Upload lights LUT */
    {
      uint32                                              *lights_luts_mapped;
      buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
      buffer_map.buffer = scene_renderer->lights_bins_sb[ gpu->current_frame ];
      buffer_map.offset = 0;
      buffer_map.size = sizeof( uint32 ) * CRUDE_GFX_LIGHT_Z_BINS;
      lights_luts_mapped = CRUDE_CAST( uint32*, crude_gfx_map_buffer( gpu, &buffer_map ) );
      if ( lights_luts_mapped )
      {
        memcpy( lights_luts_mapped, lights_luts, CRUDE_ARRAY_LENGTH( lights_luts ) * sizeof( uint32 ) );
        crude_gfx_unmap_buffer( gpu, scene_renderer->lights_bins_sb[ gpu->current_frame ] );
      }
    }
  
    {
      uint32                                              *light_tiles_bits;
      float32                                              tile_size_inv;
      uint32                                               tile_x_count, tile_y_count, tiles_entry_count, buffer_size, tile_stride;

      tile_x_count = scene_renderer->gpu->vk_swapchain_width / CRUDE_GFX_LIGHT_TILE_SIZE;
      tile_y_count = scene_renderer->gpu->vk_swapchain_height / CRUDE_GFX_LIGHT_TILE_SIZE;
      tiles_entry_count = tile_x_count * tile_y_count * CRUDE_GFX_LIGHT_WORDS_COUNT;
      buffer_size = tiles_entry_count * sizeof( uint32 );
  
      CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( light_tiles_bits, tiles_entry_count, crude_stack_allocator_pack( scene_renderer->temporary_allocator ) );
      memset( light_tiles_bits, 0, buffer_size );

      znear = camera->near_z;
      tile_size_inv = 1.0f / CRUDE_GFX_LIGHT_TILE_SIZE;
      tile_stride = tile_x_count * CRUDE_GFX_LIGHT_WORDS_COUNT;
  
      for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->lights ); ++i )
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
        camera_visible = -XMVectorGetZ( light_view_position ) - light_radius < znear;
  
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
            corner_vs = XMVectorSetZ( corner_vs, CRUDE_MAX( znear, XMVectorGetZ( corner_vs ) ) );
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
          camera_inside = ( light_view_position_length - light_radius ) < znear;
  
          if ( camera_inside )
          {
            aabb = { -1,-1, 1, 1 };
          }
        }
  
        {
          XMVECTOR                                         aabb_screen;

          aabb_screen = XMVectorSet(
            ( XMVectorGetX( aabb ) * 0.5f + 0.5f ) * ( gpu->vk_swapchain_width - 1 ),
            ( XMVectorGetY( aabb ) * 0.5f + 0.5f ) * ( gpu->vk_swapchain_height - 1 ),
            ( XMVectorGetZ( aabb ) * 0.5f + 0.5f ) * ( gpu->vk_swapchain_width - 1 ),
            ( XMVectorGetW( aabb ) * 0.5f + 0.5f ) * ( gpu->vk_swapchain_height - 1 )
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

        if ( aabb_screen_min_x > gpu->vk_swapchain_width || aabb_screen_min_y > gpu->vk_swapchain_height )
        {
          continue;
        }
  
        if ( aabb_screen_max_x < 0.0f || aabb_screen_max_y < 0.0f )
        {
          continue;
        }
  
        aabb_screen_min_x = CRUDE_MAX( aabb_screen_min_x, 0.0f );
        aabb_screen_min_y = CRUDE_MAX( aabb_screen_min_y, 0.0f );
  
        aabb_screen_max_x = CRUDE_MIN( aabb_screen_max_x, gpu->vk_swapchain_width );
        aabb_screen_max_y = CRUDE_MIN( aabb_screen_max_y, gpu->vk_swapchain_height );
  
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

              array_index = y * tile_stride + x * CRUDE_GFX_LIGHT_WORDS_COUNT;

              word_index = i / 32;
              bit_index = i % 32;
  
              light_tiles_bits[ array_index + word_index ] |= ( 1 << bit_index );
            }
          }
        }
      }
      
      {
        uint32                                              *light_tiles_mapped;

        buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
        buffer_map.buffer = scene_renderer->lights_tiles_sb[ gpu->current_frame ];
        buffer_map.offset = 0;
        buffer_map.size = sizeof( uint32 );
        light_tiles_mapped = CRUDE_CAST( uint32*, crude_gfx_map_buffer( gpu, &buffer_map ) );
        if ( light_tiles_mapped )
        {
          memcpy( light_tiles_mapped, light_tiles_bits, CRUDE_ARRAY_LENGTH( light_tiles_bits ) * sizeof( uint32 ) );
          crude_gfx_unmap_buffer( gpu, scene_renderer->lights_tiles_sb[ gpu->current_frame ] );
        }
      }
    }
    
    crude_stack_allocator_free_marker( scene_renderer->temporary_allocator, temporary_allocator_marker );
  }
}

/**
 * Scene Renderer Utils
 */
int
sorting_light_
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