#pragma once

#define CAST( t, exp ) ( ( t ) ( exp ) )
#define ARRAY_SIZE( arr ) ( sizeof( arr ) / sizeof( arr[0] ) )