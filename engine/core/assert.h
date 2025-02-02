#pragma once

#include <core/log.h>

#define CRUDE_ASSERTM( condition, format, ... )\
  if ( !( condition ) )\
  {\
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GENERAL, format, ##__VA_ARGS__ );\
    CRUDE_DEBUG_BREAK\
  }

#define CRUDE_ABORT( format, ... )\
  {\
  CRUDE_LOG_ERROR( CRUDE_CHANNEL_GENERAL, format, ##__VA_ARGS__ );\
  CRUDE_DEBUG_BREAK\
  }

#define CRUDE_OVERFLOW() CRUDE_ABORT( "Overflow! ")