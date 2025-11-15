#pragma once

#include <core/ecs.h>
#include <core/memory.h>
#include <cJSON.h>

typedef struct crude_scene crude_scene;

#define CRUDE_COMPONENT_STRING_DECLARE( component_type)\
char const *crude_component_string##component_type;

#define CRUDE_COMPONENT_STRING_DEFINE( component_type, component_name )\
char const *crude_component_string##component_type = component_name;

#define CRUDE_COMPONENT_STRING( component_type )\
crude_component_string##component_type

#define CRUDE_PARSE_JSON_TO_COMPONENT( component_type )\
crude_parse_json_to_component_func##component_type

#define CRUDE_PARSE_COMPONENT_TO_JSON( component_type )\
crude_parse_component_to_json_func##component_type

#define CRUDE_PARSE_COMPONENT_TO_IMGUI( component_type )\
crude_parse_component_to_imgui_func##component_type

#define CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( component_type )\
bool crude_parse_json_to_component_func##component_type\
(\
  _Out_ component_type                                    *component,\
  _In_ cJSON const                                        *component_json,\
  _In_ crude_entity                                        node,\
  _In_ crude_scene                                        *scene\
)

#define CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( component_type )\
cJSON* crude_parse_component_to_json_func##component_type\
(\
  _In_ component_type const                               *component\
)

#define CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( component_type )\
void crude_parse_component_to_imgui_func##component_type\
(\
  _In_ crude_entity                                        node,\
  _In_ component_type                                     *component\
)