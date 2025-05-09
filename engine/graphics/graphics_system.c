#include <core/file.h>
#include <core/string.h>
#include <core/array.h>
#include <core/profiler.h>

#include <platform/platform_components.h>
#include <graphics/graphics_components.h>
#include <graphics/command_buffer.h>
#include <graphics/renderer_resources_loader.h>
#include <graphics/renderer_scene.h>

#include <scene/scene_components.h>
#include <scene/scripts_components.h>
#include <scene/free_camera_system.h>

#include <graphics/graphics_system.h>


/************************************************
 *
 * Graphics Struct
 * 
 ***********************************************/
typedef struct crude_gfx_graphics
{
  crude_gfx_device                                          gpu;
  crude_gfx_renderer                                        renderer;
  crude_gfx_render_graph                                    render_graph;
  crude_gfx_render_graph_builder                            render_graph_builder;
  crude_gfx_asynchronous_loader                             async_loader;
  crude_gfx_renderer_scene                                  scene;
  crude_allocator_container                                 allocator_container;
  enkiPinnedTask                                           *async_load_task;
  bool                                                      async_loader_execute;
  enkiTaskScheduler                                        *task_sheduler;
  crude_entity                                              camera;
} crude_gfx_graphics;

/************************************************
 *
 * Graphics Functions Declaration
 * 
 ***********************************************/
static void
graphics_initialize_
(
  ecs_iter_t *it
);

static void
graphics_deinitialize_
(
  ecs_iter_t *it
);

static void
graphics_process_
(
  ecs_iter_t *it
);

/************************************************
 *
 * Graphics Functions Implementation
 * 
 ***********************************************/
static void
pinned_task_asyns_loop_
(
  _In_ void                                                *ctx
)
{
  CRUDE_PROFILER_SET_THREAD_NAME( "AsynchronousLoaderThread" );

  crude_gfx_graphics *graphics = ( crude_gfx_graphics* )ctx;
  while ( graphics->async_loader_execute )
  {
    crude_gfx_asynchronous_loader_update( &graphics->async_loader );
  }
}

