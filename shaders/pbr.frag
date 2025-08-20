
#ifdef CRUDE_VALIDATOR_LINTING
#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/light.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

layout(location = 0) out vec4 out_color;

layout(location=0) in vec2 in_texcoord;

layout(set=CRUDE_MATERIAL_SET, binding=10, row_major) uniform LightingConstants
{
  uvec4                                                    textures;
};

const vec3 tetrahedron_face_a = vec3( 0.0, -0.57735026, 0.81649661 );
const vec3 tetrahedron_face_b = vec3( 0.0, -0.57735026, -0.81649661 );
const vec3 tetrahedron_face_c = vec3( -0.81649661, 0.57735026, 0.0 );
const vec3 tetrahedron_face_d = vec3( 0.81649661, 0.57735026, 0.0 );

uint get_tetrahedron_face_index( vec3 dir )
{
  mat4x3 face_matrix;
  face_matrix[ 0 ] = tetrahedron_face_a;
  face_matrix[ 1 ] = tetrahedron_face_b;
  face_matrix[ 2 ] = tetrahedron_face_c;
  face_matrix[ 3 ] = tetrahedron_face_d; 
  vec4 dot_products = dir * face_matrix;
  float maximum = max (max( dot_products.x, dot_products.y ), max( dot_products.z, dot_products.w ) );
  
  uint index;
  if ( maximum == dot_products.x )
  {
    index = 0;
  }
  else if ( maximum == dot_products.y )
  {
    index = 1;
  }
  else if ( maximum == dot_products.z )
  {
    index = 2;
  }
  else
  { 
    index = 3;
  }

  return index;
}
#define NUM_SAMPLES 16
#define FILTER_RADIUS 1.5

const vec2 filter_kernel[NUM_SAMPLES] =
{
  vec2(-0.94201624, -0.39906216),
  vec2(0.94558609, -0.76890725),
  vec2(-0.094184101, -0.92938870),
  vec2(0.34495938, 0.29387760),
  vec2(-0.91588581, 0.45771432),
  vec2(-0.81544232, -0.87912464),
  vec2(-0.38277543, 0.27676845),
  vec2(0.97484398, 0.75648379),
  vec2(0.44323325, -0.97511554),
  vec2(0.53742981, -0.47373420),
  vec2(-0.26496911, -0.41893023),
  vec2(0.79197514, 0.19090188),
  vec2(-0.24188840, 0.99706507),
  vec2(-0.81409955, 0.91437590),
  vec2(0.19984126, 0.78641367),
  vec2(0.14383161, -0.14100790)
};

void main()
{ 
  float depth = texelFetch( global_textures[ nonuniformEXT( textures.w )], ivec2( gl_FragCoord.xy ), 0 ).r;
  vec4 albedo = texture( global_textures[ nonuniformEXT( textures.x ) ], in_texcoord.st ).rgba;
  vec2 packed_normal = texture( global_textures[ nonuniformEXT( textures.y ) ], in_texcoord.st ).xy;
  vec3 normal = crude_octahedral_decode( packed_normal );

  vec3 pixel_world_position = crude_world_position_from_depth( in_texcoord, depth, camera.clip_to_world );

  uvec2 position = uvec2( gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5 );
  
  vec4 color = vec4( 0.f, 0.f, 0.f, 1.f );
  if ( depth != 1.f )
  {
    crude_light light = lights[ 0 ];
    vec3 shadow_position_to_light = light.world_position - pixel_world_position.xyz;
    uint face_index = get_tetrahedron_face_index( normalize( -shadow_position_to_light ) );
    vec4 proj_pos = vec4( pixel_world_position.xyz, 1.0 ) * pointlight_world_to_clip[ 0 * 4 + face_index ];
    proj_pos.xyz /= proj_pos.w;
    
    vec2 proj_uv = ( proj_pos.xy * 0.5 ) + 0.5;
    proj_uv.y = 1.f - proj_uv.y;

    float bias = 0.01f;
    float current_depth = proj_pos.z;
    
    vec2 filter_radius = inv_shadow_map_size.xy * FILTER_RADIUS; 
    float shadow = 0;
    for(uint i = 0; i < NUM_SAMPLES; ++i)
    {
      vec2 texcoords;
      texcoords.xy = proj_uv.xy + ( filter_kernel[i] * filter_radius );
      float closest_depth = texture( global_textures[ nonuniformEXT( tiled_shadowmap_texture_index ) ], texcoords ).r;
      shadow += current_depth - bias < closest_depth ? 1 : 0;
    }

    shadow /= NUM_SAMPLES;

    color = vec4( shadow.xxx, 1.0 ) * crude_calculate_lighting( albedo, normal, pixel_world_position, position );
  }
  out_color = color;
}