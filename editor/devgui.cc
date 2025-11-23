#include <nfd.h>
#include <imgui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>

#include <engine/core/hash_map.h>
#include <engine/graphics/gpu_resources_loader.h>
#include <engine/scene/scripts_components.h>
#include <engine/physics/physics_components.h>
#include <engine/physics/physics.h>
#include <engine/external/game_components.h>
#include <editor/editor.h>

#include <editor/devgui.h>

ImGuiWindowFlags                                           window_flags_;


nfdu8filteritem_t                                          scene_file_filters_[ ] = 
{ 
  CRUDE_COMPOUNT( nfdu8filteritem_t, { "Crude Node", "crude_node" } )
};

void
crude_devgui_initialize
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_editor                                             *editor
)
{
  window_flags_ = 0;//ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground;
  devgui->menubar_enabled = true;
  devgui->editor = editor;
  devgui->last_focused_menutab_name = "Graphics";
  devgui->added_node_data = CRUDE_COMPOUNT_EMPTY( crude_devgui_added_node_data );
  devgui->node_to_add = CRUDE_COMPOUNT_EMPTY( crude_entity );
  devgui->node_to_remove = CRUDE_COMPOUNT_EMPTY( crude_entity );

  crude_devgui_nodes_tree_initialize( &devgui->dev_nodes_tree, devgui );
  crude_devgui_node_inspector_initialize( &devgui->dev_node_inspector );
  crude_devgui_viewport_initialize( &devgui->dev_viewport, editor->scene_renderer.render_graph->builder->gpu );
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
  _In_ crude_entity                                        main_scene_node,
  _In_ crude_entity                                        camera_node
)
{
  crude_editor *editor = crude_editor_instance( );
  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, devgui->editor->imgui_context ) );

  if ( devgui->menubar_enabled && ImGui::BeginMainMenuBar( ) )
  {
    if ( !ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_Tab, false ) )
    {
      ImGui::OpenPopup( devgui->last_focused_menutab_name );
    }

    if ( ImGui::BeginMenu( "Scene" ) )
    {
      devgui->last_focused_menutab_name = "Scene";
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
          crude_editor_push_reload_scene_command( crude_editor_instance( ), out_path );
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
          crude_node_manager_save_node_to_file( &editor->node_manager, editor->main_node, out_path );
          NFD_FreePathU8( out_path );
        }
      }
      if ( ImGui::MenuItem( "Node Tree", "Ctrl+S+T" ) )
      {
        devgui->dev_nodes_tree.enabled = !devgui->dev_nodes_tree.enabled;
      }
      if ( ImGui::MenuItem( "Node Inpsector", "Ctrl+S+I" ) )
      {
        devgui->dev_node_inspector.enabled = !devgui->dev_node_inspector.enabled;
      }
      ImGui::EndMenu( );
    }
    
    if ( ImGui::BeginMenu( "View" ) )
    {
      devgui->last_focused_menutab_name = "View";
      if ( ImGui::MenuItem( "Show All Collision" ) )
      {
        editor->scene_renderer.options.hide_collision = false;
      }
      if ( ImGui::MenuItem( "Hide All Collision" ) )
      {
        editor->scene_renderer.options.hide_collision = true;
      }
      ImGui::EndMenu( );
    }
    ImGui::EndMainMenuBar( );
  }

  crude_devgui_nodes_tree_draw( &devgui->dev_nodes_tree, main_scene_node );
  crude_devgui_node_inspector_draw( &devgui->dev_node_inspector, devgui->dev_nodes_tree.selected_node );
  crude_devgui_viewport_draw( &devgui->dev_viewport, camera_node, devgui->dev_nodes_tree.selected_node );
  
  if ( crude_entity_valid( devgui->node_to_add ) )
  {
    ImGui::Begin( "Create New Node" );
    ImGui::InputText( "Name", devgui->added_node_data.buffer, sizeof( devgui->added_node_data.buffer ) );
    if ( ImGui::Button( "Node3d" ) )
    {
      crude_entity new_node = crude_entity_create_empty( camera_node.world, devgui->added_node_data.buffer );
      CRUDE_ENTITY_SET_COMPONENT( new_node, crude_transform, { crude_transform_empty( ) } );
      crude_entity_set_parent( new_node, devgui->node_to_add );
      devgui->node_to_add = CRUDE_COMPOUNT_EMPTY( crude_entity );
    }
    if ( ImGui::Button( "Physics Static Collision Box" ) )
    {
      crude_entity new_node = crude_entity_create_empty( camera_node.world, devgui->added_node_data.buffer );
      CRUDE_ENTITY_SET_COMPONENT( new_node, crude_transform, { crude_transform_empty( ) } );
      CRUDE_ENTITY_SET_COMPONENT( new_node, crude_physics_collision_shape, {
        .type = CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX,
        .box = { .half_extent = { 1, 1, 1 } }
      } );
      
      devgui->node_to_add = CRUDE_COMPOUNT_EMPTY( crude_entity );
    }
    ImGui::End( );
  }
  if ( crude_entity_valid( devgui->node_to_remove ) )
  {
    crude_entity_destroy_hierarchy( devgui->node_to_remove );
    devgui->node_to_remove = CRUDE_COMPOUNT_EMPTY( crude_entity );
  }

  //ImGui::ShowDemoWindow( );
}

