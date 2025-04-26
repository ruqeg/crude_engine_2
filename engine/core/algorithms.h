#pragma once

#include <stb_ds.h>

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