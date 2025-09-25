
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#define POINTSHADOW_COMMANDS_GENERATION
//#define CRUDE_STAGE_FRAGMENT

#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/culling.glsli"
#include "crude/light.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#if defined( POINTSHADOW_CULLING )
#endif /* POINTSHADOW_CULLING */

#if defined( POINTSHADOW ) 
#extension GL_ARB_shader_draw_parameters : require
#extension GL_KHR_shader_subgroup_ballot: require

CRUDE_RBUFFER( MeshInstancesDraws, 1 )
{
  crude_mesh_instance_draw                                 mesh_instance_draws[];
};

CRUDE_RBUFFER( Meshlets, 2 )
{
  crude_meshlet                                            meshlets[];
};

CRUDE_RBUFFER( MeshBounds, 3 )
{
  vec4                                                     mesh_bounds[];
};

CRUDE_RBUFFER( Vertices, 4 )
{
  crude_vertex                                             vertices[];
};
CRUDE_RBUFFER( TrianglesIndices, 5 )
{
  uint8_t                                                  triangles_indices[];
};
CRUDE_RBUFFER( VerticesIndices, 6 )
{
  uint                                                     vertices_indices[];
};

CRUDE_RBUFFER( ShadowCameraSpheres, 7 )
{
  vec4                                                     pointlight_spheres[];
};

CRUDE_RBUFFER( PointshadowMeshletDrawCommands, 8 )
{
  uvec4                                                    pointshadow_meshlet_draw_commands[];
};

CRUDE_RBUFFER( PointLightMeshletInstances, 9 )
{
  uvec2                                                    meshletes_instances[];
};

CRUDE_RBUFFER( PointLightMeshletCount, 10 )
{
  uint                                                     pointshadow_meshletes_instances_count[];
};

CRUDE_RBUFFER( PointlightWorldToClip, 11 )
{
  mat4                                                      pointlight_world_to_clip[];
};

#if defined( CRUDE_STAGE_TASK ) || defined( CRUDE_STAGE_MESH )
taskPayloadSharedEXT struct
{
  uint                                                     meshlet_indices[ 128 ];
  uint                                                     mesh_instance_draw_indices[ 128 ];
  uint                                                     light_index_face_index;
} shared_data;
#endif /* CRUDE_STAGE_TASK || CRUDE_STAGE_MESH */

#if defined( CRUDE_STAGE_TASK )
layout(local_size_x=32) in;

