#pragma once

#include <engine/core/log.h>

#define CRUDE_ASSERT( condition )\
{\
  if ( !( condition ) )\
  {\
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GENERAL, "Crude Error" );\
    CRUDE_DEBUG_BREAK;\
  }\
}

#define CRUDE_ASSERTM( channel, condition, format, ... )\
{\
  if ( !( condition ) )\
  {\
    CRUDE_LOG_ERROR( channel, format, ##__VA_ARGS__ );\
    CRUDE_DEBUG_BREAK;\
  }\
}

#define CRUDE_ABORT( channel, format, ... )\
{\
  CRUDE_LOG_ERROR( channel, format, ##__VA_ARGS__ );\
  CRUDE_DEBUG_BREAK;\
}

#define CRUDE_OVERFLOW()\
{\
  CRUDE_ABORT( CRUDE_CHANNEL_GENERAL, "Overflow! ");\
}