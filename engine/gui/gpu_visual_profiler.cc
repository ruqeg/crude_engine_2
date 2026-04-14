#if CRUDE_DEVELOP

#include <engine/gui/gpu_visual_profiler.h>

static void
crude_gui_gpu_visual_profiler_pool_queue_draw_
(
  _In_ crude_resource_pool                                *resource_pool,
  _In_ char const                                         *resource_name
);

void
crude_gui_gpu_visual_profiler_initialize
(
  _In_ crude_gui_gpu_visual_profiler                      *profiler,
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_heap_allocator                               *allocator
)
{
  profiler->gpu = gpu;
  profiler->enabled = false;
  profiler->max_duration = 16.666f;
  profiler->max_frames = 100u;
  profiler->current_frame = 0u;
  profiler->max_visible_depth = 2;
  profiler->max_queries_per_frame = 32;
  profiler->allocator = allocator;
  profiler->paused = false;
  profiler->pipeline_statistics = NULL;
  profiler->timestamps = CRUDE_CAST( crude_gfx_gpu_time_query*, CRUDE_ALLOCATE( crude_heap_allocator_pack( profiler->allocator ), sizeof( crude_gfx_gpu_time_query ) * profiler->max_frames * profiler->max_queries_per_frame ) );
  profiler->per_frame_active = CRUDE_CAST( uint16*, CRUDE_ALLOCATE( crude_heap_allocator_pack( profiler->allocator ), sizeof( uint16 ) * profiler->max_frames ) );
  profiler->framebuffer_pixel_count = 0u;
  profiler->initial_frames_paused = 15;
  memset( profiler->per_frame_active, 0, sizeof( uint16 ) * profiler->max_frames );
  CRUDE_HASHMAPSTR_INITIALIZE( profiler->name_to_color_index, crude_heap_allocator_pack( profiler->allocator ) );}

void
crude_gui_gpu_visual_profiler_deinitialize
(
  _In_ crude_gui_gpu_visual_profiler                      *profiler
)
{
  CRUDE_HASHMAPSTR_DEINITIALIZE( profiler->name_to_color_index );
  CRUDE_DEALLOCATE( crude_heap_allocator_pack( profiler->allocator ), profiler->timestamps );
  CRUDE_DEALLOCATE( crude_heap_allocator_pack( profiler->allocator ), profiler->per_frame_active );
}

void
crude_gui_gpu_visual_profiler_update
(
  _In_ crude_gui_gpu_visual_profiler                      *profiler
)
{
  crude_gfx_gpu_set_timestamps_enable( profiler->gpu, !profiler->paused );

  if ( profiler->initial_frames_paused )
  {
    --profiler->initial_frames_paused;
    return;
  }

  if ( profiler->paused )
  {
    return;
  }

  uint32 active_timestamps = crude_gfx_copy_gpu_timestamps( profiler->gpu, &profiler->timestamps[ profiler->max_queries_per_frame * profiler->current_frame ] );

  profiler->per_frame_active[ profiler->current_frame ] = active_timestamps;
  
  profiler->framebuffer_pixel_count = profiler->gpu->renderer_size.x * profiler->gpu->renderer_size.y;

  profiler->pipeline_statistics = &profiler->gpu->gpu_time_queries_manager->frame_pipeline_statistics;

  for ( uint32 i = 0; i < active_timestamps; ++i )
  {
    crude_gfx_gpu_time_query                              *timestamp;
    int64                                                  hash_color_index;
    uint64                                                 color_index;

    timestamp = &profiler->timestamps[ profiler->max_queries_per_frame * profiler->current_frame + i ];
  
    hash_color_index = CRUDE_HASHMAPSTR_GET_INDEX( profiler->name_to_color_index, timestamp->name );

    if ( hash_color_index == -1 )
    {
      color_index = CRUDE_HASHMAPSTR_LENGTH( profiler->name_to_color_index );
      CRUDE_HASHMAPSTR_SET( profiler->name_to_color_index, CRUDE_COMPOUNT( crude_string_link, { timestamp->name } ), color_index );
    }
    else
    {
      color_index = profiler->name_to_color_index[ hash_color_index ].value;
    }
  
    timestamp->color = crude_color_get_distinct_color( color_index );
  }

  profiler->current_frame = ( profiler->current_frame + 1 ) % profiler->max_frames;

  if ( profiler->current_frame == 0 )
  {
    profiler->max_time = -FLT_MAX;
    profiler->min_time = FLT_MAX;
    profiler->average_time = 0.f;
  }
}

