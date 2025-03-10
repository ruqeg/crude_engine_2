#include <xmmintrin.h>
#include <emmintrin.h>

#include <core/alias.h>

/////////////////////
//// @Structs
/////////////////////

#if defined(_M_IX86) || defined(_M_X64)
typedef __m128 crude_vector;
#else
#error "!TODO"
#endif

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_vector_f32
{
  union
  {
    float32                         f[ 4 ];
    crude_vector                    v;
  };
} crude_vector_f32;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_vector_u32
{
  union
  {
    uint32                          u[ 4 ];
    crude_vector                    v;
  };
} crude_vector_u32;

typedef struct crude_matrix
{
  crude_vector                      r[ 4 ];
} crude_matrix;

typedef struct crude_float1
{
  float32                           x;
} crude_float1;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_float1a
{
  float32                           x;
} crude_float1a;

typedef struct crude_float2
{
  float32                           x;
  float32                           y;
} crude_float2;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_float2a
{
  float32                           x;
  float32                           y;
} crude_float2a;

typedef struct crude_float3
{
  float32                           x;
  float32                           y;
  float32                           z;
} crude_float3;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_float3a
{
  float32                           x;
  float32                           y;
  float32                           z;
} crude_float3a;

typedef struct crude_float4
{
  float32                           x;
  float32                           y;
  float32                           z;
  float32                           w;
} crude_float4;

typedef CRUDE_ALIGNED_STRUCT( 16 ) crude_float4a
{
  float32                           x;
  float32                           y;
  float32                           z;
  float32                           w;
} crude_float4a;

typedef struct crude_float4x4
{
  union
  {
    struct
    {
      float32                       _00;
      float32                       _01;
      float32                       _02;
      float32                       _03;
      float32                       _10;
      float32                       _11;
      float32                       _12;
      float32                       _13;
      float32                       _20;
      float32                       _21;
      float32                       _22;
      float32                       _23;
      float32                       _30;
      float32                       _31;
      float32                       _32;
      float32                       _33;
    };
    float32                         m[ 4 ][ 4 ];
  };
} crude_float4x4;

typedef CRUDE_ALIGNED_STRUCT( 64 ) crude_float4x4a
{
  union
  {
    struct
    {
      float32                       _00;
      float32                       _01;
      float32                       _02;
      float32                       _03;
      float32                       _10;
      float32                       _11;
      float32                       _12;
      float32                       _13;
      float32                       _20;
      float32                       _21;
      float32                       _22;
      float32                       _23;
      float32                       _30;
      float32                       _31;
      float32                       _32;
      float32                       _33;
    };
    float32                         m[ 4 ][ 4 ];
  };
} crude_float4x4a;

// !TODO INT, UINT, FLOAT?XY

/////////////////////
//// @Constants
/////////////////////
#define CRUDE_MATH_C2PI             6.2831853f
#define CRUDE_MATH_CPI              3.1415927f
#define CRUDE_MATH_CPI2             1.5707964f
#define CRUDE_MATH_CPI4             0.7853982f
#define CRUDE_MATH_C1DIVPI          0.31830987f
#define CRUDE_MATH_C1DIV2PI         0.15915494f
#define CRUDE_MATH_C2DIVPI          0.63661976f
#define CRUDE_MATH_CMAXF32          3.40282e+38
#define CRUDE_MATH_CMINF32          1.17549e-38

#define CRUDE_MATH_SELECT0          0x00000000
#define CRUDE_MATH_SELECT1          0xFFFFFFFF
#define CRUDE_MATH_SELECT_1110      CAST( crude_vector_u32, { { { CRUDE_MATH_SELECT1, CRUDE_MATH_SELECT1, CRUDE_MATH_SELECT1, CRUDE_MATH_SELECT0 } } } )

