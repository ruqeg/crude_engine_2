#include <stb_ds.h>

#include <core/file.h>
#include <core/string.h>
#include <platform/gui_components.h>
#include <graphics/render_components.h>
#include <graphics/command_buffer.h>

#include <graphics/render_system.h>

// !TODO
#include <scene/scene_components.h>
#include <scene/scripts_components.h>
#include <scene/free_camera_system.h>
#include <resources/gltf_loader.h> 

static void
crude_draw_mesh
(
  _In_ crude_renderer        *renderer,
  _In_ crude_command_buffer  *gpu_commands,
  _In_ crude_mesh_draw       *mesh_draw
)
{
  crude_descriptor_set_creation ds_creation = {
    .samplers = { ( crude_sampler_handle ) { CRUDE_INVALID_SAMPLER_INDEX }, ( crude_sampler_handle ) { CRUDE_INVALID_SAMPLER_INDEX } },
    .bindings = { 0, 1 },
    .resources= { renderer->gpu->frame_buffer.index, mesh_draw->material_buffer.index },
    .num_resources = 2,
    .layout = mesh_draw->material->program->passes[ 0 ].descriptor_set_layout
  };
  crude_descriptor_set_handle descriptor_set = crude_gfx_cmd_create_local_descriptor_set( gpu_commands, &ds_creation );
  
  crude_gfx_cmd_bind_vertex_buffer( gpu_commands, mesh_draw->position_buffer, 0, mesh_draw->position_offset );
  crude_gfx_cmd_bind_vertex_buffer( gpu_commands, mesh_draw->tangent_buffer, 1, mesh_draw->tangent_offset );
  crude_gfx_cmd_bind_vertex_buffer( gpu_commands, mesh_draw->normal_buffer, 2, mesh_draw->normal_offset );
  crude_gfx_cmd_bind_vertex_buffer( gpu_commands, mesh_draw->texcoord_buffer, 3, mesh_draw->texcoord_offset );
  crude_gfx_cmd_bind_index_buffer( gpu_commands, mesh_draw->index_buffer, mesh_draw->index_offset );
  crude_gfx_cmd_bind_local_descriptor_set( gpu_commands, descriptor_set );
  crude_gfx_cmd_draw_indexed( gpu_commands, mesh_draw->primitive_count, 1, 0, 0, 0 );
}

static void
initialize_render_core
(
  ecs_iter_t *it
)
{
  crude_render_create *render_create = ecs_field( it, crude_render_create, 0 );
  crude_window_handle *window_handle = ecs_field( it, crude_window_handle, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_renderer_component *renderer = ecs_ensure( it->world, it->entities[ i ], crude_renderer_component );
    
    crude_gpu_device_creation gpu_creation = {
      .sdl_window             = window_handle[ i ].value,
      .vk_application_name    = render_create[ i ].vk_application_name,
      .vk_application_version = render_create[ i ].vk_application_version,
      .allocator              = render_create[ i ].allocator,
      .queries_per_frame      = 1u,
      .max_frames             = render_create[ i ].max_frames,
    };

    renderer->gpu = render_create[ i ].allocator.allocate( sizeof( crude_gpu_device ), 1u );
    crude_gfx_initialize_gpu_device( renderer->gpu, &gpu_creation );

    crude_renderer_creation rendere_creation = {
      .allocator = render_create[ i ].allocator,
      .gpu = renderer->gpu
    };

    renderer->renderer = render_create[ i ].allocator.allocate( sizeof( crude_renderer ), 1u );
    crude_gfx_initialize_renderer( renderer->renderer, &rendere_creation );

    char gltf_path[ 1024 ];
    crude_get_current_working_directory( gltf_path, sizeof( gltf_path ) );
    crude_strcat( gltf_path, "\\..\\..\\resources\\glTF-Sample-Models\\2.0\\Sponza\\glTF\\Sponza.gltf" );
    
    crude_buffer_creation ubo_creation = {
      .type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .usage = CRUDE_RESOURCE_USAGE_TYPE_DYNAMIC ,
      .size = sizeof( crude_shader_frame_constants ),
      .name = "ubo",
    };
    renderer->gpu->frame_buffer = crude_gfx_create_buffer( renderer->gpu, &ubo_creation );

    renderer->scene = render_create[ i ].allocator.allocate( sizeof( crude_scene ), 1u );
    crude_load_gltf_from_file( renderer->renderer, gltf_path, renderer->scene );
    
    renderer[ i ].camera = crude_entity_create_empty( it->world, "camera1" );
    CRUDE_ENTITY_SET_COMPONENT( renderer[ i ].camera, crude_camera, {
      .fov_radians = CRUDE_CPI4,
      .near_z = 0.01,
      .far_z = 1000,
      .aspect_ratio = 1.0 } );
    CRUDE_ENTITY_SET_COMPONENT( renderer[ i ].camera, crude_transform, {
      .translation = { 0, 0, -5 },
      .rotation = { 0, 0, 0, 1 },
      .scale = { 1, 1, 1 }, } );
    
    CRUDE_ENTITY_SET_COMPONENT( renderer[ i ].camera, crude_free_camera, {
      .moving_speed_multiplier = { 7.0, 7.0, 7.0 },
      .rotating_speed_multiplier  = { -0.15f, -0.15f },
      .entity_input = ( crude_entity ){ it->entities[ i ], it->world } } );
  }
}

