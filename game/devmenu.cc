#if CRUDE_DEVELOP

#include <SDL3/SDL.h>
#include <imgui/imgui.h>

#include <engine/core/hash_map.h>
#include <engine/platform/platform.h>
#include <game/game.h>
#include <engine/scene/scripts_components.h>
#include <engine/external/game_components.h>

#include <game/devmenu.h>

typedef void ( *crude_devmenu_option_callback_function )
(
  _In_ crude_devmenu                                      *devmenu
);

typedef bool ( *crude_devmenu_hotkey_pressed_callback_function )
(
  _In_ crude_input                                        *input
);

typedef struct crude_devmenu_option
{
  char const                                              *name;
  crude_devmenu_option_callback_function                   callback;
  crude_devmenu_hotkey_pressed_callback_function           hotkey_pressed_callback;
} crude_devmenu_option;

crude_devmenu_option devmenu_options[ ] =
{
  {
    "Free Camera", crude_devmenu_free_camera_callback, crude_devmenu_free_camera_callback_hotkey_pressed_callback
  },
  {
    "Reload Techniques", crude_devmenu_reload_techniques_callback, crude_devmenu_reload_techniques_hotkey_pressed_callback
  },
  {
    "GPU Visual Profiler", crude_devmenu_gpu_visual_profiler_callback
  },
  {
    "Memory Visual Profiler", crude_devmenu_memory_visual_profiler_callback
  },
  {
    "Texture Inspector", crude_devmenu_texture_inspector_callback
  },
  {
    "Render Graph", crude_devmenu_render_graph_callback
  },
  {
    "GPU Pool", crude_devmenu_gpu_pool_callback
  },
  {
    "Scene Renderer", crude_devmenu_scene_renderer_callback
  },
  {
    "Show/Hide Collisions", crude_devmenu_collisions_view_callback
  },
  {
    "Show/Hide Debug GLTF", crude_devmenu_debug_gltf_view_callback
  },
  {
    "Nodes Tree", crude_devmenu_nodes_tree_callback
  },
  {
    "Gameplay", crude_devmenu_gameplay_callback
  }
};

void
crude_devmenu_initialize
(
  _In_ crude_devmenu                                      *devmenu
)
{
  devmenu->enabled = false;
  devmenu->selected_option = 0;
  devmenu->previous_framerate = 0.f;
  devmenu->current_framerate = 0.f;
  crude_devmenu_gpu_visual_profiler_initialize( &devmenu->gpu_visual_profiler );
  crude_devmenu_memory_visual_profiler_initialize( &devmenu->memory_visual_profiler );
  crude_devmenu_texture_inspector_initialize( &devmenu->texture_inspector );
  crude_devmenu_render_graph_initialize( &devmenu->render_graph );
  crude_devmenu_gpu_pool_initialize( &devmenu->gpu_pool );
  crude_devmenu_scene_renderer_initialize( &devmenu->scene_renderer );
  crude_devmenu_nodes_tree_initialize( &devmenu->nodes_tree );
  crude_devmenu_gameplay_initialize( &devmenu->gameplay );
}

void
crude_devmenu_deinitialize
(
  _In_ crude_devmenu                                      *devmenu
)
{
  crude_devmenu_gpu_visual_profiler_deinitialize( &devmenu->gpu_visual_profiler );
  crude_devmenu_memory_visual_profiler_deinitialize( &devmenu->memory_visual_profiler );
  crude_devmenu_texture_inspector_deinitialize( &devmenu->texture_inspector );
  crude_devmenu_render_graph_deinitialize( &devmenu->render_graph );
  crude_devmenu_gpu_pool_deinitialize( &devmenu->gpu_pool );
  crude_devmenu_scene_renderer_deinitialize( &devmenu->scene_renderer );
  crude_devmenu_nodes_tree_deinitialize( &devmenu->nodes_tree );
  crude_devmenu_gameplay_deinitialize( &devmenu->gameplay );
}

