#pragma once

#include <core/platform.h>

// heap allocator
typedef struct crude_heap
{
  void      *tlsf_handle;
  void      *memory;
  sizet     size;
} crude_heap;

void crude_heap_initialize( crude_heap* heap, sizet size );
void crude_heap_deinitialize( crude_heap* heap );
void* crude_heap_allocate( crude_heap* heap, sizet size, sizet alignment );
void crude_heap_deallocate( crude_heap* heap, void* pointer );