#pragma once

#include <core/memory.h>

typedef struct crude_engine
{
  void* world;
  bool  running;
} crude_engine;

crude_engine crude_engine_initialize( int32 num_threads );
bool crude_engine_update();