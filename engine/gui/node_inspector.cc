#include <engine/gui/node_inspector.h>

void
crude_gui_node_inspector_initialize
(
  _In_ crude_gui_node_inspector                           *node_inspector,
  _In_ crude_components_serialization_manager             *components_serialization_manager,
  _In_ crude_node_manager                                 *node_manager
)
{
  node_inspector->components_serialization_manager = components_serialization_manager;
  node_inspector->node_manager = node_manager;
}

void
crude_gui_node_inspector_deinitialize
(
  _In_ crude_gui_node_inspector                           *node_inspector
)
{
}

void
crude_gui_node_inspector_update
(
  _In_ crude_gui_node_inspector                           *node_inspector
)
{
}

void
crude_gui_node_inspector_queue_draw
(
  _In_ crude_gui_node_inspector                           *node_inspector,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node
)
{
  crude_node_external const                               *node_external;

  if ( !crude_entity_valid( world, node ) )
  {
    return;
  }

  CRUDE_IMGUI_START_OPTIONS;
  
  CRUDE_IMGUI_OPTION( "Name", {
    ImGui::InputText( "##Name", CRUDE_CAST( char*, crude_entity_get_name( world, node ) ), 4096, ImGuiInputTextFlags_ReadOnly );
  } );

  node_external = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_node_external );

  for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( node_inspector->components_serialization_manager->component_id_to_imgui_funs ); ++i )
  {
    if ( crude_hashmap_backet_key_valid( node_inspector->components_serialization_manager->component_id_to_imgui_funs[ i ].key ) )
    {
      if ( node_external && node_external->type == CRUDE_NODE_EXTERNAL_TYPE_REFERENCE )
      {
        /* Display only external component, hide other since we will now safe this changes anyway */
        if ( node_inspector->components_serialization_manager->component_id_to_imgui_funs[ i ].key != ecs_id( crude_node_external ) )
        {
          continue;
        }
      }
      node_inspector->components_serialization_manager->component_id_to_imgui_funs[ i ].value( world, node, node_inspector->node_manager );
    }
  }
}