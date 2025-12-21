
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#define IMGUI
#define CRUDE_STAGE_VERTEX
//#define CRUDE_STAGE_FRAGMENT

#include "crude/platform.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

struct imgui_vertex
{
  vec2                                                     position;
  vec2                                                     uv;
  uint                                                     color;
};

CRUDE_RBUFFER_REF_SCALAR( ImguiVerticesRef )
{
  imgui_vertex                                             data[];
};

CRUDE_RBUFFER_REF_SCALAR( ImguiIndicesRef )
{
  uint16_t                                                 data[];
};

#if defined( IMGUI )
#if defined( CRUDE_STAGE_VERTEX )

layout(location=0) out vec2 out_uv;
layout(location=1) out vec4 out_color;
layout(location=2) flat out uint out_texture_id;

CRUDE_PUSH_CONSTANT
{
  mat4                                                     projection;
  ImguiVerticesRef                                         vertices;
  ImguiIndicesRef                                          indices;
  uint32                                                   index_offset;
  uint32                                                   vertex_offset;
  uint32                                                   texture_index;
  float                                                    _padding;
};

void main()
{
  imgui_vertex vertex = vertices.data[ vertex_offset + indices.data[ index_offset + gl_VertexIndex ] ];
  out_uv = vertex.uv;
  out_color = crude_unpack_color_rgba( vertex.color );
  out_texture_id = texture_index;
  gl_Position = projection * vec4( vertex.position, 0, 1 );
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
  //out_color = vec4( out_color.b, out_color.g, out_color.r, out_color.a );
}
#endif /* CRUDE_STAGE_FRAGMENT */
#endif /* IMGUI */