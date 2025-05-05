#include <core/file.h>
#include <core/string.h>
#include <core/array.h>
#include <core/profiler.h>
#include <platform/platform_components.h>
#include <graphics/graphics_components.h>
#include <graphics/command_buffer.h>

// !TODO
#include <scene/scene_components.h>
#include <scene/scripts_components.h>
#include <scene/free_camera_system.h>

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
  crude_allocator_container                                 allocator_container;
  bool                                                      async_loader_execute;

  crude_gltf_scene                                         *scene;
  crude_entity                                              camera;
  enkiTaskScheduler                                        *ets;
  enkiPinnedTask                                           *async_load_task;
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
  crude_graphics_creation *graphics_creation_per_entity = ecs_field( it, crude_graphics_creation, 0 );
  crude_window_handle *window_handle_per_entity = ecs_field( it, crude_window_handle, 1 );

  for ( size_t i = 0; i < it->count; ++i )
  {
    crude_graphics_creation                               *graphics_creation;
    crude_gfx_graphics                                        *graphics;
    crude_window_handle                                   *window_handle;
    crude_entity                                           entity;

    graphics_creation = &graphics_creation_per_entity[ i ];
    window_handle = &window_handle_per_entity[ i ];
    entity = ( crude_entity ){ it->entities[ i ], it->world };
    
    // Ensure Graphics Component
    {
      crude_graphics_handle *graphics_handle = CRUDE_ENTITY_GET_OR_ADD_COMPONENT( entity, crude_graphics_handle );
      graphics_handle->value = CRUDE_ALLOCATE( graphics_creation->allocator_container, sizeof( crude_gfx_graphics ) );
      graphics = ( crude_gfx_graphics* )graphics_handle->value;
    }
    
    graphics->allocator_container = graphics_creation->allocator_container;

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
  }
}

static void
graphics_deinitialize_
(
  ecs_iter_t *it
)
{
  crude_graphics_handle *graphics_per_entity = ecs_field( it, crude_graphics_handle, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_gfx_graphics                                        *graphics;

    graphics = ( crude_gfx_graphics* )graphics_per_entity[ i ].value;
    
    graphics->async_loader_execute = false;
    crude_gfx_asynchronous_loader_deinitialize( &graphics->async_loader );
    crude_gfx_render_graph_deinitialize( &graphics->render_graph );
    crude_gfx_render_graph_builder_deinitialize( &graphics->render_graph_builder );
    crude_gfx_renderer_deinitialize( &graphics->renderer );
    crude_gfx_device_deinitialize( &graphics->gpu );
    CRUDE_DEALLOCATE( graphics->allocator_container, graphics );
    //crude_gltf_scene_unload( renderer[ i ].scene );
    //crude_gfx_destroy_buffer( renderer[ i ].gpu, renderer[ i ].gpu->frame_buffer );
    //crude_gfx_renderer_deinitialize( renderer[ i ].renderer );
    //crude_gfx_device_deinitialize( renderer[ i ].gpu );
  }
}

