#pragma once

#include <engine/platform/platform_components.h>

CRUDE_API void
crude_platform_service_initialize
(
);

CRUDE_API void
crude_platform_service_deinitialize
(
);

CRUDE_API void
crude_platform_hide_cursor
(
	_In_ crude_window_handle																window_handle
);

CRUDE_API void
crude_platform_show_cursor
(
	_In_ crude_window_handle																window_handle
);