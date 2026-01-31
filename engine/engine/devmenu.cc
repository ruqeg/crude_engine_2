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
#include <engine/gui/hardcoded_icon.h>
#include <engine/gui/blueprint_node_builder.h>

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
  devmenu->dev_heap_allocator = &engine->develop_heap_allocator;
  devmenu->dev_stack_allocator = &engine->develop_temporary_allocator;
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
static inline ImRect
ImGui_GetItemRect_
(
)
{
  return ImRect( ImGui::GetItemRectMin( ), ImGui::GetItemRectMax( ) );
}

static inline ImRect
ImRect_Expanded_
(
  _In_ ImRect const                                        rect,
  _In_ float32                                             x,
  _In_ float32                                             y
)
{
  ImRect result = rect;
  result.Min.x -= x;
  result.Min.y -= y;
  result.Max.x += x;
  result.Max.y += y;
  return result;
}

static bool
splitter_
(
  _In_ bool                                                split_vertically,
  _In_ float                                               thickness,
  _In_ float                                              *size1,
  _In_ float                                              *size2,
  _In_ float                                               min_size1,
  _In_ float                                               min_size2,
  _In_ float                                               splitter_long_axis_size
)
{
  ImGuiContext& g = *GImGui;
  ImGuiWindow* window = g.CurrentWindow;
  ImGuiID id = window->GetID("##Splitter");
  ImRect bb;
  bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
  bb.Max = bb.Min + ImGui::CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
  return ImGui::SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
}

crude_technique_editor_link
crude_technique_editor_link_empty_
(
)
{
  crude_technique_editor_link link = CRUDE_COMPOUNT_EMPTY( crude_technique_editor_link );
  link.color = ImColor(255,255,255);
  return link;
}

int32
crude_devmenu_technique_editor_get_next_id_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  return devmenu_technique_editor->next_id++;
}

ax::NodeEditor::LinkId
crude_devmenu_technique_editor_get_next_link_id_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  return ax::NodeEditor::LinkId( crude_devmenu_technique_editor_get_next_id_( devmenu_technique_editor ) );
}

void
crude_devmenu_technique_editor_touch_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor,
  _In_ ax::NodeEditor::NodeId                              id
)
{
  CRUDE_HASHMAP_SET( devmenu_technique_editor->node_id_to_touch_time, CRUDE_CAST( uint64, id.AsPointer( ) ), devmenu_technique_editor->touch_time );
}

float32
crude_devmenu_technique_editor_get_touch_progress_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor,
  _In_ ax::NodeEditor::NodeId                              id
)
{
  int64 index = CRUDE_HASHMAP_GET_INDEX( devmenu_technique_editor->node_id_to_touch_time, CRUDE_CAST( uint64, id.AsPointer( ) ) );
  
  if ( index != -1 && devmenu_technique_editor->node_id_to_touch_time[ index ].value > 0.0f)
  {
      return ( devmenu_technique_editor->touch_time - devmenu_technique_editor->node_id_to_touch_time[ index ].value ) / devmenu_technique_editor->touch_time;
  }

  return 0.0f;
}

void
crude_devmenu_technique_editor_update_touch_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  float32 delta_time = ImGui::GetIO( ).DeltaTime;

  for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( devmenu_technique_editor->node_id_to_touch_time ); ++i )
  {
    if ( !crude_hashmap_backet_key_valid( devmenu_technique_editor->node_id_to_touch_time[ i ].key ) )
    {
      continue;
    }
    
    if ( devmenu_technique_editor->node_id_to_touch_time[ i ].value > 0.f )
    {
      devmenu_technique_editor->node_id_to_touch_time[ i ].value -= delta_time;
    }
  }
}

crude_technique_editor_node*
crude_devmenu_technique_editor_find_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor,
  _In_ ax::NodeEditor::NodeId                              id
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->nodes ); ++i )
  {
    if ( devmenu_technique_editor->nodes[ i ].id == id )
    {
        return &devmenu_technique_editor->nodes[ i ];
    }
  }

  return NULL;
}

crude_technique_editor_link*
crude_devmenu_technique_editor_find_link_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor,
  _In_ ax::NodeEditor::LinkId                              id
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->links ); ++i )
  {
    if ( devmenu_technique_editor->links[ i ].id == id )
    {
        return &devmenu_technique_editor->links[ i ];
    }
  }

  return NULL;
}

crude_technique_editor_pin*
crude_devmenu_technique_editor_find_pin_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor,
  _In_ ax::NodeEditor::PinId                               id
)
{
  if ( !id )
  {
    return NULL;
  }
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->nodes ); ++i )
  {
    crude_technique_editor_node *node = &devmenu_technique_editor->nodes[ i ];

    for ( uint32 k = 0; k < CRUDE_ARRAY_LENGTH( node->inputs ); ++k )
    {
      if ( node->inputs[ k ].id == id )
      {
        return &node->inputs[ k ];
      }
    }
    
    for ( uint32 k = 0; k < CRUDE_ARRAY_LENGTH( node->outputs ); ++k )
    {
      if ( node->outputs[ k ].id == id )
      {
        return &node->outputs[ k ];
      }
    }
  }

  return NULL;
}

bool
crude_devmenu_technique_editor_is_pin_linked_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor,
  _In_ ax::NodeEditor::PinId                              id
)
{
  if ( !id )
  {
    return false;
  }
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->links ); ++i )
  {
    if ( devmenu_technique_editor->links[ i ].start_pin_id == id || devmenu_technique_editor->links[ i ].end_pin_id == id )
    {
      return true;
    }
  }

  return false;
}

bool
crude_devmenu_technique_editor_can_create_link_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor,
  _In_ crude_technique_editor_pin                         *a,
  _In_ crude_technique_editor_pin                         *b
)
{
  if ( !a || !b || a == b || a->kind == b->kind || a->type != b->type || a->node == b->node )
  {
    return false;
  }
  return true;
}

void
crude_devmenu_technique_editor_build_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor,
  _In_ crude_technique_editor_node                        *node
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( node->inputs ); ++i )
  {
    node->inputs[ i ].node = node;
    node->inputs[ i ].kind = CRUDE_TECHNIQUE_EDITOR_PIN_KIND_INPUT;
  }
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( node->outputs ); ++i )
  {
    node->outputs[ i ].node = node;
    node->outputs[ i ].kind = CRUDE_TECHNIQUE_EDITOR_PIN_KIND_OUTPUT;
  }
}

#define CRUDE_CREATE_NODE( var_name, var_color )\
  crude_technique_editor_node new_node;\
  crude_technique_editor_node_initialize( &new_node, devmenu_technique_editor->devmenu->dev_heap_allocator );\
  new_node.id = crude_devmenu_technique_editor_get_next_id_( devmenu_technique_editor );\
  new_node.name = var_name;\
  new_node.color = var_color;

