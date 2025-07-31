#include <imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <core/hash_map.h>
#include <core/file.h>
#include <core/process.h>
#include <platform/platform_system.h>
#include <platform/platform_components.h>
#include <scene/free_camera_system.h>
#include <scene/scene_components.h>
#include <scene/scripts_components.h>
#include <graphics/renderer_resources_loader.h>

#include <paprika.h>

CRUDE_ECS_SYSTEM_DECLARE( paprika_graphics_system_ );
CRUDE_ECS_SYSTEM_DECLARE( paprika_update_system_ );

static void
paprika_graphics_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
paprika_update_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
paprika_graphics_initialize_
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_window_handle                                 window_handle
);

static void
paprika_graphics_deinitialize_
(
  _In_ crude_paprika                                      *paprika
);

static void
paprika_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
);

static void
paprika_draw_imgui_
(
  _In_ crude_paprika                                      *paprika
);

static void
paprika_draw_imgui_debug_
(
  _In_ crude_paprika                                      *paprika
);

static void
paprika_draw_imgui_scene_nodes_
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_entity                                        node,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ uint32                                             *current_node_index
);

static void
paprika_draw_imgui_inspector_node_
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_string_buffer                                *temporary_string_buffer
);

static void
paprika_imgui_draw_viewport_
(  
  _In_ crude_paprika                                      *paprika
);

void
crude_paprika_initialize
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_engine                                       *engine
)
{
  paprika->engine = engine;
  paprika->working = true;

  crude_heap_allocator_initialize( &paprika->graphics_allocator, CRUDE_RMEGA( 64 ), "renderer_allocator" );
  crude_stack_allocator_initialize( &paprika->temporary_allocator, CRUDE_RMEGA( 64 ), "temprorary_allocator" );
  
  ECS_IMPORT( paprika->engine->world, crude_platform_system );
  ECS_IMPORT( paprika->engine->world, crude_free_camera_system );
  
  {
    IMGUI_CHECKVERSION();
    paprika->imgui_context = ImGui::CreateContext();
    ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, paprika->imgui_context ) );
    ImGui::StyleColorsDark();
    ImGuiIO *imgui_io = &ImGui::GetIO();
    imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NoMouseCursorChange;
    imgui_io->ConfigWindowsResizeFromEdges = true;
    imgui_io->ConfigWindowsMoveFromTitleBarOnly = true;
  }

  paprika->platform_node = crude_entity_create_empty( paprika->engine->world, "paprika" );
  CRUDE_ENTITY_SET_COMPONENT( paprika->platform_node, crude_window, { 
    .width     = 800,
    .height    = 600,
    .maximized = false,
    .flags     = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
  });
  CRUDE_ENTITY_SET_COMPONENT( paprika->platform_node, crude_input, {
    .callback = paprika_input_callback_,
    .ctx = paprika->imgui_context
  } );
  
  /* Create scene */
  {
    crude_scene_creation creation = {
      .world = paprika->engine->world,
      .input_entity = paprika->platform_node,
      .resources_path = "\\..\\..\\resources\\",
      .temporary_allocator = &paprika->temporary_allocator,
      .allocator_container = crude_heap_allocator_pack( &paprika->graphics_allocator ), 
    };
    crude_scene_initialize( &paprika->scene, &creation );
  }
  crude_scene_load( &paprika->scene, "scene.json" );

  /* Create Graphics */
  {
    crude_window_handle *window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( paprika->platform_node, crude_window_handle );
    paprika_graphics_initialize_( paprika, *window_handle );
    CRUDE_ECS_SYSTEM_DEFINE( paprika->engine->world, paprika_graphics_system_, EcsPreStore, paprika, {
      { .id = ecs_id( crude_transform ) },
      { .id = ecs_id( crude_free_camera ) },
    } );
  }
  
  CRUDE_ECS_SYSTEM_DEFINE( paprika->engine->world, paprika_update_system_, EcsOnUpdate, paprika, {
    { .id = ecs_id( crude_input ) },
    { .id = ecs_id( crude_window_handle ) },
  } );

  paprika->debug_camera_culling = false;
  paprika->debug_camera_view = false;

  crude_free_camera *free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( paprika->scene.main_camera, crude_free_camera );
  free_camera->enabled = true;
}

