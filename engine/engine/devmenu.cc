#if CRUDE_DEVELOP

#include <SDL3/SDL.h>

#include <engine/core/hash_map.h>
#include <engine/platform/platform.h>
#include <engine/scene/scene_ecs.h>
#include <engine/external/game_ecs.h>
#include <engine/scene/scene_debug_ecs.h>
#include <engine/physics/physics_debug_ecs.h>
#include <engine/scene/scripts/free_camera_ecs.h>
#include <engine/graphics/imgui.h>
#include <engine/engine.h>
#include <engine/core/profiler.h>

#include <engine/engine/devmenu.h>

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
    "Node Inspector", crude_devmenu_node_inspector_callback
  },
  {
    "Technique Editor", crude_devmenu_technique_editor_callback
  }
};

void
crude_devmenu_initialize
(
  _In_ crude_devmenu                                      *devmenu,
  _In_ crude_engine                                       *engine
)
{
  devmenu->engine = engine;
  devmenu->enabled = false;
  devmenu->selected_option = 0;
  devmenu->previous_framerate = 0.f;
  devmenu->current_framerate = 0.f;
  crude_devmenu_gpu_visual_profiler_initialize( &devmenu->gpu_visual_profiler, devmenu );
  crude_devmenu_memory_visual_profiler_initialize( &devmenu->memory_visual_profiler, devmenu );
  crude_devmenu_texture_inspector_initialize( &devmenu->texture_inspector, devmenu );
  crude_devmenu_render_graph_initialize( &devmenu->render_graph, devmenu );
  crude_devmenu_gpu_pool_initialize( &devmenu->gpu_pool, devmenu );
  crude_devmenu_scene_renderer_initialize( &devmenu->scene_renderer, devmenu );
  crude_devmenu_nodes_tree_initialize( &devmenu->nodes_tree, devmenu );
  crude_devmenu_node_inspector_initialize( &devmenu->node_inspector, devmenu );
  crude_devmenu_viewport_initialize( &devmenu->viewport, devmenu );
  crude_devmenu_technique_editor_initialize( &devmenu->technique_editor, devmenu );
}

void
crude_devmenu_deinitialize
(
  _In_ crude_devmenu                                      *devmenu
)
{
  crude_devmenu_technique_editor_deinitialize( &devmenu->technique_editor );
  crude_devmenu_gpu_visual_profiler_deinitialize( &devmenu->gpu_visual_profiler );
  crude_devmenu_memory_visual_profiler_deinitialize( &devmenu->memory_visual_profiler );
  crude_devmenu_texture_inspector_deinitialize( &devmenu->texture_inspector );
  crude_devmenu_render_graph_deinitialize( &devmenu->render_graph );
  crude_devmenu_gpu_pool_deinitialize( &devmenu->gpu_pool );
  crude_devmenu_scene_renderer_deinitialize( &devmenu->scene_renderer );
  crude_devmenu_nodes_tree_deinitialize( &devmenu->nodes_tree );
  crude_devmenu_node_inspector_deinitialize( &devmenu->node_inspector );
  crude_devmenu_viewport_deinitialize( &devmenu->viewport );
}