void
crude_gui_gpu_visual_profiler_queue_draw
(
  _In_ crude_gui_gpu_visual_profiler* profiler
)
{
  {
    ImGuiIO                                               *imgui_io;
    ImDrawList                                            *draw_list;
    char                                                   buf[ 128 ];
    ImVec2                                                 cursor_pos, canvas_size, mouse_pos;
    float64                                                new_average;
    float32                                                selected_frame_time, dump_height, widget_height, legend_width, graph_width;
    uint32                                                 rect_width;
    int32                                                  rect_x, selected_frame;

    draw_list = ImGui::GetWindowDrawList();
    cursor_pos = ImGui::GetCursorScreenPos();
    canvas_size = ImGui::GetContentRegionAvail();
    widget_height = canvas_size.y - 100;
    
    selected_frame_time = 0.f;

    legend_width = 250;
    graph_width = fabsf( canvas_size.x - legend_width );
    rect_width = CRUDE_CEIL( graph_width / profiler->max_frames );
    rect_x = CRUDE_CEIL( graph_width - rect_width );

    new_average = 0;

    imgui_io = &ImGui::GetIO();

    crude_memory_set( buf, 0u, sizeof( buf ) );

    mouse_pos = imgui_io->MousePos;

    selected_frame = -1;

    /* Draw legend */
    ImGui::SetCursorPosX( cursor_pos.x + graph_width );

    selected_frame = selected_frame == -1 ? ( profiler->current_frame - 1 ) % profiler->max_frames : selected_frame;
    if ( selected_frame >= 0 )
    {
      crude_gfx_gpu_time_query                            *frame_timestamps;
      float32                                              x, y;

      frame_timestamps = &profiler->timestamps[ selected_frame * profiler->max_queries_per_frame ];
    
      x = cursor_pos.x + graph_width + 8;
      y = cursor_pos.y + 14;
    
      for ( int32 j = profiler->per_frame_active[ selected_frame ] - 1; j >= 0; --j )
      {
        crude_gfx_gpu_time_query                          *timestamp;
        float32                                            timestamp_x;

        timestamp = &frame_timestamps[ j ];
    
        if ( timestamp->depth > profiler->max_visible_depth )
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
    
          y += 14;
        }
      }

      dump_height = widget_height = y - cursor_pos.y;
    }
    
    /* Draw Graph */
    ImGui::SetCursorPosX( cursor_pos.x );
    
    widget_height = CRUDE_MIN( canvas_size.y, widget_height );

    for ( uint32 i = 0; i < profiler->max_frames; ++i )
    {
      crude_gfx_gpu_time_query                            *frame_timestamps;
      uint32                                               frame_index;
      float32                                              frame_x, frame_time, rect_height, current_height;

      frame_index = ( profiler->current_frame - 1 - i ) % profiler->max_frames;

      frame_x = cursor_pos.x + rect_x;
      frame_timestamps = &profiler->timestamps[ frame_index * profiler->max_queries_per_frame ];
      frame_time = frame_timestamps[ 0 ].elapsed_ms;
      
      frame_time = CRUDE_CLAMP( frame_time, 1000.f, 0.00001f );
            
      new_average += frame_time;
      profiler->min_time = CRUDE_MIN( profiler->min_time, frame_time );
      profiler->max_time = CRUDE_MAX( profiler->max_time, frame_time );
      
      rect_height = frame_time / profiler->max_duration * widget_height;
      current_height = cursor_pos.y;
      
      /* Draw timestamps from the bottom */
      for ( uint32 j = 0; j < profiler->per_frame_active[ frame_index ]; ++j )
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
      
        rect_height = ( float32 )timestamp->elapsed_ms / profiler->max_duration * widget_height;
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
      
        selected_frame = frame_index;
        selected_frame_time = frame_time;
      }

      draw_list->AddLine( { frame_x, cursor_pos.y + widget_height }, { frame_x, cursor_pos.y }, 0x0fffffff );

      rect_x -= rect_width;
    }
    
    sprintf( buf, "%3.4fms", profiler->max_duration );
    draw_list->AddText( { cursor_pos.x, cursor_pos.y }, 0xff0000ff, buf );
    draw_list->AddRectFilled( { cursor_pos.x + rect_width, cursor_pos.y }, { cursor_pos.x + graph_width, cursor_pos.y + 1 }, 0xff0000ff );

    sprintf( buf, "%3.4fms", profiler->max_duration / 2.f );
    draw_list->AddText( { cursor_pos.x, cursor_pos.y + widget_height / 2.f }, 0xff00ffff, buf );
    draw_list->AddRectFilled( { cursor_pos.x + rect_width, cursor_pos.y + widget_height / 2.f }, { cursor_pos.x + graph_width, cursor_pos.y + widget_height / 2.f + 1 }, 0xff00ffff );

    profiler->average_time = CRUDE_CAST( float32, profiler->new_average ) / profiler->max_frames;

    ImGui::Dummy( { canvas_size.x, dump_height } );

    ImGui::SetNextItemWidth( 100.f );
    ImGui::LabelText( "", "Max %3.4fms", profiler->max_time );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 100.f );
    ImGui::LabelText( "", "Min %3.4fms", profiler->min_time );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 100.f );
    ImGui::LabelText( "", "Ave %3.4fms", profiler->average_time );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 150.f );
    if ( selected_frame_time != 0.f )
    {
      ImGui::LabelText( "", "Cur %3.4fms", selected_frame_time );
    }
    else
    {
      ImGui::LabelText( "", "No selected" );
    }
  }
  
  ImGui::Separator();
  ImGui::Checkbox( "Pause", &profiler->paused );
  
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
      profiler->max_duration = max_durations[ max_duration_index ];
    }
    
    ImGui::SliderInt( "Max Depth", &profiler->max_visible_depth, 1, 4 );
    
    ImGui::Separator();
    
    stat_unit_multiplier = stat_unit_multipliers[ stat_unit_index ];
    stat_unit_name = stat_units[ stat_unit_index ];
    if ( profiler->pipeline_statistics )
    {
      float32 stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_COUNT ];
      for ( uint32 i = 0; i < CRUDE_GFX_GPU_PIPELINE_STATISTICS_COUNT; ++i )
      {
        stat_values[ i ] = profiler->pipeline_statistics->statistics[ i ] / stat_unit_multiplier;
      }
    
      ImGui::Text( "Vertices %0.2f%s, Primitives %0.2f%s", stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_VERTICES_COUNT ], stat_unit_name,
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_PRIMITIVE_COUNT ], stat_unit_name );

      ImGui::Text( "Clipping: Invocations %0.2f%s, Visible Primitives %0.2f%s, Visible Perc %3.1f", stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_CLIPPING_INVOCATIONS ], stat_unit_name,
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_CLIPPING_PRIMITIVES ], stat_unit_name,
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_CLIPPING_PRIMITIVES ] / stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_CLIPPING_INVOCATIONS ] * 100.0f, stat_unit_name );
    
      ImGui::Text( "Invocations: Vertex Shaders %0.2f%s, Fragment Shaders %0.2f%s, Compute Shaders %0.2f%s", stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_VERTEX_SHADER_INVOCATIONS ], stat_unit_name,
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_FRAGMENT_SHADER_INVOCATIONS ], stat_unit_name, stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_COMPUTE_SHADER_INVOCATIONS ], stat_unit_name );
    
      ImGui::Text( "Invocations: Mesh Shaders %0.2f%s, Task Shaders %0.2f%s",
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_MESH_SHADER_INVOCATIONS ], stat_unit_name,
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_TASK_SHADER_INVOCATIONS ], stat_unit_name );

      ImGui::Text( "Invocations divided by number of full screen quad pixels." );
      ImGui::Text( "Vertex %0.2f, Fragment %0.2f, Compute %0.2f", stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_VERTEX_SHADER_INVOCATIONS ] * stat_unit_multiplier / profiler->framebuffer_pixel_count,
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_FRAGMENT_SHADER_INVOCATIONS ] * stat_unit_multiplier / profiler->framebuffer_pixel_count,
        stat_values[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_COMPUTE_SHADER_INVOCATIONS ] * stat_unit_multiplier / profiler->framebuffer_pixel_count );
    }
  
    ImGui::Combo( "Stat Units", &stat_unit_index, stat_unit_names, IM_ARRAYSIZE( stat_unit_names ) );
  }

  ImGui::Separator( );
  
  {
    VmaBudget                                              gpu_memory_heap_budgets[ VK_MAX_MEMORY_HEAPS ];
    VkPhysicalDeviceProperties                             vk_physical_properties;
    uint64                                                 memory_used, memory_allocated;

    crude_memory_set(gpu_memory_heap_budgets, 0u, sizeof(gpu_memory_heap_budgets));
    vmaGetHeapBudgets( profiler->gpu->vma_allocator, gpu_memory_heap_budgets);

    memory_used = memory_allocated = 0;
    for ( uint32 i = 0; i < VK_MAX_MEMORY_HEAPS; ++i )
    {
      memory_used += gpu_memory_heap_budgets[i].usage;
      memory_allocated += gpu_memory_heap_budgets[i].budget;
    }

    vkGetPhysicalDeviceProperties( profiler->gpu->vk_physical_device, &vk_physical_properties );

    ImGui::Text( "GPU used: %s", vk_physical_properties.deviceName ? vk_physical_properties.deviceName : "Unknown" );
    ImGui::Text( "GPU Memory Used: %lluMB, Total: %lluMB", memory_used / ( 1024 * 1024 ), memory_allocated / ( 1024 * 1024 ) );

    ImGui::Separator();
    crude_gui_gpu_visual_profiler_pool_queue_draw_( &profiler->gpu->buffers, "Buffers" );
    crude_gui_gpu_visual_profiler_pool_queue_draw_( &profiler->gpu->textures, "Textures" );
    crude_gui_gpu_visual_profiler_pool_queue_draw_( &profiler->gpu->pipelines, "Pipelines" );
    crude_gui_gpu_visual_profiler_pool_queue_draw_( &profiler->gpu->samplers, "Samplers" );
    crude_gui_gpu_visual_profiler_pool_queue_draw_( &profiler->gpu->descriptor_sets, "DescriptorSets" );
    crude_gui_gpu_visual_profiler_pool_queue_draw_( &profiler->gpu->descriptor_set_layouts, "DescriptorSetLayouts" );
    crude_gui_gpu_visual_profiler_pool_queue_draw_( &profiler->gpu->framebuffers, "Framebuffers" );
    crude_gui_gpu_visual_profiler_pool_queue_draw_( &profiler->gpu->render_passes, "RenderPasses" );
    crude_gui_gpu_visual_profiler_pool_queue_draw_( &profiler->gpu->shaders, "Shaders" );
  }
}

void
crude_gui_gpu_visual_profiler_pool_queue_draw_
(
  _In_ crude_resource_pool                                *resource_pool,
  _In_ char const                                         *resource_name
)
{
  ImGui::Text( "Pool %s, indices used %u, allocated %u", resource_name, resource_pool->used_indices, resource_pool->pool_size );
}

#endif /* CRUDE_DEVELOP */