void
crude_paprika_deinitialize
(
  _In_ crude_paprika                                      *paprika
)
{
  if ( !paprika->working )
  {
    return;
  }

  paprika->working = false;
  crude_entity_destroy( paprika->platform_node );
  crude_scene_deinitialize( &paprika->scene );
  paprika_graphics_deinitialize_( paprika );
  crude_heap_allocator_deinitialize( &paprika->graphics_allocator );
  crude_stack_allocator_deinitialize( &paprika->temporary_allocator );

  ImGui::DestroyContext( ( ImGuiContext* )paprika->imgui_context );
}

void
crude_paprika_update
(
  _In_ crude_paprika                                      *paprika
)
{
  crude_input const *input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( paprika->platform_node, crude_input );
  if ( input && input->should_close_window )
  {
    crude_paprika_deinitialize( paprika );
  }
}

static void
paprika_graphics_initialize_
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_window_handle                                 window_handle
)
{
  uint32                                                   temporary_allocator_marker;

  temporary_allocator_marker = crude_stack_allocator_get_marker( &paprika->temporary_allocator );
  
  paprika->graphics.asynchronous_loader_manager = &paprika->engine->asynchronous_loader_manager;
  paprika->graphics.allocator_container = crude_heap_allocator_pack( &paprika->graphics_allocator );
  
  /* Create Device */
  {
    crude_gfx_device_creation device_creation = {
      .sdl_window             = CRUDE_REINTERPRET_CAST( SDL_Window*, window_handle.value ),
      .vk_application_name    = "CrudeEngine",
      .vk_application_version = VK_MAKE_VERSION( 1, 0, 0 ),
      .allocator_container    = paprika->graphics.allocator_container,
      .temporary_allocator    = &paprika->temporary_allocator,
      .queries_per_frame      = 1u,
      .num_threads            = CRUDE_STATIC_CAST( uint16, enkiGetNumTaskThreads( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, paprika->graphics.asynchronous_loader_manager->task_sheduler ) ) ),
    };
    crude_gfx_device_initialize( &paprika->graphics.gpu, &device_creation );
  }
  
  /* Create Renderer */
  {
    crude_gfx_renderer_creation renderer_creation = {
      .gpu                 = &paprika->graphics.gpu,
      .allocator_container = paprika->graphics.allocator_container,
    };
    crude_gfx_renderer_initialize( &paprika->graphics.renderer, &renderer_creation );
  }    
  
  /* Create Render Graph */
  crude_gfx_render_graph_builder_initialize( &paprika->graphics.render_graph_builder, &paprika->graphics.gpu );
  crude_gfx_render_graph_initialize( &paprika->graphics.render_graph, &paprika->graphics.render_graph_builder );
  
  /* Create Async Loader */
  {
    crude_gfx_asynchronous_loader_initialize( &paprika->graphics.async_loader, &paprika->graphics.renderer );
    crude_gfx_asynchronous_loader_manager_add_loader( paprika->graphics.asynchronous_loader_manager, &paprika->graphics.async_loader );
  }
  
  /* Parse render graph*/
  {
    crude_string_buffer                                    temporary_name_buffer;
    char                                                   working_directory[ 512 ];
    char const                                            *render_graph_file_path;

    crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
    crude_string_buffer_initialize( &temporary_name_buffer, 1024, crude_stack_allocator_pack( &paprika->temporary_allocator ) );
  
    render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", working_directory, "\\..\\..\\resources\\render_graph.json", "\\..\\..\\resources\\render_graph.json" );
    crude_gfx_render_graph_parse_from_file( &paprika->graphics.render_graph, render_graph_file_path, &paprika->temporary_allocator );
    crude_gfx_render_graph_compile( &paprika->graphics.render_graph );
  }
  
  /* Create Render Tecnhique & Renderer Passes*/
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\mesh_technique.json", &paprika->graphics.renderer, &paprika->graphics.render_graph, &paprika->temporary_allocator );
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\meshlet_technique.json", &paprika->graphics.renderer, &paprika->graphics.render_graph, &paprika->temporary_allocator );
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\culling_technique.json", &paprika->graphics.renderer, &paprika->graphics.render_graph, &paprika->temporary_allocator );
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\shaders\\debug_technique.json", &paprika->graphics.renderer, &paprika->graphics.render_graph, &paprika->temporary_allocator );

  /* Create Scene Renderer */
  {
    crude_gfx_scene_renderer_creation creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene_renderer_creation );
    creation.node = paprika->scene.main_node;
    creation.renderer = &paprika->graphics.renderer;
    creation.async_loader = &paprika->graphics.async_loader;
    creation.allocator_container = paprika->graphics.allocator_container;
    creation.temporary_allocator = &paprika->temporary_allocator;
    creation.task_scheduler = paprika->graphics.asynchronous_loader_manager->task_sheduler;
    creation.imgui_context = paprika->imgui_context;
    crude_gfx_scene_renderer_initialize( &paprika->graphics.scene_renderer, &creation );
    crude_gfx_scene_renderer_register_passes( &paprika->graphics.scene_renderer, &paprika->graphics.render_graph );
  }
  
  crude_stack_allocator_free_marker( &paprika->temporary_allocator, temporary_allocator_marker );

  paprika->selected_node = paprika->scene.main_node;
  paprika->gizmo_operation = ImGuizmo::TRANSLATE;
  paprika->gizmo_mode = ImGuizmo::WORLD;
}