void
crude_devmenu_draw
(
  _In_ crude_devmenu                                      *devmenu
)
{
  game_t                                                  *game;

  game = game_instance( );
  
  ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
  if ( devmenu->enabled )
  {
    ImGui::SetNextWindowSize( ImVec2( game->gpu.vk_swapchain_width, 25 ) );
    ImGui::Begin( "Devmenu", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
    ImGui::GetIO().FontGlobalScale = 0.5f;
    for ( uint32 i = 0; i < CRUDE_COUNTOF( devmenu_options ); ++i  )
    {
      ImGui::SetCursorPos( ImVec2( i * ( 100 ), 0 ) );
      if ( ImGui::Button( devmenu_options[ i ].name, ImVec2( 100, 25 ) ) )
      {
        devmenu_options[ i ].callback( devmenu );
      }
      ImGui::SameLine( );
    }
    ImGui::GetIO().FontGlobalScale = 1.f;
    ImGui::End( );
  }

  {
    ImGui::SetNextWindowSize( ImVec2( game->gpu.vk_swapchain_width, 50 ) );
    ImGui::Begin( "Overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
    ImGui::Text( "FPS %u", CRUDE_MIN_INT( devmenu->previous_framerate, game->framerate ) );
    ImGui::End( );
  }

  crude_devmenu_gpu_visual_profiler_draw( &devmenu->gpu_visual_profiler );
  crude_devmenu_memory_visual_profiler_draw( &devmenu->memory_visual_profiler );
  crude_devmenu_texture_inspector_draw( &devmenu->texture_inspector );
  crude_devmenu_render_graph_draw( &devmenu->render_graph );
  crude_devmenu_gpu_pool_draw( &devmenu->gpu_pool );
  crude_devmenu_scene_renderer_draw( &devmenu->scene_renderer );
  crude_devmenu_nodes_tree_draw( &devmenu->nodes_tree );
  crude_devmenu_gameplay_draw( &devmenu->gameplay );
}

void
crude_devmenu_update
(
  _In_ crude_devmenu                                      *devmenu
)
{
  game_t *game = game_instance( );

  crude_devmenu_gpu_visual_profiler_update( &devmenu->gpu_visual_profiler );
  crude_devmenu_memory_visual_profiler_update( &devmenu->memory_visual_profiler );
  crude_devmenu_texture_inspector_update( &devmenu->texture_inspector );
  crude_devmenu_render_graph_update( &devmenu->render_graph );
  crude_devmenu_gpu_pool_update( &devmenu->gpu_pool );
  crude_devmenu_scene_renderer_update( &devmenu->scene_renderer );
  crude_devmenu_nodes_tree_update( &devmenu->nodes_tree );
  crude_devmenu_gameplay_update( &devmenu->gameplay );

  if ( game->time - devmenu->last_framerate_update_time > 1.f )
  {
    devmenu->previous_framerate = devmenu->current_framerate;
    devmenu->current_framerate = 0u;
    devmenu->last_framerate_update_time = game->time;
  }
  else
  {
    devmenu->current_framerate++;
  }
}

void
crude_devmenu_handle_input
(
  _In_ crude_devmenu                                      *devmenu,
  _In_ crude_input                                        *input
)
{
  game_t                                                  *game;

  game = game_instance( );
  
  if ( input->keys[ SDL_SCANCODE_F4 ].pressed )
  {
    devmenu->enabled = !devmenu->enabled;
    
    crude_window_handle *window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->platform_node, crude_window_handle );
    crude_entity player_controller_node = crude_ecs_lookup_entity_from_parent( game->main_node, "player" );
    if ( devmenu->enabled )
    {
      crude_platform_show_cursor( *window_handle );
    }
    else
    {
      crude_platform_hide_cursor( *window_handle );
    }
    
    crude_player_controller *player_controller = NULL;
    if ( crude_entity_valid( player_controller_node ) )
    {
      player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( player_controller_node, crude_player_controller );
    }
    if ( player_controller )
    {
      player_controller->input_enabled = !devmenu->enabled;
    }
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
crude_devmenu_debug_gltf_view_callback
(
  _In_ crude_devmenu                                      *devmenu
)
{
  game_t *game = game_instance( );
  game->scene_renderer.options.hide_debug_gltf = !game->scene_renderer.options.hide_debug_gltf;
}


bool
crude_devmenu_debug_gltf_view_callback_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
)
{
  return false;
}

void
crude_devmenu_collisions_view_callback
(
  _In_ crude_devmenu                                      *devmenu
)
{
  game_t *game = game_instance( );
  game->scene_renderer.options.hide_collision = !game->scene_renderer.options.hide_collision;
}

bool
crude_devmenu_collisions_view_callback_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
)
{
  return false;
}

void
crude_devmenu_free_camera_callback
(
  _In_ crude_devmenu                                      *devmenu
)
{
  game_t *game = game_instance( );
  crude_entity player_controller_node = crude_ecs_lookup_entity_from_parent( game->main_node, "player" );
  crude_player_controller *player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( player_controller_node, crude_player_controller );
  player_controller->fly_mode = !player_controller->fly_mode;
  crude_physics_enable_simulation( &game->physics, !game->physics.simulation_enabled );
}

bool
crude_devmenu_free_camera_callback_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
)
{
  return input->keys[ SDL_SCANCODE_F1 ].pressed;
}

void
crude_devmenu_reload_techniques_callback
(
  _In_ crude_devmenu                                      *devmenu
)
{
  game_push_reload_techniques_command( game_instance( ) );
}

bool
crude_devmenu_reload_techniques_hotkey_pressed_callback
(
  _In_ crude_input                                        *input
)
{
  return input->keys[ SDL_SCANCODE_LCTRL ].pressed && input->keys[ SDL_SCANCODE_G ].pressed && input->keys[ SDL_SCANCODE_R ].pressed;
}

/***********************
 * Devmenu
 ***********************/
void
crude_devmenu_gpu_visual_profiler_initialize
(
  _In_ crude_devmenu_gpu_visual_profiler                  *dev_gpu_profiler
)
{
  game_t                                                  *game;

  game = game_instance( );

  dev_gpu_profiler->gpu = &game->gpu;
  dev_gpu_profiler->enabled = false;
  dev_gpu_profiler->max_duration = 16.666f;
  dev_gpu_profiler->max_frames = 100u;
  dev_gpu_profiler->current_frame = 0u;
  dev_gpu_profiler->max_visible_depth = 2;
  dev_gpu_profiler->max_queries_per_frame = 32;
  dev_gpu_profiler->allocator = &game->allocator;
  dev_gpu_profiler->paused = false;
  dev_gpu_profiler->pipeline_statistics = NULL;
  dev_gpu_profiler->timestamps = CRUDE_CAST( crude_gfx_gpu_time_query*, CRUDE_ALLOCATE( crude_heap_allocator_pack( dev_gpu_profiler->allocator ), sizeof( crude_gfx_gpu_time_query ) * dev_gpu_profiler->max_frames * dev_gpu_profiler->max_queries_per_frame ) );
  dev_gpu_profiler->per_frame_active = CRUDE_CAST( uint16*, CRUDE_ALLOCATE( crude_heap_allocator_pack( dev_gpu_profiler->allocator ), sizeof( uint16 ) * dev_gpu_profiler->max_frames, allocator ) );
  dev_gpu_profiler->framebuffer_pixel_count = 0u;
  dev_gpu_profiler->initial_frames_paused = 15;
  memset( dev_gpu_profiler->per_frame_active, 0, sizeof( uint16 ) * dev_gpu_profiler->max_frames );
  CRUDE_HASHMAP_INITIALIZE( dev_gpu_profiler->name_hashed_to_color_index, crude_heap_allocator_pack( dev_gpu_profiler->allocator ) );
}

