#if CRUDE_DEVELOP

#define DEVMENU_HEIGHT 25

#include <SDL3/SDL.h>
#include <thirdparty/nativefiledialog-extended/src/include/nfd.h>

#include <engine/core/hashmapstr.h>
#include <engine/platform/platform.h>
#include <engine/scene/scene_ecs.h>
#include <engine/scene/scene_debug_ecs.h>
#include <engine/scene/scripts/free_camera_ecs.h>
#include <engine/graphics/imgui.h>
#include <engine/engine.h>
#include <engine/core/profiler.h>

#include <engine/gui/devmenu.h>

typedef void ( *crude_gui_devmenu_option_callback_function )
(
  _In_ crude_gui_devmenu                                      *devmenu
);

typedef bool ( *crude_gui_devmenu_hotkey_pressed_callback_function )
(
  _In_ crude_input                                        *input
);

typedef struct crude_gui_devmenu_option
{
  char const                                              *name;
  crude_gui_devmenu_option_callback_function                   callback;
  crude_gui_devmenu_hotkey_pressed_callback_function           hotkey_pressed_callback;
} crude_gui_devmenu_option;

crude_gui_devmenu_option devmenu_options[ ] =
{
  {
    "Free Camera", crude_gui_devmenu_free_camera_callback, crude_gui_devmenu_free_camera_callback_hotkey_pressed_callback
  },
  {
    "Reload Techniques", crude_gui_devmenu_reload_techniques_callback, crude_gui_devmenu_reload_techniques_hotkey_pressed_callback
  },
  {
    "Memory Visual Profiler", crude_gui_devmenu_memory_visual_profiler_callback
  },
  {
    "Texture Inspector", crude_gui_devmenu_texture_inspector_callback
  },
  {
    "Render Graph", crude_gui_devmenu_render_graph_callback
  },
  {
    "GPU Pool", crude_gui_devmenu_gpu_pool_callback
  },
  {
    "Scene Renderer", crude_gui_devmenu_scene_renderer_callback
  },
  {
    "Show/Hide Collisions", crude_gui_devmenu_collisions_view_callback
  },
  {
    "Show/Hide Debug GLTF", crude_gui_devmenu_debug_gltf_view_callback
  }
};

void
crude_gui_devmenu_initialize
(
  _In_ crude_gui_devmenu                                  *devmenu,
  _In_ crude_engine                                       *engine
)
{
  devmenu->engine = engine;
  devmenu->enabled = false;
  devmenu->selected_option = 0;
  devmenu->previous_framerate = 0.f;
  devmenu->current_framerate = 0.f;
  devmenu->dev_heap_allocator = &engine->develop_heap_allocator;
  devmenu->dev_stack_allocator = &engine->develop_temporary_allocator;
  crude_gui_devmenu_memory_visual_profiler_initialize( &devmenu->memory_visual_profiler, devmenu );
  crude_gui_devmenu_texture_inspector_initialize( &devmenu->texture_inspector, devmenu );
  crude_gui_devmenu_render_graph_initialize( &devmenu->render_graph, devmenu );
  crude_gui_devmenu_gpu_pool_initialize( &devmenu->gpu_pool, devmenu );
  crude_gui_devmenu_scene_renderer_initialize( &devmenu->scene_renderer, devmenu );
  crude_gui_devmenu_viewport_initialize( &devmenu->viewport, devmenu );
}

void
crude_gui_devmenu_deinitialize
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  crude_gui_devmenu_memory_visual_profiler_deinitialize( &devmenu->memory_visual_profiler );
  crude_gui_devmenu_texture_inspector_deinitialize( &devmenu->texture_inspector );
  crude_gui_devmenu_render_graph_deinitialize( &devmenu->render_graph );
  crude_gui_devmenu_gpu_pool_deinitialize( &devmenu->gpu_pool );
  crude_gui_devmenu_scene_renderer_deinitialize( &devmenu->scene_renderer );
  crude_gui_devmenu_viewport_deinitialize( &devmenu->viewport );
}

