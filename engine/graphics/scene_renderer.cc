#include <TaskScheduler_c.h>
#include <thirdparty/cgltf/cgltf.h>
#include <thirdparty/stb/stb_image.h>
#include <meshoptimizer.h>

#include <engine/core/time.h>
#include <engine/core/profiler.h>
#include <engine/core/array.h>
#include <engine/core/file.h>
#include <engine/core/hashmapstr.h>

#include <engine/audio/audio_ecs.h>
#include <engine/scene/scene_ecs.h>
#include <engine/physics/physics_ecs.h>
#include <engine/scene/scene_debug_ecs.h>
#include <engine/graphics/shaders/common/debug.h>

#include <engine/graphics/gpu_profiler.h>
#include <engine/graphics/scene_renderer.h>

/**
 * Scene Renderer Other
 */
static void
crude_gfx_scene_renderer_update_dynamic_buffers_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

#if CRUDE_GFX_RAY_TRACING_ENABLED
static void
crude_gfx_scene_renderer_create_top_level_acceleration_structure_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

static void
crude_gfx_scene_renderer_update_top_level_acceleration_structure_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

static void
crude_gfx_scene_renderer_create_acceleration_stucture_dsl_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

static void
crude_gfx_scene_renderer_create_acceleration_stucture_ds_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);
#endif

/**
 * Scene Renderer Utils
 */
static void
crude_scene_renderer_register_nodes_instances_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node
);

static void
crude_scene_renderer_cull_lights_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
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
#if CRUDE_DEVELOP
  scene_renderer->physics_shapes_manager = creation->physics_shapes_manager;
#endif

  scene_renderer->options = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene_renderer_options );

  scene_renderer->options.depth_pyramid_pass.depth = "depth";
  
  scene_renderer->options.ssr_pass.max_steps = 100;
  scene_renderer->options.ssr_pass.max_distance = 100;
  scene_renderer->options.ssr_pass.fade_end = 0.2;
  scene_renderer->options.ssr_pass.fade_start = 100;
  
  scene_renderer->options.ssr_pass.stride_zcutoff = 0.011;
  scene_renderer->options.ssr_pass.stride = 20;
  scene_renderer->options.ssr_pass.z_thickness = 0.6;
  scene_renderer->options.ssr_pass.depth_texture = "depth";
  scene_renderer->options.ssr_pass.normal_texture = "gbuffer_normal";
  scene_renderer->options.ssr_pass.roughness_metalness_texture = "gbuffer_roughness_metalness";
  scene_renderer->options.ssr_pass.ssr_hit_uv_depth_rdotv_texture = "ssr_hit_uv_depth_rdotv";
  scene_renderer->options.ssr_pass.ssr_texture = "ssr";
  scene_renderer->options.ssr_pass.direct_radiance_texture = "direct_radiance";
  
  scene_renderer->options.compose_pass.direct_radiance_texture = "direct_radiance";
  scene_renderer->options.compose_pass.ssr_texture = "ssr";
  scene_renderer->options.compose_pass.output_texture = "radiance";
  scene_renderer->options.compose_pass.packed_roughness_metalness_texture = "gbuffer_roughness_metalness";
  
#if CRUDE_GFX_SSR_ENABLED
  scene_renderer->options.postprocessing_pass.hdr_pre_tonemapping = "radiance";
#else
  scene_renderer->options.postprocessing_pass.hdr_pre_tonemapping = "direct_radiance";
#endif
  scene_renderer->options.postprocessing_pass.gamma = 2.2;
   
  scene_renderer->options.scene.background_color = CRUDE_COMPOUNT( XMFLOAT3, { 0.529, 0.807, 0.921 } );
  scene_renderer->options.scene.background_intensity = 1.f;
  scene_renderer->options.scene.ambient_color = CRUDE_COMPOUNT( XMFLOAT3, { 1, 1, 1 } );
  scene_renderer->options.scene.ambient_intensity = 2.f;

#if CRUDE_DEVELOP
  scene_renderer->options.debug.debug_mode = CRUDE_SHADER_DEBUG_MODE_NONE;
  scene_renderer->options.debug.flags1 = 0;
  scene_renderer->options.debug.force_roughness = 1;
  scene_renderer->options.debug.force_metalness = 0;
#endif /* CRUDE_DEVELOP */

  scene_renderer->total_meshes_instances_buffer_capacity = CRUDE_GFX_SCENE_RENDERER_MESH_INSTANCES_BUFFER_CAPACITY;
  scene_renderer->total_joints_matrices_buffer_capacity = CRUDE_GFX_SCENE_RENDERER_JOINT_MATRICES_BUFFER_CAPACITY;
  
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->lights, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->culled_lights, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->model_renderer_resoruces_instances, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene_renderer->prev_model_renderer_resoruces_instances, 0u, crude_heap_allocator_pack( scene_renderer->allocator ) );
  
#if CRUDE_GFX_RAY_TRACING_ENABLED
  scene_renderer->tlas_instances_hga = crude_gfx_memory_allocation_empty( );
  scene_renderer->tlas_scratch_hga = crude_gfx_memory_allocation_empty( );
  scene_renderer->tlas_hga = crude_gfx_memory_allocation_empty( );

  scene_renderer->acceleration_stucture_ds = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
  crude_gfx_scene_renderer_create_acceleration_stucture_dsl_( scene_renderer );
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */

  scene_renderer->lights_hga = crude_gfx_memory_allocation_empty( );
  scene_renderer->lights_world_to_texture_hga = crude_gfx_memory_allocation_empty( );
  
  scene_renderer->joint_matrices_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( XMFLOAT4X4 ) * scene_renderer->total_joints_matrices_buffer_capacity, CRUDE_GFX_MEMORY_TYPE_GPU, "joint_matrices_hga", 0 );

  scene_renderer->meshes_instances_draws_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_mesh_instance_draw ) * scene_renderer->total_meshes_instances_buffer_capacity, CRUDE_GFX_MEMORY_TYPE_GPU, "meshes_instances_draws", 0 );
  scene_renderer->scene_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_scene ), CRUDE_GFX_MEMORY_TYPE_GPU, "scene", 0 );
  scene_renderer->mesh_task_indirect_commands_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, scene_renderer->total_meshes_instances_buffer_capacity * sizeof( crude_gfx_mesh_draw_command ), CRUDE_GFX_MEMORY_TYPE_GPU, "mesh_task_indirect_commands", 0 );
  scene_renderer->mesh_task_indirect_commands_culled_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, scene_renderer->total_meshes_instances_buffer_capacity * sizeof( crude_gfx_mesh_draw_command ), CRUDE_GFX_MEMORY_TYPE_GPU, "mesh_task_indirect_commands_culled_hga", 0 );
  scene_renderer->mesh_task_indirect_count_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_mesh_draw_count ), CRUDE_GFX_MEMORY_TYPE_GPU, "mesh_task_indirect_count_hga", 0 );

  scene_renderer->debug_commands_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_debug_counts ), CRUDE_GFX_MEMORY_TYPE_GPU, "debug_commands_hga", 0 );
  scene_renderer->debug_line_vertices_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_debug_line_vertex ) * CRUDE_GFX_SCENE_RENDERER_MAX_DEBUG_LINES * 2u, CRUDE_GFX_MEMORY_TYPE_GPU, "debug_line_vertices_hga", 0 );
  scene_renderer->debug_cubes_instances_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_debug_cube_instance ) * CRUDE_GFX_SCENE_RENDERER_MAX_DEBUG_CUBES, CRUDE_GFX_MEMORY_TYPE_GPU, "debug_cubes_instances_hga", 0 );