void
crude_devmenu_gpu_visual_profiler_deinitialize
(
  _In_ crude_devmenu_gpu_visual_profiler                  *dev_gpu_profiler
)
{
  game_t                                                  *game;

  game = game_instance( );

  CRUDE_HASHMAP_DEINITIALIZE( dev_gpu_profiler->name_hashed_to_color_index );
  CRUDE_DEALLOCATE( crude_heap_allocator_pack( dev_gpu_profiler->allocator ), dev_gpu_profiler->timestamps );
  CRUDE_DEALLOCATE( crude_heap_allocator_pack( dev_gpu_profiler->allocator ), dev_gpu_profiler->per_frame_active );
}

void
crude_devmenu_gpu_visual_profiler_update
(
  _In_ crude_devmenu_gpu_visual_profiler                  *dev_gpu_profiler
)
{
  game_t                                                  *game;

  game = game_instance( );

  crude_gfx_gpu_set_timestamps_enable( dev_gpu_profiler->gpu, !dev_gpu_profiler->paused );

  if ( dev_gpu_profiler->initial_frames_paused )
  {
    --dev_gpu_profiler->initial_frames_paused;
    return;
  }

  if ( dev_gpu_profiler->paused )
  {
    return;
  }

  uint32 active_timestamps = crude_gfx_copy_gpu_timestamps( dev_gpu_profiler->gpu, &dev_gpu_profiler->timestamps[ dev_gpu_profiler->max_queries_per_frame * dev_gpu_profiler->current_frame ] );

  dev_gpu_profiler->per_frame_active[ dev_gpu_profiler->current_frame ] = active_timestamps;
  
  dev_gpu_profiler->framebuffer_pixel_count = dev_gpu_profiler->gpu->vk_swapchain_width * dev_gpu_profiler->gpu->vk_swapchain_height;

  dev_gpu_profiler->pipeline_statistics = &dev_gpu_profiler->gpu->gpu_time_queries_manager->frame_pipeline_statistics;

  for ( uint32 i = 0; i < active_timestamps; ++i )
  {
    crude_gfx_gpu_time_query                              *timestamp;
    int64                                                  hash_color_index;
    uint64                                                 hashed_name, color_index;

    timestamp = &dev_gpu_profiler->timestamps[ dev_gpu_profiler->max_queries_per_frame * dev_gpu_profiler->current_frame + i ];
  
    hashed_name = crude_hash_string( timestamp->name, 0u );
    hash_color_index = CRUDE_HASHMAP_GET_INDEX( dev_gpu_profiler->name_hashed_to_color_index, hashed_name );

    if ( hash_color_index == -1 )
    {
      color_index = CRUDE_HASHMAP_LENGTH( dev_gpu_profiler->name_hashed_to_color_index );
      CRUDE_HASHMAP_SET( dev_gpu_profiler->name_hashed_to_color_index, hashed_name, color_index );
    }
    else
    {
      color_index = dev_gpu_profiler->name_hashed_to_color_index[ hash_color_index ].value;
    }
  
    timestamp->color = crude_color_get_distinct_color( color_index );
  }

  dev_gpu_profiler->current_frame = ( dev_gpu_profiler->current_frame + 1 ) % dev_gpu_profiler->max_frames;

  if ( dev_gpu_profiler->current_frame == 0 )
  {
    dev_gpu_profiler->max_time = -FLT_MAX;
    dev_gpu_profiler->min_time = FLT_MAX;
    dev_gpu_profiler->average_time = 0.f;
  }
}

