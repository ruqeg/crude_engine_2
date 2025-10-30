#pragma once

#include <engine.h>
#include <scene/scene.h>
#include <graphics/scene_renderer.h>
#include <core/ecs.h>
#include <platform/platform_components.h>
#include <devgui.h>

#define CRUDE_RENDERER_ADD_TEXTURE_UPDATE_COMMANDS_THREAD_ID ( 1 )
#define CRUDE_RESOURCES_PATH "\\..\\..\\resources\\"
#define CRUDE_SHADERS_PATH "\\..\\..\\shaders\\"

typedef struct game_t
{
  /* Context */
  crude_engine                                            *engine;
  /* Common */
  crude_heap_allocator                                     allocator;
  crude_heap_allocator                                     resources_allocator;
  crude_heap_allocator                                     cgltf_temporary_allocator;
  crude_stack_allocator                                    temporary_allocator;
  /* Graphics */
  crude_gfx_device                                         gpu;
  crude_gfx_render_graph                                   render_graph;
  crude_gfx_render_graph_builder                           render_graph_builder;
  crude_gfx_asynchronous_loader                            async_loader;
  crude_gfx_scene_renderer                                 scene_renderer;
  void                                                    *imgui_context;
  /* Dev */
  crude_devgui                                             devgui;
  /* Scene */
  crude_scene                                              scene;
  /* Window & Input */
  crude_entity                                             platform_node;
  XMFLOAT2                                                 last_unrelative_mouse_position;
  /* Game */
  crude_entity                                             focused_camera_node;
  crude_entity                                             character_controller_camera;
  /* Other */
  uint32                                                   framerate;
  float32                                                  last_graphics_update_time;
} game_t;

CRUDE_API void
game_initialize
(
  _In_ game_t                                             *game,
  _In_ crude_engine                                       *engine
);

CRUDE_API void
game_update
(
  _In_ game_t                                             *game
);

CRUDE_API void
game_deinitialize
(
  _In_ game_t                                             *game
);

CRUDE_API void
game_reload_scene
(
  _In_ game_t                                             *game,
  _In_ char const                                         *filename
);