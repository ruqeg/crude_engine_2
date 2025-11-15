
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
//#define DEFERRED_MESHLET
#define COLLISION_VISUALIZER
#define CRUDE_STAGE_TASK

#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/culling.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#if defined( DEFERRED_MESHLET ) 

#extension GL_ARB_shader_draw_parameters : require
#extension GL_KHR_shader_subgroup_ballot: require

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

CRUDE_RBUFFER( Meshlets, 3 )
{
  crude_meshlet                                            meshlets[];
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

CRUDE_RBUFFER( VisibleMeshCount, 7 )
{
  uint                                                     opaque_mesh_visible_count;
  uint                                                     opaque_mesh_culled_count;
  uint                                                     transparent_mesh_visible_count;
  uint                                                     transparent_mesh_culled_count;

  uint                                                     total_count;
  uint                                                     depth_pyramid_texture_index;
  uint                                                     occlusion_culling_late_flag;
  uint                                                     meshlet_index_count;

  uint                                                     dispatch_task_x;
  uint                                                     dispatch_task_y;
  uint                                                     dispatch_task_z;
  uint                                                     meshlet_instances_count;
};

CRUDE_RBUFFER( MeshDrawCommands, 8 )
{
  crude_mesh_draw_command                                  mesh_draw_commands[];
};


#if defined( CRUDE_STAGE_TASK ) || defined( CRUDE_STAGE_MESH )
taskPayloadSharedEXT struct
{
  uint                                                     meshlet_indices[ 128 ];
  uint                                                     mesh_instance_draw_indices[ 128 ];
} shared_data;
#endif

#if defined( CRUDE_STAGE_TASK )

layout(local_size_x=32) in;

void main()
{
  uint draw_id = mesh_draw_commands[ gl_DrawIDARB ].draw_id;
  uint mesh_draw_index = mesh_instance_draws[ draw_id ].mesh_draw_index;
  uint meshlet_index = mesh_draws[ mesh_draw_index ].meshletes_offset + gl_GlobalInvocationID.x;

  if ( meshlet_index >= mesh_draws[ mesh_draw_index ].meshletes_offset + mesh_draws[ mesh_draw_index ].meshletes_count )
  {
    return;
  }

  mat4 mesh_to_world = mesh_instance_draws[ draw_id ].mesh_to_world;
  vec4 world_center = vec4( meshlets[ meshlet_index ].center, 1 ) * mesh_to_world;
  float scale = max( mesh_to_world[ 0 ][ 0 ], max( mesh_to_world[ 1 ][ 1 ], mesh_to_world[ 2 ][ 2 ] ) );
  float radius = meshlets[ meshlet_index ].radius * scale * 1.1;

  vec3 cone_axis = vec3(
    int( meshlets[ meshlet_index ].cone_axis[ 0 ] ) / 127.0,
    int( meshlets[ meshlet_index ].cone_axis[ 1 ]) / 127.0,
    int( meshlets[ meshlet_index ].cone_axis[ 2 ]) / 127.0 ) * mat3( mesh_to_world );
  float cone_cutoff = int( meshlets[ meshlet_index ].cone_cutoff ) / 127.0;
  
  bool accept = true;

  accept = !crude_clustered_backface_culling( world_center.xyz, radius, cone_axis, cone_cutoff, scene.camera.position );
  
  vec4 view_center = world_center * scene.camera.world_to_view;

  bool frustum_visible = true;
  for ( uint i = 0; i < 6; ++i )
  {
    frustum_visible = frustum_visible && ( dot( scene.camera.frustum_planes_culling[ i ], view_center ) > -radius );
  }
  accept = accept && frustum_visible;
  
  bool occlusion_visible = false;
  if ( frustum_visible )
  {
    occlusion_visible = crude_occlusion_culling( mesh_draw_index, view_center.xyz, radius, scene.camera.znear,
      scene.camera.view_to_clip[ 0 ][ 0 ], scene.camera.view_to_clip[ 1 ][ 1 ],
      depth_pyramid_texture_index,
      world_center.xyz, scene.camera.position, scene.camera.world_to_clip
    );
  }
  occlusion_visible = true; //!TODO fix
  accept = accept && occlusion_visible;

  uvec4 ballot = subgroupBallot( accept );
  
  uint previous_visible_meshlet_index = subgroupBallotExclusiveBitCount( ballot );
  if ( accept )
  {
    shared_data.meshlet_indices[ previous_visible_meshlet_index ] = meshlet_index;
    shared_data.mesh_instance_draw_indices[ previous_visible_meshlet_index ] = draw_id;
  }
  
  uint visible_meslets_count = subgroupBallotBitCount( ballot );
  if ( gl_LocalInvocationID.x == 0 )
  {
    EmitMeshTasksEXT( visible_meslets_count, 1, 1 );
  }
}
#endif /* CRUDE_STAGE_TASK */

#if /* defined( DEFERRED_MESHLET ) && */ defined( CRUDE_STAGE_MESH )
layout(local_size_x=128, local_size_y=1, local_size_z=1) in;
layout(triangles) out;
layout(max_vertices=128, max_primitives=124) out;

layout(location=0) out vec2 out_texcoord[];
layout(location=1) out flat uint out_mesh_draw_index[];
layout(location=2) out vec3 out_normals[];
layout(location=3) out vec3 out_tangents[];
layout(location=4) out vec3 out_bitangents[];
layout(location=5) out vec3 out_world_positions[];
layout(location=6) out vec3 out_view_surface_normal[];
layout(location=7) out vec3 out_view_position[];

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

  mat4 mesh_to_world = mesh_instance_draws[ mesh_instance_draw_index ].mesh_to_world;

  SetMeshOutputsEXT( vertices_count, triangles_count );
  
  float i8_inverse = 1.0 / 127.0;
  for ( uint i = task_index; i < vertices_count; i += gl_WorkGroupSize.x )
  {
    uint vertex_index = vertices_indices[ i + vertices_offset ];
    vec4 model_position = vec4( vertices[ vertex_index ].position, 1.0 );
    vec4 world_position = model_position * mesh_to_world;
    vec4 view_position = world_position * scene.camera.world_to_view;
    
    vec4 tangent = vec4( int( vertices[ vertex_index ].tx ), int( vertices[ vertex_index ].ty ), int( vertices[ vertex_index ].tz ),  int( vertices[ vertex_index ].tw ) ) * i8_inverse - 1.0;
    vec3 normal = vec3( int( vertices[ vertex_index ].nx ), int( vertices[ vertex_index ].ny ), int( vertices[ vertex_index ].nz ) ) * i8_inverse - 1.0;
    normal = normal * mat3( mesh_to_world );
    tangent.xyz = tangent.xyz * mat3( mesh_to_world );
    vec3 bitangent = cross( normal, tangent.xyz ) * tangent.w;

    vec3 view_normal = normal * mat3( scene.camera.world_to_view );

    gl_MeshVerticesEXT[ i ].gl_Position = world_position * scene.camera.world_to_clip;
    out_texcoord[ i ] = vec2( vertices[ vertex_index ].tu, vertices[ vertex_index ].tv );
    out_normals[ i ] = normal;
    out_tangents[ i ] = tangent.xyz;
    out_bitangents[ i ] = bitangent;
    out_mesh_draw_index[ i ] = mesh_draw_index;
    out_world_positions[ i ] = world_position.xyz;
    out_view_surface_normal[ i ] = view_normal;
    out_view_position[ i ] = scene.camera.position - world_position.xyz;
  }
  
  for ( uint i = task_index; i < triangles_count; i += gl_WorkGroupSize.x )
  {
    uint triangle_index = uint( 3 * i + triangles_offset );
    gl_PrimitiveTriangleIndicesEXT[ i ] = uvec3( triangles_indices[ triangle_index ], triangles_indices[ triangle_index + 1 ], triangles_indices[ triangle_index + 2 ] );
  }
}
#endif /* CRUDE_STAGE_MESH */


