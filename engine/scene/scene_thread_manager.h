#pragma once

#include <engine/core/ecs.h>
#include <engine/core/task_sheduler.h>
#include <engine/platform/platform.h>

typedef struct crude_scene_thread_manager
{
  crude_input_thread_data                                 *___input_thread_data;

  crude_ecs                                               *world;
  bool                                                     running;
  mtx_t                                                    mutex;
  int64                                                    last_update_time;
  crude_input                                              input_copy;

  crude_entity                                             main_node;
  crude_entity                                             camera_node;
} crude_scene_thread_manager;

CRUDE_API void
crude_scene_thread_manager_initialize
(
  _In_ crude_scene_thread_manager                         *manager,
  _In_ crude_task_sheduler                                *task_sheduler,
  _In_ crude_input_thread_data                            *___input_thread_data
);

CRUDE_API void
crude_scene_thread_manager_deinitialize
(
  _In_ crude_scene_thread_manager                         *manager
);

CRUDE_API crude_ecs*
crude_scene_thread_manager_lock_world
(
  _In_ crude_scene_thread_manager                         *manager
);

CRUDE_API void
crude_scene_thread_manager_unlock_world
(
  _In_ crude_scene_thread_manager                         *manager
);

CRUDE_API crude_entity
crude_scene_thread_manager_get_main_node_UNSAFE
(
  _In_ crude_scene_thread_manager                         *manager
);

CRUDE_API crude_entity
crude_scene_thread_manager_get_camera_node_UNSAFE
(
  _In_ crude_scene_thread_manager                         *manager
);

CRUDE_API void
crude_scene_thread_manager_set_camera_node_UNSAFE
(
  _In_ crude_scene_thread_manager                         *manager,
  _In_ crude_entity                                        node
);

CRUDE_API void
crude_scene_thread_manager_set_main_node_UNSAFE
(
  _In_ crude_scene_thread_manager                         *manager,
  _In_ crude_entity                                        node
);

CRUDE_API crude_input const*
crude_scene_thread_manager_get_input_copy_ptr
(
  _In_ crude_scene_thread_manager                         *manager
);

void
crude_scene_thread_manager_stop
(
  _In_ crude_scene_thread_manager                         *manager
);