void
crude_devmenu_gpu_visual_profiler_draw
(
  _In_ crude_devmenu_gpu_visual_profiler                  *dev_gpu_profiler
)
{
  game_t                                                  *game;

  game = game_instance( );

  if ( !dev_gpu_profiler->enabled )
  {
    return;
  }

  ImGui::Begin( "GPU Visual Profiler" );

  {
    ImGuiIO                                               *imgui_io;
    ImDrawList                                            *draw_list;
    char                                                   buf[ 128 ];
    ImVec2                                                 cursor_pos, canvas_size, mouse_pos;
    float64                                                new_average;
    float32                                                widget_height, legend_width, graph_width;
    uint32                                                 rect_width;
    int32                                                  rect_x, selected_frame;

    draw_list = ImGui::GetWindowDrawList();
    cursor_pos = ImGui::GetCursorScreenPos();
    canvas_size = ImGui::GetContentRegionAvail();
    widget_height = canvas_size.y - 100;
    
    legend_width = 250;
    graph_width = fabsf( canvas_size.x - legend_width );
    rect_width = CRUDE_CEIL( graph_width / dev_gpu_profiler->max_frames );
    rect_x = CRUDE_CEIL( graph_width - rect_width );

    new_average = 0;

    imgui_io = &ImGui::GetIO();

    crude_memory_set( buf, 0u, sizeof( buf ) );

    mouse_pos = imgui_io->MousePos;

    selected_frame = -1;

    /* Draw Graph */
    for ( uint32 i = 0; i < dev_gpu_profiler->max_frames; ++i )
    {
      crude_gfx_gpu_time_query                            *frame_timestamps;
      uint32                                               frame_index;
      float32                                              frame_x, frame_time, rect_height, current_height;

      frame_index = ( dev_gpu_profiler->current_frame - 1 - i ) % dev_gpu_profiler->max_frames;

      frame_x = cursor_pos.x + rect_x;
      frame_timestamps = &dev_gpu_profiler->timestamps[ frame_index * dev_gpu_profiler->max_queries_per_frame ];
      frame_time = frame_timestamps[ 0 ].elapsed_ms;
      
      frame_time = CRUDE_CLAMP( frame_time, 1000.f, 0.00001f );
            
      new_average += frame_time;
      dev_gpu_profiler->min_time = CRUDE_MIN( dev_gpu_profiler->min_time, frame_time );
      dev_gpu_profiler->max_time = CRUDE_MAX( dev_gpu_profiler->max_time, frame_time );
      
      rect_height = frame_time / dev_gpu_profiler->max_duration * widget_height;
      current_height = cursor_pos.y;
      
      /* Draw timestamps from the bottom */
      for ( uint32 j = 0; j < dev_gpu_profiler->per_frame_active[ frame_index ]; ++j )
      {
        crude_gfx_gpu_time_query const                    *timestamp;
        ImVec2                                             rect_min, rect_max;
        uint32                                             width_margin;

        timestamp = &frame_timestamps[ j ];

        if ( timestamp->depth != 1 )
        {
          continue;
        }
        
        width_margin = 2;
      
        rect_height = ( float32 )timestamp->elapsed_ms / dev_gpu_profiler->max_duration * widget_height;
        rect_min = CRUDE_COMPOUNT( ImVec2, { frame_x + width_margin, current_height + widget_height - rect_height } );
        rect_max = CRUDE_COMPOUNT( ImVec2, { frame_x + width_margin + rect_width - width_margin, current_height + widget_height } );
        draw_list->AddRectFilled( rect_min, rect_max, timestamp->color );
      
        current_height -= rect_height;
      }
      
      if ( mouse_pos.x >= frame_x && mouse_pos.x < frame_x + rect_width && mouse_pos.y >= cursor_pos.y && mouse_pos.y < cursor_pos.y + widget_height )
      {
        draw_list->AddRectFilled(
          { frame_x, cursor_pos.y + widget_height },
          { frame_x + rect_width, cursor_pos.y },
          0x0fffffff
        );
      
        ImGui::SetTooltip( "(%u): %f", frame_index, frame_time );
      
        selected_frame = frame_index;
      }

      draw_list->AddLine( { frame_x, cursor_pos.y + widget_height }, { frame_x, cursor_pos.y }, 0x0fffffff );

      rect_x -= rect_width;
    }
    
    sprintf( buf, "%3.4fms", dev_gpu_profiler->max_duration );
    draw_list->AddText( { cursor_pos.x, cursor_pos.y }, 0xff0000ff, buf );
    draw_list->AddRectFilled( { cursor_pos.x + rect_width, cursor_pos.y }, { cursor_pos.x + graph_width, cursor_pos.y + 1 }, 0xff0000ff );

    sprintf( buf, "%3.4fms", dev_gpu_profiler->max_duration / 2.f );
    draw_list->AddText( { cursor_pos.x, cursor_pos.y + widget_height / 2.f }, 0xff00ffff, buf );
    draw_list->AddRectFilled( { cursor_pos.x + rect_width, cursor_pos.y + widget_height / 2.f }, { cursor_pos.x + graph_width, cursor_pos.y + widget_height / 2.f + 1 }, 0xff00ffff );

    dev_gpu_profiler->average_time = CRUDE_CAST( float32, dev_gpu_profiler->new_average ) / dev_gpu_profiler->max_frames;

    /* Draw legend */
    ImGui::SetCursorPosX( cursor_pos.x + graph_width );

    selected_frame = selected_frame == -1 ? ( dev_gpu_profiler->current_frame - 1 ) % dev_gpu_profiler->max_frames : selected_frame;
    if ( selected_frame >= 0 )
    {
      crude_gfx_gpu_time_query                            *frame_timestamps;
      float32                                              x, y;

      frame_timestamps = &dev_gpu_profiler->timestamps[ selected_frame * dev_gpu_profiler->max_queries_per_frame ];
    
      x = cursor_pos.x + graph_width + 8;
      y = cursor_pos.y + widget_height - 14;
    
      for ( uint32 j = 0; j < dev_gpu_profiler->per_frame_active[ selected_frame ]; ++j )
      {
        crude_gfx_gpu_time_query                          *timestamp;
        float32                                            timestamp_x;

        timestamp = &frame_timestamps[ j ];
    
        if ( timestamp->depth > dev_gpu_profiler->max_visible_depth )
        {
          continue;
        }
    
        timestamp_x = x + timestamp->depth * 4;
    
        if ( timestamp->depth == 0 )
        {
          draw_list->AddRectFilled(
            { timestamp_x, cursor_pos.y + 4 },
            { timestamp_x + 8, cursor_pos.y + 12 },
            timestamp->color
          );
          
          sprintf( buf, "%2.3fms %d %s", timestamp->elapsed_ms, timestamp->depth, timestamp->name );
          draw_list->AddText( { timestamp_x + 20, cursor_pos.y }, 0xffffffff, buf );
        }
        else
        {
          /* Draw all other timestamps starting from bottom */
          draw_list->AddRectFilled(
            { timestamp_x, y + 4 },
            { timestamp_x + 8, y + 12 },
            timestamp->color
          );
    
          sprintf( buf, "%2.3fms %d %s", timestamp->elapsed_ms, timestamp->depth, timestamp->name );
          draw_list->AddText( { timestamp_x + 20, y }, 0xffffffff, buf );
    
          y -= 14;
        }
      }
    }

    ImGui::Dummy( { canvas_size.x, widget_height } );
  }

  ImGui::SetNextItemWidth( 100.f );
  ImGui::LabelText( "", "Max %3.4fms", dev_gpu_profiler->max_time );
  ImGui::SameLine();
  ImGui::SetNextItemWidth( 100.f );
  ImGui::LabelText( "", "Min %3.4fms", dev_gpu_profiler->min_time );
  ImGui::SameLine();
  ImGui::LabelText( "", "Ave %3.4fms", dev_gpu_profiler->average_time );
  
  ImGui::Separator();
  ImGui::Checkbox( "Pause", &dev_gpu_profiler->paused );
  
  {
    char const                                            *stat_unit_name;
    float32                                                stat_unit_multiplier;

    static const char                                     *items[] = { "200ms", "100ms", "66ms", "33ms", "16ms", "8ms", "4ms" };
    static const float                                     max_durations[] = { 200.f, 100.f, 66.f, 33.f, 16.f, 8.f, 4.f };
    static const char                                     *stat_unit_names[] = { "Normal", "Kilo", "Mega" };
    static const char                                     *stat_units[] = { "", "K", "M" };
    static const float32                                   stat_unit_multipliers[] = { 1.f, 1000.f, 1000000.f };
    static int                                             max_duration_index = 4;
    static int                                             stat_unit_index = 1;

    if ( ImGui::Combo( "Graph Max", &max_duration_index, items, IM_ARRAYSIZE( items ) ) )
    {
      dev_gpu_profiler->max_duration = max_durations[ max_duration_index ];
    }
    
    ImGui::SliderInt( "Max Depth", &dev_gpu_profiler->max_visible_depth, 1, 4 );
    
    ImGui::Separator();
    
    stat_unit_multiplier = stat_unit_multipliers[ stat_unit_index ];
    stat_unit_name = stat_units[ stat_unit_index ];
    if ( dev_gpu_profiler->pipeline_statistics )
    {
      float32 stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_COUNT ];
      for ( uint32 i = 0; i < CRUDE_GFX_GPU_PIPELINE_STATISTICS_COUNT; ++i )
      {
        stat_values[ i ] = dev_gpu_profiler->pipeline_statistics->statistics[ i ] / stat_unit_multiplier;
      }
    
      ImGui::Text( "Vertices %0.2f%s, Primitives %0.2f%s", stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_VERTICES_COUNT ], stat_unit_name,
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_PRIMITIVE_COUNT ], stat_unit_name );
    
      ImGui::Text( "Clipping: Invocations %0.2f%s, Visible Primitives %0.2f%s, Visible Perc %3.1f", stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_CLIPPING_INVOCATIONS ], stat_unit_name,
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_CLIPPING_PRIMITIVES ], stat_unit_name,
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_CLIPPING_PRIMITIVES ] / stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_CLIPPING_INVOCATIONS ] * 100.0f, stat_unit_name );
    
      ImGui::Text( "Invocations: Vertex Shaders %0.2f%s, Fragment Shaders %0.2f%s, Compute Shaders %0.2f%s", stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_VERTEX_SHADER_INVOCATIONS ], stat_unit_name,
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_FRAGMENT_SHADER_INVOCATIONS ], stat_unit_name, stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_COMPUTE_SHADER_INVOCATIONS ], stat_unit_name );
    
      ImGui::Text( "Invocations divided by number of full screen quad pixels." );
      ImGui::Text( "Vertex %0.2f, Fragment %0.2f, Compute %0.2f", stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_VERTEX_SHADER_INVOCATIONS ] * stat_unit_multiplier / dev_gpu_profiler->framebuffer_pixel_count,
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_FRAGMENT_SHADER_INVOCATIONS ] * stat_unit_multiplier / dev_gpu_profiler->framebuffer_pixel_count,
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_COMPUTE_SHADER_INVOCATIONS ] * stat_unit_multiplier / dev_gpu_profiler->framebuffer_pixel_count );
    }
  
    ImGui::Combo( "Stat Units", &stat_unit_index, stat_unit_names, IM_ARRAYSIZE( stat_unit_names ) );
  }
  ImGui::End( );
}

