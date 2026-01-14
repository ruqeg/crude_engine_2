
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
//#define CRUDE_RAYGEN
#define RAY_TRACING_SOLID
#define CRUDE_CLOSEST_HIT
//#define CRUDE_STAGE_FRAGMENT

#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/culling.glsli"
#include "crude/light.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_tracing_position_fetch  : require

#if defined( RAY_TRACING_SOLID )

CRUDE_UNIFORM( SceneConstant, 0 ) 
{
  crude_scene                                              scene;
};

layout(set=CRUDE_MATERIAL_SET, binding=1) uniform accelerationStructureEXT as;

CRUDE_RBUFFER( MeshDraws, 2 )
{
  crude_mesh_draw                                          mesh_draws[];
};

CRUDE_RBUFFER( MeshInstancesDraws, 3 )
{
  crude_mesh_instance_draw                                 mesh_instance_draws[];
};

CRUDE_RBUFFER( RayParams, 4 )
{
  uint                                                     sbt_offset;
  uint                                                     sbt_stride;
  uint                                                     miss_index;
  uint                                                     out_image_index;
};

struct ray_payload
{
  vec3                                                     color;
  ivec2                                                    launch_id;
};

#if defined( CRUDE_RAYGEN )

layout(location=0) rayPayloadEXT ray_payload payload;

vec3
compute_ray_dir
(
  in uvec3                                      launchID,
  in uvec3                                      launchSize
)
{
  float x = ( 2 * ( float( launchID.x ) + 0.5 ) / float( launchSize.x ) - 1.0 );
  float y = ( 1.0 - 2 * ( float( launchID.y ) + 0.5 ) / float( launchSize.y ) );
  vec4 dir = vec4( x, y, 1, 1 ) * scene.camera.clip_to_world;
  dir = normalize( dir );
  return dir.xyz;
}

void main()
{
  payload.color = vec3( 0, 0, 1 );
  payload.launch_id = ivec2( gl_LaunchIDEXT.xy );

  traceRayEXT(
    as, /* topLevel */
    gl_RayFlagsOpaqueEXT, /* rayFlags */
    0xff, /* cullMask */
    sbt_offset, /* sbtRecordOffset */
    sbt_stride, /* sbtRecordStride */
    miss_index, /* missIndex */
    scene.camera.position, /* origin */
    0.0, /* tmin */
    compute_ray_dir( gl_LaunchIDEXT, gl_LaunchSizeEXT ), /* direction */
    100.0, /* Tmax */
    0 /* payload index */
  );

  imageStore( global_images_2d[ out_image_index ], ivec2( gl_LaunchIDEXT.xy ), vec4( payload.color, 1 ) );
}

#endif /* CRUDE_RAYGEN */

#if defined( CRUDE_CLOSEST_HIT )

layout(location=0) rayPayloadInEXT ray_payload payload;
hitAttributeEXT vec2 barycentric_weights;

void main()
{
  vec3 radiance = vec3( 0 );
  float distance = 0.0f;
  
  int_array_type                                         index_buffer;
  vec2_array_type                                        texcoord_buffer;
  float_array_type                                       normal_buffer;
  crude_mesh_draw                                        mesh_draw;
  mat4                                                   mesh_to_world, world_to_mesh;
  vec3                                                   normal, p0_model, p1_model, p2_model, n0_model, n1_model, n2_model, model_position, world_position, diffuse, albedo;
  vec2                                                   texcoord, texcoord0, texcoord1, texcoord2;
  float                                                  a, b, c, ndotl, attenuation;
  int                                                    i0, i1, i2;
  uint                                                   mesh_index;

  mesh_index = mesh_instance_draws[ gl_GeometryIndexEXT ].mesh_draw_index;
  mesh_draw = mesh_draws[ mesh_index ];

  world_to_mesh = mesh_instance_draws[ gl_GeometryIndexEXT ].world_to_mesh;
  mesh_to_world = mesh_instance_draws[ gl_GeometryIndexEXT ].mesh_to_world;

  index_buffer = int_array_type( mesh_draw.index_buffer );
  i0 = index_buffer[ gl_PrimitiveID * 3 ].v;
  i1 = index_buffer[ gl_PrimitiveID * 3 + 1 ].v;
  i2 = index_buffer[ gl_PrimitiveID * 3 + 2 ].v;

  p0_model = gl_HitTriangleVertexPositionsEXT[ 0 ];
  p1_model = gl_HitTriangleVertexPositionsEXT[ 1 ];
  p2_model = gl_HitTriangleVertexPositionsEXT[ 2 ];

  if ( ( mesh_draw.flags & CRUDE_DRAW_FLAGS_HAS_NORMAL ) == 0 )
  {
    normal_buffer = float_array_type( mesh_draw.normal_buffer );
    n0_model = vec3( normal_buffer[ i0 * 3 + 0 ].v, normal_buffer[ i0 * 3 + 1 ].v, normal_buffer[ i0 * 3 + 2 ].v );
    n1_model = vec3( normal_buffer[ i1 * 3 + 0 ].v, normal_buffer[ i1 * 3 + 1 ].v, normal_buffer[ i1 * 3 + 2 ].v );
    n2_model = vec3( normal_buffer[ i2 * 3 + 0 ].v, normal_buffer[ i2 * 3 + 1 ].v, normal_buffer[ i2 * 3 + 2 ].v );
    normal = normalize( a * n0_model + b * n1_model + c * n2_model );
  }
  else
  {
    normal = normalize( cross( p1_model - p0_model, p2_model - p1_model ) );
  }

  texcoord_buffer = vec2_array_type( mesh_draw.texcoord_buffer );
  texcoord0 = texcoord_buffer[ i0 ].v;
  texcoord1 = texcoord_buffer[ i1 ].v;
  texcoord2 = texcoord_buffer[ i2 ].v;

  b = barycentric_weights.x;
  c = barycentric_weights.y;
  a = 1 - b - c;
  
  model_position = a * p0_model + b * p1_model + c * p2_model;
  world_position = vec3( vec4( model_position, 1 ) * mesh_to_world );
  texcoord = ( a * texcoord0 + b * texcoord1 + c * texcoord2 );

  vec3 v = normalize( scene.camera.position.xyz - world_position );
  vec3 bias_vector = ( normal * 0.2f + v * 0.8f) * ( 0.75f * 1 ) * 0.3;
  vec3 biased_world_position = world_position + bias_vector;

  albedo = CRUDE_TEXTURE_LOD( mesh_draw.textures.x, texcoord, 0 ).rgb;

  payload.color = vec3( 0.5 * normal + 0.5 );

  if ( payload.launch_id == ivec2( 900, 500 ) )
  {
    //crude_debug_draw_line( world_position, biased_world_position, vec4( 1, 0, 0, 1 ), vec4( 1, 0, 0, 1 ) );
    //crude_debug_draw_line( world_position, world_position + normal, vec4( 0, 1, 1, 1 ), vec4( 0, 1, 1, 1 ) );
  }

  //payload = vec4( 1.0f - attribs.x - attribs.y, attribs.y, attribs.x, 1.0 );
}

#endif /* CRUDE_CLOSEST_HIT */

#if defined( CRUDE_MISS )
layout(location=0) rayPayloadInEXT ray_payload payload;

void main()
{
  payload.color = vec3( 0.f );
}
#endif /* CRUDE_MISS */
#endif /* RAY_TRACING_SOLID */