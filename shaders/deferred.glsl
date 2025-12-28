
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
//#define TRANSPARENT_NO_CULL_CLASSIC
//#define DEFERRED_MESHLET
#define DEFERRED_CLASSIC
//#define TRANSPARENT_NO_CULL
//#define CRUDE_STAGE_MESH
#define CRUDE_STAGE_VERTEX
//#define CRUDE_STAGE_FRAGMENT
//#define CRUDE_STAGE_FRAGMENT
#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/culling.glsli"
#include "crude/light.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

CRUDE_RBUFFER_REF( VisibleMeshCountRef )
{
  uint                                                     opaque_mesh_visible_early_count;
  uint                                                     opaque_mesh_visible_late_count;
  uint                                                     opaque_mesh_culled_count;
  uint                                                     transparent_mesh_visible_count;

  uint                                                     transparent_mesh_culled_count;
  uint                                                     total_mesh_count;
  uint                                                     depth_pyramid_texture_index;
  uint                                                     meshlet_index_count;

  uint                                                     dispatch_task_x;
  uint                                                     dispatch_task_y;
  uint                                                     dispatch_task_z;
  uint                                                     meshlet_instances_count;
};

#if defined( DEFERRED_CLASSIC ) 
CRUDE_PUSH_CONSTANT
{
  SceneRef                                                 scene;
  MeshDrawsRef                                             mesh_draws;

  MeshInstancesDrawsRef                                    mesh_instance_draws;
  VisibleMeshCountRef                                      visible_mesh_count;

  MeshDrawCommandsRef                                      mesh_draw_commands;
  vec2                                                     _padding;
};
#elif defined( TRANSPARENT_NO_CULL_CLASSIC ) 
CRUDE_PUSH_CONSTANT
{
  SceneRef                                                 scene;
  MeshDrawsRef                                             mesh_draws;

  MeshInstancesDrawsRef                                    mesh_instance_draws;
  VisibleMeshCountRef                                      visible_mesh_count;

  MeshDrawCommandsRef                                      mesh_draw_commands;
  LightsZBinsRef                                           zbins;

  LightsTilesRef                                           lights_tiles;
  LightsTrianglesIndicesRef                                lights_indices;

  LightsRef                                                lights;
  LightsShadowViewsRef                                     light_shadow_views;

  float                                                    inv_radiance_texture_width;
  float                                                    inv_radiance_texture_height;
  vec2                                                     _pust_constant_padding;
};
#elif defined( DEFERRED_MESHLET )
CRUDE_PUSH_CONSTANT
{
  SceneRef                                                 scene;
  MeshDrawsRef                                             mesh_draws;

  MeshInstancesDrawsRef                                    mesh_instance_draws;
  MeshletsRef                                              meshlets;

  VerticesRef                                              vertices;
  TrianglesIndicesRef                                      triangles_indices;

  VerticesIndicesRef                                       vertices_indices;
  VisibleMeshCountRef                                      visible_mesh_count;

  MeshDrawCommandsRef                                      mesh_draw_commands;
  vec2                                                     _pust_constant_padding;
};
#elif defined( TRANSPARENT_NO_CULL )
CRUDE_PUSH_CONSTANT
{
  SceneRef                                                 scene;
  MeshDrawsRef                                             mesh_draws;

  MeshInstancesDrawsRef                                    mesh_instance_draws;
  MeshletsRef                                              meshlets;

  VerticesRef                                              vertices;
  TrianglesIndicesRef                                      triangles_indices;

  VerticesIndicesRef                                       vertices_indices;
  VisibleMeshCountRef                                      visible_mesh_count;

  MeshDrawCommandsRef                                      mesh_draw_commands;
  LightsZBinsRef                                           zbins;

  LightsTilesRef                                           lights_tiles;
  LightsTrianglesIndicesRef                                lights_indices;

  LightsRef                                                lights;
  LightsShadowViewsRef                                     light_shadow_views;

  float                                                    inv_radiance_texture_width;
  float                                                    inv_radiance_texture_height;
  vec2                                                     _pust_constant_padding;
};
#endif

#if defined( DEFERRED_MESHLET ) || defined ( TRANSPARENT_NO_CULL ) 

