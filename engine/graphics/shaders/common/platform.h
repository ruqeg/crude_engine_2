
#ifndef CRUDE_PLATFORM_GLSL
#define CRUDE_PLATFORM_GLSL

#ifdef __cplusplus
#include <vulkan/vulkan.h>
#include <engine/core/alias.h>
#include <engine/core/math.h>
#endif

#define CRUDE_SHADER_DEVELOP 1

#define CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX 0
#define CRUDE_ACCELERATION_STRUCTURE_DESCRIPTOR_SET_INDEX 1

#define CRUDE_ACCELERATION_STRUCTURE_BINDING 0

#define CRUDE_BINDLESS_TEXTURE_BINDING 10
#define CRUDE_BINDLESS_IMAGE_BINDING 11

#define CRUDE_PI 3.1415926538
#define CRUDE_1DIVPI 0.318309886f
#define CRUDE_SHADER_TEXTURE_UNDEFINED 0xffffffff

#define CRUDE_RIGHT_HAND 1

#define CRUDE_MESH_DRAW_FLAGS_ALPHA_MASK ( 1 << 1 )
#define CRUDE_MESH_DRAW_FLAGS_TRANSLUCENT_MASK ( 1 << 2 )
#define CRUDE_MESH_DRAW_FLAGS_HAS_NORMAL ( 1 << 3 )
#define CRUDE_MESH_DRAW_FLAGS_HAS_TANGENTS ( 1 << 4 )
#define CRUDE_MESH_DRAW_FLAGS_INDEX_16 ( 1 << 5 )

#ifdef __cplusplus
#define CRUDE_SHADER_STRUCT( name ) typedef CRUDE_ALIGNED_STRUCT( 16 ) name
#else
#define CRUDE_SHADER_STRUCT( name ) struct name
#endif

#ifdef __cplusplus
#define CRUDE_SHADER_RBUFFER_REF( name, type )  typedef VkDeviceAddress name;
#define CRUDE_SHADER_RBUFFER_REF_ARRAY( name, type )  typedef VkDeviceAddress name;
#define CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( name, type ) typedef VkDeviceAddress name;
#define CRUDE_SHADER_RBUFFER_REF_SCALAR( name, type )  typedef VkDeviceAddress name;
#else
#define CRUDE_SHADER_RBUFFER_REF( name, type ) layout(buffer_reference, row_major, std430) readonly buffer name { type data; };
#define CRUDE_SHADER_RBUFFER_REF_ARRAY( name, type ) layout(buffer_reference, row_major, std430) readonly buffer name { type data[]; };
#define CRUDE_SHADER_RBUFFER_REF_SCALAR( name, type ) layout(buffer_reference, row_major, scalar) readonly buffer name { type data; };
#define CRUDE_SHADER_RBUFFER_REF_ARRAY_SCALAR( name, type ) layout(buffer_reference, row_major, scalar) readonly buffer name { type data[]; };
#endif

#ifndef __cplusplus
#define CRUDE_PUSH_CONSTANT(name) layout(push_constant) uniform name
#else
#define CRUDE_PUSH_CONSTANT(name) CRUDE_ALIGNED_STRUCT( 16 ) name
#endif

#ifndef __cplusplus

#define float32 float
#define int8 int8_t
#define int16 int16_t
#define int64 int64_t
#define int32 int
#define uint8 uint8_t
#define uint16 uint16_t
#define uint64 uint64_t
#define uint32 uint
#define XMINT2 ivec2
#define XMINT3 ivec3
#define XMINT4 ivec4
#define XMFLOAT2 vec2
#define XMFLOAT3 vec3
#define XMFLOAT4 vec4
#define XMUINT2 uvec2
#define XMUINT3 uvec3
#define XMUINT4 uvec4
#define XMFLOAT2A vec4
#define XMFLOAT3A vec4
#define XMFLOAT4A vec4
#define XMUINT2A uvec4
#define XMUINT3A uvec4
#define XMUINT4A uvec4
#define XMFLOAT4X4 mat4

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

#define CRUDE_TEXTURE( ti, uv ) texture( global_textures[ nonuniformEXT( ti ) ], uv )
#define CRUDE_TEXTURE_FETCH( ti, coords, mip ) texelFetch( global_textures[ nonuniformEXT( ti ) ], coords, mip )
#define CRUDE_TEXTURE_LOD( ti, uv, mip ) textureLod( global_textures[ nonuniformEXT( ti ) ], uv, mip )
#define CRUDE_IMAGE_STORE( ti, coords, data ) imageStore( global_images_2d[ nonuniformEXT( ti ) ], coords, data )
#define CRUDE_IMAGE_LOAD( ti, coords ) imageLoad( global_images_2d[ nonuniformEXT( ti ) ], coords )