#if CRUDE_DEVELOP
  {
    crude_gfx_model_renderer_resources_instance_initialize(
      &scene_renderer->light_model_renderer_resources_instance,
      scene_renderer->model_renderer_resources_manager,
      crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_light_tetrahedron.gltf" ) );
    scene_renderer->light_model_renderer_resources_instance.cast_shadow = false;
    
    crude_gfx_model_renderer_resources_instance_initialize(
      &scene_renderer->camera_model_renderer_resources_instance,
      scene_renderer->model_renderer_resources_manager,
      crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_camera.gltf" ) );
    scene_renderer->camera_model_renderer_resources_instance.cast_shadow = false;
    
    crude_gfx_model_renderer_resources_instance_initialize(
      &scene_renderer->capsule_model_renderer_resources_instance,
      scene_renderer->model_renderer_resources_manager,
      crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_capsule.gltf" ) );
    scene_renderer->capsule_model_renderer_resources_instance.cast_shadow = false;
    
    crude_gfx_model_renderer_resources_instance_initialize(
      &scene_renderer->physics_box_collision_model_renderer_resources_instance,
      scene_renderer->model_renderer_resources_manager,
      crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_physics_box_collision_shape.gltf" ) );
    scene_renderer->physics_box_collision_model_renderer_resources_instance.cast_shadow = false;
    
    crude_gfx_model_renderer_resources_instance_initialize(
      &scene_renderer->audo_player_model_renderer_resources_instance,
      scene_renderer->model_renderer_resources_manager,
      crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_audio_player.gltf" ) );
    scene_renderer->audo_player_model_renderer_resources_instance.cast_shadow = false;
    
    crude_gfx_model_renderer_resources_instance_initialize(
      &scene_renderer->audio_listener_model_renderer_resources_instance,
      scene_renderer->model_renderer_resources_manager,
      crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_audio_listener.gltf" ) );
    scene_renderer->audio_listener_model_renderer_resources_instance.cast_shadow = false;
    
    crude_gfx_model_renderer_resources_instance_initialize(
      &scene_renderer->ray_model_renderer_resources_instance,
      scene_renderer->model_renderer_resources_manager,
      crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_ray.gltf" ) );
    scene_renderer->ray_model_renderer_resources_instance.cast_shadow = false;
  }
#endif /* CRUDE_DEVELOP */
  crude_gfx_scene_renderer_on_resize( scene_renderer );

  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Initialize scene renderer passes." );

  if ( scene_renderer->imgui_pass_enalbed )
  {
    crude_gfx_imgui_pass_initialize( &scene_renderer->imgui_pass, scene_renderer );
  }
  crude_gfx_opaque_early_pass_initialize( &scene_renderer->opaque_early_pass, scene_renderer );
  crude_gfx_opaque_late_pass_initialize( &scene_renderer->opaque_late_pass, scene_renderer );
  crude_gfx_depth_pyramid_pass_initialize( &scene_renderer->depth_pyramid_pass, scene_renderer );
  crude_gfx_pointlight_shadow_pass_initialize( &scene_renderer->pointlight_shadow_pass, scene_renderer );
  crude_gfx_culling_early_pass_initialize( &scene_renderer->culling_early_pass, scene_renderer );
  crude_gfx_culling_late_pass_initialize( &scene_renderer->culling_late_pass, scene_renderer );
  crude_gfx_debug_pass_initialize( &scene_renderer->debug_pass, scene_renderer );
  crude_gfx_compose_pass_initialize( &scene_renderer->compose_pass, scene_renderer );
  crude_gfx_postprocessing_pass_initialize( &scene_renderer->postprocessing_pass, scene_renderer );
  crude_gfx_translucent_pass_initialize( &scene_renderer->translucent_pass, scene_renderer );
  crude_gfx_light_lut_pass_initialize( &scene_renderer->light_lut_pass, scene_renderer );
  //crude_gfx_ssr_pass_initialize( &scene_renderer->ssr_pass, scene_renderer );
#if CRUDE_GFX_RAY_TRACING_SOLID_DEBUG_ENABLED
  crude_gfx_ray_tracing_solid_pass_initialize( &scene_renderer->ray_tracing_solid_pass, scene_renderer );
#endif
#if CRUDE_GFX_RAY_TRACING_DDGI_ENABLED
  crude_gfx_indirect_light_pass_initialize( &scene_renderer->indirect_light_pass, scene_renderer );
#endif
}

void
crude_gfx_scene_renderer_deinitialize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_render_graph_builder_unregister_all_render_passes( scene_renderer->render_graph->builder );
#if CRUDE_GFX_RAY_TRACING_SOLID_DEBUG_ENABLED
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "ray_tracing_solid_pass" );
#endif
#if CRUDE_GFX_RAY_TRACING_DDGI_ENABLED
  crude_gfx_render_graph_builder_unregister_render_pass( scene_renderer->render_graph->builder, "indirect_light_pass" );
#endif
  
  if ( scene_renderer->imgui_pass_enalbed )
  {
    crude_gfx_imgui_pass_deinitialize( &scene_renderer->imgui_pass );
  }
  crude_gfx_opaque_early_pass_deinitialize( &scene_renderer->opaque_early_pass );
  crude_gfx_opaque_late_pass_deinitialize( &scene_renderer->opaque_late_pass );
  crude_gfx_depth_pyramid_pass_deinitialize( &scene_renderer->depth_pyramid_pass );
  crude_gfx_pointlight_shadow_pass_deinitialize( &scene_renderer->pointlight_shadow_pass );
  crude_gfx_culling_early_pass_deinitialize( &scene_renderer->culling_early_pass );
  crude_gfx_culling_late_pass_deinitialize( &scene_renderer->culling_late_pass );
  crude_gfx_debug_pass_deinitialize( &scene_renderer->debug_pass );
  crude_gfx_compose_pass_deinitialize( &scene_renderer->compose_pass );
  crude_gfx_postprocessing_pass_deinitialize( &scene_renderer->postprocessing_pass );
  crude_gfx_translucent_pass_deinitialize( &scene_renderer->translucent_pass );
  crude_gfx_light_lut_pass_deinitialize( &scene_renderer->light_lut_pass );
  //crude_gfx_ssr_pass_deinitialize( &scene_renderer->ssr_pass );
  
#if CRUDE_GFX_RAY_TRACING_SOLID_DEBUG_ENABLED
  crude_gfx_ray_tracing_solid_pass_deinitialize( &scene_renderer->ray_tracing_solid_pass );
#endif

#if CRUDE_GFX_RAY_TRACING_DDGI_ENABLED
  crude_gfx_indirect_light_pass_deinitialize( &scene_renderer->indirect_light_pass );
#endif

  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->lights_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->lights_world_to_texture_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->scene_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->meshes_instances_draws_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->mesh_task_indirect_commands_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->mesh_task_indirect_commands_culled_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->mesh_task_indirect_count_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->debug_commands_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->debug_cubes_instances_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->debug_line_vertices_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->joint_matrices_hga );
  
#if CRUDE_GFX_RAY_TRACING_ENABLED
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->tlas_instances_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->tlas_scratch_hga );
  crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->tlas_hga );
  scene_renderer->gpu->vkDestroyAccelerationStructureKHR( scene_renderer->gpu->vk_device, scene_renderer->vk_tlas, scene_renderer->gpu->vk_allocation_callbacks );

  crude_gfx_destroy_descriptor_set( scene_renderer->gpu, scene_renderer->acceleration_stucture_ds );
  crude_gfx_destroy_descriptor_set_layout( scene_renderer->gpu, scene_renderer->acceleration_stucture_dsl );
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */

  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->prev_model_renderer_resoruces_instances );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->model_renderer_resoruces_instances );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->lights );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->culled_lights );
  
#if CRUDE_DEVELOP
  crude_gfx_model_renderer_resources_instance_deinitialize( &scene_renderer->light_model_renderer_resources_instance );
  crude_gfx_model_renderer_resources_instance_deinitialize( &scene_renderer->camera_model_renderer_resources_instance );
  crude_gfx_model_renderer_resources_instance_deinitialize( &scene_renderer->capsule_model_renderer_resources_instance );
  crude_gfx_model_renderer_resources_instance_deinitialize( &scene_renderer->physics_box_collision_model_renderer_resources_instance );
  crude_gfx_model_renderer_resources_instance_deinitialize( &scene_renderer->audio_listener_model_renderer_resources_instance );
  crude_gfx_model_renderer_resources_instance_deinitialize( &scene_renderer->audo_player_model_renderer_resources_instance );
  crude_gfx_model_renderer_resources_instance_deinitialize( &scene_renderer->ray_model_renderer_resources_instance );
#endif /* CRUDE_DEVELOP */
}

