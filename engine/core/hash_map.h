#pragma once

#include <core/memory.h>

#define CRUDE_HASHMAP_INITIAL_CAPACITY                     ( 16 )

typedef struct crude_hashmap_header
{
  uint32                                                   length;
  uint32                                                   capacity;
  crude_allocator_container                                allocator;
  uint64                                                   temp;
} crude_hashmap_header;

CRUDE_API uint64
crude_hash_bytes
(
  _In_ uint8                                              *p,
  _In_ size_t                                              len,
  _In_ size_t                                              seed
);

CRUDE_API uint8*
crude_hashmap_growf
(
  _In_opt_ int8                                           *h,
  _In_ size_t                                              elemsize,
  _In_ size_t                                              cap,
  _In_ crude_allocator_container                           allocator
);

CRUDE_API int64
crude_hashmap_get_index
(
  _In_ int8                                               *h,
  _In_ uint64                                              key,
  _In_ size_t                                              elemsize
);

CRUDE_API int64
crude_hashmap_set_index
(
  _In_ int8                                               *h,
  _In_ uint64                                              key,
  _In_ size_t                                              elemsize
);

#define CRUDE_HASHMAP_HEADER( h )  ( ( crude_hashmap_header* ) ( h ) - 1 )
#define CRUDE_HASHMAP_ALLOCATOR( h ) ( ( h ) ? CRUDE_HASHMAP_HEADER( h )->allocator : ( crude_allocator_container ) { 0 } )
#define CRUDE_HASHMAP_CAPACITY( h )  ( CRUDE_HASHMAP_HEADER( h )->capacity )
#define CRUDE_HASHMAP_LENGTH( h )  ( CRUDE_HASHMAP_HEADER( h )->length )
#define CRUDE_HASHMAP_TEMP( h )  ( CRUDE_HASHMAP_HEADER( h )->temp )

#define CRUDE_HASHMAP_INITIALIZE( h, l ) ( ( h ) = NULL, ( h ) = crude_hashmap_growf( NULL, sizeof*( h ), CRUDE_HASHMAP_INITIAL_CAPACITY, ( l ) ) )
#define CRUDE_HASHMAP_DEINITIALIZE( h ) ( CRUDE_DEALLOCATE( CRUDE_HASHMAP_ALLOCATOR( h ), CRUDE_HASHMAP_HEADER( h ) ) )
#define CRUDE_HASHMAP_GET_INDEX( h, k ) ( crude_hashmap_get_index( h, k, sizeof*( h ) ) )
#define CRUDE_HASHMAP_GET( h, k ) ( (void)CRUDE_HASHMAP_GET_INDEX( h, k ), &( h )[ CRUDE_HASHMAP_TEMP( h ) ] )
#define CRUDE_HASHMAP_SET( h, k, v ) ( crude_hashmap_set_index( h, k, sizeof*( h ) ), ( h )[ CRUDE_HASHMAP_TEMP( h ) ].value = v )