#define CRUDE_IMAGE_STORE_FORMATTED_DEFINE( ti, coords, data, format ) imageStore( global_images_2d_##format[ nonuniformEXT( ti ) ], coords, data )
#define CRUDE_IMAGE_LOAD_FORMATTED_DEFINE( ti, coords, format ) imageLoad( global_images_2d_##format[ nonuniformEXT( ti ) ], coords )

#define CRUDE_IMAGE_R32_STORE( ti, coords, data, format ) CRUDE_IMAGE_STORE_FORMATTED_DEFINE( ti, coords, data, r32f )
#define CRUDE_IMAGE_R32_LOAD( ti, coords, format ) CRUDE_IMAGE_LOAD_FORMATTED_DEFINE( ti, coords, r32f )

#define CRUDE_IMAGE_R16G16_STORE( ti, coords, data ) CRUDE_IMAGE_STORE_FORMATTED_DEFINE( ti, coords, data, rg16f )
#define CRUDE_IMAGE_R16G16_LOAD( ti, coords ) CRUDE_IMAGE_LOAD_FORMATTED_DEFINE( ti, coords, rg16f )

#define CRUDE_IMAGE_R16G16B16A16_STORE( ti, coords, data ) CRUDE_IMAGE_STORE_FORMATTED_DEFINE( ti, coords, data, rgba16f )
#define CRUDE_IMAGE_R16G16B16A16_LOAD( ti, coords ) CRUDE_IMAGE_LOAD_FORMATTED_DEFINE( ti, coords, rgba16f )

#define CRUDE_DEAFULT_F0 vec3( 0.04f )
#define CRUDE_SATURATE( v ) clamp( v, 0, 1 )

#define CRUDE_WAVE_IS_FIRST_LANE                           ( gl_SubgroupInvocationID == 0 )
#define CRUDE_WAVE_ACTIVE_COUNT_BITS( condition )          ( subgroupBallot( condition ) )
#define CRUDE_MAKE_UNIFORM( value )                        ( subgroupBroadcastFirst( value ) )

#define CRUDE_BARRIER                                      memoryBarrierShared(); barrier();

#define crude_device_address uint64

/* Read only */
layout(set=CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX, binding=CRUDE_BINDLESS_TEXTURE_BINDING) uniform sampler2D global_textures[];
layout(set=CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX, binding=CRUDE_BINDLESS_TEXTURE_BINDING) uniform sampler3D global_textures_3d[];

layout(set=CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX, binding=CRUDE_BINDLESS_IMAGE_BINDING) writeonly uniform image2D global_images_2d[];
layout(set=CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX, binding=CRUDE_BINDLESS_IMAGE_BINDING) writeonly uniform image3D global_images_3d[];
layout(set=CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX, binding=CRUDE_BINDLESS_IMAGE_BINDING) writeonly uniform uimage2D global_uimages_2d[];
layout(set=CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX, binding=CRUDE_BINDLESS_IMAGE_BINDING,r32f) uniform image2D global_images_2d_r32f[];
layout(set=CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX, binding=CRUDE_BINDLESS_IMAGE_BINDING,rg16f) uniform image2D global_images_2d_rg16f[];
layout(set=CRUDE_BINDLESS_DESCRIPTOR_SET_INDEX, binding=CRUDE_BINDLESS_IMAGE_BINDING,rgba16f) uniform image2D global_images_2d_rgba16f[];

/* 64 Distinct Colors. Used for anything that needs random colors. */
const uint32 crude_distinct_colors_u32[ 64 ] =
{
  0xFF000000, 0xFF00FF00, 0xFFFF0000, 0xFF0000FF, 0xFFFEFF01, 0xFFFEA6FF, 0xFF66DBFF, 0xFF016400,
  0xFF670001, 0xFF3A0095, 0xFFB57D00, 0xFFF600FF, 0xFFE8EEFF, 0xFF004D77, 0xFF92FB90, 0xFFFF7600,
  0xFF00FFD5, 0xFF7E93FF, 0xFF6C826A, 0xFF9D02FF, 0xFF0089FE, 0xFF82477A, 0xFFD22D7E, 0xFF00A985,
  0xFF5600FF, 0xFF0024A4, 0xFF7EAE00, 0xFF3B3D68, 0xFFFFC6BD, 0xFF003426, 0xFF93D3BD, 0xFF17B900,
  0xFF8E009E, 0xFF441500, 0xFF9F8CC2, 0xFFA374FF, 0xFFFFD001, 0xFF544700, 0xFFFE6FE5, 0xFF318278,
  0xFFA14C0E, 0xFFCBD091, 0xFF7099BE, 0xFFE88A96, 0xFF0088BB, 0xFF2C0043, 0xFF74FFDE, 0xFFC6FF00,
  0xFF02E5FF, 0xFF000E62, 0xFF9C8F00, 0xFF52FF98, 0xFFB14475, 0xFFFF00B5, 0xFF78FF00, 0xFF416EFF,
  0xFF395F00, 0xFF82686B, 0xFF4EAD5F, 0xFF4057A7, 0xFFD2FFA5, 0xFF67B1FF, 0xFFFF9B00, 0xFFBE5EE8
};

