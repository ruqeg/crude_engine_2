#pragma once

#include <core/alias.h>

typedef struct crude_heap_allocator
{
  void    *tlsf_handle;
  void    *memory;
  sizet   capacity;
} crude_heap_allocator;

typedef struct crude_stack_allocator
{
  void    *memory;
  sizet   capacity;
  sizet   occupied;
} crude_stack_allocator;

CRUDE_API void crude_heap_allocator_initialize( crude_heap_allocator* allocator, sizet capacity );
CRUDE_API void crude_heap_allocator_deinitialize( crude_heap_allocator* allocator );
CRUDE_API void* crude_heap_allocator_allocate( crude_heap_allocator* allocator, sizet size, sizet alignment );
CRUDE_API void crude_heap_allocator_deallocate( crude_heap_allocator* allocator, void* pointer );

CRUDE_API void crude_stack_allocator_initialize( crude_stack_allocator* allocator, sizet capacity );
CRUDE_API void crude_stack_allocator_deinitialize( crude_stack_allocator* allocator );
CRUDE_API void* crude_stack_allocator_allocate( crude_stack_allocator* allocator, sizet size, sizet alignment );
CRUDE_API void crude_stack_allocator_deallocate( crude_stack_allocator* allocator, void* pointer );

typedef void* (*crude_allocate_fun)( sizet size, sizet alignment );
typedef void* (*crude_deallocate_fun)( void* pointer );

typedef struct crude_allocator
{
  crude_allocate_fun    allocate;
  crude_deallocate_fun  deallocate;
} crude_allocator;