bool
crude_gfx_scene_renderer_update_instances_from_node
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        main_node
)
{
  bool                                                     should_recreated_tlas, buffers_recrteated, model_initialized;
 
  CRUDE_PROFILER_ZONE_NAME( "crude_gfx_scene_renderer_update_instances_from_node" );

  model_initialized = false;
  
#if CRUDE_DEVELOP
  // load debug light (in case it was cleaned because of new model manager resource clean)
  scene_renderer->light_model_renderer_resources_instance.model_renderer_resources_handle = crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_light_tetrahedron.gltf" );
  scene_renderer->camera_model_renderer_resources_instance.model_renderer_resources_handle = crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_camera.gltf" );
  scene_renderer->capsule_model_renderer_resources_instance.model_renderer_resources_handle = crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_capsule.gltf" );
  scene_renderer->physics_box_collision_model_renderer_resources_instance.model_renderer_resources_handle = crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_physics_box_collision_shape.gltf" );
  scene_renderer->audo_player_model_renderer_resources_instance.model_renderer_resources_handle = crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_audio_player.gltf" );
  scene_renderer->audio_listener_model_renderer_resources_instance.model_renderer_resources_handle = crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_audio_listener.gltf" );
  scene_renderer->ray_model_renderer_resources_instance.model_renderer_resources_handle = crude_gfx_model_renderer_resources_manager_get_gltf_model( scene_renderer->model_renderer_resources_manager, "editor\\models\\crude_ray.gltf" );
#endif /* CRUDE_DEVELOP */
  
  CRUDE_SWAP( scene_renderer->model_renderer_resoruces_instances, scene_renderer->prev_model_renderer_resoruces_instances );
  CRUDE_ARRAY_SET_LENGTH( scene_renderer->model_renderer_resoruces_instances, 0u );
  CRUDE_ARRAY_SET_LENGTH( scene_renderer->lights, 0u );
  CRUDE_ARRAY_SET_LENGTH( scene_renderer->culled_lights, 0u );

  crude_scene_renderer_register_nodes_instances_( scene_renderer, world, main_node );

  should_recreated_tlas = false;

  if ( CRUDE_ARRAY_LENGTH( scene_renderer->prev_model_renderer_resoruces_instances ) != CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances ) )
  {
    should_recreated_tlas = true;
  }
  else
  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->prev_model_renderer_resoruces_instances ); ++i )
    {
      if ( scene_renderer->prev_model_renderer_resoruces_instances[ i ].unique_id != scene_renderer->model_renderer_resoruces_instances[ i ].unique_id )
      {
        should_recreated_tlas = true;
      }
    }
  }

  scene_renderer->total_meshes_instances_count = 0u;
  scene_renderer->total_joints_matrices_count = 1u; /* reserve default matrix */
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances ); ++i )
  {
    crude_gfx_model_renderer_resources const              *model_renderer_resources;
    
    model_renderer_resources = crude_gfx_model_renderer_resources_manager_access_model_renderer_resources( scene_renderer->model_renderer_resources_manager, scene_renderer->model_renderer_resoruces_instances[ i ].model_renderer_resources_handle );
    
    for ( uint32 node_index = 0; node_index < CRUDE_ARRAY_LENGTH( model_renderer_resources->nodes ); ++node_index )
    {
      crude_gfx_node                                      *node;

      node = &model_renderer_resources->nodes[ node_index ];

      if ( node->meshes )
      {
        scene_renderer->total_meshes_instances_count += CRUDE_ARRAY_LENGTH( node->meshes );
      }

      if ( node->skin != -1 )
      {
        crude_gfx_skin                                    *skin;

        skin = &model_renderer_resources->skins[ node->skin ];
        scene_renderer->total_joints_matrices_count += CRUDE_ARRAY_LENGTH( skin->joints );
      }
    }
  }

  buffers_recrteated = should_recreated_tlas;
  
#if CRUDE_GFX_RAY_TRACING_ENABLED
  if ( should_recreated_tlas )
  {
    if ( scene_renderer->tlas_hga.gpu_address )
    {
      crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->tlas_hga );
      crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->tlas_instances_hga );
      crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->tlas_scratch_hga );
      scene_renderer->gpu->vkDestroyAccelerationStructureKHR( scene_renderer->gpu->vk_device, scene_renderer->vk_tlas, scene_renderer->gpu->vk_allocation_callbacks );
    }

    crude_gfx_cmd_buffer *cmd = crude_gfx_access_cmd_buffer( scene_renderer->gpu, scene_renderer->gpu->immediate_transfer_cmd_buffer );
    crude_gfx_cmd_begin_primary( cmd );
    crude_gfx_scene_renderer_create_top_level_acceleration_structure_( scene_renderer, cmd );
    crude_gfx_submit_immediate( cmd );

    crude_gfx_scene_renderer_create_acceleration_stucture_ds_( scene_renderer );
  }
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */

  if ( scene_renderer->total_joints_matrices_count > scene_renderer->total_joints_matrices_buffer_capacity )
  {
    scene_renderer->total_joints_matrices_buffer_capacity = 4 * scene_renderer->total_joints_matrices_count; 

    if ( crude_gfx_memory_allocation_valid( &scene_renderer->joint_matrices_hga ) )
    {
      crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->joint_matrices_hga );
    }

    scene_renderer->joint_matrices_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( XMFLOAT4X4 ) * scene_renderer->total_joints_matrices_buffer_capacity, CRUDE_GFX_MEMORY_TYPE_GPU, "joint_matrices_hga", 0 );

    buffers_recrteated = true;
  }
  
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

    scene_renderer->meshes_instances_draws_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_mesh_instance_draw ) * scene_renderer->total_meshes_instances_buffer_capacity, CRUDE_GFX_MEMORY_TYPE_GPU, "meshes_instances_draws", 0 );
    scene_renderer->mesh_task_indirect_commands_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, scene_renderer->total_meshes_instances_buffer_capacity * sizeof( crude_gfx_mesh_draw_command ), CRUDE_GFX_MEMORY_TYPE_GPU, "mesh_task_indirect_commands", 0 );
    scene_renderer->mesh_task_indirect_commands_culled_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, scene_renderer->total_meshes_instances_buffer_capacity * sizeof( crude_gfx_mesh_draw_command ), CRUDE_GFX_MEMORY_TYPE_GPU, "mesh_task_indirect_commands_culled_hga", 0 );

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
  
  if ( crude_gfx_memory_allocation_valid( &scene_renderer->lights_world_to_texture_hga ) )
  {
    crude_gfx_memory_deallocate( scene_renderer->gpu, scene_renderer->lights_world_to_texture_hga );
  }

  scene_renderer->lights_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( crude_gfx_light ) * CRUDE_LIGHTS_MAX_COUNT, CRUDE_GFX_MEMORY_TYPE_GPU, "lights_hga", 0 );
  scene_renderer->lights_world_to_texture_hga = crude_gfx_memory_allocate_with_name( scene_renderer->gpu, sizeof( XMFLOAT4X4 ) * CRUDE_LIGHTS_MAX_COUNT * 4u, CRUDE_GFX_MEMORY_TYPE_GPU, "lights_world_to_texture_hga", 0 );
}

void
crude_gfx_scene_renderer_start_frame
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  scene_renderer->primary_cmd = crude_gfx_get_primary_cmd( scene_renderer->gpu, 0, true );
}

void
crude_gfx_scene_renderer_update_dynamic_buffers
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_gfx_scene_renderer_update_dynamic_buffers" );
  crude_gfx_cmd_push_marker( scene_renderer->primary_cmd, "crude_gfx_scene_renderer_update_dynamic_buffers" );
  crude_scene_renderer_cull_lights_( scene_renderer );
  crude_gfx_scene_renderer_update_dynamic_buffers_( scene_renderer, scene_renderer->primary_cmd );
  crude_gfx_cmd_pop_marker( scene_renderer->primary_cmd );
  CRUDE_PROFILER_ZONE_END;
}

void
crude_gfx_scene_renderer_render
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_gfx_scene_renderer_render" );
  crude_gfx_cmd_push_marker( scene_renderer->primary_cmd, "crude_gfx_scene_renderer_render" );
  crude_gfx_render_graph_render( scene_renderer->render_graph, scene_renderer->primary_cmd );
  crude_gfx_cmd_pop_marker( scene_renderer->primary_cmd );
  CRUDE_PROFILER_ZONE_END;
}

void
crude_gfx_scene_renderer_queue
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_gfx_scene_renderer_queue" );
  crude_gfx_cmd_push_marker( scene_renderer->primary_cmd, "crude_gfx_scene_renderer_queue" );
  crude_gfx_queue_cmd( scene_renderer->primary_cmd );
  crude_gfx_cmd_pop_marker( scene_renderer->primary_cmd );
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
#if CRUDE_DEVELOP
    crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "imgui_editor_pass" ), crude_gfx_imgui_pass_pack( &scene_renderer->imgui_pass ) );
#else
    crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "imgui_game_pass" ), crude_gfx_imgui_pass_pack( &scene_renderer->imgui_pass ) );
