#include <string.h>

#include <engine/core/string.h>

#include <engine/core/hashmapstr.h>

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

static uint64
key_hash_to_index_
(
  _In_ uint8                                              *h,
  _In_ uint64                                              key_hash
);

uint64
crude_hashmapstr_backet_key_hash_valid
(
  _In_ uint64                                              key_hash
)
{
  return key_hash != CRUDE_HASHMAPSTR_BACKET_STATE_EMPTY && key_hash != CRUDE_HASHMAPSTR_BACKET_STATE_REMOVED;
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

  if ( !crude_hashmapstr_backet_key_hash_valid( hash ) )
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

void*
crude_hashmapstr_growf
(
  _In_opt_ uint8                                          *h,
  _In_ size_t                                              elemsize,
  _In_ size_t                                              cap,
  _In_ crude_allocator_container                           allocator
)
{
  uint8                                                   *nh;
  
  nh = CRUDE_REINTERPRET_CAST( uint8*, CRUDE_ALLOCATE( allocator, sizeof( crude_hashmapstr_header ) + elemsize * cap ) );
  nh = nh + sizeof( crude_hashmapstr_header );
  CRUDE_HASHMAPSTR_HEADER( nh )->capacity = cap;
  CRUDE_HASHMAPSTR_HEADER( nh )->allocator = allocator;
  CRUDE_HASHMAPSTR_HEADER( nh )->length = 0;
  crude_memory_set( nh, 0, elemsize * cap ); /* fill with CRUDE_HASHMAP_BACKET_STATE_EMPTY*/

  if ( h )
  {
    for ( size_t i = 0; i < CRUDE_HASHMAPSTR_CAPACITY( h ); i++ )
    {
      crude_hashmapstr *backet = CRUDE_REINTERPRET_CAST( crude_hashmapstr*, h + i * elemsize );
      if ( crude_hashmapstr_backet_key_hash_valid( backet->key_hash ) )
      {
         /* Hashmap shouldn't be resizes here since it's already resized based on the previous hashmap */
        CRUDE_ASSERT( nh == crude_hashmapstr_set_index( nh, backet->key, elemsize ) );
        uint64 temp = CRUDE_HASHMAPSTR_TEMP( nh );
        crude_memory_copy( nh + CRUDE_HASHMAPSTR_TEMP( nh ) * elemsize, backet, elemsize );
      }
    }
    CRUDE_DEALLOCATE( CRUDE_HASHMAPSTR_ALLOCATOR( h ), h - sizeof( crude_hashmapstr_header ) );
  }
  
  return nh;
}

int64
crude_hashmapstr_get_index
(
  _In_ uint8                                              *h,
  _In_ char const                                         *key,
  _In_ size_t                                              elemsize
)
{
  crude_hashmapstr                                        *backet;
  int64                                                    index;
  int64                                                    key_hash;

  key_hash = crude_hash_string( key, 0 );
  index = key_hash_to_index_( h, key_hash );
  backet = CRUDE_REINTERPRET_CAST( crude_hashmapstr*, h + elemsize * index );
  while ( backet->key_hash != CRUDE_HASHMAPSTR_BACKET_STATE_EMPTY )
  {
    if ( key_hash == backet->key_hash )
    {
      /* i can use && on top, but naaaaah */
      if ( crude_string_cmp( key, backet->key.data ) == 0 )
      {
        CRUDE_HASHMAPSTR_HEADER( h )->temp = index;
        return index;
      }
    }
    
    index = ( index + 1 ) % CRUDE_HASHMAPSTR_CAPACITY( h );
    backet = CRUDE_REINTERPRET_CAST( crude_hashmapstr*, h + elemsize * index );
  }
  
  CRUDE_HASHMAPSTR_HEADER( h )->temp = -1;
  return -1;
}

void*
crude_hashmapstr_set_index
(
  _In_ uint8                                              *h,
  _In_ crude_string_link                                   key,
  _In_ size_t                                              elemsize
)
{
  uint8                                                   *nh;
  crude_hashmapstr                                        *backet;
  int64                                                    index;
  int64                                                    key_hash;

  key_hash = crude_hash_string( key.data, 0 );

  if ( CRUDE_HASHMAPSTR_LENGTH( h ) >= CRUDE_HASHMAPSTR_CAPACITY( h ) / 2 )
  {
    nh = CRUDE_REINTERPRET_CAST( uint8*, crude_hashmapstr_growf( h, elemsize, CRUDE_HASHMAPSTR_CAPACITY( h ) * 2, CRUDE_HASHMAPSTR_ALLOCATOR( h ) ) );
  }
  else
  {
    nh = h;
  }
  
  index = key_hash_to_index_( nh, key_hash );
  backet = CRUDE_REINTERPRET_CAST( crude_hashmapstr*, nh + elemsize * index );
  while ( backet->key_hash != CRUDE_HASHMAPSTR_BACKET_STATE_EMPTY && backet->key_hash != CRUDE_HASHMAPSTR_BACKET_STATE_REMOVED )
  {
    if ( key_hash == backet->key_hash )
    {
      /* i can use && on top, but naaaaah */
      if ( crude_string_cmp( key.data, backet->key.data ) == 0 )
      {
         /* fuck "removed" backet, loser backet will be replaced by MY HAND HAHAHAH 0W0 */
        CRUDE_HASHMAPSTR_HEADER( nh )->temp = index;
        return nh;
      }
    }

    index = ( index + 1 ) % CRUDE_HASHMAPSTR_CAPACITY( nh );
    backet = CRUDE_REINTERPRET_CAST( crude_hashmapstr*, nh + elemsize * index );
  }

  CRUDE_HASHMAPSTR_HEADER( nh )->length = CRUDE_HASHMAPSTR_HEADER( nh )->length + 1;
  
  backet->key = key;
  backet->key_hash = key_hash;
  CRUDE_HASHMAPSTR_HEADER( nh )->temp = index;
  return nh;
}

uint64
key_hash_to_index_
(
  _In_ uint8                                              *h,
  _In_ uint64                                              key_hash
)
{
  return ( uint64 )( key_hash & ( uint64 )( CRUDE_HASHMAPSTR_CAPACITY( h ) - 1 ) );
}