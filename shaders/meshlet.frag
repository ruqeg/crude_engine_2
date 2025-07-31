
#ifdef CRUDE_VALIDATOR_LINTING
#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#include "crude/meshlet.glsli"
#include "crude/mesh.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

layout(location = 0) out vec4 out_color;

layout(location=0) in vec2 in_texcoord0;
layout(location=1) in flat uint mesh_draw_index;

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

void main()
{
  crude_mesh_draw mesh_draw = mesh_draws[ mesh_draw_index ];
  
  // !TODO move to sep when gbuffer will be done or use clamp with default last texture
  vec4 albedo;
  if ( mesh_draw.textures.x != CRUDE_TEXTURE_INVALID )
  {
    albedo = texture( global_textures[ nonuniformEXT( mesh_draw.textures.x ) ], in_texcoord0 ) * mesh_draw.albedo_color_factor;
  }
  else
  {
    albedo = mesh_draw.albedo_color_factor;
  }
      if ( mesh_draw_index == 40 )
    {
  out_color = vec4( 1, 0, 0, 1 );
    }
    else
    {
  out_color = vec4( albedo );
    }
}