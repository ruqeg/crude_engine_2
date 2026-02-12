#pragma once

#if CRUDE_DEVELOP

#include <engine/core/ecs.h>
#include <engine/scene/components_serialization.h>
#include <engine/core/string.h>

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_physics_debug_system_context
{
  crude_string_buffer                                     *string_bufffer;
  char const                                              *resources_absolute_directory;
} crude_physics_debug_system_context;

CRUDE_API void
crude_physics_debug_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_physics_debug_system_context                 *ctx
);

#endif