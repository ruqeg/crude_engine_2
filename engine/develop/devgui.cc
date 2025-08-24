#include <core/hash_map.h>
#include <core/ecs.h>
#include <scene/scene_components.h>
#include <scene/scripts_components.h>
#include <graphics/renderer_resources_loader.h>

#include <develop/devgui.h>

static void
crude_devgui_reload_techniques_
(
  _In_ crude_devgui                                       *devgui
)
{
  for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( devgui->renderer->resource_cache.techniques ); ++i )
  {
    if ( !devgui->renderer->resource_cache.techniques[ i ].key )
    {
      continue;
    }
    
    crude_gfx_renderer_technique *technique = devgui->renderer->resource_cache.techniques[ i ].value; 
    crude_gfx_renderer_destroy_technique( devgui->renderer, technique );
    crude_gfx_renderer_technique_load_from_file( technique->json_name, devgui->renderer, devgui->render_graph, &devgui->temporary_allocator );
  }

  crude_gfx_render_graph_on_techniques_reloaded( devgui->render_graph );
}

void
crude_devgui_initialize
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_heap_allocator                               *allocator
)
{
  devgui->should_reload_shaders = false;
  devgui->menubar_enabled = false;
  devgui->renderer = renderer;
  devgui->render_graph = render_graph;
  devgui->allocator = allocator;
  crude_stack_allocator_initialize( &devgui->temporary_allocator, CRUDE_RMEGA( 32 ), "devgui_temp_allocator" );
  crude_devgui_nodes_tree_initialize( &devgui->dev_nodes_tree );
  crude_devgui_node_inspector_initialize( &devgui->dev_node_inspector );
  crude_devgui_viewport_initialize( &devgui->dev_viewport, render_graph->builder->gpu );
  crude_devgui_render_graph_initialize( &devgui->dev_render_graph, render_graph );
  crude_devgui_gpu_initialize( &devgui->dev_gpu, render_graph->builder->gpu, &devgui->temporary_allocator );
  crude_devgui_gpu_visual_profiler_initialize( &devgui->dev_gpu_profiler, render_graph->builder->gpu, allocator );
}

void
crude_devgui_deinitialize
(
  _In_ crude_devgui                                       *devgui
)
{
  crude_devgui_gpu_visual_profiler_deinitialize( &devgui->dev_gpu_profiler );
  crude_stack_allocator_deinitialize( &devgui->temporary_allocator );
}

void
crude_devgui_draw
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_entity                                        main_scene_node,
  _In_ crude_entity                                        camera_node
)
{
  if ( devgui->menubar_enabled && ImGui::BeginMainMenuBar( ) )
  {
    if ( ImGui::BeginMenu( "Graphics" ) )
    {
      if ( ImGui::MenuItem( "Reload Techniques" ) )
      {
        devgui->should_reload_shaders = true;
      }
      if ( ImGui::MenuItem( "Render Graph" ) )
      {
        devgui->dev_render_graph.enabled = !devgui->dev_render_graph.enabled;
      }
      if ( ImGui::MenuItem( "GPU Pools" ) )
      {
        devgui->dev_gpu.enabled = !devgui->dev_gpu.enabled;
      }
      if ( ImGui::MenuItem( "GPU Profiler" ) )
      {
        devgui->dev_gpu_profiler.enabled = !devgui->dev_gpu_profiler.enabled;
      }
      ImGui::EndMenu( );
    }
    if ( ImGui::BeginMenu( "Scene" ) )
    {
      if ( ImGui::MenuItem( "Node Tree" ) )
      {
        devgui->dev_nodes_tree.enabled = !devgui->dev_nodes_tree.enabled;
      }
      if ( ImGui::MenuItem( "Node Inpsector" ) )
      {
        devgui->dev_node_inspector.enabled = !devgui->dev_node_inspector.enabled;
      }
      ImGui::EndMenu( );
    }
    ImGui::EndMainMenuBar( );
  }

  crude_devgui_nodes_tree_draw( &devgui->dev_nodes_tree, main_scene_node );
  crude_devgui_node_inspector_draw( &devgui->dev_node_inspector, devgui->dev_nodes_tree.selected_node );
  crude_devgui_viewport_draw( &devgui->dev_viewport, camera_node, devgui->dev_nodes_tree.selected_node );
  crude_devgui_render_graph_draw( &devgui->dev_render_graph );
  crude_devgui_gpu_draw( &devgui->dev_gpu );
  crude_devgui_gpu_visual_profiler_draw( &devgui->dev_gpu_profiler );
}

