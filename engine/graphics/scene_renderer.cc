#include <TaskScheduler_c.h>
#include <cgltf.h>
#include <stb_image.h>

#if defined(__cplusplus)
#include <meshoptimizer.h>
#else
#error "meh"
#endif

#include <core/profiler.h>
#include <core/array.h>
#include <core/file.h>
#include <core/hash_map.h>

#include <scene/scene_components.h>
#include <scene/scene.h>

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
 * Scene Renderer Register Nodes & Gltf & Lights
 */
static void
register_nodes_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_entity                                        node,
  _In_ crude_stack_allocator                              *temporary_allocator
);

static void
register_gltf_
(
  _In_ crude_gfx_scene_renderer                          *scene_renderer,
  _In_ char const                                        *gltf_path,
  _In_ crude_entity                                       node,
  _In_ crude_stack_allocator                             *temporary_allocator
);

static bool
create_mesh_material_
(
  _In_ cgltf_data                                         *gltf,
  _In_ crude_entity                                        node,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_material                                     *material,
  _In_ crude_gfx_mesh_cpu                                 *mesh_draw,
  _In_ size_t                                             scene_renderer_images_offset,
  _In_ size_t                                             scene_renderer_samplers_offset
);

static cgltf_data*
parse_gltf_
(
  _In_ crude_stack_allocator                              *temporary_allocator,
  _In_ char const                                         *gltf_path
);

static void
load_images_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
);

static void
load_samplers_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
);

static void
load_buffers_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
);

static void
load_meshes_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_entity                                        node,
  _In_ cgltf_data                                         *gltf,
  _In_ uint32                                             *gltf_mesh_index_to_mesh_primitive_index,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory,
  _In_ uint32                                              scene_renderer_buffers_offset,
  _In_ uint32                                              scene_renderer_images_offset,
  _In_ uint32                                              scene_renderer_samplers_offset
);

static void
load_meshlets_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_stack_allocator                              *temporary_allocator
);

static void
load_nodes_
(
  _In_ cgltf_data                                         *gltf,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_entity                                        parent_node,
  _In_ cgltf_node                                        **gltf_nodes,
  _In_ uint32                                              gltf_nodes_count,
  _In_ uint32                                             *gltf_mesh_index_to_mesh_primitive_index,
  _In_ crude_stack_allocator                              *temporary_allocator
);

static void
load_meshlet_vertices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ crude_gfx_meshlet_vertex_gpu                      **vertices
);

static void
load_meshlet_indices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ uint32                                            **indices,
  _In_ uint32                                              vertices_offset
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
  scene_renderer->temporary_allocator = creation->temporary_allocator;
  scene_renderer->renderer = creation->renderer;
  scene_renderer->async_loader = creation->async_loader;
  scene_renderer->task_scheduler = creation->task_scheduler;
  scene_renderer->imgui_context = creation->imgui_context;
  
  /* Common resources arrays initialization */
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->images, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->samplers, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->buffers, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );

  /* Common mesh & meshlets arrays initialization */
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshes, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshlets_vertices, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshlets_triangles_indices, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshlets_vertices_indices, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshlets, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->meshes_instances, 0u, scene_renderer->renderer->allocator_container );
  
  /* Common lights arrays initialization */
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->lights, 0u, scene_renderer->renderer->allocator_container );
  
  /* Register scene nodes */
  register_nodes_( scene_renderer, scene_renderer->scene->main_node, scene_renderer->temporary_allocator );
  
  /* Common gpu data */
  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  buffer_creation.size = sizeof( crude_gfx_scene_constant_gpu );
  buffer_creation.name = "frame_buffer";
  scene_renderer->scene_cb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

  /* Common debug gpu data */
  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( crude_gfx_debug_draw_command_gpu );
    buffer_creation.name = "debug_line_commands";
    scene_renderer->debug_line_commands_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof( crude_gfx_debug_line_vertex_gpu ) * CRUDE_GFX_MAX_DEBUG_LINES * 2u; /* 2 vertices per line */
    buffer_creation.name = "debug_line_vertices";
    scene_renderer->debug_line_vertices_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
  }

  /* Common lights gpu data */
  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  buffer_creation.size = sizeof( crude_gfx_light_gpu ) * CRUDE_ARRAY_LENGTH( scene_renderer->lights );
  buffer_creation.name = "lights_sb";
  scene_renderer->lights_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
  
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
    scene_renderer->pointlight_world_to_clip_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( uint32 ) * CRUDE_ARRAY_LENGTH( scene_renderer->lights );
    buffer_creation.name = "lights_indices_sb";
    scene_renderer->lights_indices_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( uint32 ) * CRUDE_GFX_LIGHT_Z_BINS;
    buffer_creation.name = "lights_bins_sb";
    scene_renderer->lights_bins_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
  }

  /* Common mesh & meshlets gpu data */
  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  buffer_creation.size = sizeof( crude_gfx_mesh_instance_draw_gpu ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances );
  buffer_creation.name = "meshes_instances_draws_sb";
  scene_renderer->meshes_instances_draws_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
  
  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = sizeof*( scene_renderer->meshlets ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshlets );
  buffer_creation.initial_data = scene_renderer->meshlets;
  buffer_creation.name = "meshlet_sb";
  scene_renderer->meshlets_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
  
  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = sizeof*( scene_renderer->meshlets_vertices ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices );
  buffer_creation.initial_data = scene_renderer->meshlets_vertices;
  buffer_creation.name = "meshlets_vertices_sb";
  scene_renderer->meshlets_vertices_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = sizeof*( scene_renderer->meshlets_vertices_indices ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices_indices );
  buffer_creation.initial_data = scene_renderer->meshlets_vertices_indices;
  buffer_creation.name = "meshlets_vertices_indices_sb";
  scene_renderer->meshlets_vertices_indices_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = sizeof*( scene_renderer->meshlets_triangles_indices ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_triangles_indices );
  buffer_creation.initial_data = scene_renderer->meshlets_triangles_indices;
  buffer_creation.name = "meshlets_primitives_indices_sb";
  scene_renderer->meshlets_triangles_indices_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances ) * sizeof( crude_gfx_mesh_draw_command_gpu );
    buffer_creation.name = "draw_commands_early_sb";
    scene_renderer->mesh_task_indirect_commands_early_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances ) * sizeof( crude_gfx_mesh_draw_command_gpu );
    buffer_creation.name = "draw_commands_late_sb";
    scene_renderer->mesh_task_indirect_commands_late_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( crude_gfx_mesh_draw_counts_gpu );
    buffer_creation.name = "mesh_count_early_sb";
    scene_renderer->mesh_task_indirect_count_early_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( crude_gfx_mesh_draw_counts_gpu );
    buffer_creation.name = "mesh_count_late_sb";
    scene_renderer->mesh_task_indirect_count_late_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
  }

  {
    uint32                                                 temporary_allocator_marker;
    crude_gfx_mesh_draw_gpu                               *meshes_draws;
    XMFLOAT4                                              *meshes_bounds;
    
    temporary_allocator_marker = crude_stack_allocator_get_marker( scene_renderer->temporary_allocator );

    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( meshes_draws, CRUDE_ARRAY_LENGTH( scene_renderer->meshes ), crude_stack_allocator_pack( scene_renderer->temporary_allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( meshes_bounds, CRUDE_ARRAY_LENGTH( scene_renderer->meshes ), crude_stack_allocator_pack( scene_renderer->temporary_allocator ) );

    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->meshes ); ++i )
    {
      crude_gfx_mesh_cpu_to_mesh_draw_gpu( &scene_renderer->meshes[ i ], &meshes_draws[ i ] );
      meshes_bounds[ i ] = scene_renderer->meshes[ i ].bounding_sphere;
    }

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof( crude_gfx_mesh_darw_gpu ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshes );
    buffer_creation.name = "meshes_draws_sb";
    buffer_creation.initial_data = meshes_draws;
    scene_renderer->meshes_draws_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = sizeof( XMFLOAT4 ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshes );
    buffer_creation.name = "meshes_bounds_sb";
    buffer_creation.initial_data = meshes_bounds;
    scene_renderer->meshes_bounds_sb = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    crude_stack_allocator_free_marker( scene_renderer->temporary_allocator, temporary_allocator_marker );
  }

  crude_gfx_scene_renderer_on_resize( scene_renderer );

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
}

