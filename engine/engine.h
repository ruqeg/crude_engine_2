#pragma once

#include <core/memory.h>

typedef struct crude_engine
{
  void* world;
  bool  running;
} crude_engine;

CRUDE_API crude_engine crude_engine_initialize( int32 num_threads );
CRUDE_API bool crude_engine_update( crude_engine *engine );