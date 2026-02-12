#include <engine/gui/node_tree.h>

static void
crude_gui_node_tree_queue_draw_internal_
(
  _In_ crude_gui_node_tree                                *node_tree,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _Inout_ crude_entity                                    *selected_node,
  _In_ uint32                                             *current_node_index
);

void
crude_gui_node_tree_initialize
(
  _In_ crude_gui_node_tree                                *node_tree
)
{
  node_tree->node_reference = CRUDE_COMPOUNT_EMPTY( crude_entity );
  node_tree->new_node_name[ 0 ] = 0;
  node_tree->node_popup_should_be_opened = false;
}

void
crude_gui_node_tree_deinitialize
(
  _In_ crude_gui_node_tree                                *node_tree
)
{
}

void
crude_gui_node_tree_update
(
  _In_ crude_gui_node_tree                                *node_tree
)
{
}

void
crude_gui_node_tree_queue_draw
(
  _In_ crude_gui_node_tree                                *node_tree,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _Inout_ crude_entity                                    *selected_node
)
{
  uint32                                                   current_node_index;

  if ( !crude_entity_valid( world, node ) )
  {
    return;
  }
  
  if ( crude_entity_valid( world, node_tree->node_reference ) )
  {
    if ( node_tree->node_popup_should_be_opened && !ImGui::IsPopupOpen( "Node Popup" ) )
    {
      ImGui::OpenPopup( "Node Popup", ImGuiPopupFlags_NoOpenOverExistingPopup );
      node_tree->node_popup_should_be_opened = false;
    }
  
    if ( ImGui::BeginPopup( "Node Popup" ) )
    {
      if ( ImGui::Button( "Dublicate" ) )
      {
        crude_snprintf( node_tree->new_node_name, sizeof( node_tree->new_node_name ), "%s_copy", crude_entity_get_name( world, node_tree->node_reference ), sizeof( node_tree->new_node_name ) );
        ImGui::OpenPopup("Node Popup Dublicate");
      }
      if ( ImGui::BeginPopupModal( "Node Popup Dublicate", NULL, ImGuiWindowFlags_MenuBar ) )
      {
        ImGui::Text( "Create dublicate of \"%s\"", crude_entity_get_name( world, node_tree->node_reference ) );
        ImGui::InputText( "Name", node_tree->new_node_name, sizeof( node_tree->new_node_name ) );
        ImGui::Separator( );
        if ( ImGui::Button( "Apply" ) )
        {
          crude_entity new_node = crude_entity_copy_hierarchy( world, node_tree->node_reference, node_tree->new_node_name, true, true );
          crude_entity_set_parent( world, new_node, crude_entity_get_parent( world, node_tree->node_reference ) );
          node_tree->node_reference = CRUDE_COMPOUNT_EMPTY( crude_entity );
          *selected_node = new_node;
        }
        ImGui::SameLine( );
        if ( ImGui::Button( "Cancel" ) )
        {
          node_tree->node_reference = CRUDE_COMPOUNT_EMPTY( crude_entity );
        }
        ImGui::EndPopup();
      }
      if ( ImGui::Button( "Create Child" ) )
      {
        crude_snprintf( node_tree->new_node_name, sizeof( node_tree->new_node_name ), "%s_child", crude_entity_get_name( world, node_tree->node_reference ), sizeof( node_tree->new_node_name ) );
        ImGui::OpenPopup("Node Popup Create Child");
      }
      if ( ImGui::BeginPopupModal( "Node Popup Create Child", NULL, ImGuiWindowFlags_MenuBar ) )
      {
        ImGui::Text( "Create child for parent \"%s\"", crude_entity_get_name( world, node_tree->node_reference ) );
        ImGui::InputText( "Name", node_tree->new_node_name, sizeof( node_tree->new_node_name ) );
        ImGui::Separator( );
        if ( ImGui::Button( "Apply" ) )
        {
          crude_entity new_node = crude_entity_create_empty( world, node_tree->new_node_name );
          crude_entity_set_parent( world, new_node, node_tree->node_reference );
          node_tree->node_reference = CRUDE_COMPOUNT_EMPTY( crude_entity );
          *selected_node = new_node;
        }
        ImGui::SameLine( );
        if ( ImGui::Button( "Cancel" ) )
        {
          node_tree->node_reference = CRUDE_COMPOUNT_EMPTY( crude_entity );
        }
        ImGui::EndPopup();
      }
      if ( ImGui::Button( "Rename" ) )
      {
        crude_snprintf( node_tree->new_node_name, sizeof( node_tree->new_node_name ), "%s", crude_entity_get_name( world, node_tree->node_reference ), sizeof( node_tree->new_node_name ) );
        ImGui::OpenPopup("Node Popup Rename");
      }
      if ( ImGui::BeginPopupModal( "Node Popup Rename", NULL, ImGuiWindowFlags_MenuBar ) )
      {
        ImGui::Text( "Rename node \"%s\"", crude_entity_get_name( world, node_tree->node_reference ) );
        ImGui::InputText( "Name", node_tree->new_node_name, sizeof( node_tree->new_node_name ) );
        ImGui::Separator( );
        if ( ImGui::Button( "Apply" ) )
        {
          crude_entity_set_name( world, node_tree->node_reference, node_tree->new_node_name );
          node_tree->node_reference = CRUDE_COMPOUNT_EMPTY( crude_entity );
        }
        ImGui::SameLine( );
        if ( ImGui::Button( "Cancel" ) )
        {
          node_tree->node_reference = CRUDE_COMPOUNT_EMPTY( crude_entity );
        }
        ImGui::EndPopup();
      }
      if ( ImGui::Button( "Cancel" ) )
      {
        node_tree->node_reference = CRUDE_COMPOUNT_EMPTY( crude_entity );
      }
      ImGui::EndPopup();
    }
  }

  current_node_index = 0u;
  crude_gui_node_tree_queue_draw_internal_( node_tree, world, node, selected_node, &current_node_index );
}