void
crude_gfx_scene_renderer_deinitialize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
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
  
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshes_instances );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->images ); ++i )
  {
    crude_gfx_renderer_destroy_texture( scene_renderer->renderer, &scene_renderer->images[ i ] );
  }
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->images );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->samplers ); ++i )
  {
    crude_gfx_renderer_destroy_sampler( scene_renderer->renderer, &scene_renderer->samplers[ i ] );
  }
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->samplers );
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->buffers ); ++i )
  {
    crude_gfx_renderer_destroy_buffer( scene_renderer->renderer, &scene_renderer->buffers[ i ] );
  }
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->buffers );
  
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->lights_sb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->scene_cb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshlets_sb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshlets_vertices_sb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshlets_vertices_indices_sb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshlets_triangles_indices_sb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshes_draws_sb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshes_bounds_sb );
  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshes_instances_draws_sb );

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->pointlight_world_to_clip_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->lights_indices_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->lights_bins_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->lights_tiles_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->debug_line_vertices_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->debug_line_commands_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->mesh_task_indirect_commands_early_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->mesh_task_indirect_count_early_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->mesh_task_indirect_commands_late_sb[ i ] );
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->mesh_task_indirect_count_late_sb[ i ] );
  }
  
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->lights );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshes );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->buffers );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshes_instances );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshlets_vertices );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshlets_triangles_indices );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshlets_vertices_indices );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshlets );
}

void
crude_gfx_scene_renderer_submit_draw_task
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ bool                                                use_secondary
)
{
  crude_gfx_cmd_buffer                                    *primary_cmd;
 
  primary_cmd = crude_gfx_get_primary_cmd( scene_renderer->renderer->gpu, 0, true );
  
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
      crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->lights_tiles_sb[ i ] );
    }
  }

  for ( uint32 i = 0; i < CRUDE_GFX_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    uint32 tile_x_count = scene_renderer->renderer->gpu->vk_swapchain_width / CRUDE_GFX_LIGHT_TILE_SIZE;
    uint32 tile_y_count = scene_renderer->renderer->gpu->vk_swapchain_height / CRUDE_GFX_LIGHT_TILE_SIZE;
    uint32 tiles_entry_count = tile_x_count * tile_y_count * CRUDE_GFX_LIGHT_WORDS_COUNT;
    buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
    buffer_creation.size = sizeof( uint32 ) * tiles_entry_count;
    buffer_creation.name = "lights_tiles_sb";
    scene_renderer->lights_tiles_sb[ i ] = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
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
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->debug_line_vertices_sb[ frame ], 50u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->debug_line_commands_sb[ frame ], 51u );
}

void
crude_gfx_scene_renderer_add_meshlet_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
)
{
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshlets_sb, 5u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshlets_vertices_sb, 6u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshlets_triangles_indices_sb, 7u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshlets_vertices_indices_sb, 8u );
}

void
crude_gfx_scene_renderer_add_scene_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
)
{
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->scene_cb, 0u );
}

void
crude_gfx_scene_renderer_add_mesh_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
)
{
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshes_draws_sb, 1u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshes_instances_draws_sb, 2u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->meshes_bounds_sb, 3u );
}