#endif
  }

  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "opaque_early_pass" ), crude_gfx_opaque_early_pass_pack( &scene_renderer->opaque_early_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "opaque_late_pass" ), crude_gfx_opaque_late_pass_pack( &scene_renderer->opaque_late_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "depth_pyramid_early_pass" ), crude_gfx_depth_pyramid_pass_pack( &scene_renderer->depth_pyramid_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "depth_pyramid_late_pass" ), crude_gfx_depth_pyramid_pass_pack( &scene_renderer->depth_pyramid_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "culling_early_pass" ), crude_gfx_culling_early_pass_pack( &scene_renderer->culling_early_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "culling_late_pass" ), crude_gfx_culling_late_pass_pack( &scene_renderer->culling_late_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "debug_pass" ), crude_gfx_debug_pass_pack( &scene_renderer->debug_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "compose_pass" ), crude_gfx_compose_pass_pack( &scene_renderer->compose_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "postprocessing_pass" ), crude_gfx_postprocessing_pass_pack( &scene_renderer->postprocessing_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "translucent_pass" ), crude_gfx_translucent_pass_pack( &scene_renderer->translucent_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "light_lut_pass" ), crude_gfx_light_lut_pass_pack( &scene_renderer->light_lut_pass ) );
  //crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "ssr_pass" ), crude_gfx_ssr_pass_pack( &scene_renderer->ssr_pass ) );
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "pointlight_shadows_pass" ), crude_gfx_pointlight_shadow_pass_pack( &scene_renderer->pointlight_shadow_pass ) );
#if CRUDE_GFX_RAY_TRACING_SOLID_DEBUG_ENABLED
  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "ray_tracing_solid_pass" ), crude_gfx_ray_tracing_solid_pass_pack( &scene_renderer->ray_tracing_solid_pass ) );
#endif
#if CRUDE_GFX_RAY_TRACING_DDGI_ENABLED
  //crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, CRUDE_STRING_NODE( "indirect_light_pass" ), crude_gfx_indirect_light_pass_pack( &scene_renderer->indirect_light_pass ) );
#endif
}

void
crude_gfx_scene_renderer_on_resize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_gfx_scene_renderer_on_resize" );

  CRUDE_PROFILER_ZONE_END;
}

/**
 * Scene Renderer Other
 */
static void
crude_gfx_scene_renderer_update_dynamic_buffers_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_device                                        *gpu;

  crude_gfx_cmd_push_marker( primary_cmd, "crude_gfx_scene_renderer_update_dynamic_buffers_" );

  gpu = scene_renderer->gpu;
  
  /* Update scene constant buffer*/
  {
    crude_gfx_scene                                       *scene;
    crude_gfx_memory_allocation                            scene_tca;

    scene_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, sizeof( crude_gfx_scene ) );
    scene = CRUDE_CAST( crude_gfx_scene*, scene_tca.cpu_address );

    *scene = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene );
    scene->flags = 0u;
    scene->camera_previous = scene->camera;
    scene->resolution.x = scene_renderer->gpu->renderer_size.x;
    scene->resolution.y = scene_renderer->gpu->renderer_size.y;
    scene->resolution_ratio = CRUDE_CAST( float32, scene_renderer->gpu->renderer_size.x ) / scene_renderer->gpu->renderer_size.y;
    crude_gfx_camera_to_camera_gpu( &scene_renderer->options.scene.camera, scene_renderer->options.scene.camera_view_to_world, &scene->camera );
    scene->meshes_instances_count = scene_renderer->total_visible_meshes_instances_count;
    scene->active_lights_count = CRUDE_ARRAY_LENGTH( scene_renderer->culled_lights );
    scene->tiled_shadowmap_texture_index = scene_renderer->pointlight_shadow_pass.tetrahedron_shadow_texture.index;
    scene->inv_shadow_map_size.x = 1.f / CRUDE_GFX_TETRAHEDRON_SHADOWMAP_SIZE;
    scene->inv_shadow_map_size.y = 1.f / CRUDE_GFX_TETRAHEDRON_SHADOWMAP_SIZE;
#if CRUDE_GFX_RAY_TRACING_DDGI_ENABLED
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
#if CRUDE_DEVELOP
    scene->debug_mode = scene_renderer->options.debug.debug_mode;
    scene->debug_flags1 = scene_renderer->options.debug.flags1;
    scene->debug_force_metalness = scene_renderer->options.debug.force_metalness;
    scene->debug_force_roughness = scene_renderer->options.debug.force_roughness;
#endif

    crude_gfx_cmd_memory_copy( primary_cmd, scene_tca, scene_renderer->scene_hga, 0, 0 );
  }
  
  /* Update meshes instanse draws & joints matrices buffers */
  {
    crude_gfx_mesh_instance_draw                          *meshes_instances_draws;
    XMFLOAT4X4                                            *joint_matrices;
    crude_gfx_memory_allocation                            meshes_instances_draws_tca;
    crude_gfx_memory_allocation                            joint_matrices_tca;
    uint64                                                 joint_matrix_index;
    uint64                                                 joints_matrices_offset;

    meshes_instances_draws_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, sizeof( crude_gfx_mesh_instance_draw ) * scene_renderer->total_meshes_instances_count );
    meshes_instances_draws = CRUDE_CAST( crude_gfx_mesh_instance_draw*, meshes_instances_draws_tca.cpu_address );
  
    joint_matrices_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, sizeof( XMFLOAT4X4 ) * scene_renderer->total_joints_matrices_count );
    joint_matrices = CRUDE_CAST( XMFLOAT4X4*, joint_matrices_tca.cpu_address );
    
    joints_matrices_offset = 0u;
    joint_matrix_index = 0u;
    XMStoreFloat4x4( &joint_matrices[ joint_matrix_index++ ], XMMatrixIdentity( ) );

    scene_renderer->total_visible_meshes_instances_count = 0u;

    for ( uint32 model_instance_index = 0u; model_instance_index < CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances ); ++model_instance_index )
    {
      crude_gfx_model_renderer_resources_instance const   *model_renderer_resources_instance;
      crude_gfx_model_renderer_resources const            *model_renderer_resources;

      model_renderer_resources_instance = &scene_renderer->model_renderer_resoruces_instances[ model_instance_index ];
      model_renderer_resources = crude_gfx_model_renderer_resources_manager_access_model_renderer_resources( scene_renderer->model_renderer_resources_manager, model_renderer_resources_instance->model_renderer_resources_handle );

      for ( uint32 node_index = 0u; node_index < CRUDE_ARRAY_LENGTH( model_renderer_resources->nodes ); ++node_index )
      {
        crude_gfx_node                                    *node;
        XMMATRIX                                           mesh_to_model, model_to_world, mesh_to_world;

        node = &model_renderer_resources->nodes[ node_index ];

        joints_matrices_offset = joint_matrix_index;
        
        if ( node->meshes || node->skin != -1 )
        {
          mesh_to_model = crude_gfx_node_to_model( model_renderer_resources->nodes, model_renderer_resources_instance->nodes_transforms, node_index );
          model_to_world = XMLoadFloat4x4( &model_renderer_resources_instance->model_to_world );
          mesh_to_world = XMMatrixMultiply( mesh_to_model, model_to_world );
        }

        if ( node->meshes )
        {
          for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARRAY_LENGTH( node->meshes ); ++mesh_index )
          {
            crude_gfx_mesh_cpu                            *cpu_mesh;
            crude_gfx_mesh_instance_draw                  *gpu_meshe_instance_draw;

            cpu_mesh = &model_renderer_resources->meshes[ node->meshes[ mesh_index ] ];
            gpu_meshe_instance_draw = &meshes_instances_draws[ scene_renderer->total_visible_meshes_instances_count ]; 

            XMStoreFloat4x4( &gpu_meshe_instance_draw->mesh_to_world, mesh_to_world );
            XMStoreFloat4x4( &gpu_meshe_instance_draw->world_to_mesh, XMMatrixInverse( NULL, mesh_to_world ) );
            gpu_meshe_instance_draw->mesh_draw_index = cpu_mesh->gpu_mesh_global_index;

            if ( node->skin != -1 )
            {
              if ( CRUDE_ARRAY_LENGTH( cpu_mesh->affected_joints ) )
              {
                crude_gfx_skin                             *skin;
                XMMATRIX                                   model_to_mesh;
                XMVECTOR                                   position_max, position_min;
                XMVECTOR                                   bounding_center;
                float32                                    bounding_radius;
              
                skin = &model_renderer_resources->skins[ node->skin ];
                model_to_mesh = XMMatrixInverse( NULL, mesh_to_model );

                position_max = XMLoadFloat3( &cpu_mesh->affected_joints_local_aabb[ 0 ].max );
                position_min = XMLoadFloat3( &cpu_mesh->affected_joints_local_aabb[ 0 ].min );

                for ( uint32 affected_joint = 0; affected_joint < CRUDE_ARRAY_LENGTH( cpu_mesh->affected_joints ); ++affected_joint )
                {
                  crude_gfx_aabb_cpu                      *local_joint_aabb;
                  XMVECTOR                                 animated_joint_aabb_max;
                  XMVECTOR                                 animated_joint_aabb_min;
                  XMMATRIX                                 joint_matrix, inverse_bind_matrix;

                  local_joint_aabb = &cpu_mesh->affected_joints_local_aabb[ affected_joint ];

                  inverse_bind_matrix = XMLoadFloat4x4( &skin->inverse_bind_matrices[ cpu_mesh->affected_joints[ affected_joint ] ] );
                  joint_matrix = crude_gfx_node_to_model( model_renderer_resources->nodes, model_renderer_resources_instance->nodes_transforms, skin->joints[ cpu_mesh->affected_joints[ affected_joint ] ] );
                  joint_matrix = XMMatrixMultiply( XMMatrixMultiply( inverse_bind_matrix, joint_matrix ), model_to_mesh );
                  
                  animated_joint_aabb_max = XMVector3Transform( XMLoadFloat3( &local_joint_aabb->max ), joint_matrix );
                  animated_joint_aabb_min = XMVector3Transform( XMLoadFloat3( &local_joint_aabb->min ), joint_matrix );
                  position_max = XMVectorMax( position_max, animated_joint_aabb_max );
                  position_min = XMVectorMin( position_min, animated_joint_aabb_min );
                }

                bounding_center = XMVectorAdd( position_max, position_min );
                bounding_center = XMVectorScale( bounding_center, 0.5f );
                bounding_radius = XMVectorGetX( XMVectorMax( XMVector3Length( position_max - bounding_center ), XMVector3Length( position_min - bounding_center ) ) );
                
                XMStoreFloat4( &gpu_meshe_instance_draw->bounding_sphere, XMVectorSet( XMVectorGetX( bounding_center ), XMVectorGetY( bounding_center ), XMVectorGetZ( bounding_center ), bounding_radius ) );
              }
              else
              {
                gpu_meshe_instance_draw->bounding_sphere = cpu_mesh->default_bounding_sphere;
              }

              gpu_meshe_instance_draw->joints_matrices_offset = joints_matrices_offset;
            }
            else
            {
              gpu_meshe_instance_draw->bounding_sphere = cpu_mesh->default_bounding_sphere;
              gpu_meshe_instance_draw->joints_matrices_offset = 0;
            }

            gpu_meshe_instance_draw->flags = 0;

            if ( model_renderer_resources_instance->cast_shadow )
            {
              gpu_meshe_instance_draw->flags |= CRUDE_GFX_MESH_INSTANCE_DRAW_FLAG_CAST_SHADOW;
            }
            
            if ( node->skin != -1 )
            {
              gpu_meshe_instance_draw->flags |= CRUDE_GFX_MESH_INSTANCE_DRAW_FLAG_ANIMATED;
            }

            ++scene_renderer->total_visible_meshes_instances_count;
          }
        }

        if ( node->skin != -1 )
        {
          crude_gfx_skin                                  *skin;
          XMMATRIX                                         model_to_mesh;

          skin = &model_renderer_resources->skins[ node->skin ];
          
          model_to_mesh = XMMatrixInverse( NULL, mesh_to_model );

          for ( uint64 i = 0; i < CRUDE_ARRAY_LENGTH( skin->joints ); ++i )
          {
            XMMATRIX                                           joint_matrix, inverse_bind_matrix;

            inverse_bind_matrix = XMLoadFloat4x4( &skin->inverse_bind_matrices[ i ] );
            joint_matrix = crude_gfx_node_to_model( model_renderer_resources->nodes, model_renderer_resources_instance->nodes_transforms, skin->joints[ i ] );
            XMStoreFloat4x4( &joint_matrices[ joint_matrix_index++ ], XMMatrixMultiply( XMMatrixMultiply( inverse_bind_matrix, joint_matrix ), model_to_mesh ) );
          }
        }
      }
    }
  
    crude_gfx_cmd_memory_copy( primary_cmd, joint_matrices_tca, scene_renderer->joint_matrices_hga, 0, 0 );
    crude_gfx_cmd_memory_copy( primary_cmd, meshes_instances_draws_tca, scene_renderer->meshes_instances_draws_hga, 0, 0 );
  }
  