void
crude_gui_devmenu_draw
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_gui_devmenu_draw" );
  
  ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
  if ( devmenu->enabled )
  {
    ImGui::SetNextWindowSize( ImVec2( devmenu->engine->gpu.renderer_size.x, DEVMENU_HEIGHT ) );
    ImGui::Begin( "Devmenu", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
    ImGui::GetIO().FontGlobalScale = 0.5f;
    for ( uint32 i = 0; i < CRUDE_COUNTOF( devmenu_options ); ++i  )
    {
      ImGui::SetCursorPos( ImVec2( i * ( 100 ), 0 ) );
      if ( ImGui::Button( devmenu_options[ i ].name, ImVec2( 100, DEVMENU_HEIGHT ) ) )
      {
        devmenu_options[ i ].callback( devmenu );
      }
      ImGui::SameLine( );
    }
    ImGui::GetIO().FontGlobalScale = 1.f;
    ImGui::End( );
  }

  //{
  //  ImGui::SetNextWindowSize( ImVec2( game->gpu.vk_swapchain_width, 50 ) );
  //  ImGui::Begin( "Overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
  //  ImGui::Text( "FPS %u", CRUDE_MIN_INT( devmenu->previous_framerate, game->framerate ) );
  //  ImGui::End( );
  //}
  
  crude_gui_devmenu_viewport_draw( &devmenu->viewport );
  crude_gui_devmenu_memory_visual_profiler_draw( &devmenu->memory_visual_profiler );
  crude_gui_devmenu_texture_inspector_draw( &devmenu->texture_inspector );
  crude_gui_devmenu_render_graph_draw( &devmenu->render_graph );
  crude_gui_devmenu_gpu_pool_draw( &devmenu->gpu_pool );
  crude_gui_devmenu_scene_renderer_draw( &devmenu->scene_renderer );
  CRUDE_PROFILER_ZONE_END;
}

void
crude_gui_devmenu_update
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  crude_gui_devmenu_memory_visual_profiler_update( &devmenu->memory_visual_profiler );
  crude_gui_devmenu_texture_inspector_update( &devmenu->texture_inspector );
  crude_gui_devmenu_render_graph_update( &devmenu->render_graph );
  crude_gui_devmenu_gpu_pool_update( &devmenu->gpu_pool );
  crude_gui_devmenu_scene_renderer_update( &devmenu->scene_renderer );
  crude_gui_devmenu_viewport_update( &devmenu->viewport );

  //if ( game->time - devmenu->last_framerate_update_time > 1.f )
  //{
  //  devmenu->previous_framerate = devmenu->current_framerate;
  //  devmenu->current_framerate = 0u;
  //  devmenu->last_framerate_update_time = game->time;
  //}
  //else
  //{
  //  devmenu->current_framerate++;
  //}
}

void
crude_gui_devmenu_handle_input
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  crude_input *input = &devmenu->engine->platform.input;

  if ( input->keys[ SDL_SCANCODE_F4 ].pressed )
  {
    devmenu->enabled = !devmenu->enabled;
    
    crude_entity player_controller_node = devmenu->engine->player_controller_node;
    //if ( devmenu->enabled )
    //{
    //  crude_platform_show_cursor( &devmenu->engine->platform );
    //}
    //else
    //{
    //  crude_platform_hide_cursor( &devmenu->engine->platform );
    //}
  }

  for ( uint32 i = 0; i < CRUDE_COUNTOF( devmenu_options ); ++i  )
  {
    if ( devmenu_options[ i ].hotkey_pressed_callback && devmenu_options[ i ].hotkey_pressed_callback( input ) )
    {
      devmenu_options[ i ].callback( devmenu );
    }
  }
}

/***********************
 * 
 * Common Commmads
 * 
 ***********************/
void
crude_gui_devmenu_debug_gltf_view_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  devmenu->engine->scene_renderer.options.debug.hide_debug_gltf = !devmenu->engine->scene_renderer.options.debug.hide_debug_gltf;
}


bool
crude_gui_devmenu_debug_gltf_view_callback_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
)
{
  return false;
}

void
crude_gui_devmenu_collisions_view_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  devmenu->engine->scene_renderer.options.debug.hide_collision = !devmenu->engine->scene_renderer.options.debug.hide_collision;
}

bool
crude_gui_devmenu_collisions_view_callback_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
)
{
  return false;
}

void
crude_gui_devmenu_free_camera_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  crude_entity player_controller_node = devmenu->engine->player_controller_node;
  crude_physics_enable_simulation( &devmenu->engine->physics, !devmenu->engine->physics.simulation_enabled );
}

bool
crude_gui_devmenu_free_camera_callback_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
)
{
  return input->keys[ SDL_SCANCODE_F1 ].pressed;
}

void
crude_gui_devmenu_reload_techniques_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  crude_engine_commands_manager_push_reload_techniques_command( &devmenu->engine->commands_manager );
}

bool
crude_gui_devmenu_reload_techniques_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
)
{
  return input->keys[ SDL_SCANCODE_LCTRL ].pressed && input->keys[ SDL_SCANCODE_G ].pressed && input->keys[ SDL_SCANCODE_R ].pressed;
}

/***********************
 * 
 * Develop Memory Visual Profiler
 * 
 ***********************/
void
crude_gui_devmenu_memory_visual_profiler_initialize
(
  _In_ crude_gui_devmenu_memory_visual_profiler           *dev_mem_profiler,
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  dev_mem_profiler->devmenu = devmenu;

  dev_mem_profiler->enabled = false;
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( dev_mem_profiler->allocators_containers, 16, crude_heap_allocator_pack( &devmenu->engine->common_allocator ) );
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_heap_allocator_pack( &devmenu->engine->cgltf_temporary_allocator ) );
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_stack_allocator_pack( &devmenu->engine->model_renderer_resources_manager_temporary_allocator ) );
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_heap_allocator_pack( &devmenu->engine->resources_allocator ) );
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_stack_allocator_pack( &devmenu->engine->temporary_allocator ) );
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_linear_allocator_pack( &devmenu->engine->render_graph.linear_allocator ) );
}