#define CRUDE_CREATE_OUTPUT_PIN( var_name, var_type )\
{\
  crude_technique_editor_pin new_pin = crude_technique_editor_pin_empty( );\
  new_pin.id = crude_devmenu_technique_editor_get_next_id_( devmenu_technique_editor );\
  new_pin.name = var_name;\
  new_pin.type = var_type;\
  CRUDE_ARRAY_PUSH( new_node.outputs, new_pin );\
}
#define CRUDE_CREATE_INPUT_PIN( var_name, var_type )\
{\
  crude_technique_editor_pin new_pin = crude_technique_editor_pin_empty( );\
  new_pin.id = crude_devmenu_technique_editor_get_next_id_( devmenu_technique_editor );\
  new_pin.name = var_name;\
  new_pin.type = var_type;\
  CRUDE_ARRAY_PUSH( new_node.inputs, new_pin );\
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_input_action_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "InputAction Fire", CRUDE_COMPOUNT( XMFLOAT4, { 255 / 255.f, 128 / 255.f, 128 / 255.f, 1.f } ) );
  CRUDE_CREATE_OUTPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_DELEGATE );
  CRUDE_CREATE_OUTPUT_PIN( "Pressed", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_OUTPUT_PIN( "Released", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_branch_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "Branch", CRUDE_COMPOUNT( XMFLOAT4, { 1.f, 1.f, 1.f, 1.f } ) );
  CRUDE_CREATE_INPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_INPUT_PIN( "Condition", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_BOOL );
  CRUDE_CREATE_OUTPUT_PIN( "True", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_OUTPUT_PIN( "False", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );

}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_do_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "Do N", CRUDE_COMPOUNT( XMFLOAT4, { 1.f, 1.f, 1.f, 1.f } ) );
  CRUDE_CREATE_INPUT_PIN( "Enter", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_INPUT_PIN( "N", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_INT );
  CRUDE_CREATE_INPUT_PIN( "Reset", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_OUTPUT_PIN( "True", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_OUTPUT_PIN( "False", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_INT );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_output_action_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "OutputAction", CRUDE_COMPOUNT( XMFLOAT4, { 1.f, 1.f, 1.f, 1.f } ) );
  CRUDE_CREATE_INPUT_PIN( "Sample", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOAT );
  CRUDE_CREATE_INPUT_PIN( "Event", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_DELEGATE );
  CRUDE_CREATE_OUTPUT_PIN( "Condition", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_BOOL );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_print_string_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "Print String", CRUDE_COMPOUNT( XMFLOAT4, { 1.f, 1.f, 1.f, 1.f } ) );
  CRUDE_CREATE_INPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_INPUT_PIN( "In String", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_STRING );
  CRUDE_CREATE_OUTPUT_PIN( "Condition", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_message_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "", CRUDE_COMPOUNT( XMFLOAT4, { 128/ 255.f, 195/ 255.f, 248 / 255.f, 1.f } ) );
  new_node.type = CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_SIMPLE;
  CRUDE_CREATE_OUTPUT_PIN( "Message", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_STRING );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_set_timer_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "Set Timer", CRUDE_COMPOUNT( XMFLOAT4, { 128 / 255.f, 195 / 255.f, 248 / 255.f, 1.f } ) );
  CRUDE_CREATE_INPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_INPUT_PIN( "Object", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_OBJECT );
  CRUDE_CREATE_INPUT_PIN( "Function Name", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FUNCTION );
  CRUDE_CREATE_INPUT_PIN( "Time", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOAT );
  CRUDE_CREATE_INPUT_PIN( "Looping", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_BOOL );
  CRUDE_CREATE_OUTPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_less_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "<", CRUDE_COMPOUNT( XMFLOAT4, { 128 / 255.f, 195 / 255.f, 248 / 255.f, 1.f } ) );
  new_node.type = CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_SIMPLE;
  CRUDE_CREATE_INPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOAT );
  CRUDE_CREATE_INPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOAT );
  CRUDE_CREATE_OUTPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOAT );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_weird_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "o.O", CRUDE_COMPOUNT( XMFLOAT4, { 128 / 255.f, 195 / 255.f, 248 / 255.f, 1.f } ) );
  new_node.type = CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_SIMPLE;
  CRUDE_CREATE_INPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOAT );
  CRUDE_CREATE_OUTPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOAT );
  CRUDE_CREATE_OUTPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOAT );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_trace_by_channel_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "Single Line Trace by Channel", CRUDE_COMPOUNT( XMFLOAT4, { 128 / 255.f, 195 / 255.f, 248 / 255.f, 1.f } ) );
  CRUDE_CREATE_INPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_INPUT_PIN( "Start", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_INPUT_PIN( "End", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_INT );
  CRUDE_CREATE_INPUT_PIN( "Trace Channel", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOAT );
  CRUDE_CREATE_INPUT_PIN( "Trace Complex", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_BOOL );
  CRUDE_CREATE_INPUT_PIN( "Actors to Ignore", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_INT );
  CRUDE_CREATE_INPUT_PIN( "Draw Debug Type", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_BOOL );
  CRUDE_CREATE_INPUT_PIN( "Ignore Self", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_BOOL );
  CRUDE_CREATE_OUTPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_OUTPUT_PIN( "Out Hit", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_OUTPUT_PIN( "Return Value", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_BOOL );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_tree_sequence_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "Sequence", CRUDE_COMPOUNT( XMFLOAT4, { 1, 1, 1, 1.f } ) );
  new_node.type = CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_TREE;
  CRUDE_CREATE_INPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_OUTPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_tree_task_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "Move To", CRUDE_COMPOUNT( XMFLOAT4, { 1, 1, 1, 1.f } ) );
  new_node.type = CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_TREE;
  CRUDE_CREATE_INPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_tree_task_2node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "Random Wait", CRUDE_COMPOUNT( XMFLOAT4, { 1, 1, 1, 1.f } ) );
  new_node.type = CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_TREE;
  CRUDE_CREATE_INPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_comment_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "Test Comment", CRUDE_COMPOUNT( XMFLOAT4, { 1, 1, 1, 1.f } ) );
  new_node.type = CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_COMMENT;
  new_node.size = CRUDE_COMPOUNT( XMFLOAT2, {300, 200 });
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_houdini_transform_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "Transform", CRUDE_COMPOUNT( XMFLOAT4, { 1, 1, 1, 1.f } ) );
  new_node.type = CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_HOUDINI;
  CRUDE_CREATE_INPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_OUTPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

crude_technique_editor_node*
crude_devmenu_technique_editor_spawn_houdini_group_node_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  CRUDE_CREATE_NODE( "Group", CRUDE_COMPOUNT( XMFLOAT4, { 1, 1, 1, 1.f } ) );
  new_node.type = CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_HOUDINI;
  CRUDE_CREATE_INPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_INPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  CRUDE_CREATE_OUTPUT_PIN( "", CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW );
  crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &new_node );
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->nodes, new_node );
  return &CRUDE_ARRAY_BACK( devmenu_technique_editor->nodes );
}

void
crude_devmenu_technique_editor_build_nodes_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->nodes ); ++i )
  {
    crude_devmenu_technique_editor_build_node_( devmenu_technique_editor, &devmenu_technique_editor->nodes[ i ] );
  }
}

bool
crude_devmenu_technique_editor_config_save_node_settings_
(
  _In_ ax::NodeEditor::NodeId                              node_id,
  _In_ char const*                                         data,
  _In_ size_t                                              size,
  _In_ ax::NodeEditor::SaveReasonFlags                     reason,
  _In_ void                                               *user_data
)
{
  crude_devmenu_technique_editor                          *devmenu_technique_editor;
  crude_technique_editor_node                             *node;

  devmenu_technique_editor = CRUDE_CAST( crude_devmenu_technique_editor*, user_data );

  node = crude_devmenu_technique_editor_find_node_( devmenu_technique_editor, node_id );
  if ( !node )
  {
    return false;
  }

  crude_string_copy( node->state, data, size );
  crude_devmenu_technique_editor_touch_node_( devmenu_technique_editor, node_id );

  return true;
}

size_t
crude_devmenu_technique_editor_config_load_node_settings_
(
  _In_ ax::NodeEditor::NodeId                              node_id, 
  _In_ char                                               *data,
  _In_ void                                               *user_data
)
{
  crude_devmenu_technique_editor                          *devmenu_technique_editor;
  crude_technique_editor_node                             *node;
  uint64                                                   length;

  devmenu_technique_editor = CRUDE_CAST( crude_devmenu_technique_editor*, user_data );

  node = crude_devmenu_technique_editor_find_node_( devmenu_technique_editor, node_id );
  if ( !node )
  {
    return 0;
  }
  
  if ( data != NULL )
  {
    length = crude_string_copy_unknow_length( data, node->state, sizeof( node->state ) );
  }
  return length;
}

void
crude_devmenu_technique_editor_on_start_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  crude_technique_editor_node                             *node;
  ax::NodeEditor::Config                                   config;

  config.SettingsFile = "Blueprints.json";
  config.UserPointer = devmenu_technique_editor;
  config.LoadNodeSettings = crude_devmenu_technique_editor_config_load_node_settings_;
  config.SaveNodeSettings = crude_devmenu_technique_editor_config_save_node_settings_;

  devmenu_technique_editor->ax_context = ax::NodeEditor::CreateEditor(&config);
  ax::NodeEditor::SetCurrentEditor( devmenu_technique_editor->ax_context );

  node = crude_devmenu_technique_editor_spawn_input_action_node_( devmenu_technique_editor );
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(-252, 220));
  node = crude_devmenu_technique_editor_spawn_branch_node_( devmenu_technique_editor );
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(-300, 351));
  node = crude_devmenu_technique_editor_spawn_do_node_( devmenu_technique_editor );
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(-238, 504));
  node = crude_devmenu_technique_editor_spawn_output_action_node_( devmenu_technique_editor );
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(71, 80));
  node = crude_devmenu_technique_editor_spawn_set_timer_node_( devmenu_technique_editor );
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(168, 316));

  node = crude_devmenu_technique_editor_spawn_tree_sequence_node_( devmenu_technique_editor );
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(1028, 329));
  node = crude_devmenu_technique_editor_spawn_tree_task_node_( devmenu_technique_editor );
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(1204, 458));
  node = crude_devmenu_technique_editor_spawn_tree_task_2node_( devmenu_technique_editor );
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(868, 538));

  node = crude_devmenu_technique_editor_spawn_comment_( devmenu_technique_editor );
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(112, 576));
  ax::NodeEditor::SetGroupSize(node->id, ImVec2(384, 154));
  node = crude_devmenu_technique_editor_spawn_comment_( devmenu_technique_editor );
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(800, 224));
  ax::NodeEditor::SetGroupSize(node->id, ImVec2(640, 400));

  node = crude_devmenu_technique_editor_spawn_less_node_( devmenu_technique_editor );             
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(366, 652));
  node = crude_devmenu_technique_editor_spawn_weird_node_( devmenu_technique_editor );            
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(144, 652));
  node = crude_devmenu_technique_editor_spawn_message_node_( devmenu_technique_editor );          
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(-348, 698));
  node = crude_devmenu_technique_editor_spawn_print_string_node_( devmenu_technique_editor );      
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(-69, 652));

  node = crude_devmenu_technique_editor_spawn_houdini_transform_node_( devmenu_technique_editor );
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(500, -70));
  node = crude_devmenu_technique_editor_spawn_houdini_group_node_( devmenu_technique_editor );
  ax::NodeEditor::SetNodePosition(node->id, ImVec2(500, 42));

  ax::NodeEditor::NavigateToContent();

  crude_devmenu_technique_editor_build_nodes_( devmenu_technique_editor );

  crude_technique_editor_link link = crude_technique_editor_link_empty_( );
  link.id = crude_devmenu_technique_editor_get_next_link_id_( devmenu_technique_editor );
  link.start_pin_id = devmenu_technique_editor->nodes[ 5 ].outputs[ 0 ].id;
  link.end_pin_id = devmenu_technique_editor->nodes[ 6 ].inputs[ 0 ].id;
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->links, link );
  
  link.id = crude_devmenu_technique_editor_get_next_link_id_( devmenu_technique_editor );
  link.start_pin_id = devmenu_technique_editor->nodes[ 5 ].outputs[ 0 ].id;
  link.end_pin_id = devmenu_technique_editor->nodes[ 7 ].inputs[ 0 ].id;
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->links, link );
  
  link.id = crude_devmenu_technique_editor_get_next_link_id_( devmenu_technique_editor );
  link.start_pin_id = devmenu_technique_editor->nodes[ 14 ].outputs[ 0 ].id;
  link.end_pin_id = devmenu_technique_editor->nodes[ 15 ].inputs[ 0 ].id;
  CRUDE_ARRAY_PUSH( devmenu_technique_editor->links, link );
}

