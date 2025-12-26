#include <nfd.h>

#include <engine/core/hash_map.h>
#include <engine/graphics/gpu_resources_loader.h>
#include <engine/physics/physics_ecs.h>
#include <engine/physics/physics.h>
#include <engine/external/game_ecs.h>
#include <engine/scene/scene_debug_ecs.h>
#include <engine/engine/engine_commands_manager.h>
#include <editor/editor.h>

#include <editor/devgui.h>

nfdu8filteritem_t                                          scene_file_filters_[ ] = 
{ 
  CRUDE_COMPOUNT( nfdu8filteritem_t, { "Crude Node", "crude_node" } )
};

void
crude_devgui_initialize
(
  _In_ crude_devgui                                       *devgui
)
{
  devgui->menubar_enabled = true;

  crude_devgui_nodes_tree_initialize( &devgui->dev_nodes_tree );
  crude_devgui_node_inspector_initialize( &devgui->dev_node_inspector );
  crude_devgui_viewport_initialize( &devgui->dev_viewport );
  crude_devgui_editor_camera_initialize( &devgui->dev_editor_camera );
}

void
crude_devgui_deinitialize
(
  _In_ crude_devgui                                       *devgui
)
{
}

void
crude_devgui_draw
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_ecs                                          *world
)
{
  crude_editor *editor = crude_editor_instance( );
  
  ImGui::SetCurrentContext( editor->engine->imgui_context );

  if ( devgui->menubar_enabled && ImGui::BeginMainMenuBar( ) )
  {
    if ( ImGui::BeginMenu( "Scene" ) )
    {
      if ( ImGui::MenuItem( "Open Scene" ) )
      {
        nfdu8char_t                                       *out_path;
        nfdopendialogu8args_t                              args;
        nfdresult_t                                        result;

        args = CRUDE_COMPOUNT_EMPTY( nfdopendialogu8args_t );
        args.filterList = scene_file_filters_;
        args.filterCount = CRUDE_COUNTOF( scene_file_filters_ );
        
        result = NFD_OpenDialogU8_With( &out_path, &args );
        if ( result == NFD_OKAY )
        {
          crude_engine_commands_manager_push_reload_scene_command( &editor->engine->commands_manager );
        }
      }
      if ( ImGui::MenuItem( "Save Scene" ) )
      {
        nfdu8char_t                                       *out_path;
        nfdsavedialogu8args_t                              args;
        nfdresult_t                                        result;

        args = CRUDE_COMPOUNT_EMPTY( nfdsavedialogu8args_t );
        args.filterList = scene_file_filters_;
        args.filterCount = CRUDE_COUNTOF( scene_file_filters_ );
        
        result = NFD_SaveDialogU8_With( &out_path, &args );
        if ( result == NFD_OKAY )
        {
          crude_node_manager_save_node_to_file( &editor->engine->node_manager, world, editor->main_node, out_path );
          NFD_FreePathU8( out_path );
        }
      }
      ImGui::EndMenu( );
    }
    if ( ImGui::BeginMenu( "Layout" ) )
    {
      if ( ImGui::MenuItem( "Node Tree" ) )
      {
        devgui->dev_nodes_tree.enabled = !devgui->dev_nodes_tree.enabled;
      }
      if ( ImGui::MenuItem( "Node Inpsector" ) )
      {
        devgui->dev_node_inspector.enabled = !devgui->dev_node_inspector.enabled;
      }
      if ( ImGui::MenuItem( "Editor Camera Node Inpsector" ) )
      {
        devgui->dev_editor_camera.enabled = !devgui->dev_editor_camera.enabled;
      }
      ImGui::EndMenu( );
    }
    
    if ( ImGui::BeginMenu( "View" ) )
    {
      if ( ImGui::MenuItem( "Show All Collision" ) )
      {
        //editor->engine->scene_renderer.options.hide_collision = false;
      }
      if ( ImGui::MenuItem( "Hide All Collision" ) )
      {
        //editor->engine->scene_renderer.options.hide_collision = true;
      }
      if ( ImGui::MenuItem( "Show All Debug GLTF" ) )
      {
        //editor->engine->scene_renderer.options.hide_debug_gltf = false;
      }
      if ( ImGui::MenuItem( "Hide All Debug GLTF" ) )
      {
        //editor->engine->scene_renderer.options.hide_debug_gltf = true;
      }
      ImGui::EndMenu( );
    }
    ImGui::EndMainMenuBar( );
  }

  crude_devgui_nodes_tree_draw( &devgui->dev_nodes_tree, world );
  crude_devgui_node_inspector_draw( &devgui->dev_node_inspector, world );
  crude_devgui_viewport_draw( &devgui->dev_viewport, world );
  crude_devgui_editor_camera_draw( &devgui->dev_editor_camera, world );
  
  if ( crude_entity_valid( world, editor->node_to_add ) )
  {
    ImGui::Begin( "Create New Node" );
    ImGui::InputText( "Name", editor->added_node_data.buffer, sizeof( editor->added_node_data.buffer ) );
    if ( ImGui::Button( "Node3d" ) )
    {
      crude_entity new_node = crude_entity_create_empty( world, editor->added_node_data.buffer );
      CRUDE_ENTITY_SET_COMPONENT( world, new_node, crude_transform, { crude_transform_empty( ) } );
      crude_entity_set_parent( world, new_node, editor->node_to_add );
      editor->node_to_add = CRUDE_COMPOUNT_EMPTY( crude_entity );
    }
    if ( ImGui::Button( "Physics Static Collision Box" ) )
    {
      crude_entity new_node = crude_entity_create_empty( world, editor->added_node_data.buffer );
      CRUDE_ENTITY_SET_COMPONENT( world, new_node, crude_transform, { crude_transform_empty( ) } );
      CRUDE_ENTITY_SET_COMPONENT( world, new_node, crude_physics_collision_shape, {
        .type = CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX,
        .box = { .half_extent = { 1, 1, 1 } }
      } );
      
      editor->node_to_add = CRUDE_COMPOUNT_EMPTY( crude_entity );
    }
    ImGui::End( );
  }
  
  if ( crude_entity_valid( world, editor->node_to_dublicate ) )
  {
    ImGui::Begin( "Dublicate Node" );
    ImGui::InputText( "Name", editor->added_node_data.buffer, sizeof( editor->added_node_data.buffer ) );
    if ( ImGui::Button( "Dublicate" ) )
    {
      crude_entity new_node =  crude_entity_copy_hierarchy( world, editor->node_to_dublicate, editor->added_node_data.buffer, true, true );
      editor->node_to_dublicate = CRUDE_COMPOUNT_EMPTY( crude_entity );
    }
    ImGui::End( );
  }
  
  if ( crude_entity_valid( world, editor->node_to_remove ) )
  {
    crude_entity_destroy_hierarchy( world, editor->node_to_remove );
    editor->node_to_remove = CRUDE_COMPOUNT_EMPTY( crude_entity );
  }

  //ImGui::ShowDemoWindow( );
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
}

