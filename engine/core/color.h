#pragma once

#include <engine/core/alias.h>

typedef uint32 crude_color;

CRUDE_API crude_color
crude_color_set
(
  _In_ float32                                             r,
  _In_ float32                                             g,
  _In_ float32                                             b,
  _In_ float32                                             a
);

CRUDE_API float32 
crude_color_r
(
  _In_ crude_color                                         color
);

CRUDE_API float32 
crude_color_g
(
  _In_ crude_color                                         color
);

CRUDE_API float32 
crude_color_b
(
  _In_ crude_color                                         color
);

CRUDE_API float32 
crude_color_a
(
  _In_ crude_color                                         color
);

CRUDE_API uint8 
crude_color_r_u8
(
  _In_ crude_color                                         color
);

CRUDE_API uint8 
crude_color_g_u8
(
  _In_ crude_color                                         color
);

CRUDE_API uint8 
crude_color_b_u8
(
  _In_ crude_color                                         color
);

CRUDE_API crude_color
crude_color_from_u8
(
  _In_ uint8 r,
  _In_ uint8 g,
  _In_ uint8 b,
  _In_ uint8 a
);

CRUDE_API crude_color
crude_color_get_distinct_color
(
  _In_ uint32                                               index
);

#define CRUDE_COLOR_RED 0xff0000ff
#define CRUDE_COLOR_GREEN 0xff00ff00
#define CRUDE_COLOR_BLUE 0xffff0000
#define CRUDE_COLOR_YELLOW 0xff00ffff
#define CRUDE_COLOR_BLACK 0xff000000
#define CRUDE_COLOR_WHITE 0xffffffff
#define CRUDE_COLOR_TRANSPARENT 0x00000000