
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#define DEBUG_CUBE
#define CRUDE_STAGE_VERTEX

#include "crude/platform.glsli"
#include "crude/scene.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

/********************
  *
  * DEBUG_LINE2D
  * VERTEX & FRAGMENT
  *
  *******************/
#if defined( DEBUG_LINE2D )

#if defined( CRUDE_STAGE_VERTEX )
layout(location=0) in vec3 in_position;
layout(location=1) in uint in_packed_color;

layout(location=0) out vec4 out_color;

void main()
{
  out_color = crude_unpack_color_rgba( in_packed_color );
  gl_Position = vec4( in_position, 1.0 );
}
#endif /* CRUDE_STAGE_VERTEX */

#if defined( CRUDE_STAGE_FRAGMENT )
layout(location=0) in vec4 in_color;

layout(location=0) out vec4 out_color;

void main()
{
  out_color = in_color;
}
#endif /* CRUDE_STAGE_FRAGMENT */

#endif /* DEBUG_LINE2D */

/********************
  *
  * DEBUG_LINE3D
  * VERTEX & FRAGMENT
  *
  *******************/
#if defined( DEBUG_LINE3D )

#if defined( CRUDE_STAGE_VERTEX )

layout(location=0) in vec3 in_position;
layout(location=1) in uint in_packed_color;

layout(location=0) out vec4 out_color;

CRUDE_UNIFORM( SceneConstant, 0 ) 
{
  crude_scene                                              scene;
};

void main()
{
  out_color = crude_unpack_color_rgba( in_packed_color );
  gl_Position = vec4( in_position, 1.0 ) * scene.camera.world_to_view * scene.camera.view_to_clip;
}

#endif /* CRUDE_STAGE_VERTEX */

#if defined( CRUDE_STAGE_FRAGMENT )

layout(location=0) in vec4 in_color;

layout(location=0) out vec4 out_color;

void main()
{
  out_color = in_color;
}

#endif /* CRUDE_STAGE_FRAGMENT */

#endif /* DEBUG_LINE3D */

/********************
  *
  * DEBUG_CUBE
  * VERTEX & FRAGMENT
  *
  *******************/
#if defined( DEBUG_CUBE )

CRUDE_UNIFORM( SceneConstant, 0 ) 
{
  crude_scene                                              scene;
};

CRUDE_RBUFFER( CrudeDebugCubes, 1 )
{
  crude_debug_cube_instance                                debug_cube_instances[];
};

#if defined( CRUDE_STAGE_VERTEX )

vec3 cube_vertices[ 36 ] = 
{
  vec3(-1.0, -1.0, -1.0),
  vec3( 1.0, -1.0, -1.0),
  vec3( 1.0,  1.0, -1.0),
  vec3( 1.0,  1.0, -1.0),
  vec3(-1.0,  1.0, -1.0),
  vec3(-1.0, -1.0, -1.0),
  vec3(-1.0, -1.0,  1.0),
  vec3( 1.0, -1.0,  1.0),
  vec3( 1.0,  1.0,  1.0),
  vec3( 1.0,  1.0,  1.0),
  vec3(-1.0,  1.0,  1.0),
  vec3(-1.0, -1.0,  1.0),
  vec3(-1.0, -1.0, -1.0),
  vec3(-1.0, -1.0,  1.0),
  vec3(-1.0,  1.0,  1.0),
  vec3(-1.0,  1.0,  1.0),
  vec3(-1.0,  1.0, -1.0),
  vec3(-1.0, -1.0, -1.0),
  vec3( 1.0, -1.0, -1.0),
  vec3( 1.0, -1.0,  1.0),
  vec3( 1.0,  1.0,  1.0),
  vec3( 1.0,  1.0,  1.0),
  vec3( 1.0,  1.0, -1.0),
  vec3( 1.0, -1.0, -1.0),
  vec3(-1.0, -1.0, -1.0),
  vec3( 1.0, -1.0, -1.0),
  vec3( 1.0, -1.0,  1.0),
  vec3( 1.0, -1.0,  1.0),
  vec3(-1.0, -1.0,  1.0),
  vec3(-1.0, -1.0, -1.0),
  vec3(-1.0,  1.0, -1.0),
  vec3( 1.0,  1.0, -1.0),
  vec3( 1.0,  1.0,  1.0),
  vec3( 1.0,  1.0,  1.0),
  vec3(-1.0,  1.0,  1.0),
  vec3(-1.0,  1.0, -1.0)
};

layout(location=0) out vec4 out_color;

void main()
{
  uint vertex_index = gl_VertexIndex;
  int probe_index = gl_InstanceIndex;

  vec3 position = cube_vertices[ gl_VertexIndex ];
  
  position = position * debug_cube_instances[ gl_InstanceIndex ].scale;
  position = position + debug_cube_instances[ gl_InstanceIndex ].translation;
  
  out_color = crude_unpack_color_rgba( debug_cube_instances[ gl_InstanceIndex ].color );
  gl_Position = vec4( position, 1 ) * scene.camera.world_to_clip;
}

#endif /* CRUDE_STAGE_VERTEX */

#if defined( CRUDE_STAGE_FRAGMENT )

layout(location=0) in vec4 in_color;

layout(location=0) out vec4 out_color;

void main()
{
  out_color = in_color;
}

#endif /* CRUDE_STAGE_FRAGMENT */

#endif /* DEBUG_CUBE */