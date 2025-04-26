#include <core/file.h>
#include <core/string.h>
#include <core/algorithms.h>
#include <platform/gui_components.h>
#include <graphics/render_components.h>
#include <graphics/command_buffer.h>

#include <graphics/render_system.h>

// !TODO
#include <scene/scene_components.h>
#include <scene/scripts_components.h>
#include <scene/free_camera_system.h>


void PinnedTaskRunPinnedTaskLoop( void* pArgs_ )
{
  // !TODO
  enkiTaskScheduler *ets = ( enkiTaskScheduler*) pArgs_;
  
  while( !enkiGetIsShutdownRequested( ets ) )
  {
    enkiWaitForNewPinnedTasks( ets );
    enkiRunPinnedTasks( ets );
  }
}

void PinnedTaskRunPinnedTaskLoopAsyns( void* pArgs_ )
{
  // !TODO
  while ( true )
  {
    crude_gfx_asynchronous_loader *async_loader = ( crude_gfx_asynchronous_loader*) pArgs_;
    crude_gfx_asynchronous_loader_update( async_loader );
  }
}

static void
initialize_render_core
(
  ecs_iter_t *it
)
{
  crude_render_create *render_create = ecs_field( it, crude_render_create, 0 );
  crude_window_handle *window_handle = ecs_field( it, crude_window_handle, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_renderer_component *renderer = ecs_ensure( it->world, it->entities[ i ], crude_renderer_component );
    
    renderer->ets = enkiNewTaskScheduler();
    struct enkiTaskSchedulerConfig config = enkiGetTaskSchedulerConfig( renderer->ets );
    config.numTaskThreadsToCreate += 1;
    enkiInitTaskSchedulerWithConfig( renderer->ets, config );

    crude_gfx_device_creation gpu_creation = {
      .sdl_window             = window_handle[ i ].value,
      .vk_application_name    = render_create[ i ].vk_application_name,
      .vk_application_version = render_create[ i ].vk_application_version,
      .allocator              = render_create[ i ].allocator,
      .queries_per_frame      = 1u,
      .num_threads            = enkiGetNumTaskThreads( renderer->ets ),
      .max_frames             = render_create[ i ].max_frames,
    };

    renderer->gpu = render_create[ i ].allocator.allocate( sizeof( crude_gfx_device ), 1u );
    crude_gfx_initialize_device( renderer->gpu, &gpu_creation );

    crude_gfx_renderer_creation rendere_creation = {
      .allocator = render_create[ i ].allocator,
      .gpu = renderer->gpu
    };

    renderer->renderer = render_create[ i ].allocator.allocate( sizeof( crude_gfx_renderer ), 1u );
    crude_gfx_initialize_renderer( renderer->renderer, &rendere_creation );

    renderer->async_loader = render_create[ i ].allocator.allocate( sizeof( crude_gfx_asynchronous_loader ), 1u );

    crude_gfx_initialize_asynchronous_loader( renderer->async_loader, renderer->renderer );
    
    uint32 threadNumIOTasks = config.numTaskThreadsToCreate;

    renderer->pinned_task_run_pinned_task_loop = enkiCreatePinnedTask( renderer->ets, PinnedTaskRunPinnedTaskLoop, threadNumIOTasks );
    enkiAddPinnedTaskArgs( renderer->ets, renderer->pinned_task_run_pinned_task_loop, renderer->ets );
    
    renderer->async_load_task = enkiCreatePinnedTask( renderer->ets, PinnedTaskRunPinnedTaskLoopAsyns, threadNumIOTasks );
    enkiAddPinnedTaskArgs( renderer->ets, renderer->async_load_task, renderer->async_loader );

    char gltf_path[ 1024 ];
    crude_get_current_working_directory( gltf_path, sizeof( gltf_path ) );
    crude_strcat( gltf_path, "\\..\\..\\resources\\glTF-Sample-Models\\2.0\\Sponza\\glTF\\Sponza.gltf" );
    
    crude_gfx_buffer_creation ubo_creation = {
      .type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC ,
      .size = sizeof( crude_gfx_shader_frame_constants ),
      .name = "ubo",
    };
    renderer->gpu->frame_buffer = crude_gfx_create_buffer( renderer->gpu, &ubo_creation );

    renderer->scene = render_create[ i ].allocator.allocate( sizeof( crude_gltf_scene ), 1u );
    
    crude_gltf_scene_creation gltf_creation = {
      .renderer = renderer->renderer,
      .path = gltf_path,
      .async_loader = renderer->async_loader,
    };
    crude_gltf_scene_load_from_file( renderer->scene, &gltf_creation );
    
    renderer[ i ].camera = crude_entity_create_empty( it->world, "camera1" );
    CRUDE_ENTITY_SET_COMPONENT( renderer[ i ].camera, crude_camera, {
      .fov_radians = CRUDE_CPI4,
      .near_z = 0.01,
      .far_z = 1000,
      .aspect_ratio = 1.0 } );
    CRUDE_ENTITY_SET_COMPONENT( renderer[ i ].camera, crude_transform, {
      .translation = { 0, 0, -5 },
      .rotation = { 0, 0, 0, 1 },
      .scale = { 1, 1, 1 }, } );
    
    CRUDE_ENTITY_SET_COMPONENT( renderer[ i ].camera, crude_free_camera, {
      .moving_speed_multiplier = { 7.0, 7.0, 7.0 },
      .rotating_speed_multiplier  = { -0.15f, -0.15f },
      .entity_input = ( crude_entity ){ it->entities[ i ], it->world } } );
  }
}

