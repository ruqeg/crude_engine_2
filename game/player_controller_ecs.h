#pragma once

#include <engine/core/ecs.h>
#include <engine/platform/platform.h>
#include <engine/scene/components_serialization.h>
#include <engine/physics/physics.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
typedef struct crude_player_controller
{
  crude_input const                                       *input;

  /* Publix settings */
  float32                                                  pitch_limit;
  bool                                                     input_enabled;
  bool                                                     camera_enabled;
  XMFLOAT2                                                 walk_speed;
  float32                                                  run_speed;
  float32                                                  rotate_speed;
  
  /* Joints */
  uint64                                                   head_joint_node;
  uint64                                                   spine_joint_node;
  uint64                                                   right_hand_joint_node;

  /* Angles */
  float32                                                  head_pitch_angle;
  float32                                                  head_yaw_angle;
  float32                                                  spine_yaw_angle;
  float32                                                  pivot_pitch_angle;
  float32                                                  pivot_yaw_angle;

  /* Animations */
  uint32                                                   idle_animation_index;
  uint32                                                   walk_animation_index;
  uint32                                                   strafe_animation_index;
  uint32                                                   run_animation_index;
  uint32                                                   aim_down_animation_index;
  uint32                                                   fire_animation_index;

  /* Animations blending */
  XMFLOAT2                                                 move_blend;
  XMFLOAT2                                                 move_blend_max;
  float32                                                  aim_blend;
  float32                                                  shot_blend;
} crude_player_controller;

CRUDE_API ECS_COMPONENT_DECLARE( crude_player_controller );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_player_controller );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_player_controller );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_player_controller );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_player_controller );

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_player_controller_system_context
{
  crude_input const                                       *input;
  crude_heap_allocator                                    *player_controller_allocator;
  crude_allocator_container                                player_controller_allocator_container;
  crude_physics                                           *physics_manager;
} crude_player_controller_system_context;

CRUDE_API void
crude_player_controller_game_update_system_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_player_controller_engine_update_system_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_player_controller_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_player_controller_system_context             *ctx
);