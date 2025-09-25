
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#define CULLING

#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/culling.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#if defined( CULLING )
layout(local_size_x=64, local_size_y=1, local_size_z=1) in;

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

CRUDE_RWBUFFER( MeshDrawCommandsEarly, 4 )
{
  crude_mesh_draw_command                                  mesh_draw_commands[];
};

CRUDE_RWBUFFER( MeshDrawCommandsLate, 5 )
{
  crude_mesh_draw_command                                  late_mesh_draw_commands[];
};

CRUDE_RWBUFFER( MeshDrawCountsEarly, 6 )
{
  uint                                                     early_opaque_mesh_visible_count;
  uint                                                     early_opaque_mesh_culled_count;
  uint                                                     early_transparent_mesh_visible_count;
  uint                                                     early_transparent_mesh_culled_count;

  uint                                                     early_total_mesh_count;
  uint                                                     early_depth_pyramid_texture_index;
  uint                                                     early_occlusion_culling_late_flag;
  uint                                                     early_meshlet_index_count;

  uint                                                     early_dispatch_task_x;
  uint                                                     early_dispatch_task_y;
  uint                                                     early_dispatch_task_z;
  uint                                                     early_meshlet_instances_count;
};

CRUDE_RWBUFFER( MeshDrawCountsLate, 7 )
{
  uint                                                     opaque_mesh_visible_count;
  uint                                                     opaque_mesh_culled_count;
  uint                                                     transparent_mesh_visible_count;
  uint                                                     transparent_mesh_culled_count;

  uint                                                     total_mesh_count;
  uint                                                     depth_pyramid_texture_index;
  uint                                                     occlusion_culling_late_flag;
  uint                                                     meshlet_index_count;

  uint                                                     dispatch_task_x;
  uint                                                     dispatch_task_y;
  uint                                                     dispatch_task_z;
  uint                                                     meshlet_instances_count;
};

void main()
{
  uint mesh_instance_draw_index = gl_GlobalInvocationID.x;
  uint count = occlusion_culling_late_flag == 1 ? early_opaque_mesh_culled_count : total_mesh_count;

  if ( mesh_instance_draw_index < count )
  {
    if ( occlusion_culling_late_flag == 1 )
    {
      mesh_instance_draw_index = late_mesh_draw_commands[ mesh_instance_draw_index ].draw_id;
    }
    uint mesh_draw_index = mesh_instance_draws[ mesh_instance_draw_index ].mesh_draw_index;
    mat4 model_to_world = mesh_instance_draws[ mesh_instance_draw_index ].model_to_world;
    vec4 bounding_sphere = mesh_bounds[ mesh_draw_index ];
    vec4 world_center = vec4( bounding_sphere.xyz, 1 ) * model_to_world;
    float scale = max( model_to_world[ 0 ][ 0 ], max( model_to_world[ 1 ][ 1 ], model_to_world[ 2 ][ 2 ] ) );
    float radius = bounding_sphere.w * scale * 1.1;

    vec4 view_center = world_center * scene.camera.world_to_view;

    bool frustum_visible = true;
    for ( uint i = 0; i < 6; ++i )
    {
      frustum_visible = frustum_visible && ( dot( scene.camera.frustum_planes_culling[ i ], view_center ) > -radius );
    }

    bool occlusion_visible = true;
    if ( frustum_visible )
    {
      occlusion_visible = crude_occlusion_culling(
        mesh_draw_index,
        view_center.xyz, radius, scene.camera.znear, 
        scene.camera.view_to_clip[ 0 ][ 0 ],
        scene.camera.view_to_clip[ 1 ][ 1 ],
        depth_pyramid_texture_index, world_center.xyz,
        scene.camera.position,
        scene.camera.world_to_clip
      );
    }

    if ( frustum_visible && occlusion_visible )
    {
      uint draw_index = atomicAdd( opaque_mesh_visible_count, 1 );
      mesh_draw_commands[ draw_index ].draw_id = mesh_instance_draw_index;
      mesh_draw_commands[ draw_index ].indirect_meshlet_group_count_x = ( mesh_draws[ mesh_draw_index ].meshletes_count + 31 ) / 32;
      mesh_draw_commands[ draw_index ].indirect_meshlet_group_count_y = 1;
      mesh_draw_commands[ draw_index ].indirect_meshlet_group_count_z = 1;
    }
    else if ( occlusion_culling_late_flag == 0 )
    {
      uint draw_index = atomicAdd( opaque_mesh_culled_count, 1 );
      late_mesh_draw_commands[ draw_index ].draw_id = mesh_instance_draw_index;
      late_mesh_draw_commands[ draw_index ].indirect_meshlet_group_count_x = ( mesh_draws[ mesh_draw_index ].meshletes_count + 31 ) / 32;
      late_mesh_draw_commands[ draw_index ].indirect_meshlet_group_count_y = 1;
      late_mesh_draw_commands[ draw_index ].indirect_meshlet_group_count_z = 1;
    }
  }
}
#endif /* CULLING */

#if defined( DEPTH_PYRAMID )
layout(set=CRUDE_MATERIAL_SET, binding=0) uniform sampler2D src;
layout(set=CRUDE_MATERIAL_SET, binding=1) uniform writeonly image2D dst;

layout(local_size_x=8, local_size_y=8, local_size_z=1) in;

void main()
{
  ivec2 texel_position00 = ivec2( gl_GlobalInvocationID.xy ) * 2;
  ivec2 texel_position01 = texel_position00 + ivec2( 0, 1 );
  ivec2 texel_position10 = texel_position00 + ivec2( 1, 0 );
  ivec2 texel_position11 = texel_position00 + ivec2( 1, 1 );

  float color00 = texelFetch( src, texel_position00, 0 ).r;
  float color01 = texelFetch( src, texel_position01, 0 ).r;
  float color10 = texelFetch( src, texel_position10, 0 ).r;
  float color11 = texelFetch( src, texel_position11, 0 ).r;

  float result = max( max( max( color00, color01 ), color10 ), color11 );

  imageStore( dst, ivec2( gl_GlobalInvocationID.xy ), vec4( result, 0, 0, 0 ) );

  groupMemoryBarrier();
  barrier();
}
#endif /* DEPTH_PYRAMID */