/******************************
 * Dev Gui Nodes Tree
 *******************************/
static void
crude_devgui_nodes_tree_draw_internal_
(
  _In_ crude_devgui_nodes_tree                            *devgui_nodes_tree,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _In_ uint32                                             *current_node_index
);

void
crude_devgui_nodes_tree_initialize
(
  _In_ crude_devgui_nodes_tree                            *devgui_nodes_tree
)
{
  devgui_nodes_tree->enabled = true;
}

void
crude_devgui_nodes_tree_draw
(
  _In_ crude_devgui_nodes_tree                            *devgui_nodes_tree,
  _In_ crude_ecs                                          *world
)
{
  crude_editor *editor = crude_editor_instance( );

  if ( !devgui_nodes_tree->enabled )
  {
    return;
  }

  uint32 current_node_index = 0u;
  crude_devgui_nodes_tree_draw_internal_( devgui_nodes_tree, world, editor->main_node, &current_node_index );
}

void
crude_devgui_nodes_tree_draw_internal_
(
  _In_ crude_devgui_nodes_tree                            *devgui_nodes_tree,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _In_ uint32                                             *current_node_index
)
{
  crude_editor                                            *editor;
  ImGuiTreeNodeFlags                                       tree_node_flags;
  bool                                                     can_open_children_nodes, tree_node_opened;
  
  editor = crude_editor_instance( );

  ImGui::Begin( "Scene Node Tree" );
  
  {
    can_open_children_nodes = false;

    ecs_iter_t it = crude_ecs_children( world, node );
    if ( !CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_gltf ) && !CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_node_external ) && ecs_children_next( &it ) )
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

  if ( editor->selected_node == node )
  {
    tree_node_flags |= ImGuiTreeNodeFlags_Selected;
  }

  tree_node_opened = ImGui::TreeNodeEx( ( void* )( intptr_t )*current_node_index, tree_node_flags, crude_entity_get_name( world, node ) );
  if ( ImGui::IsItemClicked( ) && !ImGui::IsItemToggledOpen( ) )
  {
    editor->selected_node = node;
  }

  if ( ImGui::IsItemClicked( 1 ) && !ImGui::IsItemToggledOpen( ) )
  {
    ImGui::OpenPopup( crude_entity_get_name( world, node ) );
  }

  if (ImGui::BeginPopup( crude_entity_get_name( world, node ) ) )
  {
    // Add component
    if ( ImGui::Button( "Add Child Note" ) )
    {
      editor->node_to_add = node;
    }
    if ( ImGui::Button( "Dublicate Note" ) )
    {
      editor->node_to_dublicate = node;
    }
    if ( ImGui::Button( "Remove Note" ) )
    {
      editor->node_to_remove = node;
    }
    ImGui::EndPopup( );
  }

  if (ImGui::BeginDragDropSource( ) )
  {
    ImGui::SetDragDropPayload( crude_entity_get_name( world, node ), NULL, 0 );
    ImGui::Text( crude_entity_get_name( world, node ) );
    ImGui::EndDragDropSource( );
  }

  ++( *current_node_index );
  if ( tree_node_opened )
  {
    if ( can_open_children_nodes )
    {
      ecs_iter_t it = crude_ecs_children( world, node );
      while ( ecs_children_next( &it ) )
      {
        for (int i = 0; i < it.count; i ++)
        {
          crude_entity child = crude_entity_from_iterator( &it, i );
          crude_devgui_nodes_tree_draw_internal_( devgui_nodes_tree, world, child, current_node_index );
        }
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
  devgui_inspector->enabled = true;
}

void
crude_devgui_node_inspector_draw
(
  _In_ crude_devgui_node_inspector                        *devgui_inspector,
  _In_ crude_ecs                                          *world
)
{
  crude_editor *editor = crude_editor_instance( );
  
  if ( !devgui_inspector->enabled )
  {
    return;
  }

  if ( !crude_entity_valid( world, editor->selected_node ) )
  {
    return;
  }

  ImGui::Begin( "Node Inspector" );
  ImGui::Text( "Node: \"%s\"", crude_entity_get_name( world, editor->selected_node ) );

  crude_node_external *node_external = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_node_external );
  if ( node_external )
  {
    ImGui::Text( "External \"%s\"", node_external->path );
  }
  
  crude_transform *transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_transform );
  if ( transform && ImGui::CollapsingHeader( "crude_transform" ) )
  {
    bool transform_edited = false;
    transform_edited |= ImGui::DragFloat3( "Translation", &transform->translation.x, .1f );
    transform_edited |= ImGui::DragFloat3( "Scale", &transform->scale.x, .1f );
    transform_edited |= ImGui::DragFloat4( "Rotation", &transform->rotation.x, .1f );
    if ( transform_edited )
    {
      CRUDE_ENTITY_COMPONENT_MODIFIED( world, editor->selected_node, crude_transform );
    }
  }
  
  crude_free_camera *free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_free_camera );
  if ( free_camera && ImGui::CollapsingHeader( "crude_free_camera" ) )
  {
    ImGui::InputFloat( "Moving Speed", &free_camera->moving_speed_multiplier, .1f );
    ImGui::InputFloat( "Rotating Speed", &free_camera->rotating_speed_multiplier, .1f );
  }
  
  crude_camera *camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_camera );
  if ( camera && ImGui::CollapsingHeader( "crude_camera" ) )
  {
    ImGui::InputFloat( "Far Z", &camera->far_z );
    ImGui::InputFloat( "Near Z", &camera->near_z );
    ImGui::SliderAngle( "FOV Radians", &camera->fov_radians );
    ImGui::InputFloat( "Aspect Ratio", &camera->aspect_ratio );
    if ( ImGui::Button( "Set Active" ) )
    {
      crude_scene_thread_manager_set_camera_node_UNSAFE( &editor->engine->___scene_thread_manager, editor->selected_node );
    }
  }
  
  crude_light *light = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_light );
  if ( light && ImGui::CollapsingHeader( "crude_light" ) )
  {
    ImGui::ColorEdit3( "color", &light->color.x );
    ImGui::InputFloat( "intensity", &light->intensity );
    ImGui::InputFloat( "radius", &light->radius );
  }
  
  crude_gltf *gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_gltf );
  if ( gltf && ImGui::CollapsingHeader( "crude_gltf" ) )
  {
    ImGui::Text( "Relative Path \"%s\"", gltf->original_path );
    ImGui::Text( "Absolute Path \"%s\"", gltf->path );
    ImGui::Checkbox( "Hidden", &gltf->hidden );
  }
  
  crude_level_starting_room *level_starting_room = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_level_starting_room );
  if ( level_starting_room && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_level_starting_room ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_level_starting_room )( world, editor->selected_node, level_starting_room, &editor->engine->node_manager );
  }

  crude_level_01 *level_01 = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_level_01 );
  if ( level_01 && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_level_01 ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_level_01 )( world, editor->selected_node, level_01, &editor->engine->node_manager );
  }
  
  crude_physics_character_body_handle *dynamic_body = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_physics_character_body_handle );
  if ( dynamic_body && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_physics_character_body_handle ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_physics_character_body_handle )( world, editor->selected_node, dynamic_body, &editor->engine->node_manager );
  }
  
  crude_physics_static_body_handle *static_body = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_physics_static_body_handle );
  if ( static_body && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_physics_static_body_handle ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_physics_static_body_handle )( world, editor->selected_node, static_body, &editor->engine->node_manager );
  }
  
  crude_physics_collision_shape *collision_shape = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_physics_collision_shape );
  if ( collision_shape && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_physics_collision_shape ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_physics_collision_shape )( world, editor->selected_node, collision_shape, &editor->engine->node_manager );
  }
  
  crude_player_controller *player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_player_controller );
  if ( player_controller && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_player_controller ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_player_controller )( world, editor->selected_node, player_controller, &editor->engine->node_manager );
  }
  
  crude_enemy *enemy = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_enemy );
  if ( enemy && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_enemy ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_enemy )( world, editor->selected_node, enemy, &editor->engine->node_manager );
  }
  
  crude_weapon *weapon = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_weapon );
  if ( weapon && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_weapon ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_weapon )( world, editor->selected_node, weapon, &editor->engine->node_manager );
  }
  
  crude_debug_collision *debug_collision = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_debug_collision );
  if ( debug_collision && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_debug_collision ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_debug_collision )( world, editor->selected_node, debug_collision, &editor->engine->node_manager );
  }

  crude_debug_gltf *debug_gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_debug_gltf );
  if ( debug_gltf && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_debug_gltf ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_debug_gltf )( world, editor->selected_node, debug_gltf, &editor->engine->node_manager );
  }

  crude_node_runtime *runtime_node = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_node_runtime );
  if ( runtime_node && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_node_runtime ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_node_runtime )( world, editor->selected_node, runtime_node, &editor->engine->node_manager );
  }

  crude_recycle_station *recycle_station = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->selected_node, crude_recycle_station );
  if ( recycle_station && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_recycle_station ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_recycle_station )( world, editor->selected_node, recycle_station, &editor->engine->node_manager );
  }
  
  ImGui::End( );
}