void
crude_gfx_scene_renderer_add_light_resources_to_descriptor_set_creation
(
  _In_ crude_gfx_descriptor_set_creation                  *creation,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ uint32                                              frame
)
{
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->lights_bins_sb[ frame ], 20u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->lights_sb, 21u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->lights_tiles_sb[ frame ], 22u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->lights_indices_sb[ frame ], 23u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->lights_indices_sb[ frame ], 23u );
  crude_gfx_descriptor_set_creation_add_buffer( creation, scene_renderer->pointlight_world_to_clip_sb[ frame ], 24u );
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

  gpu = scene_renderer->renderer->gpu;

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
      scene_constant->camera_debug_previous = scene_constant->camera_debug;
      scene_constant->resolution.x = scene_renderer->renderer->gpu->vk_swapchain_width;
      scene_constant->resolution.y = scene_renderer->renderer->gpu->vk_swapchain_height;
      crude_gfx_camera_to_camera_gpu( scene_renderer->scene->main_camera, &scene_constant->camera );
      crude_gfx_camera_to_camera_gpu( scene_renderer->scene->debug_camera, &scene_constant->camera_debug );
      scene_constant->mesh_instances_count = CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances );
      scene_constant->active_lights_count = CRUDE_ARRAY_LENGTH( scene_renderer->lights );
      scene_constant->tiled_shadowmap_texture_index = scene_renderer->pointlight_shadow_pass.tetrahedron_shadow_texture.index;
      scene_constant->inv_shadow_map_size.x = 1.f / CRUDE_GFX_TETRAHEDRON_SHADOWMAP_WIDTH;
      scene_constant->inv_shadow_map_size.y = 1.f / CRUDE_GFX_TETRAHEDRON_SHADOWMAP_HEIGHT;
      crude_gfx_unmap_buffer( gpu, scene_renderer->scene_cb );
    }
  }

  /* Update meshes instanse draws buffers*/
  {
    crude_gfx_mesh_instance_draw_gpu                      *meshes_instances_draws;
  
    buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
    buffer_map.buffer = scene_renderer->meshes_instances_draws_sb;
    buffer_map.offset = 0;
    buffer_map.size = sizeof( crude_gfx_mesh_instance_draw_gpu ) * CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances );
    meshes_instances_draws = CRUDE_CAST( crude_gfx_mesh_instance_draw_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
  
    if ( meshes_instances_draws )
    {
      scene_renderer->total_meshes_instances_count = 0u;
      for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances ); ++i )
      {
        crude_gfx_mesh_cpu *mesh_cpu = scene_renderer->meshes_instances[ i ].mesh;
        
        bool mesh_textures_ready = ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->albedo_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->albedo_texture_handle ) )
          && ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->normal_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->normal_texture_handle ) )
          && ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->occlusion_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->occlusion_texture_handle ) )
          && ( CRUDE_RESOURCE_HANDLE_IS_INVALID( mesh_cpu->roughness_texture_handle ) || crude_gfx_texture_ready( gpu, mesh_cpu->roughness_texture_handle ) );

        if ( mesh_textures_ready )
        {
          crude_transform const                             *transform;
          XMMATRIX                                           model_to_world;

          transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( scene_renderer->meshes_instances[ i ].node, crude_transform );
          model_to_world = crude_transform_node_to_world( scene_renderer->meshes_instances[ i ].node, transform );

          XMStoreFloat4x4( &meshes_instances_draws[ scene_renderer->total_meshes_instances_count ].model_to_world, model_to_world );
          meshes_instances_draws[ scene_renderer->total_meshes_instances_count ].mesh_draw_index = scene_renderer->meshes_instances[ i ].mesh->gpu_mesh_index;

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
    buffer_map.buffer = scene_renderer->debug_line_commands_sb[ gpu->current_frame ];
    buffer_map.offset = 0;
    buffer_map.size = sizeof( crude_gfx_debug_draw_command_gpu );
    debug_draw_command = CRUDE_CAST( crude_gfx_debug_draw_command_gpu*, crude_gfx_map_buffer( gpu, &buffer_map ) );
    if ( debug_draw_command )
    {
      *debug_draw_command = CRUDE_COMPOUNT_EMPTY( crude_gfx_debug_draw_command_gpu );
      debug_draw_command->draw_indirect_2dline.instanceCount = 1u;
      debug_draw_command->draw_indirect_3dline.instanceCount = 1u;
      crude_gfx_unmap_buffer( gpu, scene_renderer->debug_line_commands_sb[ gpu->current_frame ] );
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

    camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( scene_renderer->scene->main_camera, crude_camera );
    camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( scene_renderer->scene->main_camera, crude_transform );
    
    view_to_world = crude_transform_node_to_world( scene_renderer->scene->main_camera, camera_transform );
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

      tile_x_count = scene_renderer->renderer->gpu->vk_swapchain_width / CRUDE_GFX_LIGHT_TILE_SIZE;
      tile_y_count = scene_renderer->renderer->gpu->vk_swapchain_height / CRUDE_GFX_LIGHT_TILE_SIZE;
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
 * Register Nodes & Gltf & Lights
 */
void
register_nodes_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_entity                                        node,
  _In_ crude_stack_allocator                              *temporary_allocator
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
        crude_gltf const* child_gltf = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( child, crude_gltf );
        register_gltf_( scene_renderer, child_gltf->path, child, temporary_allocator );
      }
      if ( CRUDE_ENTITY_HAS_COMPONENT( child, crude_light ) )
      {
        crude_gfx_light_cpu light_gpu = CRUDE_COMPOUNT_EMPTY( crude_gfx_light_cpu );
        light_gpu.node = child;
        CRUDE_ARRAY_PUSH( scene_renderer->lights, light_gpu );
      }

      register_nodes_( scene_renderer, child, temporary_allocator );
    }
  }
}

void
register_gltf_
(
  _In_ crude_gfx_scene_renderer                          *scene_renderer,
  _In_ char const                                        *gltf_path,
  _In_ crude_entity                                       node,
  _In_ crude_stack_allocator                             *temporary_allocator
)
{
  cgltf_data                                              *gltf;
  uint32                                                  *gltf_mesh_index_to_mesh_primitive_index;
  crude_string_buffer                                      temporary_string_buffer;
  char                                                     gltf_directory[ 512 ];
  uint32                                                   temporary_allocator_mark;
  size_t                                                   scene_renderer_images_offset;
  size_t                                                   scene_renderer_samplers_offset;
  size_t                                                   scene_renderer_buffers_offset;

  temporary_allocator_mark = crude_stack_allocator_get_marker( temporary_allocator );
  
  /* Parse gltf */
  gltf = parse_gltf_( temporary_allocator, gltf_path );
  if ( !gltf )
  {
    crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_mark );
    return;
  }

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( gltf_mesh_index_to_mesh_primitive_index, 0, crude_stack_allocator_pack( temporary_allocator ) );
  
  /* Initialize tmp */
  crude_string_buffer_initialize( &temporary_string_buffer, 1024, crude_stack_allocator_pack( temporary_allocator ) );
  
  /* Get gltf directory */
  crude_memory_copy( gltf_directory, gltf_path, sizeof( gltf_directory ) );
  crude_file_directory_from_path( gltf_directory );
  
  /* Get scene renderer offsets */
  scene_renderer_images_offset = CRUDE_ARRAY_LENGTH( scene_renderer->images );
  scene_renderer_samplers_offset = CRUDE_ARRAY_LENGTH( scene_renderer->samplers );
  scene_renderer_buffers_offset = CRUDE_ARRAY_LENGTH( scene_renderer->buffers );

  /* Load gltf resources */
  load_images_( scene_renderer, gltf, &temporary_string_buffer, gltf_directory );
  load_samplers_( scene_renderer, gltf, &temporary_string_buffer, gltf_directory );
  load_buffers_( scene_renderer, gltf, &temporary_string_buffer, gltf_directory );
  load_meshes_( scene_renderer, node, gltf, gltf_mesh_index_to_mesh_primitive_index, &temporary_string_buffer, gltf_directory, scene_renderer_buffers_offset, scene_renderer_images_offset, scene_renderer_samplers_offset );
  load_meshlets_( scene_renderer, gltf, temporary_allocator );

  for ( uint32 i = 0; i < gltf->scenes_count; ++i )
  {
    load_nodes_( gltf, scene_renderer, node, gltf->scene[ i ].nodes, gltf->scene[ i ].nodes_count, gltf_mesh_index_to_mesh_primitive_index, temporary_allocator );
  }
  
  cgltf_free( gltf );
  crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_mark );
}


