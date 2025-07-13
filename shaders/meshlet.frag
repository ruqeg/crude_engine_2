
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive: require
#include "crude/platform.glsli"
#include "crude/scene.glsli"
#include "crude/meshlet.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

layout(location = 0) out vec4 out_color;

layout(location=0) in vec2 in_texcoord0;
layout(location=1) in flat uint mesh_draw_index;

void main()
{
  crude_mesh_draw mesh_draw = mesh_draws[ mesh_draw_index ];
  vec4 albedo = texture( global_textures[ nonuniformEXT( mesh_draw.textures.x ) ], in_texcoord0 ) * mesh_draw.albedo_color_factor;
  out_color = vec4( albedo );
}