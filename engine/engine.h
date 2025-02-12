#pragma once

#include <core/memory.h>

typedef struct crude_engine
{
  void* world;
  bool  running;
} crude_engine;

CRUDE_API crude_engine crude_engine_initialize( _In_ int32 num_threads );
CRUDE_API bool crude_engine_update( _In_ crude_engine *engine );