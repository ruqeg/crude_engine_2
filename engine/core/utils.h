#pragma once

#define CRUDE_CAST( t, exp ) ( ( t ) ( exp ) )
#define CRUDE_ARRAY_SIZE( arr ) ( sizeof( arr ) / sizeof( arr[0] ) )