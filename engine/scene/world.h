#pragma once

#include <flecs.h>

typedef struct ecs_world_t crude_world;

crude_world *crude_world_create();
void crude_world_destroy( crude_world *world );