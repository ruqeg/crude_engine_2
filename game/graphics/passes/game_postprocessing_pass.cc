#include <engine/core/profiler.h>
#include <engine/core/hash_map.h>
#include <engine/scene/scene_components.h>
#include <engine/graphics/scene_renderer.h>
#include <game/game.h>

#include <game/graphics/passes/game_postprocessing_pass.h>>

typedef struct crude_gfx_game_postprocessing_push_constant
{
  XMFLOAT3                                                 player_position;
  uint32                                                   depth_texture_index;
  XMFLOAT4                                                 fog_color;
  XMFLOAT4                                                 pulse_color;
  uint32                                                   pbr_texture_index;
  float32                                                  fog_distance;
  float32                                                  fog_coeff;
  float32                                                  wave_size;
  float32                                                  wave_texcoord_scale;
  float32                                                  wave_absolute_frame_scale;
  float32                                                  aberration_strength_scale;
  float32                                                  aberration_strength_offset;
  float32                                                  aberration_strength_sin_affect;
  float32                                                  pulse_frame_scale;
  float32                                                  pulse_scale;
  float32                                                  pulse_distance_coeff;
  float32                                                  pulse_distance;
  float32                                                  star_coeff;
} crude_gfx_game_postprocessing_push_constant;

void
crude_gfx_game_postprocessing_pass_initialize
(
  _In_ crude_gfx_game_postprocessing_pass                 *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  pass->options.fog_color = CRUDE_COMPOUNT_EMPTY( XMFLOAT4 );
  pass->options.fog_distance = 10.f;
  pass->options.fog_coeff = 0.5f;
  pass->options.wave_size = 0.0;
  pass->options.wave_texcoord_scale = 0.0;
  pass->options.wave_absolute_frame_scale = 0.0;
  pass->options.aberration_strength_scale = 0.0;
  pass->options.aberration_strength_offset = 0.0;
  pass->options.aberration_strength_sin_affect = 0.0;
  pass->options.pulse_color = CRUDE_COMPOUNT( XMFLOAT4, { 0, 0, 0, 0 } );
  pass->options.pulse_frame_scale = 0.0f;
  pass->options.pulse_scale = 0.0f;
  pass->options.pulse_distance_coeff = 0.1f;
  pass->options.pulse_distance = 0.0f;
  pass->options.star_coeff = 0.0f;

  pass->scene_renderer = scene_renderer;
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    pass->game_postprocessing_ds[ i ] = CRUDE_GFX_DESCRIPTOR_SET_HANDLE_INVALID;
  }

  crude_gfx_game_postprocessing_pass_on_techniques_reloaded( pass );
}

void
crude_gfx_game_postprocessing_pass_deinitialize
(
  _In_ crude_gfx_game_postprocessing_pass                       *pass
)
{
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->game_postprocessing_ds[ i ] );
  }
}

