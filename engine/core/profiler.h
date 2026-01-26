#pragma once

#if CRUDE_DEVELOP
#include <thirdparty/tracy/public/tracy/TracyC.h>

#include <engine/core/alias.h>
#include <engine/core/log.h>

#define CRUDE_PROFILER_ZONE                            TracyCZone( _ctx, 1 )
#define CRUDE_PROFILER_ZONE_NAME( name )\
TracyCZoneN( _ctx, name, 1 );\
//static char const *____crude_zone_name = name; CRUDE_LOG_INFO( CRUDE_CHANNEL_ALL, "[%s] start", name ); 
#define CRUDE_PROFILER_ZONE_END \
TracyCZoneEnd( _ctx );\
//CRUDE_LOG_INFO( CRUDE_CHANNEL_ALL, "[%s] end", ____crude_zone_name );
#define CRUDE_PROFILER_MARK_FRAME                      TracyCFrameMark
#define CRUDE_PROFILER_SET_THREAD_NAME( name )         TracyCSetThreadName( name )

#define CRUDE_PROFILER_ALLOC( ptr, size )              TracyCAlloc( ptr, size )
#define CRUDE_PROFILER_FREE( ptr )                     TracyCFree( ptr )
#define CRUDE_PROFILER_ALLOC_NAME( ptr, size, name )   TracyCAllocN( ptr, size, name )
#define CRUDE_PROFILER_FREE_NAME( ptr, name )          TracyCFreeN( ptr, name )
#define CRUDE_PROFILER_SECURE_ALLOC( ptr, size )       TracyCSecureAlloc( ptr, size )
#define CRUDE_PROFILER_SECURE_FREE( ptr )              TracyCSecureFree( ptr )

#else
#define CRUDE_PROFILER_ZONE
#define CRUDE_PROFILER_ZONE_NAME( name )
#define CRUDE_PROFILER_ZONE_END
#define CRUDE_PROFILER_MARK_FRAME
#define CRUDE_PROFILER_SET_THREAD_NAME( name )
#define CRUDE_PROFILER_ALLOC( ptr, size )
#define CRUDE_PROFILER_FREE( ptr )
#define CRUDE_PROFILER_ALLOC_NAME( ptr, size, name )
#define CRUDE_PROFILER_FREE_NAME( ptr, name )
#define CRUDE_PROFILER_SECURE_ALLOC( ptr, size )
#define CRUDE_PROFILER_SECURE_FREE( ptr )
#endif