void
paprika_graphics_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_paprika *paprika = ( crude_paprika* )it->ctx;

  crude_gfx_new_frame( &paprika->graphics.gpu );
  
  ImGui::SetCurrentContext( ( ImGuiContext* ) paprika->imgui_context );
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
  ImGuizmo::SetOrthographic( false );
  ImGuizmo::BeginFrame();
  ImGui::DockSpaceOverViewport( 0u, ImGui::GetMainViewport( ) );

  if ( paprika->graphics.gpu.swapchain_resized_last_frame )
  {
    crude_gfx_render_graph_on_resize( &paprika->graphics.render_graph, paprika->graphics.gpu.vk_swapchain_width, paprika->graphics.gpu.vk_swapchain_height );
  }
  
  /* Update frame buffer */
  {
    crude_gfx_map_buffer_parameters frame_buffer_map = { paprika->graphics.scene_renderer.scene_cb, 0 };
    crude_gfx_scene_constant_gpu *scene_data = CRUDE_REINTERPRET_CAST( crude_gfx_scene_constant_gpu*, crude_gfx_map_buffer( &paprika->graphics.gpu, &frame_buffer_map ) );
    if ( scene_data )
    {
      scene_data->flags = 0u;
      if ( paprika->debug_camera_culling )
      {
        scene_data->flags |= 1;
      }
      if ( paprika->debug_camera_view )
      {
        scene_data->flags |= 2;
      }
      scene_data->camera_previous = scene_data->camera;
      scene_data->camera_debug_previous = scene_data->camera_debug;
      crude_gfx_camera_to_camera_gpu( paprika->scene.main_camera, &scene_data->camera );
      crude_gfx_camera_to_camera_gpu( paprika->scene.debug_camera, &scene_data->camera_debug );
      crude_gfx_unmap_buffer( &paprika->graphics.gpu, paprika->graphics.scene_renderer.scene_cb );
    }
  }

  paprika_draw_imgui_( paprika );
  
  crude_gfx_scene_renderer_submit_draw_task( &paprika->graphics.scene_renderer, false );
  
  crude_gfx_renderer_add_texture_update_commands( &paprika->graphics.renderer, 0 );

  {
    crude_gfx_render_graph_node *final_render_graph_node = crude_gfx_render_graph_builder_access_node_by_name( &paprika->graphics.render_graph_builder, "imgui_pass" );
    crude_gfx_framebuffer *final_render_framebuffer = crude_gfx_access_framebuffer( &paprika->graphics.gpu, final_render_graph_node->framebuffer );
    crude_gfx_texture *final_render_texture = crude_gfx_access_texture( &paprika->graphics.gpu, final_render_framebuffer->color_attachments[ 0 ] );
    crude_gfx_present( &paprika->graphics.gpu, final_render_texture );
  }
}

void
paprika_graphics_deinitialize_
(
  _In_ crude_paprika                                      *paprika
)
{
  crude_gfx_asynchronous_loader_manager_remove_loader( paprika->graphics.asynchronous_loader_manager, &paprika->graphics.async_loader );
  vkDeviceWaitIdle( paprika->graphics.gpu.vk_device );
  crude_gfx_scene_renderer_deinitialize( &paprika->graphics.scene_renderer );
  crude_gfx_asynchronous_loader_deinitialize( &paprika->graphics.async_loader );
  crude_gfx_render_graph_deinitialize( &paprika->graphics.render_graph );
  crude_gfx_render_graph_builder_deinitialize( &paprika->graphics.render_graph_builder );
  crude_gfx_renderer_deinitialize( &paprika->graphics.renderer );
  crude_gfx_device_deinitialize( &paprika->graphics.gpu );
}

