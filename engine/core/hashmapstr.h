#pragma once

/* yes, it's not optimized, bud it works, and i don't have any plans to fix it the near future */

#include <engine/core/core_config.h>
#include <engine/core/memory.h>
#include <engine/core/string.h>
#include <engine/core/assert.h>

#define CRUDE_HASHMAPSTR_BACKET_STATE_EMPTY ( 0 )
#define CRUDE_HASHMAPSTR_BACKET_STATE_REMOVED CRUDE_CAST( uint64, -1 )

typedef struct crude_hashmapstr
{
  uint64                                                   key_hash;
  crude_string_link                                        key;
} crude_hashmapstr;

typedef struct crude_hashmapstr_header
{
  uint32                                                   length;
  uint32                                                   capacity;
  crude_allocator_container                                allocator;
  uint64                                                   temp;
} crude_hashmapstr_header;

CRUDE_API uint64
crude_hashmapstr_backet_key_hash_valid
(
  _In_ uint64                                              key
);

CRUDE_API void*
crude_hashmapstr_growf
(
  _In_opt_ uint8                                          *h,
  _In_ size_t                                              elemsize,
  _In_ size_t                                              cap,
  _In_ crude_allocator_container                           allocator
);

CRUDE_API int64
crude_hashmapstr_get_index
(
  _In_ uint8                                              *h,
  _In_ char const                                         *key,
  _In_ size_t                                              elemsize
);

CRUDE_API void*
crude_hashmapstr_set_index
(
  _In_ uint8                                              *h,
  _In_ crude_string_link                                   key,
  _In_ size_t                                              elemsize
);

#define CRUDE_HASHMAPSTR( component ) struct { crude_hashmapstr key; component value; }
#define CRUDE_HASHMAPSTR_HEADER( h )  ( CRUDE_REINTERPRET_CAST( crude_hashmapstr_header*, h ) - 1 )
#define CRUDE_HASHMAPSTR_ALLOCATOR( h ) ( ( h ) ? CRUDE_HASHMAPSTR_HEADER( h )->allocator : CRUDE_COMPOUNT_EMPTY( crude_allocator_container ) )
#define CRUDE_HASHMAPSTR_CAPACITY( h )  ( CRUDE_HASHMAPSTR_HEADER( h )->capacity )
#define CRUDE_HASHMAPSTR_LENGTH( h )  ( CRUDE_HASHMAPSTR_HEADER( h )->length )
#define CRUDE_HASHMAPSTR_TEMP( h )  ( CRUDE_HASHMAPSTR_HEADER( h )->temp )

#define CRUDE_HASHMAPSTR_INITIALIZE( h, l ) ( ( h ) = NULL, ( h ) = CRUDE_REINTERPRET_CAST( CRUDE_TYPE( h ), crude_hashmapstr_growf( NULL, sizeof*( h ), CRUDE_CORE_HASHMAP_INITIAL_CAPACITY, ( l ) ) ) )
#define CRUDE_HASHMAPSTR_INITIALIZE_WITH_CAPACITY( h, c, l ) ( ( h ) = NULL, ( h ) = CRUDE_REINTERPRET_CAST( CRUDE_TYPE( h ), crude_hashmapstr_growf( NULL, sizeof*( h ), c, ( l ) ) ) )
#define CRUDE_HASHMAPSTR_DEINITIALIZE( h ) ( CRUDE_DEALLOCATE( CRUDE_HASHMAPSTR_ALLOCATOR( h ), CRUDE_HASHMAPSTR_HEADER( h ) ) )
#define CRUDE_HASHMAPSTR_GET_INDEX( h, k ) ( crude_hashmapstr_get_index( CRUDE_REINTERPRET_CAST( uint8*, h ), k, sizeof*( h ) ) )
#define CRUDE_HASHMAPSTR_GET( h, k ) ( (void)CRUDE_HASHMAPSTR_GET_INDEX( h, k ), ( CRUDE_HASHMAPSTR_TEMP( h ) == -1 ) ? NULL : &( h )[ CRUDE_HASHMAPSTR_TEMP( h ) ] )
#define CRUDE_HASHMAPSTR_SET( h, k, v ) ( ( h ) = CRUDE_REINTERPRET_CAST( CRUDE_TYPE( h ), crude_hashmapstr_set_index( CRUDE_REINTERPRET_CAST( uint8*, h ), k, sizeof*( h ) ) ), ( h )[ CRUDE_HASHMAPSTR_TEMP( h ) ].value = v )
#define CRUDE_HASHMAPSTR_REMOVE( h, k ) ( (void)CRUDE_HASHMAPSTR_GET_INDEX( h, k ), ( ( CRUDE_HASHMAPSTR_TEMP( h ) == -1 ) ? ( 0 ) : ( ( h )[ CRUDE_HASHMAPSTR_TEMP( h ) ].key.key_hash = CRUDE_HASHMAPSTR_BACKET_STATE_REMOVED, CRUDE_HASHMAPSTR_HEADER( h )->length-- ) ) )