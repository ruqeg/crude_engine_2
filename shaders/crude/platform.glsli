
#ifndef CRUDE_PLATFORM_GLSL
#define CRUDE_PLATFORM_GLSL

//#define CRUDE_GRAPHICS_RAY_TRACING_ENABLED
//#define CRUDE_RAYTRACED_SHADOWS
//#define CRUDE_RAYTRACED_DDGI

#define CRUDE_GLOBAL_SET 0
#define CRUDE_MATERIAL_SET 1

#define CRUDE_BINDLESS_BINDING 10
#define CRUDE_BINDLESS_IMAGES 11

#define PI 3.1415926538
#define CRUDE_1DIVPI 3.1415926538
#define CRUDE_GRAPHICS_SHADER_TEXTURE_UNDEFINED 0xffffffff

#define CRUDE_DRAW_FLAGS_ALPHA_MASK ( 1 << 1 )
#define CRUDE_DRAW_FLAGS_TRANSPARENT_MASK ( 1 << 2 )
#define CRUDE_DRAW_FLAGS_HAS_NORMAL ( 1 << 3 )
#define CRUDE_DRAW_FLAGS_HAS_TANGENTS ( 1 << 4 )
#define CRUDE_DRAW_FLAGS_INDEX_16 ( 1 << 5 )

#define CRUDE_UNIFORM( name, bind ) layout(set=CRUDE_MATERIAL_SET, binding=bind, row_major, std140) uniform name
#define CRUDE_RBUFFER( name, bind ) layout(set=CRUDE_MATERIAL_SET, binding=bind, row_major, std430) readonly buffer name
#define CRUDE_RWBUFFER( name, bind ) layout(set=CRUDE_MATERIAL_SET, binding=bind, row_major, std430) buffer name
#define CRUDE_PUSH_CONSTANT layout(push_constant) uniform _Crude_Push_Constant

#define CRUDE_TEXTURE( ti, uv ) texture( global_textures[ nonuniformEXT( ti ) ], uv )
#define CRUDE_TEXTURE_FETCH( ti, coords, mip ) texelFetch( global_textures[ nonuniformEXT( ti ) ], coords, mip )
#define CRUDE_TEXTURE_LOD( ti, uv, mip ) textureLod( global_textures[ nonuniformEXT( ti ) ], uv, mip )
#define CRUDE_IMAGE_STORE( ti, coords, data ) imageStore( global_images_2d[ nonuniformEXT( ti ) ], coords, data )

#define CRUDE_RBUFFER_REF_SCALAR( name ) layout(buffer_reference, row_major, scalar) readonly buffer name
#define CRUDE_RBUFFER_REF_ALIGNED( name, align ) layout(buffer_reference, row_major, std430, buffer_reference_align=align) readonly buffer name
#define CRUDE_RBUFFER_REF( name ) layout(buffer_reference, row_major, std430) readonly buffer name
#define CRUDE_RWBUFFER_REF( name ) layout(buffer_reference, row_major, std430) buffer name

#define CRUDE_DEAFULT_F0 vec3( 0.04f )
#define CRUDE_SATURATE( v ) clamp( v, 0, 1 )

#extension GL_EXT_debug_printf : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_KHR_shader_subgroup_ballot: require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require

#define uint8 uint8_t
#define uint16 uint16_t
#define uint64 uint64_t
#define uint32 uint

#define crude_device_address uint64

/* Read only */
layout(set=CRUDE_GLOBAL_SET, binding=CRUDE_BINDLESS_BINDING) uniform sampler2D global_textures[];
layout(set=CRUDE_GLOBAL_SET, binding=CRUDE_BINDLESS_BINDING) uniform sampler3D global_textures_3d[];

layout(set=CRUDE_GLOBAL_SET, binding=CRUDE_BINDLESS_IMAGES) writeonly uniform image2D global_images_2d[];
layout(set=CRUDE_GLOBAL_SET, binding=CRUDE_BINDLESS_IMAGES) writeonly uniform image3D global_images_3d[];
layout(set=CRUDE_GLOBAL_SET, binding=CRUDE_BINDLESS_IMAGES) writeonly uniform uimage2D global_uimages_2d[];
layout(set=CRUDE_GLOBAL_SET, binding=CRUDE_BINDLESS_IMAGES,r32f) uniform image2D global_images_2d_r32f[];