void
paprika_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_paprika *paprika = ( crude_paprika* )it->ctx;
  crude_input *input_per_entity = ecs_field( it, crude_input, 0 );
  crude_window_handle *window_handle_per_entity = ecs_field( it, crude_window_handle, 1 );

  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, paprika->imgui_context ) );
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input *input = &input_per_entity[ i ];
    crude_window_handle *window_handle = &window_handle_per_entity[ i ];
    
    if ( input->mouse.right.current && input->mouse.right.current != input->prev_mouse.right.current )
    {
      ImGuiIO *imgui_io = &ImGui::GetIO();
      imgui_io->ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
      paprika->wrapwnd.x = input->mouse.wnd.x;
      paprika->wrapwnd.y = input->mouse.wnd.y;
      SDL_HideCursor();
    }
    
    if ( !input->mouse.right.current && input->mouse.right.current != input->prev_mouse.right.current )
    {
      ImGuiIO *imgui_io = &ImGui::GetIO();
      imgui_io->ConfigFlags ^= ImGuiConfigFlags_NoMouseCursorChange;
      SDL_WarpMouseInWindow( CRUDE_CAST( SDL_Window*, window_handle->value ), paprika->wrapwnd.x, paprika->wrapwnd.y );
      SDL_ShowCursor();
    }
    
    if ( input->mouse.rel.x || input->mouse.rel.y )
    {
      if ( !SDL_CursorVisible() )
      {
        SDL_WarpMouseInWindow( CRUDE_CAST( SDL_Window*, window_handle->value ), paprika->wrapwnd.x, paprika->wrapwnd.y );
      }
    }
    if ( input->keys[ 'z' ].pressed )
    {
      paprika->gizmo_operation = ImGuizmo::TRANSLATE;
    }
    if ( input->keys[ 'x' ].pressed )
    {
      paprika->gizmo_operation = ImGuizmo::ROTATE;
    }
    if ( input->keys[ 'c' ].pressed )
    {
      paprika->gizmo_operation = ImGuizmo::SCALE;
    }
  }
}

void
paprika_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
)
{
  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, ctx ) );
  ImGui_ImplSDL3_ProcessEvent( CRUDE_CAST( SDL_Event*, sdl_event ) );
}

void
paprika_draw_imgui_
(
  _In_ crude_paprika                                      *paprika
)
{
  crude_string_buffer                                      temporary_string_buffer;
  uint32                                                   temporary_allocator_mark;
  
  temporary_allocator_mark = crude_stack_allocator_get_marker( &paprika->temporary_allocator );
  crude_string_buffer_initialize( &temporary_string_buffer, 4096, crude_stack_allocator_pack( &paprika->temporary_allocator ) );
  
  ImGui::Begin( "techniques" );
  for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( paprika->graphics.renderer.resource_cache.techniques ); ++i )
  {
    if ( !paprika->graphics.renderer.resource_cache.techniques[ i ].key )
    {
      continue;
    }
  
    crude_gfx_renderer_technique *technique = paprika->graphics.renderer.resource_cache.techniques[ i ].value;
    if ( ImGui::TreeNode( technique->name ) )
    {
      if ( ImGui::Button( "reload" ) )
      {
        char const *json_name = technique->json_name;
        crude_gfx_renderer_destroy_technique( &paprika->graphics.renderer, technique );
        crude_gfx_renderer_technique_load_from_file( json_name, &paprika->graphics.renderer, &paprika->graphics.render_graph, &paprika->temporary_allocator );
      }
      ImGui::TreePop();
    }
  }
  ImGui::End();
  ImGui::Begin("scene");
  uint32 current_node_index = 0u;
  paprika_draw_imgui_scene_nodes_( paprika, paprika->scene.main_node, &temporary_string_buffer, &current_node_index );
  ImGui::End();
  ImGui::Begin("inspector");
  paprika_draw_imgui_inspector_node_( paprika, &temporary_string_buffer );
  ImGui::End();
  ImGui::Begin("viewport");
  paprika_imgui_draw_viewport_( paprika );
  ImGui::End();
  ImGui::Begin( "debug" );
  paprika_draw_imgui_debug_( paprika );
  ImGui::End( );
  crude_stack_allocator_free_marker( &paprika->temporary_allocator, temporary_allocator_mark );
}

