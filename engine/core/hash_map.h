#pragma once

/* yes, it's not optimized, bud it works, and i don't have any plans to fix it the near future */

#include <core/memory.h>
#include <core/assert.h>

#define CRUDE_HASHMAP_INITIAL_CAPACITY                     ( 16 )

typedef struct crude_hashmap_header
{
  uint32                                                   length;
  uint32                                                   capacity;
  crude_allocator_container                                allocator;
  uint64                                                   temp;
} crude_hashmap_header;

typedef enum crude_hashmap_backet_state
{
  CRUDE_HASHMAP_BACKET_STATE_EMPTY = 0,
  CRUDE_HASHMAP_BACKET_STATE_REMOVED = -1,
} crude_hashmap_backet_state;

CRUDE_API uint64
crude_hashmap_backet_key_valid
(
  _In_ uint64                                              key
);

CRUDE_API uint64
crude_hash_bytes
(
  _In_ uint8 const                                        *p,
  _In_ size_t                                              len,
  _In_ size_t                                              seed
);

CRUDE_API uint64
crude_hash_string
(
  _In_ char const                                          str[],
  _In_ size_t                                              seed
);

CRUDE_API void*
crude_hashmap_growf
(
  _In_opt_ uint8                                          *h,
  _In_ size_t                                              elemsize,
  _In_ size_t                                              cap,
  _In_ crude_allocator_container                           allocator
);

CRUDE_API int64
crude_hashmap_get_index
(
  _In_ uint8                                              *h,
  _In_ uint64                                              key,
  _In_ size_t                                              elemsize
);

CRUDE_API void*
crude_hashmap_set_index
(
  _In_ uint8                                              *h,
  _In_ uint64                                              key,
  _In_ size_t                                              elemsize
);

#define CRUDE_HASHMAP_HEADER( h )  ( CRUDE_REINTERPRET_CAST( crude_hashmap_header*, h ) - 1 )
#define CRUDE_HASHMAP_ALLOCATOR( h ) ( ( h ) ? CRUDE_HASHMAP_HEADER( h )->allocator : CRUDE_COMPOUNT_EMPTY( crude_allocator_container ) )
#define CRUDE_HASHMAP_CAPACITY( h )  ( CRUDE_HASHMAP_HEADER( h )->capacity )
#define CRUDE_HASHMAP_LENGTH( h )  ( CRUDE_HASHMAP_HEADER( h )->length )
#define CRUDE_HASHMAP_TEMP( h )  ( CRUDE_HASHMAP_HEADER( h )->temp )

#define CRUDE_HASHMAP_INITIALIZE( h, l ) ( ( h ) = NULL, ( h ) = CRUDE_REINTERPRET_CAST( CRUDE_TYPE( h ), crude_hashmap_growf( NULL, sizeof*( h ), CRUDE_HASHMAP_INITIAL_CAPACITY, ( l ) ) ) )
#define CRUDE_HASHMAP_INITIALIZE_WITH_CAPACITY( h, c, l ) ( ( h ) = NULL, ( h ) = CRUDE_REINTERPRET_CAST( CRUDE_TYPE( h ), crude_hashmap_growf( NULL, sizeof*( h ), c, ( l ) ) ) )
#define CRUDE_HASHMAP_DEINITIALIZE( h ) ( CRUDE_DEALLOCATE( CRUDE_HASHMAP_ALLOCATOR( h ), CRUDE_HASHMAP_HEADER( h ) ) )
#define CRUDE_HASHMAP_GET_INDEX( h, k ) ( crude_hashmap_get_index( CRUDE_REINTERPRET_CAST( uint8*, h ), k, sizeof*( h ) ) )
#define CRUDE_HASHMAP_GET( h, k ) ( (void)CRUDE_HASHMAP_GET_INDEX( h, k ), ( CRUDE_HASHMAP_TEMP( h ) == -1 ) ? NULL : &( h )[ CRUDE_HASHMAP_TEMP( h ) ] )
#define CRUDE_HASHMAP_SET( h, k, v ) ( ( h ) = CRUDE_REINTERPRET_CAST( CRUDE_TYPE( h ), crude_hashmap_set_index( CRUDE_REINTERPRET_CAST( uint8*, h ), k, sizeof*( h ) ) ), ( h )[ CRUDE_HASHMAP_TEMP( h ) ].value = v )
#define CRUDE_HASHMAP_REMOVE( h, k ) ( (void)CRUDE_HASHMAP_GET_INDEX( h, k ), ( CRUDE_HASHMAP_TEMP( h ) == -1 ) ? ( 0 ) : ( ( h )[ CRUDE_HASHMAP_TEMP( h ) ].key = CRUDE_HASHMAP_BACKET_STATE_REMOVED, CRUDE_HASHMAP_HEADER( h )->length-- ) )