#extension GL_ARB_shader_draw_parameters : require
#extension GL_KHR_shader_subgroup_ballot: require

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
#if defined( DEFERRED_MESHLET )
  uint draw_id = mesh_draw_commands.data[ gl_DrawIDARB ].draw_id;
#else /* TRANSPARENT_NO_CULL */
  uint draw_id = mesh_draw_commands.data[ gl_DrawIDARB + visible_mesh_count.total_mesh_count ].draw_id;
#endif
  uint mesh_draw_index = mesh_instance_draws.data[ draw_id ].mesh_draw_index;
  uint meshlet_index = mesh_draws.data[ mesh_draw_index ].meshletes_offset + gl_GlobalInvocationID.x;

  if ( meshlet_index >= mesh_draws.data[ mesh_draw_index ].meshletes_offset + mesh_draws.data[ mesh_draw_index ].meshletes_count )
  {
    return;
  }

  mat4 mesh_to_world = mesh_instance_draws.data[ draw_id ].mesh_to_world;
  vec4 world_center = vec4( meshlets.data[ meshlet_index ].center, 1 ) * mesh_to_world;
  float scale = crude_calculate_scale_from_matrix( mat3( mesh_to_world ) );
  float radius = meshlets.data[ meshlet_index ].radius * scale * 1.1;

  vec3 cone_axis = vec3(
    int( meshlets.data[ meshlet_index ].cone_axis[ 0 ] ) / 127.0,
    int( meshlets.data[ meshlet_index ].cone_axis[ 1 ]) / 127.0,
    int( meshlets.data[ meshlet_index ].cone_axis[ 2 ]) / 127.0 ) * mat3( mesh_to_world );
  float cone_cutoff = int( meshlets.data[ meshlet_index ].cone_cutoff ) / 127.0;
  
  bool accept = true;

#if defined( DEFERRED_MESHLET )
  accept = true;//!crude_clustered_backface_culling( world_center.xyz, radius, cone_axis, cone_cutoff, scene.data.camera.position );
  
  vec4 view_center = world_center * scene.data.camera.world_to_view;

  bool frustum_visible = accept;
  for ( uint i = 0; i < 6; ++i )
  {
    frustum_visible = frustum_visible && ( dot( scene.data.camera.frustum_planes_culling[ i ], view_center ) > -radius );
  }
  
  bool occlusion_visible = false;
  if ( frustum_visible )
  {
    occlusion_visible = crude_occlusion_culling( mesh_draw_index, view_center.xyz, radius, scene.data.camera.znear,
      scene.data.camera.view_to_clip[ 0 ][ 0 ], scene.data.camera.view_to_clip[ 1 ][ 1 ],
      visible_mesh_count.depth_pyramid_texture_index,
      world_center.xyz, scene.data.camera.position, scene.data.camera.world_to_clip
    );
  }
  accept = occlusion_visible;