void
crude_devmenu_technique_editor_on_stop_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  ax::NodeEditor::DestroyEditor( devmenu_technique_editor->ax_context );
}

ImColor
crude_technique_editor_get_icon_color
(
  _In_ crude_technique_editor_pin_type                     type
)
{
  switch (type)
  {
    default:
    case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW:     return ImColor(255, 255, 255);
    case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_BOOL:     return ImColor(220,  48,  48);
    case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_INT:      return ImColor( 68, 201, 156);
    case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOAT:    return ImColor(147, 226,  74);
    case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_STRING:   return ImColor(124,  21, 153);
    case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_OBJECT:   return ImColor( 51, 150, 215);
    case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FUNCTION: return ImColor(218,   0, 183);
    case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_DELEGATE: return ImColor(255,  48,  48);
  }
}

void
crude_technique_editor_draw_pin_icon
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor,
  _In_ crude_technique_editor_pin const                   *pin,
  _In_ bool                                                connected,
  _In_ int                                                 alpha
)
{
  
  crude_gui_hardcoded_icon_type                            icon_type;
  ImColor                                                  icon_color;

  icon_color = crude_technique_editor_get_icon_color( pin->type );
  icon_color.Value.w = alpha / 255.0f;

  switch ( pin->type )
  {
  case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOW:     icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_FLOW;   break;
  case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_BOOL:     icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE; break;
  case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_INT:      icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE; break;
  case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FLOAT:    icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE; break;
  case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_STRING:   icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE; break;
  case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_OBJECT:   icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE; break;
  case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_FUNCTION: icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE; break;
  case CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_DELEGATE: icon_type = CRUDE_GUI_HARDCODED_ICON_TYPE_SQUARE; break;
  default:
    return;
  }
  
  crude_gui_draw_icon(
    XMFLOAT2 { CRUDE_CAST( float32, devmenu_technique_editor->pin_icon_size ), CRUDE_CAST( float32, devmenu_technique_editor->pin_icon_size ) },
    icon_type,
    connected,
    XMFLOAT4 { icon_color.Value.x, icon_color.Value.y, icon_color.Value.z, icon_color.Value.w },
    XMFLOAT4 { 32 / 255.f, 32/ 255.f, 32/ 255.f, alpha / 255.0f } );
}

void
crude_devmenu_technique_editor_show_style_editor
(
  _In_opt_ bool                                           *show
)
{
  if ( !ImGui::Begin( "Style", show ) )
  {
    ImGui::End();
    return;
  }

  float32 pane_width = ImGui::GetContentRegionAvail().x;

  ax::NodeEditor::Style *editor_style = &ax::NodeEditor::GetStyle( );

  ImGui::BeginHorizontal("Style buttons", ImVec2(pane_width, 0), 1.0f);
  ImGui::TextUnformatted("Values");
  ImGui::Spring();
  if ( ImGui::Button("Reset to defaults") )
  {
    *editor_style = ax::NodeEditor::Style( );
  }

  ImGui::EndHorizontal();
  ImGui::Spacing();
  ImGui::DragFloat4("Node Padding", &editor_style->NodePadding.x, 0.1f, 0.0f, 40.0f);
  ImGui::DragFloat("Node Rounding", &editor_style->NodeRounding, 0.1f, 0.0f, 40.0f);
  ImGui::DragFloat("Node Border Width", &editor_style->NodeBorderWidth, 0.1f, 0.0f, 15.0f);
  ImGui::DragFloat("Hovered Node Border Width", &editor_style->HoveredNodeBorderWidth, 0.1f, 0.0f, 15.0f);
  ImGui::DragFloat("Hovered Node Border Offset", &editor_style->HoverNodeBorderOffset, 0.1f, -40.0f, 40.0f);
  ImGui::DragFloat("Selected Node Border Width", &editor_style->SelectedNodeBorderWidth, 0.1f, 0.0f, 15.0f);
  ImGui::DragFloat("Selected Node Border Offset", &editor_style->SelectedNodeBorderOffset, 0.1f, -40.0f, 40.0f);
  ImGui::DragFloat("Pin Rounding", &editor_style->PinRounding, 0.1f, 0.0f, 40.0f);
  ImGui::DragFloat("Pin Border Width", &editor_style->PinBorderWidth, 0.1f, 0.0f, 15.0f);
  ImGui::DragFloat("Link Strength", &editor_style->LinkStrength, 1.0f, 0.0f, 500.0f);
  //ImVec2  SourceDirection;
  //ImVec2  TargetDirection;
  ImGui::DragFloat("Scroll Duration", &editor_style->ScrollDuration, 0.001f, 0.0f, 2.0f);
  ImGui::DragFloat("Flow Marker Distance", &editor_style->FlowMarkerDistance, 1.0f, 1.0f, 200.0f);
  ImGui::DragFloat("Flow Speed", &editor_style->FlowSpeed, 1.0f, 1.0f, 2000.0f);
  ImGui::DragFloat("Flow Duration", &editor_style->FlowDuration, 0.001f, 0.0f, 5.0f);
  //ImVec2  PivotAlignment;
  //ImVec2  PivotSize;
  //ImVec2  PivotScale;
  //float   PinCorners;
  //float   PinRadius;
  //float   PinArrowSize;
  //float   PinArrowWidth;
  ImGui::DragFloat("Group Rounding", &editor_style->GroupRounding, 0.1f, 0.0f, 40.0f);
  ImGui::DragFloat("Group Border Width", &editor_style->GroupBorderWidth, 0.1f, 0.0f, 15.0f);

  ImGui::Separator();

  static ImGuiColorEditFlags edit_mode = ImGuiColorEditFlags_DisplayRGB;
  ImGui::BeginHorizontal("Color Mode", ImVec2(pane_width, 0), 1.0f);
  ImGui::TextUnformatted("Filter Colors");
  ImGui::Spring();
  ImGui::RadioButton("RGB", &edit_mode, ImGuiColorEditFlags_DisplayRGB);
  ImGui::Spring(0);
  ImGui::RadioButton("HSV", &edit_mode, ImGuiColorEditFlags_DisplayHSV);
  ImGui::Spring(0);
  ImGui::RadioButton("HEX", &edit_mode, ImGuiColorEditFlags_DisplayHex);
  ImGui::EndHorizontal();

  static ImGuiTextFilter filter;
  filter.Draw("##filter", pane_width);

  ImGui::Spacing();

  ImGui::PushItemWidth(-160);
  for ( int32 i = 0; i < ax::NodeEditor::StyleColor_Count; ++i )
  {
    char const *name = ax::NodeEditor::GetStyleColorName( CRUDE_CAST( ax::NodeEditor::StyleColor, i ) );
    if ( !filter.PassFilter( name ) )
    {
      continue;
    }

    ImGui::ColorEdit4( name, &editor_style->Colors[ i ].x, edit_mode );
  }

  ImGui::PopItemWidth();

  ImGui::End();
}

