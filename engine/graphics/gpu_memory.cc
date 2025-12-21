#include <engine/graphics/gpu_device.h>

#include <engine/graphics/gpu_memory.h>

void
crude_gfx_stack_allocator_initialize
(
  _In_ crude_gfx_stack_allocator                          *allocator,
  _In_ crude_gfx_device                                   *gpu,
  _In_ sizet                                               capacity,
  _In_ char const                                         *name
)
{
  allocator->gpu = gpu;
  allocator->allocation = crude_gfx_memory_allocate_with_name( gpu, capacity, CRUDE_GFX_MEMORY_TYPE_CPU_GPU, name );
  allocator->capacity = capacity;
  allocator->occupied = 0u;
  allocator->name = name;
  CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "GPU Stack allocator of capacity %llu created", capacity );
}

void
crude_gfx_stack_allocator_deinitialize
(
  _In_ crude_gfx_stack_allocator                          *allocator
)
{
  CRUDE_ASSERTM( CRUDE_CHANNEL_MEMORY, allocator->occupied == 0u, "GPU Stack allocator \"%s\" shutdown. Allocated memory detected. Allocated %llu, total %llu", allocator->name, allocator->occupied, allocator->capacity );
  crude_gfx_memory_deallocate( allocator->gpu, allocator->allocation );
}

crude_gfx_memory_allocation
crude_gfx_stack_allocator_allocate
( 
  _In_ crude_gfx_stack_allocator                          *allocator,
  _In_ uint64                                              size
)
{
  crude_gfx_memory_allocation                              allocation;

  if ( allocator->occupied + size > allocator->capacity )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_MEMORY, "GPU new memory block is too big for current stack allocator! %i occupied, %i requested size, %i capacity", allocator->occupied, size, allocator->capacity );
  }

  allocation = crude_gfx_memory_allocation_empty( );
  allocation.cpu_address = CRUDE_CAST( uint8*, allocator->allocation.cpu_address ) + allocator->occupied;
  allocation.gpu_address = allocator->allocation.gpu_address + allocator->occupied;
  allocation.size = size;
  allocation.type = CRUDE_GFX_MEMORY_TYPE_CPU_GPU;
  allocation.buffer_handle = allocator->allocation.buffer_handle;
  allocation.offset = allocator->occupied;

  allocator->occupied += size;

  return allocation;
}

uint64
crude_gfx_stack_allocator_get_marker
(
  _In_ crude_gfx_stack_allocator                          *allocator
)
{
  return allocator->occupied;
}

void
crude_gfx_stack_allocator_free_marker
(
  _In_ crude_gfx_stack_allocator                          *allocator,
  _In_ uint64                                              marker
)
{
  int64 difference = allocator->occupied - marker;
  if ( difference > 0u )
  {
    allocator->occupied = marker;
  }
}

void
crude_gfx_linear_allocator_initialize
(
  _In_ crude_gfx_linear_allocator                         *allocator,
  _In_ crude_gfx_device                                   *gpu,
  _In_ sizet                                               capacity,
  _In_ char const                                         *name
)
{
  allocator->gpu = gpu;
  allocator->allocation = crude_gfx_memory_allocate_with_name( gpu, capacity, CRUDE_GFX_MEMORY_TYPE_CPU_GPU, name );
  allocator->capacity = capacity;
  allocator->occupied = 0u;
  allocator->name = name;
  CRUDE_LOG_INFO( CRUDE_CHANNEL_MEMORY, "GPU Linear allocator of capacity %llu created", capacity );
}

void
crude_gfx_linear_allocator_deinitialize
(
  _In_ crude_gfx_linear_allocator                         *allocator
)
{
  CRUDE_ASSERTM( CRUDE_CHANNEL_MEMORY, allocator->occupied == 0u, "GPU Linear allocator \"%s\" shutdown. Allocated memory detected. Allocated %llu, total %llu", allocator->name, allocator->occupied, allocator->capacity );
  crude_gfx_memory_deallocate( allocator->gpu, allocator->allocation );
}

