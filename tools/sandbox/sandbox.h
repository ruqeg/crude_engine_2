#pragma once

#include <engine.h>
#include <core/ecs.h>

typedef struct crude_sandbox
{
  crude_engine                                            *engine;
  bool                                                     working;
} crude_sandbox;

CRUDE_API void
crude_sandbox_initialize
(
  _In_ crude_sandbox                                      *sandbox,
  _In_ crude_engine                                       *engine
);

CRUDE_API void
crude_sandbox_deinitialize
(
  _In_ crude_sandbox                                      *sandbox
);

CRUDE_API bool
crude_sandbox_update
(
  _In_ crude_sandbox                                      *sandbox
);