#endif

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
  uint global_meshlet_index = shared_data.meshlet_indices[ local_meshlet_index ];
  uint mesh_instance_draw_index = shared_data.mesh_instance_draw_indices[ local_meshlet_index ];
  uint mesh_draw_index = mesh_instance_draws.data[ mesh_instance_draw_index ].mesh_draw_index;
  
  uint mesh_index = meshlets.data[ global_meshlet_index ].mesh_index;
  uint vertices_count = uint( meshlets.data[ global_meshlet_index ].vertices_count );
  uint triangles_count = uint( meshlets.data[ global_meshlet_index ].triangles_count );
  
  uint vertices_offset = meshlets.data[ global_meshlet_index ].vertices_offset;
  uint triangles_offset = meshlets.data[ global_meshlet_index ].triangles_offset;

  mat4 mesh_to_world = mesh_instance_draws.data[ mesh_instance_draw_index ].mesh_to_world;

  SetMeshOutputsEXT( vertices_count, triangles_count );
  
  float i8_inverse = 1.0 / 127.0;
  for ( uint i = task_index; i < vertices_count; i += gl_WorkGroupSize.x )
  {
    uint vertex_index = vertices_indices.data[ i + vertices_offset ];
    vec4 model_position = vec4( vertices.data[ vertex_index ].position, 1.0 );
    vec4 world_position = model_position * mesh_to_world;
    vec4 view_position = world_position * scene.data.camera.world_to_view;
    
    vec4 tangent = vec4( int( vertices.data[ vertex_index ].tx ), int( vertices.data[ vertex_index ].ty ), int( vertices.data[ vertex_index ].tz ),  int( vertices.data[ vertex_index ].tw ) ) * i8_inverse - 1.0;
    vec3 normal = vec3( int( vertices.data[ vertex_index ].nx ), int( vertices.data[ vertex_index ].ny ), int( vertices.data[ vertex_index ].nz ) ) * i8_inverse - 1.0;
    normal = normal * mat3( mesh_to_world );
    tangent.xyz = tangent.xyz * mat3( mesh_to_world );
    vec3 bitangent = cross( normal, tangent.xyz ) * tangent.w;

    vec3 view_normal = normal * mat3( scene.data.camera.world_to_view );

    gl_MeshVerticesEXT[ i ].gl_Position = world_position * scene.data.camera.world_to_clip;
    out_texcoord[ i ] = vec2( vertices.data[ vertex_index ].tu, vertices.data[ vertex_index ].tv );
    out_normals[ i ] = normal;
    out_tangents[ i ] = tangent.xyz;
    out_bitangents[ i ] = bitangent;
    out_mesh_draw_index[ i ] = mesh_draw_index;
    out_world_positions[ i ] = world_position.xyz;
    out_view_surface_normal[ i ] = view_normal;
    out_view_position[ i ] = scene.data.camera.position - world_position.xyz;
  }
  
  for ( uint i = task_index; i < triangles_count; i += gl_WorkGroupSize.x )
  {
    uint triangle_index = uint( 3 * i + triangles_offset );
    gl_PrimitiveTriangleIndicesEXT[ i ] = uvec3( triangles_indices.data[ triangle_index ], triangles_indices.data[ triangle_index + 1 ], triangles_indices.data[ triangle_index + 2 ] );
  }
}
#endif /* CRUDE_STAGE_MESH */
#endif /* DEFERRED_MESHLET || TRANSPARENT_NO_CULL */

#if defined( CRUDE_STAGE_VERTEX ) && ( defined( DEFERRED_CLASSIC ) || defined( TRANSPARENT_NO_CULL_CLASSIC ) )

layout(location=0) out vec2 out_texcoord;
layout(location=1) out flat uint out_mesh_draw_index;
layout(location=2) out vec3 out_normal;
layout(location=3) out vec3 out_tangent;
layout(location=4) out vec3 out_bitangent;
layout(location=5) out vec3 out_world_position;
layout(location=6) out vec3 out_view_surface_normal;
layout(location=7) out vec3 out_view_position;