void
paprika_draw_imgui_debug_
(
  _In_ crude_paprika                                      *paprika
)
{
  ImGui::Checkbox( "debug_camera_culling", &paprika->debug_camera_culling );
  if ( ImGui::Checkbox( "debug_camera_view", &paprika->debug_camera_view ) )
  {
    crude_free_camera *main_free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( paprika->scene.main_camera, crude_free_camera );
    crude_free_camera *debug_free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( paprika->scene.debug_camera, crude_free_camera );

    main_free_camera->enabled = !paprika->debug_camera_view;
    debug_free_camera->enabled = paprika->debug_camera_view;
  }
}

void
paprika_draw_imgui_scene_nodes_
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_entity                                        node,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ uint32                                             *current_node_index
)
{
  ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
  
  if ( paprika->selected_node_index == *current_node_index )
  {
    tree_node_flags |= ImGuiTreeNodeFlags_Selected;
  }

  bool tree_node_open = ImGui::TreeNodeEx( ( void* )( intptr_t )*current_node_index, tree_node_flags, crude_entity_get_name( node ) );
  if ( ImGui::IsItemClicked( ) && !ImGui::IsItemToggledOpen( ) )
  {
    paprika->selected_node = node;
    paprika->selected_node_index = *current_node_index;
  }

  char const *popup_id = crude_string_buffer_append_use_f( temporary_string_buffer, "node_pop_up %s", crude_entity_get_name( node ) );

  if (ImGui::IsItemClicked( ImGuiMouseButton_Right ) && !ImGui::IsItemToggledOpen())
  {
    ImGui::OpenPopup( popup_id );
  }

  if ( ImGui::BeginPopup( popup_id ) )
  {
    if ( ImGui::BeginMenu( "add_component" ) )
    {
      //for (Component_Type componentType : cComponentsTypes)
      //{
      //  if (ImGui::MenuItem(componentTypeToStr(componentType)))
      //  {
      //    addComponentToNode(node, componentType);
      //  }
      //}
      //ImGui::EndMenu();
    }

    if ( ImGui::MenuItem( "add_node" ) )
    {
    }
    ImGui::EndPopup();
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
        paprika_draw_imgui_scene_nodes_( paprika, child, temporary_string_buffer, current_node_index );
      }
    }
    ImGui::TreePop( );
  }
}

