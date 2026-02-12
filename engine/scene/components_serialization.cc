#include <engine/scene/components_serialization.h>

void
crude_components_serialization_manager_initialize
(
  _In_ crude_components_serialization_manager             *manager,
	_In_ crude_heap_allocator																*allocator
)
{
	CRUDE_HASHMAP_INITIALIZE_WITH_CAPACITY( manager->component_id_to_imgui_funs, 512, crude_heap_allocator_pack( allocator ) );
}

void
crude_components_serialization_manager_deinitialize
(
  _In_ crude_components_serialization_manager             *manager
)
{
	CRUDE_HASHMAP_DEINITIALIZE( manager->component_id_to_imgui_funs );
}

void
crude_components_serialization_manager_add_component_to_imgui
(
  _In_ crude_components_serialization_manager             *manager,
  _In_ ecs_id_t                                            component_id,
  _In_ crude_crude_components_serialization_parse_component_to_imgui_func fn
)
{
  uint64 key = crude_hash_bytes( CRUDE_CAST( uint8*, &component_id ), sizeof( component_id ), 0 );
  CRUDE_HASHMAP_SET( manager->component_id_to_imgui_funs, key, fn );
}