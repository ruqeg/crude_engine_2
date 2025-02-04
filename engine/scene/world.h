#pragma once

#include <flecs.h>
#include <core/alias.h>

typedef struct ecs_world_t crude_world;

CRUDE_API crude_world *crude_world_create();
CRUDE_API void crude_world_destroy( crude_world *world );