static void
graphics_initialize_
(
  ecs_iter_t *it
)
{
  crude_gfx_graphics_creation *graphics_creation_per_entity = ecs_field( it, crude_gfx_graphics_creation, 0 );
  crude_window_handle *window_handle_per_entity = ecs_field( it, crude_window_handle, 1 );

  for ( size_t i = 0; i < it->count; ++i )
  {
    crude_gfx_graphics_creation                           *graphics_creation;
    crude_gfx_graphics                                    *graphics;
    crude_window_handle                                   *window_handle;
    crude_entity                                           entity;
    uint32                                                 temporary_allocator_marker;

    graphics_creation = &graphics_creation_per_entity[ i ];
    window_handle = &window_handle_per_entity[ i ];
    entity = ( crude_entity ){ it->entities[ i ], it->world };
    
    temporary_allocator_marker = crude_stack_allocator_get_marker( graphics_creation->temporary_allocator );

    // Ensure Graphics Component
    {
      crude_gfx_graphics_handle *graphics_handle = CRUDE_ENTITY_GET_OR_ADD_COMPONENT( entity, crude_gfx_graphics_handle );
      graphics_handle->value = CRUDE_ALLOCATE( graphics_creation->allocator_container, sizeof( crude_gfx_graphics ) );
      graphics = ( crude_gfx_graphics* )graphics_handle->value;
    }
    
    graphics->allocator_container = graphics_creation->allocator_container;
    graphics->task_sheduler = graphics_creation->task_sheduler;

    /* Create Device */
    {
      crude_gfx_device_creation device_creation = {
        .sdl_window             = window_handle->value,
        .vk_application_name    = "CrudeEngine",
        .vk_application_version = VK_MAKE_VERSION( 1, 0, 0 ),
        .allocator_container    = graphics_creation->allocator_container,
        .queries_per_frame      = 1u,
        .num_threads            = enkiGetNumTaskThreads( graphics_creation->task_sheduler ),
        .temporary_allocator    = graphics_creation->temporary_allocator
      };
      crude_gfx_device_initialize( &graphics->gpu, &device_creation );
    }
    
    /* Create Renderer */
    {
      crude_gfx_renderer_creation renderer_creation = {
        .allocator_container = graphics_creation->allocator_container,
        .gpu                 = &graphics->gpu
      };
      crude_gfx_renderer_initialize( &graphics->renderer, &renderer_creation );
    }    
    
    /* Create Render Graph */
    crude_gfx_render_graph_builder_initialize( &graphics->render_graph_builder, &graphics->gpu );
    crude_gfx_render_graph_initialize( &graphics->render_graph, &graphics->render_graph_builder );
    
    /* Create Async Loader */
    {
      struct enkiTaskSchedulerConfig                       config;
      
      crude_gfx_asynchronous_loader_initialize( &graphics->async_loader, &graphics->renderer );
      graphics->async_loader_execute = true;

      config = enkiGetTaskSchedulerConfig( graphics_creation->task_sheduler );
      graphics->async_load_task = enkiCreatePinnedTask( graphics_creation->task_sheduler, pinned_task_asyns_loop_, config.numTaskThreadsToCreate );
      enkiAddPinnedTaskArgs( graphics_creation->task_sheduler, graphics->async_load_task, graphics );
    }

    /* Parse */
    {
      crude_string_buffer                                  temporary_name_buffer;
      crude_allocator_container                            temporary_allocator_container;
      char                                                 working_directory[ 512 ];
      
      crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
      temporary_allocator_container = crude_stack_allocator_pack( graphics_creation->temporary_allocator );
      crude_string_buffer_initialize( &temporary_name_buffer, 1024, temporary_allocator_container );

      /* Parse render graph*/
      {
        char const *render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", working_directory, "\\..\\..\\resources\\render_graph.json" );
        crude_gfx_render_graph_parse_from_file( &graphics->render_graph, render_graph_file_path, graphics_creation->temporary_allocator );
      }

      crude_gfx_render_graph_compile( &graphics->render_graph );
      
      /* Parse gltf*/
      {
        crude_gfx_renderer_scene_creation scene_creation = {
          .task_scheduler = graphics->task_sheduler,
          .allocator_container = graphics->allocator_container,
          .async_loader = &graphics->async_loader,
          .renderer = &graphics->renderer
        };
        crude_gfx_renderer_scene_initialize( &graphics->scene, &scene_creation );

        char const *gltf_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", working_directory, "\\..\\..\\resources\\glTF-Sample-Models\\2.0\\Sponza\\glTF\\Sponza.gltf" );
        crude_gfx_renderer_scene_load_from_file( &graphics->scene, gltf_path, graphics_creation->temporary_allocator );
      }
    }

    crude_gfx_renderer_technique_load_from_file( "\\..\\..\\resources\\render_technique.json", &graphics->renderer, &graphics->render_graph, graphics_creation->temporary_allocator );
    
    crude_gfx_renderer_scene_register_render_passes( &graphics->scene, &graphics->render_graph );
    
    crude_gfx_buffer_creation ubo_creation = {
      .type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC ,
      .size = sizeof( crude_gfx_shader_frame_constants ),
      .name = "ubo",
    };
    graphics->gpu.frame_buffer = crude_gfx_create_buffer( &graphics->gpu, &ubo_creation );

    /* Create free camera */
    {
      graphics->camera = crude_entity_create_empty( it->world, "camera1" );
      CRUDE_ENTITY_SET_COMPONENT( graphics->camera, crude_camera, {
        .fov_radians = CRUDE_CPI4,
        .near_z = 0.01,
        .far_z = 1000,
        .aspect_ratio = 1.0 } );
      CRUDE_ENTITY_SET_COMPONENT( graphics->camera, crude_transform, {
        .translation = { 0, 0, -5 },
        .rotation = { 0, 0, 0, 1 },
        .scale = { 1, 1, 1 }, } );
      CRUDE_ENTITY_SET_COMPONENT( graphics->camera, crude_free_camera, {
        .moving_speed_multiplier = { 7.0, 7.0, 7.0 },
        .rotating_speed_multiplier  = { -0.15f, -0.15f },
        .entity_input = ( crude_entity ){ it->entities[ i ], it->world } } );
    }

    crude_gfx_renderer_scene_prepare_draws( &graphics->scene, graphics_creation->temporary_allocator );

    crude_stack_allocator_free_marker( graphics_creation->temporary_allocator, temporary_allocator_marker );
  }
}

