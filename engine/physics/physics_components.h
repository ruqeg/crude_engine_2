#pragma once

#include <core/ecs.h>
#include <physics/physics.h>

CRUDE_API CRUDE_ECS_COMPONENT_DECLARE( crude_physics_particle );
CRUDE_API CRUDE_ECS_COMPONENT_DECLARE( crude_physics_particle_spring );

CRUDE_ECS_MODULE_IMPORT_DECL( crude_physics_components );