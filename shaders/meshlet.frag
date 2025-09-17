
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/meshlet.glsli"
#include "crude/mesh.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

layout(location = 0) out vec4 out_abledo;
layout(location = 1) out vec2 out_normal;
layout(location = 2) out vec2 out_roughness_metalness;

layout(location=0) in vec2 in_texcoord0;
layout(location=1) in flat uint in_mesh_draw_index;
layout(location=2) in vec3 in_normal;
layout(location=3) in vec3 in_tangent;
layout(location=4) in vec3 in_bitangent;
layout(location=5) in vec3 in_world_position;

layout(set=CRUDE_MATERIAL_SET, binding=10) readonly buffer VisibleMeshCount
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

layout(set=CRUDE_MATERIAL_SET, binding=11, row_major, std430) readonly buffer MeshDrawCommands
{
  crude_mesh_draw_command                                  mesh_draw_commands[];
};

#define CRUDE_DRAW_FLAGS_HAS_NORMAL ( 1 << 4 )
#define CRUDE_DRAW_FLAGS_HAS_TANGENTS ( 1 << 8 )

void crude_calculate_geometric_tbn
(
  inout vec3                                               vertex_normal,
  inout vec3                                               vertex_tangent,
  inout vec3                                               vertex_bitangent,
  in vec2                                                  uv,
  in vec3                                                  vertex_world_position,
  in uint                                                  flags
)
{
  vec3 normal = normalize( vertex_normal );
  
  if ( ( flags & CRUDE_DRAW_FLAGS_HAS_NORMAL ) == 0 )
  {
    normal = normalize( cross( dFdx( vertex_world_position ), dFdy( vertex_world_position ) ) );
  }

  vec3 tangent = normalize( vertex_tangent );
  vec3 bitangent = normalize( vertex_bitangent );
  if ( ( flags & CRUDE_DRAW_FLAGS_HAS_TANGENTS ) == 0 )
  {
    vec3 uv_dx = dFdx( vec3( uv, 0.0 ) );
    vec3 uv_dy = dFdy( vec3( uv, 0.0 ) );

    vec3 t_ = ( uv_dy.t * dFdx( vertex_world_position ) - uv_dx.t * dFdy( vertex_world_position ) ) / ( uv_dx.s * uv_dy.t - uv_dy.s * uv_dx.t );
    tangent = normalize( t_ - normal * dot( normal, t_ ) );
    bitangent = cross( normal, tangent );
  }

  if ( !gl_FrontFacing )
  {
    tangent *= -1.0;
    bitangent *= -1.0;
    normal *= -1.0;
  }

  vertex_normal = normal;
  vertex_tangent = tangent;
  vertex_bitangent = bitangent;
}

void main()
{
  crude_mesh_draw mesh_draw = mesh_draws[ in_mesh_draw_index ];

  vec4 albedo = mesh_draw.albedo_color_factor;
  if ( mesh_draw.textures.x != CRUDE_TEXTURE_INVALID )
  {
    albedo = pow( texture( global_textures[ nonuniformEXT( mesh_draw.textures.x ) ], in_texcoord0 ) * albedo, vec4( 2.2 ) );
  }

  vec2 roughness_metalness = mesh_draw.metallic_roughness_occlusion_factor.yx;
  if ( mesh_draw.textures.y != CRUDE_TEXTURE_INVALID )
  {
    roughness_metalness = texture( global_textures[ nonuniformEXT( mesh_draw.textures.y ) ], in_texcoord0 ).gb;
  }
  
  vec3 normal = normalize( in_normal );
  vec3 tangent = normalize( in_tangent );
  vec3 bitangent = normalize( in_bitangent );

  crude_calculate_geometric_tbn( normal, tangent, bitangent, in_texcoord0, in_world_position, mesh_draw.flags );

  if ( mesh_draw.textures.z != CRUDE_TEXTURE_INVALID )
  {
    const vec3 bump_normal = normalize( texture( global_textures[ nonuniformEXT( mesh_draw.textures.z ) ], in_texcoord0 ).rgb * 2.0 - 1.0 );
    const mat3 tbn = mat3( tangent, bitangent, normal );
    normal = normalize( tbn * normalize( bump_normal ) );
  }

  out_abledo = albedo;
  out_normal = crude_octahedral_encode( normal );
  out_roughness_metalness = roughness_metalness;
}