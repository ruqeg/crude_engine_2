#include <string.h>

#include <engine/core/hash_map.h>

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

typedef struct hash_backet
{
  uint64                                                   key;
} hash_backet;

static uint64
key_to_index_
(
  _In_ uint8                                              *h,
  _In_ uint64                                              key
);

uint64
crude_hashmap_backet_key_valid
(
  _In_ uint64                                              key
)
{
  return key != CRUDE_HASHMAP_BACKET_STATE_EMPTY && key != CRUDE_HASHMAP_BACKET_STATE_REMOVED;
}

uint64
crude_hash_bytes
(
  _In_ uint8 const                                        *p,
  _In_ size_t                                              len,
  _In_ size_t                                              seed
)
{
  uint64 hash = FNV_OFFSET;
  for ( uint32 i = 0; i < len; ++i )
  {
    hash ^= ( uint64 )( unsigned char )( p[ i ] );
    hash *= FNV_PRIME;
  }

  if ( !crude_hashmap_backet_key_valid( hash ) )
  {
    hash = 1;
  }

  return hash;
}

uint64
crude_hash_string
(
  _In_ char const                                         *str,
  _In_ size_t                                              seed
)
{
  return crude_hash_bytes( CRUDE_REINTERPRET_CAST( uint8 const*, str ), strlen( str ), 0 );
}


CRUDE_API uint64
crude_hash_bytes
(
  _In_ uint8 const                                        *p,
  _In_ size_t                                              len,
  _In_ size_t                                              seed
);


void*
crude_hashmap_growf
(
  _In_opt_ uint8                                          *h,
  _In_ size_t                                              elemsize,
  _In_ size_t                                              cap,
  _In_ crude_allocator_container                           allocator
)
{
  uint8                                                   *nh;
  
  nh = CRUDE_REINTERPRET_CAST( uint8*, CRUDE_ALLOCATE( allocator, sizeof( crude_hashmap_header ) + elemsize * cap ) );
  nh = nh + sizeof( crude_hashmap_header );
  CRUDE_HASHMAP_HEADER( nh )->capacity = cap;
  CRUDE_HASHMAP_HEADER( nh )->allocator = allocator;
  CRUDE_HASHMAP_HEADER( nh )->length = 0;
  crude_memory_set( nh, 0, elemsize * cap ); /* fill with CRUDE_HASHMAP_BACKET_STATE_EMPTY*/

  if ( h )
  {
    for ( size_t i = 0; i < CRUDE_HASHMAP_CAPACITY( h ); i++ )
    {
      hash_backet *backet = CRUDE_REINTERPRET_CAST( hash_backet*, h + i * elemsize );
      if ( crude_hashmap_backet_key_valid( backet->key ) )
      {
         /* Hashmap shouldn't be resizes here since it's already resized based on the previous hashmap */
        CRUDE_ASSERT( nh == crude_hashmap_set_index( nh, backet->key, elemsize ) );
        uint64 temp = CRUDE_HASHMAP_TEMP( nh );
        crude_memory_copy( nh + CRUDE_HASHMAP_TEMP( nh ) * elemsize, backet, elemsize );
      }
    }
    CRUDE_DEALLOCATE( CRUDE_HASHMAP_ALLOCATOR( h ), h - sizeof( crude_hashmap_header ) );
  }
  
  return nh;
}

int64
crude_hashmap_get_index
(
  _In_ uint8                                              *h,
  _In_ uint64                                              key,
  _In_ size_t                                              elemsize
)
{
  hash_backet                                             *backet;
  int64                                                    index;

  index = key_to_index_( h, key );
  backet = CRUDE_REINTERPRET_CAST( hash_backet*, h + elemsize * index );
  while ( backet->key != CRUDE_HASHMAP_BACKET_STATE_EMPTY )
  {
    if ( key == backet->key )
    {
      CRUDE_HASHMAP_HEADER( h )->temp = index;
      return index;
    }
    
    index = ( index + 1 ) % CRUDE_HASHMAP_CAPACITY( h );
    backet = CRUDE_REINTERPRET_CAST( hash_backet*, h + elemsize * index );
  }
  
  CRUDE_HASHMAP_HEADER( h )->temp = -1;
  return -1;
}

void*
crude_hashmap_set_index
(
  _In_ uint8                                              *h,
  _In_ uint64                                              key,
  _In_ size_t                                              elemsize
)
{
  uint8                                                   *nh;
  hash_backet                                             *backet;
  int64                                                    index;

  if ( CRUDE_HASHMAP_LENGTH( h ) >= CRUDE_HASHMAP_CAPACITY( h ) / 2 )
  {
    nh = CRUDE_REINTERPRET_CAST( uint8*, crude_hashmap_growf( h, elemsize, CRUDE_HASHMAP_CAPACITY( h ) * 2, CRUDE_HASHMAP_ALLOCATOR( h ) ) );
  }
  else
  {
    nh = h;
  }
  uint64 length = CRUDE_HASHMAP_LENGTH( nh );
  uint64 cappppp = CRUDE_HASHMAP_CAPACITY( nh );
  index = key_to_index_( nh, key );
  backet = CRUDE_REINTERPRET_CAST( hash_backet*, nh + elemsize * index );
  while ( backet->key != CRUDE_HASHMAP_BACKET_STATE_EMPTY && backet->key != CRUDE_HASHMAP_BACKET_STATE_REMOVED )
  {
    if ( key == backet->key ) /* fuck "removed" backet, loser backet will be replaced by MY HAND HAHAHAH 0W0 */
    {
      CRUDE_HASHMAP_HEADER( nh )->temp = index;
      return nh;
    }

    index = ( index + 1 ) % CRUDE_HASHMAP_CAPACITY( nh );
    backet = CRUDE_REINTERPRET_CAST( hash_backet*, nh + elemsize * index );
  }

  CRUDE_HASHMAP_HEADER( nh )->length = CRUDE_HASHMAP_HEADER( nh )->length + 1;
  
  backet->key = key;
  CRUDE_HASHMAP_HEADER( nh )->temp = index;
  return nh;
}

uint64
key_to_index_
(
  _In_ uint8                                              *h,
  _In_ uint64                                              key
)
{
  return ( uint64 )( key & ( uint64 )( CRUDE_HASHMAP_CAPACITY( h ) - 1 ) );
}