static void
deinitialize_render_core
(
  ecs_iter_t *it
)
{
  crude_renderer_component *renderer = ecs_field( it, crude_renderer_component, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_gfx_deinitialize_renderer( renderer->renderer );
    crude_gfx_deinitialize_gpu_device( renderer[ i ].gpu );
    renderer[ i ].gpu->allocator.deallocate( renderer[ i ].gpu );
    renderer[ i ].renderer->allocator.deallocate( renderer[ i ].renderer );
    renderer[ i ].renderer->allocator.deallocate( renderer[ i ].scene );
  }
}

static void
render
(
  ecs_iter_t *it
)
{
  crude_renderer_component *renderer = ecs_field( it, crude_renderer_component, 0 );
  crude_window   *window_handle = ecs_field( it, crude_window, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_gfx_new_frame( renderer[ i ].gpu );
    crude_command_buffer *gpu_commands = crude_gfx_get_cmd_buffer( renderer[ i ].gpu, CRUDE_QUEUE_TYPE_GRAPHICS, true );

    // update fame buffer
    crude_map_buffer_parameters constant_buffer_map = { renderer[ i ].gpu->frame_buffer, 0, 0 };
    crude_shader_frame_constants *frame_buffer_data = CAST( crude_shader_frame_constants*, crude_gfx_map_buffer( renderer[ i ].gpu, &constant_buffer_map ) );
    if ( frame_buffer_data )
    {
      crude_camera const *camera =  CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( renderer[ i ].camera, crude_camera );
      crude_transform const *transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( renderer[ i ].camera, crude_transform );

      crude_matrix world_to_view = crude_mat_inverse( NULL, crude_transform_node_to_world( renderer[ i ].camera, transform ) );
      crude_matrix view_to_clip = crude_camera_view_to_clip( camera );
      
      crude_store_float4x4a( &frame_buffer_data->world_to_view, world_to_view ); 
      crude_store_float4x4a( &frame_buffer_data->view_to_clip, view_to_clip ); 
      crude_gfx_unmap_buffer( renderer[ i ].gpu, renderer[ i ].gpu->frame_buffer );
    }
    
    // update mesh buffer
    for ( uint32 mesh_index = 0; mesh_index < arrlen( renderer->scene->mesh_draws ); ++mesh_index )
    {
      crude_mesh_draw *mesh_draw = &renderer->scene->mesh_draws[ mesh_index ];
      
      constant_buffer_map.buffer = mesh_draw->material_buffer;
      
      crude_shader_mesh_constants *mesh_data = CAST( crude_shader_frame_constants*, crude_gfx_map_buffer( renderer[ i ].gpu, &constant_buffer_map ) );
      if ( mesh_data )
      {
        mesh_data->textures.x = mesh_draw->albedo_texture_index;
        mesh_data->textures.y = mesh_draw->roughness_texture_index;
        mesh_data->textures.z = mesh_draw->normal_texture_index;
        mesh_data->textures.w = mesh_draw->occlusion_texture_index;
        mesh_data->base_color_factor = ( crude_float4a ){ mesh_draw->base_color_factor.x, mesh_draw->base_color_factor.y, mesh_draw->base_color_factor.z, mesh_draw->base_color_factor.w } ;
        mesh_data->metallic_roughness_occlusion_factor = ( crude_float3a ){ mesh_draw->metallic_roughness_occlusion_factor.x, mesh_draw->metallic_roughness_occlusion_factor.y, mesh_draw->metallic_roughness_occlusion_factor.z };
        mesh_data->alpha_cutoff.x = mesh_draw->alpha_cutoff;
        mesh_data->flags.x = mesh_draw->flags;
      
        crude_transform model_transform = {
          .translation = { 0, 0, 0 },
          .rotation = { 0, 0, 0, 0 },
          .scale = { 0.0005, 0.0005, 0.0005 },
        };
        crude_matrix model_to_world = crude_transform_node_to_world( renderer[ i ].camera, &model_transform );
        crude_store_float4x4a( &mesh_data->modelToWorld, model_to_world ); 
      
        crude_gfx_unmap_buffer( renderer[ i ].gpu, mesh_draw->material_buffer );
      }
    }

    crude_gfx_cmd_bind_render_pass( gpu_commands, renderer[ i ].gpu->swapchain_pass );
    crude_gfx_cmd_set_viewport( gpu_commands, NULL );
    crude_gfx_cmd_set_scissor( gpu_commands, NULL );
    
    crude_material *last_material = NULL;
    for ( uint32 mesh_index = 0; mesh_index < arrlen( renderer->scene->mesh_draws ); ++mesh_index )
    {
      crude_mesh_draw *mesh_draw = &renderer->scene->mesh_draws[ mesh_index ];
      if ( mesh_draw->material != last_material )
      {
        crude_gfx_cmd_bind_pipeline( gpu_commands, mesh_draw->material->program->passes[ 0 ].pipeline );
        last_material = mesh_draw->material;
      }
      crude_draw_mesh( renderer->renderer, gpu_commands, mesh_draw );
    }
    crude_gfx_queue_cmd_buffer( gpu_commands );
    crude_gfx_present( renderer[ i ].gpu );
  }
}

void
crude_render_systemImport
(
  ecs_world_t *world
)
{
  ECS_MODULE( world, crude_render_system );
  ECS_IMPORT( world, crude_gui_components );
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_render_components );
 
  ecs_observer( world, {
    .query.terms = { 
      ( ecs_term_t ) { .id = ecs_id( crude_render_create ), .oper = EcsAnd },
      ( ecs_term_t ) { .id = ecs_id( crude_window_handle ), .oper = EcsAnd },
      ( ecs_term_t ) { .id = ecs_id( crude_renderer_component ), .oper = EcsNot }
    },
    .events = { EcsOnSet },
    .callback = initialize_render_core
    });
  ecs_observer( world, {
    .query.terms = { { .id = ecs_id( crude_renderer_component ) } },
    .events = { EcsOnRemove },
    .callback = deinitialize_render_core
    } );
  ecs_system( world, {
    .entity = ecs_entity( world, { .name = "render_system", .add = ecs_ids( ecs_dependson( EcsOnStore ) ) } ),
    .callback = render,
    .query.terms = { 
      {.id = ecs_id( crude_renderer_component ) },
      {.id = ecs_id( crude_window ) },
    } } );
}