/******************************
 * Dev Gui Viewport
 *******************************/
static void
crude_devgui_viewport_draw_viewport_texture_
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_entity                                        camera_node,
  _In_ crude_entity                                        selected_node
);

static void
crude_devgui_viewport_draw_viewport_imguizmo_
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        camera_node,
  _In_ crude_entity                                        selected_node
);

void
crude_devgui_viewport_initialize
(
  _In_ crude_devgui_viewport                              *devgui_viewport
)
{
  crude_editor *editor = crude_editor_instance( );
  //devgui_viewport->selected_texture = crude_gfx_render_graph_builder_access_resource_by_name( editor->engine->scene_renderer.render_graph->builder, "final" )->resource_info.texture.handle;
}

void
crude_devgui_viewport_draw
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_ecs                                          *world
)
{
  crude_editor *editor = crude_editor_instance( );
  ImGui::Begin( "Viewport" );
  
  crude_entity camera_node = crude_scene_thread_manager_get_camera_node_UNSAFE( &editor->engine->___scene_thread_manager );
  ImGuiContext *cc = ImGui::GetCurrentContext( );
  crude_devgui_viewport_draw_viewport_texture_( devgui_viewport, camera_node, editor->selected_node );
  crude_devgui_viewport_draw_viewport_imguizmo_( devgui_viewport, world, camera_node, editor->selected_node );
  ImGui::End();
}

