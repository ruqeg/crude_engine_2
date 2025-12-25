#pragma once

#include <engine/core/ecs.h>
#include <engine/platform/platform.h>

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_free_camera_system_context
{
  crude_ecs                                               *world;
  crude_platform                                          *platform;
} crude_free_camera_system_context;

CRUDE_API void
crude_free_camera_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_free_camera_system_context                   *ctx
);