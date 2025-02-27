#include <crude_shaders/main.frag.inl>
#include <crude_shaders/main.vert.inl>

#include <platform/gui_components.h>
#include <graphics/render_components.h>
#include <graphics/command_buffer.h>

#include <graphics/render_system.h>

static void initialize_render_core( ecs_iter_t *it  )
{
  crude_render_create *render_create = ecs_field( it, crude_render_create, 0 );
  crude_window_handle *window_handle = ecs_field( it, crude_window_handle, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_renderer *renderer = ecs_ensure( it->world, it->entities[i], crude_renderer );
    
    crude_gpu_device_creation create = {
      .sdl_window             = window_handle[i].value,
      .vk_application_name    = render_create[i].vk_application_name,
      .vk_application_version = render_create[i].vk_application_version,
      .allocator              = render_create[i].allocator,
      .queries_per_frame      = 1u,
      .max_frames             = render_create[i].max_frames,
    };

    renderer->gpu = render_create[i].allocator.allocate( sizeof( crude_gpu_device ), 1u );
    crude_initialize_gpu_device( renderer->gpu, &create );

    crude_pipeline_creation pipeline_creation;
    memset( &pipeline_creation, 0, sizeof( pipeline_creation ) );
    pipeline_creation.shaders.name = "triangle";
    pipeline_creation.shaders.spv_input = true;
    pipeline_creation.shaders.stages[0].code = crude_compiled_shader_main_vert;
    pipeline_creation.shaders.stages[0].code_size = sizeof( crude_compiled_shader_main_vert );
    pipeline_creation.shaders.stages[0].type = VK_SHADER_STAGE_VERTEX_BIT;
    pipeline_creation.shaders.stages[1].code = crude_compiled_shader_main_frag;
    pipeline_creation.shaders.stages[1].code_size = sizeof( crude_compiled_shader_main_frag );
    pipeline_creation.shaders.stages[1].type = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipeline_creation.shaders.stages_count = 2u;
    renderer->pipeline = crude_create_pipeline( renderer->gpu, &pipeline_creation );
  }
}

static void deinitialize_render_core( ecs_iter_t *it )
{
  crude_renderer *renderer = ecs_field( it, crude_renderer, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_destroy_pipeline( renderer[i].gpu, renderer[i].pipeline );
    crude_deinitialize_gpu_device( renderer[i].gpu );
    renderer[i].gpu->allocator.deallocate( renderer[i].gpu );
  }
}

static void render( ecs_iter_t *it )
{
  crude_renderer *renderer = ecs_field( it, crude_renderer, 0 );
  crude_window   *window_handle = ecs_field( it, crude_window, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_new_frame( renderer[i].gpu );
    crude_command_buffer *gpu_commands = crude_get_command_buffer( renderer[i].gpu, CRUDE_QUEUE_TYPE_GRAPHICS, true );
    crude_gfx_cmd_bind_render_pass( gpu_commands, renderer[i].gpu->swapchain_pass );
    crude_gfx_cmd_bind_pipeline( gpu_commands, renderer[i].pipeline );
    crude_gfx_cmd_set_viewport( gpu_commands, NULL );
    crude_gfx_cmd_set_scissor( gpu_commands, NULL );
    crude_gfx_cmd_draw( gpu_commands, 0, 3, 0, 1);
    crude_queue_command_buffer( gpu_commands );
    crude_present( renderer[i].gpu );
  }
}

void crude_render_systemImport( ecs_world_t *world )
{
  ECS_MODULE( world, crude_render_system );
  ECS_IMPORT( world, crude_gui_components );
  ECS_IMPORT( world, crude_render_components );
 
  ecs_observer( world, {
    .query.terms = { 
      ( ecs_term_t ) { .id = ecs_id( crude_render_create ), .oper = EcsAnd },
      ( ecs_term_t ) { .id = ecs_id( crude_window_handle ), .oper = EcsAnd },
      ( ecs_term_t ) { .id = ecs_id( crude_renderer ), .oper = EcsNot }
    },
    .events = { EcsOnSet },
    .callback = initialize_render_core
    });
  ecs_observer( world, {
    .query.terms = { { .id = ecs_id( crude_renderer ) } },
    .events = { EcsOnRemove },
    .callback = deinitialize_render_core
    } );
  ecs_system( world, {
    .entity = ecs_entity( world, { .name = "render_system", .add = ecs_ids( ecs_dependson( EcsOnStore ) ) } ),
    .callback = render,
    .query.terms = { 
      {.id = ecs_id( crude_renderer ) },
      {.id = ecs_id( crude_window ) },
    } } );
}
