#pragma once

#include <core/platform.h>

// heap allocator
typedef enum crude_allocator_type
{
  CRUDE_ALLOCATOR_TYPE_HEAP
} crude_allocator_type;

typedef struct crude_allocator_common_data
{
  crude_allocator_type type;
} crude_allocator_common_data;

typedef struct crude_heap_allocator
{
  crude_allocator_common_data   common;
  void                          *tlsf_handle;
  void                          *memory;
  sizet                         size;
} crude_heap_allocator;

void crude_heap_allocator_initialize( crude_heap_allocator* allocator, sizet size );
void crude_heap_allocator_deinitialize( crude_heap_allocator* allocator );
void* crude_heap_allocator_allocate( crude_heap_allocator* allocator, sizet size, sizet alignment );
void crude_heap_allocator_deallocate( crude_heap_allocator* allocator, void* pointer );

void* crude_allocate( void* allocator, sizet size, sizet alignment );
void crude_deallocate( void* allocator, void* pointer );