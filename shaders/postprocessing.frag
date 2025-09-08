
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#include "crude/platform.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

layout(location = 0) out vec4 out_color;

layout( push_constant ) uniform Constants
{
  uint                                                     luminance_average_texture_index;
  uint                                                     pbr_texture_index;
};

void main()
{ 
  vec4 color = texelFetch( global_textures[ nonuniformEXT( pbr_texture_index )], ivec2( gl_FragCoord.xy ), 0 );
  float luminance_average = texelFetch( global_textures[ nonuniformEXT( luminance_average_texture_index )], ivec2( 0, 0 ), 0 ).r;
  
  color.xyz = color.xyz * ( crude_rgb_to_luminance( color.xyz ) / ( 9.6 * luminance_average ) );
  color.xyz = crude_aces_fitted( color.xyz );
  color.xyz = pow( color.xyz, vec3( 1 / 2.2 ) );
  out_color = color;
}