void
crude_gui_devmenu_memory_visual_profiler_deinitialize
(
  _In_ crude_gui_devmenu_memory_visual_profiler           *dev_mem_profiler
)
{
  CRUDE_ARRAY_DEINITIALIZE( dev_mem_profiler->allocators_containers );
}

void
crude_gui_devmenu_memory_visual_profiler_update
(
  _In_ crude_gui_devmenu_memory_visual_profiler           *dev_mem_profiler
)
{
}

void
crude_gui_devmenu_memory_visual_profiler_draw
(
  _In_ crude_gui_devmenu_memory_visual_profiler           *dev_mem_profiler
)
{
  if ( !dev_mem_profiler->enabled )
  {
    return;
  }

  ImGui::Begin( "Memory Visual Profiler" );

  {
    ImDrawList                                            *draw_list;
    char                                                   buf[ 128 ];
    ImVec2                                                 cursor_pos, canvas_size;
    float32                                                widget_width, widget_height, legend_width, graph_width;
    uint32                                                 rect_height;

    draw_list = ImGui::GetWindowDrawList();
    cursor_pos = ImGui::GetCursorScreenPos();
    canvas_size = ImGui::GetContentRegionAvail();
    widget_width = canvas_size.x;
    widget_height = canvas_size.y;
    
    legend_width = 250;
    graph_width = CRUDE_MIN( 400.f, fabsf( canvas_size.x - legend_width ) );
    rect_height = CRUDE_CEIL( widget_height / CRUDE_ARRAY_LENGTH( dev_mem_profiler->allocators_containers ) );
    
    crude_memory_set( buf, 0u, sizeof( buf ) );

    /* Draw Graph */
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( dev_mem_profiler->allocators_containers ); ++i )
    {
      crude_allocator_container                            allocator_container;
      char const                                          *allocator_name;
      crude_color                                          allocator_color;
      uint32                                               allocator_capacity, allocator_occupied;
      float32                                              allocator_occupied_precent;

      allocator_container = dev_mem_profiler->allocators_containers[ i ];

      switch ( crude_allocator_container_get_type( allocator_container ) )
      {
      case CRUDE_ALLOCATOR_TYPE_HEAP:
      {
        crude_heap_allocator *heap_allocator = CRUDE_CAST( crude_heap_allocator*, allocator_container.ctx );
        allocator_occupied_precent = heap_allocator->occupied / CRUDE_CAST( float32, heap_allocator->capacity );
        allocator_capacity = heap_allocator->capacity;
        allocator_occupied = heap_allocator->occupied;
        allocator_name = heap_allocator->name;
        break;
      }
      case CRUDE_ALLOCATOR_TYPE_STACK:
      {
        crude_stack_allocator *stack_allocator = CRUDE_CAST( crude_stack_allocator*, allocator_container.ctx );
        allocator_occupied_precent = stack_allocator->occupied / CRUDE_CAST( float32, stack_allocator->capacity );
        allocator_capacity = stack_allocator->capacity;
        allocator_occupied = stack_allocator->occupied;
        allocator_name = stack_allocator->name;
        break;
      }
      case CRUDE_ALLOCATOR_TYPE_LINEAR:
      {
        crude_linear_allocator *linear_allocator = CRUDE_CAST( crude_linear_allocator*, allocator_container.ctx );
        allocator_occupied_precent = linear_allocator->occupied / CRUDE_CAST( float32, linear_allocator->capacity );
        allocator_capacity = linear_allocator->capacity;
        allocator_occupied = linear_allocator->occupied;
        allocator_name = linear_allocator->name;
        break;
      }
      }

      allocator_color = crude_color_get_distinct_color( i );

      draw_list->AddRectFilled(
        { cursor_pos.x, cursor_pos.y },
        { cursor_pos.x + graph_width, cursor_pos.y + rect_height },
        crude_color_set( crude_color_r( allocator_color ), crude_color_g( allocator_color ), crude_color_b( allocator_color ), 0.5 )
      );
      draw_list->AddRectFilled(
        { cursor_pos.x, cursor_pos.y },
        { cursor_pos.x + allocator_occupied_precent * graph_width, cursor_pos.y + rect_height },
        allocator_color
      );
      draw_list->AddLine(
        { cursor_pos.x, cursor_pos.y + rect_height },
        { cursor_pos.x + graph_width, cursor_pos.y + rect_height },
        1
      );

      sprintf( buf, "%s: O %.3f MB | C %.3f MB | R %.3f", allocator_name, allocator_occupied / ( 1024.f * 1024.f ), allocator_capacity / ( 1024.f * 1024.f ), allocator_occupied_precent );
      draw_list->AddText( { cursor_pos.x + graph_width + 20, cursor_pos.y + 0.5f * rect_height }, 0xffffffff, buf );

      cursor_pos.y += rect_height;
    }
  
    ImGui::Dummy( { widget_width, canvas_size.y } );
  }
  
  ImGui::End( );
}