void
paprika_draw_imgui_inspector_node_
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_string_buffer                                *temporary_string_buffer
)
{
  if ( !crude_entity_valid( paprika->selected_node ) )
  {
    return;
  }

  ImGui::Text( "node: \"%s\"", crude_entity_get_name( paprika->selected_node ) );
  
  crude_transform *transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( paprika->selected_node, crude_transform );
  if ( transform && ImGui::CollapsingHeader( "crude_transform" ) )
  {
    ImGui::DragFloat3( "translation", &transform->translation.x, .1f );
    ImGui::DragFloat3( "scale", &transform->scale.x, .1f );
  }
  
  crude_free_camera *free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( paprika->selected_node, crude_free_camera );
  if ( free_camera && ImGui::CollapsingHeader( "crude_free_camera" ) )
  {
    ImGui::DragFloat3( "moving_speed", &free_camera->moving_speed_multiplier.x, .1f );
    ImGui::DragFloat2( "rotating_speed", &free_camera->rotating_speed_multiplier.x, .1f );
  }
  
  crude_camera *camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( paprika->selected_node, crude_camera );
  if ( camera && ImGui::CollapsingHeader( "crude_camera" ) )
  {
    ImGui::InputFloat( "far_z", &camera->far_z );
    ImGui::InputFloat( "near_z", &camera->near_z );
    ImGui::SliderAngle( "fov_radians", &camera->fov_radians );
    ImGui::InputFloat( "aspect_ratio", &camera->aspect_ratio );
  }
}
void
paprika_imgui_draw_viewport_
(  
  _In_ crude_paprika                                      *paprika
)
{
  crude_entity                                             camera_node;
  crude_camera                                            *camera;
  crude_transform                                         *camera_transform;
  crude_transform                                         *selected_node_transform;
  XMFLOAT4X4                                               selected_node_to_parent, selected_parent_to_view, view_to_clip;
  XMMATRIX                                                 world_to_view;
  crude_entity                                             selected_node_parent;
  XMVECTOR                                                 new_translation, new_rotation, new_scale;
  ImVec2                                                   viewport_size;

  ImGuizmo::SetDrawlist( );
  ImGuizmo::SetRect( ImGui::GetWindowPos( ).x, ImGui::GetWindowPos( ).y, ImGui::GetWindowWidth( ), ImGui::GetWindowHeight( ) );

  viewport_size = ImGui::GetContentRegionAvail();
  
  paprika->viewport_bindless_texture = 0u;
  for ( uint32 i = 0; i < paprika->graphics.gpu.textures.pool_size; ++i )
  {
    crude_gfx_texture *texture = crude_gfx_access_texture( &paprika->graphics.gpu, crude_gfx_texture_handle{ i } );
    if ( texture->name && crude_string_cmp( texture->name, "albedo" ) == 0 )
    {
      paprika->viewport_bindless_texture = i;
      break;
    }
  }

  ImGui::Image( CRUDE_CAST( ImTextureRef, &paprika->viewport_bindless_texture ), viewport_size );

  camera_node = paprika->debug_camera_view ? paprika->scene.debug_camera : paprika->scene.main_camera;
  camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( camera_node, crude_camera );
  camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( camera_node, crude_transform );
  selected_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( paprika->selected_node, crude_transform );

  if ( !camera || !camera_transform || !selected_node_transform )
  {
    return;
  }

  ImGui::SetCursorPos( ImGui::GetWindowContentRegionMin( ) );
  if ( ImGui::RadioButton( "translate", paprika->gizmo_operation == ImGuizmo::TRANSLATE ) )
  {
    paprika->gizmo_operation = ImGuizmo::TRANSLATE;
  }
  ImGui::SameLine();
  if ( ImGui::RadioButton( "rotate", paprika->gizmo_operation == ImGuizmo::ROTATE ) )
  {
    paprika->gizmo_operation = ImGuizmo::ROTATE;
  }
  ImGui::SameLine();
  if ( ImGui::RadioButton( "scale", paprika->gizmo_operation == ImGuizmo::SCALE ) )
  {
    paprika->gizmo_operation = ImGuizmo::SCALE;
  }
  if ( paprika->gizmo_operation != ImGuizmo::SCALE )
  {
    if ( ImGui::RadioButton( "local", paprika->gizmo_mode == ImGuizmo::LOCAL ) )
    {
      paprika->gizmo_mode = ImGuizmo::LOCAL;
    }
    ImGui::SameLine();
    if ( ImGui::RadioButton( "world", paprika->gizmo_mode == ImGuizmo::WORLD ) )
    {
      paprika->gizmo_mode = ImGuizmo::WORLD;
    }
  }

  ImGui::SetCursorPos( ImGui::GetWindowContentRegionMin( ) );
  XMStoreFloat4x4( &view_to_clip, crude_camera_view_to_clip( camera ) );
  XMStoreFloat4x4( &selected_node_to_parent, crude_transform_node_to_parent( selected_node_transform ) );
  
  world_to_view = XMMatrixInverse( NULL, crude_transform_node_to_world( camera_node, camera_transform ) );

  selected_node_parent = crude_entity_get_parent( paprika->selected_node );
  if ( crude_entity_valid( selected_node_parent ) )
  {
    XMMATRIX seleted_node_parent_to_world = crude_transform_node_to_world( selected_node_parent, NULL );
    XMStoreFloat4x4( &selected_parent_to_view, XMMatrixMultiply( seleted_node_parent_to_world, world_to_view ) );
  }
  else
  {
    XMStoreFloat4x4( &selected_parent_to_view, world_to_view );
  }

  ImGuizmo::SetID( 0 );
  ImGuizmo::SetRect( ImGui::GetWindowPos( ).x, ImGui::GetWindowPos( ).y, ImGui::GetWindowWidth( ), ImGui::GetWindowHeight( ) );
  ImGuizmo::Manipulate( &selected_parent_to_view._11, &view_to_clip._11, paprika->gizmo_operation, paprika->gizmo_mode, &selected_node_to_parent._11, NULL, NULL );
  
  XMMatrixDecompose( &new_scale, &new_rotation, &new_translation, XMLoadFloat4x4( &selected_node_to_parent ) );
  XMStoreFloat3( &selected_node_transform->translation, new_translation );
  XMStoreFloat3( &selected_node_transform->scale, new_scale );
  XMStoreFloat4( &selected_node_transform->rotation, new_rotation );
}