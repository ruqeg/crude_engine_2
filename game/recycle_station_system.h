#pragma once

#include <engine/external/game_components.h>

CRUDE_API void
crude_recycle_station_start_recycle
(
  _In_ crude_recycle_station                              *station,
  _In_ crude_player                                       *player,
  _In_ crude_entity                                        station_node
);

CRUDE_ECS_MODULE_IMPORT_DECL( crude_recycle_station_system );