void
crude_devmenu_draw
(
  _In_ crude_devmenu                                      *devmenu
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_devmenu_draw" );
  
  ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
  if ( devmenu->enabled )
  {
    ImGui::SetNextWindowSize( ImVec2( devmenu->engine->gpu.renderer_size.x, 25 ) );
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

  //{
  //  ImGui::SetNextWindowSize( ImVec2( game->gpu.vk_swapchain_width, 50 ) );
  //  ImGui::Begin( "Overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
  //  ImGui::Text( "FPS %u", CRUDE_MIN_INT( devmenu->previous_framerate, game->framerate ) );
  //  ImGui::End( );
  //}
  
  crude_devmenu_viewport_draw( &devmenu->viewport );
  crude_devmenu_gpu_visual_profiler_draw( &devmenu->gpu_visual_profiler );
  crude_devmenu_memory_visual_profiler_draw( &devmenu->memory_visual_profiler );
  crude_devmenu_texture_inspector_draw( &devmenu->texture_inspector );
  crude_devmenu_render_graph_draw( &devmenu->render_graph );
  crude_devmenu_gpu_pool_draw( &devmenu->gpu_pool );
  crude_devmenu_scene_renderer_draw( &devmenu->scene_renderer );
  crude_devmenu_nodes_tree_draw( &devmenu->nodes_tree );
  crude_devmenu_node_inspector_draw( &devmenu->node_inspector );
  crude_devmenu_technique_editor_draw( &devmenu->technique_editor );
  CRUDE_PROFILER_ZONE_END;
}

void
crude_devmenu_update
(
  _In_ crude_devmenu                                      *devmenu
)
{
  crude_devmenu_gpu_visual_profiler_update( &devmenu->gpu_visual_profiler );
  crude_devmenu_memory_visual_profiler_update( &devmenu->memory_visual_profiler );
  crude_devmenu_texture_inspector_update( &devmenu->texture_inspector );
  crude_devmenu_render_graph_update( &devmenu->render_graph );
  crude_devmenu_gpu_pool_update( &devmenu->gpu_pool );
  crude_devmenu_scene_renderer_update( &devmenu->scene_renderer );
  crude_devmenu_nodes_tree_update( &devmenu->nodes_tree );
  crude_devmenu_node_inspector_update( &devmenu->node_inspector );
  crude_devmenu_viewport_update( &devmenu->viewport );
  crude_devmenu_technique_editor_update( &devmenu->technique_editor );

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
crude_devmenu_handle_input
(
  _In_ crude_devmenu                                      *devmenu
)
{
  crude_input *input = &devmenu->engine->platform.input;

  if ( input->keys[ SDL_SCANCODE_F4 ].pressed )
  {
    devmenu->enabled = !devmenu->enabled;
    
    crude_entity player_controller_node = devmenu->engine->player_controller_node;
    if ( devmenu->enabled )
    {
      crude_platform_show_cursor( &devmenu->engine->platform );
    }
    else
    {
      crude_platform_hide_cursor( &devmenu->engine->platform );
    }
    
    crude_player_controller *player_controller = NULL;
    if ( crude_entity_valid( devmenu->engine->world, player_controller_node ) )
    {
      player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( devmenu->engine->world, player_controller_node, crude_player_controller );
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
  devmenu->engine->scene_renderer.options.debug.hide_debug_gltf = !devmenu->engine->scene_renderer.options.debug.hide_debug_gltf;
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
  devmenu->engine->scene_renderer.options.debug.hide_collision = !devmenu->engine->scene_renderer.options.debug.hide_collision;
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
  crude_entity player_controller_node = devmenu->engine->player_controller_node;
  crude_player_controller *player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( devmenu->engine->world, player_controller_node, crude_player_controller );
  player_controller->fly_mode = !player_controller->fly_mode;
  crude_physics_enable_simulation( &devmenu->engine->physics, !devmenu->engine->physics.simulation_enabled );
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
  crude_engine_commands_manager_push_reload_techniques_command( &devmenu->engine->commands_manager );
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
  _In_ crude_devmenu_gpu_visual_profiler                  *dev_gpu_profiler,
  _In_ crude_devmenu                                      *devmenu
)
{
  dev_gpu_profiler->devmenu = devmenu;
  dev_gpu_profiler->gpu = &devmenu->engine->gpu;
  dev_gpu_profiler->enabled = false;
  dev_gpu_profiler->max_duration = 16.666f;
  dev_gpu_profiler->max_frames = 100u;
  dev_gpu_profiler->current_frame = 0u;
  dev_gpu_profiler->max_visible_depth = 2;
  dev_gpu_profiler->max_queries_per_frame = 32;
  dev_gpu_profiler->allocator = &devmenu->engine->common_allocator;
  dev_gpu_profiler->paused = false;
  dev_gpu_profiler->pipeline_statistics = NULL;
  dev_gpu_profiler->timestamps = CRUDE_CAST( crude_gfx_gpu_time_query*, CRUDE_ALLOCATE( crude_heap_allocator_pack( dev_gpu_profiler->allocator ), sizeof( crude_gfx_gpu_time_query ) * dev_gpu_profiler->max_frames * dev_gpu_profiler->max_queries_per_frame ) );
  dev_gpu_profiler->per_frame_active = CRUDE_CAST( uint16*, CRUDE_ALLOCATE( crude_heap_allocator_pack( dev_gpu_profiler->allocator ), sizeof( uint16 ) * dev_gpu_profiler->max_frames ) );
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
  
  dev_gpu_profiler->framebuffer_pixel_count = dev_gpu_profiler->gpu->renderer_size.x * dev_gpu_profiler->gpu->renderer_size.y;

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
  _In_ crude_devmenu_memory_visual_profiler               *dev_mem_profiler,
  _In_ crude_devmenu                                      *devmenu
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
  CRUDE_ARRAY_PUSH( dev_mem_profiler->allocators_containers, crude_linear_allocator_pack( &devmenu->engine->node_manager.string_linear_allocator ) );
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
  _In_ crude_devmenu_memory_visual_profiler               *dev_mem_profiler
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
  _In_ crude_devmenu_texture_inspector                    *dev_texture_inspector,
  _In_ crude_devmenu                                      *devmenu
)
{
  dev_texture_inspector->devmenu = devmenu;
  dev_texture_inspector->enabled = false;
  dev_texture_inspector->texture_handle = crude_gfx_access_texture( &devmenu->engine->gpu, crude_gfx_render_graph_builder_access_resource_by_name( devmenu->engine->scene_renderer.render_graph->builder, "final" )->resource_info.texture.handle )->handle;
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
  _In_ crude_devmenu_render_graph                         *dev_render_graph,
  _In_ crude_devmenu                                      *devmenu
)
{
  dev_render_graph->devmenu = devmenu;
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
  if ( !dev_render_graph->enabled )
  {
    return;
  }

  if ( ImGui::Begin( "Render Graph Debug" ) )
  {
    if ( ImGui::CollapsingHeader( "Nodes" ) )
    {
      //for ( uint32 n = 0; n < CRUDE_ARRAY_LENGTH( game->render_graph.nodes ); ++n )
      //{
      //  crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( game->render_graph.builder, game->render_graph.nodes[ n ] );
      //
      //  ImGui::Separator( );
      //  ImGui::Text( "Pass: %s", node->name );
      //
      //  ImGui::Text( "\tInputs" );
      //  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( node->inputs ); ++i )
      //  {
      //    crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( game->render_graph.builder, node->inputs[ i ] );
      //    ImGui::Text( "\t\t%s %u", resource->name, resource->resource_info.texture.handle.index );
      //  }
      //
      //  ImGui::Text( "\tOutputs" );
      //  for ( uint32 o = 0; o < CRUDE_ARRAY_LENGTH( node->outputs ); ++o )
      //  {
      //    crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( game->render_graph.builder, node->outputs[ o ] );
      //    ImGui::Text( "\t\t%s %u", resource->name, resource->resource_info.texture.handle.index );
      //  }
      //
      //  ImGui::PushID( n );
      //  ImGui::Checkbox( "Enabled", &node->enabled );
      //  ImGui::PopID( );
      //}
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
  _In_ char const                                         *resource_name,
  _In_ crude_devmenu                                      *devmenu
)
{
  ImGui::Text( "Pool %s, indices used %u, allocated %u", resource_name, resource_pool->used_indices, resource_pool->pool_size );
}

void
crude_devmenu_gpu_pool_initialize
(
  _In_ crude_devmenu_gpu_pool                             *dev_gpu_pool,
  _In_ crude_devmenu                                      *devmenu
)
{
  dev_gpu_pool->devmenu = devmenu;
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
  crude_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.buffers, "Buffers", dev_gpu_pool->devmenu );
  crude_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.textures, "Textures", dev_gpu_pool->devmenu );
  crude_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.pipelines, "Pipelines", dev_gpu_pool->devmenu );
  crude_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.samplers, "Samplers", dev_gpu_pool->devmenu );
  crude_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.descriptor_sets, "DescriptorSets", dev_gpu_pool->devmenu );
  crude_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.descriptor_set_layouts, "DescriptorSetLayouts", dev_gpu_pool->devmenu );
  crude_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.framebuffers, "Framebuffers", dev_gpu_pool->devmenu );
  crude_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.render_passes, "RenderPasses", dev_gpu_pool->devmenu );
  crude_devmenu_gpu_pool_draw_pool_( &dev_gpu_pool->devmenu->engine->gpu.shaders, "Shaders", dev_gpu_pool->devmenu );
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
  _In_ crude_devmenu_scene_renderer                       *dev_scene_rendere,
  _In_ crude_devmenu                                      *devmenu
)
{
  dev_scene_rendere->devmenu = devmenu;
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
  ImGuiTreeNodeFlags                                       tree_node_flags;
  bool                                                     can_open_children_nodes, tree_node_opened;
  
  {
    can_open_children_nodes = false;

    ecs_iter_t it = ecs_children( dev_nodes_tree->devmenu->engine->world, node );
    if ( !CRUDE_ENTITY_HAS_COMPONENT( dev_nodes_tree->devmenu->engine->world, node, crude_gltf ) && ecs_children_next( &it ) )
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

  if ( dev_nodes_tree->selected_node == node )
  {
    tree_node_flags |= ImGuiTreeNodeFlags_Selected;
  }

  tree_node_opened = ImGui::TreeNodeEx( ( void* )( intptr_t )*current_node_index, tree_node_flags, crude_entity_get_name( dev_nodes_tree->devmenu->engine->world, node ) );
  if ( ImGui::IsItemClicked( ImGuiMouseButton_Left ) && !ImGui::IsItemToggledOpen( ) )
  {
    dev_nodes_tree->selected_node = node;
  }

  if ( ImGui::IsItemClicked( ImGuiMouseButton_Right ) )
  {
    dev_nodes_tree->dublicate_node_reference = node;
    crude_string_copy( dev_nodes_tree->dublicate_node_name, crude_entity_get_name( dev_nodes_tree->devmenu->engine->world, node ), sizeof( dev_nodes_tree->dublicate_node_reference ) );
  }

  if ( ImGui::IsItemClicked( 1 ) && !ImGui::IsItemToggledOpen( ) )
  {
    ImGui::OpenPopup( crude_entity_get_name( dev_nodes_tree->devmenu->engine->world, node ) );
  }

  if (ImGui::BeginDragDropSource( ) )
  {
    ImGui::SetDragDropPayload( crude_entity_get_name( dev_nodes_tree->devmenu->engine->world, node ), NULL, 0 );
    ImGui::Text( crude_entity_get_name( dev_nodes_tree->devmenu->engine->world, node ) );
    ImGui::EndDragDropSource( );
  }

  ++( *current_node_index );
  if ( tree_node_opened )
  {
    if ( can_open_children_nodes )
    {
      ecs_iter_t it = ecs_children( dev_nodes_tree->devmenu->engine->world, node );
      while ( ecs_children_next( &it ) )
      {
        for ( int32 i = 0; i < it.count; ++i )
        {
          crude_entity child = crude_entity_from_iterator( &it, i );
          crude_devmenu_nodes_tree_draw_internal_( dev_nodes_tree, child, current_node_index );
        }
      }
    }
    
    ImGui::TreePop( );
  }
}

void
crude_devmenu_nodes_tree_initialize
(
  _In_ crude_devmenu_nodes_tree                           *dev_nodes_tree,
  _In_ crude_devmenu                                      *devmenu
)
{
  dev_nodes_tree->devmenu = devmenu;
  dev_nodes_tree->selected_node = CRUDE_COMPOUNT_EMPTY( crude_entity );
  dev_nodes_tree->dublicate_node_reference = CRUDE_COMPOUNT_EMPTY( crude_entity );
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
  crude_ecs                                               *world;
  uint32                                                   current_node_index;

  if ( !dev_nodes_tree->enabled )
  {
    return;
  }

  world = dev_nodes_tree->devmenu->engine->world;

  if ( !crude_entity_valid( world, dev_nodes_tree->selected_node ) )
  {
    dev_nodes_tree->selected_node = dev_nodes_tree->devmenu->engine->main_node;
  }
  
  if ( crude_entity_valid( world, dev_nodes_tree->dublicate_node_reference ) )
  {
    ImGui::Begin( "Dublicate Node" );
    ImGui::Text( "Reference \"%s\"", crude_entity_get_name( world, dev_nodes_tree->dublicate_node_reference ) );
    ImGui::InputText( "Name", dev_nodes_tree->dublicate_node_name, sizeof( dev_nodes_tree ) );
    if ( ImGui::Button( "Dublicate" ) )
    {
      crude_entity new_node = crude_entity_copy_hierarchy( world, dev_nodes_tree->dublicate_node_reference, dev_nodes_tree->dublicate_node_name, true, true );
      crude_entity_set_parent( world, new_node, crude_entity_get_parent( world, dev_nodes_tree->dublicate_node_reference ) );
      dev_nodes_tree->dublicate_node_reference = CRUDE_COMPOUNT_EMPTY( crude_entity );
      dev_nodes_tree->selected_node = new_node;
    }
    ImGui::End( );
  }
  
  ImGui::Begin( "Scene Node Tree" );
  current_node_index = 0u;
  crude_devmenu_nodes_tree_draw_internal_( dev_nodes_tree, dev_nodes_tree->devmenu->engine->main_node, &current_node_index );
  ImGui::End( );
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
 * Develop Node Inspector
 * 
 ***********************/
void
crude_devmenu_node_inspector_initialize
(
  _In_ crude_devmenu_node_inspector                       *dev_node_inspector,
  _In_ crude_devmenu                                      *devmenu
)
{
  dev_node_inspector->devmenu = devmenu;
  dev_node_inspector->enabled = false;
}

void
crude_devmenu_node_inspector_deinitialize
(
  _In_ crude_devmenu_node_inspector                       *dev_node_inspector
)
{
}

void
crude_devmenu_node_inspector_update
(
  _In_ crude_devmenu_node_inspector                       *dev_node_inspector
)
{
}

void
crude_devmenu_node_inspector_draw
(
  _In_ crude_devmenu_node_inspector                       *dev_node_inspector
)
{
  crude_ecs *world = dev_node_inspector->devmenu->engine->world;
  crude_entity selected_node = dev_node_inspector->devmenu->nodes_tree.selected_node;
  if ( !crude_entity_valid( world, selected_node ) )
  {
    return;
  }

  ImGui::Begin( "Node Inspector" );
  ImGui::Text( "Node: \"%s\"", crude_entity_get_name( world, selected_node ) );

  crude_node_external *node_external = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_node_external );
  if ( node_external )
  {
    ImGui::Text( "External \"%s\"", node_external->path );
  }
  
  crude_transform *transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_transform );
  if ( transform && ImGui::CollapsingHeader( "crude_transform" ) )
  {
    bool transform_edited = false;
    transform_edited |= ImGui::DragFloat3( "Translation", &transform->translation.x, .1f );
    transform_edited |= ImGui::DragFloat3( "Scale", &transform->scale.x, .1f );
    transform_edited |= ImGui::DragFloat4( "Rotation", &transform->rotation.x, .1f );
    if ( transform_edited )
    {
      CRUDE_ENTITY_COMPONENT_MODIFIED( world, selected_node, crude_transform );
    }
  }
  
  crude_free_camera *free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_free_camera );
  if ( free_camera && ImGui::CollapsingHeader( "crude_free_camera" ) )
  {
    ImGui::InputFloat( "Moving Speed", &free_camera->moving_speed_multiplier, .1f );
    ImGui::InputFloat( "Rotating Speed", &free_camera->rotating_speed_multiplier, .1f );
  }
  
  crude_camera *camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_camera );
  if ( camera && ImGui::CollapsingHeader( "crude_camera" ) )
  {
    ImGui::InputFloat( "Far Z", &camera->far_z );
    ImGui::InputFloat( "Near Z", &camera->near_z );
    ImGui::SliderAngle( "FOV Radians", &camera->fov_radians );
    ImGui::InputFloat( "Aspect Ratio", &camera->aspect_ratio );
    if ( ImGui::Button( "Set Active" ) )
    {
      dev_node_inspector->devmenu->engine->camera_node = selected_node;
    }
  }
  
  crude_light *light = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_light );
  if ( light && ImGui::CollapsingHeader( "crude_light" ) )
  {
    ImGui::ColorEdit3( "color", &light->color.x );
    ImGui::InputFloat( "intensity", &light->intensity );
    ImGui::InputFloat( "radius", &light->radius );
  }
  
  crude_gltf *gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_gltf );
  if ( gltf && ImGui::CollapsingHeader( "crude_gltf" ) )
  {
    ImGui::Text( "Relative Path \"%s\"", gltf->original_path );
    ImGui::Text( "Absolute Path \"%s\"", gltf->path );
    ImGui::Checkbox( "Hidden", &gltf->hidden );
  }
  
  crude_physics_character_body_handle *dynamic_body = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_physics_character_body_handle );
  if ( dynamic_body && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_physics_character_body_handle ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_physics_character_body_handle )( world, selected_node, dynamic_body, &dev_node_inspector->devmenu->engine->node_manager );
  }
  
  crude_physics_static_body_handle *static_body = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_physics_static_body_handle );
  if ( static_body && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_physics_static_body_handle ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_physics_static_body_handle )( world, selected_node, static_body, &dev_node_inspector->devmenu->engine->node_manager );
  }
  
  crude_physics_collision_shape *collision_shape = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_physics_collision_shape );
  if ( collision_shape && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_physics_collision_shape ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_physics_collision_shape )( world, selected_node, collision_shape, &dev_node_inspector->devmenu->engine->node_manager );
  }
  
  crude_player_controller *player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_player_controller );
  if ( player_controller && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_player_controller ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_player_controller )( world, selected_node, player_controller, &dev_node_inspector->devmenu->engine->node_manager );
  }
  
  crude_debug_collision *debug_collision = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_debug_collision );
  if ( debug_collision && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_debug_collision ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_debug_collision )( world, selected_node, debug_collision, &dev_node_inspector->devmenu->engine->node_manager );
  }

  crude_debug_gltf *debug_gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_debug_gltf );
  if ( debug_gltf && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_debug_gltf ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_debug_gltf )( world, selected_node, debug_gltf, &dev_node_inspector->devmenu->engine->node_manager );
  }

  crude_node_runtime *runtime_node = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_node_runtime );
  if ( runtime_node && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_node_runtime ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_node_runtime )( world, selected_node, runtime_node, &dev_node_inspector->devmenu->engine->node_manager );
  }
  
  ImGui::End( );
}

