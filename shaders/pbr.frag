
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
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


void main()
{ 
  float depth = texelFetch( global_textures[ nonuniformEXT( textures.w )], ivec2( gl_FragCoord.xy ), 0 ).r;
  vec4 albedo = texture( global_textures[ nonuniformEXT( textures.x ) ], in_texcoord.st ).rgba;
  vec2 packed_normal = texture( global_textures[ nonuniformEXT( textures.y ) ], in_texcoord.st ).xy;
  float packed_roughness = texture( global_textures[ nonuniformEXT( textures.z ) ], in_texcoord.st ).x;
  vec3 normal = crude_octahedral_decode( packed_normal );

  vec3 pixel_world_position = crude_world_position_from_depth( in_texcoord, depth, camera.clip_to_world );

  uvec2 position = uvec2( gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5 );
  
  vec4 radiance = vec4( 0.f, 0.f, 0.f, 1.f );
  if ( depth != 1.f )
  {
    radiance = vec4( crude_calculate_lighting( albedo, packed_roughness, normal, pixel_world_position, camera.position, position ), 1 );
  }
  out_color = radiance;
}