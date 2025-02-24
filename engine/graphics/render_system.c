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
    crude_gpu_device_creation create = {
      .sdl_window             = window_handle[i].value,
      .vk_application_name    = render_create[i].vk_application_name,
      .vk_application_version = render_create[i].vk_application_version,
      .allocator              = render_create[i].allocator,
      .queries_per_frame      = 1u,
      .max_frames             = render_create[i].max_frames,
    };
    crude_gpu_device *gpu = ecs_ensure( it->world, it->entities[i], crude_gpu_device );
    crude_initialize_gpu_device( gpu, &create );

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
    crude_pipeline_handle pipeline = crude_create_pipeline( gpu, &pipeline_creation );
  }
}

static void deinitialize_render_core( ecs_iter_t *it )
{
  crude_gpu_device *gpu = ecs_field( it, crude_gpu_device, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_deinitialize_gpu_device( &gpu[i] );
  }
}

static void render( ecs_iter_t *it )
{
  crude_gpu_device *gpu = ecs_field( it, crude_gpu_device, 0 );
  crude_window     *window_handle = ecs_field( it, crude_window, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    //crude_new_frame( &gpu[i] );
    //
    //crude_command_buffer *gpu_commands = crude_get_command_buffer( gpu, CRUDE_QUEUE_TYPE_GRAPHICS, true );
    //crude_queue_command_buffer( gpu_commands );
    //crude_present( &gpu[i] );
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
      ( ecs_term_t ) { .id = ecs_id( crude_gpu_device ), .oper = EcsNot }
    },
    .events = { EcsOnSet },
    .callback = initialize_render_core
    });
  ecs_observer( world, {
    .query.terms = { { ecs_id( crude_gpu_device ) } },
    .events = { EcsOnRemove },
    .callback = deinitialize_render_core
    } );
  ecs_system( world, {
    .entity = ecs_entity( world, { .name = "render_system", .add = ecs_ids( ecs_dependson( EcsOnStore ) ) } ),
    .callback = render,
    .query.terms = { 
      {.id = ecs_id( crude_gpu_device ) },
      {.id = ecs_id( crude_window ) },
    } } );
}
