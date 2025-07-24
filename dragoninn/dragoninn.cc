#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>

#include <platform/platform_system.h>
#include <platform/platform_components.h>

#include <dragoninn.h>

CRUDE_ECS_SYSTEM_DECLARE( dragoninn_update_system_ );

static void
dragoninn_update_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
dragoninn_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
);

void
crude_dragoninn_initialize
(
  _In_ crude_dragoninn                                    *dragoninn,
  _In_ crude_engine                                       *engine
)
{
  dragoninn->working = true;
  dragoninn->engine = engine;
  
  ImGuiStyle                                              *imgui_style;
  ImGuiIO                                                 *imgui_io;
  SDL_Window                                              *sdl_window;
  char                                                     temporary_buffer[ 1024 ];
  float32                                                  display_content_scale;

  ECS_IMPORT( dragoninn->engine->world, crude_platform_system );

  if ( !SDL_Init( SDL_INIT_VIDEO) )
  {
    return;
  }
  
  display_content_scale = SDL_GetDisplayContentScale( SDL_GetPrimaryDisplay() );
  
  IMGUI_CHECKVERSION();
  dragoninn->imgui_context = ImGui::CreateContext();

  dragoninn->platform_node = crude_entity_create_empty( dragoninn->engine->world, "crude_dragoninn" );
  CRUDE_ENTITY_SET_COMPONENT( dragoninn->platform_node, crude_window, { 
    .width     = CRUDE_CAST( int32, 1200 * display_content_scale ),
    .height    = CRUDE_CAST( int32, 600 * display_content_scale ),
    .maximized = false,
    .flags     = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY
  });
  CRUDE_ENTITY_SET_COMPONENT( dragoninn->platform_node, crude_input, { 
    .callback = dragoninn_input_callback_,
    .ctx = dragoninn->imgui_context
  } );

  sdl_window = ( SDL_Window* ) CRUDE_ENTITY_GET_MUTABLE_COMPONENT( dragoninn->platform_node, crude_window_handle )->value;

  dragoninn->sdl_renderer = SDL_CreateRenderer( sdl_window, nullptr );
  SDL_SetRenderVSync( dragoninn->sdl_renderer, 1 );
  SDL_ShowWindow( sdl_window );

  ImGui::SetCurrentContext( dragoninn->imgui_context );
  imgui_io = &ImGui::GetIO();
  imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NoMouseCursorChange;
  imgui_io->ConfigWindowsResizeFromEdges = true;
  imgui_io->ConfigWindowsMoveFromTitleBarOnly = true;
  ImGui::StyleColorsDark();

  imgui_style = &ImGui::GetStyle();
  imgui_style->ScaleAllSizes( display_content_scale );

  ImGui_ImplSDL3_InitForSDLRenderer( sdl_window, dragoninn->sdl_renderer );
  ImGui_ImplSDLRenderer3_Init( dragoninn->sdl_renderer );
  
  ax::NodeEditor::Config config;
  config.SettingsFile = "Simple.json";
  dragoninn->node_editor_context = ax::NodeEditor::CreateEditor( &config );
  
  crude_heap_allocator_initialize( &dragoninn->common_allocator, CRUDE_RMEGA( 32 ), "common_allocator" );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( dragoninn->render_graph_nodes, 0u, crude_heap_allocator_pack( &dragoninn->common_allocator ) );

  crude_render_graph_resource resource;
  resource.type = CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT;
  resource.external = false;
  crude_string_copy( resource.name, "resource 1", CRUDE_COUNTOF( resource.name ) );

  crude_render_graph_node render_graph_node;
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( render_graph_node.inputs, 0u, crude_heap_allocator_pack( &dragoninn->common_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( render_graph_node.outputs, 0u, crude_heap_allocator_pack( &dragoninn->common_allocator ) );
  CRUDE_ARRAY_PUSH( render_graph_node.inputs, resource );
  CRUDE_ARRAY_PUSH( render_graph_node.outputs, resource );
  render_graph_node.enabled = true;
  crude_string_copy( render_graph_node.name, "pass 1", CRUDE_COUNTOF( resource.name ) );
  render_graph_node.type = CRUDE_GFX_RENDER_GRAPH_NODE_TYPE_GRAPHICS;
  CRUDE_ARRAY_PUSH( dragoninn->render_graph_nodes, render_graph_node );

  CRUDE_ECS_SYSTEM_DEFINE( dragoninn->engine->world, dragoninn_update_system_, EcsOnUpdate, dragoninn, {
    { .id = ecs_id( crude_input ) },
    { .id = ecs_id( crude_window_handle ) },
  } );
}

void
crude_dragoninn_deinitialize
(
  _In_ crude_dragoninn                                    *dragoninn
)
{
  if ( !dragoninn->working )
  {
    return;
  }
  
  axn::DestroyEditor( dragoninn->node_editor_context );

  dragoninn->working = false;
}

void
crude_dragoninn_update
(
  _In_ crude_dragoninn                                    *dragoninn
)
{
  crude_input const *input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( dragoninn->platform_node, crude_input );
  if ( input && input->should_close_window )
  {
    crude_dragoninn_deinitialize( dragoninn );
  }
}

static const char*
to_type
(
  crude_gfx_render_graph_resource_type type
)
{
  switch ( type )
  {
  case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT: return "attachment";
  case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_BUFFER: return "buffer";
  case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE: return "texture";
  }
  return "unknown";
}

void
dragoninn_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_dragoninn *dragoninn = ( crude_dragoninn* )it->ctx;
  crude_input *input_per_entity = ecs_field( it, crude_input, 0 );
  crude_window_handle *window_handle_per_entity = ecs_field( it, crude_window_handle, 1 );
  
  ImVec4 clear_color = ImVec4( 14 / 255.f, 6 / 255.f, 19 / 255.f, 1.00f );

  ImGui::SetCurrentContext( dragoninn->imgui_context );

  ImGuiIO *imgui_io = &ImGui::GetIO();

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  auto& io = ImGui::GetIO();
  
  axn::SetCurrentEditor( dragoninn->node_editor_context );
  
  axn::Begin("My Editor", ImVec2(0.0, 0.0f));
  
  int uniqueId = 1;
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( dragoninn->render_graph_nodes ); ++i )
  {
    crude_render_graph_node *graph_node = &dragoninn->render_graph_nodes[ i ];
    axn::BeginNode(uniqueId++);
    
    ImGui::PushItemWidth( 300 );
    ImGui::InputText( "name", graph_node->name, CRUDE_COUNTOF( graph_node->name ) );
    ImGui::PopItemWidth();
    ImGui::Checkbox( "enabled", &graph_node->enabled ); ImGui::SameLine();
    int type = graph_node->type;
    ImGui::RadioButton( "graphics", &type, 0 ); ImGui::SameLine();
    ImGui::RadioButton( "compute", &type, 1 );
    graph_node->type = CRUDE_CAST( crude_gfx_render_graph_node_type, type );
    
    ImGui::Separator( );

    for ( uint32 i = 0; i < crude_max( CRUDE_ARRAY_LENGTH( graph_node->inputs ), CRUDE_ARRAY_LENGTH( graph_node->outputs ) ); ++i )
    {
      if ( i < CRUDE_ARRAY_LENGTH( graph_node->inputs ) )
      {
        axn::BeginPin( uniqueId++, axn::PinKind::Input );

        ImGui::PushID( uniqueId++ );
        ImGui::Text( "-> [in]" );
        ImGui::PushItemWidth( 300 );
        ImGui::Text( "name %s", graph_node->inputs[ i ].name );
        ImGui::PopItemWidth();
        ImGui::Text( graph_node->inputs[ i ].external ? "external" : "not external" ); ImGui::SameLine();
        switch ( graph_node->inputs[ i ].type )
        {
        case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT:
        {
          ImGui::Text( "attachment" );
          break;
        }
        case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_BUFFER:
        {
          ImGui::Text( "buffer" );
          break;
        }
        case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE:
        {
          ImGui::Text( "texture" );
          break;
        }
        }

        ImGui::PopID( );

        axn::EndPin( );
      }
      ImGui::SameLine();
      ImGui::Dummy( ImVec2( 10, 0 ) ); /* Hacky magic number to space out the output pin. */
      ImGui::SameLine();
      if ( i < CRUDE_ARRAY_LENGTH( graph_node->outputs ) )
      {
        axn::BeginPin( uniqueId++, axn::PinKind::Output );

        ImGui::PushID( uniqueId++ );

        ImGui::Text( "[out] ->" );
        ImGui::PushItemWidth( 300 );
        ImGui::InputText( "name", graph_node->outputs[ i ].name, CRUDE_COUNTOF( graph_node->outputs[ i ].name ) );
        ImGui::PopItemWidth();
        ImGui::Checkbox( "external", &graph_node->outputs[ i ].external ); ImGui::SameLine();
        type = graph_node->outputs[ i ].type;
        ImGui::RadioButton( "buffer", &type, 0 ); ImGui::SameLine();
        ImGui::RadioButton( "texture", &type, 1 ); ImGui::SameLine();
        ImGui::RadioButton( "attachment", &type, 2 );
        graph_node->outputs[ i ].type = CRUDE_CAST( crude_gfx_render_graph_resource_type, type );

        ImGui::PopID( );

        axn::EndPin();
      }
    }
    
    axn::EndNode();
  }
            // ==================================================================================================
            // Link Drawing Section

            for (auto& linkInfo : dragoninn->m_Links)
                axn::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);

            // ==================================================================================================
            // Interaction Handling Section
            // This was coppied from BasicInteration.cpp. See that file for commented code.
            static int                  m_NextLinkId = 100; 
            // Handle creation action ---------------------------------------------------------------------------
            if (axn::BeginCreate())
            {
                axn::PinId inputPinId, outputPinId;
                if (axn::QueryNewLink(&inputPinId, &outputPinId))
                {
                    if (inputPinId && outputPinId)
                    {
                        if (axn::AcceptNewItem())
                        {
                            dragoninn->m_Links.push_back({ axn::LinkId(m_NextLinkId++), inputPinId, outputPinId });
                            axn::Link(dragoninn->m_Links.back().Id, dragoninn->m_Links.back().InputId, dragoninn->m_Links.back().OutputId);
                        }
                    }
                }
            }
            axn::EndCreate();

            // Handle deletion action ---------------------------------------------------------------------------
            if (axn::BeginDelete())
            {
                axn::LinkId deletedLinkId;
                while (axn::QueryDeletedLink(&deletedLinkId))
                {
                    if (axn::AcceptDeletedItem())
                    {
                        for (auto& link : dragoninn->m_Links)
                        {
                            if (link.Id == deletedLinkId)
                            {
                                dragoninn->m_Links.erase(&link);
                                break;
                            }
                        }
                    }
                }
            }
            axn::EndDelete();

  axn::End();
  axn::SetCurrentEditor(nullptr);

  ImGui::Render();
  SDL_SetRenderScale( dragoninn->sdl_renderer, imgui_io->DisplayFramebufferScale.x, imgui_io->DisplayFramebufferScale.y );
  SDL_SetRenderDrawColorFloat( dragoninn->sdl_renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w );
  SDL_RenderClear( dragoninn->sdl_renderer );
  ImGui_ImplSDLRenderer3_RenderDrawData( ImGui::GetDrawData(), dragoninn->sdl_renderer );
  SDL_RenderPresent( dragoninn->sdl_renderer );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input *input = &input_per_entity[ i ];
    crude_window_handle *window_handle = &window_handle_per_entity[ i ];
  }
}

void
dragoninn_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
)
{
  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, ctx ) );
  ImGui_ImplSDL3_ProcessEvent( CRUDE_CAST( SDL_Event*, sdl_event ) );
}