#if CRUDE_GFX_RAY_TRACING_ENABLED
  crude_gfx_scene_renderer_update_top_level_acceleration_structure_(scene_renderer, primary_cmd );
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */

  /* Update meshlets counes storage buffers*/
  {
    crude_gfx_mesh_draw_count                             *mesh_draw_counts;
    crude_gfx_memory_allocation                            mesh_task_indirect_count_tca;

    mesh_task_indirect_count_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, sizeof( crude_gfx_mesh_draw_count ) );
    mesh_draw_counts = CRUDE_CAST( crude_gfx_mesh_draw_count*, mesh_task_indirect_count_tca.cpu_address );

    *mesh_draw_counts = CRUDE_COMPOUNT_EMPTY( crude_gfx_mesh_draw_count );
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
    crude_gfx_debug_counts                                *debug_draw_command;
    crude_gfx_memory_allocation                            debug_draw_command_tca;

    debug_draw_command_tca = crude_gfx_linear_allocator_allocate( &gpu->frame_linear_allocator, sizeof( crude_gfx_debug_counts ) );
    debug_draw_command = CRUDE_CAST( crude_gfx_debug_counts*, debug_draw_command_tca.cpu_address );

    *debug_draw_command = CRUDE_COMPOUNT_EMPTY( crude_gfx_debug_counts );
    debug_draw_command->draw_indirect_2dline.instanceCount = 1u;
    debug_draw_command->draw_indirect_3dline.instanceCount = 1u;
    debug_draw_command->draw_indirect_cube.vertexCount = 36u;
    
    crude_gfx_cmd_memory_copy( primary_cmd, debug_draw_command_tca, scene_renderer->debug_commands_hga, 0, 0 );
  }

  crude_gfx_cmd_pop_marker( primary_cmd );
}

void
crude_scene_renderer_register_nodes_instances_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node
)
{
  uint32                                                   model_renderer_resoruces_instances_count;
  ecs_iter_t                                               children_it;
  XMMATRIX                                                 model_to_custom_model;

  children_it = crude_ecs_children( world, node );

  model_to_custom_model = XMMatrixIdentity( );
  
  model_renderer_resoruces_instances_count = CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances );

  if ( CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_gltf ) )
  {
    crude_gltf                                            *child_gltf;
  
    child_gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, node, crude_gltf );
  
    if ( !child_gltf->hidden && child_gltf->model_renderer_resources_instance.model_renderer_resources_handle.index != -1 )
    {
      XMStoreFloat4x4( &child_gltf->model_renderer_resources_instance.model_to_world, XMMatrixMultiply( model_to_custom_model, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform ) ) ) );
      child_gltf->model_renderer_resources_instance.unique_id = node;
      CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, child_gltf->model_renderer_resources_instance );
    }
  }
  
  if ( CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_light ) )
  {
    crude_gfx_light_cpu light_gpu = CRUDE_COMPOUNT_EMPTY( crude_gfx_light_cpu );
    light_gpu.light = *CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, node, crude_light );
    XMStoreFloat3( &light_gpu.translation, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, node, crude_transform ) ).r[ 3 ] );
    CRUDE_ARRAY_PUSH( scene_renderer->lights, light_gpu ); 
  }
  
