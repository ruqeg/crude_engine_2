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
  if ( !crude_entity_valid( world, node ) )
  {
    return;
  }
  
  for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( node_inspector->components_serialization_manager->component_id_to_imgui_funs ); ++i )
  {
    if ( crude_hashmap_backet_key_valid( node_inspector->components_serialization_manager->component_id_to_imgui_funs[ i ].key ) )
    {
      node_inspector->components_serialization_manager->component_id_to_imgui_funs[ i ].value( world, node, node_inspector->node_manager );
    }
  }
}