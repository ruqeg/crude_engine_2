#include <engine.h>
#include <core/log.h>
#include <core/memory.h>
#include <core/utils.h>

crude_heap_allocator g_heap_allocators[1];

void crude_initalize()
{
  crude_heap_allocator_initialize( &g_heap_allocators[0], 1024 * 1024 * 1024 );
  void* ptr = crude_allocate( &g_heap_allocators[0], 1024, 0u );
  crude_deallocate( &g_heap_allocators[0], ptr );
  crude_heap_allocator_deinitialize( &g_heap_allocators[0] );
}

void crude_deinitalize()
{
}