#if defined( CRUDE_STAGE_FRAGMENT )
layout(location = 0) out vec4 out_abledo;
layout(location = 1) out vec2 out_normal;
layout(location = 2) out vec2 out_roughness_metalness;

layout(location=0) in vec2 in_texcoord0;
layout(location=1) in flat uint in_mesh_draw_index;
layout(location=2) in vec3 in_normal;
layout(location=3) in vec3 in_tangent;
layout(location=4) in vec3 in_bitangent;
layout(location=5) in vec3 in_world_position;

void main()
{
  crude_mesh_draw mesh_draw = mesh_draws[ in_mesh_draw_index ];

  vec4 albedo = mesh_draw.albedo_color_factor;
  if ( mesh_draw.textures.x != CRUDE_GRAPHICS_SHADER_TEXTURE_UNDEFINED )
  {
    albedo = pow( texture( global_textures[ nonuniformEXT( mesh_draw.textures.x ) ], in_texcoord0 ) * albedo, vec4( 2.2 ) );
  }

  vec2 roughness_metalness = mesh_draw.metallic_roughness_occlusion_factor.yx;
  if ( mesh_draw.textures.y != CRUDE_GRAPHICS_SHADER_TEXTURE_UNDEFINED )
  {
    roughness_metalness = texture( global_textures[ nonuniformEXT( mesh_draw.textures.y ) ], in_texcoord0 ).gb;
  }
  
  vec3 normal = normalize( in_normal );
  vec3 tangent = normalize( in_tangent );
  vec3 bitangent = normalize( in_bitangent );

  crude_calculate_geometric_tbn( normal, tangent, bitangent, in_texcoord0, in_world_position, mesh_draw.flags );

  if ( mesh_draw.textures.z != CRUDE_GRAPHICS_SHADER_TEXTURE_UNDEFINED )
  {
    const vec3 bump_normal = normalize( texture( global_textures[ nonuniformEXT( mesh_draw.textures.z ) ], in_texcoord0 ).rgb * 2.0 - 1.0 );
    const mat3 tbn = mat3( tangent, bitangent, normal );
    normal = normalize( tbn * normalize( bump_normal ) );
  }

  out_abledo = albedo;
  out_normal = crude_octahedral_encode( normal );
  out_roughness_metalness = roughness_metalness;
}
#endif /* CRUDE_STAGE_FRAGMENT */
#endif /* DEFERRED_MESHLET */