/////////////////////
//// @Scalar
/////////////////////
CRUDE_API float32                   crude_max( _In_ float32 s1, _In_ float32 s2 );
CRUDE_API float32                   crude_min( _In_ float32 s1, _In_ float32 s2 );
CRUDE_API float32                   crude_round( _In_ float32 s );
CRUDE_API float32                   crude_floor( _In_ float32 s );
CRUDE_API float32                   crude_ceil( _In_ float32 s );
CRUDE_API float32                   crude_trunc( _In_ float32 s );
CRUDE_API float32                   crude_clamp( _In_ float32 s, _In_ float32 min, _In_ float32 max );
CRUDE_API float32                   crude_abs( _In_ float32 s );

CRUDE_API float32                   crude_pow( _In_ float32 s1, _In_ float32 s2 );
CRUDE_API float32                   crude_sqrt( _In_ float32 s);

CRUDE_API float32                   crude_exp2( _In_ float32 s );
CRUDE_API float32                   crude_exp( _In_ float32 s );

CRUDE_API float32                   crude_log2( _In_ float32 s );
CRUDE_API float32                   crude_log10( _In_ float32 s );
CRUDE_API float32                   crude_log( _In_ float32 s );

CRUDE_API float32                   crude_sin( _In_ float32 s );
CRUDE_API float32                   crude_cos( _In_ float32 s );
CRUDE_API float32                   crude_tan( _In_ float32 s );
CRUDE_API float32                   crude_arc_sin( _In_ float32 s );
CRUDE_API float32                   crude_arc_cos( _In_ float32 s );
CRUDE_API float32                   crude_arc_tan( _In_ float32 s );

/////////////////////
//// @Vector int
/////////////////////
CRUDE_API crude_vector              crude_vec_set_int( _In_ uint32 x, _In_ uint32 y, _In_ uint32 z, _In_ uint32 w);
CRUDE_API crude_vector              crude_vec_fill_int( _In_ uint32 value );
CRUDE_API crude_vector              crude_vec_true_int( );
CRUDE_API crude_vector              crude_vec_false_int( );

CRUDE_API uint32                    crude_vec_get_int_x( _In_ crude_vector const *v );
CRUDE_API uint32                    crude_vec_get_int_y( _In_ crude_vector const *v );
CRUDE_API uint32                    crude_vec_get_int_z( _In_ crude_vector const *v );
CRUDE_API uint32                    crude_vec_get_int_w( _In_ crude_vector const *v );

CRUDE_API crude_vector              crude_vec_set_int_x( _In_ crude_vector const *v, _In_ uint32 x );
CRUDE_API crude_vector              crude_vec_set_int_y( _In_ crude_vector const *v, _In_ uint32 y );
CRUDE_API crude_vector              crude_vec_set_int_z( _In_ crude_vector const *v, _In_ uint32 z );
CRUDE_API crude_vector              crude_vec_set_int_w( _In_ crude_vector const *v, _In_ uint32 w );

CRUDE_API crude_vector              crude_vec_equal_int( _In_ crude_vector const *v1, _In_ crude_vector const * v2 );
CRUDE_API crude_vector              crude_vec_not_equal_int( _In_ crude_vector const *v1, _In_ crude_vector const * v2 );

