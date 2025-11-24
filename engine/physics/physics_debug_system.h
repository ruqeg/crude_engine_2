#pragma once

#if CRUDE_DEVELOP

#include <engine/core/ecs.h>

typedef struct crude_physics_debug_system_context
{
	crude_string_buffer																			*string_bufffer;
	char const																							*resources_absolute_directory;
} crude_physics_debug_system_context;

CRUDE_API void
crude_physics_debug_system_import
(
  _In_ ecs_world_t                                        *world,
	_In_ crude_physics_debug_system_context									*ctx
);

#endif