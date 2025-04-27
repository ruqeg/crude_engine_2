#include <tracy/TracyC.h>
#include <core/alias.h>

#define CRUDE_TRACING_ZONE                     TracyCZone( _ctx, 1 )
#define CRUDE_TRACING_ZONE_NAME( name )        TracyCZoneN( _ctx, name, 1 )
#define CRUDE_TRACING_END                      TracyCZoneEnd( _ctx )

#define CRUDE_TRACING_MARK_FRAME               TracyCFrameMark

#define CRUDE_TRACING_SET_THREAD_NAME( name )  TracyCSetThreadName( name )
//#define CRUDE_PROFILER_MESSAGE_TRACE( txt ) TracyCMessageLC( ( txt ), 0xffffff )
//#define CRUDE_PROFILER_MESSAGE_DEBUG( txt ) TracyCMessageLC( ( txt ), 0xff1493 )
//#define CRUDE_PROFILER_MESSAGE_INFO( txt ) TracyCMessageLC( ( txt ), 0x00ff00 )
//#define CRUDE_PROFILER_MESSAGE_WARN( txt ) TracyCMessageLC( ( txt ), 0xffa500 )
//#define CRUDE_PROFILER_MESSAGE_ERROR( txt ) TracyCMessageLC( ( txt ), 0xff6347 )
//#define CRUDE_PROFILER_MESSAGE_FATAL( txt ) TracyCMessageLC( ( txt ), 0xff0000 )
//#define CRUDE_PROFILER_RECORD_VALUE( name, value ) TracyCPlot( ( name ), ( value ) )


//#define message_trace(TXT) \
//  MESSAGE_TRACE((TXT));    \
//  log_trace((TXT));
//
//#define message_debug(TXT) \
//  MESSAGE_DEBUG((TXT));    \
//  log_debug((TXT));
//
//#define message_info(TXT) \
//  MESSAGE_INFO((TXT));    \
//  log_info((TXT));
//
//#define message_warn(TXT) \
//  MESSAGE_WARN((TXT));    \
//  log_warn((TXT));
//
//#define message_error(TXT) \
//  MESSAGE_ERROR((TXT));    \
//  log_error((TXT));
//
//#define message_fatal(TXT) \
//  MESSAGE_FATAL((TXT));    \
//  log_fatal((TXT));