void
crude_devmenu_technique_editor_show_left_pane_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor,
  _In_ float32                                             pane_width
)
{
  uint64 temporary_allocator_marker = crude_stack_allocator_get_marker( devmenu_technique_editor->devmenu->dev_stack_allocator );

  ImGuiIO *io = &ImGui::GetIO();

  ImGui::BeginChild("Selection", ImVec2(pane_width, 0));

  pane_width = ImGui::GetContentRegionAvail().x;

  static bool show_style_editor = false;

  ImGui::BeginHorizontal("Style Editor", ImVec2(pane_width, 0));
  ImGui::Spring(0.0f, 0.0f);
  if (ImGui::Button("Zoom to Content"))
  {
    ax::NodeEditor::NavigateToContent( );
  }
  
  ImGui::Spring(0.0f);
  if (ImGui::Button("Show Flow"))
  {
    for ( uint64 i = 0; i < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->links ); ++i )
    {
      ax::NodeEditor::Flow( devmenu_technique_editor->links[ i ].id );
    }
  }

  ImGui::Spring();
  
  if (ImGui::Button("Edit Style"))
  {
    show_style_editor = true;
  }

  ImGui::EndHorizontal();
  ImGui::Checkbox("Show Ordinals", &devmenu_technique_editor->show_ordinals);

  if ( show_style_editor )
  {
    crude_devmenu_technique_editor_show_style_editor( &show_style_editor );
  }

  ax::NodeEditor::NodeId *selected_nodes;
  ax::NodeEditor::LinkId *selected_links;

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( selected_nodes, ax::NodeEditor::GetSelectedObjectCount( ), crude_stack_allocator_pack( devmenu_technique_editor->devmenu->dev_stack_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( selected_links, ax::NodeEditor::GetSelectedObjectCount( ), crude_stack_allocator_pack( devmenu_technique_editor->devmenu->dev_stack_allocator ) );

  int32 node_count = ax::NodeEditor::GetSelectedNodes( selected_nodes, ax::NodeEditor::GetSelectedObjectCount( ) );
  int32 link_count = ax::NodeEditor::GetSelectedLinks( selected_links, ax::NodeEditor::GetSelectedObjectCount( ) );

  CRUDE_ARRAY_SET_LENGTH( selected_nodes, node_count );
  CRUDE_ARRAY_SET_LENGTH( selected_links, link_count );

  crude_gfx_texture *save_icon_texture = crude_gfx_access_texture( &devmenu_technique_editor->devmenu->engine->gpu, devmenu_technique_editor->save_icon_texture_handle );
  crude_gfx_texture *restore_icon_texture = crude_gfx_access_texture( &devmenu_technique_editor->devmenu->engine->gpu, devmenu_technique_editor->restore_icon_texture_handle );
  int32 saveIconWidth = save_icon_texture ? save_icon_texture->width : 100;
  int32 saveIconHeight = save_icon_texture ? save_icon_texture->width : 100;
  int32 restoreIconWidth = restore_icon_texture ? restore_icon_texture->width : 0;
  int32 restoreIconHeight = restore_icon_texture ? restore_icon_texture->width : 0;

  ImGui::GetWindowDrawList()->AddRectFilled(
    ImGui::GetCursorScreenPos(),
    ImGui::GetCursorScreenPos() + ImVec2(pane_width, ImGui::GetTextLineHeight()),
    ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f );

  ImGui::Spacing(); ImGui::SameLine();
  ImGui::TextUnformatted("Nodes");
  ImGui::Indent();
  
  for ( uint64 node_index = 0; node_index < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->nodes ); ++node_index )
  {
    crude_technique_editor_node *node = &devmenu_technique_editor->nodes[ node_index ];
    ImGui::PushID( node->id.AsPointer( ) );

    ImVec2 start = ImGui::GetCursorScreenPos();

    float32 progress = crude_devmenu_technique_editor_get_touch_progress_( devmenu_technique_editor, node->id );
    if ( progress )
    {
      ImGui::GetWindowDrawList()->AddLine(
        start + ImVec2(-8, 0),
        start + ImVec2(-8, ImGui::GetTextLineHeight()),
        IM_COL32(255, 0, 0, 255 - (int)(255 * progress)), 4.0f);
    }

    bool is_selected = false;
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( selected_nodes ); ++i )
    {
      if ( selected_nodes[ i ] == node->id )
      {
        is_selected = true;
        break;
      }
    }

    ImGui::SetNextItemAllowOverlap( );
    
    char node_name_with_id[ sizeof( node->name ) + 128 ]{};
    crude_snprintf( node_name_with_id, sizeof( node_name_with_id ), "%s##%u", node->name, CRUDE_CAST( uint64, node->id.AsPointer( ) ) );

    if ( ImGui::Selectable( node_name_with_id, &is_selected ) )
    {
      if ( io->KeyCtrl )
      {
        if ( is_selected )
        {
          ax::NodeEditor::SelectNode( node->id, true );
        }
        else
        {
          ax::NodeEditor::DeselectNode( node->id );
        }
      }
      else
      {
        ax::NodeEditor::SelectNode( node->id, false );
      }

      ax::NodeEditor::NavigateToSelection( );
    }

    if ( ImGui::IsItemHovered( ) && node->state[ 0 ] )
    {
      ImGui::SetTooltip( "State: %s", node->state );
    }

    char id_str[ 128 ] = {};
    crude_snprintf( id_str, sizeof( id_str ), "(%u)", CRUDE_CAST( uint64, node->id.AsPointer( ) ) );
    ImVec2 textSize = ImGui::CalcTextSize( id_str, NULL );
    ImVec2 iconPanelPos = start + ImVec2(
      pane_width - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().IndentSpacing - saveIconWidth - restoreIconWidth - ImGui::GetStyle().ItemInnerSpacing.x * 1,
      (ImGui::GetTextLineHeight() - saveIconHeight) / 2);
    ImGui::GetWindowDrawList()->AddText(
      ImVec2(iconPanelPos.x - textSize.x - ImGui::GetStyle().ItemInnerSpacing.x, start.y),
      IM_COL32(255, 255, 255, 255), id_str, nullptr);

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    ImGui::SetCursorScreenPos(iconPanelPos);
    ImGui::SetNextItemAllowOverlap();
    
    if ( !node->saved_state[ 0 ] )
    {
      if ( ImGui::InvisibleButton( "save", ImVec2( saveIconWidth, saveIconHeight ) ) )
      {
        crude_string_copy_unknow_length( node->saved_state, node->state, sizeof( node->saved_state ) );
      }
      
      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( devmenu_technique_editor->save_icon_texture_handle ) )
      {
        if ( ImGui::IsItemActive( ) )
        {
          drawList->AddImage( CRUDE_CAST( ImTextureRef, &devmenu_technique_editor->save_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
        }
        else if (ImGui::IsItemHovered())
        {
          drawList->AddImage( CRUDE_CAST( ImTextureRef, &devmenu_technique_editor->save_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
        }
        else
        {
          drawList->AddImage( CRUDE_CAST( ImTextureRef, &devmenu_technique_editor->save_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
        }
      }
    }
    else
    {
      ImGui::Dummy(ImVec2(saveIconWidth, saveIconHeight));
      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( devmenu_technique_editor->save_icon_texture_handle ) )
      {
        drawList->AddImage( CRUDE_CAST( ImTextureRef, &devmenu_technique_editor->save_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
      }
    }

    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
    ImGui::SetNextItemAllowOverlap();
    if ( node->saved_state[ 0 ] )
    {
      if ( ImGui::InvisibleButton( "restore", ImVec2( restoreIconWidth, restoreIconHeight ) ) )
      {
        crude_string_copy_unknow_length( node->state, node->saved_state, sizeof( node->state ) );
        ax::NodeEditor::RestoreNodeState( node->id );
        node->saved_state[ 0 ] = 0;
      }

      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( devmenu_technique_editor->restore_icon_texture_handle ) )
      {
        if (ImGui::IsItemActive())
        {
          drawList->AddImage( CRUDE_CAST( ImTextureRef, &devmenu_technique_editor->restore_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
        }
        else if (ImGui::IsItemHovered())
        {
          drawList->AddImage( CRUDE_CAST( ImTextureRef, &devmenu_technique_editor->restore_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
        }
        else
        {
          drawList->AddImage( CRUDE_CAST( ImTextureRef, &devmenu_technique_editor->restore_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
        }
      }
    }
    else
    {
      ImGui::Dummy(ImVec2((float)restoreIconWidth, (float)restoreIconHeight));
      if ( CRUDE_RESOURCE_HANDLE_IS_VALID( devmenu_technique_editor->restore_icon_texture_handle ) )
      {
        drawList->AddImage( CRUDE_CAST( ImTextureRef, &devmenu_technique_editor->restore_icon_texture_handle.index ), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
      }
    }

    ImGui::SameLine(0, 0);
    ImGui::Dummy(ImVec2(0, (float)restoreIconHeight));

    ImGui::PopID();
  }
  
  ImGui::Unindent();

  static int changeCount = 0;

  ImGui::GetWindowDrawList()->AddRectFilled(
    ImGui::GetCursorScreenPos(),
    ImGui::GetCursorScreenPos() + ImVec2(pane_width, ImGui::GetTextLineHeight()),
    ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
  ImGui::Spacing(); ImGui::SameLine();
  ImGui::TextUnformatted("Selection");

  ImGui::BeginHorizontal("Selection Stats", ImVec2(pane_width, 0));
  ImGui::Text("Changed %d time%s", changeCount, changeCount > 1 ? "s" : "");
  ImGui::Spring();
  if (ImGui::Button("Deselect All"))
  {
    ax::NodeEditor::ClearSelection( );
  }

  ImGui::EndHorizontal();
  ImGui::Indent();
  for ( int32 i = 0; i < node_count; ++i )
  {
    ImGui::Text( "Node (%p)", selected_nodes[ i ].AsPointer( ) );
  }
  for ( int32 i = 0; i < link_count; ++i )
  {
    ImGui::Text( "Link (%p)", selected_links[ i ].AsPointer( ) );
  }
  ImGui::Unindent( );

  if (ImGui::IsKeyPressed(ImGuiKey_Z))
  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->links ); ++i )
    {
      ax::NodeEditor::Flow( devmenu_technique_editor->links[ i ].id );
    }
  }

  if ( ax::NodeEditor::HasSelectionChanged( ) )
  {
    ++changeCount;
  }

  ImGui::EndChild();
  
cleanup:
  crude_stack_allocator_free_marker( devmenu_technique_editor->devmenu->dev_stack_allocator, temporary_allocator_marker );
}

void
crude_devmenu_technique_editor_on_frame_
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  crude_devmenu_technique_editor_update_touch_( devmenu_technique_editor );

  ImGuiIO *io = &ImGui::GetIO( );

  ax::NodeEditor::SetCurrentEditor( devmenu_technique_editor->ax_context );

  #if 0
  {
    for ( float32 x = -io->DisplaySize.y; x < io->DisplaySize.x; x += 10.0f )
    {
      ImGui::GetWindowDrawList()->AddLine(ImVec2(x, 0), ImVec2(x + io.DisplaySize.y, io.DisplaySize.y), IM_COL32(255, 255, 0, 255));
    }
  }
  #endif

  static ax::NodeEditor::NodeId contextNodeId      = 0;
  static ax::NodeEditor::LinkId contextLinkId      = 0;
  static ax::NodeEditor::PinId  contextPinId       = 0;
  static bool createNewNode  = false;
  static crude_technique_editor_pin *newNodeLinkPin = NULL;
  static crude_technique_editor_pin *new_link_pin     = NULL;

  static float leftPaneWidth  = 400.0f;
  static float rightPaneWidth = 800.0f;

  splitter_( true, 4.0f, &leftPaneWidth, &rightPaneWidth, 50.0f, 50.0f, false );

  crude_devmenu_technique_editor_show_left_pane_( devmenu_technique_editor, leftPaneWidth - 4.0f );

  ImGui::SameLine(0.0f, 12.0f);

  ax::NodeEditor::Begin("Node editor");
  {
    ImVec2 cursorTopLeft = ImGui::GetCursorScreenPos();

    crude_gui_blueprint_node_builder builder = crude_gui_blueprint_node_builder_empty( &devmenu_technique_editor->devmenu->engine->gpu );
    builder.header_texture_handle = devmenu_technique_editor->header_background_texture_handle;
    
    for ( uint32 node_index = 0u; node_index < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->nodes ); ++node_index )
    {
      crude_technique_editor_node *node = &devmenu_technique_editor->nodes[ node_index ];

      if ( node->type != CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_BLUEPRINT && node->type != CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_SIMPLE )
      {
        continue;
      }

      const bool is_simple = node->type == CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_SIMPLE;

      bool has_output_delegates = false;
      for ( uint32 output_index = 0u; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
      {
        if (node->outputs[ output_index ].type == CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_DELEGATE )
        {
          has_output_delegates = true;
        }
      }
        
      crude_gui_blueprint_node_builder_begin( &builder, node->id );

      if ( !is_simple )
      {
        crude_gui_blueprint_node_builder_header( &builder, node->color );

        ImGui::Spring( 0 );
        ImGui::TextUnformatted( node->name );
        ImGui::Spring( 1 );
        ImGui::Dummy( ImVec2( 0, 28 ) );
        if ( has_output_delegates )
        {
          ImGui::BeginVertical( "delegates", ImVec2(0, 28) );
          ImGui::Spring( 1, 0 );
          for ( uint32 output_index = 0u; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
          {
            crude_technique_editor_pin *output = &node->outputs[ output_index ];
            if ( output->type != CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_DELEGATE )
            {
              continue;
            }

            float32 alpha = ImGui::GetStyle().Alpha;
            if ( new_link_pin && ! crude_devmenu_technique_editor_can_create_link_( devmenu_technique_editor, new_link_pin, output ) && output != new_link_pin )
            {
              alpha = alpha * ( 48.0f / 255.0f );
            }

            ax::NodeEditor::BeginPin( output->id, ax::NodeEditor::PinKind::Output );
            ax::NodeEditor::PinPivotAlignment( ImVec2( 1.0f, 0.5f ) );
            ax::NodeEditor::PinPivotSize( ImVec2(0, 0 ) );
            ImGui::BeginHorizontal( output->id.AsPointer( ) );
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
            if ( output->name[ 0 ] )
            {
              ImGui::TextUnformatted( output->name );
              ImGui::Spring(0);
            }

            crude_technique_editor_draw_pin_icon( devmenu_technique_editor, output, crude_devmenu_technique_editor_is_pin_linked_( devmenu_technique_editor, output->id ), (int)(alpha * 255) );
            ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
            ImGui::EndHorizontal();
            ImGui::PopStyleVar();
            ax::NodeEditor::EndPin();

            //DrawItemRect(ImColor(255, 0, 0));
          }
          ImGui::Spring(1, 0);
          ImGui::EndVertical();
          ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
        }
        else
        {
          ImGui::Spring(0);
        }
        crude_gui_blueprint_node_builder_end_header( &builder );
      }
      
      for ( uint32 input_index = 0u; input_index < CRUDE_ARRAY_LENGTH( node->inputs ); ++input_index )
      {
        crude_technique_editor_pin *input = &node->inputs[ input_index ];

        float32 alpha = ImGui::GetStyle( ).Alpha;
        if ( new_link_pin && !crude_devmenu_technique_editor_can_create_link_( devmenu_technique_editor, new_link_pin, input ) && input != new_link_pin )
        {
          alpha = alpha * (48.0f / 255.0f);
        }

        crude_gui_blueprint_node_builder_input( &builder, input->id );
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        crude_technique_editor_draw_pin_icon( devmenu_technique_editor, input, crude_devmenu_technique_editor_is_pin_linked_( devmenu_technique_editor, input->id ), (int)(alpha * 255) );
        ImGui::Spring(0);
        if ( input->name[ 0 ] )
        {
          ImGui::TextUnformatted( input->name );
          ImGui::Spring(0);
        }

        if ( input->type == CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_BOOL )
        {
             ImGui::Button("Hello");
             ImGui::Spring(0);
        }
        
        ImGui::PopStyleVar();
        crude_gui_blueprint_node_builder_end_input( &builder );
      }

      if ( is_simple )
      {
        crude_gui_blueprint_node_builder_middle( &builder );

        ImGui::Spring(1, 0);
        ImGui::TextUnformatted( node->name );
        ImGui::Spring(1, 0);
      }
      
      for ( uint32 output_index = 0u; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
      {
        crude_technique_editor_pin *output = &node->outputs[ output_index ];
        
        if ( !is_simple && output->type == CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_DELEGATE )
        {
          continue;
        }

        float32 alpha = ImGui::GetStyle( ).Alpha;
        if ( new_link_pin && !crude_devmenu_technique_editor_can_create_link_( devmenu_technique_editor, new_link_pin, output ) && output != new_link_pin )
        {
          alpha = alpha * (48.0f / 255.0f);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        crude_gui_blueprint_node_builder_output( &builder, output->id );
        if ( output->type == CRUDE_TECHNIQUE_EDITOR_PIN_TYPE_STRING )
        {
          static char buffer[128] = "Edit Me\nMultiline!";
          static bool wasActive = false;

          ImGui::PushItemWidth(100.0f);
          ImGui::InputText("##edit", buffer, 127);
          ImGui::PopItemWidth();
          if (ImGui::IsItemActive() && !wasActive)
          {
            ax::NodeEditor::EnableShortcuts(false);
            wasActive = true;
          }
          else if (!ImGui::IsItemActive() && wasActive)
          {
            ax::NodeEditor::EnableShortcuts(true);
            wasActive = false;
          }
          ImGui::Spring(0);
        }
        
        if ( output->name )
        {
          ImGui::Spring(0);
          ImGui::TextUnformatted( output->name );
        }
        ImGui::Spring(0);
        crude_technique_editor_draw_pin_icon( devmenu_technique_editor, output, crude_devmenu_technique_editor_is_pin_linked_( devmenu_technique_editor, output->id ), (int)(alpha * 255) );
        ImGui::PopStyleVar();
        crude_gui_blueprint_node_builder_end_output( &builder );
      }

      crude_gui_blueprint_node_builder_end( &builder );
    }
//    
    for ( uint32 node_index = 0u; node_index < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->nodes ); ++node_index )
    {
      crude_technique_editor_node *node = &devmenu_technique_editor->nodes[ node_index ];

      if ( node->type != CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_TREE )
      {
        continue;
      }

      const float rounding = 5.0f;
      const float padding  = 12.0f;

      const auto pinBackground = ax::NodeEditor::GetStyle().Colors[ax::NodeEditor::StyleColor_NodeBg];

      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBg,        ImColor(128, 128, 128, 200));
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBorder,    ImColor( 32,  32,  32, 200));
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_PinRect,       ImColor( 60, 180, 255, 150));
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_PinRectBorder, ImColor( 60, 180, 255, 150));

      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_NodePadding,  ImVec4(0, 0, 0, 0));
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_NodeRounding, rounding);
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_SourceDirection, ImVec2(0.0f,  1.0f));
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_LinkStrength, 0.0f);
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinBorderWidth, 1.0f);
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinRadius, 5.0f);
      
      ax::NodeEditor::BeginNode( node->id );

      ImGui::BeginVertical( node->id.AsPointer( ) );
      ImGui::BeginHorizontal("inputs");
      ImGui::Spring(0, padding * 2);

      ImRect inputsRect;
      int inputAlpha = 200;
      if ( CRUDE_ARRAY_LENGTH( node->inputs ) )
      {
        crude_technique_editor_pin *pin = &node->inputs[ 0 ];
        ImGui::Dummy(ImVec2(0, padding));
        ImGui::Spring(1, 0);
        inputsRect = ImGui_GetItemRect_( );
      
        ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinArrowSize, 10.0f);
        ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinArrowWidth, 10.0f);
        ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinCorners, ImDrawFlags_RoundCornersBottom);
        ax::NodeEditor::BeginPin( pin->id, ax::NodeEditor::PinKind::Input);
        ax::NodeEditor::PinPivotRect(inputsRect.GetTL(), inputsRect.GetBR());
        ax::NodeEditor::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
        ax::NodeEditor::EndPin();
        ax::NodeEditor::PopStyleVar(3);
      
        if ( new_link_pin && !crude_devmenu_technique_editor_can_create_link_( devmenu_technique_editor, new_link_pin, pin ) && pin != new_link_pin )
        {
          inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
        }
      }
      else
      {
        ImGui::Dummy(ImVec2(0, padding));
      }

      ImGui::Spring(0, padding * 2);
      ImGui::EndHorizontal();

      ImGui::BeginHorizontal("content_frame");
      ImGui::Spring(1, padding);

      ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
      ImGui::Dummy(ImVec2(160, 0));
      ImGui::Spring(1);
      ImGui::TextUnformatted( node->name );
      ImGui::Spring(1);
      ImGui::EndVertical();
      ImRect contentRect = ImGui_GetItemRect_();

      ImGui::Spring(1, padding);
      ImGui::EndHorizontal();

      ImGui::BeginHorizontal("outputs");
      ImGui::Spring(0, padding * 2);

      ImRect outputsRect;
      int outputAlpha = 200;
      if ( CRUDE_ARRAY_LENGTH( node->outputs ) )
      {
        crude_technique_editor_pin *pin = &node->outputs[ 0 ];

        ImGui::Dummy(ImVec2(0, padding));
        ImGui::Spring(1, 0);
        outputsRect = ImGui_GetItemRect_();
      
        ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinCorners, ImDrawFlags_RoundCornersTop);
        ax::NodeEditor::BeginPin(pin->id, ax::NodeEditor::PinKind::Output);
        ax::NodeEditor::PinPivotRect(outputsRect.GetTL(), outputsRect.GetBR());
        ax::NodeEditor::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
        ax::NodeEditor::EndPin();
        ax::NodeEditor::PopStyleVar();
      
        if (new_link_pin && !crude_devmenu_technique_editor_can_create_link_( devmenu_technique_editor, new_link_pin, pin ) && pin != new_link_pin)
        {
          outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
        }
      }
      else
      {
        ImGui::Dummy(ImVec2(0, padding));
      }

      ImGui::Spring(0, padding * 2);
      ImGui::EndHorizontal();

      ImGui::EndVertical();

      ax::NodeEditor::EndNode();
      ax::NodeEditor::PopStyleVar(7);
      ax::NodeEditor::PopStyleColor(4);

      ImDrawList *drawList = ax::NodeEditor::GetNodeBackgroundDrawList( node->id );

                //const auto fringeScale = ImGui::GetStyle().AntiAliasFringeScale;
                //const auto unitSize    = 1.0f / fringeScale;

                //const auto ImDrawList_AddRect = [](ImDrawList* drawList, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners, float thickness)
                //{
                //    if ((col >> 24) == 0)
                //        return;
                //    drawList->PathRect(a, b, rounding, rounding_corners);
                //    drawList->PathStroke(col, true, thickness);
                //};

      const ImDrawFlags_ topRoundCornersFlags = ImDrawFlags_RoundCornersTop;
      const ImDrawFlags_ bottomRoundCornersFlags = ImDrawFlags_RoundCornersBottom;

      drawList->AddRectFilled(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, bottomRoundCornersFlags);
      //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
      drawList->AddRect(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, bottomRoundCornersFlags);
      //ImGui::PopStyleVar();
      drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, topRoundCornersFlags);
      //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
      drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, topRoundCornersFlags);
      //ImGui::PopStyleVar();
      drawList->AddRectFilled(contentRect.GetTL(), contentRect.GetBR(), IM_COL32(24, 64, 128, 200), 0.0f);
      //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
      drawList->AddRect(
        contentRect.GetTL(),
        contentRect.GetBR(),
        IM_COL32(48, 128, 255, 100), 0.0f);
      //ImGui::PopStyleVar();
    }
  
    for ( uint32 node_index = 0u; node_index < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->nodes ); ++node_index )
    {
      crude_technique_editor_node *node = &devmenu_technique_editor->nodes[ node_index ];

      if ( node->type != CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_HOUDINI )
      {
        continue;
      }

      const float rounding = 10.0f;
      const float padding  = 12.0f;


      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBg,        ImColor(229, 229, 229, 200));
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBorder,    ImColor(125, 125, 125, 200));
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_PinRect,       ImColor(229, 229, 229, 60));
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_PinRectBorder, ImColor(125, 125, 125, 60));

      const ImVec4 pinBackground = ax::NodeEditor::GetStyle().Colors[ax::NodeEditor::StyleColor_NodeBg];

      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_NodePadding,  ImVec4(0, 0, 0, 0));
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_NodeRounding, rounding);
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_SourceDirection, ImVec2(0.0f,  1.0f));
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_LinkStrength, 0.0f);
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinBorderWidth, 1.0f);
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinRadius, 6.0f);
      ax::NodeEditor::BeginNode( node->id );

      ImGui::BeginVertical( node->id.AsPointer( ) );
      if ( CRUDE_ARRAY_LENGTH( node->inputs ) )
      {
        ImGui::BeginHorizontal("inputs");
        ImGui::Spring(1, 0);

        ImRect inputsRect;
        int inputAlpha = 200;
        
        for ( uint32 input_index = 0u; input_index < CRUDE_ARRAY_LENGTH( node->inputs ); ++input_index )
        {
          crude_technique_editor_pin *pin = &node->inputs[ input_index ];

          ImGui::Dummy(ImVec2(padding, padding));
          inputsRect = ImGui_GetItemRect_();
          ImGui::Spring(1, 0);
          inputsRect.Min.y -= padding;
          inputsRect.Max.y -= padding;

          const ImDrawFlags_ allRoundCornersFlags = ImDrawFlags_RoundCornersAll;
          //ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
          //ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
          ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinCorners, allRoundCornersFlags);

          ax::NodeEditor::BeginPin( pin->id, ax::NodeEditor::PinKind::Input);
          ax::NodeEditor::PinPivotRect(inputsRect.GetCenter(), inputsRect.GetCenter());
          ax::NodeEditor::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
          ax::NodeEditor::EndPin();
          //ed::PopStyleVar(3);
          ax::NodeEditor::PopStyleVar(1);

          ImDrawList *drawList = ImGui::GetWindowDrawList();
          drawList->AddRectFilled(inputsRect.GetTL(), inputsRect.GetBR(),
            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, allRoundCornersFlags);
          drawList->AddRect(inputsRect.GetTL(), inputsRect.GetBR(),
            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, allRoundCornersFlags);

          if ( new_link_pin && !crude_devmenu_technique_editor_can_create_link_( devmenu_technique_editor, new_link_pin, pin ) && pin != new_link_pin )
          {
            inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
          }
        }

        //ImGui::Spring(1, 0);
        ImGui::EndHorizontal();
      }

      ImGui::BeginHorizontal("content_frame");
      ImGui::Spring(1, padding);

      ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
      ImGui::Dummy(ImVec2(160, 0));
      ImGui::Spring(1);
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
      ImGui::TextUnformatted( node->name );
      ImGui::PopStyleColor();
      ImGui::Spring(1);
      ImGui::EndVertical();
      ImRect contentRect = ImGui_GetItemRect_();

      ImGui::Spring(1, padding);
      ImGui::EndHorizontal();

      if ( CRUDE_ARRAY_LENGTH( node->outputs ) )
      {
        ImGui::BeginHorizontal("outputs");
        ImGui::Spring(1, 0);

        ImRect outputsRect;
        int outputAlpha = 200;
        for ( uint32 output_index = 0u; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
        {
          crude_technique_editor_pin *pin = &node->outputs[ output_index ];

          ImGui::Dummy(ImVec2(padding, padding));
          outputsRect = ImGui_GetItemRect_( );
          ImGui::Spring(1, 0);
          outputsRect.Min.y += padding;
          outputsRect.Max.y += padding;

          const ImDrawFlags_ allRoundCornersFlags = ImDrawFlags_RoundCornersAll;
          const ImDrawFlags_ topRoundCornersFlags = ImDrawFlags_RoundCornersTop;

          ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinCorners, topRoundCornersFlags);
          ax::NodeEditor::BeginPin( pin->id, ax::NodeEditor::PinKind::Output );
          ax::NodeEditor::PinPivotRect(outputsRect.GetCenter(), outputsRect.GetCenter());
          ax::NodeEditor::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
          ax::NodeEditor::EndPin();
          ax::NodeEditor::PopStyleVar();

          ImDrawList *drawList = ImGui::GetWindowDrawList();
          drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR(),
            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, allRoundCornersFlags);
          drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR(),
            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, allRoundCornersFlags);


          if ( new_link_pin && !crude_devmenu_technique_editor_can_create_link_( devmenu_technique_editor, new_link_pin, pin ) && pin != new_link_pin )
          {
            outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
          }
        }

        ImGui::EndHorizontal();
      }

      ImGui::EndVertical();

      ax::NodeEditor::EndNode();
      ax::NodeEditor::PopStyleVar(7);
      ax::NodeEditor::PopStyleColor(4);

      // auto drawList = ed::GetNodeBackgroundDrawList(node.ID);

      //const auto fringeScale = ImGui::GetStyle().AntiAliasFringeScale;
      //const auto unitSize    = 1.0f / fringeScale;

      //const auto ImDrawList_AddRect = [](ImDrawList* drawList, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners, float thickness)
      //{
      //    if ((col >> 24) == 0)
      //        return;
      //    drawList->PathRect(a, b, rounding, rounding_corners);
      //    drawList->PathStroke(col, true, thickness);
      //};

      //drawList->AddRectFilled(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
      //    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
      //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
      //drawList->AddRect(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
      //    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
      //ImGui::PopStyleVar();
      //drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
      //    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
      ////ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
      //drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
      //    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
      ////ImGui::PopStyleVar();
      //drawList->AddRectFilled(contentRect.GetTL(), contentRect.GetBR(), IM_COL32(24, 64, 128, 200), 0.0f);
      //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
      //drawList->AddRect(
      //    contentRect.GetTL(),
      //    contentRect.GetBR(),
      //    IM_COL32(48, 128, 255, 100), 0.0f);
      //ImGui::PopStyleVar();
    }
    
    for ( uint32 node_index = 0u; node_index < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->nodes ); ++node_index )
    {
      crude_technique_editor_node *node = &devmenu_technique_editor->nodes[ node_index ];

      if ( node->type != CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_COMMENT )
      {
        continue;
      }

      const float commentAlpha = 0.75f;

      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha);
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBg, ImColor(255, 255, 255, 64));
      ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBorder, ImColor(255, 255, 255, 64));
      ax::NodeEditor::BeginNode(node->id);
      ImGui::PushID(node->id.AsPointer());
      ImGui::BeginVertical("content");
      ImGui::BeginHorizontal("horizontal");
      ImGui::Spring(1);
      ImGui::TextUnformatted(node->name);
      ImGui::Spring(1);
      ImGui::EndHorizontal();
      ax::NodeEditor::Group( ImVec2( node->size.x, node->size.y ) );
      ImGui::EndVertical();
      ImGui::PopID();
      ax::NodeEditor::EndNode();
      ax::NodeEditor::PopStyleColor(2);
      ImGui::PopStyleVar();

      if (ax::NodeEditor::BeginGroupHint(node->id))
      {
        //auto alpha   = static_cast<int>(commentAlpha * ImGui::GetStyle().Alpha * 255);
        int bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);
      
        //ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha * ImGui::GetStyle().Alpha);
      
        auto min = ax::NodeEditor::GetGroupMin();
        //auto max = ed::GetGroupMax();
      
        ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
        ImGui::BeginGroup();
        ImGui::TextUnformatted( node->name );
        ImGui::EndGroup();
      
        auto drawList = ax::NodeEditor::GetHintBackgroundDrawList();
      
        auto hintBounds      = ImGui_GetItemRect_();
        auto hintFrameBounds = ImRect_Expanded_(hintBounds, 8, 4);
      
        drawList->AddRectFilled(
            hintFrameBounds.GetTL(),
            hintFrameBounds.GetBR(),
            IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f);
      
        drawList->AddRect(
            hintFrameBounds.GetTL(),
            hintFrameBounds.GetBR(),
            IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 4.0f);
      
        //ImGui::PopStyleVar();
      }
      ax::NodeEditor::EndGroupHint();
    }

    for ( uint32 link_index = 0; link_index < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->links ); ++link_index )
    {
      crude_technique_editor_link *link = &devmenu_technique_editor->links[ link_index ];
      ax::NodeEditor::Link( link->id, link->start_pin_id, link->end_pin_id, link->color, 2.0f);
    }

    if (!createNewNode)
    {
      if (ax::NodeEditor::BeginCreate(ImColor(255, 255, 255), 2.0f))
      {
        auto showLabel = [](const char* label, ImColor color)
        {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
            auto size = ImGui::CalcTextSize(label);
        
            auto padding = ImGui::GetStyle().FramePadding;
            auto spacing = ImGui::GetStyle().ItemSpacing;
        
            ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));
        
            auto rectMin = ImGui::GetCursorScreenPos() - padding;
            auto rectMax = ImGui::GetCursorScreenPos() + size + padding;
        
            auto drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
            ImGui::TextUnformatted(label);
        };
      
        ax::NodeEditor::PinId startPinId = 0, endPinId = 0;
        if (ax::NodeEditor::QueryNewLink(&startPinId, &endPinId))
        {
          crude_technique_editor_pin *startPin = crude_devmenu_technique_editor_find_pin_( devmenu_technique_editor, startPinId);
          crude_technique_editor_pin *endPin = crude_devmenu_technique_editor_find_pin_( devmenu_technique_editor, endPinId);
        
          new_link_pin = startPin ? startPin : endPin;
        
          if ( startPin->kind == CRUDE_TECHNIQUE_EDITOR_PIN_KIND_INPUT )
          {
            std::swap(startPin, endPin);
            std::swap(startPinId, endPinId);
          }
        
          if (startPin && endPin)
          {
            if (endPin == startPin)
            {
              ax::NodeEditor::RejectNewItem(ImColor(255, 0, 0), 2.0f);
            }
            else if (endPin->kind == startPin->kind)
            {
              showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
              ax::NodeEditor::RejectNewItem(ImColor(255, 0, 0), 2.0f);
            }
            //else if (endPin->Node == startPin->Node)
            //{
            //    showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
            //    ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
            //}
            else if ( endPin->type != startPin->type )
            {
              showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
              ax::NodeEditor::RejectNewItem(ImColor(255, 128, 128), 1.0f);
            }
            else
            {
              showLabel("+ Create Link", ImColor(32, 45, 32, 180));
              if (ax::NodeEditor::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
              {
                crude_technique_editor_link link = crude_technique_editor_link_empty_( );
                link.id = crude_devmenu_technique_editor_get_next_id_( devmenu_technique_editor );
                link.start_pin_id = startPinId;
                link.end_pin_id = endPinId;
                link.color = crude_technique_editor_get_icon_color( startPin->type );
                CRUDE_ARRAY_PUSH( devmenu_technique_editor->links, link );
              }
            }
          }
        }
      
        ax::NodeEditor::PinId pinId = 0;
        if (ax::NodeEditor::QueryNewNode(&pinId))
        {
          new_link_pin = crude_devmenu_technique_editor_find_pin_( devmenu_technique_editor, pinId);
          if (new_link_pin)
          {
            showLabel("+ Create Node", ImColor(32, 45, 32, 180));
          }
      
          if (ax::NodeEditor::AcceptNewItem())
          {
            createNewNode  = true;
            newNodeLinkPin = crude_devmenu_technique_editor_find_pin_( devmenu_technique_editor, pinId);
            new_link_pin = nullptr;
            ax::NodeEditor::Suspend();
            ImGui::OpenPopup("Create New Node");
            ax::NodeEditor::Resume();
          }
        }
      }
      else
      {
        new_link_pin = nullptr;
      }

      ax::NodeEditor::EndCreate();

      if (ax::NodeEditor::BeginDelete())
      {
        ax::NodeEditor::NodeId nodeId = 0;
        while (ax::NodeEditor::QueryDeletedNode(&nodeId))
        {
          if (ax::NodeEditor::AcceptDeletedItem())
          {
            uint32 i; 
            for ( i = 0; i < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->nodes ); ++i )
            {
              if ( devmenu_technique_editor->nodes[ i ].id == nodeId )
              {
                break;
              }
            }
            if ( i != CRUDE_ARRAY_LENGTH( devmenu_technique_editor->nodes ) )
            {
              CRUDE_ARRAY_DELSWAP( devmenu_technique_editor->nodes, i );
            }
          }
        }

        ax::NodeEditor::LinkId linkId = 0;
        while (ax::NodeEditor::QueryDeletedLink(&linkId))
        {
          if (ax::NodeEditor::AcceptDeletedItem())
          {
            uint32 i; 
            for ( i = 0; i < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->links ); ++i )
            {
              if ( devmenu_technique_editor->links[ i ].id == linkId )
              {
                break;
              }
            }
            if ( i != CRUDE_ARRAY_LENGTH( devmenu_technique_editor->links ) )
            {
              CRUDE_ARRAY_DELSWAP( devmenu_technique_editor->links, i );
            }
          }
        }
      }
      
      ax::NodeEditor::EndDelete();
    }

    ImGui::SetCursorScreenPos(cursorTopLeft);
}