crude_gfx_memory_allocation
crude_gfx_linear_allocator_allocate
( 
  _In_ crude_gfx_linear_allocator                         *allocator,
  _In_ uint64                                              size
)
{
  crude_gfx_memory_allocation                              allocation;

  if ( allocator->occupied + size > allocator->capacity )
  {
    CRUDE_ABORT( CRUDE_CHANNEL_MEMORY, "GPU new memory block is too big for current linear allocator! %i occupied, %i requested size, %i capacity", allocator->occupied, size, allocator->capacity );
  }

  allocation = crude_gfx_memory_allocation_empty( );
  allocation.cpu_address = CRUDE_CAST( uint8*, allocator->allocation.cpu_address ) + allocator->occupied;
  allocation.gpu_address = allocator->allocation.gpu_address + allocator->occupied;
  allocation.size = size;
  allocation.type = CRUDE_GFX_MEMORY_TYPE_CPU_GPU;
  allocation.buffer_handle = allocator->allocation.buffer_handle;
  allocation.offset = allocator->occupied;

  allocator->occupied += size;

  return allocation;
}

void
crude_gfx_linear_allocator_clear
(
  _In_ crude_gfx_linear_allocator                         *allocator
)
{
  allocator->occupied = 0;
}

crude_gfx_memory_allocation
crude_gfx_memory_allocation_empty
(
)
{
  crude_gfx_memory_allocation empty = CRUDE_COMPOUNT_EMPTY( crude_gfx_memory_allocation );
  empty.buffer_handle = CRUDE_GFX_BUFFER_HANDLE_INVALID;
  return empty;
}

bool
crude_gfx_memory_allocation_valid
(
  _In_ crude_gfx_memory_allocation const                  *allocation
)
{
  return allocation->buffer_handle.index != CRUDE_GFX_BUFFER_HANDLE_INVALID.index;
}

crude_gfx_memory_allocation
crude_gfx_memory_allocate
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint64                                              size,
  _In_ crude_gfx_memory_type                               type
)
{
  return crude_gfx_memory_allocate_with_name( gpu, size, type, "default_allocated_buffer" );
}

crude_gfx_memory_allocation
crude_gfx_memory_allocate_with_name
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint64                                              size,
  _In_ crude_gfx_memory_type                               type,
  _In_ char const                                         *name
)
{
  crude_gfx_memory_allocation                              memory_allocation;
  crude_gfx_buffer_creation                                buffer_creation;

  memory_allocation = crude_gfx_memory_allocation_empty( );

  switch ( type )
  {
  case CRUDE_GFX_MEMORY_TYPE_CPU_GPU:
  {
    buffer_creation = crude_gfx_buffer_creation_empty();
    buffer_creation.name = name;
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = size;
    buffer_creation.persistent = true;

    memory_allocation.buffer_handle = crude_gfx_create_buffer( gpu, &buffer_creation );
    memory_allocation.gpu_address = crude_gfx_get_buffer_device_address( gpu, memory_allocation.buffer_handle );
    memory_allocation.cpu_address = crude_gfx_access_buffer( gpu, memory_allocation.buffer_handle )->mapped_data;
    memory_allocation.type = type;
    memory_allocation.size = size;
    break;
  }
  case CRUDE_GFX_MEMORY_TYPE_GPU:
  {
    buffer_creation = crude_gfx_buffer_creation_empty();
    buffer_creation.name = name;
    buffer_creation.type_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = size;
    buffer_creation.device_only = true;

    memory_allocation.buffer_handle = crude_gfx_create_buffer( gpu, &buffer_creation );
    memory_allocation.gpu_address = crude_gfx_get_buffer_device_address( gpu, memory_allocation.buffer_handle );
    memory_allocation.type = type;
    memory_allocation.size = size;
    break;
  }
  }

  return memory_allocation;
}

void
crude_gfx_memory_deallocate
(
  _In_ crude_gfx_device                            *gpu,
  _In_ crude_gfx_memory_allocation                  allocation
)
{
  crude_gfx_destroy_buffer( gpu, allocation.buffer_handle ); 
}