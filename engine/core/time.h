#pragma once

#include <core/alias.h>

CRUDE_API void
crude_time_service_initialize
(
);

CRUDE_API void  
crude_time_service_deinitialize
(
);

CRUDE_API int64
crude_time_now
(
);

CRUDE_API float64
crude_time_seconds
(
  _In_ int64 time
);

CRUDE_API float64
crude_time_delta_seconds
(
  _In_ int64 starting_time,
  _In_ int64 ending_time
);