#if CRUDE_DEVELOP
  if ( scene_renderer->options.debug.hide_collision)
  {
    if ( CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_light ) )
    {
      crude_gfx_light_cpu light_gpu = CRUDE_COMPOUNT_EMPTY( crude_gfx_light_cpu );
      XMStoreFloat3( &light_gpu.translation, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, node, crude_transform ) ).r[ 3 ] );
      
      XMStoreFloat4x4( &scene_renderer->light_model_renderer_resources_instance.model_to_world, XMMatrixTranslation( light_gpu.translation.x, light_gpu.translation.y, light_gpu.translation.z ) );
      CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, scene_renderer->light_model_renderer_resources_instance );
    }
  
    if ( CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_camera ) )
    {
      XMStoreFloat4x4( &scene_renderer->camera_model_renderer_resources_instance.model_to_world, XMMatrixMultiply( model_to_custom_model, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform ) ) ) );
      CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, scene_renderer->camera_model_renderer_resources_instance );
    }
  
    if ( CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_audio_player ) )
    {
      XMStoreFloat4x4( &scene_renderer->audo_player_model_renderer_resources_instance.model_to_world, XMMatrixMultiply( model_to_custom_model, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform ) ) ) );
      CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, scene_renderer->audo_player_model_renderer_resources_instance );
    }
  
    if ( CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_audio_listener ) )
    {
      XMStoreFloat4x4( &scene_renderer->audio_listener_model_renderer_resources_instance.model_to_world, XMMatrixMultiply( model_to_custom_model, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform ) ) ) );
      CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, scene_renderer->audio_listener_model_renderer_resources_instance );
    }
  
    if ( CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_ray ) )
    {
      float32 ray_distance = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_ray )->distance;
      XMMATRIX node_to_world = crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform ) );
      XMMATRIX scale_matrix = XMMatrixScaling( 1, 1, ray_distance );
      XMMATRIX model_to_world = model_to_custom_model;

      model_to_world = XMMatrixMultiply( model_to_world, scale_matrix );
      model_to_world = XMMatrixMultiply( model_to_world, node_to_world );
      XMStoreFloat4x4( &scene_renderer->ray_model_renderer_resources_instance.model_to_world, model_to_world );
      CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, scene_renderer->ray_model_renderer_resources_instance );
    }
  }
  
  if ( scene_renderer->options.debug.hide_collision)
  {
    if (  CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_physics_character ) )
    {
      crude_physics_character                               *character;

      character = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, node, crude_physics_character );
    
      model_to_custom_model = XMMatrixTranslation( 0.f, character->height, 0.f );
      model_to_custom_model = XMMatrixMultiply( XMMatrixScaling( character->radius, character->height, character->radius ), model_to_custom_model );
      XMStoreFloat4x4( &scene_renderer->capsule_model_renderer_resources_instance.model_to_world, XMMatrixMultiply( model_to_custom_model, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform ) ) ) );
      CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, scene_renderer->capsule_model_renderer_resources_instance );
    }
    
    if ( CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_physics_static_body ) )
    {
      crude_physics_static_body                             *static_body;

      static_body = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, node, crude_physics_static_body );
    
      if ( static_body->type == CRUDE_PHYSICS_BODY_SHAPE_TYPE_BOX )
      {
        model_to_custom_model = XMMatrixScaling( static_body->box.extent.x, static_body->box.extent.y, static_body->box.extent.z );
        XMStoreFloat4x4( &scene_renderer->physics_box_collision_model_renderer_resources_instance.model_to_world, XMMatrixMultiply( model_to_custom_model, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform ) ) ) );
        CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, scene_renderer->physics_box_collision_model_renderer_resources_instance );
      }
      else if ( static_body->type == CRUDE_PHYSICS_BODY_SHAPE_TYPE_MESH )
      {
        if ( static_body->mesh.handle.index != -1 )
        {
          crude_physics_mesh_shape_container *mesh_shape_container = crude_physics_shapes_manager_access_mesh_shape( scene_renderer->physics_shapes_manager, static_body->mesh.handle );
          XMStoreFloat4x4( &mesh_shape_container->debug_model_renderer_resource_instance.model_to_world, XMMatrixMultiply( model_to_custom_model, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform ) ) ) );
          CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, mesh_shape_container->debug_model_renderer_resource_instance );
        }
      }
    }
    
    if ( CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_physics_kinematic_body ) )
    {
      crude_physics_kinematic_body                             *static_body;

      static_body = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, node, crude_physics_kinematic_body );
    
      if ( static_body->type == CRUDE_PHYSICS_BODY_SHAPE_TYPE_BOX )
      {
        model_to_custom_model = XMMatrixScaling( static_body->box.extent.x, static_body->box.extent.y, static_body->box.extent.z );
        XMStoreFloat4x4( &scene_renderer->physics_box_collision_model_renderer_resources_instance.model_to_world, XMMatrixMultiply( model_to_custom_model, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform ) ) ) );
        CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, scene_renderer->physics_box_collision_model_renderer_resources_instance );
      }
      else if ( static_body->type == CRUDE_PHYSICS_BODY_SHAPE_TYPE_MESH )
      {
        if ( static_body->mesh.handle.index != -1 )
        {
          crude_physics_mesh_shape_container *mesh_shape_container = crude_physics_shapes_manager_access_mesh_shape( scene_renderer->physics_shapes_manager, static_body->mesh.handle );
          XMStoreFloat4x4( &mesh_shape_container->debug_model_renderer_resource_instance.model_to_world, XMMatrixMultiply( model_to_custom_model, crude_transform_node_to_world( world, node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_transform ) ) ) );
          CRUDE_ARRAY_PUSH( scene_renderer->model_renderer_resoruces_instances, mesh_shape_container->debug_model_renderer_resource_instance );
        }
      }
    }
  }
#endif /* CRUDE_DEVELOP */

  if ( model_renderer_resoruces_instances_count != CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances ) )
  {
    for ( uint32 i = model_renderer_resoruces_instances_count; i < CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances ); ++i )
    {
      scene_renderer->model_renderer_resoruces_instances[ i ].unique_id = node;
    }
  }

  while ( ecs_children_next( &children_it ) )
  {
    for ( size_t i = 0; i < children_it.count; ++i )
    {
      crude_entity child = crude_entity_from_iterator( &children_it, i );
      crude_scene_renderer_register_nodes_instances_( scene_renderer, world, child );
    }
  }
}

void
crude_scene_renderer_cull_lights_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_device                                        *gpu;
  crude_camera const                                      *camera;
  crude_transform const                                   *camera_transform;
  XMMATRIX                                                 view_to_world, world_to_view, view_to_clip, clip_to_view, world_to_clip;
  float32                                                  tile_size, tile_position_x, tile_position_y;
  
  gpu = scene_renderer->gpu;

  camera = &scene_renderer->options.scene.camera;
  view_to_world = XMLoadFloat4x4( &scene_renderer->options.scene.camera_view_to_world );

  world_to_view = XMMatrixInverse( NULL, view_to_world );
  view_to_clip = crude_camera_view_to_clip( camera );
  clip_to_view = XMMatrixInverse( NULL, view_to_clip );
  world_to_clip = XMMatrixMultiply( world_to_view, view_to_clip );
  
  CRUDE_ARRAY_SET_LENGTH( scene_renderer->culled_lights, 0 );
  
  tile_size = 1024.f / CRUDE_GFX_TETRAHEDRON_SHADOWMAP_SIZE;
  
  tile_position_x = 0.f;
  tile_position_y = 0.f;

  for ( uint32 light_index = 0; light_index < CRUDE_ARRAY_LENGTH( scene_renderer->lights ); ++light_index )
  {
    crude_gfx_light_cpu                               *light;
    XMVECTOR                                           light_world_position, light_view_position;
    XMVECTOR                                           aabb, aabb_screen;
    float32                                            aabb_screen_min_x, aabb_screen_max_x, aabb_screen_min_y, aabb_screen_max_y;
    float32                                            aabb_screen_width, aabb_screen_height, aabb_screen_width_cropped, aabb_screen_height_cropped;
    float32                                            light_view_position_length, light_radius;
    bool                                               camera_inside, camera_visible, ty_camera_inside, tx_camera_inside;
  
    light = &scene_renderer->lights[ light_index ];
  
    /* Transform light in camera space */
    light_world_position = XMVectorSet( light->translation.x, light->translation.y, light->translation.z, 1.0f );
    light_radius = light->light.radius;
  
    light_view_position = XMVector4Transform( light_world_position, world_to_view );
#if CRUDE_RIGHT_HAND
    light_view_position = XMVectorSetZ( light_view_position, -1.f * XMVectorGetZ( light_view_position ) );
#endif
    camera_visible = -XMVectorGetZ( light_view_position ) - light_radius < camera->near_z;
  
    if ( !camera_visible )
    {
      continue;
    }
  
    aabb = crude_compute_projected_sphere_aabb( light_world_position, light_radius, world_to_view, view_to_clip, camera->near_z );
  
    light_view_position_length = XMVectorGetX( XMVector3Length( light_view_position ) );
    camera_inside = ( light_view_position_length - light_radius ) < camera->near_z;
  
    if ( camera_inside )
    {
      aabb = { -1,-1, 1, 1 };
    }

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

    aabb_screen_width_cropped = aabb_screen_max_x - aabb_screen_min_x;
    aabb_screen_height_cropped = aabb_screen_max_y - aabb_screen_min_y;

    crude_gfx_culled_light_cpu culled_light_cpu;
    culled_light_cpu.light_index = light_index;
    culled_light_cpu.screen_area = aabb_screen_width_cropped * aabb_screen_height_cropped;
    culled_light_cpu.tile_position.x = tile_position_x;
    culled_light_cpu.tile_position.y = tile_position_y;
    culled_light_cpu.tile_size = tile_size;
    CRUDE_ARRAY_PUSH( scene_renderer->culled_lights, culled_light_cpu );

    tile_position_x += tile_size;

    if ( tile_position_x > 0.99 )
    {
      tile_position_x = 0;
      tile_position_y += tile_size;
    }
  }
}

