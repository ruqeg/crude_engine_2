#pragma once

#include <flecs.h>

#include <core/math.h>
#include <scene/entity.h>

typedef struct crude_free_camera
{
  crude_float3  moving_speed_multiplier;
  crude_float2  rotating_speed_multiplier;
  crude_entity  entity_input;
} crude_free_camera;

CRUDE_API ECS_COMPONENT_DECLARE( crude_free_camera );

CRUDE_API void
crude_scripts_componentsImport
(
  _In_ ecs_world_t            *world
);