void
crude_devgui_handle_input
(
  _In_ crude_devgui                                       *devgui,
  _In_ crude_input                                        *input
)
{
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
  _In_ crude_devgui_nodes_tree                            *devgui_nodes_tree,
  _In_ crude_devgui                                       *devgui
)
{
  devgui_nodes_tree->enabled = true;
  devgui_nodes_tree->selected_node = CRUDE_COMPOUNT_EMPTY( crude_entity );
  devgui_nodes_tree->devgui = devgui;
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
  ImGuiTreeNodeFlags                                       tree_node_flags;
  bool                                                     can_open_children_nodes, tree_node_opened;
  
  ImGui::Begin( "Scene Node Tree", NULL, window_flags_ );
  
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

  if ( devgui_nodes_tree->selected_node.handle == node.handle )
  {
    tree_node_flags |= ImGuiTreeNodeFlags_Selected;
  }

  tree_node_opened = ImGui::TreeNodeEx( ( void* )( intptr_t )*current_node_index, tree_node_flags, crude_entity_get_name( node ) );
  if ( ImGui::IsItemClicked( ) && !ImGui::IsItemToggledOpen( ) )
  {
    devgui_nodes_tree->selected_node = node;
  }

  if ( ImGui::IsItemClicked( 1 ) && !ImGui::IsItemToggledOpen( ) )
  {
    ImGui::OpenPopup( crude_entity_get_name( node ) );
  }

  if (ImGui::BeginPopup( crude_entity_get_name( node ) ) )
  {
    // Add component
    if ( ImGui::Button( "Add Child Note" ) )
    {
      devgui_nodes_tree->devgui->node_to_add = node;
    }
    if ( ImGui::Button( "Remove Note" ) )
    {
      devgui_nodes_tree->devgui->node_to_remove = node;
    }
    ImGui::EndPopup( );
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
          crude_devgui_nodes_tree_draw_internal_( devgui_nodes_tree, child, current_node_index );
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
  _In_ crude_entity                                        node
)
{
  crude_editor *editor = crude_editor_instance( );
  if ( !devgui_inspector->enabled )
  {
    return;
  }

  if ( !crude_entity_valid( node ) )
  {
    return;
  }

  ImGui::Begin( "Node Inspector", NULL, window_flags_ );
  ImGui::Text( "Node: \"%s\"", crude_entity_get_name( node ) );

  crude_node_external *node_external = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_node_external );
  if ( node_external )
  {
    ImGui::Text( "External \"%s\"", node_external->path );
  }
  
  crude_transform *transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_transform );
  if ( transform && ImGui::CollapsingHeader( "crude_transform" ) )
  {
    bool transform_edited = false;
    transform_edited |= ImGui::DragFloat3( "Translation", &transform->translation.x, .1f );
    transform_edited |= ImGui::DragFloat3( "Scale", &transform->scale.x, .1f );
    transform_edited |= ImGui::DragFloat4( "Rotation", &transform->rotation.x, .1f );
    if ( transform_edited )
    {
      CRUDE_ENTITY_COMPONENT_MODIFIED( node, crude_transform );
    }
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
  
  crude_gltf *gltf = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_gltf );
  if ( gltf && ImGui::CollapsingHeader( "crude_gltf" ) )
  {
    ImGui::Text( "TODO" );
  }
  
  crude_level_01 *level_01 = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_level_01 );
  if ( level_01 && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_level_01 ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_level_01 )( node, level_01, &editor->node_manager );
  }
  
  crude_physics_character_body_handle *dynamic_body = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_physics_character_body_handle );
  if ( dynamic_body && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_physics_character_body_handle ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_physics_character_body_handle )( node, dynamic_body, &editor->node_manager );
  }
  
  crude_physics_static_body_handle *static_body = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_physics_static_body_handle );
  if ( static_body && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_physics_static_body_handle ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_physics_static_body_handle )( node, static_body, &editor->node_manager );
  }
  
  crude_physics_collision_shape *collision_shape = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_physics_collision_shape );
  if ( collision_shape && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_physics_collision_shape ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_physics_collision_shape )( node, collision_shape, &editor->node_manager );
  }
  
  crude_player_controller *player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_player_controller );
  if ( player_controller && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_player_controller ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_player_controller )( node, player_controller, &editor->node_manager );
  }
  
  crude_enemy *enemy = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_enemy );
  if ( enemy && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_enemy ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_enemy )( node, enemy, &editor->node_manager );
  }
  crude_debug_collision *debug_collision = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_debug_collision );
  if ( debug_collision && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( crude_debug_collision ) ) )
  {
    CRUDE_PARSE_COMPONENT_TO_IMGUI( crude_debug_collision )( node, debug_collision, &editor->node_manager );
  }
  ImGui::End( );
}