void
crude_devgui_handle_input
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_input                                        *input
)
{
  if ( input->keys[ 9 ].pressed )
  {
    devgui->menubar_enabled = !devgui->menubar_enabled;
  }
  crude_devgui_viewport_input( &devgui->dev_viewport, input );
}

void
crude_devgui_on_resize
(
  _In_ crude_devgui                                       *devgui
)
{
}

void
crude_devgui_graphics_pre_update
(
  _In_ crude_devgui                                       *devgui
)
{
  crude_devgui_gpu_visual_profiler_update( &devgui->dev_gpu_profiler );
}

void
crude_devgui_graphics_post_update
(
  _In_ crude_devgui                                       *devgui
)
{
  if ( devgui->should_reload_shaders )
  {
    crude_devgui_reload_techniques_( devgui );
    devgui->should_reload_shaders = false;
  }
}

/******************************
 * Dev Gui Nodes Tree
 *******************************/
static void
crude_devgui_nodes_tree_draw_internal_
(
  _In_ crude_devgui_nodes_tree                            *devgui_nodes_tree,
  _In_ crude_entity                                        node,
  _In_ uint32                                             *current_node_index
);

void
crude_devgui_nodes_tree_initialize
(
  _In_ crude_devgui_nodes_tree                            *devgui_nodes_tree
)
{
  devgui_nodes_tree->enabled = false;
  devgui_nodes_tree->selected_node = CRUDE_COMPOUNT_EMPTY( crude_entity );
  devgui_nodes_tree->selected_node_index = 0;
}

void
crude_devgui_nodes_tree_draw
(
  _In_ crude_devgui_nodes_tree                            *devgui_nodes_tree,
  _In_ crude_entity                                        node
)
{
  if ( !devgui_nodes_tree->enabled )
  {
    return;
  }

  uint32 current_node_index = 0u;
  crude_devgui_nodes_tree_draw_internal_( devgui_nodes_tree, node, &current_node_index );
}

void
crude_devgui_nodes_tree_draw_internal_
(
  _In_ crude_devgui_nodes_tree                            *devgui_nodes_tree,
  _In_ crude_entity                                        node,
  _In_ uint32                                             *current_node_index
)
{
  ImGui::Begin( "Scene Node Tree" );

  ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
  
  if ( devgui_nodes_tree->selected_node_index == *current_node_index )
  {
    tree_node_flags |= ImGuiTreeNodeFlags_Selected;
  }

  bool tree_node_open = ImGui::TreeNodeEx( ( void* )( intptr_t )*current_node_index, tree_node_flags, crude_entity_get_name( node ) );
  if ( ImGui::IsItemClicked( ) && !ImGui::IsItemToggledOpen( ) )
  {
    devgui_nodes_tree->selected_node = node;
    devgui_nodes_tree->selected_node_index = *current_node_index;
  }

  if (ImGui::BeginDragDropSource( ) )
  {
    ImGui::SetDragDropPayload( crude_entity_get_name( node ), NULL, 0 );
    ImGui::Text( crude_entity_get_name( node ) );
    ImGui::EndDragDropSource( );
  }

  ++( *current_node_index );
  if ( tree_node_open )
  {
    ecs_iter_t it = ecs_children( node.world, node.handle );
    while ( ecs_children_next( &it ) )
    {
      for (int i = 0; i < it.count; i ++)
      {
        crude_entity child = CRUDE_COMPOUNT_EMPTY( crude_entity );
        child.world = it.world;
        child.handle = it.entities[ i ];
        crude_devgui_nodes_tree_draw_internal_( devgui_nodes_tree, child, current_node_index );
      }
    }
    ImGui::TreePop( );
  }

  ImGui::End( );
}