/************************************************
 *
 * GLTF Utils Functinos Implementation
 * 
 ***********************************************/
bool
create_mesh_material_
(
  _In_ cgltf_data                                         *gltf,
  _In_ crude_entity                                        node,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_material                                     *material,
  _In_ crude_gfx_mesh_cpu                                 *mesh_draw,
  _In_ size_t                                             scene_renderer_images_offset,
  _In_ size_t                                             scene_renderer_samplers_offset
)
{
  bool transparent = false;
  
  mesh_draw->flags = 0;

  mesh_draw->albedo_color_factor = CRUDE_COMPOUNT( XMFLOAT4, {
    material->pbr_metallic_roughness.base_color_factor[ 0 ],
    material->pbr_metallic_roughness.base_color_factor[ 1 ],
    material->pbr_metallic_roughness.base_color_factor[ 2 ],
    material->pbr_metallic_roughness.base_color_factor[ 3 ],
  } );
  
  mesh_draw->metallic_roughness_occlusion_factor.x = material->pbr_metallic_roughness.roughness_factor;
  mesh_draw->metallic_roughness_occlusion_factor.y = material->pbr_metallic_roughness.metallic_factor;
  mesh_draw->alpha_cutoff = material->alpha_cutoff;
  
  if (material->alpha_mode == cgltf_alpha_mode_mask )
  {
    mesh_draw->flags |= CRUDE_GFX_DRAW_FLAGS_ALPHA_MASK;
    transparent = true;
  }

  if ( material->pbr_metallic_roughness.base_color_texture.texture )
  {
    cgltf_texture *albedo_texture = material->pbr_metallic_roughness.base_color_texture.texture;
    crude_gfx_renderer_texture *albedo_texture_gpu = &scene_renderer->images[ scene_renderer_images_offset + cgltf_image_index( gltf, albedo_texture->image ) ];
    crude_gfx_renderer_sampler *albedo_sampler_gpu = &scene_renderer->samplers[ scene_renderer_samplers_offset + cgltf_sampler_index( gltf, albedo_texture->sampler ) ];
  
    mesh_draw->albedo_texture_handle = albedo_texture_gpu->handle;
    crude_gfx_link_texture_sampler( renderer->gpu, albedo_texture_gpu->handle, albedo_sampler_gpu->handle );
  }
  else
  {
    mesh_draw->albedo_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  }
  
  if ( material->pbr_metallic_roughness.metallic_roughness_texture.texture )
  {
    cgltf_texture *roughness_texture = material->pbr_metallic_roughness.metallic_roughness_texture.texture;
    crude_gfx_renderer_texture *roughness_texture_gpu = &scene_renderer->images[ scene_renderer_images_offset + cgltf_image_index( gltf, roughness_texture->image ) ];
    crude_gfx_renderer_sampler *roughness_sampler_gpu = &scene_renderer->samplers[ scene_renderer_samplers_offset + cgltf_sampler_index( gltf, roughness_texture->sampler ) ];
    
    mesh_draw->roughness_texture_handle = roughness_texture_gpu->handle;
    crude_gfx_link_texture_sampler( renderer->gpu, roughness_texture_gpu->handle, roughness_sampler_gpu->handle );
  }
  else
  {
    mesh_draw->roughness_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  }

  if ( material->occlusion_texture.texture )
  {
    cgltf_texture *occlusion_texture = material->occlusion_texture.texture;
    crude_gfx_renderer_texture *occlusion_texture_gpu = &scene_renderer->images[ scene_renderer_images_offset + cgltf_image_index( gltf, occlusion_texture->image ) ];
    crude_gfx_renderer_sampler *occlusion_sampler_gpu = &scene_renderer->samplers[ scene_renderer_samplers_offset + cgltf_sampler_index( gltf, occlusion_texture->sampler ) ];
    
    mesh_draw->occlusion_texture_handle = occlusion_texture_gpu->handle;
    mesh_draw->metallic_roughness_occlusion_factor.z = material->occlusion_texture.scale;
    
    crude_gfx_link_texture_sampler( renderer->gpu, occlusion_texture_gpu->handle, occlusion_sampler_gpu->handle );
  }
  else
  {
    mesh_draw->occlusion_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  }
  
  if ( material->normal_texture.texture )
  {
    cgltf_texture *normal_texture = material->normal_texture.texture;
    crude_gfx_renderer_texture *normal_texture_gpu = &scene_renderer->images[ scene_renderer_images_offset + cgltf_image_index( gltf, normal_texture->image ) ];
    crude_gfx_renderer_sampler *normal_sampler_gpu = &scene_renderer->samplers[ scene_renderer_samplers_offset + cgltf_sampler_index( gltf, normal_texture->sampler ) ];
    
    mesh_draw->normal_texture_handle = normal_texture_gpu->handle;
    crude_gfx_link_texture_sampler( renderer->gpu, normal_texture_gpu->handle, normal_sampler_gpu->handle );
  }
  else
  {
    mesh_draw->normal_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  }
  
  return transparent;
}

