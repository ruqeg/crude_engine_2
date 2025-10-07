/* okay I give up... directxmath is too good not to use */
#include <DirectXMath.h>
#include <math.h>

#include <core/alias.h>

using namespace DirectX;

#define CRUDE_FLOOR( a ) floor( a )
#define CRUDE_CEIL( a ) ceil( a )
#define CRUDE_MAX( a, b ) fmaxf( a, b )
#define CRUDE_MIN( a, b ) fminf( a, b )
#define CRUDE_CLAMP( x, u, l ) CRUDE_MIN( u, CRUDE_MAX( x, l ) )

CRUDE_API float32
crude_random_unit_f32
(
);