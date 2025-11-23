#if CRUDE_DEVELOP

#include <SDL3/SDL.h>
#include <imgui/imgui.h>

#include <engine/core/hash_map.h>
#include <engine/platform/platform.h>
#include <game.h>
#include <engine/scene/scripts_components.h>
#include <game_components.h>

#include <devmenu.h>

void
crude_devmenu_initialize
(
	_In_ crude_devmenu																			*devmenu
)
{
	devmenu->enabled = false;
  devmenu->selected_option = 0;
  crude_devmenu_gpu_visual_profiler_initialize( &devmenu->gpu_visual_profiler );
  crude_devmenu_texture_inspector_initialize( &devmenu->texture_inspector );
}

void
crude_devmenu_deinitialize
(
	_In_ crude_devmenu																			*devmenu
)
{
  crude_devmenu_gpu_visual_profiler_deinitialize( &devmenu->gpu_visual_profiler );
  crude_devmenu_texture_inspector_deinitialize( &devmenu->texture_inspector );
}

void
crude_devmenu_draw
(
	_In_ crude_devmenu																			*devmenu
)
{
  game_t                                                  *game;

  game = game_instance( );
  
	if ( devmenu->enabled )
	{
    ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
    ImGui::SetNextWindowSize( ImVec2( game->gpu.vk_swapchain_width, 25 ) );
    ImGui::Begin( "Devmenu", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
    char const* devmenu_options[ ] =
    {
      "GPU Visual Profiler",
      "Texture Inspector"
    };
    
    ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 1.0f, 0.0f, 0.0f, 1.0f )); // Red
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 1.0f, 0.5f, 0.5f, 1.0f ) );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.8f, 0.0f, 0.0f, 1.0f ) );
    ImGui::GetIO().FontGlobalScale = 0.5f;
    for ( uint32 i = 0; i < CRUDE_COUNTOF( devmenu_options ); ++i  )
    {
	    ImGui::SetCursorPos( ImVec2( i * ( 100 + 20 ), 0 ) );
      if ( ImGui::Button( devmenu_options[ i ], ImVec2( 100, 25 ) ) )
      {
        devmenu->selected_option = i;
        if ( i == 0 )
        {
          devmenu->gpu_visual_profiler.enabled = !devmenu->gpu_visual_profiler.enabled;
        }
        else if ( i == 1 )
        {
          devmenu->texture_inspector.enabled = !devmenu->texture_inspector.enabled;
        }
      }
      ImGui::SameLine( );
    }
    ImGui::PopStyleColor( 3 );
    ImGui::GetIO().FontGlobalScale = 1.f;
    ImGui::End( );
  }

  crude_devmenu_gpu_visual_profiler_draw( &devmenu->gpu_visual_profiler );
  crude_devmenu_texture_inspector_draw( &devmenu->texture_inspector );
}

void
crude_devmenu_update
(
	_In_ crude_devmenu																			*devmenu
)
{
  crude_devmenu_gpu_visual_profiler_update( &devmenu->gpu_visual_profiler );
  crude_devmenu_texture_inspector_update( &devmenu->texture_inspector );
}

void
crude_devmenu_handle_input
(
	_In_ crude_devmenu																			*devmenu,
	_In_ crude_input																				*input
)
{
  game_t                                                  *game;

  game = game_instance( );
  
	if ( input->keys[ SDL_SCANCODE_F1 ].pressed )
	{
    crude_entity player_controller_node = crude_ecs_lookup_entity_from_parent( game->main_node, "player" );
    crude_player_controller *player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( player_controller_node, crude_player_controller );
    player_controller->fly_mode = !player_controller->fly_mode;
    crude_physics_enable_simulation( &game->physics, !game->physics.simulation_enabled );
  }
	if ( input->keys[ SDL_SCANCODE_F4 ].pressed )
	{
		devmenu->enabled = !devmenu->enabled;
    
    crude_window_handle *window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->platform_node, crude_window_handle );
    crude_entity player_controller_node = crude_ecs_lookup_entity_from_parent( game->main_node, "player" );
    crude_player_controller *player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( player_controller_node, crude_player_controller );
    if ( devmenu->enabled )
    {
      player_controller->input_enabled = false;
      crude_platform_show_cursor( *window_handle );
    }
    else
    {
      player_controller->input_enabled = true;
      crude_platform_hide_cursor( *window_handle );
    }
	}
}

/***********************
 * Devmenu
 ***********************/
void
crude_devmenu_gpu_visual_profiler_initialize
(
	_In_ crude_devmenu_gpu_visual_profiler									*dev_gpu_profiler
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
	_In_ crude_devmenu_gpu_visual_profiler									*dev_gpu_profiler
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
	_In_ crude_devmenu_gpu_visual_profiler									*dev_gpu_profiler
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
	_In_ crude_devmenu_gpu_visual_profiler									*dev_gpu_profiler
)
{
  game_t                                                  *game;

  game = game_instance( );

  if ( !dev_gpu_profiler->enabled )
  {
    return;
  }

  ImGui::Begin( "Visual Profiler" );

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

/***********************
 * 
 * Develop Texture Inspector
 * 
 ***********************/
void
crude_devmenu_texture_inspector_initialize
(
	_In_ crude_devmenu_texture_inspector									  *dev_texture_inspector
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
	_In_ crude_devmenu_texture_inspector									  *dev_texture_inspector
)
{
}

void
crude_devmenu_texture_inspector_update
(
	_In_ crude_devmenu_texture_inspector									  *dev_texture_inspector
)
{
}

void
crude_devmenu_texture_inspector_draw
(
	_In_ crude_devmenu_texture_inspector									  *dev_texture_inspector
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

#endif