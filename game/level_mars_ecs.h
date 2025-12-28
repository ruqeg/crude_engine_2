#pragma once

#include <engine/core/math.h>
#include <engine/core/ecs.h>

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_level_mars_system_context
{
} crude_level_mars_system_context;

CRUDE_API void
crude_level_mars_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_level_mars_system_context                    *ctx
);