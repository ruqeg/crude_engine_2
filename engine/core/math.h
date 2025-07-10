/* okay I give up... directxmath is too good not to use */
#include <DirectXMath.h>
#include <math.h>

using namespace DirectX;

#define crude_ceil( a ) ceil( a )
#define crude_max( a, b ) fmaxf( a, b )
#define crude_min( a, b ) fminf( a, b )
#define crude_clamp( x, u, l ) crude_min( u, crude_max( x, l ) )