cgltf_data*
parse_gltf_
(
  _In_ crude_stack_allocator                              *temporary_allocator,
  _In_ char const                                         *gltf_path
)
{
  cgltf_data                                              *gltf;
  cgltf_result                                             result;
  cgltf_options                                            gltf_options;
  crude_allocator_container                                temporary_allocator_container;

  temporary_allocator_container = crude_stack_allocator_pack( temporary_allocator );

  gltf_options = CRUDE_COMPOUNT( cgltf_options, { 
    .memory = {
      .alloc_func = temporary_allocator_container.allocate,
      .free_func  = temporary_allocator_container.deallocate,
      .user_data = temporary_allocator_container.ctx
    },
  } );

  gltf = NULL;
  result = cgltf_parse_file( &gltf_options, gltf_path, &gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to parse gltf file: %s", gltf_path );
    return NULL;
  }

  result = cgltf_load_buffers( &gltf_options, gltf, gltf_path );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to load buffers from gltf file: %s", gltf_path );
    return NULL;
  }

  result = cgltf_validate( gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to validate gltf file: %s", gltf_path );
    return NULL;
  }

  return gltf;
}

static void
load_images_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
)
{
  for ( uint32 image_index = 0; image_index < gltf->images_count; ++image_index )
  {
    crude_gfx_renderer_texture                            *texture_resource;
    crude_gfx_texture_creation                             texture_creation;
    cgltf_image const                                     *image;
    char                                                  *image_full_filename;
    int                                                    comp, width, height;
    
    image = &gltf->images[ image_index ];
    image_full_filename = crude_string_buffer_append_use_f( temporary_string_buffer, "%s%s", gltf_directory, image->uri );
    stbi_info( image_full_filename, &width, &height, &comp );

    texture_creation = crude_gfx_texture_creation_empty();
    texture_creation.initial_data = NULL;
    texture_creation.width = width;
    texture_creation.height = height;
    texture_creation.depth = 1u;
    texture_creation.flags = 0u;
    texture_creation.format = VK_FORMAT_R8G8B8A8_UNORM;
    texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
    texture_creation.name = image_full_filename;

    texture_resource = crude_gfx_renderer_create_texture( scene_renderer->renderer, &texture_creation );
    CRUDE_ARRAY_PUSH( scene_renderer->images, *texture_resource );
    crude_gfx_asynchronous_loader_request_texture_data( scene_renderer->async_loader, image_full_filename, texture_resource->handle );
    crude_string_buffer_clear( temporary_string_buffer );
  }
}

void
load_samplers_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
)
{
  for ( uint32 sampler_index = 0; sampler_index < gltf->samplers_count; ++sampler_index )
  {
    crude_gfx_renderer_sampler                            *sampler_resource;
    crude_gfx_sampler_creation                             creation;
    cgltf_sampler                                         *sampler;

    sampler = &gltf->samplers[ sampler_index ];

    creation = crude_gfx_sampler_creation_empty();
    switch ( sampler->min_filter )
    {
    case cgltf_filter_type_nearest:
      creation.min_filter = VK_FILTER_NEAREST;
      break;
    case cgltf_filter_type_linear:
      creation.min_filter = VK_FILTER_LINEAR;
      break;
    case cgltf_filter_type_linear_mipmap_nearest:
      creation.min_filter = VK_FILTER_LINEAR;
      creation.mip_filter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
      break;
    case cgltf_filter_type_linear_mipmap_linear:
      creation.min_filter = VK_FILTER_LINEAR;
      creation.mip_filter = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      break;
    case cgltf_filter_type_nearest_mipmap_nearest:
      creation.min_filter = VK_FILTER_NEAREST;
      creation.mip_filter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
      break;
    case cgltf_filter_type_nearest_mipmap_linear:
      creation.min_filter = VK_FILTER_NEAREST;
      creation.mip_filter = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      break;
    }
    
    creation.mag_filter = sampler->mag_filter == cgltf_filter_type_linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    
    switch ( sampler->wrap_s )
    {
      case cgltf_wrap_mode_clamp_to_edge:
        creation.address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        break;
      case cgltf_wrap_mode_mirrored_repeat:
        creation.address_mode_u = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        break;
      case cgltf_wrap_mode_repeat:
        creation.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        break;
    }
    
    switch ( sampler->wrap_t )
    {
    case cgltf_wrap_mode_clamp_to_edge:
      creation.address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      break;
    case cgltf_wrap_mode_mirrored_repeat:
      creation.address_mode_v = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
      break;
    case cgltf_wrap_mode_repeat:
      creation.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      break;
    }

    creation.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    
    creation.name = "";
    
    sampler_resource = crude_gfx_renderer_create_sampler( scene_renderer->renderer, &creation );
    CRUDE_ARRAY_PUSH( scene_renderer->samplers, *sampler_resource );
  }
}

void
load_buffers_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
)
{
  for ( uint32 buffer_view_index = 0; buffer_view_index < gltf->buffer_views_count; ++buffer_view_index )
  {
    cgltf_buffer_view const                               *buffer_view;
    cgltf_buffer const                                    *buffer;
    crude_gfx_buffer_creation                              cpu_buffer_creation;
    crude_gfx_buffer_creation                              gpu_buffer_creation;
    crude_gfx_buffer_handle                                cpu_buffer;
    crude_gfx_renderer_buffer                             *gpu_buffer_resource;
    uint8                                                 *buffer_data;
    char const                                            *cpu_buffer_name;
    char const                                            *gpu_buffer_name;

    buffer_view = &gltf->buffer_views[ buffer_view_index ];
    buffer = buffer_view->buffer;
  
    buffer_data = ( uint8* )buffer->data + buffer_view->offset;
  
    if ( buffer_view->name == NULL )
    {
      cpu_buffer_name = crude_string_buffer_append_use_f( temporary_string_buffer, "scene_renderer_buffer_cpu_%i", buffer_view_index );
      gpu_buffer_name = crude_string_buffer_append_use_f( temporary_string_buffer, "scene_renderer_buffer_gpu_%i", buffer_view_index );
    }
    else
    {
      cpu_buffer_name = gpu_buffer_name = buffer_view->name;
    }
    
    cpu_buffer_creation = crude_gfx_buffer_creation_empty();
    cpu_buffer_creation.initial_data = buffer_data;
    cpu_buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    cpu_buffer_creation.size = buffer_view->size;
    cpu_buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    cpu_buffer_creation.name = cpu_buffer_name;
    cpu_buffer = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &cpu_buffer_creation );

    gpu_buffer_creation = crude_gfx_buffer_creation_empty();
    gpu_buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    gpu_buffer_creation.size = buffer_view->size;
    gpu_buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    gpu_buffer_creation.name = gpu_buffer_name;
    gpu_buffer_creation.device_only = true;
    gpu_buffer_resource = crude_gfx_renderer_create_buffer( scene_renderer->renderer, &gpu_buffer_creation );
    CRUDE_ARRAY_PUSH( scene_renderer->buffers, *gpu_buffer_resource );

    crude_gfx_asynchronous_loader_request_buffer_copy( scene_renderer->async_loader, cpu_buffer, gpu_buffer_resource->handle );

    crude_string_buffer_clear( temporary_string_buffer );
  }
}

