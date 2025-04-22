#pragma once

#include <flecs.h>

#include <physics/physics.h>

CRUDE_API ECS_COMPONENT_DECLARE( crude_physics_particle );
CRUDE_API ECS_COMPONENT_DECLARE( crude_physics_particle_spring );

CRUDE_API void
crude_physics_componentsImport
(
  ecs_world_t *world
);