#include <engine/scene/components_serialization.h>

void
crude_components_serialization_manager_initialize
(
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_heap_allocator                                *allocator
)
{
  CRUDE_HASHMAP_INITIALIZE_WITH_CAPACITY( manager->component_id_to_imgui_funs, 512, crude_heap_allocator_pack( allocator ) );
  CRUDE_HASHMAP_INITIALIZE_WITH_CAPACITY( manager->component_id_to_json_funs, 512, crude_heap_allocator_pack( allocator ) );
  CRUDE_HASHMAPSTR_INITIALIZE_WITH_CAPACITY( manager->component_name_to_json_funs, 512, crude_heap_allocator_pack( allocator ) );
}

void
crude_components_serialization_manager_deinitialize
(
  _In_ crude_components_serialization_manager             *manager
)
{
  CRUDE_HASHMAP_DEINITIALIZE( manager->component_id_to_imgui_funs );
  CRUDE_HASHMAP_DEINITIALIZE( manager->component_id_to_json_funs );
  CRUDE_HASHMAPSTR_DEINITIALIZE( manager->component_name_to_json_funs );
}

void
crude_components_serialization_manager_add_component_to_imgui
(
  _In_ crude_components_serialization_manager             *manager,
  _In_ ecs_id_t                                            component_id,
  _In_ crude_crude_components_serialization_parse_component_to_imgui_func fn
)
{
  CRUDE_HASHMAP_SET( manager->component_id_to_imgui_funs, component_id, fn );
}
    
void
crude_components_serialization_manager_add_component_to_json
(
  _In_ crude_components_serialization_manager             *manager,
  _In_ ecs_id_t                                            component_id,
  _In_ crude_crude_components_serialization_parse_component_to_json_func fn
)
{
  CRUDE_HASHMAP_SET( manager->component_id_to_json_funs, component_id, fn );
}

void
crude_components_serialization_manager_add_json_to_component
(
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_string_link                                   component_name,
  _In_ crude_crude_components_serialization_parse_json_to_component_func fn
)
{
  CRUDE_HASHMAPSTR_SET( manager->component_name_to_json_funs, component_name, fn );
}