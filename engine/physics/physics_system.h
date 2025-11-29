#pragma once

#include <engine/core/ecs.h>
#include <engine/physics/physics.h>

typedef struct crude_physics_system_context
{
  crude_physics                          *physics;
} crude_physics_system_context;

CRUDE_API void
crude_physics_system_import
(
  _In_ ecs_world_t                                        *world,
  _In_ crude_physics_system_context                        *ctx
);