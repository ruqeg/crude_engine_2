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

CRUDE_API void crude_initialize_heap_allocator( _In_ crude_heap_allocator* allocator, _In_ sizet capacity );
CRUDE_API void crude_deinitialize_heap_allocator( _In_ crude_heap_allocator* allocator );
CRUDE_API void* crude_allocate_heap( _In_ crude_heap_allocator* allocator, _In_ sizet size, _In_ sizet alignment );
CRUDE_API void crude_deallocate_heap( _In_ crude_heap_allocator* allocator, _In_ void* pointer );

CRUDE_API void crude_initialize_stack_allocator( _In_ crude_stack_allocator* allocator, _In_ sizet capacity );
CRUDE_API void crude_deinitialize_stack_allocator( _In_ crude_stack_allocator* allocator );
CRUDE_API void* crude_allocate_stack( _In_ crude_stack_allocator* allocator, _In_ sizet size, _In_ sizet alignment );
CRUDE_API void crude_deallocate_stack( _In_ crude_stack_allocator* allocator, _In_ void* pointer );

typedef void* (*crude_allocate_fun)( sizet size, sizet alignment );
typedef void (*crude_deallocate_fun)( void* pointer );

typedef struct crude_allocator
{
  crude_allocate_fun    allocate;
  crude_deallocate_fun  deallocate;
} crude_allocator;