/******************************
 * Dev Gui Viewport
 *******************************/
static void
crude_devgui_viewport_draw_viewport_texture
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_entity                                        camera_node,
  _In_ crude_entity                                        selected_node
);

static void
crude_devgui_viewport_draw_viewport_imguizmo
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_entity                                        camera_node,
  _In_ crude_entity                                        selected_node
);

void
crude_devgui_viewport_initialize
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_gfx_device                                   *gpu
)
{
  crude_editor *editor = crude_editor_instance( );
  devgui_viewport->gpu = gpu;
  devgui_viewport->selected_texture = crude_gfx_render_graph_builder_access_resource_by_name( editor->scene_renderer.render_graph->builder, "final" )->resource_info.texture.handle;
}

void
crude_devgui_viewport_draw
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_entity                                        camera_node,
  _In_ crude_entity                                        selected_node
)
{
  ImGui::Begin( "Viewport", NULL, window_flags_ );
  crude_devgui_viewport_draw_viewport_texture( devgui_viewport, camera_node, selected_node );
  crude_devgui_viewport_draw_viewport_imguizmo( devgui_viewport, camera_node, selected_node );
  ImGui::End();
}

void
crude_devgui_viewport_draw_viewport_texture
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_entity                                        camera_node,
  _In_ crude_entity                                        selected_node
)
{
  char const                                              *preview_texture_name;
  uint32                                                   id;
  

  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( devgui_viewport->selected_texture ) )
  {
    ImGui::Image( CRUDE_CAST( ImTextureRef, &devgui_viewport->selected_texture.index ), ImGui::GetContentRegionAvail( ) );
  }

  ImGui::SetCursorPos( ImGui::GetWindowContentRegionMin( ) );
  
  preview_texture_name = "Unknown";
  id = 0;

  if ( CRUDE_RESOURCE_HANDLE_IS_VALID( devgui_viewport->selected_texture ) )
  {
    crude_gfx_texture *selected_texture = crude_gfx_access_texture( devgui_viewport->gpu, devgui_viewport->selected_texture );
    if ( selected_texture && selected_texture->name )
    {
      preview_texture_name = selected_texture->name;
    };
  }

  if ( ImGui::BeginCombo( "Texture ID", preview_texture_name ) )
  {
    for ( uint32 t = 0; t < devgui_viewport->gpu->textures.pool_size; ++t )
    {
      crude_gfx_texture                                   *texture;
      crude_gfx_texture_handle                             texture_handle;
      bool                                                 is_selected;

      texture_handle = CRUDE_CAST( crude_gfx_texture_handle, t );
      if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( texture_handle ) )
      {
        continue;
      }
      
      texture = crude_gfx_access_texture( devgui_viewport->gpu, texture_handle );
      if ( !texture || !texture->name )
      {
        continue;
      }
      
      ImGui::PushID( id++ );

      is_selected = ( devgui_viewport->selected_texture.index == texture_handle.index );
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
}

void
crude_devgui_viewport_draw_viewport_imguizmo
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
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
  
  if ( !crude_entity_valid( selected_node ) )
  {
    return;
  }

  selected_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( selected_node, crude_transform );
  camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( camera_node, crude_camera  );
  camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( camera_node, crude_transform  );
  selected_node_parent = crude_entity_get_parent( selected_node );
  
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
  
  if ( crude_entity_valid( selected_node_parent ) )
  {
    selected_node_parent_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( selected_node_parent, crude_transform  );
  }
  else
  {
    selected_node_parent_transform = NULL;
  }

  if ( selected_node_parent_transform )
  {
    XMStoreFloat4x4( &selected_parent_to_camera_view, DirectX::XMMatrixMultiply( crude_transform_node_to_world( selected_node_parent, selected_node_parent_transform ), XMMatrixInverse( NULL, crude_transform_node_to_world( camera_node, camera_transform ) ) ) );
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

    CRUDE_ENTITY_COMPONENT_MODIFIED( selected_node, crude_transform );
  }

  //ImGuizmo::DrawGrid(cameraView, cameraProjection, identityMatrix, 100.f);
}

void
crude_devgui_viewport_input
(
  _In_ crude_devgui_viewport                              *devgui_viewport,
  _In_ crude_input                                        *input
)
{
}