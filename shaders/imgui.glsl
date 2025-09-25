
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#define CRUDE_STAGE_FRAGMENT

#include "crude/platform.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#if defined( IMGUI )
#if defined( CRUDE_STAGE_VERTEX )

layout(location=0) in vec2 in_position;
layout(location=1) in vec2 in_uv;
layout(location=2) in uvec4 in_color;

layout(location=0) out vec2 out_uv;
layout(location=1) out vec4 out_color;
layout(location=2) flat out uint out_texture_id;

layout(std140, set=1, binding=0) uniform LocalConstants
{
  mat4                                                     projection;
};

void main()
{
 out_uv = in_uv;
 out_color = in_color / 255.f;
 out_texture_id = gl_InstanceIndex;
 gl_Position = projection * vec4( in_position.xy, 0, 1 );
}
#endif /* CRUDE_STAGE_VERTEX */

#if defined( CRUDE_STAGE_FRAGMENT )
layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;
layout(location = 2) flat in uint in_texture_id;

layout(location = 0) out vec4 out_color;

void main()
{
  out_color = in_color * texture( global_textures[ nonuniformEXT( in_texture_id ) ], in_uv.st );
}
#endif /* CRUDE_STAGE_FRAGMENT */
#endif /* IMGUI */