void
crude_devmenu_node_inspector_callback
(
  _In_ crude_devmenu                                      *devmenu
)
{
  devmenu->node_inspector.enabled = !devmenu->node_inspector.enabled;
}

/***********************
 * 
 * Develop Viewport
 * 
 ***********************/
void
crude_devmenu_viewport_initialize
(
  _In_ crude_devmenu_viewport                             *dev_viewport,
  _In_ crude_devmenu                                      *devmenu
)
{
  dev_viewport->devmenu = devmenu;
  dev_viewport->enabled = true;
}

void
crude_devmenu_viewport_deinitialize
(
  _In_ crude_devmenu_viewport                             *dev_viewport
)
{
}

void
crude_devmenu_viewport_update
(
  _In_ crude_devmenu_viewport                             *dev_viewport
)
{
}

void
crude_devmenu_viewport_draw
(
  _In_ crude_devmenu_viewport                             *dev_viewport
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
crude_devmenu_viewport_callback
(
  _In_ crude_devmenu                                      *devmenu
)
{
  devmenu->viewport.enabled = !devmenu->viewport.enabled;
}

/***********************
 * 
 * Develop Technique Editor
 * 
 ***********************/
void
crude_devmenu_technique_editor_initialize
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor,
  _In_ crude_devmenu                                      *devmenu
)
{
  devmenu->technique_editor.enabled = false;

  devmenu_technique_editor->ax_context = ax::NodeEditor::CreateEditor( );
}

