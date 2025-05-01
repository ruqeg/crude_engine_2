#pragma once

#include <core/memory.h>

typedef struct crude_array_header
{
  uint32                                                   length;
  uint32                                                   capacity;
  crude_allocator_container                                allocator;
} crude_array_header;

CRUDE_API void*
crude_array_growf
(
  _In_opt_ void                                           *a,
  _In_ size_t                                              elemsize,
  _In_ size_t                                              addlen,
  _In_ size_t                                              min_cap,
  _In_ crude_allocator_container                           allocator
);

#define CRUDE_ARRAY_HEADER( a )  ( ( crude_array_header* ) ( a ) - 1 )
#define CRUDE_ARRAY_ARRAY_GROW( a, b, c, d ) ( ( a ) = crude_array_growf( ( a ), sizeof *( a ), ( b ), ( c ), ( d ) ) )
#define CRUDE_ARRAY_MAYBE_GROW( a, n ) ( ( CRUDE_ARRAY_HEADER( a )->length + ( n ) > CRUDE_ARRAY_HEADER( a )->capacity ) ? ( CRUDE_ARRAY_ARRAY_GROW( a, n, 0, CRUDE_ARRAY_HEADER( a )->allocator ), 0 ) : 0 )

#define CRUDE_ARRAY_ALLOCATOR( a ) ( ( a ) ? CRUDE_ARRAY_HEADER( a )->allocator : ( crude_allocator_container ) { 0 } )
#define CRUDE_ARRAY_CAPACITY( a ) ( ( a ) ? CRUDE_ARRAY_HEADER( a )->capacity : 0 )
#define CRUDE_ARRAY_LENGTH( a ) ( ( a ) ? CRUDE_ARRAY_HEADER( a )->length : 0 )
#define CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( a, c, l )( ( a ) = NULL, CRUDE_ARRAY_ARRAY_GROW( a, 0, c, l ) )
#define CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( a, s, l )( ( a ) = NULL, CRUDE_ARRAY_ARRAY_GROW( a, s, s, l ) )
#define CRUDE_ARRAY_FREE( a ) ( ( void ) ( ( a ) ? CRUDE_DEALLOCATE( CRUDE_ARRAY_ALLOCATOR( a ), CRUDE_ARRAY_HEADER( a ) ) : ( void )0 ), ( a ) = NULL )
#define CRUDE_ARRAY_SET_CAPACITY( a, n ) ( CRUDE_ARRAY_ARRAY_GROW( a, 0, n, CRUDE_ARRAY_ALLOCATOR( a ) ) )
#define CRUDE_ARRAY_SET_LENGTH( a, n ) ( ( CRUDE_ARRAY_CAPACITY( a ) < ( size_t ) ( n ) ? CRUDE_ARRAY_SET_CAPACITY( ( a ), ( size_t )( n ) ), 0 : 0 ), ( a ) ? CRUDE_ARRAY_HEADER( a )->length = ( size_t )( n ) : 0 )
#define CRUDE_ARRAY_PUSH( a, v ) ( CRUDE_ARRAY_MAYBE_GROW( a, 1 ), ( a )[ CRUDE_ARRAY_HEADER( a )->length++ ] = ( v ) )
#define CRUDE_ARRAY_LAST( a ) ( ( a )[ CRUDE_ARRAY_HEADER( a )->length - 1 ] )
#define CRUDE_ARRAY_DELSWAP( a, i ) ( ( a )[ i ] = CRUDE_ARRAY_LAST( a ), CRUDE_ARRAY_HEADER( a )->length -= 1 )
#define CRUDE_ARRAY_POP( a ) ( CRUDE_ARRAY_HEADER( a )->length--, ( a )[ CRUDE_ARRAY_HEADER( a )->length ] )