void main()
{
  uint meshlete_instance_index = gl_GlobalInvocationID.x;

  uint packed_light_index_face_index = pointshadow_meshlet_draw_commands[ gl_DrawIDARB ].w;
  uint light_index = packed_light_index_face_index >> 16;

  if ( meshlete_instance_index >= pointshadow_meshletes_instances_count[ light_index ] )
  {
    return;
  }

  uvec2 packed_meshlet_instance = meshletes_instances[ light_index * CRUDE_MAX_MESHLETS_PER_LIGHT + meshlete_instance_index ];

  uint mesh_instance_index = packed_meshlet_instance.x;
  uint global_meshlet_index = packed_meshlet_instance.y;

  uint face_index = ( packed_light_index_face_index & 0xf );

  mat4 model_to_world = mesh_instance_draws[ mesh_instance_index ].model_to_world;
  vec4 world_center = vec4( meshlets[ global_meshlet_index ].center, 1 ) * model_to_world;
  float scale = max( model_to_world[ 0 ][ 0 ], max( model_to_world[ 1 ][ 1 ], model_to_world[ 2 ][ 2 ] ) );
  float radius = meshlets[ global_meshlet_index ].radius * scale * 1.1;
  vec3 cone_axis = vec3(
    int( meshlets[ global_meshlet_index ].cone_axis[ 0 ] ) / 127.f,
    int( meshlets[ global_meshlet_index ].cone_axis[ 1 ] ) / 127.f,
    int( meshlets[ global_meshlet_index ].cone_axis[ 2 ] ) / 127.f ) * mat3( model_to_world );
  float cone_cutoff = int( meshlets[ global_meshlet_index ].cone_cutoff ) / 127.f;

  const vec4 camera_sphere = pointlight_spheres[ light_index ];

  bool accept = !crude_clustered_backface_culling( world_center.xyz, radius, cone_axis, cone_cutoff, camera_sphere.xyz );

  if ( accept )
  {
    uint visible_faces = crude_get_cube_face_mask( camera_sphere.xyz, world_center.xyz - vec3( radius ), world_center.xyz + vec3( radius ) );

    switch ( face_index )
    {
    case 0:
      accept = accept || ( ( visible_faces & 1 ) != 0 );
      break;
    case 1:
      accept = accept || ( ( visible_faces & 2 ) != 0 );
      break;
    case 2:
      accept = accept || ( ( visible_faces & 4 ) != 0 );
      break;
    case 3:
      accept = accept || ( ( visible_faces & 8 ) != 0 );
      break;
    case 4:
      accept = accept || ( ( visible_faces & 16 ) != 0 );
      break;
    case 5:
      accept = accept || ( ( visible_faces & 32 ) != 0 );
      break;
    }
  }

  uvec4 ballot = subgroupBallot( accept );
  uint index = subgroupBallotExclusiveBitCount( ballot );

  if ( accept )
  {
    shared_data.meshlet_indices[ index ] = global_meshlet_index;
    shared_data.mesh_instance_draw_indices[ index ] = mesh_instance_index;
  }

  uint visible_meslets_count = subgroupBallotBitCount( ballot );

  shared_data.light_index_face_index = packed_light_index_face_index;

  if ( gl_LocalInvocationID.x == 0 )
  {
    EmitMeshTasksEXT( visible_meslets_count, 1, 1 );
  }
}
#endif /* CRUDE_STAGE_TASK */

#if defined( CRUDE_STAGE_MESH )
layout(local_size_x=128, local_size_y=1, local_size_z=1) in;
layout(triangles) out;
layout(max_vertices=128, max_primitives=124) out;

layout(location=0) out vec2 out_texcoord[];
layout(location=1) out flat uint out_mesh_draw_index[];
layout(location=2) out vec3 out_normals[];

/* GPU Pro 6 Tile-Based Omnidirectional Shadows Hawar Doghramachi, http://www.hd-prg.com/tileBasedShadows.html */
const vec3 plane_normals[ 12 ] =
{
  vec3(0.00000000, -0.03477280, 0.99939519),
  vec3(-0.47510946, -0.70667917, 0.52428567),
  vec3(0.47510946, -0.70667917, 0.52428567),
  vec3(0.00000000, -0.03477280, -0.99939519),
  vec3(0.47510946, -0.70667917, -0.52428567),
  vec3(-0.47510946, -0.70667917, -0.52428567),
  vec3(-0.52428567, 0.70667917, -0.47510946),
  vec3(-0.52428567, 0.70667917, 0.47510946),
  vec3(-0.99939519, 0.03477280, 0.00000000),
  vec3(0.52428567, 0.70667917, -0.47510946),
  vec3(0.99939519, 0.03477280, 0.00000000),
  vec3(0.52428567, 0.70667917, 0.47510946)
};

float get_clip_distance( vec3 light_position, vec3 vertex_position, uint plane_index )
{
  vec3 normal = plane_normals[ plane_index ];
  return dot( vertex_position, normal ) + dot( light_position, -normal );
}

