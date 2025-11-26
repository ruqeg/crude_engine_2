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
  uint32                                                   pbr_texture_index;
  float32                                                  visibility_sq;
  XMFLOAT2                                                 padding;
} crude_gfx_game_postprocessing_push_constant;

void
crude_gfx_game_postprocessing_pass_initialize
(
  _In_ crude_gfx_game_postprocessing_pass                 *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
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
  
  player_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->player_node, crude_transform );

  game_postprocessing_constant = CRUDE_COMPOUNT_EMPTY( crude_gfx_game_postprocessing_push_constant );
  game_postprocessing_constant.pbr_texture_index = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, "pbr" )->resource_info.texture.handle.index;
  game_postprocessing_constant.depth_texture_index = crude_gfx_render_graph_builder_access_resource_by_name( pass->scene_renderer->render_graph->builder, pass->scene_renderer->options.depth_texture_name )->resource_info.texture.handle.index;
  game_postprocessing_constant.player_position = player_transform->translation;
  game_postprocessing_constant.fog_color = game->fog_color;
  game_postprocessing_constant.visibility_sq = 20.0;
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