#if 1
  ImVec2 openPopupPosition = ImGui::GetMousePos();
  ax::NodeEditor::Suspend();
  if (ax::NodeEditor::ShowNodeContextMenu(&contextNodeId))
  {
    ImGui::OpenPopup("Node Context Menu");
  }
  else if (ax::NodeEditor::ShowPinContextMenu(&contextPinId))
  {
    ImGui::OpenPopup("Pin Context Menu");
  }
  else if (ax::NodeEditor::ShowLinkContextMenu(&contextLinkId))
  {
    ImGui::OpenPopup("Link Context Menu");
  }
  else if (ax::NodeEditor::ShowBackgroundContextMenu())
  {
    ImGui::OpenPopup("Create New Node");
    newNodeLinkPin = nullptr;
  }
    
  ax::NodeEditor::Resume();

  ax::NodeEditor::Suspend();
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
  if (ImGui::BeginPopup("Node Context Menu"))
  {
    crude_technique_editor_node *node = crude_devmenu_technique_editor_find_node_( devmenu_technique_editor, contextNodeId);

    ImGui::TextUnformatted("Node Context Menu");
    ImGui::Separator();
    if (node)
    {
      ImGui::Text("ID: %p", node->id.AsPointer());
      ImGui::Text("Type: %s", node->type == CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_BLUEPRINT ? "Blueprint" : (node->type == CRUDE_TECHNIQUE_EDITOR_NODE_TYPE_TREE ? "Tree" : "Comment"));
      ImGui::Text("Inputs: %d", CRUDE_ARRAY_LENGTH( node->inputs ) );
      ImGui::Text("Outputs: %d", CRUDE_ARRAY_LENGTH( node->outputs ) );
    }
    else
    {
      ImGui::Text("Unknown node: %p", contextNodeId.AsPointer());
    }
    
    ImGui::Separator();
    if (ImGui::MenuItem("Delete"))
    {
      ax::NodeEditor::DeleteNode(contextNodeId);
    }
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup("Pin Context Menu"))
  {
    crude_technique_editor_pin *pin = crude_devmenu_technique_editor_find_pin_( devmenu_technique_editor, contextPinId);

    ImGui::TextUnformatted("Pin Context Menu");
    ImGui::Separator();
    if (pin)
    {
      ImGui::Text("ID: %p", pin->id.AsPointer());
      if (pin->node )
      {
        ImGui::Text("Node: %p", pin->node->id.AsPointer());
      }
      else
      {
        ImGui::Text("Node: %s", "<none>");
      }
    }
    else
    {
      ImGui::Text("Unknown pin: %p", contextPinId.AsPointer());
    }

    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup("Link Context Menu"))
  {
    crude_technique_editor_link *link = crude_devmenu_technique_editor_find_link_( devmenu_technique_editor, contextLinkId);

    ImGui::TextUnformatted("Link Context Menu");
    ImGui::Separator();
    if (link)
    {
      ImGui::Text("ID: %p", link->id.AsPointer());
      ImGui::Text("From: %p", link->start_pin_id.AsPointer());
      ImGui::Text("To: %p", link->end_pin_id.AsPointer());
    }
    else
    {
      ImGui::Text("Unknown link: %p", contextLinkId.AsPointer());
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Delete"))
    {
      ax::NodeEditor::DeleteLink(contextLinkId);
    }
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup("Create New Node"))
  {
    ImVec2 newNodePostion = openPopupPosition;
    //ImGui::SetCursorScreenPos(ImGui::GetMousePosOnOpeningCurrentPopup());

    //auto drawList = ImGui::GetWindowDrawList();
    //drawList->AddCircleFilled(ImGui::GetMousePosOnOpeningCurrentPopup(), 10.0f, 0xFFFF00FF);

    crude_technique_editor_node *node = NULL;
    if (ImGui::MenuItem("Input Action"))
    {
      node = crude_devmenu_technique_editor_spawn_input_action_node_( devmenu_technique_editor );
    }
    if (ImGui::MenuItem("Output Action"))
    {
      node = crude_devmenu_technique_editor_spawn_output_action_node_( devmenu_technique_editor );
    }
    if (ImGui::MenuItem("Branch"))
    {
      node = crude_devmenu_technique_editor_spawn_branch_node_( devmenu_technique_editor );
    }
    if (ImGui::MenuItem("Do N"))
    {
      node = crude_devmenu_technique_editor_spawn_do_node_( devmenu_technique_editor );
    }
    if (ImGui::MenuItem("Set Timer"))
    {
      node = crude_devmenu_technique_editor_spawn_set_timer_node_( devmenu_technique_editor );
    }
    if (ImGui::MenuItem("Less"))
    {
      node = crude_devmenu_technique_editor_spawn_less_node_( devmenu_technique_editor );
    }
    if (ImGui::MenuItem("Weird"))
    {
      node = crude_devmenu_technique_editor_spawn_weird_node_( devmenu_technique_editor );
    }
    if (ImGui::MenuItem("Trace by Channel"))
    {
      node = crude_devmenu_technique_editor_spawn_trace_by_channel_node_( devmenu_technique_editor );
    }
    if (ImGui::MenuItem("Print String"))
    {
      node = crude_devmenu_technique_editor_spawn_print_string_node_( devmenu_technique_editor );
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Comment"))
    {
      node = crude_devmenu_technique_editor_spawn_comment_( devmenu_technique_editor );
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Sequence"))
    {
      node = crude_devmenu_technique_editor_spawn_tree_sequence_node_( devmenu_technique_editor );
    }
    if (ImGui::MenuItem("Move To"))
    {
      node = crude_devmenu_technique_editor_spawn_tree_task_node_( devmenu_technique_editor );
    }
    if (ImGui::MenuItem("Random Wait"))
    {
      node = crude_devmenu_technique_editor_spawn_tree_task_2node_( devmenu_technique_editor );
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Message"))
    {
      node = crude_devmenu_technique_editor_spawn_message_node_( devmenu_technique_editor );
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Transform"))
    {
      node = crude_devmenu_technique_editor_spawn_houdini_transform_node_( devmenu_technique_editor );
    }
    if (ImGui::MenuItem("Group"))
    {
      node = crude_devmenu_technique_editor_spawn_houdini_group_node_( devmenu_technique_editor );
    }
    
    if (node)
    {
      crude_devmenu_technique_editor_build_nodes_( devmenu_technique_editor );

      createNewNode = false;

      ax::NodeEditor::SetNodePosition(node->id, newNodePostion);

      crude_technique_editor_pin *startPin = newNodeLinkPin;
      if (startPin)
      {
        crude_technique_editor_pin *pins = startPin->kind == CRUDE_TECHNIQUE_EDITOR_PIN_KIND_INPUT ? node->outputs : node->inputs;
        
        for ( uint32 pin_index = 0u; pin_index < CRUDE_ARRAY_LENGTH( pins ); ++pin_index )
        {
          crude_technique_editor_pin *pin = &pins[ pin_index ];

          if ( crude_devmenu_technique_editor_can_create_link_( devmenu_technique_editor, startPin, pin ) )
          {
            crude_technique_editor_pin *endPin = pin;
            if (startPin->kind == CRUDE_TECHNIQUE_EDITOR_PIN_KIND_INPUT)
            {
              std::swap(startPin, endPin);
            }
      
            crude_technique_editor_link link = crude_technique_editor_link_empty_( );
            link.id = crude_devmenu_technique_editor_get_next_id_( devmenu_technique_editor );
            link.start_pin_id = startPin->id;
            link.end_pin_id = endPin->id;
            link.color = crude_technique_editor_get_icon_color( startPin->type );
            CRUDE_ARRAY_PUSH( devmenu_technique_editor->links, link );
            break;
          }
        }
      }
    }

    ImGui::EndPopup();
  }
  else
  {
    createNewNode = false;
  }
  ImGui::PopStyleVar();
  ax::NodeEditor::Resume();
#endif


    /*
        cubic_bezier_t c;
        c.p0 = pointf(100, 600);
        c.p1 = pointf(300, 1200);
        c.p2 = pointf(500, 100);
        c.p3 = pointf(900, 600);

        auto drawList = ImGui::GetWindowDrawList();
        auto offset_radius = 15.0f;
        auto acceptPoint = [drawList, offset_radius](const bezier_subdivide_result_t& r)
        {
            drawList->AddCircle(to_imvec(r.point), 4.0f, IM_COL32(255, 0, 255, 255));

            auto nt = r.tangent.normalized();
            nt = pointf(-nt.y, nt.x);

            drawList->AddLine(to_imvec(r.point), to_imvec(r.point + nt * offset_radius), IM_COL32(255, 0, 0, 255), 1.0f);
        };

        drawList->AddBezierCurve(to_imvec(c.p0), to_imvec(c.p1), to_imvec(c.p2), to_imvec(c.p3), IM_COL32(255, 255, 255, 255), 1.0f);
        cubic_bezier_subdivide(acceptPoint, c);
    */

  ax::NodeEditor::End();

  ImVec2 editorMin = ImGui::GetItemRectMin();
  ImVec2 editorMax = ImGui::GetItemRectMax();

  if ( devmenu_technique_editor->show_ordinals )
  {
    uint64 temporary_allocator_marker = crude_stack_allocator_get_marker( devmenu_technique_editor->devmenu->dev_stack_allocator );
    int nodeCount = ax::NodeEditor::GetNodeCount();
    ax::NodeEditor::NodeId *orderedNodeIds;
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( orderedNodeIds, nodeCount, crude_stack_allocator_pack( devmenu_technique_editor->devmenu->dev_stack_allocator ) );
    ax::NodeEditor::GetOrderedNodeIds( orderedNodeIds, nodeCount );
  
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(editorMin, editorMax);
  
    int ordinal = 0;
    for ( uint32 i = 0; i < nodeCount; ++i )
    {
      ax::NodeEditor::NodeId nodeId = orderedNodeIds[ i ];
      ImVec2 p0 = ax::NodeEditor::GetNodePosition(nodeId);
      ImVec2 p1 = p0 + ax::NodeEditor::GetNodeSize(nodeId);
      p0 = ax::NodeEditor::CanvasToScreen(p0);
      p1 = ax::NodeEditor::CanvasToScreen(p1);
    
    
      ImGuiTextBuffer builder;
      builder.appendf("#%d", ordinal++);
    
      ImVec2 textSize   = ImGui::CalcTextSize(builder.c_str());
      ImVec2 padding    = ImVec2(2.0f, 2.0f);
      ImVec2 widgetSize = textSize + padding * 2;
    
      ImVec2 widgetPosition = ImVec2(p1.x, p0.y) + ImVec2(0.0f, -widgetSize.y);
    
      drawList->AddRectFilled(widgetPosition, widgetPosition + widgetSize, IM_COL32(100, 80, 80, 190), 3.0f, ImDrawFlags_RoundCornersAll);
      drawList->AddRect(widgetPosition, widgetPosition + widgetSize, IM_COL32(200, 160, 160, 190), 3.0f, ImDrawFlags_RoundCornersAll);
      drawList->AddText(widgetPosition + padding, IM_COL32(255, 255, 255, 255), builder.c_str());
    }

    drawList->PopClipRect();
    crude_stack_allocator_free_marker( devmenu_technique_editor->devmenu->dev_stack_allocator, temporary_allocator_marker );
  }

  //ImGui::ShowTestWindow();
  //ImGui::ShowMetricsWindow();
}

crude_technique_editor_pin
crude_technique_editor_pin_empty
(
)
{
  crude_technique_editor_pin pin = CRUDE_COMPOUNT_EMPTY( crude_technique_editor_pin );
  return pin;
}

void
crude_technique_editor_node_initialize
(
  _In_ crude_technique_editor_node                        *node,
  _In_ crude_heap_allocator                               *allocator
)
{
  *node = CRUDE_COMPOUNT_EMPTY( crude_technique_editor_node );
  node->color = CRUDE_COMPOUNT_EMPTY( XMFLOAT4, { 255, 255, 255 } );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->inputs, 0, crude_heap_allocator_pack( allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->outputs, 0, crude_heap_allocator_pack( allocator ) );
}

void
crude_technique_editor_node_deinitialize
(
  _In_ crude_technique_editor_node                        *node
)
{
  CRUDE_ARRAY_DEINITIALIZE( node->inputs );
  CRUDE_ARRAY_DEINITIALIZE( node->outputs );
}

void
crude_devmenu_technique_editor_initialize
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor,
  _In_ crude_devmenu                                      *devmenu
)
{
  *devmenu_technique_editor = CRUDE_COMPOUNT_EMPTY( crude_devmenu_technique_editor );

  devmenu->technique_editor.enabled = false;
  
  devmenu_technique_editor->next_id = 1;
  devmenu_technique_editor->pin_icon_size = 24;
  devmenu_technique_editor->touch_time = 1.f;
  devmenu_technique_editor->show_ordinals = false;

  devmenu_technique_editor->header_background_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  devmenu_technique_editor->restore_icon_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  devmenu_technique_editor->save_icon_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  devmenu_technique_editor->devmenu = devmenu;

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( devmenu_technique_editor->nodes, 0, crude_heap_allocator_pack( devmenu->dev_heap_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( devmenu_technique_editor->links, 0, crude_heap_allocator_pack( devmenu->dev_heap_allocator ) );
  CRUDE_HASHMAP_INITIALIZE( devmenu_technique_editor->node_id_to_touch_time, crude_heap_allocator_pack( devmenu->dev_heap_allocator ) );

  crude_devmenu_technique_editor_on_start_( devmenu_technique_editor );
}

void
crude_devmenu_technique_editor_deinitialize
(
  _In_ crude_devmenu_technique_editor                     *devmenu_technique_editor
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( devmenu_technique_editor->nodes ); ++i )
  {
    crude_technique_editor_node_deinitialize( &devmenu_technique_editor->nodes[ i ] );
  }

  CRUDE_ARRAY_DEINITIALIZE( devmenu_technique_editor->nodes );
  CRUDE_ARRAY_DEINITIALIZE( devmenu_technique_editor->links );
  CRUDE_HASHMAP_DEINITIALIZE( devmenu_technique_editor->node_id_to_touch_time );

  crude_devmenu_technique_editor_on_stop_( devmenu_technique_editor );
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
  crude_devmenu_technique_editor_on_frame_( devmenu_technique_editor );
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