void main()
{
  uint task_index = gl_LocalInvocationID.x;
  uint local_meshlet_index = gl_WorkGroupID.x;
  uint global_meshlet_index = shared_data.meshlet_indices[ local_meshlet_index ];
  uint mesh_instance_draw_index = shared_data.mesh_instance_draw_indices[ local_meshlet_index ];
  uint mesh_draw_index = mesh_instance_draws[ mesh_instance_draw_index ].mesh_draw_index;
  
  uint mesh_index = meshlets[ global_meshlet_index ].mesh_index;
  uint vertices_count = uint( meshlets[ global_meshlet_index ].vertices_count );
  uint triangles_count = uint( meshlets[ global_meshlet_index ].triangles_count );
  
  uint vertices_offset = meshlets[ global_meshlet_index ].vertices_offset;
  uint triangles_offset = meshlets[ global_meshlet_index ].triangles_offset;

  uint light_index = shared_data.light_index_face_index >> 16;
  uint face_index = ( shared_data.light_index_face_index & 0xf );
  int layer_index = int( 4 * light_index + face_index );

  mat4 model_to_world = mesh_instance_draws[ mesh_instance_draw_index ].model_to_world;

  SetMeshOutputsEXT( vertices_count, triangles_count );
  
  for ( uint i = task_index; i < vertices_count; i += gl_WorkGroupSize.x )
  {
    uint vertex_index = vertices_indices[ i + vertices_offset ];
    vec4 model_position = vec4( vertices[ vertex_index ].position, 1.0 );
    
    vec4 world_position = model_position * model_to_world;
    gl_MeshVerticesEXT[ i ].gl_Position = world_position * pointlight_world_to_clip[ layer_index ];
    
    float clip_distances[ 3 ];
    vec3 light_position = pointlight_spheres[ light_index ].xyz;
    for( uint side_index = 0; side_index < 3; side_index++ )
    {
      clip_distances[ side_index ] = get_clip_distance( light_position, world_position.xyz, 3 * face_index + side_index );
    }
    gl_MeshVerticesEXT[ i ].gl_ClipDistance[ 0 ] = clip_distances[ 0 ]; 
    gl_MeshVerticesEXT[ i ].gl_ClipDistance[ 1 ] = clip_distances[ 1 ];
    gl_MeshVerticesEXT[ i ].gl_ClipDistance[ 2 ] = clip_distances[ 2 ];
  }

  for ( uint i = task_index; i < triangles_count; i += gl_WorkGroupSize.x )
  {
    uint triangle_index = uint( 3 * i + triangles_offset );
    gl_PrimitiveTriangleIndicesEXT[ i ] = uvec3( triangles_indices[ triangle_index ], triangles_indices[ triangle_index + 1 ], triangles_indices[ triangle_index + 2 ] );
  }
}
#endif /* CRUDE_STAGE_MESH */
#endif /* POINTSHADOW */

#if defined( POINTSHADOW_CULLING  )
layout(local_size_x=32, local_size_y=1, local_size_z=1) in;

CRUDE_UNIFORM( SceneConstant, 0 )
{
  crude_scene                                              scene;
};

CRUDE_RBUFFER( MeshDraws, 1 )
{
  crude_mesh_draw                                          mesh_draws[];
};

CRUDE_RBUFFER( MeshInstancesDraws, 2 )
{
  crude_mesh_instance_draw                                 mesh_instance_draws[];
};

CRUDE_RBUFFER( MeshBounds, 3 )
{
  vec4                                                     mesh_bounds[];
};

CRUDE_RBUFFER( Meshlets, 4 )
{
  crude_meshlet                                            meshlets[];
};

CRUDE_RWBUFFER( PointLightMeshletInstances, 5 )
{
  uvec2                                                    meshletes_instances[];
};

CRUDE_RWBUFFER( PointLightMeshletCount, 6 )
{
  uint                                                     pointshadow_meshletes_instances_count[];
};

CRUDE_RBUFFER( Lights, 7 )
{
  crude_light                                              lights[];
};