static void
deinitialize_render_core
(
  ecs_iter_t *it
)
{
  crude_renderer_component *renderer = ecs_field( it, crude_renderer_component, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    enkiWaitforAllAndShutdown( renderer[ i ].ets );
    enkiDeletePinnedTask( renderer[ i ].ets, renderer[ i ].pinned_task_run_pinned_task_loop );
    enkiDeleteTaskScheduler( renderer[ i ].ets );
    crude_gltf_scene_unload( renderer[ i ].scene );
    crude_gfx_destroy_buffer( renderer[ i ].gpu, renderer[ i ].gpu->frame_buffer );
    crude_gfx_deinitialize_renderer( renderer[ i ].renderer );
    crude_gfx_deinitialize_device( renderer[ i ].gpu );
    renderer[ i ].renderer->allocator.deallocate( renderer[ i ].scene );
    renderer[ i ].renderer->allocator.deallocate( renderer[ i ].renderer );
    renderer[ i ].gpu->allocator.deallocate( renderer[ i ].gpu );
  }
}

static void
render
(
  ecs_iter_t *it
)
{
  crude_renderer_component *renderer = ecs_field( it, crude_renderer_component, 0 );
  crude_window   *window_handle = ecs_field( it, crude_window, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_gfx_new_frame( renderer[ i ].gpu );

    // update fame buffer
    crude_gfx_map_buffer_parameters constant_buffer_map = { renderer[ i ].gpu->frame_buffer, 0, 0 };
    crude_gfx_shader_frame_constants *frame_buffer_data = CAST( crude_gfx_shader_frame_constants*, crude_gfx_map_buffer( renderer[ i ].gpu, &constant_buffer_map ) );
    if ( frame_buffer_data )
    {
      crude_camera const *camera =  CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( renderer[ i ].camera, crude_camera );
      crude_transform const *transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( renderer[ i ].camera, crude_transform );

      crude_matrix world_to_view = crude_mat_inverse( NULL, crude_transform_node_to_world( renderer[ i ].camera, transform ) );
      crude_matrix view_to_clip = crude_camera_view_to_clip( camera );
      
      crude_store_float4x4a( &frame_buffer_data->world_to_view, world_to_view ); 
      crude_store_float4x4a( &frame_buffer_data->view_to_clip, view_to_clip ); 
      crude_gfx_unmap_buffer( renderer[ i ].gpu, renderer[ i ].gpu->frame_buffer );
    }
    
    // update mesh buffer
    for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARR_LEN( renderer->scene->mesh_draws ); ++mesh_index )
    {
      crude_mesh_draw *mesh_draw = &renderer->scene->mesh_draws[ mesh_index ];
      
      constant_buffer_map.buffer = mesh_draw->material_buffer;
      
      crude_gfx_shader_mesh_constants *mesh_data = CAST( crude_gfx_shader_frame_constants*, crude_gfx_map_buffer( renderer[ i ].gpu, &constant_buffer_map ) );
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
        crude_matrix model_to_world = crude_transform_node_to_world( renderer[ i ].camera, &model_transform );
        crude_store_float4x4a( &mesh_data->modelToWorld, model_to_world ); 
      
        crude_gfx_unmap_buffer( renderer[ i ].gpu, mesh_draw->material_buffer );
      }
    }

    crude_gltf_scene_submit_draw_task( renderer[ i ].scene, renderer->ets, true );

    crude_gfx_present( renderer[ i ].gpu );
  }
}

void
crude_render_systemImport
(
  ecs_world_t *world
)
{
  ECS_MODULE( world, crude_render_system );
  ECS_IMPORT( world, crude_gui_components );
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_render_components );
 
  ecs_observer( world, {
    .query.terms = { 
      ( ecs_term_t ) { .id = ecs_id( crude_render_create ), .oper = EcsAnd },
      ( ecs_term_t ) { .id = ecs_id( crude_window_handle ), .oper = EcsAnd },
      ( ecs_term_t ) { .id = ecs_id( crude_renderer_component ), .oper = EcsNot }
    },
    .events = { EcsOnSet },
    .callback = initialize_render_core
    });
  ecs_observer( world, {
    .query.terms = { { .id = ecs_id( crude_renderer_component ) } },
    .events = { EcsOnRemove },
    .callback = deinitialize_render_core
    } );
  ecs_system( world, {
    .entity = ecs_entity( world, { .name = "render_system", .add = ecs_ids( ecs_dependson( EcsOnStore ) ) } ),
    .callback = render,
    .query.terms = { 
      {.id = ecs_id( crude_renderer_component ) },
      {.id = ecs_id( crude_window ) },
    } } );
}