const vec3 crude_distinct_colors_f32[ 64 ] = 
{
  vec3(0.000,0.000,0.000),vec3(0.000,1.000,0.000),vec3(1.000,0.000,0.000),vec3(0.000,0.000,1.000),
  vec3(0.996,1.000,0.004),vec3(0.996,0.651,1.000),vec3(0.400,0.859,1.000),vec3(0.004,0.392,0.251),
  vec3(0.404,0.000,0.004),vec3(0.227,0.000,0.584),vec3(0.710,0.490,0.000),vec3(0.965,0.000,1.000),
  vec3(0.910,0.933,1.000),vec3(0.000,0.302,0.467),vec3(0.573,0.984,0.565),vec3(1.000,0.463,0.000),
  vec3(0.000,1.000,0.835),vec3(0.494,0.576,1.000),vec3(0.424,0.510,0.416),vec3(0.615,0.008,1.000),
  vec3(0.000,0.537,0.996),vec3(0.510,0.278,0.478),vec3(0.824,0.176,0.494),vec3(0.000,0.663,0.522),
  vec3(0.337,0.000,1.000),vec3(0.000,0.141,0.643),vec3(0.494,0.682,0.000),vec3(0.231,0.239,0.408),
  vec3(1.000,0.776,0.741),vec3(0.000,0.204,0.149),vec3(0.576,0.827,0.741),vec3(0.090,0.725,0.000),
  vec3(0.557,0.000,0.620),vec3(0.267,0.082,0.000),vec3(0.624,0.549,0.761),vec3(0.639,0.455,1.000),
  vec3(1.000,0.816,0.004),vec3(0.329,0.278,0.000),vec3(0.996,0.435,0.898),vec3(0.192,0.510,0.471),
  vec3(0.631,0.298,0.055),vec3(0.796,0.816,0.569),vec3(0.441,0.600,0.745),vec3(0.910,0.541,0.588),
  vec3(0.000,0.533,0.733),vec3(0.173,0.000,0.263),vec3(0.455,1.000,0.871),vec3(0.776,1.000,0.000),
  vec3(0.008,0.898,1.000),vec3(0.000,0.055,0.384),vec3(0.612,0.561,0.000),vec3(0.322,1.000,0.596),
  vec3(0.694,0.267,0.459),vec3(1.000,0.000,0.710),vec3(0.471,1.000,0.000),vec3(0.255,0.431,1.000),
  vec3(0.224,0.373,0.000),vec3(0.510,0.408,0.420),vec3(0.306,0.678,0.373),vec3(0.251,0.341,0.655),
  vec3(0.824,1.000,0.647),vec3(0.404,0.694,1.000),vec3(1.000,0.608,0.000),vec3(0.745,0.369,0.910)
};

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
  float phi = 2.0 * CRUDE_PI * madfrac( i, PHI - 1 );
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
  
  if ( ( flags & CRUDE_MESH_DRAW_FLAGS_HAS_NORMAL ) == 0 )
  {
    normal = normalize( cross( dFdx( vertex_world_position ), dFdy( vertex_world_position ) ) );
  }

  vec3 tangent = normalize( vertex_tangent );
  vec3 bitangent = normalize( vertex_bitangent );
  if ( ( flags & CRUDE_MESH_DRAW_FLAGS_HAS_TANGENTS ) == 0 )
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

#if defined( CRUDE_CLOSEST_HIT ) || defined( CRUDE_RAYGEN ) || defined( CRUDE_MISS )
#extension GL_EXT_ray_tracing : require
#endif

#define CRUDE_ACCELERATION_STRUCTURE_DEFINE( name ) layout(set=CRUDE_ACCELERATION_STRUCTURE_DESCRIPTOR_SET_INDEX, binding=CRUDE_ACCELERATION_STRUCTURE_BINDING) uniform accelerationStructureEXT name;

#endif

#if defined( CRUDE_CLOSEST_HIT ) || defined( CRUDE_RAYGEN ) || defined( CRUDE_MISS ) || defined( __cplusplus )
CRUDE_SHADER_RBUFFER_REF_SCALAR( AccelerationStructureRef, uint64 );
#endif

#endif /* CRUDE_PLATFORM_GLSL */