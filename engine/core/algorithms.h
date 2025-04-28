#pragma once

void*
_crude_array_realloc
(
  _In_ void                                               *ptr,
  _In_ uint32_t                                            size
);

void
_crude_array_dealloc
(
  void                                               *ptr
);

#define STBDS_REALLOC( c, ptr, size ) _crude_array_realloc( ptr, size )
#define STBDS_FREE( c, ptr ) _crude_array_dealloc( ptr )
#include <stb_ds.h>

#include <core/memory.h>

CRUDE_API void
crude_array_set_allocator
(
  _In_ crude_heap_allocator                               *allocator
);

/************************************************
 *
 * Abstraction over STB_DS
 * 
 ***********************************************/
#define CRUDE_ARR_SETLEN( arr, len )     arrsetlen( arr, len )
#define CRUDE_ARR_LEN( arr )             arrlen( arr )
#define CRUDE_ARR_FREE( arr )            arrfree( arr )
#define CRUDE_ARR_PUSH( arr, value )     arrpush( arr, value )
#define CRUDE_ARR_SETCAP( arr, cap )     arrsetcap( arr, cap )
#define CRUDE_ARR_PUT( arr, value )      arrput( arr, value )
#define CRUDE_ARR_DELSWAP( arr, index )  arrdelswap( arr, index )
#define CRUDE_ARR_POP( arr )             arrpop( arr )

#define CRUDE_ARR( type )                type*