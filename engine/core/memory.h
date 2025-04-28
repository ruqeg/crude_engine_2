#pragma once

#include <core/alias.h>

/*****************************************
 *
 * Heap Allocator
 * 
 ******************************************/
typedef struct crude_heap_allocator
{
  void                        *tlsf_handle;
  void                        *memory;
  sizet                        capacity;
  char const                  *name;
} crude_heap_allocator;

CRUDE_API void
crude_initialize_heap_allocator
(
  _In_ crude_heap_allocator   *allocator,
  _In_ sizet                   capacity,
  char const                  *name
);

CRUDE_API void
crude_deinitialize_heap_allocator
(
  _In_ crude_heap_allocator   *allocator
);

CRUDE_API void*
crude_heap_allocate
(
  _In_ crude_heap_allocator   *allocator,
  _In_ sizet                   size
);

CRUDE_API void*
crude_heap_allocate_align
(
  _In_ crude_heap_allocator   *allocator,
  _In_ sizet                   size,
  _In_ sizet                   alignment
);

CRUDE_API void*
crude_heap_reallocate
(
  _In_ crude_heap_allocator *allocator,
  _In_ void                 *pointer,
  _In_ sizet                 size
);

CRUDE_API void
crude_heap_deallocate
(
  _In_ crude_heap_allocator   *allocator,
  _In_ void                   *pointer
);

/*****************************************
 *
 * Stack Allocator
 * 
 ******************************************/
typedef struct crude_stack_allocator
{
  void                        *memory;
  sizet                        capacity;
  sizet                        occupied;
} crude_stack_allocator;

CRUDE_API void
crude_initialize_stack_allocator
(
  _In_ crude_stack_allocator  *allocator,
  _In_ sizet                   capacity
);

CRUDE_API void
crude_deinitialize_stack_allocator
(
  _In_ crude_stack_allocator  *allocator
);

CRUDE_API void*
crude_stack_allocate
( 
  _In_ crude_stack_allocator  *allocator,
  _In_ sizet                   size,
  _In_ sizet                   alignment
);

CRUDE_API void
crude_stack_deallocate
(
  _In_ crude_stack_allocator  *allocator,
  _In_ void                   *pointer
);

/*****************************************
 *
 * Utils
 * 
 ******************************************/
CRUDE_API sizet
crude_memory_align
(
  _In_ sizet                   size,
  _In_ sizet                   alignment
);

/*****************************************
 *
 * Common Allocator Interface
 * 
 ******************************************/
typedef void* (*crude_allocate_fun)( void *ctx, sizet size );
typedef void* (*crude_allocate_align_fun)( void *ctx, sizet size, sizet alignment );
typedef void* (*crude_reallocate_fun)( void *ctx, void *pointer, sizet size );
typedef void  (*crude_deallocate_fun)(  void *ctx, void *pointer );

typedef struct crude_allocator
{
  crude_allocate_fun                                       allocate;
  crude_reallocate_fun                                     reallocate;
  crude_deallocate_fun                                     deallocate;
  crude_allocate_align_fun                                 allocate_align;
  void                                                    *ctx;
} crude_allocator;

#define CRUDE_ALLOCATE( allocator, size ) allocator.allocate( allocator.ctx, size )
#define CRUDE_DEALLOCATE( allocator, ptr ) allocator.deallocate( allocator.ctx, ptr )
#define CRUDE_REALLOCATE( allocator, ptr, size ) allocator.reallocate( allocator.ctx, ptr, size )
#define CRUDE_ALLOCATE_ALIGN( allocator, size, alignment ) allocator.allocate_align( allocator.ctx, size, alignment )

CRUDE_API crude_allocator 
crude_pack_heap_allocator
(
  _In_ crude_heap_allocator                               *heap_allocator
);