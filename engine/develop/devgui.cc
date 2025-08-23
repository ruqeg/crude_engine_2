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
  _In_ crude_gfx_renderer                                 *renderer
)
{
  devgui->should_reload_shaders = false;
  devgui->menubar_enabled = false;
  devgui->renderer = renderer;
  devgui->render_graph = render_graph;
  crude_stack_allocator_initialize( &devgui->temporary_allocator, CRUDE_RMEGA( 32 ), "devgui_temp_allocator" );
  crude_devgui_nodes_tree_initialize( &devgui->dev_nodes_tree );
  crude_devgui_node_inspector_initialize( &devgui->dev_node_inspector );
  crude_devgui_viewport_initialize( &devgui->dev_viewport, render_graph->builder->gpu );
  crude_devgui_render_graph_initialize( &devgui->dev_render_graph, render_graph );
}

void
crude_devgui_deinitialize
(
  _In_ crude_devgui                                       *devgui
)
{
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
crude_devgui_post_graphics_update
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
    ImGui::End( );
  }
}