void
crude_gui_devmenu_memory_visual_profiler_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  devmenu->memory_visual_profiler.enabled = !devmenu->memory_visual_profiler.enabled;
}

/***********************
 * 
 * Develop Texture Inspector
 * 
 ***********************/
void
crude_gui_devmenu_texture_inspector_initialize
(
  _In_ crude_gui_devmenu_texture_inspector                *dev_texture_inspector,
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  dev_texture_inspector->devmenu = devmenu;
  dev_texture_inspector->enabled = false;
  dev_texture_inspector->texture_handle = crude_gfx_access_texture( &devmenu->engine->gpu, crude_gfx_render_graph_builder_access_resource_by_name( devmenu->engine->scene_renderer.render_graph->builder, "game_final" )->resource_info.texture.handle )->handle;
}

void
crude_gui_devmenu_texture_inspector_deinitialize
(
  _In_ crude_gui_devmenu_texture_inspector                *dev_texture_inspector
)
{
}

void
crude_gui_devmenu_texture_inspector_update
(
  _In_ crude_gui_devmenu_texture_inspector                *dev_texture_inspector
)
{
}

void
crude_gui_devmenu_texture_inspector_draw
(
  _In_ crude_gui_devmenu_texture_inspector                *dev_texture_inspector
)
{
  char const                                              *preview_texture_name;
  uint32                                                   id;
  
  if ( !dev_texture_inspector->enabled )
  {
    return;
  }

  ImGui::Begin( "Texture Inspector" );

  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( dev_texture_inspector->texture_handle ) )
  {
    ImGui::Image( CRUDE_CAST( ImTextureRef, &dev_texture_inspector->texture_handle.index ), ImGui::GetContentRegionAvail( ) );
  }

  ImGui::SetCursorPos( ImGui::GetWindowContentRegionMin( ) );
  
  preview_texture_name = "Unknown";
  id = 0;

  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( dev_texture_inspector->texture_handle ) )
  {
    crude_gfx_texture *selected_texture = crude_gfx_access_texture( &dev_texture_inspector->devmenu->engine->gpu, dev_texture_inspector->texture_handle );
    if ( selected_texture && selected_texture->name )
    {
      preview_texture_name = selected_texture->name;
    };
  }
  
  if ( ImGui::BeginCombo( "Texture ID", preview_texture_name ) )
  {
    for ( uint32 t = 0; t < dev_texture_inspector->devmenu->engine->gpu.textures.pool_size; ++t )
    {
      crude_gfx_texture                                   *texture;
      crude_gfx_texture_handle                             texture_handle;
      bool                                                 is_selected;
  
      texture_handle = CRUDE_CAST( crude_gfx_texture_handle, t );
      if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( texture_handle ) )
      {
        continue;
      }
      
      texture = crude_gfx_access_texture( &dev_texture_inspector->devmenu->engine->gpu, texture_handle );
      if ( !texture || !texture->name )
      {
        continue;
      }
      
      ImGui::PushID( id++ );
  
      is_selected = ( dev_texture_inspector->texture_handle.index == texture_handle.index );
      if ( ImGui::Selectable( texture->name ) )
      {
        dev_texture_inspector->texture_handle = texture_handle;
      }
      
      ImGui::PopID( );
      if ( is_selected )
      {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  
  ImGui::End( );
}

void
crude_gui_devmenu_texture_inspector_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  devmenu->texture_inspector.enabled = !devmenu->texture_inspector.enabled;
}


/***********************
 * 
 * Develop Render Graph
 * 
 ***********************/
void
crude_gui_devmenu_render_graph_initialize
(
  _In_ crude_gui_devmenu_render_graph                     *dev_render_graph,
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  dev_render_graph->devmenu = devmenu;
  dev_render_graph->enabled = false;
  dev_render_graph->technique_absolute_filepath[ 0 ] = 0;
  crude_gfx_render_graph_builder_initialize( &dev_render_graph->render_graph_builder, &devmenu->engine->gpu );
  crude_gfx_render_graph_initialize( &dev_render_graph->render_graph, &dev_render_graph->render_graph_builder );
}

void
crude_gui_devmenu_render_graph_deinitialize
(
  _In_ crude_gui_devmenu_render_graph                     *dev_render_graph
)
{
  crude_gfx_render_graph_builder_deinitialize( &dev_render_graph->render_graph_builder );
  crude_gfx_render_graph_deinitialize( &dev_render_graph->render_graph );
}

