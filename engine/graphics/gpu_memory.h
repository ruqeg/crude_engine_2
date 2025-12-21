#pragma once

typedef struct crude_gfx_device crude_gfx_device;

typedef enum crude_gfx_memory_type
{
  CRUDE_GFX_MEMORY_TYPE_NONE,
  CRUDE_GFX_MEMORY_TYPE_CPU_GPU,
  CRUDE_GFX_MEMORY_TYPE_GPU
} crude_gfx_memory_type;

typedef struct crude_gfx_memory_allocation
{
  void                                                    *cpu_address;
  VkDeviceAddress                                          gpu_address;
  crude_gfx_buffer_handle                                  buffer_handle;
  crude_gfx_memory_type                                    type;
  uint64                                                   size;
  uint64                                                   offset;
} crude_gfx_memory_allocation;

typedef struct crude_gfx_stack_allocator
{
  crude_gfx_device                                        *gpu;
  crude_gfx_memory_allocation                              allocation;
  sizet                                                    capacity;
  sizet                                                    occupied;
  char const                                              *name;
} crude_gfx_stack_allocator;

CRUDE_API void
crude_gfx_stack_allocator_initialize
(
  _In_ crude_gfx_stack_allocator                          *allocator,
  _In_ crude_gfx_device                                   *gpu,
  _In_ sizet                                               capacity,
  _In_ char const                                         *name
);

CRUDE_API void
crude_gfx_stack_allocator_deinitialize
(
  _In_ crude_gfx_stack_allocator                          *allocator
);

CRUDE_API crude_gfx_memory_allocation
crude_gfx_stack_allocator_allocate
( 
  _In_ crude_gfx_stack_allocator                          *allocator,
  _In_ uint64                                              size
);

CRUDE_API uint64
crude_gfx_stack_allocator_get_marker
(
  _In_ crude_gfx_stack_allocator                          *allocator
);

CRUDE_API void
crude_gfx_stack_allocator_free_marker
(
  _In_ crude_gfx_stack_allocator                          *allocator,
  _In_ uint64                                              marker
);

CRUDE_API crude_gfx_memory_allocation
crude_gfx_memory_allocation_empty
(
);

CRUDE_API bool
crude_gfx_memory_allocation_valid
(
  _In_ crude_gfx_memory_allocation const                  *allocation
);

CRUDE_API crude_gfx_memory_allocation
crude_gfx_memory_allocate
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint64                                              size,
  _In_ crude_gfx_memory_type                               type
);

CRUDE_API void
crude_gfx_memory_deallocate
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_memory_allocation                         allocation
);

CRUDE_API crude_gfx_memory_allocation
crude_gfx_memory_allocate_with_name
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint64                                              size,
  _In_ crude_gfx_memory_type                               type,
  _In_ char const                                         *name
);