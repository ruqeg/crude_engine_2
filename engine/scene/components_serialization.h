#pragma once

#include <thirdparty/cJSON/cJSON.h>

#include <engine/core/ecs.h>
#include <engine/core/memory.h>
#include <engine/core/string.h>
#include <engine/core/hashmap.h>
#include <engine/core/hashmapstr.h>

typedef struct crude_node_manager crude_node_manager;

typedef void (*crude_crude_components_serialization_parse_component_to_imgui_func)
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _In_ crude_node_manager                                 *manager
);

typedef cJSON* (*crude_crude_components_serialization_parse_component_to_json_func)
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _In_ crude_node_manager                                 *manager
);

typedef void (*crude_crude_components_serialization_parse_json_to_component_func)
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _In_ cJSON const                                        *component_json,
  _In_ crude_node_manager                                 *manager
);

typedef struct crude_components_serialization_manager
{
  CRUDE_HASHMAP( crude_crude_components_serialization_parse_component_to_imgui_func ) *component_id_to_imgui_funs;
  CRUDE_HASHMAP( crude_crude_components_serialization_parse_component_to_json_func ) *component_id_to_json_funs;
  CRUDE_HASHMAPSTR( crude_crude_components_serialization_parse_json_to_component_func ) *component_name_to_json_funs;
} crude_components_serialization_manager;

CRUDE_API void
crude_components_serialization_manager_initialize
(
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_heap_allocator                                *allocator
);

CRUDE_API void
crude_components_serialization_manager_deinitialize
(
  _In_ crude_components_serialization_manager             *manager
);

CRUDE_API void
crude_components_serialization_manager_add_component_to_imgui
(
  _In_ crude_components_serialization_manager             *manager,
  _In_ ecs_id_t                                            component_id,
  _In_ crude_crude_components_serialization_parse_component_to_imgui_func fn
);

CRUDE_API void
crude_components_serialization_manager_add_component_to_json
(
  _In_ crude_components_serialization_manager             *manager,
  _In_ ecs_id_t                                            component_id,
  _In_ crude_crude_components_serialization_parse_component_to_json_func fn
);

CRUDE_API void
crude_components_serialization_manager_add_json_to_component
(
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_string_link                                   component_name,
  _In_ crude_crude_components_serialization_parse_json_to_component_func fn
);

#define CRUDE_COMPONENT_STRING( component_type )\
__crude_component_string##component_type

#define CRUDE_COMPONENT_STRING_DECLARE( component_type)\
char const *CRUDE_COMPONENT_STRING( component_type );

#define CRUDE_COMPONENT_STRING_DEFINE( component_type, component_name )\
char const *CRUDE_COMPONENT_STRING( component_type ) = component_name;

#define CRUDE_PARSE_JSON_TO_COMPONENT( component_type )\
crude_parse_json_to_component_func##component_type

#define CRUDE_PARSE_COMPONENT_TO_JSON( component_type )\
crude_parse_component_to_json_func##component_type

#define CRUDE_PARSE_COMPONENT_TO_IMGUI( component_type )\
crude_parse_component_to_imgui_func##component_type


#define CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( component_type )\
void crude_components_serialization_parse_json_to_component__func_raw##component_type\
(\
  _In_ crude_ecs                                          *world,\
  _In_ crude_entity                                        node,\
  _In_ cJSON const                                        *component_json,\
  _In_ crude_node_manager                                 *manager\
);\
bool crude_parse_json_to_component_func##component_type\
(\
  _Out_ component_type                                    *component,\
  _In_ cJSON const                                        *component_json,\
  _In_ crude_ecs                                          *world,\
  _In_ crude_entity                                        node,\
  _In_ crude_node_manager                                 *manager\
);

#define CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( component_type )\
cJSON* crude_components_serialization_parse_component_to_json_func_raw##component_type\
(\
  _In_ crude_ecs                                          *world,\
  _In_ crude_entity                                        node,\
  _In_ crude_node_manager                                 *manager\
);\
cJSON* crude_parse_component_to_json_func##component_type\
(\
  _In_ component_type const                               *component,\
  _In_ crude_node_manager                                 *manager\
);

#define CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( component_type )\
void crude_parse_component_to_imgui_func##component_type\
(\
  _In_ crude_ecs                                          *world,\
  _In_ crude_entity                                        node,\
  _In_ component_type                                     *component,\
  _In_ crude_node_manager                                 *manager\
);\
void crude_components_serialization_parse_component_to_imgui_func_raw##component_type\
(\
  _In_ crude_ecs                                          *world,\
  _In_ crude_entity                                        node,\
  _In_ crude_node_manager                                 *manager\
);

#define CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, component_type )\
crude_components_serialization_manager_add_component_to_imgui( manager, ecs_id( component_type ), crude_components_serialization_parse_component_to_imgui_func_raw##component_type )

#define CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, component_type )\
crude_components_serialization_manager_add_component_to_json( manager, ecs_id( component_type ), crude_components_serialization_parse_component_to_json_func_raw##component_type )

#define CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, component_type )\
crude_components_serialization_manager_add_json_to_component( manager, CRUDE_COMPOUNT( crude_string_link, { #component_type } ), crude_components_serialization_parse_json_to_component__func_raw##component_type )

#define CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( component_type )\
void crude_components_serialization_parse_component_to_imgui_func_raw##component_type\
(\
  _In_ crude_ecs                                          *world,\
  _In_ crude_entity                                        node,\
  _In_ crude_node_manager                                 *manager\
)\
{\
  component_type *component = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, node, component_type );\
  if ( component && ImGui::CollapsingHeader( CRUDE_COMPONENT_STRING( component_type ) ) )\
  {\
    CRUDE_PARSE_COMPONENT_TO_IMGUI( component_type )( world, node, component, manager );\
  }\
}\
void crude_parse_component_to_imgui_func##component_type\
(\
  _In_ crude_ecs                                          *world,\
  _In_ crude_entity                                        node,\
  _In_ component_type                                     *component,\
  _In_ crude_node_manager                                 *manager\
)

#define CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( component_type )\
cJSON* crude_components_serialization_parse_component_to_json_func_raw##component_type\
(\
  _In_ crude_ecs                                          *world,\
  _In_ crude_entity                                        node,\
  _In_ crude_node_manager                                 *manager\
)\
{\
  component_type const *component = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, component_type );\
  if ( component )\
  {\
    return CRUDE_PARSE_COMPONENT_TO_JSON( component_type )( component, manager );\
  }\
  return NULL;\
}\
cJSON* crude_parse_component_to_json_func##component_type\
(\
  _In_ component_type const                               *component,\
  _In_ crude_node_manager                                 *manager\
)

#define CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( component_type )\
void crude_components_serialization_parse_json_to_component__func_raw##component_type\
(\
  _In_ crude_ecs                                          *world,\
  _In_ crude_entity                                        node,\
  _In_ cJSON const                                        *component_json,\
  _In_ crude_node_manager                                 *manager\
)\
{\
  component_type                                           component;\
  CRUDE_PARSE_JSON_TO_COMPONENT( component_type )( &component, component_json, world, node, manager );\
  CRUDE_ENTITY_SET_COMPONENT( world, node, component_type, { component } );\
}\
bool crude_parse_json_to_component_func##component_type\
(\
  _Out_ component_type                                    *component,\
  _In_ cJSON const                                        *component_json,\
  _In_ crude_ecs                                          *world,\
  _In_ crude_entity                                        node,\
  _In_ crude_node_manager                                 *manager\
)