void
crude_gui_devmenu_render_graph_update
(
  _In_ crude_gui_devmenu_render_graph                     *dev_render_graph
)
{
}

void
crude_gui_devmenu_render_graph_draw
(
  _In_ crude_gui_devmenu_render_graph                     *dev_render_graph
)
{
  crude_gfx_render_graph                                  *render_graph;
  crude_gfx_render_graph_builder                          *render_graph_builder;
  ImGuiIO                                                 *imgui_io;
  float32                                                  window_border_size, window_rounding;

  if ( !dev_render_graph->enabled )
  {
    return;
  }

  imgui_io = &ImGui::GetIO();

  ImGui::Begin( "Technique Editor" );

  if ( ImGui::Button( "Parse From File" ) )
  {
    nfdu8filteritem_t                                       ndf_filters[ ] = { { "Crude Render Graph", "crude_render_graph" } };

    nfdu8char_t                                            *ndf_absolute_filepath;
    nfdopendialogu8args_t                                   ndf_args;
    nfdresult_t                                             ndf_result;

    ndf_args = CRUDE_COMPOUNT_EMPTY( nfdopendialogu8args_t );
    ndf_args.filterList = ndf_filters;
    ndf_args.filterCount = CRUDE_COUNTOF( ndf_filters );

    ndf_result = NFD_OpenDialogU8_With( &ndf_absolute_filepath, &ndf_args );
    if ( ndf_result == NFD_OKAY )
    {
      crude_snprintf( dev_render_graph->technique_absolute_filepath, sizeof( dev_render_graph->technique_absolute_filepath ), "%s", ndf_absolute_filepath );
      crude_gfx_render_graph_parse_from_file( &dev_render_graph->render_graph, dev_render_graph->technique_absolute_filepath, dev_render_graph->devmenu->dev_stack_allocator );
      NFD_FreePathU8( ndf_absolute_filepath );
    }
    else if ( ndf_result == NFD_CANCEL )
    {
      CRUDE_LOG_INFO( CRUDE_CHANNEL_FILEIO, "User pressed cancel!" );
    }
    else 
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_FILEIO, "Error: %s", NFD_GetError( ) );
    }
  }

  render_graph = &dev_render_graph->render_graph;
  render_graph_builder = &dev_render_graph->render_graph_builder;

  uint32 child_index = 0;
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( render_graph->nodes ); ++i )
  {
    crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( render_graph_builder, render_graph->nodes[ i ] );
    
    ImGui::PushID( i );
    if ( ImGui::TreeNode( node->name ) )
    {
      ImGui::Text( "Type: %s", node->type == CRUDE_GFX_RENDER_GRAPH_NODE_TYPE_GRAPHICS ? "Graphics" : "Compute" );
      if ( ImGui::TreeNode ( "Inputs" ) )
      {
        for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( node->inputs ); ++i )
        {
          crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( render_graph_builder, node->inputs[ i ] );
          if ( ImGui::TreeNode( resource->name ) )
          {
            ImGui::Text( "Type: %s", crude_gfx_render_graph_resource_type_to_string( resource->type ) );
            ImGui::Text( "External: %s", resource->resource_info.external ? "true" : "false" );
            ImGui::TreePop( );
            ImGui::Spacing( );
          }
        }
        ImGui::TreePop( );
        ImGui::Spacing( );
      }
      ImGui::TreePop( );
      ImGui::Spacing( );
    }
    ImGui::PopID( );
  }

  ImGui::End( );
}

void
crude_gui_devmenu_render_graph_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  devmenu->render_graph.enabled = !devmenu->render_graph.enabled;
}

/***********************
 * 
 * Develop GPU Pools
 * 
 ***********************/
static void
crude_gui_devmenu_gpu_pool_draw_pool_
(
  _In_ crude_resource_pool                                *resource_pool,
  _In_ char const                                         *resource_name,
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  ImGui::Text( "Pool %s, indices used %u, allocated %u", resource_name, resource_pool->used_indices, resource_pool->pool_size );
}

void
crude_gui_devmenu_gpu_pool_initialize
(
  _In_ crude_gui_devmenu_gpu_pool                         *dev_gpu_pool,
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  dev_gpu_pool->devmenu = devmenu;
  dev_gpu_pool->enabled = false;
}

void
crude_gui_devmenu_gpu_pool_deinitialize
(
  _In_ crude_gui_devmenu_gpu_pool                         *dev_gpu_pool
)
{
}

void
crude_gui_devmenu_gpu_pool_update
(
  _In_ crude_gui_devmenu_gpu_pool                         *dev_gpu_pool
)
{
}