void main()
{
#if defined( DEFERRED_CLASSIC )
  uint draw_id = mesh_draw_commands.data[ gl_DrawIDARB ].draw_id;
#else /* TRANSPARENT_NO_CULL_CLASSIC */
  uint draw_id = mesh_draw_commands.data[ gl_DrawIDARB + visible_mesh_count.total_mesh_count ].draw_id;
#endif
  uint mesh_draw_index = mesh_instance_draws.data[ draw_id ].mesh_draw_index;
  crude_mesh_draw mesh_draw = mesh_draws.data[ mesh_draw_index ];

  mat4 mesh_to_world = mesh_instance_draws.data[ draw_id ].mesh_to_world;

  uint vertex_index = 0;
  if ( ( ( mesh_draw.flags & CRUDE_DRAW_FLAGS_INDEX_16 ) == 0 ) )
  {
    vertex_index = MeshIndices32Ref( mesh_draw.indices ).data[ gl_VertexIndex ];
  }
  else
  {
    vertex_index = MeshIndices16Ref( mesh_draw.indices ).data[ gl_VertexIndex ];
  }

  vec4 model_position = vec4(
    mesh_draw.positions.data[ vertex_index + 0 ],
    1.0 );
  vec4 world_position = model_position * mesh_to_world;
  vec4 view_position = world_position * scene.data.camera.world_to_view;
  
  vec4 tangent = ( ( mesh_draw.flags & CRUDE_DRAW_FLAGS_HAS_TANGENTS ) == 1 ) ? vec4(
    mesh_draw.tangents.data[ vertex_index + 0 ]) : vec4( 0 );
  vec3 normal = ( ( mesh_draw.flags & CRUDE_DRAW_FLAGS_HAS_NORMAL ) == 1 ) ? vec3(
    mesh_draw.normals.data[ vertex_index ] )  : vec3( 0 );
  normal = normal * mat3( mesh_to_world );
  tangent.xyz = tangent.xyz * mat3( mesh_to_world );
  vec3 bitangent = cross( normal, tangent.xyz ) * tangent.w;
  vec3 view_normal = normal * mat3( scene.data.camera.world_to_view );

  gl_Position = world_position * scene.data.camera.world_to_clip;
  out_texcoord = vec2( mesh_draw.texcoords.data[ 2 * vertex_index ], mesh_draw.texcoords.data[ 2 * vertex_index + 1 ] );
  out_normal = normal;
  out_tangent = tangent.xyz;
  out_bitangent = bitangent;
  out_mesh_draw_index = mesh_draw_index;
  out_world_position = world_position.xyz;
  out_view_surface_normal = view_normal;
  out_view_position = scene.data.camera.position - world_position.xyz;
}
#endif /* CRUDE_STAGE_VERTEX && ( DEFERRED_CLASSIC || TRANSPARENT_NO_CULL_CLASSIC ) */

#if defined( DEFERRED_MESHLET ) || defined( DEFERRED_CLASSIC )
#if defined( CRUDE_STAGE_FRAGMENT )

layout(location = 0) out vec4 out_abledo;
layout(location = 1) out vec2 out_normal;
layout(location = 2) out vec2 out_roughness_metalness;
#if CRUDE_DEVELOP
layout(location = 3) out uint out_mesh_draw_index;
#endif

layout(location=0) in vec2 in_texcoord0;
layout(location=1) in flat uint in_mesh_draw_index;
layout(location=2) in vec3 in_normal;
layout(location=3) in vec3 in_tangent;
layout(location=4) in vec3 in_bitangent;
layout(location=5) in vec3 in_world_position;

void main()
{
  crude_mesh_draw mesh_draw = mesh_draws.data[ in_mesh_draw_index ];

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
#if CRUDE_DEVELOP
  out_mesh_draw_index = in_mesh_draw_index;
#endif
}

#endif /* CRUDE_STAGE_FRAGMENT */
#endif /* DEFERRED_MESHLET || DEFERRED_CLASSIC*/

#if defined( TRANSPARENT_NO_CULL ) || defined( TRANSPARENT_NO_CULL_CLASSIC )
#if defined( CRUDE_STAGE_FRAGMENT )

layout(location = 0) out vec4 out_radiance;

layout(location=0) in vec2 in_texcoord0;
layout(location=1) in flat uint in_mesh_draw_index;
layout(location=2) in vec3 in_normal;
layout(location=3) in vec3 in_tangent;
layout(location=4) in vec3 in_bitangent;
layout(location=5) in vec3 in_world_position;

void main()
{
  crude_mesh_draw mesh_draw = mesh_draws.data[ in_mesh_draw_index ];

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

  uvec2 screen_position = uvec2( gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5 );
  float depth = gl_FragCoord.z;
  vec2 screen_texcoord = vec2( gl_FragCoord.x * inv_radiance_texture_width, gl_FragCoord * inv_radiance_texture_height );
  
  vec3 radiance = vec3( 0.f, 0.f, 0.f );
  if ( depth != 1.f )
  {
    radiance = crude_calculate_lighting( 
      albedo, roughness_metalness.x, roughness_metalness.y, normal, in_world_position, scene.data.camera.position, screen_position, screen_texcoord,
      scene, zbins, lights_tiles, lights_indices, lights, light_shadow_views );
  }
  else
  {
    radiance = scene.data.background_color * scene.data.background_intensity;
  }

  out_radiance = vec4( radiance, sqrt( albedo.a ) );
}

#endif /* CRUDE_STAGE_FRAGMENT */
#endif /* TRANSPARENT_NO_CULL */