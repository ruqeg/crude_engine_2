#include <engine/gui/node_tree.h>

static char const* crude_gui_node_tree_node_types_names_[ CRUDE_GUI_NODE_TYPE_COUNT ] =
{
  "Empty 3D",
  "GLTF"
};

static bool
crude_gui_node_tree_passed_filter_internal_
(
  _In_ crude_gui_node_tree                                *node_tree,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node
);

static void
crude_gui_node_tree_queue_draw_internal_
(
  _In_ crude_gui_node_tree                                *node_tree,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _Inout_ crude_entity                                    *selected_node
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
        CRUDE_IMGUI_START_OPTIONS;
        ImGui::Text( "Create child for parent \"%s\"", crude_entity_get_name( world, node_tree->node_reference ) );
        CRUDE_IMGUI_OPTION( "Name", {
          ImGui::InputText( "##Name", node_tree->new_node_name, sizeof( node_tree->new_node_name ) );
        } );
        CRUDE_IMGUI_OPTION( "Type", {
          ImGui::Combo( "##Type", &node_tree->new_node_type_index, crude_gui_node_tree_node_types_names_, IM_ARRAYSIZE( crude_gui_node_tree_node_types_names_ ) );
        } );
        ImGui::Separator( );
        if ( ImGui::Button( "Apply" ) )
        {
          crude_entity new_node = crude_entity_create_empty( world, node_tree->new_node_name );
          crude_entity_set_parent( world, new_node, node_tree->node_reference );
          switch ( node_tree->new_node_type_index )
          {
          case CRUDE_GUI_NODE_TYPE_EMPTY_3D:
          {
            CRUDE_ENTITY_SET_COMPONENT( world, new_node, crude_transform, { crude_transform_empty( ) } );
            break;
          }
          case CRUDE_GUI_NODE_TYPE_GLTF:
          {
            CRUDE_ENTITY_SET_COMPONENT( world, new_node, crude_gltf, { } );
            CRUDE_ENTITY_SET_COMPONENT( world, new_node, crude_transform, { crude_transform_empty( ) } );
            break;
          }
          }
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

  ImGui::SetNextItemWidth( -FLT_MIN );
  ImGui::SetNextItemShortcut( ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_Tooltip );
  ImGui::PushItemFlag( ImGuiItemFlags_NoNavDefaultFocus, true );
  if ( ImGui::InputTextWithHint( "##Filter", "incl,-excl", node_tree->im_text_filter.InputBuf, IM_COUNTOF( node_tree->im_text_filter.InputBuf ), ImGuiInputTextFlags_EscapeClearsAll ) )
  {
    node_tree->im_text_filter.Build( );
  }
  ImGui::PopItemFlag();

  if ( ImGui::BeginTable( "##bg", 1 ) )
  {
    crude_gui_node_tree_queue_draw_internal_( node_tree, world, node, selected_node );
    ImGui::EndTable( );
  }
}

static void
crude_gui_node_tree_queue_draw_internal_
(
  _In_ crude_gui_node_tree                                *node_tree,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _Inout_ crude_entity                                    *selected_node
)
{
  ecs_iter_t                                               child_it;
  ImGuiTreeNodeFlags                                       tree_node_flags;
  bool                                                     passed_filter, can_open_children_nodes, tree_node_opened;

  tree_node_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_OpenOnDoubleClick;
  passed_filter = can_open_children_nodes = false;
  
  if ( node_tree->im_text_filter.IsActive( ) && !crude_gui_node_tree_passed_filter_internal_( node_tree, world, node ) )
  {
    return;
  }

  child_it = ecs_children( world, node );
  if ( !CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_gltf ) && ecs_children_next( &child_it ) )
  {
    if ( child_it.count )
    {
      can_open_children_nodes = true;
    }
  }

  if ( node_tree->im_text_filter.IsActive( ) && node_tree->im_text_filter.PassFilter( crude_entity_get_name( world, node ) ) )
  {
    tree_node_flags |= ImGuiTreeNodeFlags_Selected;
    ImGui::TreeNodeSetOpen( ImGui::GetCurrentWindow( )->GetID( node ), true );
  }

  if ( !can_open_children_nodes )
  {
    tree_node_flags |= ImGuiTreeNodeFlags_Leaf;
  }
  
  if ( *selected_node == node )
  {
    tree_node_flags |= ImGuiTreeNodeFlags_Selected;
  }
  
  ImGui::TableNextRow( );
  ImGui::TableNextColumn( );

  tree_node_opened = ImGui::TreeNodeEx( ( void* )( intptr_t )node, tree_node_flags, crude_entity_get_name( world, node ) );
  if ( ImGui::IsItemClicked( ImGuiMouseButton_Left ) && !ImGui::IsItemToggledOpen( ) )
  {
    *selected_node = node;
  }

  if ( ImGui::IsItemClicked( ImGuiMouseButton_Right ) )
  {
    node_tree->node_reference = node;
    node_tree->node_popup_should_be_opened = true;
  }

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
          crude_gui_node_tree_queue_draw_internal_( node_tree, world, child, selected_node );
        }
      }
    }
    
    ImGui::TreePop( );
  }
}

bool
crude_gui_node_tree_passed_filter_internal_
(
  _In_ crude_gui_node_tree                                *node_tree,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node
)
{
  ecs_iter_t                                               child_it;
  bool                                                     passed_filter;

  passed_filter = node_tree->im_text_filter.PassFilter( crude_entity_get_name( world, node ) );

  child_it = ecs_children( world, node );
  while ( ecs_children_next( &child_it ) )
  {
    for ( int32 i = 0; i < child_it.count; ++i )
    {
      crude_entity child = crude_entity_from_iterator( &child_it, i );
      passed_filter |= crude_gui_node_tree_passed_filter_internal_( node_tree, world, child );
    }
  }

  return passed_filter;
}