void
crude_gui_devmenu_gpu_pool_draw
(
  _In_ crude_gui_devmenu_gpu_pool                         *dev_gpu_pool
)
{
  if ( !dev_gpu_pool->enabled )
  {
    return;
  }

  VkPhysicalDeviceProperties                               vk_physical_properties;
  VmaBudget                                                gpu_memory_heap_budgets[ VK_MAX_MEMORY_HEAPS ];
  uint64                                                   memory_used, memory_allocated;

  crude_memory_set( gpu_memory_heap_budgets, 0u, sizeof( gpu_memory_heap_budgets ) );
  vmaGetHeapBudgets( dev_gpu_pool->devmenu->engine->gpu.vma_allocator, gpu_memory_heap_budgets );
 
  memory_used = memory_allocated = 0;
  for ( uint32 i = 0; i < VK_MAX_MEMORY_HEAPS; ++i )
  {
    memory_used += gpu_memory_heap_budgets[ i ].usage;
    memory_allocated += gpu_memory_heap_budgets[ i ].budget;
  }
   
  vkGetPhysicalDeviceProperties( dev_gpu_pool->devmenu->engine->gpu.vk_physical_device, &vk_physical_properties );
  
  ImGui::Text( "GPU used: %s", vk_physical_properties.deviceName ? vk_physical_properties.deviceName : "Unknown" );
  ImGui::Text( "GPU Memory Used: %lluMB, Total: %lluMB", memory_used / ( 1024 * 1024 ), memory_allocated / ( 1024 * 1024 ) );
  
  ImGui::Separator();
  crude_gui_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.buffers, "Buffers", dev_gpu_pool->devmenu );
  crude_gui_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.textures, "Textures", dev_gpu_pool->devmenu );
  crude_gui_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.pipelines, "Pipelines", dev_gpu_pool->devmenu );
  crude_gui_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.samplers, "Samplers", dev_gpu_pool->devmenu );
  crude_gui_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.descriptor_sets, "DescriptorSets", dev_gpu_pool->devmenu );
  crude_gui_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.descriptor_set_layouts, "DescriptorSetLayouts", dev_gpu_pool->devmenu );
  crude_gui_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.framebuffers, "Framebuffers", dev_gpu_pool->devmenu );
  crude_gui_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.render_passes, "RenderPasses", dev_gpu_pool->devmenu );
  crude_gui_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.shaders, "Shaders", dev_gpu_pool->devmenu );
}

void
crude_gui_devmenu_gpu_pool_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  devmenu->gpu_pool.enabled = !devmenu->gpu_pool.enabled;
}

/***********************
 * 
 * Develop Scene Renderer
 * 
 ***********************/
void
crude_gui_devmenu_scene_renderer_initialize
(
  _In_ crude_gui_devmenu_scene_renderer                   *dev_scene_rendere,
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  dev_scene_rendere->devmenu = devmenu;
  dev_scene_rendere->enabled = false;
}

void
crude_gui_devmenu_scene_renderer_deinitialize
(
  _In_ crude_gui_devmenu_scene_renderer                   *dev_scene_rendere
)
{
}

void
crude_gui_devmenu_scene_renderer_update
(
  _In_ crude_gui_devmenu_scene_renderer                   *dev_scene_rendere
)
{
}