#if CRUDE_GFX_RAY_TRACING_ENABLED
void
crude_gfx_scene_renderer_create_top_level_acceleration_structure_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  VkAccelerationStructureInstanceKHR                      *vk_acceleration_structure_instances;
  VkAccelerationStructureBuildRangeInfoKHR                 vk_acceleration_structure_build_range_info;
  VkAccelerationStructureCreateInfoKHR                     vk_acceleration_structure_create_info;
  VkAccelerationStructureBuildSizesInfoKHR                 vk_acceleration_structure_build_sizes_info;
  VkAccelerationStructureBuildGeometryInfoKHR              vk_acceleration_build_geometry_info;
  VkAccelerationStructureGeometryKHR                       vk_acceleration_structure_geometry;
  uint64                                                   temporary_allocator_marker;
  uint32                                                   max_instance_count;

  temporary_allocator_marker = crude_stack_allocator_get_marker( scene_renderer->temporary_allocator );

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( vk_acceleration_structure_instances, scene_renderer->total_visible_meshes_instances_count, crude_stack_allocator_pack( scene_renderer->temporary_allocator ) );

  for ( uint32 model_instance_index = 0u; model_instance_index < CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances ); ++model_instance_index )
  {
    crude_gfx_model_renderer_resources_instance const     *model_renderer_resources_instance;
    crude_gfx_model_renderer_resources const              *model_renderer_resources;
  
    model_renderer_resources_instance = &scene_renderer->model_renderer_resoruces_instances[ model_instance_index ];
    model_renderer_resources = crude_gfx_model_renderer_resources_manager_access_model_renderer_resources( scene_renderer->model_renderer_resources_manager, model_renderer_resources_instance->model_renderer_resources_handle );
  
    if ( !model_renderer_resources->rtx_affected )
    {
      continue;
    }

    for ( uint32 node_index = 0u; node_index < CRUDE_ARRAY_LENGTH( model_renderer_resources->nodes ); ++node_index )
    {
      crude_gfx_node                                      *node;
      XMMATRIX                                             mesh_to_model, model_to_world, mesh_to_world;

      node = &model_renderer_resources->nodes[ node_index ];
        
      if ( node->meshes )
      {
        mesh_to_model = crude_gfx_node_to_model( model_renderer_resources->nodes, model_renderer_resources_instance->nodes_transforms, node_index );
        model_to_world = XMLoadFloat4x4( &model_renderer_resources_instance->model_to_world );
        mesh_to_world = XMMatrixMultiply( mesh_to_model, model_to_world );
      }

      /* Ignore skin animated meshes for rtx... i will figure out something in the future */
      if ( node->meshes && node->skin == -1 )
      {
        for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARRAY_LENGTH( node->meshes ); ++mesh_index )
        {
          crude_gfx_mesh_cpu                             *cpu_mesh;
          crude_gfx_mesh_instance_draw                   *gpu_meshe_instance_draw;
          VkAccelerationStructureInstanceKHR              vk_acceleration_structure_instance;
          VkAccelerationStructureDeviceAddressInfoKHR     vk_acceleration_structure_address_info;
          VkDeviceAddress                                 vk_blas_address;
          VkTransformMatrixKHR                            vk_transform; 

          cpu_mesh = &model_renderer_resources->meshes[ node->meshes[ mesh_index ] ];

          for ( int32 y = 0; y < 3; ++y )
          {
            for ( int32 x = 0; x < 4; ++x )
            {
              vk_transform.matrix[ y ][ x ] = XMVectorGetByIndex( mesh_to_world.r[ x ], y );
            }
          }

          vk_acceleration_structure_address_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureDeviceAddressInfoKHR );
          vk_acceleration_structure_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
          vk_acceleration_structure_address_info.accelerationStructure = model_renderer_resources->vk_blases[ mesh_index ];

          vk_blas_address = scene_renderer->gpu->vkGetAccelerationStructureDeviceAddressKHR( scene_renderer->gpu->vk_device, &vk_acceleration_structure_address_info );
    
          vk_acceleration_structure_instance = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureInstanceKHR );
          vk_acceleration_structure_instance.mask = 0xff;
          vk_acceleration_structure_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR; /* TODO ??? */
          vk_acceleration_structure_instance.accelerationStructureReference = vk_blas_address;
          vk_acceleration_structure_instance.transform = vk_transform;

          CRUDE_ARRAY_PUSH( vk_acceleration_structure_instances, vk_acceleration_structure_instance );
        }
      }
    }
  }

  max_instance_count = CRUDE_ARRAY_LENGTH( vk_acceleration_structure_instances );

  scene_renderer->tlas_instances_hga = crude_gfx_memory_allocate_with_name(
    scene_renderer->gpu,
    max_instance_count * sizeof( VkAccelerationStructureInstanceKHR ),
    CRUDE_GFX_MEMORY_TYPE_CPU_GPU, /* !TODO try gpu only and check if it affect perfomance*/
    "tlas_instances_hga",
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR );

  crude_memory_copy( scene_renderer->tlas_instances_hga.cpu_address, vk_acceleration_structure_instances, max_instance_count * sizeof( VkAccelerationStructureInstanceKHR ) );

  vk_acceleration_structure_geometry = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureGeometryKHR );
  vk_acceleration_structure_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  vk_acceleration_structure_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  vk_acceleration_structure_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
  vk_acceleration_structure_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  vk_acceleration_structure_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
  vk_acceleration_structure_geometry.geometry.instances.data.deviceAddress = scene_renderer->tlas_instances_hga.gpu_address;

  vk_acceleration_build_geometry_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildGeometryInfoKHR );
  vk_acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  vk_acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  vk_acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
  vk_acceleration_build_geometry_info.geometryCount = 1;
  vk_acceleration_build_geometry_info.pGeometries = &vk_acceleration_structure_geometry;

  vk_acceleration_structure_build_sizes_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildSizesInfoKHR );
  vk_acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  scene_renderer->gpu->vkGetAccelerationStructureBuildSizesKHR( scene_renderer->gpu->vk_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &vk_acceleration_build_geometry_info, &max_instance_count, &vk_acceleration_structure_build_sizes_info );
  
  scene_renderer->tlas_hga = crude_gfx_memory_allocate_with_name(
    scene_renderer->gpu,
    vk_acceleration_structure_build_sizes_info.accelerationStructureSize,
    CRUDE_GFX_MEMORY_TYPE_GPU,
    "tlas_hga",
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR );
  
  vk_acceleration_structure_create_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureCreateInfoKHR );
  vk_acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  vk_acceleration_structure_create_info.buffer = crude_gfx_access_buffer( scene_renderer->gpu, scene_renderer->tlas_hga.buffer_handle )->vk_buffer;
  vk_acceleration_structure_create_info.size = vk_acceleration_structure_build_sizes_info.accelerationStructureSize;
  vk_acceleration_structure_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

  scene_renderer->gpu->vkCreateAccelerationStructureKHR( scene_renderer->gpu->vk_device, &vk_acceleration_structure_create_info, scene_renderer->gpu->vk_allocation_callbacks, &scene_renderer->vk_tlas );

  scene_renderer->tlas_scratch_hga = crude_gfx_memory_allocate_with_name(
    scene_renderer->gpu, 
    vk_acceleration_structure_build_sizes_info.buildScratchSize,
    CRUDE_GFX_MEMORY_TYPE_GPU,
    "tlas_scratch_hga", VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR );

  vk_acceleration_build_geometry_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildGeometryInfoKHR );
  vk_acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  vk_acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  vk_acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
  vk_acceleration_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  vk_acceleration_build_geometry_info.dstAccelerationStructure = scene_renderer->vk_tlas;
  vk_acceleration_build_geometry_info.geometryCount = 1;
  vk_acceleration_build_geometry_info.pGeometries = &vk_acceleration_structure_geometry;
  vk_acceleration_build_geometry_info.scratchData.deviceAddress = scene_renderer->tlas_scratch_hga.gpu_address;
  
  vk_acceleration_structure_build_range_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildRangeInfoKHR );
  vk_acceleration_structure_build_range_info.primitiveCount = CRUDE_ARRAY_LENGTH( vk_acceleration_structure_instances );
  vk_acceleration_structure_build_range_info.primitiveOffset = 0;
  vk_acceleration_structure_build_range_info.firstVertex = 0;
  vk_acceleration_structure_build_range_info.transformOffset = 0;

  VkAccelerationStructureBuildRangeInfoKHR *tlas_ranges[ ] = {
    &vk_acceleration_structure_build_range_info
  };
  
  scene_renderer->gpu->vkCmdBuildAccelerationStructuresKHR( primary_cmd->vk_cmd_buffer, 1, &vk_acceleration_build_geometry_info, tlas_ranges );

  crude_stack_allocator_free_marker( scene_renderer->temporary_allocator, temporary_allocator_marker );
}

