//#pragma once
//
//#include <core/ecs.h>
//#include <graphics/renderer_resources.h>
//#include <graphics/render_graph.h>
//
//typedef struct crude_gfx_mesh_instance
//{
//  crude_gfx_mesh                                          *mesh;
//  uint32                                                   material_pass_index;
//} crude_gfx_mesh_instance;
//
//typedef struct crude_gfx_mesh
//{
//  crude_entity                                             node;
//  crude_gfx_renderer_material                             *material;
//  crude_float3                                             scale;
//  crude_float3                                             translation;
//  crude_float4                                             rotation;
//  crude_gfx_buffer_handle                                  index_buffer;
//  crude_gfx_buffer_handle                                  position_buffer;
//  crude_gfx_buffer_handle                                  tangent_buffer;
//  crude_gfx_buffer_handle                                  normal_buffer;
//  crude_gfx_buffer_handle                                  texcoord_buffer;
//  crude_gfx_buffer_handle                                  material_buffer;
//  uint32                                                   index_offset;
//  uint32                                                   position_offset;
//  uint32                                                   tangent_offset;
//  uint32                                                   normal_offset;
//  uint32                                                   texcoord_offset;
//  uint32                                                   primitive_count;
//  crude_float4                                             albedo_color_factor;
//  crude_float3                                             metallic_roughness_occlusion_factor;
//  float32                                                  alpha_cutoff;
//  uint32                                                   flags;
//  uint16                                                   albedo_texture_index;
//  uint16                                                   roughness_texture_index;
//  uint16                                                   normal_texture_index;
//  uint16                                                   occlusion_texture_index;
//  uint32                                                   gpu_mesh_index;
//} crude_gfx_mesh;
//
//typedef struct crude_gfx_scene_renderer_geometry_pass
//{
//  crude_gfx_scene_renderer                                *scene;
//  crude_gfx_mesh_instance                                 *mesh_instances;
//  uint32                                                   meshlet_technique_index;
//} crude_gfx_scene_renderer_geometry_pass;