static void
graphics_process_
(
  ecs_iter_t *it
)
{
  //crude_renderer_component *renderer = ecs_field( it, crude_renderer_component, 0 );
  //crude_window   *window_handle = ecs_field( it, crude_window, 1 );
  //
  //CRUDE_PROFILER_ZONE_NAME( "Rendering" );
  //for ( uint32 i = 0; i < it->count; ++i )
  //{
  //  crude_gfx_new_frame( renderer[ i ].gpu );
  //
  //  // update fame buffer
  //  crude_gfx_map_buffer_parameters constant_buffer_map = { renderer[ i ].gpu->frame_buffer, 0, 0 };
  //  crude_gfx_shader_frame_constants *frame_buffer_data = crude_gfx_map_buffer( renderer[ i ].gpu, &constant_buffer_map );
  //  if ( frame_buffer_data )
  //  {
  //    CRUDE_PROFILER_ZONE_NAME( "UpdateFrameBuffer" );
  //    crude_camera const *camera =  CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( renderer[ i ].camera, crude_camera );
  //    crude_transform const *transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( renderer[ i ].camera, crude_transform );
  //
  //    crude_matrix world_to_view = crude_mat_inverse( NULL, crude_transform_node_to_world( renderer[ i ].camera, transform ) );
  //    crude_matrix view_to_clip = crude_camera_view_to_clip( camera );
  //    
  //    crude_store_float4x4a( &frame_buffer_data->world_to_view, world_to_view ); 
  //    crude_store_float4x4a( &frame_buffer_data->view_to_clip, view_to_clip ); 
  //    crude_gfx_unmap_buffer( renderer[ i ].gpu, renderer[ i ].gpu->frame_buffer );
  //    CRUDE_PROFILER_END;
  //  }
  //  
  //  // update mesh buffer
  //  CRUDE_PROFILER_ZONE_NAME( "UpdateMeshBuffer" );
  //  for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARRAY_LENGTH( renderer->scene->mesh_draws ); ++mesh_index )
  //  {
  //    crude_mesh_draw *mesh_draw = &renderer->scene->mesh_draws[ mesh_index ];
  //    
  //    constant_buffer_map.buffer = mesh_draw->material_buffer;
  //    
  //    crude_gfx_shader_mesh_constants *mesh_data = crude_gfx_map_buffer( renderer[ i ].gpu, &constant_buffer_map );
  //    if ( mesh_data )
  //    {
  //      mesh_data->textures.x = mesh_draw->albedo_texture_index;
  //      mesh_data->textures.y = mesh_draw->roughness_texture_index;
  //      mesh_data->textures.z = mesh_draw->normal_texture_index;
  //      mesh_data->textures.w = mesh_draw->occlusion_texture_index;
  //      mesh_data->base_color_factor = ( crude_float4a ){ mesh_draw->base_color_factor.x, mesh_draw->base_color_factor.y, mesh_draw->base_color_factor.z, mesh_draw->base_color_factor.w } ;
  //      mesh_data->metallic_roughness_occlusion_factor = ( crude_float3a ){ mesh_draw->metallic_roughness_occlusion_factor.x, mesh_draw->metallic_roughness_occlusion_factor.y, mesh_draw->metallic_roughness_occlusion_factor.z };
  //      mesh_data->alpha_cutoff.x = mesh_draw->alpha_cutoff;
  //      mesh_data->flags.x = mesh_draw->flags;
  //      
  //      crude_transform model_transform = {
  //        .translation = { 0.0, 0.0, -4.0 },
  //        .rotation = { 0.0, 0.0, 0.0, 0.0 },
  //        .scale = { 0.005,0.005,0.005 },
  //      };
  //      crude_matrix model_to_world = crude_transform_node_to_world( renderer[ i ].camera, &model_transform );
  //      crude_store_float4x4a( &mesh_data->modelToWorld, model_to_world ); 
  //    
  //      crude_gfx_unmap_buffer( renderer[ i ].gpu, mesh_draw->material_buffer );
  //    }
  //  }
  //  CRUDE_PROFILER_END;
  //
  //  crude_gltf_scene_submit_draw_task( renderer[ i ].scene, renderer->ets, true );
  //
  //  crude_gfx_present( renderer[ i ].gpu );
  //}
  //CRUDE_PROFILER_END;
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
      ( ecs_term_t ) { .id = ecs_id( crude_graphics_creation ), .oper = EcsAnd },
      ( ecs_term_t ) { .id = ecs_id( crude_window_handle ), .oper = EcsAnd },
      ( ecs_term_t ) { .id = ecs_id( crude_graphics_handle ), .oper = EcsNot }
    },
    .events = { EcsOnSet },
    .callback = graphics_initialize_
    });
  ecs_observer( world, {
    .query.terms = { { .id = ecs_id( crude_graphics_handle ) } },
    .events = { EcsOnRemove },
    .callback = graphics_deinitialize_
    } );
  ecs_system( world, {
    .entity = ecs_entity( world, { .name = "GraphicsSystem", .add = ecs_ids( ecs_dependson( EcsOnStore ) ) } ),
    .callback = graphics_process_,
    .query.terms = { 
      {.id = ecs_id( crude_graphics_handle ) },
      {.id = ecs_id( crude_window ) },
    } } );
}
