#pragma once

#include <core/platform.h>

typedef enum crude_allocator_type
{
  CRUDE_ALLOCATOR_TYPE_HEAP,
  CRUDE_ALLOCATOR_TYPE_STACK,
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
  sizet                         capacity;
} crude_heap_allocator;

typedef struct crude_stack_allocator
{
  crude_allocator_common_data   common;
  void                          *memory;
  sizet                         capacity;
  sizet                         occupied;
} crude_stack_allocator;

void* crude_allocate( void* allocator, sizet size, sizet alignment );
void crude_deallocate( void* allocator, void* pointer );

void crude_heap_allocator_initialize( crude_heap_allocator* allocator, sizet capacity );
void crude_heap_allocator_deinitialize( crude_heap_allocator* allocator );
void* crude_heap_allocator_allocate( crude_heap_allocator* allocator, sizet size, sizet alignment );
void crude_heap_allocator_deallocate( crude_heap_allocator* allocator, void* pointer );

void crude_stack_allocator_initialize( crude_stack_allocator* allocator, sizet capacity );
void crude_stack_allocator_deinitialize( crude_stack_allocator* allocator );
void* crude_stack_allocator_allocate( crude_stack_allocator* allocator, sizet size, sizet alignment );
void crude_stack_allocator_deallocate( crude_stack_allocator* allocator, void* pointer );