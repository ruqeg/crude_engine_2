#include <core/array.h>

void*
crude_array_growf
(
  _In_opt_ void                                           *a,
  _In_ size_t                                              elemsize,
  _In_ size_t                                              addlen,
  _In_ size_t                                              min_cap,
  _In_ crude_allocator_container                           allocator
)
{
  void                                                    *b;
  size_t                                                   min_len;

  min_len = CRUDE_ARRAY_LENGTH( a ) + addlen;

  if ( min_len > min_cap )
  {
    min_cap = min_len;
  }

  if ( min_cap <= CRUDE_ARRAY_CAPACITY( a ) )
  {
    return a;
  }

  if ( min_cap < 2 * CRUDE_ARRAY_CAPACITY( a ) )
  {
    min_cap = 2 * CRUDE_ARRAY_CAPACITY( a );
  }
  else if ( min_cap < 4 )
  {
    min_cap = 4;
  }

  b = CRUDE_REALLOCATE( allocator, ( a ) ? CRUDE_ARRAY_HEADER( a ) : NULL, elemsize * min_cap + sizeof( crude_array_header ) );
  b = ( char* ) b + sizeof( crude_array_header );

  if ( a == NULL )
  {
    CRUDE_ARRAY_HEADER( b )->length = 0;
  }
  CRUDE_ARRAY_HEADER( b )->capacity = min_cap;
  CRUDE_ARRAY_HEADER( b )->allocator = allocator;

  return b;
}