void
crude_devmenu_gpu_visual_profiler_callback
(
  _In_ crude_devmenu                                      *devmenu
)
{
  devmenu->gpu_visual_profiler.enabled = !devmenu->gpu_visual_profiler.enabled;
}

/***********************
 * 
 * Develop Memory Visual Profiler
 * 
 ***********************/
void
crude_devmenu_memory_visual_profiler_initialize
(
  _In_ crude_devmenu_memory_visual_profiler                *dev_mem_profiler
)
{
  game_t *game = game_instance( );

  dev_mem_profiler->enabled = false;
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( dev_mem_profiler->allocators_containers, 16, crude_heap_allocator_pack( &game->allocator ) );
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_heap_allocator_pack( &game->engine->asynchronous_loader_manager_allocator ) );
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_heap_allocator_pack( &game->allocator ) );
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_heap_allocator_pack( &game->cgltf_temporary_allocator ) );
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_stack_allocator_pack( &game->model_renderer_resources_manager_temporary_allocator ) );
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_heap_allocator_pack( &game->resources_allocator ) );
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_stack_allocator_pack( &game->temporary_allocator ) );
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_linear_allocator_pack( &game->render_graph.linear_allocator ) );
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_linear_allocator_pack( &game->node_manager.string_linear_allocator ) );
}

void
crude_devmenu_memory_visual_profiler_deinitialize
(
  _In_ crude_devmenu_memory_visual_profiler                *dev_mem_profiler
)
{
  CRUDE_ARRAY_DEINITIALIZE( dev_mem_profiler->allocators_containers );
}

void
crude_devmenu_memory_visual_profiler_update
(
  _In_ crude_devmenu_memory_visual_profiler                *dev_mem_profiler
)
{
}