/******************************
 * Dev Gui Node Inspector
 *******************************/
void
crude_devgui_node_inspector_initialize
(
  _In_ crude_devgui_node_inspector                        *devgui_inspector
)
{
  devgui_inspector->enabled = false;
}

void
crude_devgui_node_inspector_draw
(
  _In_ crude_devgui_node_inspector                        *devgui_inspector,
  _In_ crude_entity                                        node
)
{
  if ( !devgui_inspector->enabled )
  {
    return;
  }

  if ( !crude_entity_valid( node ) )
  {
    return;
  }

  ImGui::Begin( "Node Inspector" );
  ImGui::Text( "Node: \"%s\"", crude_entity_get_name( node ) );
  
  crude_transform *transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_transform );
  if ( transform && ImGui::CollapsingHeader( "crude_transform" ) )
  {
    ImGui::DragFloat3( "Translation", &transform->translation.x, .1f );
    ImGui::DragFloat3( "Scale", &transform->scale.x, .1f );
  }
  
  crude_free_camera *free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_free_camera );
  if ( free_camera && ImGui::CollapsingHeader( "crude_free_camera" ) )
  {
    ImGui::DragFloat3( "Moving Speed", &free_camera->moving_speed_multiplier.x, .1f );
    ImGui::DragFloat2( "Rotating Speed", &free_camera->rotating_speed_multiplier.x, .1f );
  }
  
  crude_camera *camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_camera );
  if ( camera && ImGui::CollapsingHeader( "crude_camera" ) )
  {
    ImGui::InputFloat( "Far Z", &camera->far_z );
    ImGui::InputFloat( "Near Z", &camera->near_z );
    ImGui::SliderAngle( "FOV Radians", &camera->fov_radians );
    ImGui::InputFloat( "Aspect Ratio", &camera->aspect_ratio );
  }
  
  crude_light *light = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_light );
  if ( light && ImGui::CollapsingHeader( "crude_light" ) )
  {
    ImGui::ColorEdit3( "color", &light->color.x );
    ImGui::InputFloat( "intensity", &light->intensity );
    ImGui::InputFloat( "radius", &light->radius );
  }
  ImGui::End( );
}

/******************************
 * Dev Gui Viewport
 *******************************/
void
crude_devgui_viewport_initialize
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_gfx_device                                   *gpu
)
{
  devgui_viewport->gpu = gpu;
  devgui_viewport->selected_texture = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
}

void
crude_devgui_viewport_draw
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_entity                                        camera_node,
  _In_ crude_entity                                        selected_node
)
{
  crude_camera                                            *camera;
  crude_transform                                         *camera_transform;
  crude_transform                                         *selected_node_transform;
  
  ImGui::Begin( "Viewport" );

  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( devgui_viewport->selected_texture ) )
  {
    ImGui::Image( CRUDE_CAST( ImTextureRef, &devgui_viewport->selected_texture.index ), ImGui::GetContentRegionAvail( ) );
  }

  ImGui::SetCursorPos( ImGui::GetWindowContentRegionMin( ) );

  char const *preview_texture_name = "Unknown";
  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( devgui_viewport->selected_texture ) )
  {
    crude_gfx_texture *selected_texture = crude_gfx_access_texture( devgui_viewport->gpu, devgui_viewport->selected_texture );
    if ( selected_texture && selected_texture->name )
    {
      preview_texture_name = selected_texture->name;
    };
  }
  uint32 id = 0;
  if ( ImGui::BeginCombo( "Texture ID", preview_texture_name ) )
  {
    for ( uint32 t = 0; t < devgui_viewport->gpu->textures.pool_size; ++t )
    {
      crude_gfx_texture_handle texture_handle = CRUDE_CAST( crude_gfx_texture_handle, t );
      if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( texture_handle ) )
      {
        continue;
      }
      
      crude_gfx_texture *texture = crude_gfx_access_texture( devgui_viewport->gpu, texture_handle );
      if ( !texture || !texture->name )
      {
        continue;
      }
      
      ImGui::PushID( id++ );

      bool is_selected = ( devgui_viewport->selected_texture.index == texture_handle.index );
      if ( ImGui::Selectable( texture->name ) )
      {
        devgui_viewport->selected_texture = texture_handle;
      }
      
      ImGui::PopID( );
      if ( is_selected )
      {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::End();
}

void
crude_devgui_viewport_input
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_input                                        *input
)
{
}