uint crude_vec4_to_rgba( vec4 color )
{
  return ( uint( color.r * 255.f ) | ( uint( color.g * 255.f ) << 8 ) | ( uint( color.b * 255.f ) << 16 ) | ( ( uint( color.a * 255.f ) << 24 ) ) );
}

vec4 crude_unpack_color_rgba( uint color )
{
  return vec4( ( color & 0xffu ) / 255.f, ( ( color >> 8u ) & 0xffu ) / 255.f, ( ( color >> 16u ) & 0xffu ) / 255.f, ( ( color >> 24u ) & 0xffu ) / 255.f );
}

vec4 crude_unpack_color_abgr( uint color )
{
  return vec4( ( ( color >> 24u ) & 0xffu ) / 255.f, ( ( color >> 16u ) & 0xffu ) / 255.f, ( ( color >> 8u ) & 0xffu ) / 255.f, ( color & 0xffu ) / 255.f );
}

vec2 crude_sign_not_zero( vec2 v )
{
  return vec2( ( v.x >= 0.0 ) ? 1.0 : -1.0, ( v.y >= 0.0 ) ? 1.0 : -1.0 );
}

vec2 crude_octahedral_encode( vec3 n )
{
  vec2 p = n.xy * ( 1.0f / ( abs( n.x ) + abs( n.y ) + abs( n.z ) ) );
  return ( n.z < 0.0f ) ? ( ( 1.0 - abs( p.yx ) ) * crude_sign_not_zero( p ) ) : p;
}

/* https://twitter.com/Stubbesaurus/status/937994790553227264?s=20&t=U36PKMj7v2BFeQwDX6gEGQ */
vec3 crude_octahedral_decode( vec2 f )
{
  vec3 n = vec3( f.x, f.y, 1.0 - abs( f.x ) - abs( f.y ) );
  float t = max( -n.z, 0.0 );
  n.x += n.x >= 0.0 ? -t : t;
  n.y += n.y >= 0.0 ? -t : t;
  return normalize( n );
}

vec3 crude_ndc_from_uv_depth( vec2 uv, float depth )
{
  return vec3( uv.x * 2 - 1, ( 1 - uv.y ) * 2 - 1, depth );
}

vec3 crude_world_position_from_depth( vec2 uv, float depth, mat4 clip_to_world )
{
  vec4 h = vec4( crude_ndc_from_uv_depth( uv, depth ), 1.0 );
  vec4 d = h * clip_to_world;
  return d.xyz / d.w;
}

vec2
crude_oct_encode
(
  in vec3                                                  v
)
{
  float l1norm = abs( v.x ) + abs( v.y ) + abs( v.z );
  vec2 result = v.xy * ( 1.0 / l1norm );
  if ( v.z < 0.0 )
  {
    result = ( 1.0 - abs( result.yx ) ) * crude_sign_not_zero( result.xy );
  }
  return result;
}

vec3
crude_oct_decode
(
  in vec2                                                  o
)
{
  vec3 v = vec3( o.x, o.y, 1.0 - abs( o.x ) - abs( o.y ) );
  if ( v.z < 0.0 )
  {
    v.xy = ( 1.0 - abs( v.yx ) ) * crude_sign_not_zero( v.xy );
  }
  return normalize( v );
}

vec2
crude_uv_nearest
(
  in ivec2                                                 pixel,
  in vec2                                                  texture_size
)
{
  vec2 uv = pixel + .5;
  return uv / texture_size;
}

float
crude_calculate_scale_from_matrix
(
  in mat3                                                  m
)
{
  return max( length( m[ 0 ] ), max( length( m[ 1 ] ), length( m[ 2 ] ) ) );
}

float
crude_linearize_depth
(
  in float                                                 d,
  in float                                                 znear,
  in float                                                 zfar
)
{
  return znear * zfar / ( zfar + d * ( znear - zfar ) );
}

void
crude_swap
(
  inout float a,
  inout float b
)
{
  float t = a;
  a = b;
  b = t;
}

/* https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl */

/* sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT */
const mat3 crude_aces_input_mat = mat3(
  0.59719f, 0.35458f, 0.04823f,
  0.07600f, 0.90834f, 0.01566f,
  0.02840f, 0.13383f, 0.83777f
);

/* ODT_SAT => XYZ => D60_2_D65 => sRGB */
const mat3 crude_aces_output_mat = mat3(
   1.60475f, -0.53108f, -0.07367f,
  -0.10208f,  1.10813f, -0.00605f,
  -0.00327f, -0.07276f,  1.07602f
);

