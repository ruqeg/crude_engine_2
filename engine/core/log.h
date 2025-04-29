#pragma once

#include <core/alias.h>

typedef enum crude_verbosity
{
  CRUDE_VERBOSITY_OFF = -1,
  CRUDE_VERBOSITY_INFO = 0,
  CRUDE_VERBOSITY_WARNING = 1,
  CRUDE_VERBOSITY_ERROR = 2,
  CRUDE_VERBOSITY_ALL = 3
} crude_verbosity;

typedef enum crude_channel
{
  CRUDE_CHANNEL_CORE,
  CRUDE_CHANNEL_PLATFORM,
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

CRUDE_API void
crude_initialize_log
();

CRUDE_API void
crude_deinitialize_log
();

CRUDE_API void
crude_log_common
(
  _In_ char const                                         *filename,
  _In_ int32                                               line,
  _In_ crude_channel                                       channel,
  _In_ crude_verbosity                                     verbosity,
  _In_ char const                                         *format,
  _In_ ...
);

#define CRUDE_LOG( channel, format, ... )\
{\
  crude_log_common( __FILE__, __LINE__, channel, CRUDE_VERBOSITY_ALL, format, ##__VA_ARGS__ );\
}

#define CRUDE_LOG_INFO( channel, format, ... )\
{\
  crude_log_common( __FILE__, __LINE__, channel, CRUDE_VERBOSITY_INFO, format, ##__VA_ARGS__ );\
}

#define CRUDE_LOG_WARNING( channel, format, ... )\
{\
  crude_log_common( __FILE__, __LINE__, channel, CRUDE_VERBOSITY_WARNING, format, ##__VA_ARGS__ );\
}

#define CRUDE_LOG_ERROR( channel, format, ... )\
{\
  crude_log_common( __FILE__, __LINE__, channel, CRUDE_VERBOSITY_ERROR, format, ##__VA_ARGS__ );\
}