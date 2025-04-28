#include <stdint.h>
#include <malloc.h>

#define STB_DS_IMPLEMENTATION
#include <core/algorithms.h>

crude_heap_allocator                                      *s_allocator;

void*
_crude_array_realloc
(
  _In_ void                                               *ptr,
  _In_ uint32_t                                            size
)
{
  crude_reallocate_heap( s_allocator, ptr, size );
}

void
_crude_array_dealloc
(
  void                                               *ptr
)
{
  crude_deallocate_heap( s_allocator, ptr );
}

#include <core/algorithms.h>

void
crude_array_set_allocator
(
  _In_ crude_heap_allocator                               *allocator
)
{
  s_allocator = allocator;
}