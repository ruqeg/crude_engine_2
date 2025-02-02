#pragma once

#include <core/memory.h>

crude_heap heap_allocators;
crude_stack stack_allocators;

void crude_initalize();
void crude_deinitalize();