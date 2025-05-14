#pragma once

#include <engine.h>
#include <core/ecs.h>

typedef struct crude_paprika
{
  crude_engine                                            *engine;
  crude_heap_allocator                                     graphics_allocator;
  crude_stack_allocator                                    temporary_allocator;
  crude_entity                                             camera;
  crude_entity                                             scene;
  bool                                                     working;
} crude_paprika;

CRUDE_API void
crude_paprika_initialize
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_engine                                       *engine
);

CRUDE_API void
crude_paprika_deinitialize
(
  _In_ crude_paprika                                      *paprika
);

CRUDE_API bool
crude_paprika_update
(
  _In_ crude_paprika                                      *paprika
);