vec3 crude_rrt_and_odt_fit( vec3 v )
{
  vec3 a = v * ( v + 0.0245786f ) - 0.000090537f;
  vec3 b = v * ( 0.983729f * v + 0.4329510f ) + 0.238081f;
  return a / b;
}

vec3 crude_aces_fitted( vec3 color )
{
  color = color * crude_aces_input_mat;
  color = crude_rrt_and_odt_fit( color );
  color = color * crude_aces_output_mat;
  color = clamp( color, 0.f, 1.f );
  return color;
}

vec3 crude_aces_fitted2( vec3 x )
{
  float a = 2.51f;
  float b = 0.03f;
  float c = 2.43f;
  float d = 0.59f;
  float e = 0.14f;
  return clamp( ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e ), 0, 1 );
}

vec3 crude_uncharted2_tonemap( vec3 x )
{
  const float A = 0.22;
  const float B = 0.30;
  const float C = 0.10;
  const float D = 0.20;
  const float E = 0.01;
  const float F = 0.30;
  return ( ( x * ( A * x + C * B ) + D * E ) / ( x * ( A * x + B ) + D * F ) ) - E / F;
}


float crude_rgb_to_luminance( vec3 rgb )
{
  return dot( rgb, vec3( 0.2125, 0.7154, 0.0721 ) );
}

vec3 crude_spherical_fibonacci( float i, float n )
{
  float PHI = sqrt( 5.0f ) * 0.5 + 0.5;
#define madfrac( A, B ) ( ( A ) * ( B )-floor( ( A ) * ( B ) ) )
  float phi = 2.0 * PI * madfrac( i, PHI - 1 );
  float cos_theta = 1.0 - ( 2.0 * i + 1.0 ) * ( 1.0 / n );
  float sin_theta = sqrt( clamp( 1.0 - cos_theta * cos_theta, 0.0f, 1.0f ) );
  return vec3( cos( phi ) * sin_theta, sin( phi ) * sin_theta, cos_theta );
#undef madfrac
}

float
crude_light_attenuation
(
  in float                                                 light_distance,
  in float                                                 light_radius
)
{
  float attenuation = max( 1.f - pow( light_distance / light_radius, 2.f ), 0.f );
  attenuation = attenuation * attenuation;
  return attenuation;
}

#if defined( CRUDE_STAGE_COMPUTE )
#endif /* CRUDE_STAGE_COMPUTE */


#if defined( CRUDE_STAGE_FRAGMENT )
void crude_calculate_geometric_tbn
(
  inout vec3                                               vertex_normal,
  inout vec3                                               vertex_tangent,
  inout vec3                                               vertex_bitangent,
  in vec2                                                  uv,
  in vec3                                                  vertex_world_position,
  in uint                                                  flags
)
{
  vec3 normal = normalize( vertex_normal );
  
  if ( ( flags & CRUDE_DRAW_FLAGS_HAS_NORMAL ) == 0 )
  {
    normal = normalize( cross( dFdx( vertex_world_position ), dFdy( vertex_world_position ) ) );
  }

  vec3 tangent = normalize( vertex_tangent );
  vec3 bitangent = normalize( vertex_bitangent );
  if ( ( flags & CRUDE_DRAW_FLAGS_HAS_TANGENTS ) == 0 )
  {
    vec3 uv_dx = dFdx( vec3( uv, 0.0 ) );
    vec3 uv_dy = dFdy( vec3( uv, 0.0 ) );

    vec3 t_ = ( uv_dy.t * dFdx( vertex_world_position ) - uv_dx.t * dFdy( vertex_world_position ) ) / ( uv_dx.s * uv_dy.t - uv_dy.s * uv_dx.t );
    tangent = normalize( t_ - normal * dot( normal, t_ ) );
    bitangent = cross( normal, tangent );
  }

  if ( !gl_FrontFacing )
  {
    tangent *= -1.0;
    bitangent *= -1.0;
    normal *= -1.0;
  }

  vertex_normal = normal;
  vertex_tangent = tangent;
  vertex_bitangent = bitangent;
}
#endif /* CRUDE_STAGE_FRAGMENT */

#if defined( CRUDE_STAGE_TASK ) || defined( CRUDE_STAGE_MESH )
#extension GL_EXT_mesh_shader : require
#endif /* CRUDE_STAGE_TASK || CRUDE_STAGE_MESH */

#endif /* CRUDE_PLATFORM_GLSL */