#include <engine/core/time.h>
#include <engine/core/profiler.h>
#include <engine/core/ecs.h>

#include <engine/scene/scene_thread_manager.h>

static void
crude_scene_thread_manager_pinned_task_ecs_loop_
(
  _In_ void                                               *ctx
);

void
crude_scene_thread_manager_initialize
(
  _In_ crude_scene_thread_manager                         *manager,
  _In_ crude_task_sheduler                                *task_sheduler,
  _In_ crude_input_thread_data                            *___input_thread_data
)
{
  manager->running = true;
  manager->last_update_time = crude_time_now();
  manager->___input_thread_data = ___input_thread_data;
  mtx_init( &manager->mutex, mtx_plain );

  manager->world = crude_ecs_create( );
  
  CRUDE_ECS_TAG_DEFINE( manager->world, crude_entity_tag );
  
  crude_ecs_set_threads( manager->world, 1 );

  crude_task_sheduler_add_pinned_task( task_sheduler, crude_scene_thread_manager_pinned_task_ecs_loop_, manager, CRUDE_ECS_ACTIVE_THREAD );
}

void
crude_scene_thread_manager_deinitialize
(
  _In_ crude_scene_thread_manager                         *manager
)
{
}

crude_ecs*
crude_scene_thread_manager_lock_world
(
  _In_ crude_scene_thread_manager                         *manager
)
{
  mtx_lock( &manager->mutex );
  return manager->world;
}

void
crude_scene_thread_manager_unlock_world
(
  _In_ crude_scene_thread_manager                         *manager
)
{
  mtx_unlock( &manager->mutex );
}

crude_entity
crude_scene_thread_manager_get_main_node_UNSAFE
(
  _In_ crude_scene_thread_manager                         *manager
)
{
  return manager->main_node;
}

crude_entity
crude_scene_thread_manager_get_camera_node_UNSAFE
(
  _In_ crude_scene_thread_manager                         *manager
)
{
  return manager->camera_node;
}

void
crude_scene_thread_manager_set_camera_node_UNSAFE
(
  _In_ crude_scene_thread_manager                         *manager,
  _In_ crude_entity                                        node
)
{
  manager->camera_node = node;
}

void
crude_scene_thread_manager_set_main_node_UNSAFE
(
  _In_ crude_scene_thread_manager                         *manager,
  _In_ crude_entity                                        node
)
{
  manager->main_node = node;
}

crude_input const*
crude_scene_thread_manager_get_input_copy_ptr
(
  _In_ crude_scene_thread_manager                         *manager
)
{
  return &manager->input_copy;
}

void
crude_scene_thread_manager_stop
(
  _In_ crude_scene_thread_manager                         *manager
)
{
  mtx_lock( &manager->mutex );
  manager->running = false;
  mtx_unlock( &manager->mutex );
}

void
crude_scene_thread_manager_pinned_task_ecs_loop_
(
  _In_ void                                               *ctx
)
{
  crude_scene_thread_manager *manager = CRUDE_REINTERPRET_CAST( crude_scene_thread_manager*, ctx );
  
  CRUDE_PROFILER_SET_THREAD_NAME( "crude_engine_pinned_task_ecs_loop_" );

  while ( manager->running )
  {
    int64 current_time = crude_time_now( );
    float64 delta_time = crude_time_delta_seconds( manager->last_update_time, current_time );
  
    mtx_lock( &manager->mutex );

    crude_input_thread_data_flush_input( manager->___input_thread_data, &manager->input_copy );

    crude_ecs_progress( manager->world, delta_time );

    mtx_unlock( &manager->mutex );
    
    manager->last_update_time = current_time;
  }
}