static void
graphics_deinitialize_
(
  ecs_iter_t *it
)
{
  crude_gfx_graphics_handle *graphics_per_entity = ecs_field( it, crude_gfx_graphics_handle, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_gfx_graphics                                        *graphics;

    graphics = ( crude_gfx_graphics* )graphics_per_entity[ i ].value;
    
    vkDeviceWaitIdle( graphics->gpu.vk_device );
    crude_gfx_destroy_buffer( &graphics->gpu, graphics->gpu.frame_buffer );
    crude_gfx_renderer_scene_deinitialize( &graphics->scene );
    graphics->async_loader_execute = false;
    crude_gfx_asynchronous_loader_deinitialize( &graphics->async_loader );
    crude_gfx_render_graph_deinitialize( &graphics->render_graph );
    crude_gfx_render_graph_builder_deinitialize( &graphics->render_graph_builder );
    crude_gfx_renderer_deinitialize( &graphics->renderer );
    crude_gfx_device_deinitialize( &graphics->gpu );
    CRUDE_DEALLOCATE( graphics->allocator_container, graphics );
  }
}

static void
graphics_process_
(
  ecs_iter_t *it
)
{
  crude_gfx_graphics_handle *graphics_per_entity = ecs_field( it, crude_gfx_graphics_handle, 0 );

  CRUDE_PROFILER_ZONE_NAME( "Rendering" );
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_gfx_graphics                                        *graphics;

    graphics = ( crude_gfx_graphics* )graphics_per_entity[ i ].value;

    crude_gfx_new_frame( &graphics->gpu );
  
    if ( graphics->gpu.swapchain_resized_last_frame )
    {
      crude_gfx_render_graph_on_resize( &graphics->render_graph, graphics->gpu.vk_swapchain_width, graphics->gpu.vk_swapchain_height );
    }

    // update fame buffer
    crude_gfx_map_buffer_parameters constant_buffer_map = { graphics->gpu.frame_buffer, 0, 0 };
    crude_gfx_shader_frame_constants *frame_buffer_data = crude_gfx_map_buffer( &graphics->gpu, &constant_buffer_map );
    if ( frame_buffer_data )
    {
      CRUDE_PROFILER_ZONE_NAME( "UpdateFrameBuffer" );
      crude_camera const *camera =  CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( graphics->camera, crude_camera );
      crude_transform const *transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( graphics->camera, crude_transform );
    
      crude_matrix world_to_view = crude_mat_inverse( NULL, crude_transform_node_to_world( graphics->camera, transform ) );
      crude_matrix view_to_clip = crude_camera_view_to_clip( camera );
      
      crude_store_float4x4a( &frame_buffer_data->world_to_view, world_to_view ); 
      crude_store_float4x4a( &frame_buffer_data->view_to_clip, view_to_clip ); 
      crude_gfx_unmap_buffer( &graphics->gpu, graphics->gpu.frame_buffer );
      CRUDE_PROFILER_END;
    }
    
    // update mesh buffer
    CRUDE_PROFILER_ZONE_NAME( "UpdateMeshBuffer" );
    for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARRAY_LENGTH( graphics->scene.meshes ); ++mesh_index )
    {
      crude_gfx_mesh *mesh_draw = &graphics->scene.meshes[ mesh_index ];
      
      constant_buffer_map.buffer = mesh_draw->material_buffer;
      
      crude_gfx_shader_mesh_constants *mesh_data = crude_gfx_map_buffer( &graphics->gpu, &constant_buffer_map );
      if ( mesh_data )
      {
        mesh_data->textures.x = mesh_draw->albedo_texture_index;
        mesh_data->textures.y = mesh_draw->roughness_texture_index;
        mesh_data->textures.z = mesh_draw->normal_texture_index;
        mesh_data->textures.w = mesh_draw->occlusion_texture_index;
        mesh_data->base_color_factor = ( crude_float4a ){ mesh_draw->base_color_factor.x, mesh_draw->base_color_factor.y, mesh_draw->base_color_factor.z, mesh_draw->base_color_factor.w } ;
        mesh_data->metallic_roughness_occlusion_factor = ( crude_float3a ){ mesh_draw->metallic_roughness_occlusion_factor.x, mesh_draw->metallic_roughness_occlusion_factor.y, mesh_draw->metallic_roughness_occlusion_factor.z };
        mesh_data->alpha_cutoff.x = mesh_draw->alpha_cutoff;
        mesh_data->flags.x = mesh_draw->flags;
        
        crude_transform model_transform = {
          .translation = { 0.0, 0.0, -4.0 },
          .rotation = { 0.0, 0.0, 0.0, 0.0 },
          .scale = { 0.005,0.005,0.005 },
        };
        crude_matrix model_to_world = crude_transform_node_to_world( graphics->camera, &model_transform );
        crude_store_float4x4a( &mesh_data->modelToWorld, model_to_world ); 
      
        crude_gfx_unmap_buffer( &graphics->gpu, mesh_draw->material_buffer );
      }
    }
    CRUDE_PROFILER_END;
    
    crude_gfx_renderer_scene_submit_draw_task( &graphics->scene, graphics->task_sheduler, true );
    
    {
      crude_gfx_render_graph_node *final_render_graph_node = crude_gfx_render_graph_builder_access_node_by_name( &graphics->render_graph_builder, "geometry_pass" );
      crude_gfx_framebuffer *final_render_framebuffer = crude_gfx_access_framebuffer( &graphics->gpu, final_render_graph_node->framebuffer );
      crude_gfx_texture *final_render_texture = crude_gfx_access_texture( &graphics->gpu, final_render_framebuffer->color_attachments[ 0 ] );
      crude_gfx_present( &graphics->gpu, final_render_texture );
    }
  }
  CRUDE_PROFILER_END;
}

void
crude_graphics_systemImport
(
  ecs_world_t *world
)
{
  ECS_MODULE( world, crude_graphics_system );
  ECS_IMPORT( world, crude_platform_components );
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_graphics_components );
 
  ecs_observer( world, {
    .query.terms = { 
      ( ecs_term_t ) { .id = ecs_id( crude_gfx_graphics_creation ), .oper = EcsAnd },
      ( ecs_term_t ) { .id = ecs_id( crude_window_handle ), .oper = EcsAnd },
      ( ecs_term_t ) { .id = ecs_id( crude_gfx_graphics_handle ), .oper = EcsNot }
    },
    .events = { EcsOnSet },
    .callback = graphics_initialize_
    });
  ecs_observer( world, {
    .query.terms = { { .id = ecs_id( crude_gfx_graphics_handle ) } },
    .events = { EcsOnRemove },
    .callback = graphics_deinitialize_
    } );
  ecs_system( world, {
    .entity = ecs_entity( world, { .name = "GraphicsSystem", .add = ecs_ids( ecs_dependson( EcsOnStore ) ) } ),
    .callback = graphics_process_,
    .query.terms = { 
      {.id = ecs_id( crude_gfx_graphics_handle ) }
    } } );
}