void
crude_devmenu_technique_editor_deinitialize
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  ax::NodeEditor::DestroyEditor( devmenu_technique_editor->ax_context );
}

void
crude_devmenu_technique_editor_update
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
}

void
crude_devmenu_technique_editor_draw
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  if ( !devmenu_technique_editor->enabled )
  {
    return;
  }

  // !TODO
  // ImGui::ShowMetricsWindow( );

  ImGuiIO *imgui_io = &ImGui::GetIO();
  ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
  ImGui::SetNextWindowSize( imgui_io->DisplaySize);
  const auto windowBorderSize = ImGui::GetStyle().WindowBorderSize;
  const auto windowRounding   = ImGui::GetStyle().WindowRounding;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::Begin("Technique Editor", nullptr, ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, windowBorderSize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, windowRounding);

  ax::NodeEditor::SetCurrentEditor( devmenu_technique_editor->ax_context );
  ax::NodeEditor::Begin( "Technique Editor Node", ImVec2( 0.0, 0.0f ) );

  int unique_id = 1;
  ax::NodeEditor::BeginNode(unique_id++);
      ImGui::Text("Node A");
      ax::NodeEditor::BeginPin(unique_id++, ax::NodeEditor::PinKind::Input);
          ImGui::Text("-> In");
      ax::NodeEditor::EndPin();
      ImGui::SameLine();
      ax::NodeEditor::BeginPin(unique_id++, ax::NodeEditor::PinKind::Output);
          ImGui::Text("Out ->");
      ax::NodeEditor::EndPin();
  ax::NodeEditor::EndNode();

  ax::NodeEditor::End();
  ax::NodeEditor::SetCurrentEditor(nullptr);
  
  ImGui::PopStyleVar(2);
  ImGui::End();
  ImGui::PopStyleVar(2);
}

void
crude_devmenu_technique_editor_callback
(
  _In_ crude_devmenu                                      *devmenu
)
{
  devmenu->technique_editor.enabled = !devmenu->technique_editor.enabled;
}

#endif