CRUDE_API crude_vector              crude_vec_and_int( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_or_int( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_nor_int( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_xor_int( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );

/////////////////////
//// @Vector Common
/////////////////////
CRUDE_API crude_vector              crude_vec_select( _In_ crude_vector const *v1, _In_ crude_vector const *v2, _In_ crude_vector const *control );
CRUDE_API crude_vector              crude_vec_zero( );
CRUDE_API crude_vector              crude_vec_set( _In_ float32 x, _In_ float32 y, _In_ float32 z, _In_ float32 w );
CRUDE_API crude_vector              crude_vec_fill( _In_ float32 value );
CRUDE_API crude_vector              crude_vec_splat_x( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_splat_y( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_splat_z( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_splat_w( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_splat_one( );
CRUDE_API crude_vector              crude_vec_splat_infinity( );
CRUDE_API crude_vector              crude_vec_splat_qnan( );
CRUDE_API crude_vector              crude_vec_splat_epsilon( );

CRUDE_API float32                   crude_vec_get_x( _In_ crude_vector const *v );
CRUDE_API float32                   crude_vec_get_y( _In_ crude_vector const *v );
CRUDE_API float32                   crude_vec_get_z( _In_ crude_vector const *v );
CRUDE_API float32                   crude_vec_get_w( _In_ crude_vector const *v );

CRUDE_API crude_vector              crude_vec_set_x( _In_ crude_vector const *v, float32 x );
CRUDE_API crude_vector              crude_vec_set_y( _In_ crude_vector const *v, float32 y );
CRUDE_API crude_vector              crude_vec_set_z( _In_ crude_vector const *v, float32 z );
CRUDE_API crude_vector              crude_vec_set_w( _In_ crude_vector const *v, float32 w );

CRUDE_API crude_vector              crude_vec_equal( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_near_equal( _In_ crude_vector const *v1, _In_ crude_vector const *v2, _In_ crude_vector const *vepsilon );
CRUDE_API crude_vector              crude_vec_not_equal( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_greater( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_greater_or_equal( _In_ crude_vector const * v1, crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_less( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_less_or_equal( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_in_bounds( _In_ crude_vector const *v, _In_ crude_vector const *vbounds );

CRUDE_API crude_vector              crude_vec_is_nan( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_is_infinite( _In_ crude_vector const *v );

CRUDE_API crude_vector              crude_vec_min( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_max( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_round( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_floor( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_ceil( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_trunc( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_clamp( _In_ crude_vector const *v, crude_vector const *vmin, crude_vector const *vmax );

CRUDE_API crude_vector              crude_vec_negate( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_add( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_subtract( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_multiply( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_multiply_add( _In_ crude_vector const *v1, _In_ crude_vector const *v2, _In_ crude_vector const *v3 );
CRUDE_API crude_vector              crude_vec_divide( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_divide_add( _In_ crude_vector const *v1, _In_ crude_vector const *v2, _In_ crude_vector const *v3 );
CRUDE_API crude_vector              crude_vec_scale( _In_ crude_vector const *v, _In_ float32 s );
CRUDE_API crude_vector              crude_vec_sqrt( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_exp2( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_exp( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_log2( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_log10( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_log( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_pow( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_abs( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_cos( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_sin( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_tan( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_arc_cos( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_arc_sin( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_arc_tan( _In_ crude_vector const *v );

/////////////////////
//// @Vector 1
/////////////////////
CRUDE_API crude_vector              crude_vec_covector1( _In_ crude_vector const *v, _In_ crude_vector const *e1 );
CRUDE_API crude_vector              crude_vec_dot1( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_length1( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_length_sq1( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_normalize1( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_cos1( _In_ crude_vector const * v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_project1( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_reject1( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );

/////////////////////
//// @Vector 2
/////////////////////
CRUDE_API crude_vector              crude_vec_covector2( _In_ crude_vector const *v, _In_ crude_vector const *e1, _In_ crude_vector const *e2 );
CRUDE_API crude_vector              crude_vec_dot2( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_length2( _In_ crude_vector const * v );
CRUDE_API crude_vector              crude_vec_length_sq2( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_normalize2( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_cos2( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_project2( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_reject2( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );

/////////////////////
//// @Vector 3
/////////////////////
CRUDE_API crude_vector              crude_vec_covector3( _In_ crude_vector const *v, _In_ crude_vector const *e1, _In_ crude_vector const *e2, _In_ crude_vector const *e3 );
CRUDE_API crude_vector              crude_vec_dot3( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_length3( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_length_sq3( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_normalize3( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_cos3( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_triple_product3( _In_ crude_vector const *v1, _In_ crude_vector const *v2, _In_ crude_vector const *v3, _In_ crude_vector const *e1, _In_ crude_vector const *e2, _In_ crude_vector const *e3 );
CRUDE_API crude_vector              crude_vec_cross3( _In_ crude_vector const *v1, _In_ crude_vector const *v2, _In_ crude_vector const *e1, _In_ crude_vector const *e2, _In_ crude_vector const *e3 );
CRUDE_API crude_vector              crude_vec_triple_product3( _In_ crude_vector const *v1, _In_ crude_vector const *v2, _In_ crude_vector const *v3 );
CRUDE_API crude_vector              crude_vec_cross3( _In_ crude_vector const *v1, _In_ crude_vector const *v2);
CRUDE_API crude_vector              crude_vec_project3( _In_ crude_vector const *v1, _In_ crude_vector const *v2);
CRUDE_API crude_vector              crude_vec_reject3( _In_ crude_vector const *v1, _In_ crude_vector const *v2);

/////////////////////
//// @Vector 4
/////////////////////
CRUDE_API crude_vector              crude_vec_covector4( _In_ crude_vector const * v, _In_ crude_vector const *e1, _In_ crude_vector const *e2, _In_ crude_vector const *e3, _In_ crude_vector const *e4 );
CRUDE_API crude_vector              crude_vec_dot4( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_length4( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_length_sq4( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_normalize4( _In_ crude_vector const *v );
CRUDE_API crude_vector              crude_vec_cos4( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_project4( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );
CRUDE_API crude_vector              crude_vec_reject4( _In_ crude_vector const *v1, _In_ crude_vector const *v2 );

/////////////////////
//// @Matrix
/////////////////////
CRUDE_API bool                      crude_mat_is_identity( _In_ crude_matrix const *m );
CRUDE_API crude_matrix              crude_mat_multiply( _In_ crude_matrix const *m1, _In_ crude_matrix const *m2 );
CRUDE_API crude_matrix              crude_mat_multiply_transpose( _In_ crude_matrix const *m1, _In_ crude_matrix const *m2 );
CRUDE_API crude_matrix              crude_mat_transpose( _In_ crude_matrix const *m );
CRUDE_API crude_matrix              crude_mat_inverse( _In_ crude_matrix const *m );
CRUDE_API crude_vector              crude_mat_determinant( _In_ crude_matrix const *m );
CRUDE_API crude_matrix              crude_mat_identity();
CRUDE_API crude_matrix              crude_mat_set( _In_ float32 m00, _In_ float32 m01, _In_ float32 m02, _In_ float32 m03, _In_ float32 m10, _In_ float32 m11, _In_ float32 m12, _In_ float32 m13, _In_ float32 m20, _In_ float32 m21, _In_ float32 m22, _In_ float32 m23, _In_ float32 m30, _In_ float32 m31, _In_ float32 m32, _In_ float32 m33 );
CRUDE_API crude_matrix              crude_mat_translation( _In_ float32 offsetx, _In_ float32 offsety, _In_ float32 offsetz );
CRUDE_API crude_matrix              crude_mat_translation_from_vector( _In_ crude_vector const *offset );
CRUDE_API crude_matrix              crude_mat_scaling( _In_ float32 scalex, _In_ float32 scaley, _In_ float32 scalez );
CRUDE_API crude_matrix              crude_mat_scaling_from_vector( _In_ crude_vector const *scale );
CRUDE_API crude_matrix              crude_mat_rotation_x( _In_ float32 angle );
CRUDE_API crude_matrix              crude_mat_rotation_y( _In_ float32 angle );
CRUDE_API crude_matrix              crude_mat_rotation_z( _In_ float32 angle );
CRUDE_API crude_matrix              crude_mat_rotation_roll_pitch_yaw( _In_ float32 pitch, _In_ float32 yaw, _In_ float32 roll );
CRUDE_API crude_matrix              crude_mat_rotation_roll_pitch_yaw_from_vector( _In_ crude_vector const *angles );
CRUDE_API crude_matrix              crude_mat_rotation_normal( _In_ crude_vector const *normalAxis, _In_ float32 angle );
CRUDE_API crude_matrix              crude_mat_rotation_axis( _In_ crude_vector const *axis, _In_ float32 angle );
CRUDE_API crude_matrix              crude_mat_reflect( _In_ crude_vector const *reflection_plane );
CRUDE_API crude_matrix              crude_mat_look_at_lh( _In_ crude_vector const *eye_position, _In_ crude_vector const *focus_position, _In_ crude_vector const *up_direction );
CRUDE_API crude_matrix              crude_mat_look_at_rh( _In_ crude_vector const *eye_position, _In_ crude_vector const *focus_position, _In_ crude_vector const *up_direction );
CRUDE_API crude_matrix              crude_mat_look_to_lh( _In_ crude_vector const *eye_position, _In_ crude_vector const *eye_direction, _In_ crude_vector const *up_direction );
CRUDE_API crude_matrix              crude_mat_look_to_rh( _In_ crude_vector const *eye_position, _In_ crude_vector const *eye_direction, _In_ crude_vector const *up_direction );
CRUDE_API crude_matrix              crude_mat_perspective_lh( _In_ float32 view_width, _In_ float32 view_height, _In_ float32 nearz, _In_ float32 farz );
CRUDE_API crude_matrix              crude_mat_perspective_rh( _In_ float32 view_width, _In_ float32 view_height, _In_ float32 nearz, _In_ float32 farz );
CRUDE_API crude_matrix              crude_mat_perspective_fov_lh( _In_ float32 fov_angle_y, _In_ float32 aspect_ratio, _In_ float32 nearz, _In_ float32 farz );
CRUDE_API crude_matrix              crude_mat_perspective_fov_rh( _In_ float32 fov_angle_y, _In_ float32 aspect_ratio, _In_ float32 nearz, _In_ float32 farz );
CRUDE_API crude_matrix              crude_mat_perspective_off_center_lh( _In_ float32 view_left, _In_ float32 view_right, _In_ float32 view_bottom, _In_ float32 view_top, _In_ float32 nearz, _In_ float32 farz );
CRUDE_API crude_matrix              crude_mat_perspective_off_center_rh( _In_ float32 view_left, _In_ float32 view_right, _In_ float32 view_bottom, _In_ float32 view_top, _In_ float32 nearz, _In_ float32 farz );
CRUDE_API crude_matrix              crude_mat_orthographic_lh( _In_ float32 view_width, _In_ float32 view_height, _In_ float32 nearz, _In_ float32 farz );
CRUDE_API crude_matrix              crude_mat_orthographic_rh( _In_ float32 view_width, _In_ float32 view_height, _In_ float32 nearz, _In_ float32 farz );
CRUDE_API crude_matrix              crude_mat_orthographic_off_center_lh( _In_ float32 view_left, _In_ float32 view_right, _In_ float32 view_bottom, _In_ float32 view_top, _In_ float32 nearz, _In_ float32 farz );
CRUDE_API crude_matrix              crude_mat_orthographic_off_center_rh( _In_ float32 view_left, _In_ float32 view_right, _In_ float32 view_bottom, _In_ float32 view_top, _In_ float32 nearz, _In_ float32 farz );

/////////////////////
//// @Convert
/////////////////////
void                                crude_store_float3( _Out_ crude_float3 *f, crude_vector v );
void                                crude_store_float4( _Out_ crude_float4 *f, crude_vector v );
void                                crude_store_float4x4( _Out_ crude_float4x4 *f, crude_matrix m );
crude_vector                        crude_load_float3( _In_ crude_float3 const *f3 );
crude_vector                        crude_load_float4( _In_ crude_float4 const *f4 );
crude_matrix                        crude_load_float4x4( _In_ crude_float4x4 const *f4x4 );