void
crude_gfx_game_postprocessing_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  game_t                                                  *game;
  crude_gfx_game_postprocessing_pass                      *pass;
  crude_gfx_device                                        *gpu;
  crude_transform const                                   *player_transform;
  crude_gfx_pipeline_handle                                pipeline;
  crude_gfx_game_postprocessing_push_constant              game_postprocessing_constant;


  game = game_instance( );

  pass = CRUDE_REINTERPRET_CAST( crude_gfx_game_postprocessing_pass*, ctx );
  gpu = pass->scene_renderer->gpu;

  pipeline = crude_gfx_access_technique_pass_by_name( gpu, "game_fullscreen", "game_postprocessing" )->pipeline;
  crude_gfx_cmd_bind_pipeline( primary_cmd, pipeline );
  crude_gfx_cmd_bind_descriptor_set( primary_cmd, pass->game_postprocessing_ds[ gpu->current_frame ] );
  
  game_postprocessing_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_game_postprocessing_push_constant );
  if ( crude_entity_valid( game->focused_camera_node ) )
  {
    player_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->focused_camera_node, crude_transform );
    game_postprocessing_constant.pbr_texture_index = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, "pbr" )->resource_info.texture.handle.index;
    game_postprocessing_constant.depth_texture_index = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, pass->scene_renderer->options.depth_texture_name )->resource_info.texture.handle.index;
    XMStoreFloat3( &game_postprocessing_constant.player_position, crude_transform_node_to_world( game->focused_camera_node, player_transform ).r[ 3 ] );
    game_postprocessing_constant.fog_color = pass->options.fog_color;
    game_postprocessing_constant.fog_distance = pass->options.fog_distance;
    game_postprocessing_constant.fog_coeff = pass->options.fog_coeff;
    game_postprocessing_constant.wave_size = pass->options.wave_size;
    game_postprocessing_constant.wave_texcoord_scale = pass->options.wave_texcoord_scale;
    game_postprocessing_constant.wave_absolute_frame_scale = pass->options.wave_absolute_frame_scale;
    game_postprocessing_constant.aberration_strength_scale = pass->options.aberration_strength_scale;
    game_postprocessing_constant.aberration_strength_offset = pass->options.aberration_strength_offset;
    game_postprocessing_constant.aberration_strength_sin_affect = pass->options.aberration_strength_sin_affect;
    game_postprocessing_constant.pulse_color = pass->options.pulse_color;
    game_postprocessing_constant.pulse_frame_scale = pass->options.pulse_frame_scale;
    game_postprocessing_constant.pulse_scale = pass->options.pulse_scale;
    game_postprocessing_constant.pulse_distance_coeff = pass->options.pulse_distance_coeff;
    game_postprocessing_constant.pulse_distance = pass->options.pulse_distance;
    game_postprocessing_constant.star_coeff = pass->options.star_coeff;
  }

  crude_gfx_cmd_push_constant( primary_cmd, &game_postprocessing_constant, sizeof( game_postprocessing_constant ) );

  crude_gfx_cmd_draw( primary_cmd, 0u, 3u, 0u, 1u );
}

void
crude_gfx_game_postprocessing_pass_on_techniques_reloaded
(
  _In_ void                                               *ctx
)
{
  crude_gfx_game_postprocessing_pass                       *pass;
  crude_gfx_technique_pass                                *meshlet_pass;
  crude_gfx_descriptor_set_layout_handle                   layout;
  
  pass = CRUDE_REINTERPRET_CAST( crude_gfx_game_postprocessing_pass*, ctx );

  meshlet_pass = crude_gfx_access_technique_pass_by_name( pass->scene_renderer->gpu, "game_fullscreen", "game_postprocessing" );
  layout = crude_gfx_get_descriptor_set_layout( pass->scene_renderer->gpu, meshlet_pass->pipeline, CRUDE_GRAPHICS_MATERIAL_DESCRIPTOR_SET_INDEX );
  
  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    if ( CRUDE_RESOURCE_HANDLE_IS_VALID( pass->game_postprocessing_ds[ i ] ) )
    {
      crude_gfx_destroy_descriptor_set( pass->scene_renderer->gpu, pass->game_postprocessing_ds[ i ] );
    }
  }

  for ( uint32 i = 0; i < CRUDE_GRAPHICS_MAX_SWAPCHAIN_IMAGES; ++i )
  {
    crude_gfx_descriptor_set_creation                    ds_creation;
    
    ds_creation = crude_gfx_descriptor_set_creation_empty();
    ds_creation.layout = layout;
    ds_creation.name = "game_postprocessing_ds";
  
    crude_gfx_descriptor_set_creation_add_buffer( &ds_creation, pass->scene_renderer->scene_cb, 0u );
    crude_gfx_scene_renderer_add_debug_resources_to_descriptor_set_creation( &ds_creation, pass->scene_renderer, i );
    
    pass->game_postprocessing_ds[ i ] = crude_gfx_create_descriptor_set( pass->scene_renderer->gpu, &ds_creation );
  }
}

crude_gfx_render_graph_pass_container
crude_gfx_game_postprocessing_pass_pack
(
  _In_ crude_gfx_game_postprocessing_pass                 *pass
)
{
  crude_gfx_render_graph_pass_container container = crude_gfx_render_graph_pass_container_empty();
  container.ctx = pass;
  container.render = crude_gfx_game_postprocessing_pass_render;
  container.on_techniques_reloaded = crude_gfx_game_postprocessing_pass_on_techniques_reloaded;
  return container;
}