static void
crude_gui_node_tree_queue_draw_internal_
(
  _In_ crude_gui_node_tree                                *node_tree,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _Inout_ crude_entity                                    *selected_node,
  _In_ uint32                                             *current_node_index
)
{
  ecs_iter_t                                               child_it;
  ImGuiTreeNodeFlags                                       tree_node_flags;
  bool                                                     can_open_children_nodes, tree_node_opened;
  
  tree_node_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
  can_open_children_nodes = false;
  
  child_it = ecs_children( world, node );
  if ( !CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_gltf ) && ecs_children_next( &child_it ) )
  {
    if ( child_it.count )
    {
      can_open_children_nodes = true;
    }
  }
  
  tree_node_flags |= ImGuiTreeNodeFlags_DrawLinesFull;

  if ( !can_open_children_nodes )
  {
    tree_node_flags |= ImGuiTreeNodeFlags_Leaf;
  }

  if ( *selected_node == node )
  {
    tree_node_flags |= ImGuiTreeNodeFlags_Selected;
  }

  tree_node_opened = ImGui::TreeNodeEx( ( void* )( intptr_t )*current_node_index, tree_node_flags, crude_entity_get_name( world, node ) );
  if ( ImGui::IsItemClicked( ImGuiMouseButton_Left ) && !ImGui::IsItemToggledOpen( ) )
  {
    *selected_node = node;
  }

  if ( ImGui::IsItemClicked( ImGuiMouseButton_Right ) )
  {
    node_tree->node_reference = node;
    node_tree->node_popup_should_be_opened = true;
  }

  ++( *current_node_index );
  if ( tree_node_opened )
  {
    if ( can_open_children_nodes )
    {
      child_it = ecs_children( world, node );
      while ( ecs_children_next( &child_it ) )
      {
        for ( int32 i = 0; i < child_it.count; ++i )
        {
          crude_entity child = crude_entity_from_iterator( &child_it, i );
          crude_gui_node_tree_queue_draw_internal_( node_tree, world, child, selected_node, current_node_index );
        }
      }
    }
    
    ImGui::TreePop( );
  }
}