/******************************
 * Dev Gui Render Graph
 *******************************/
void
crude_devgui_render_graph_initialize
(
  _In_ crude_devgui_render_graph                          *devgui_render_graph,
  _In_ crude_gfx_render_graph                             *render_graph
)
{
  devgui_render_graph->render_graph = render_graph;
  devgui_render_graph->enabled = false;
}

void
crude_devgui_render_graph_draw
(
  _In_ crude_devgui_render_graph                          *devgui_render_graph
)
{
  if ( !devgui_render_graph->enabled )
  {
    return;
  }
  if ( ImGui::Begin( "Render Graph Debug" ) )
  {
    if ( ImGui::CollapsingHeader( "Nodes" ) )
    {
      for ( uint32 n = 0; n < CRUDE_ARRAY_LENGTH( devgui_render_graph->render_graph->nodes ); ++n )
      {
        crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( devgui_render_graph->render_graph->builder, devgui_render_graph->render_graph->nodes[ n ] );

        ImGui::Separator( );
        ImGui::Text( "Pass: %s", node->name );

        ImGui::Text( "\tInputs" );
        for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( node->inputs ); ++i )
        {
          crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( devgui_render_graph->render_graph->builder, node->inputs[ i ] );
          ImGui::Text( "\t\t%s %u %u", resource->name, resource->resource_info.texture.handle.index, resource->resource_info.buffer.handle.index );
        }

        ImGui::Text( "\tOutputs" );
        for ( uint32 o = 0; o < CRUDE_ARRAY_LENGTH( node->outputs ); ++o )
        {
          crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( devgui_render_graph->render_graph->builder, node->outputs[ o ] );
          ImGui::Text( "\t\t%s %u %u", resource->name, resource->resource_info.texture.handle.index, resource->resource_info.buffer.handle.index );
        }
      }
    }
  }
  ImGui::End( );
}

/******************************
 * Dev Gui GPU
 *******************************/
static void
crude_devgui_gpu_pool_draw_
(
  _In_ crude_resource_pool                                *resource_pool,
  _In_ char const                                         *resource_name
)
{
  ImGui::Text( "Pool %s, indices used %u, allocated %u", resource_name, resource_pool->used_indices, resource_pool->pool_size );
}

void
crude_devgui_gpu_initialize
(
  _In_ crude_devgui_gpu                                   *dev_gpu,
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  dev_gpu->enabled = false;
  dev_gpu->gpu = gpu;
  dev_gpu->temporary_allocator = temporary_allocator;
}