void
crude_devgui_viewport_draw_viewport_texture_
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_entity                                        camera_node,
  _In_ crude_entity                                        selected_node
)
{
  crude_editor                                            *editor;
  char const                                              *preview_texture_name;
  uint32                                                   id;
  
  editor = crude_editor_instance( );

  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( devgui_viewport->selected_texture ) )
  {
    //ImGui::Image( CRUDE_CAST( ImTextureRef, &devgui_viewport->selected_texture.index ), ImGui::GetContentRegionAvail( ) );
  }

  ImGui::SetCursorPos( ImGui::GetWindowContentRegionMin( ) );
  
  preview_texture_name = "Unknown";
  id = 0;

  //if ( CRUDE_RESOURCE_HANDLE_IS_VALID( devgui_viewport->selected_texture ) )
  //{
  //  crude_gfx_texture *selected_texture = crude_gfx_access_texture( &editor->engine->gpu, devgui_viewport->selected_texture );
  //  if ( selected_texture && selected_texture->name )
  //  {
  //    preview_texture_name = selected_texture->name;
  //  };
  //}
  //
  //if ( ImGui::BeginCombo( "Texture ID", preview_texture_name ) )
  //{
  //  for ( uint32 t = 0; t < editor->engine->gpu.textures.pool_size; ++t )
  //  {
  //    crude_gfx_texture                                   *texture;
  //    crude_gfx_texture_handle                             texture_handle;
  //    bool                                                 is_selected;
  //
  //    texture_handle = CRUDE_CAST( crude_gfx_texture_handle, t );
  //    if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( texture_handle ) )
  //    {
  //      continue;
  //    }
  //    
  //    texture = crude_gfx_access_texture( &editor->engine->gpu, texture_handle );
  //    if ( !texture || !texture->name )
  //    {
  //      continue;
  //    }
  //    
  //    ImGui::PushID( id++ );
  //
  //    is_selected = ( devgui_viewport->selected_texture.index == texture_handle.index );
  //    if ( ImGui::Selectable( texture->name ) )
  //    {
  //      devgui_viewport->selected_texture = texture_handle;
  //    }
  //    
  //    ImGui::PopID( );
  //    if ( is_selected )
  //    {
  //      ImGui::SetItemDefaultFocus();
  //    }
  //  }
  //  ImGui::EndCombo();
  //}
}

