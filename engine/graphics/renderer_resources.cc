#include <core/hash_map.h>

#include <graphics/renderer_resources.h>

uint32
crude_gfx_renderer_technique_get_pass_index
(
  _In_ crude_gfx_renderer_technique                       *technique,
  _In_ char const                                         *name
)
{
  uint64 hashed_name = crude_hash_string( name, 0 );
  uint32 index = CRUDE_HASHMAP_GET( technique->name_hashed_to_pass_index, hashed_name )->value;
  return index;
}

uint16
crude_gfx_renderer_technique_pass_get_binding_index
(
  _In_ crude_gfx_renderer_technique_pass                  *technique_pass,
  _In_ char const                                         *name
)
{
  uint64 hashed_name = crude_hash_string( name, 0 );
  uint32 index = CRUDE_HASHMAP_GET( technique_pass->name_hashed_to_descriptor_index, hashed_name )->value;
  return index;
}