void
crude_gui_devmenu_scene_renderer_draw
(
  _In_ crude_gui_devmenu_scene_renderer                   *dev_scene_rendere
)
{
  if ( !dev_scene_rendere->enabled )
  {
    return;
  }

  ImGui::Begin( "Scene Renderer" );
  
  if ( ImGui::CollapsingHeader( "Debug" ) )
  {
    char const *debug_modes_str[] = { "None", "Lights Per Pixel" };
    int32 debug_mode = dev_scene_rendere->devmenu->engine->scene_renderer.options.debug.debug_mode;
    ImGui::Combo( "Mode", &debug_mode, debug_modes_str, IM_ARRAYSIZE( debug_modes_str ) );
    dev_scene_rendere->devmenu->engine->scene_renderer.options.debug.debug_mode = debug_mode;

    ImGui::CheckboxFlags( "Force Roughness", &dev_scene_rendere->devmenu->engine->scene_renderer.options.debug.flags1, 1 << 0 );
    ImGui::CheckboxFlags( "Force Metalness", &dev_scene_rendere->devmenu->engine->scene_renderer.options.debug.flags1, 1 << 1 );
    ImGui::DragFloat( "Force Roughness Value", &dev_scene_rendere->devmenu->engine->scene_renderer.options.debug.force_roughness, 0.01f, 0.f, 1.f );
    ImGui::DragFloat( "Force Metalness Value", &dev_scene_rendere->devmenu->engine->scene_renderer.options.debug.force_metalness, 0.01f, 0.f, 1.f );
  }

  if ( ImGui::CollapsingHeader( "Background" ) )
  {
    ImGui::ColorEdit3( "Background Color", &dev_scene_rendere->devmenu->engine->scene_renderer.options.scene.background_color.x );
    ImGui::DragFloat( "Background Intensity", &dev_scene_rendere->devmenu->engine->scene_renderer.options.scene.background_intensity, 1.f, 0.f );
  }
  
  if ( ImGui::CollapsingHeader( "SSR" ) )
  { 
    ImGui::DragFloat( "Max Steps", &dev_scene_rendere->devmenu->engine->scene_renderer.options.ssr_pass.max_steps, 1.f, 0.f );
    ImGui::DragFloat( "Max Distance", &dev_scene_rendere->devmenu->engine->scene_renderer.options.ssr_pass.max_distance, 1.f, 0.f );
    ImGui::DragFloat( "Stride zcutoff", &dev_scene_rendere->devmenu->engine->scene_renderer.options.ssr_pass.stride_zcutoff, 1.f, 0.f );
    ImGui::DragFloat( "Stride", &dev_scene_rendere->devmenu->engine->scene_renderer.options.ssr_pass.stride, 1.f, 0.f );
    ImGui::DragFloat( "Zthickness", &dev_scene_rendere->devmenu->engine->scene_renderer.options.ssr_pass.z_thickness, 1.f, 0.f );
    ImGui::DragFloat( "Fade Start", &dev_scene_rendere->devmenu->engine->scene_renderer.options.ssr_pass.fade_start, 1.f, 0.f );
    ImGui::DragFloat( "Fade End", &dev_scene_rendere->devmenu->engine->scene_renderer.options.ssr_pass.fade_end, 1.f, 0.f );
  }
  if ( ImGui::CollapsingHeader( "Global Illumination" ) )
  {
    ImGui::ColorEdit3( "Ambient Color", &dev_scene_rendere->devmenu->engine->scene_renderer.options.scene.ambient_color.x );
    ImGui::DragFloat( "Ambient Intensity", &dev_scene_rendere->devmenu->engine->scene_renderer.options.scene.ambient_intensity, 0.1f, 0.f );
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
    ImGui::DragFloat3( "Probe Grid Position", &dev_scene_renderer->scene_renderer->indirect_light_pass.options.probe_grid_position.x );
    ImGui::DragFloat3( "Probe Spacing", &dev_scene_renderer->scene_renderer->indirect_light_pass.options.probe_spacing.x );
    ImGui::DragFloat( "Max Probe Offset", &dev_scene_renderer->scene_renderer->indirect_light_pass.options.max_probe_offset );
    ImGui::DragFloat( "Self Shadow Bias", &dev_scene_renderer->scene_renderer->indirect_light_pass.options.self_shadow_bias );
    ImGui::SliderFloat( "Hysteresis", &dev_scene_renderer->scene_renderer->indirect_light_pass.options.hysteresis, 0.0, 1.0 );
    ImGui::DragFloat( "Shadow Weight Power", &dev_scene_renderer->scene_renderer->indirect_light_pass.options.shadow_weight_power );
    ImGui::SliderFloat( "Infinite Bounces Multiplier", &dev_scene_renderer->scene_renderer->indirect_light_pass.options.infinite_bounces_multiplier, 0.0, 1.0 );
    ImGui::DragInt( "Probe Update Per Frame", &dev_scene_renderer->scene_renderer->indirect_light_pass.options.probe_update_per_frame );
    ImGui::Text( "Probe Debug Flags" );
    ImGui::CheckboxFlags( "Statues | OV", &dev_scene_renderer->scene_renderer->indirect_light_pass.options.probe_debug_flags, 1 );
    ImGui::CheckboxFlags( "Radiance | OV", &dev_scene_renderer->scene_renderer->indirect_light_pass.options.probe_debug_flags, 2 );
    ImGui::CheckboxFlags( "Probe Index | OV", &dev_scene_renderer->scene_renderer->indirect_light_pass.options.probe_debug_flags, 4 );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
  }
  
  ImGui::End( );
}

void
crude_gui_devmenu_scene_renderer_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  devmenu->scene_renderer.enabled = !devmenu->scene_renderer.enabled;
}

/***********************
 * 
 * Develop Viewport
 * 
 ***********************/
void
crude_gui_devmenu_viewport_initialize
(
  _In_ crude_gui_devmenu_viewport                         *dev_viewport,
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  dev_viewport->devmenu = devmenu;
  dev_viewport->enabled = true;
}

void
crude_gui_devmenu_viewport_deinitialize
(
  _In_ crude_gui_devmenu_viewport                         *dev_viewport
)
{
}

void
crude_gui_devmenu_viewport_update
(
  _In_ crude_gui_devmenu_viewport                         *dev_viewport
)
{
}