void
crude_gfx_scene_renderer_update_top_level_acceleration_structure_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  crude_gfx_buffer                                        *tlas_buffer;
  VkAccelerationStructureInstanceKHR                      *vk_acceleration_structure_instances;
  VkAccelerationStructureBuildRangeInfoKHR                 vk_acceleration_structure_build_range_info;
  VkAccelerationStructureBuildSizesInfoKHR                 vk_acceleration_structure_build_sizes_info;
  VkAccelerationStructureBuildGeometryInfoKHR              vk_acceleration_build_geometry_info;
  VkAccelerationStructureGeometryKHR                       vk_acceleration_structure_geometry;
  uint64                                                   temporary_allocator_marker;
  uint32                                                   max_instance_count;
  
  temporary_allocator_marker = crude_stack_allocator_get_marker( scene_renderer->temporary_allocator );

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( vk_acceleration_structure_instances, scene_renderer->total_visible_meshes_instances_count, crude_stack_allocator_pack( scene_renderer->temporary_allocator ) );

  for ( uint32 model_instance_index = 0u; model_instance_index < CRUDE_ARRAY_LENGTH( scene_renderer->model_renderer_resoruces_instances ); ++model_instance_index )
  {
    crude_gfx_model_renderer_resources_instance const     *model_renderer_resources_instance;
    crude_gfx_model_renderer_resources const              *model_renderer_resources;
  
    model_renderer_resources_instance = &scene_renderer->model_renderer_resoruces_instances[ model_instance_index ];
    model_renderer_resources = crude_gfx_model_renderer_resources_manager_access_model_renderer_resources( scene_renderer->model_renderer_resources_manager, model_renderer_resources_instance->model_renderer_resources_handle );
  
    if ( !model_renderer_resources->rtx_affected )
    {
      continue;
    }

    for ( uint32 node_index = 0u; node_index < CRUDE_ARRAY_LENGTH( model_renderer_resources->nodes ); ++node_index )
    {
      crude_gfx_node                                      *node;
      XMMATRIX                                             mesh_to_model, model_to_world, mesh_to_world;

      node = &model_renderer_resources->nodes[ node_index ];
        
      if ( node->meshes )
      {
        mesh_to_model = crude_gfx_node_to_model( model_renderer_resources->nodes, model_renderer_resources_instance->nodes_transforms, node_index );
        model_to_world = XMLoadFloat4x4( &model_renderer_resources_instance->model_to_world );
        mesh_to_world = XMMatrixMultiply( mesh_to_model, model_to_world );
      }

      /* Ignore skin animated meshes for rtx... i will figure out something in the future */
      if ( node->meshes && node->skin == -1 )
      {
        for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARRAY_LENGTH( node->meshes ); ++mesh_index )
        {
          crude_gfx_mesh_cpu                             *cpu_mesh;
          crude_gfx_mesh_instance_draw                   *gpu_meshe_instance_draw;
          VkAccelerationStructureInstanceKHR              vk_acceleration_structure_instance;
          VkAccelerationStructureDeviceAddressInfoKHR     vk_acceleration_structure_address_info;
          VkDeviceAddress                                 vk_blas_address;
          VkTransformMatrixKHR                            vk_transform; 

          cpu_mesh = &model_renderer_resources->meshes[ node->meshes[ mesh_index ] ];
          
          for ( int32 y = 0; y < 3; ++y )
          {
            for ( int32 x = 0; x < 4; ++x )
            {
              vk_transform.matrix[ y ][ x ] = XMVectorGetByIndex( mesh_to_world.r[ x ], y );
            }
          }

          vk_acceleration_structure_address_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureDeviceAddressInfoKHR );
          vk_acceleration_structure_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
          vk_acceleration_structure_address_info.accelerationStructure = model_renderer_resources->vk_blases[ mesh_index ];

          vk_blas_address = scene_renderer->gpu->vkGetAccelerationStructureDeviceAddressKHR( scene_renderer->gpu->vk_device, &vk_acceleration_structure_address_info );
    
          vk_acceleration_structure_instance = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureInstanceKHR );
          vk_acceleration_structure_instance.mask = 0xff;
          vk_acceleration_structure_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR; /* TODO ??? */
          vk_acceleration_structure_instance.accelerationStructureReference = vk_blas_address;
          vk_acceleration_structure_instance.transform = vk_transform;

          CRUDE_ARRAY_PUSH( vk_acceleration_structure_instances, vk_acceleration_structure_instance );
        }
      }
    }
  }
  
  max_instance_count = CRUDE_ARRAY_LENGTH( vk_acceleration_structure_instances );

  crude_memory_copy( scene_renderer->tlas_instances_hga.cpu_address, vk_acceleration_structure_instances, sizeof( VkAccelerationStructureInstanceKHR ) * max_instance_count );
  
  
  vk_acceleration_structure_geometry = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureGeometryKHR );
  vk_acceleration_structure_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  vk_acceleration_structure_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  vk_acceleration_structure_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
  vk_acceleration_structure_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  vk_acceleration_structure_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
  vk_acceleration_structure_geometry.geometry.instances.data.deviceAddress = scene_renderer->tlas_instances_hga.gpu_address;

  vk_acceleration_build_geometry_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildGeometryInfoKHR );
  vk_acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  vk_acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  vk_acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
  vk_acceleration_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
  vk_acceleration_build_geometry_info.dstAccelerationStructure = scene_renderer->vk_tlas;
  vk_acceleration_build_geometry_info.srcAccelerationStructure = scene_renderer->vk_tlas;
  vk_acceleration_build_geometry_info.geometryCount = 1;
  vk_acceleration_build_geometry_info.pGeometries = &vk_acceleration_structure_geometry;
  vk_acceleration_build_geometry_info.scratchData.deviceAddress = scene_renderer->tlas_scratch_hga.gpu_address;

  vk_acceleration_structure_build_range_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildRangeInfoKHR );
  vk_acceleration_structure_build_range_info.primitiveCount = CRUDE_ARRAY_LENGTH( vk_acceleration_structure_instances );
  vk_acceleration_structure_build_range_info.primitiveOffset = 0;
  vk_acceleration_structure_build_range_info.firstVertex = 0;
  vk_acceleration_structure_build_range_info.transformOffset = 0;

  VkAccelerationStructureBuildRangeInfoKHR *tlas_ranges[ ] = {
    &vk_acceleration_structure_build_range_info
  };
  
  scene_renderer->gpu->vkCmdBuildAccelerationStructuresKHR( primary_cmd->vk_cmd_buffer, 1, &vk_acceleration_build_geometry_info, tlas_ranges );

  crude_stack_allocator_free_marker( scene_renderer->temporary_allocator, temporary_allocator_marker );
}

void
crude_gfx_scene_renderer_create_acceleration_stucture_dsl_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_descriptor_set_layout_creation                 dsl_creation;
  
  dsl_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_descriptor_set_layout_creation );
  dsl_creation.name = "acceleration_stucture_dsl";
  dsl_creation.bindless = false;
  dsl_creation.set_index = CRUDE_ACCELERATION_STRUCTURE_DESCRIPTOR_SET_INDEX;
  crude_gfx_descriptor_set_layout_creation_add_binding( &dsl_creation, CRUDE_COMPOUNT( crude_gfx_descriptor_set_layout_binding, {
    .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
    .start = CRUDE_ACCELERATION_STRUCTURE_BINDING,
    .count = 1u,
  } ) );

  scene_renderer->acceleration_stucture_dsl = crude_gfx_create_descriptor_set_layout( scene_renderer->gpu, &dsl_creation );
}

void
crude_gfx_scene_renderer_create_acceleration_stucture_ds_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  crude_gfx_descriptor_set_creation                        ds_creation;

  if ( scene_renderer->acceleration_stucture_ds.index != CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID.index )
  {
    crude_gfx_destroy_descriptor_set( scene_renderer->gpu, scene_renderer->acceleration_stucture_ds );
  }

  ds_creation = crude_gfx_descriptor_set_creation_empty();
  ds_creation.layout = scene_renderer->acceleration_stucture_dsl;
  ds_creation.name = "acceleration_stucture_dsl";

  crude_gfx_descriptor_set_creation_add_acceleration_structure( &ds_creation, scene_renderer->vk_tlas, 0u );
    
  scene_renderer->acceleration_stucture_ds = crude_gfx_create_descriptor_set( scene_renderer->gpu, &ds_creation );
}

#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */