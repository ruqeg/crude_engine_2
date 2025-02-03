#pragma once

#include <core/platform.h>
#include <stdarg.h>

typedef enum crude_verbosity
{
  CRUDE_VERBOSITY_OFF,
  CRUDE_VERBOSITY_ERROR,
  CRUDE_VERBOSITY_WARNING,
  CRUDE_VERBOSITY_INFO,
  CRUDE_VERBOSITY_ALL
} crude_verbosity;

typedef enum crude_channel
{
  CRUDE_CHANNEL_CORE,
  CRUDE_CHANNEL_GENERAL,
  CRUDE_CHANNEL_MEMORY,
  CRUDE_CHANNEL_NETWORKING,
  CRUDE_CHANNEL_GRAPHICS,
  CRUDE_CHANNEL_COLLISIONS,
  CRUDE_CHANNEL_GAMEPLAY,
  CRUDE_CHANNEL_SYSTEM,
  CRUDE_CHANNEL_SOUND,
  CRUDE_CHANNEL_FILEIO,
  CRUDE_CHANNEL_GUI,
  CRUDE_CHANNEL_ALL,
} crude_channel;

void crude_log_common( const char* filename, int32 line, crude_channel channel, crude_verbosity verbosity, const char* format, ... );

#define CRUDE_LOG( channel, format, ... ) crude_log_common( __FILE__, __LINE__, channel, CRUDE_VERBOSITY_ALL, format, ##__VA_ARGS__ );
#define CRUDE_LOG_INFO( channel, format, ... ) crude_log_common( __FILE__, __LINE__, channel, CRUDE_VERBOSITY_INFO, format, ##__VA_ARGS__ );
#define CRUDE_LOG_WARNING( channel, format, ... ) crude_log_common( __FILE__, __LINE__, channel, CRUDE_VERBOSITY_WARNING, format, ##__VA_ARGS__ );
#define CRUDE_LOG_ERROR( channel, format, ... ) crude_log_common( __FILE__, __LINE__, channel, CRUDE_VERBOSITY_ERROR, format, ##__VA_ARGS__ );