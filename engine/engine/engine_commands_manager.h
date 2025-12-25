#pragma once

#include <engine/core/alias.h>
#include <engine/core/memory.h>

typedef struct crude_engine crude_engine;

typedef enum crude_engine_commands_manager_queue_command_type
{
  CRUDE_ENGINE_COMMANDS_MANAGER_QUEUE_COMMAND_TYPE_LOAD_SCENE,
  CRUDE_ENGINE_COMMANDS_MANAGER_QUEUE_COMMAND_TYPE_RELOAD_SCENE,
  CRUDE_ENGINE_COMMANDS_MANAGER_QUEUE_COMMAND_TYPE_RELOAD_TECHNIQUES,
  CRUDE_ENGINE_COMMANDS_MANAGER_QUEUE_COMMAND_TYPE_COUNT,
} crude_engine_commands_manager_queue_command_type;

typedef struct crude_engine_commands_manager_queue_command
{
  crude_engine_commands_manager_queue_command_type         type;
  union
  {
    struct 
    {
      char const                                          *absolute_filepath;
    } load_scene;
    struct 
    {
    } reload_scene;
    struct 
    {
    } reload_techniques;
  };
} crude_engine_commands_manager_queue_command;

typedef struct crude_engine_commands_manager
{
  crude_engine                                            *engine;
  crude_engine_commands_manager_queue_command             *commands_queue;
} crude_engine_commands_manager;

CRUDE_API void
crude_engine_commands_manager_initialize
(
  _In_ crude_engine_commands_manager                      *manager,
  _In_ crude_engine                                       *engine,
  _In_ crude_heap_allocator                               *allocator
);

CRUDE_API void
crude_engine_commands_manager_deinitialize
(
  _In_ crude_engine_commands_manager                      *manager
);

CRUDE_API void
crude_engine_commands_manager_push_reload_scene_command
(
  _In_ crude_engine_commands_manager                      *manager
);

CRUDE_API void
crude_engine_commands_manager_push_load_scene_command
(
  _In_ crude_engine_commands_manager                      *manager,
  _In_ char const                                         *absolute_filepath
);

CRUDE_API void
crude_engine_commands_manager_push_reload_techniques_command
(
  _In_ crude_engine_commands_manager                      *manager
);

CRUDE_API void
crude_engine_commands_manager_update
(
  _In_ crude_engine_commands_manager                      *manager
);