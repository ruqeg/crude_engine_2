
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#define CRUDE_RAYGEN
//#define CRUDE_STAGE_FRAGMENT

#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/culling.glsli"
#include "crude/light.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#extension GL_EXT_ray_tracing : enable

#if defined( RAY_TRACING_SOLID )

CRUDE_UNIFORM( SceneConstant, 0 ) 
{
  crude_scene                                              scene;
};

layout(set=CRUDE_MATERIAL_SET, binding=1) uniform accelerationStructureEXT as;

CRUDE_RBUFFER(RayParams, 2)
{
  uint                                                     sbt_offset;
  uint                                                     sbt_stride;
  uint                                                     miss_index;
  uint                                                     out_image_index;
};

#if defined( CRUDE_CLOSEST_HIT )
layout(location=0) rayPayloadInEXT vec4 payload;
hitAttributeEXT vec2 attribs;

void main()
{
  payload = vec4( 1.0f - attribs.x, attribs.y, attribs.x, 1.0 );
}
#endif /* CRUDE_CLOSEST_HIT */

#if defined( CRUDE_RAYGEN )
layout(location=0) rayPayloadEXT vec4 payload;

vec3 compute_ray_dir( uvec3 launchID, uvec3 launchSize )
{
  float x = ( 2 * ( float( launchID.x ) + 0.5 ) / float( launchSize.x ) - 1.0 );
  float y = ( 1.0 - 2 * ( float( launchID.y ) + 0.5 ) / float( launchSize.y ) );
  vec4 dir = vec4( x, y, 1, 1 ) * scene.camera.clip_to_world;
  dir = normalize( dir );
  return dir.xyz;
}

void main()
{
  payload = vec4( 0, 0, 1, 1 );

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

  imageStore( global_images_2d[ out_image_index ], ivec2( gl_LaunchIDEXT.xy ), payload );
}
#endif /* CRUDE_RAYGEN */


#if defined( CRUDE_MISS )
layout(location=0) rayPayloadInEXT vec4 payload;

void main()
{
  payload = vec4( 0.0, 0.0, 0.0, 1.0 );
}
#endif /* CRUDE_MISS */
#endif /* RAY_TRACING_SOLID */