void
load_meshes_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_entity                                        node,
  _In_ cgltf_data                                         *gltf,
  _In_ uint32                                             *gltf_mesh_index_to_mesh_primitive_index,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory,
  _In_ uint32                                              scene_renderer_buffers_offset,
  _In_ uint32                                              scene_renderer_images_offset,
  _In_ uint32                                              scene_renderer_samplers_offset
)
{
  CRUDE_ARRAY_SET_LENGTH( gltf_mesh_index_to_mesh_primitive_index, gltf->meshes_count );
  for ( uint32 mesh_index = 0; mesh_index < gltf->meshes_count; ++mesh_index )
  {
    cgltf_mesh *mesh = &gltf->meshes[ mesh_index ];
    
    gltf_mesh_index_to_mesh_primitive_index[ mesh_index ] = CRUDE_ARRAY_LENGTH( scene_renderer->meshes );

    for ( uint32 primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index )
    {
      crude_gfx_mesh_cpu                                   mesh_draw;
      cgltf_primitive                                     *mesh_primitive;
      cgltf_accessor                                      *indices_accessor;
      cgltf_buffer_view                                   *indices_buffer_view;
      crude_gfx_renderer_buffer                           *indices_buffer_gpu;
      XMVECTOR                                             bounding_center;
      float32                                              bounding_radius;
      bool                                                 material_transparent;
      
      mesh_primitive = &mesh->primitives[ primitive_index ];

      mesh_draw = CRUDE_COMPOUNT_EMPTY( crude_gfx_mesh_cpu );
      for ( uint32 i = 0; i < mesh_primitive->attributes_count; ++i )
      {
        crude_gfx_renderer_buffer *buffer_gpu = &scene_renderer->buffers[ scene_renderer_buffers_offset + cgltf_buffer_view_index( gltf, mesh_primitive->attributes[ i ].data->buffer_view ) ];
        switch ( mesh_primitive->attributes[ i ].type )
        {
        case cgltf_attribute_type_position:
        {
          XMVECTOR                                         position_max;
          XMVECTOR                                         position_min;

          mesh_draw.position_buffer = buffer_gpu->handle;
          mesh_draw.position_offset = mesh_primitive->attributes[ i ].data->offset;

          CRUDE_ASSERT( sizeof( cgltf_float[4] ) == sizeof( XMFLOAT4 ) );
          CRUDE_ASSERT( mesh_primitive->attributes[ i ].data->has_max && mesh_primitive->attributes[ i ].data->has_min );
          
          position_max = XMLoadFloat4( CRUDE_REINTERPRET_CAST( XMFLOAT4 const*, mesh_primitive->attributes[ i ].data->min ) );
          position_min = XMLoadFloat4( CRUDE_REINTERPRET_CAST( XMFLOAT4 const*, mesh_primitive->attributes[ i ].data->max ) );

          bounding_center = XMVectorAdd( position_min, position_min );
          bounding_center = XMVectorScale( bounding_center, 0.5f );
          bounding_radius = XMVectorGetX( XMVectorMax( XMVector3Length( position_max - bounding_center ), XMVector3Length( position_min - bounding_center ) ) );
          break;
        }
        case cgltf_attribute_type_tangent:
        {
          mesh_draw.tangent_buffer = buffer_gpu->handle;
          mesh_draw.tangent_offset = mesh_primitive->attributes[ i ].data->offset;
          break;
        }
        case cgltf_attribute_type_normal:
        {
          mesh_draw.normal_buffer = buffer_gpu->handle;
          mesh_draw.normal_offset = mesh_primitive->attributes[ i ].data->offset;
          break;
        }
        case cgltf_attribute_type_texcoord:
        {
          mesh_draw.texcoord_buffer = buffer_gpu->handle;
          mesh_draw.texcoord_offset = mesh_primitive->attributes[ i ].data->offset;
          break;
        }
        }
      }
      
      indices_accessor = mesh_primitive->indices;
      indices_buffer_view = indices_accessor->buffer_view;
      
      indices_buffer_gpu = &scene_renderer->buffers[ scene_renderer_buffers_offset + cgltf_buffer_view_index( gltf, indices_accessor->buffer_view ) ];

      material_transparent = create_mesh_material_( gltf, node, scene_renderer->renderer, scene_renderer, mesh_primitive->material, &mesh_draw, scene_renderer_images_offset, scene_renderer_samplers_offset );
      
      mesh_draw.index_buffer = indices_buffer_gpu->handle;
      mesh_draw.index_offset = indices_accessor->offset;
      mesh_draw.primitive_count = indices_accessor->count;
      mesh_draw.gpu_mesh_index = CRUDE_ARRAY_LENGTH( scene_renderer->meshes );

      mesh_draw.bounding_sphere.x = XMVectorGetX( bounding_center );
      mesh_draw.bounding_sphere.y = XMVectorGetY( bounding_center );
      mesh_draw.bounding_sphere.z = XMVectorGetZ( bounding_center );
      mesh_draw.bounding_sphere.w = bounding_radius;

      CRUDE_ARRAY_PUSH( scene_renderer->meshes, mesh_draw );
    }
  }
}