#if defined( COLLISION_VISUALIZER ) 

#extension GL_ARB_shader_draw_parameters : require
#extension GL_KHR_shader_subgroup_ballot: require

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

CRUDE_RBUFFER( Meshlets, 3 )
{
  crude_meshlet                                            meshlets[];
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

#if defined( CRUDE_STAGE_TASK ) || defined( CRUDE_STAGE_MESH )
taskPayloadSharedEXT struct
{
  uint                                                     mesh_instance_draw_index;
  uint                                                     meshletes_offset;
} shared_data;
#endif

#if defined( CRUDE_STAGE_TASK )

layout(local_size_x=1) in;

void main()
{
  uint mesh_instance_draw_index = gl_GlobalInvocationID.x;
  uint mesh_draw_index = mesh_instance_draws[ mesh_instance_draw_index ].mesh_draw_index;
  
  shared_data.meshletes_offset = mesh_draws[ mesh_draw_index ].meshletes_offset;
  shared_data.mesh_instance_draw_index = mesh_instance_draw_index;

  if ( gl_LocalInvocationID.x == 0 )
  {
    EmitMeshTasksEXT( mesh_draws[ mesh_draw_index ].meshletes_count, 1, 1 );
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
layout(location=3) out vec3 out_tangents[];
layout(location=4) out vec3 out_bitangents[];
layout(location=5) out vec3 out_world_positions[];
layout(location=6) out vec3 out_view_surface_normal[];
layout(location=7) out vec3 out_view_position[];

void main()
{
  uint task_index = gl_LocalInvocationID.x;
  uint local_meshlet_index = gl_WorkGroupID.x;
  uint global_meshlet_index = local_meshlet_index + shared_data.meshletes_offset;
  uint mesh_instance_draw_index = shared_data.mesh_instance_draw_index;
  uint mesh_draw_index = mesh_instance_draws[ mesh_instance_draw_index ].mesh_draw_index;
  
  uint mesh_index = meshlets[ global_meshlet_index ].mesh_index;
  uint vertices_count = uint( meshlets[ global_meshlet_index ].vertices_count );
  uint triangles_count = uint( meshlets[ global_meshlet_index ].triangles_count );
  
  uint vertices_offset = meshlets[ global_meshlet_index ].vertices_offset;
  uint triangles_offset = meshlets[ global_meshlet_index ].triangles_offset;

  mat4 mesh_to_world = mesh_instance_draws[ mesh_instance_draw_index ].mesh_to_world;

  SetMeshOutputsEXT( vertices_count, triangles_count );
  
  float i8_inverse = 1.0 / 127.0;
  for ( uint i = task_index; i < vertices_count; i += gl_WorkGroupSize.x )
  {
    uint vertex_index = vertices_indices[ i + vertices_offset ];
    vec4 model_position = vec4( vertices[ vertex_index ].position, 1.0 );
    vec4 world_position = model_position * mesh_to_world;
    vec4 view_position = world_position * scene.camera.world_to_view;
    
    vec4 tangent = vec4( int( vertices[ vertex_index ].tx ), int( vertices[ vertex_index ].ty ), int( vertices[ vertex_index ].tz ),  int( vertices[ vertex_index ].tw ) ) * i8_inverse - 1.0;
    vec3 normal = vec3( int( vertices[ vertex_index ].nx ), int( vertices[ vertex_index ].ny ), int( vertices[ vertex_index ].nz ) ) * i8_inverse - 1.0;
    normal = normal * mat3( mesh_to_world );
    tangent.xyz = tangent.xyz * mat3( mesh_to_world );
    vec3 bitangent = cross( normal, tangent.xyz ) * tangent.w;

    vec3 view_normal = normal * mat3( scene.camera.world_to_view );

    gl_MeshVerticesEXT[ i ].gl_Position = world_position * scene.camera.world_to_clip;
    out_texcoord[ i ] = vec2( vertices[ vertex_index ].tu, vertices[ vertex_index ].tv );
    out_normals[ i ] = normal;
    out_tangents[ i ] = tangent.xyz;
    out_bitangents[ i ] = bitangent;
    out_mesh_draw_index[ i ] = mesh_draw_index;
    out_world_positions[ i ] = world_position.xyz;
    out_view_surface_normal[ i ] = view_normal;
    out_view_position[ i ] = scene.camera.position - world_position.xyz;
  }
  
  for ( uint i = task_index; i < triangles_count; i += gl_WorkGroupSize.x )
  {
    uint triangle_index = uint( 3 * i + triangles_offset );
    gl_PrimitiveTriangleIndicesEXT[ i ] = uvec3( triangles_indices[ triangle_index ], triangles_indices[ triangle_index + 1 ], triangles_indices[ triangle_index + 2 ] );
  }
}
#endif /* CRUDE_STAGE_MESH */


#if defined( CRUDE_STAGE_FRAGMENT )
layout(location = 0) out vec4 out_color;

layout(location=0) in vec2 in_texcoord0;
layout(location=1) in flat uint in_mesh_draw_index;
layout(location=2) in vec3 in_normal;
layout(location=3) in vec3 in_tangent;
layout(location=4) in vec3 in_bitangent;
layout(location=5) in vec3 in_world_position;

void main()
{
  out_color = vec4( 0, 1, 1, 0.3 );
}
#endif /* CRUDE_STAGE_FRAGMENT */
#endif /* COLLISION_VISUALIZER */