void
crude_devgui_viewport_draw_viewport_imguizmo_
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        camera_node,
  _In_ crude_entity                                        selected_node
)
{
  static ImGuizmo::OPERATION                               selected_gizmo_operation = ImGuizmo::TRANSLATE;
  static ImGuizmo::MODE                                    selected_gizmo_mode = ImGuizmo::WORLD;

  crude_camera                                            *camera;
  crude_transform                                         *camera_transform;
  crude_transform                                         *selected_node_transform;
  crude_transform                                         *selected_node_parent_transform;
  crude_entity                                             selected_node_parent;
  XMFLOAT4X4                                               camera_view_to_clip, selected_node_to_parent, selected_parent_to_camera_view;
  XMVECTOR                                                 new_scale, new_translation, new_rotation_quat;
  
  if ( !crude_entity_valid( world, selected_node ) )
  {
    return;
  }

  selected_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, selected_node, crude_transform );
  camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, camera_node, crude_camera  );
  camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, camera_node, crude_transform  );
  selected_node_parent = crude_entity_get_parent( world, selected_node );
  
  if ( selected_node_transform == NULL )
  {
    return;
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
}


/******************************
 * 
 * Editor Camera
 * 
 *******************************/
void
crude_devgui_editor_camera_initialize
(
  _In_ crude_devgui_editor_camera                         *dev_editor_camera
)
{
  dev_editor_camera->enabled = false;
}