void
crude_devmenu_memory_visual_profiler_draw
(
  _In_ crude_devmenu_memory_visual_profiler                *dev_mem_profiler
)
{
  game_t                                                  *game;

  game = game_instance( );

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
crude_devmenu_memory_visual_profiler_callback
(
  _In_ crude_devmenu                                      *devmenu
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
crude_devmenu_texture_inspector_initialize
(
  _In_ crude_devmenu_texture_inspector                    *dev_texture_inspector
)
{
  game_t                                                  *game;

  game = game_instance( );

  dev_texture_inspector->enabled = false;
  dev_texture_inspector->texture_handle = crude_gfx_access_texture( &game->gpu, crude_gfx_render_graph_builder_access_resource_by_name( game->scene_renderer.render_graph->builder, "final" )->resource_info.texture.handle )->handle;
}

void
crude_devmenu_texture_inspector_deinitialize
(
  _In_ crude_devmenu_texture_inspector                    *dev_texture_inspector
)
{
}

void
crude_devmenu_texture_inspector_update
(
  _In_ crude_devmenu_texture_inspector                    *dev_texture_inspector
)
{
}

void
crude_devmenu_texture_inspector_draw
(
  _In_ crude_devmenu_texture_inspector                    *dev_texture_inspector
)
{
  char const                                              *preview_texture_name;
  game_t                                                  *game;
  uint32                                                   id;

  game = game_instance( );
  
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
    crude_gfx_texture *selected_texture = crude_gfx_access_texture( &game->gpu, dev_texture_inspector->texture_handle );
    if ( selected_texture && selected_texture->name )
    {
      preview_texture_name = selected_texture->name;
    };
  }

  if ( ImGui::BeginCombo( "Texture ID", preview_texture_name ) )
  {
    for ( uint32 t = 0; t < game->gpu.textures.pool_size; ++t )
    {
      crude_gfx_texture                                   *texture;
      crude_gfx_texture_handle                             texture_handle;
      bool                                                 is_selected;

      texture_handle = CRUDE_CAST( crude_gfx_texture_handle, t );
      if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( texture_handle ) )
      {
        continue;
      }
      
      texture = crude_gfx_access_texture( &game->gpu, texture_handle );
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
crude_devmenu_texture_inspector_callback
(
  _In_ crude_devmenu                                      *devmenu
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
crude_devmenu_render_graph_initialize
(
  _In_ crude_devmenu_render_graph                         *dev_render_graph
)
{
  dev_render_graph->enabled = false;
}

void
crude_devmenu_render_graph_deinitialize
(
  _In_ crude_devmenu_render_graph                         *dev_render_graph
)
{
}

void
crude_devmenu_render_graph_update
(
  _In_ crude_devmenu_render_graph                         *dev_render_graph
)
{
}

void
crude_devmenu_render_graph_draw
(
  _In_ crude_devmenu_render_graph                         *dev_render_graph
)
{
  game_t *game = game_instance( );

  if ( !dev_render_graph->enabled )
  {
    return;
  }

  if ( ImGui::Begin( "Render Graph Debug" ) )
  {
    if ( ImGui::CollapsingHeader( "Nodes" ) )
    {
      for ( uint32 n = 0; n < CRUDE_ARRAY_LENGTH( game->render_graph.nodes ); ++n )
      {
        crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( game->render_graph.builder, game->render_graph.nodes[ n ] );

        ImGui::Separator( );
        ImGui::Text( "Pass: %s", node->name );

        ImGui::Text( "\tInputs" );
        for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( node->inputs ); ++i )
        {
          crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( game->render_graph.builder, node->inputs[ i ] );
          ImGui::Text( "\t\t%s %u", resource->name, resource->resource_info.texture.handle.index );
        }

        ImGui::Text( "\tOutputs" );
        for ( uint32 o = 0; o < CRUDE_ARRAY_LENGTH( node->outputs ); ++o )
        {
          crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( game->render_graph.builder, node->outputs[ o ] );
          ImGui::Text( "\t\t%s %u", resource->name, resource->resource_info.texture.handle.index );
        }

        ImGui::PushID( n );
        ImGui::Checkbox( "Enabled", &node->enabled );
        ImGui::PopID( );
      }
    }
  }
  ImGui::End( );
}

void
crude_devmenu_render_graph_callback
(
  _In_ crude_devmenu                                      *devmenu
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
crude_devmenu_gpu_pool_draw_pool_
(
  _In_ crude_resource_pool                                *resource_pool,
  _In_ char const                                         *resource_name
)
{
  ImGui::Text( "Pool %s, indices used %u, allocated %u", resource_name, resource_pool->used_indices, resource_pool->pool_size );
}

void
crude_devmenu_gpu_pool_initialize
(
  _In_ crude_devmenu_gpu_pool                             *dev_gpu_pool
)
{
  dev_gpu_pool->enabled = false;
}

void
crude_devmenu_gpu_pool_deinitialize
(
  _In_ crude_devmenu_gpu_pool                             *dev_gpu_pool
)
{
}

void
crude_devmenu_gpu_pool_update
(
  _In_ crude_devmenu_gpu_pool                             *dev_gpu_pool
)
{
}

void
crude_devmenu_gpu_pool_draw
(
  _In_ crude_devmenu_gpu_pool                             *dev_gpu_pool
)
{
  game_t *game = game_instance( );

  if ( !dev_gpu_pool->enabled )
  {
    return;
  }

  VkPhysicalDeviceProperties                               vk_physical_properties;
  VmaBudget                                                gpu_memory_heap_budgets[ VK_MAX_MEMORY_HEAPS ];
  uint64                                                   memory_used, memory_allocated;

  crude_memory_set( gpu_memory_heap_budgets, 0u, sizeof( gpu_memory_heap_budgets ) );
  vmaGetHeapBudgets( game->gpu.vma_allocator, gpu_memory_heap_budgets );
 
  memory_used = memory_allocated = 0;
  for ( uint32 i = 0; i < VK_MAX_MEMORY_HEAPS; ++i )
  {
    memory_used += gpu_memory_heap_budgets[ i ].usage;
    memory_allocated += gpu_memory_heap_budgets[ i ].budget;
  }
   
  vkGetPhysicalDeviceProperties( game->gpu.vk_physical_device, &vk_physical_properties );

  ImGui::Text( "GPU used: %s", vk_physical_properties.deviceName ? vk_physical_properties.deviceName : "Unknown" );
  ImGui::Text( "GPU Memory Used: %lluMB, Total: %lluMB", memory_used / ( 1024 * 1024 ), memory_allocated / ( 1024 * 1024 ) );

  ImGui::Separator();
  crude_devmenu_gpu_pool_draw_pool_( &game->gpu.buffers, "Buffers" );
  crude_devmenu_gpu_pool_draw_pool_( &game->gpu.textures, "Textures" );
  crude_devmenu_gpu_pool_draw_pool_( &game->gpu.pipelines, "Pipelines" );
  crude_devmenu_gpu_pool_draw_pool_( &game->gpu.samplers, "Samplers" );
  crude_devmenu_gpu_pool_draw_pool_( &game->gpu.descriptor_sets, "DescriptorSets" );
  crude_devmenu_gpu_pool_draw_pool_( &game->gpu.descriptor_set_layouts, "DescriptorSetLayouts" );
  crude_devmenu_gpu_pool_draw_pool_( &game->gpu.framebuffers, "Framebuffers" );
  crude_devmenu_gpu_pool_draw_pool_( &game->gpu.render_passes, "RenderPasses" );
  crude_devmenu_gpu_pool_draw_pool_( &game->gpu.shaders, "Shaders" );
}

void
crude_devmenu_gpu_pool_callback
(
  _In_ crude_devmenu                                      *devmenu
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
crude_devmenu_scene_renderer_initialize
(
  _In_ crude_devmenu_scene_renderer                       *dev_scene_rendere
)
{
  dev_scene_rendere->enabled = false;
}

void
crude_devmenu_scene_renderer_deinitialize
(
  _In_ crude_devmenu_scene_renderer                       *dev_scene_rendere
)
{
}

void
crude_devmenu_scene_renderer_update
(
  _In_ crude_devmenu_scene_renderer                       *dev_scene_rendere
)
{
}

void
crude_devmenu_scene_renderer_draw
(
  _In_ crude_devmenu_scene_renderer                       *dev_scene_rendere
)
{
  game_t *game = game_instance( );

  if ( !dev_scene_rendere->enabled )
  {
    return;
  }

  ImGui::Begin( "Scene Renderer" );
  if ( ImGui::CollapsingHeader( "Background" ) )
  {
    ImGui::ColorEdit3( "Background Color", &game->scene_renderer.options.background_color.x );
    ImGui::DragFloat( "Background Intensity", &game->scene_renderer.options.background_intensity, 1.f, 0.f );
  }
  if ( ImGui::CollapsingHeader( "Fog" ) )
  {
    ImGui::ColorEdit3( "Fog Color", &game->game_postprocessing_pass.options.fog_color.x );
    ImGui::DragFloat( "Fog Intensity", &game->game_postprocessing_pass.options.fog_color.w, 1.f, 0.f );
    ImGui::DragFloat( "Fog Distance", &game->game_postprocessing_pass.options.fog_distance, 1.f, 0.f );
    ImGui::DragFloat( "Fog Coeff", &game->game_postprocessing_pass.options.fog_coeff, 1.f, 0.f );
  }
  if ( ImGui::CollapsingHeader( "Drunk Effect" ) )
  {
    ImGui::DragFloat( "Wave Size", &game->game_postprocessing_pass.options.wave_size, 1.f, 0.f );
    ImGui::DragFloat( "Wave Texcoord Scale", &game->game_postprocessing_pass.options.wave_texcoord_scale, 1.f, 0.f );
    ImGui::DragFloat( "Wave Absolute Frame Scale", &game->game_postprocessing_pass.options.wave_absolute_frame_scale, 1.f, 0.f );
    ImGui::DragFloat( "Aberration Strength Scale", &game->game_postprocessing_pass.options.aberration_strength_scale, 1.f, 0.f );
    ImGui::DragFloat( "Aberration Strength Offset", &game->game_postprocessing_pass.options.aberration_strength_offset, 1.f, 0.f );
    ImGui::DragFloat( "Aberration Strength Sin Affect", &game->game_postprocessing_pass.options.aberration_strength_sin_affect, 1.f, 0.f );
  }
  if ( ImGui::CollapsingHeader( "Health Pulse Effect" ) )
  {
    ImGui::ColorEdit3( "Pulse Color", &game->game_postprocessing_pass.options.pulse_color.x );
    ImGui::DragFloat( "Pulse Intensity", &game->game_postprocessing_pass.options.pulse_color.w, 1.f, 0.f );
    ImGui::DragFloat( "Pulse Frame Scale", &game->game_postprocessing_pass.options.pulse_frame_scale, 1.f, 0.f );
    ImGui::DragFloat( "Pulse Scale", &game->game_postprocessing_pass.options.pulse_scale, 1.f, 0.f );
    ImGui::DragFloat( "Pulse Coeff", &game->game_postprocessing_pass.options.pulse_distance_coeff, 1.f, 0.f );
    ImGui::DragFloat( "Pulse Distance", &game->game_postprocessing_pass.options.pulse_distance, 1.f, 0.f );
  }
  if ( ImGui::CollapsingHeader( "Global Illumination" ) )
  {
    ImGui::ColorEdit3( "Ambient Color", &game->scene_renderer.options.ambient_color.x );
    ImGui::DragFloat( "Ambient Intensity", &game->scene_renderer.options.ambient_intensity, 0.1f, 0.f );
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
crude_devmenu_scene_renderer_callback
(
  _In_ crude_devmenu                                      *devmenu
)
{
  devmenu->scene_renderer.enabled = !devmenu->scene_renderer.enabled;
}

/***********************
 * 
 * Develop Nodes Tree
 * 
 ***********************/
void
crude_devmenu_nodes_tree_draw_internal_
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree,
  _In_ crude_entity                                        node,
  _In_ uint32                                             *current_node_index
)
{
  game_t                                                  *game;
  ImGuiTreeNodeFlags                                       tree_node_flags;
  bool                                                     can_open_children_nodes, tree_node_opened;
  
  game = game_instance( );

  ImGui::Begin( "Scene Node Tree" );
  
  {
    can_open_children_nodes = false;

    ecs_iter_t it = ecs_children( node.world, node.handle );
    if ( !CRUDE_ENTITY_HAS_COMPONENT( node, crude_gltf ) && ecs_children_next( &it ) )
    {
      if ( it.count )
      {
        can_open_children_nodes = true;
      }
    }
  }

  tree_node_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
  
  if ( !can_open_children_nodes )
  {
    tree_node_flags |= ImGuiTreeNodeFlags_Leaf;
  }

  if ( dev_nodes_tree->selected_node.handle == node.handle )
  {
    tree_node_flags |= ImGuiTreeNodeFlags_Selected;
  }

  tree_node_opened = ImGui::TreeNodeEx( ( void* )( intptr_t )*current_node_index, tree_node_flags, crude_entity_get_name( node ) );
  if ( ImGui::IsItemClicked( ) && !ImGui::IsItemToggledOpen( ) )
  {
    dev_nodes_tree->selected_node = node;
  }

  if ( ImGui::IsItemClicked( 1 ) && !ImGui::IsItemToggledOpen( ) )
  {
    ImGui::OpenPopup( crude_entity_get_name( node ) );
  }

  if (ImGui::BeginDragDropSource( ) )
  {
    ImGui::SetDragDropPayload( crude_entity_get_name( node ), NULL, 0 );
    ImGui::Text( crude_entity_get_name( node ) );
    ImGui::EndDragDropSource( );
  }

  ++( *current_node_index );
  if ( tree_node_opened )
  {
    if ( can_open_children_nodes )
    {
      ecs_iter_t it = ecs_children( node.world, node.handle );
      while ( ecs_children_next( &it ) )
      {
        for (int i = 0; i < it.count; i ++)
        {
          crude_entity child = CRUDE_COMPOUNT_EMPTY( crude_entity );
          child.world = it.world;
          child.handle = it.entities[ i ];
          crude_devmenu_nodes_tree_draw_internal_( dev_nodes_tree, child, current_node_index );
        }
      }
    }
    
    ImGui::TreePop( );
  }

  ImGui::End( );
}

void
crude_devmenu_nodes_tree_initialize
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree
)
{
  game_t *game = game_instance( );
  dev_nodes_tree->selected_node = game->main_node;
  dev_nodes_tree->enabled = false;
}

void
crude_devmenu_nodes_tree_deinitialize
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree
)
{
}

void
crude_devmenu_nodes_tree_update
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree
)
{
}

void
crude_devmenu_nodes_tree_draw
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree
)
{
  game_t *game = game_instance( );

  if ( !dev_nodes_tree->enabled )
  {
    return;
  }

  uint32 current_node_index = 0u;
  crude_devmenu_nodes_tree_draw_internal_( dev_nodes_tree, game->main_node, &current_node_index );
}

void
crude_devmenu_nodes_tree_callback
(
  _In_ crude_devmenu                                      *devmenu
)
{
  devmenu->nodes_tree.enabled = !devmenu->nodes_tree.enabled;
}


/***********************
 * 
 * Develop Gameplay
 * 
 ***********************/
void
crude_devmenu_gameplay_initialize
(
  _In_ crude_devmenu_gameplay                             *dev_gameplay
)
{
  dev_gameplay->enabled = false;
}

void
crude_devmenu_gameplay_deinitialize
(
  _In_ crude_devmenu_gameplay                             *dev_gameplay
)
{
}

void
crude_devmenu_gameplay_update
(
  _In_ crude_devmenu_gameplay                             *dev_gameplay
)
{
}

void
crude_devmenu_gameplay_draw
(
  _In_ crude_devmenu_gameplay                             *dev_gameplay
)
{
  game_t *game = game_instance( );

  if ( !dev_gameplay->enabled )
  {
    return;
  }

  ImGui::Begin( "Gameplay" );
  if ( ImGui::CollapsingHeader( "Player" ) )
  {
    crude_player                                          *player;
    crude_weapon                                          *weapon;

    player = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->player_node, crude_player );
    weapon = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( crude_ecs_lookup_entity_from_parent( crude_ecs_lookup_entity_from_parent( game->player_node, "pivot" ), "weapon" ), crude_weapon );
    CRUDE_ASSERT( player );

    ImGui::SliderFloat( "Health", &player->health, 1.f, 0.f );
    ImGui::SliderFloat( "Drug Withdrawal", &player->drug_withdrawal, 1.f, 0.f );
    ImGui::SliderFloat( "Sanity", &player->sanity, 1.f, 0.f );
    ImGui::Checkbox( "Stop Updating Gameplay Values", &player->stop_updating_gameplay_values );
    ImGui::Checkbox( "Stop Updating Visual Values", &player->stop_updating_visual_values );
    ImGui::SliderInt( "Ammo", &weapon->ammo, 0, 1000 );
    
    if ( ImGui::Button( "Reset Position" ) )
    {
      crude_transform                                     *player_transform;
      player_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->player_node, crude_transform );
      player_transform->translation = CRUDE_COMPOUNT( XMFLOAT3, { 0.f, 1.5f, 0.f } );
    }
  }
  
  ImGui::End( );
}

void
crude_devmenu_gameplay_callback
(
  _In_ crude_devmenu                                      *devmenu
)
{
  devmenu->gameplay.enabled = !devmenu->gameplay.enabled;
}

#endif