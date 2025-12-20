#pragma once

#include <engine/core/ecs.h>

/************************************************
 *
 * ECS Components Declaration
 * 
 ***********************************************/

typedef struct crude_audio_listener
{
  float32                                                   last_local_to_world_update_time;
} crude_audio_listener;

CRUDE_API ECS_COMPONENT_DECLARE( crude_audio_listener );

/************************************************
 *
 * Functions Declaratin
 * 
 ***********************************************/
CRUDE_ECS_MODULE_IMPORT_DECL( crude_audio_components );