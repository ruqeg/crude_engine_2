#pragma once

#include <engine/core/alias.h>

#define CRUDE_RKILO( size ) ( size * 1024u )
#define CRUDE_RMEGA( size ) ( size * 1024u * 1024u )
#define CRUDE_RGIGA( size ) ( size * 1024u * 1024u * 1024u )

typedef enum crude_allocator_type
{
  CRUDE_ALLOCATOR_TYPE_UNKNOWN,
  CRUDE_ALLOCATOR_TYPE_HEAP,
  CRUDE_ALLOCATOR_TYPE_STACK,
  CRUDE_ALLOCATOR_TYPE_LINEAR,
} crude_allocator_type;

/*****************************************
 *
 * Heap Allocator
 * 
 ******************************************/
typedef struct crude_heap_allocator
{
  void                                                    *tlsf_handle;
  void                                                    *memory;
  sizet                                                    capacity;
  char const                                              *name;
  int64                                                    occupied;
} crude_heap_allocator;

CRUDE_API void
crude_heap_allocator_initialize
(
  _In_ crude_heap_allocator                               *allocator,
  _In_ sizet                                               capacity,
  _In_ char const                                         *name
);

CRUDE_API void
crude_heap_allocator_deinitialize
(
  _In_ crude_heap_allocator                               *allocator
);

CRUDE_API void*
crude_heap_allocator_allocate
(
  _In_ crude_heap_allocator                               *allocator,
  _In_ sizet                                               size
);

CRUDE_API void*
crude_heap_allocator_allocate_align
(
  _In_ crude_heap_allocator                               *allocator,
  _In_ sizet                                               size,
  _In_ sizet                                               alignment
);

CRUDE_API void*
crude_heap_allocator_reallocate
(
  _In_ crude_heap_allocator                               *allocator,
  _In_ void                                               *pointer,
  _In_ sizet                                               size
);

CRUDE_API void
crude_heap_allocator_deallocate
(
  _In_ crude_heap_allocator                               *allocator,
  _In_ void                                               *pointer
);

/*****************************************
 *
 * Stack Allocator
 * 
 ******************************************/
typedef struct crude_stack_allocator
{
  void                                                    *memory;
  sizet                                                    capacity;
  sizet                                                    occupied;
  char const                                              *name;
} crude_stack_allocator;

CRUDE_API void
crude_stack_allocator_initialize
(
  _In_ crude_stack_allocator                              *allocator,
  _In_ sizet                                               capacity,
  _In_ char const                                         *name
);

CRUDE_API void
crude_stack_allocator_deinitialize
(
  _In_ crude_stack_allocator                              *allocator
);

CRUDE_API void*
crude_stack_allocator_allocate
( 
  _In_ crude_stack_allocator                              *allocator,
  _In_ sizet                                               size
);

CRUDE_API size_t
crude_stack_allocator_get_marker
(
  _In_ crude_stack_allocator                              *allocator
);

CRUDE_API void
crude_stack_allocator_free_marker
(
  _In_ crude_stack_allocator                              *allocator,
  _In_ size_t                                              marker
);


/*****************************************
 *
 * Linear Allocator
 * 
 ******************************************/
typedef struct crude_linear_allocator
{
  void                                                    *memory;
  sizet                                                    capacity;
  sizet                                                    occupied;
  char const                                              *name;
} crude_linear_allocator;

CRUDE_API void
crude_linear_allocator_initialize
(
  _In_ crude_linear_allocator                             *allocator,
  _In_ sizet                                               capacity,
  _In_ char const                                         *name
);

CRUDE_API void
crude_linear_allocator_deinitialize
(
  _In_ crude_linear_allocator                             *allocator
);

CRUDE_API void*
crude_linear_allocator_allocate
( 
  _In_ crude_linear_allocator                             *allocator,
  _In_ sizet                                               size
);

CRUDE_API void
crude_linear_allocator_clear
(
  _In_ crude_linear_allocator                             *allocator
);

/*****************************************
 *
 * Utils
 * 
 ******************************************/
CRUDE_API sizet
crude_memory_align
(
  _In_ sizet                                               size,
  _In_ sizet                                               alignment
);

CRUDE_API void
crude_memory_copy
(
  _Out_ void                                              *dst,
  _In_ void const                                         *src,
  _In_ sizet                                               size
);

CRUDE_API void
crude_memory_set
(
  _Out_ void                                              *dst,
  _In_ int32                                               val,
  _In_ sizet                                               size
);

/*****************************************
 *
 * Common Allocator Interface
 * 
 ******************************************/
typedef void* (*crude_allocate_fun)( void *ctx, sizet size );
typedef void* (*crude_allocate_align_fun)( void *ctx, sizet size, sizet alignment );
typedef void  (*crude_deallocate_fun)(  void *ctx, void *pointer );

typedef struct crude_allocator_container
{
  crude_allocate_fun                                       allocate;
  crude_deallocate_fun                                     deallocate;
  crude_allocate_align_fun                                 allocate_align;
  void                                                    *ctx;
} crude_allocator_container;

CRUDE_API crude_allocator_container 
crude_heap_allocator_pack
(
  _In_ crude_heap_allocator                               *heap_allocator
);

CRUDE_API crude_allocator_container 
crude_stack_allocator_pack
(
  _In_ crude_stack_allocator                              *stack_allocator
);

CRUDE_API crude_allocator_container 
crude_linear_allocator_pack
(
  _In_ crude_linear_allocator                             *linear_allocator
);

CRUDE_API crude_allocator_type
crude_allocator_container_get_type
(
  _In_ crude_allocator_container                           allocator_container
);

#define CRUDE_ALLOCATE( allocator_container, size ) ( ( allocator_container ).allocate( ( allocator_container ).ctx, size ) )
#define CRUDE_DEALLOCATE( allocator_container, ptr ) ( ( allocator_container ).deallocate( ( allocator_container ).ctx, ptr ) )
#define CRUDE_ALLOCATE_ALIGN( allocator_container, size, alignment ) ( ( allocator_container ).allocate_align( ( allocator_container ).ctx, size, alignment ) )