void
load_meshlets_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  size_t                                                   max_vertices = 128u;
  size_t                                                   max_triangles = 124u;
  float32                                                  cone_weight = 0.0f;
  uint32                                                   mesh_index;

  mesh_index = 0u;
  for ( uint32 i = 0; i < gltf->meshes_count; ++i )
  {
    cgltf_mesh *mesh = &gltf->meshes[ i ];
    for ( uint32 primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index )
    {
      cgltf_primitive                                     *mesh_primitive;
      uint32                                              *local_indices;
      meshopt_Meshlet                                     *local_meshlets;
      size_t                                               local_max_meshlets, local_meshletes_count;
      uint32                                               temporary_allocator_marker;
      uint32                                               vertices_offset, meshlets_offset;

      temporary_allocator_marker = crude_stack_allocator_get_marker( temporary_allocator );
      
      mesh_primitive = &mesh->primitives[ primitive_index ];

      /* Load meshlets data*/
      CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( local_indices, 0u, crude_stack_allocator_pack( temporary_allocator ) );
      vertices_offset = CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices );
      load_meshlet_vertices_( mesh_primitive, &scene_renderer->meshlets_vertices );
      load_meshlet_indices_( mesh_primitive, &local_indices, vertices_offset );
      
      /* Build meshlets*/
      local_max_meshlets = meshopt_buildMeshletsBound( CRUDE_ARRAY_LENGTH( local_indices ), max_vertices, max_triangles );
      
      CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( local_meshlets, local_max_meshlets, crude_stack_allocator_pack( temporary_allocator ) );

      /* Increase capacity to have acces to the previous offset, than set lenght based on the last meshlet */
      CRUDE_ARRAY_SET_CAPACITY( scene_renderer->meshlets_vertices_indices, CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices_indices ) + local_max_meshlets * max_vertices );
      CRUDE_ARRAY_SET_CAPACITY( scene_renderer->meshlets_triangles_indices, CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_triangles_indices ) + local_max_meshlets * max_triangles * 3 );
      uint32 length = CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices_indices);
      local_meshletes_count = meshopt_buildMeshlets(
        local_meshlets,
        scene_renderer->meshlets_vertices_indices + CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices_indices ),
        scene_renderer->meshlets_triangles_indices + CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_triangles_indices ),
        local_indices, CRUDE_ARRAY_LENGTH( local_indices ), 
        &scene_renderer->meshlets_vertices[ 0 ].position.x, CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices ), sizeof( crude_gfx_meshlet_vertex_gpu ), 
        max_vertices, max_triangles, cone_weight
      );
      
      meshlets_offset = CRUDE_ARRAY_LENGTH( scene_renderer->meshlets );
      CRUDE_ARRAY_SET_CAPACITY( scene_renderer->meshlets, meshlets_offset + local_meshletes_count );

      for ( uint32 meshlet_index = 0; meshlet_index < local_meshletes_count; ++meshlet_index )
      {
        meshopt_Meshlet const *local_meshlet = &local_meshlets[ meshlet_index ];

        CRUDE_ASSERT( local_meshlet->vertex_count <= max_vertices );
        CRUDE_ASSERT( local_meshlet->triangle_count <= max_triangles );

        meshopt_optimizeMeshlet(
          scene_renderer->meshlets_vertices_indices + CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices_indices ) + local_meshlet->vertex_offset,
          scene_renderer->meshlets_triangles_indices + CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_triangles_indices ) + local_meshlet->triangle_offset,
          local_meshlet->triangle_count, local_meshlet->vertex_count
        );
      }

      for ( uint32 meshlet_index = 0; meshlet_index < local_meshletes_count; ++meshlet_index )
      {
        meshopt_Meshlet const *local_meshlet = &local_meshlets[ meshlet_index ];

        meshopt_Bounds meshlet_bounds = meshopt_computeMeshletBounds(
          scene_renderer->meshlets_vertices_indices + CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices_indices ) + local_meshlet->vertex_offset,
          scene_renderer->meshlets_triangles_indices + CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_triangles_indices ) + local_meshlet->triangle_offset,
          local_meshlet->triangle_count, &scene_renderer->meshlets_vertices[ 0 ].position.x, CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices ), sizeof( crude_gfx_meshlet_vertex_gpu )
        );;

        crude_gfx_meshlet_gpu new_meshlet = CRUDE_COMPOUNT_EMPTY( crude_gfx_meshlet_gpu );
        new_meshlet.vertices_offset = CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices_indices ) + local_meshlet->vertex_offset;
        new_meshlet.triangles_offset = CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_triangles_indices ) + local_meshlet->triangle_offset;
        new_meshlet.vertices_count = local_meshlet->vertex_count;
        new_meshlet.triangles_count = local_meshlet->triangle_count;
        new_meshlet.mesh_index = mesh_index;

        new_meshlet.center = CRUDE_COMPOUNT( XMFLOAT3, { meshlet_bounds.center[ 0 ], meshlet_bounds.center[ 1 ], meshlet_bounds.center[ 2 ] } );
        new_meshlet.radius = meshlet_bounds.radius;
        
        new_meshlet.cone_axis[ 0 ] = meshlet_bounds.cone_axis_s8[ 0 ];
        new_meshlet.cone_axis[ 1 ] = meshlet_bounds.cone_axis_s8[ 1 ];
        new_meshlet.cone_axis[ 2 ] = meshlet_bounds.cone_axis_s8[ 2 ];

        new_meshlet.cone_cutoff = meshlet_bounds.cone_cutoff_s8;

        CRUDE_ARRAY_PUSH( scene_renderer->meshlets, new_meshlet );
      }
      
      crude_gfx_meshlet_gpu const *last_meshlet = &CRUDE_ARRAY_BACK( scene_renderer->meshlets );
      CRUDE_ARRAY_SET_LENGTH( scene_renderer->meshlets_vertices_indices, last_meshlet->vertices_offset + last_meshlet->vertices_count );
      CRUDE_ARRAY_SET_LENGTH( scene_renderer->meshlets_triangles_indices, last_meshlet->triangles_offset + 3u * last_meshlet->triangles_count );

      crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_marker );
      
      scene_renderer->meshes[ mesh_index ].meshlets_count = local_meshletes_count;
      scene_renderer->meshes[ mesh_index ].meshlets_offset = meshlets_offset;
      ++mesh_index;
    }
  }
}