void
crude_gui_devmenu_viewport_draw
(
  _In_ crude_gui_devmenu_viewport                         *dev_viewport
)
{
  static ImGuizmo::OPERATION                               selected_gizmo_operation = ImGuizmo::TRANSLATE;
  static ImGuizmo::MODE                                    selected_gizmo_mode = ImGuizmo::WORLD;
  
  crude_entity                                             camera_node, selected_node;
  crude_camera                                            *camera;
  crude_ecs                                               *world;
  crude_transform                                         *camera_transform;
  crude_transform                                         *selected_node_transform;
  crude_transform                                         *selected_node_parent_transform;
  crude_entity                                             selected_node_parent;
  XMFLOAT4X4                                               camera_view_to_clip, selected_node_to_parent, selected_parent_to_camera_view;
  XMVECTOR                                                 new_scale, new_translation, new_rotation_quat;
  
  ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
  ImGui::SetNextWindowSize( ImVec2( dev_viewport->devmenu->engine->gpu.renderer_size.x, dev_viewport->devmenu->engine->gpu.renderer_size.y )  );
  ImGui::Begin( "Viewport", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs );

  world = dev_viewport->devmenu->engine->world;

  camera_node = dev_viewport->devmenu->engine->camera_node;
  selected_node = dev_viewport->devmenu->nodes_tree.selected_node;
  if ( !crude_entity_valid( world, selected_node ) )
  {
    goto cleanup;
  }
  
  selected_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_transform );
  camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, camera_node, crude_camera  );
  camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, camera_node, crude_transform  );
  selected_node_parent = crude_entity_get_parent( world, selected_node );
  
  if ( selected_node_transform == NULL )
  {
    goto cleanup;
  }
  
  if ( ImGui::IsKeyPressed( ImGuiKey_Z ) )
  {
    selected_gizmo_operation = ImGuizmo::TRANSLATE;
  }
  
  if ( ImGui::IsKeyPressed( ImGuiKey_X ) )
  {
    selected_gizmo_operation = ImGuizmo::ROTATE;
  }
  if ( ImGui::IsKeyPressed( ImGuiKey_C ) ) // r Key
  {
    selected_gizmo_operation = ImGuizmo::SCALE;
  }
  
  if ( ImGui::RadioButton( "Translate", selected_gizmo_operation == ImGuizmo::TRANSLATE ) )
  {
    selected_gizmo_operation = ImGuizmo::TRANSLATE;
  }
  
  ImGui::SameLine();
  
  if ( ImGui::RadioButton( "Rotate", selected_gizmo_operation == ImGuizmo::ROTATE ) )
  {
    selected_gizmo_operation = ImGuizmo::ROTATE;
  }
  ImGui::SameLine();
  if ( ImGui::RadioButton( "Scale", selected_gizmo_operation == ImGuizmo::SCALE ) )
  {
    selected_gizmo_operation = ImGuizmo::SCALE;
  }
  
  ImGui::SetCursorPos( ImGui::GetWindowContentRegionMin( ) );
  ImGuizmo::SetDrawlist( );
  ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
  
  if ( crude_entity_valid( world, selected_node_parent ) )
  {
    selected_node_parent_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node_parent, crude_transform  );
  }
  else
  {
    selected_node_parent_transform = NULL;
  }
  
  if ( selected_node_parent_transform )
  {
    XMStoreFloat4x4( &selected_parent_to_camera_view, DirectX::XMMatrixMultiply( crude_transform_node_to_world( world, selected_node_parent, selected_node_parent_transform ), XMMatrixInverse( NULL, crude_transform_node_to_world( world, camera_node, camera_transform ) ) ) );
  }
  else
  {
    XMStoreFloat4x4( &selected_parent_to_camera_view, XMMatrixIdentity( ) );
  }
  
  XMStoreFloat4x4( &camera_view_to_clip, crude_camera_view_to_clip( camera ) );
  XMStoreFloat4x4( &selected_node_to_parent, crude_transform_node_to_parent( selected_node_transform ) );
  
  ImGuizmo::SetID( 0 );
  if ( ImGuizmo::Manipulate( &selected_parent_to_camera_view._11, &camera_view_to_clip._11, selected_gizmo_operation, selected_gizmo_mode, &selected_node_to_parent._11, NULL, NULL ) )
  {
    XMMatrixDecompose( &new_scale, &new_rotation_quat, &new_translation, XMLoadFloat4x4( &selected_node_to_parent ) );
  
    XMStoreFloat4( &selected_node_transform->rotation, new_rotation_quat );
    XMStoreFloat3( &selected_node_transform->scale, new_scale );
    XMStoreFloat3( &selected_node_transform->translation, new_translation );
  
    CRUDE_ENTITY_COMPONENT_MODIFIED( world, selected_node, crude_transform );
  }
  //ImGuizmo::DrawGrid(cameraView, cameraProjection, identityMatrix, 100.f);

cleanup:
  ImGui::End( );
}

void
crude_gui_devmenu_viewport_callback
(
  _In_ crude_gui_devmenu                                  *devmenu
)
{
  devmenu->viewport.enabled = !devmenu->viewport.enabled;
}

#endif