void main()
{
  uint light_index = gl_GlobalInvocationID.x % scene.active_lights_count;
  if ( light_index >= scene.active_lights_count )
  {
    return;
  }

  const crude_light light = lights[ light_index ];

  uint mesh_instance_index = gl_GlobalInvocationID.x / scene.active_lights_count;
  if ( mesh_instance_index >= scene.mesh_instances_count )
  {
    return;
  }
  
  uint mesh_draw_index = mesh_instance_draws[ mesh_instance_index ].mesh_draw_index;

  crude_mesh_draw mesh_draw = mesh_draws[ mesh_draw_index ];

  vec4 bounding_sphere = mesh_bounds[ mesh_draw_index ];
  mat4 model_to_world = mesh_instance_draws[ mesh_instance_index ].model_to_world;

  vec4 mesh_world_bounding_center = vec4( bounding_sphere.xyz, 1 ) * model_to_world;

  float scale = max( model_to_world[ 0 ][ 0 ], max( model_to_world[ 1 ][ 1 ], model_to_world[ 2 ][ 2 ] ) );
  float mesh_radius = bounding_sphere.w * scale * 1.1;

  const bool mesh_intersects_sphere = crude_sphere_intersect( mesh_world_bounding_center.xyz, mesh_radius, light.world_position, light.radius );
  if ( !mesh_intersects_sphere )
  {
    return;
  }


  for ( uint i = 0; i < mesh_draw.meshletes_count; ++i )
  {
    uint meshlet_index = mesh_draw.meshletes_offset + i;
    float meshlet_radius = meshlets[ meshlet_index ].radius * scale * 1.1;
    vec4 meshlet_world_center = vec4( meshlets[ meshlet_index ].center, 1 ) * model_to_world;

    if ( crude_sphere_intersect( meshlet_world_center.xyz, meshlet_radius, light.world_position, light.radius ) )
    {
      uint offset = atomicAdd( pointshadow_meshletes_instances_count[ light_index ], 1 );
      meshletes_instances[ light_index * CRUDE_MAX_MESHLETS_PER_LIGHT + offset ] = uvec2( mesh_instance_index, meshlet_index );
    }
  }
}

#endif /* POINTSHADOW_CULLING */

#if defined ( POINTSHADOW_COMMANDS_GENERATION )
layout(local_size_x=32, local_size_y=1, local_size_z=1) in;

CRUDE_UNIFORM( SceneConstant, 0 )
{
  crude_scene                                              scene;
};

CRUDE_RWBUFFER( PointLightMeshletCount, 1 )
{
  uint                                                     pointshadow_meshletes_instances_count[];
};

CRUDE_RWBUFFER( PointLightMeshletDrawCommands, 2 )
{
  uvec4                                                    pointshadow_meshlet_draw_commands[];
};

void main()
{
  uint light_index = gl_GlobalInvocationID.x;
  if ( light_index >= scene.active_lights_count )
  {
    return;
  }

  const uint visible_meshlets = pointshadow_meshletes_instances_count[ light_index ];

  if ( visible_meshlets > 0 )
  {
    const uint command_offset = atomicAdd( pointshadow_meshletes_instances_count[ CRUDE_LIGHTS_MAX_COUNT ], 4 );
    uint packed_light_index = ( light_index & 0xffff ) << 16;
    pointshadow_meshlet_draw_commands[ command_offset ] = uvec4( ( ( visible_meshlets + 31 ) / 32 ), 1, 1, packed_light_index | 0 );
    pointshadow_meshlet_draw_commands[ command_offset + 1 ] = uvec4( ( ( visible_meshlets + 31 ) / 32 ), 1, 1, packed_light_index | 1 );
    pointshadow_meshlet_draw_commands[ command_offset + 2 ] = uvec4( ( ( visible_meshlets + 31 ) / 32 ), 1, 1, packed_light_index | 2 );
    pointshadow_meshlet_draw_commands[ command_offset + 3 ] = uvec4( ( ( visible_meshlets + 31 ) / 32 ), 1, 1, packed_light_index | 3 );
  }
}

#endif /* POINTSHADOW_COMMANDS_GENERATION */