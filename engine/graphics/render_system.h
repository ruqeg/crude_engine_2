#pragma once

#include <flecs.h>

#include <core/alias.h>
#include <core/math.h>

CRUDE_API typedef struct TMP_UBO
{
  crude_float4x4a world_to_clip;
} TMP_UBO;

CRUDE_API void
crude_render_systemImport
(
  ecs_world_t *world
);