void
load_nodes_
(
  _In_ cgltf_data                                         *gltf,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_entity                                        parent_node,
  _In_ cgltf_node                                        **gltf_nodes,
  _In_ uint32                                              gltf_nodes_count,
  _In_ uint32                                             *gltf_mesh_index_to_mesh_primitive_index,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{ 
  for ( uint32 i = 0u; i < gltf_nodes_count; ++i )
  {
    crude_entity                                           node;
    crude_transform                                        transform;
    
    node = crude_entity_create_empty( parent_node.world, gltf_nodes[ i ]->name );
    crude_entity_set_parent( node, parent_node );
    
    if ( gltf_nodes[ i ]->has_translation )
    {
      XMStoreFloat3( &transform.translation, XMVectorSet( gltf_nodes[ i ]->translation[ 0 ], gltf_nodes[ i ]->translation[ 1 ], gltf_nodes[ i ]->translation[ 2 ], 1 ));
    }
    else
    {
      XMStoreFloat3( &transform.translation, XMVectorZero( ) );
    }
    XMStoreFloat3( &transform.scale, XMVectorReplicate( 1.f ) );
    XMStoreFloat4( &transform.rotation, XMQuaternionIdentity( ) );

    CRUDE_ENTITY_SET_COMPONENT( node, crude_transform, {
      transform.translation, transform.rotation, transform.scale
    } );
    
    if ( gltf_nodes[ i ]->mesh )
    {
      uint32 mesh_index_offset = gltf_mesh_index_to_mesh_primitive_index[ cgltf_mesh_index( gltf, gltf_nodes[ i ]->mesh ) ];
      for ( uint32 pi = 0; pi < gltf_nodes[ i ]->mesh->primitives_count; ++pi )
      {
        crude_gfx_mesh_instance_cpu mesh_instance;
        mesh_instance.node = node;
        mesh_instance.mesh = &scene_renderer->meshes[ mesh_index_offset + pi ];
        mesh_instance.material_pass_index = 0;
        CRUDE_ARRAY_PUSH( scene_renderer->meshes_instances, mesh_instance );
      }
    }

    load_nodes_( gltf, scene_renderer, node, gltf_nodes[ i ]->children, gltf_nodes[ i ]->children_count, gltf_mesh_index_to_mesh_primitive_index, temporary_allocator );
  }
}

void
load_meshlet_vertices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ crude_gfx_meshlet_vertex_gpu                      **vertices
)
{
  XMFLOAT3                                                *primitive_positions;
  XMFLOAT3                                                *primitive_normals;
  XMFLOAT2                                                *primitive_texcoords;
  uint32                                                   meshlet_vertices_count;
  
  primitive_positions = primitive_normals = NULL;
  primitive_texcoords = NULL;

  meshlet_vertices_count = primitive->attributes[ 0 ].data->count;
  
  for ( uint32 i = 0; i < primitive->attributes_count; ++i )
  {
    cgltf_attribute *attribute = &primitive->attributes[ i ];
    CRUDE_ASSERT( meshlet_vertices_count == attribute->data->count );

    uint8 *attribute_data = CRUDE_STATIC_CAST( uint8*, attribute->data->buffer_view->buffer->data ) + attribute->data->buffer_view->offset;
    switch ( attribute->type )
    {
    case cgltf_attribute_type_position:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec3 );
      CRUDE_ASSERT( attribute->data->stride == sizeof( XMFLOAT3 ) );
      primitive_positions = CRUDE_CAST( XMFLOAT3*, attribute_data );
      break;
    }
    case cgltf_attribute_type_normal:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec3 );
      CRUDE_ASSERT( attribute->data->stride == sizeof( XMFLOAT3 ) );
      primitive_normals = CRUDE_CAST( XMFLOAT3*, attribute_data );
      break;
    }
    case cgltf_attribute_type_texcoord:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec2 );
      CRUDE_ASSERT( attribute->data->stride == sizeof( XMFLOAT2 ) );
      primitive_texcoords = CRUDE_CAST( XMFLOAT2*, attribute_data );
      break;
    }
    }
  }

  uint32 old_length = CRUDE_ARRAY_LENGTH( *vertices );
  uint32 new_cap = CRUDE_ARRAY_LENGTH( *vertices ) + meshlet_vertices_count;
  CRUDE_ARRAY_SET_CAPACITY( *vertices, CRUDE_ARRAY_LENGTH( *vertices ) + meshlet_vertices_count );
  for ( uint32 i = 0; i < meshlet_vertices_count; ++i )
  {
    CRUDE_ASSERT( primitive_positions );
    CRUDE_ASSERT( primitive_normals );
    CRUDE_ASSERT( primitive_texcoords );
    
    crude_gfx_meshlet_vertex_gpu new_meshlet_vertex;
    new_meshlet_vertex.position.x = primitive_positions[ i ].x;
    new_meshlet_vertex.position.y = primitive_positions[ i ].y;
    new_meshlet_vertex.position.z = primitive_positions[ i ].z;
    new_meshlet_vertex.normal[ 0 ] = ( primitive_normals[ i ].x + 1.0f ) * 127.0f;
    new_meshlet_vertex.normal[ 1 ] = ( primitive_normals[ i ].y + 1.0f ) * 127.0f;
    new_meshlet_vertex.normal[ 2 ] = ( primitive_normals[ i ].z + 1.0f ) * 127.0f;
    new_meshlet_vertex.texcoords[ 0 ] = meshopt_quantizeHalf( primitive_texcoords[ i ].x );
    new_meshlet_vertex.texcoords[ 1 ] = meshopt_quantizeHalf( primitive_texcoords[ i ].y );
    CRUDE_ARRAY_PUSH( *vertices, new_meshlet_vertex );
  }
}

static void
load_meshlet_indices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ uint32                                            **indices,
  _In_ uint32                                              vertices_offset
)
{
  uint32                                                   meshlet_vertices_indices_count;
  uint8                                                   *buffer_data;

  meshlet_vertices_indices_count = primitive->indices->count;
  buffer_data = CRUDE_CAST( uint8*, primitive->indices->buffer_view->buffer->data ) + primitive->indices->buffer_view->offset + primitive->indices->offset;
  CRUDE_ARRAY_SET_LENGTH( *indices, meshlet_vertices_indices_count );
  
  CRUDE_ASSERT( primitive->indices->type == cgltf_type_scalar );

  if ( primitive->indices->component_type == cgltf_component_type_r_16u )
  {
    uint16 *primitive_indices = CRUDE_CAST( uint16*, buffer_data );
    for ( uint32 i = 0; i < meshlet_vertices_indices_count; ++i )
    {
      ( *indices )[ i ] = primitive_indices[ i ] + vertices_offset;
    }
  }
  else if ( primitive->indices->component_type == cgltf_component_type_r_32u )
  {
    uint32 *primitive_indices = CRUDE_CAST( uint32*, buffer_data );
    for ( uint32 i = 0; i < meshlet_vertices_indices_count; ++i )
    {
      ( *indices )[ i ] = primitive_indices[ i ] + vertices_offset;
    }
  }
  else
  {
    CRUDE_ASSERT( false );
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