void
crude_devgui_gpu_draw
(
  _In_ crude_devgui_gpu                                   *dev_gpu
)
{
  if ( !dev_gpu->enabled )
  {
    return;
  }

  VkPhysicalDeviceProperties                               vk_physical_properties;
  VmaBudget                                                gpu_memory_heap_budgets[ VK_MAX_MEMORY_HEAPS ];
  uint64                                                   memory_used, memory_allocated;

  crude_memory_set( gpu_memory_heap_budgets, 0u, sizeof( gpu_memory_heap_budgets ) );
  vmaGetHeapBudgets( dev_gpu->gpu->vma_allocator, gpu_memory_heap_budgets );
 
  memory_used = memory_allocated = 0;
  for ( uint32 i = 0; i < VK_MAX_MEMORY_HEAPS; ++i )
  {
    memory_used += gpu_memory_heap_budgets[ i ].usage;
    memory_allocated += gpu_memory_heap_budgets[ i ].budget;
  }
   
  vkGetPhysicalDeviceProperties( dev_gpu->gpu->vk_physical_device, &vk_physical_properties );

  ImGui::Text( "GPU used: %s", vk_physical_properties.deviceName ? vk_physical_properties.deviceName : "Unknown" );
  ImGui::Text( "GPU Memory Used: %lluMB, Total: %lluMB", memory_used / ( 1024 * 1024 ), memory_allocated / ( 1024 * 1024 ) );

  ImGui::Separator();
  crude_devgui_gpu_pool_draw_( &dev_gpu->gpu->buffers, "Buffers" );
  crude_devgui_gpu_pool_draw_( &dev_gpu->gpu->textures, "Textures" );
  crude_devgui_gpu_pool_draw_( &dev_gpu->gpu->pipelines, "Pipelines" );
  crude_devgui_gpu_pool_draw_( &dev_gpu->gpu->samplers, "Samplers" );
  crude_devgui_gpu_pool_draw_( &dev_gpu->gpu->descriptor_sets, "DescriptorSets" );
  crude_devgui_gpu_pool_draw_( &dev_gpu->gpu->descriptor_set_layouts, "DescriptorSetLayouts" );
  crude_devgui_gpu_pool_draw_( &dev_gpu->gpu->framebuffers, "Framebuffers" );
  crude_devgui_gpu_pool_draw_( &dev_gpu->gpu->render_passes, "RenderPasses" );
  crude_devgui_gpu_pool_draw_( &dev_gpu->gpu->shaders, "Shaders" );
}

/******************************
 * Dev Gui GPU Visual Profiler
 *******************************/
void
crude_devgui_gpu_visual_profiler_initialize
(
  _In_ crude_devgui_gpu_visual_profiler                   *dev_gpu_profiler,
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_heap_allocator                               *allocator
)
{
  dev_gpu_profiler->gpu = gpu;
  dev_gpu_profiler->enabled = false;
  dev_gpu_profiler->max_duration = 16.666f;
  dev_gpu_profiler->max_frames = 100u;
  dev_gpu_profiler->current_frame = 0u;
  dev_gpu_profiler->max_visible_depth = 2;
  dev_gpu_profiler->max_queries_per_frame = 32;
  dev_gpu_profiler->allocator = allocator;
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
crude_devgui_gpu_visual_profiler_deinitialize
(
  _In_ crude_devgui_gpu_visual_profiler                   *dev_gpu_profiler
)
{
  CRUDE_HASHMAP_DEINITIALIZE( dev_gpu_profiler->name_hashed_to_color_index );
  CRUDE_DEALLOCATE( crude_heap_allocator_pack( dev_gpu_profiler->allocator ), dev_gpu_profiler->timestamps );
  CRUDE_DEALLOCATE( crude_heap_allocator_pack( dev_gpu_profiler->allocator ), dev_gpu_profiler->per_frame_active );
}

void
crude_devgui_gpu_visual_profiler_update
(
  _In_ crude_devgui_gpu_visual_profiler                   *dev_gpu_profiler
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
    color_index;

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
crude_devgui_gpu_visual_profiler_draw
(
  _In_ crude_devgui_gpu_visual_profiler                   *dev_gpu_profiler
)
{
  if ( !dev_gpu_profiler->enabled )
  {
    return;
  }

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
}