void
crude_devgui_editor_camera_draw
(
  _In_ crude_devgui_editor_camera                         *dev_editor_camera,
  _In_ crude_ecs                                          *world
)
{
  crude_editor *editor = crude_editor_instance( );

  if ( !dev_editor_camera->enabled )
  {
    return;
  }

  if ( !crude_entity_valid( world, editor->editor_camera_node ) )
  {
    return;
  }

  ImGui::Begin( "Editor Camera Node Inspector" );
  ImGui::Text( "Node: \"%s\"", crude_entity_get_name( world, editor->editor_camera_node ) );

  
  crude_transform *transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->editor_camera_node, crude_transform );
  if ( transform && ImGui::CollapsingHeader( "crude_transform" ) )
  {
    bool transform_edited = false;
    transform_edited |= ImGui::DragFloat3( "Translation", &transform->translation.x, .1f );
    transform_edited |= ImGui::DragFloat3( "Scale", &transform->scale.x, .1f );
    transform_edited |= ImGui::DragFloat4( "Rotation", &transform->rotation.x, .1f );
    if ( transform_edited )
    {
      CRUDE_ENTITY_COMPONENT_MODIFIED( world, editor->editor_camera_node, crude_transform );
    }
  }
  
  crude_free_camera *free_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->editor_camera_node, crude_free_camera );
  if ( free_camera && ImGui::CollapsingHeader( "crude_free_camera" ) )
  {
    ImGui::InputFloat( "Moving Speed", &free_camera->moving_speed_multiplier, .1f );
    ImGui::InputFloat( "Rotating Speed", &free_camera->rotating_speed_multiplier, .1f );
  }
  
  crude_camera *camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, editor->editor_camera_node, crude_camera );
  if ( camera && ImGui::CollapsingHeader( "crude_camera" ) )
  {
    ImGui::InputFloat( "Far Z", &camera->far_z );
    ImGui::InputFloat( "Near Z", &camera->near_z );
    ImGui::SliderAngle( "FOV Radians", &camera->fov_radians );
    ImGui::InputFloat( "Aspect Ratio", &camera->aspect_ratio );
    if ( ImGui::Button( "Set Active" ) )
    {
      crude_scene_thread_manager_set_camera_node_UNSAFE( &editor->engine->___scene_thread_manager, editor->editor_camera_node );
    }
  }

  ImGui::End( );
}