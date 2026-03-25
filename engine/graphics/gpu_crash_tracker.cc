#include <engine/graphics/graphics_config.h>

#if CRUDE_GFX_USE_NSIGHT_AFTERMATH

#include <engine/core/assert.h>
#include <engine/core/array.h>
#include <engine/core/string.h>
#include <engine/core/file.h>
#include <engine/core/time.h>

#include <vulkan/vulkan.h>
#include <thirdparty/NVIDIA_Nsight_Aftermath_SDK_2025.5.0.25317/GFSDK_Aftermath.h>
#include <thirdparty/NVIDIA_Nsight_Aftermath_SDK_2025.5.0.25317/GFSDK_Aftermath_GpuCrashDump.h>
#include <thirdparty/NVIDIA_Nsight_Aftermath_SDK_2025.5.0.25317/GFSDK_Aftermath_GpuCrashDumpDecoding.h>

#include <engine/graphics/gpu_crash_tracker.h>

#define CRUDE_GFX_AFTERMATH_CHECK_ERROR( fc )\
{\
  GFSDK_Aftermath_Result _result = fc;\
  if ( !GFSDK_Aftermath_SUCCEED( _result ) )\
  {\
    switch ( _result )\
    {\
    case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:\
    {\
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Unsupported driver version - requires an NVIDIA R495 display driver or newer" );\
    }\
    default:\
    {\
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Aftermath Error 0x%08x", _result );\
    }\
    }\
  }\
}

static void
crude_gfx_gpu_crash_tracker_resolve_marker_callback_
(
  _In_ void const                                         *marker_data,
  _In_ uint32 const                                        marker_datasize,
  _In_ void                                               *user_data,
  _In_ PFN_GFSDK_Aftermath_ResolveMarker                   resolve_marker
)
{
  //for (auto& map : m_markerMap)
  //{
  //    const auto& foundMarker = map.find((uint64_t)pMarkerData);
  //    if (foundMarker != map.end())
  //    {
  //        const std::string& foundMarkerData = foundMarker->second;
  //        resolveMarker(foundMarkerData.data(), (uint32_t)foundMarkerData.length());
  //        return;
  //    }
  //}
}

static void
crude_gfx_gpu_crash_tracker_crash_dump_description_callback_
(
  _In_ PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription      add_description,
  _In_ void                                               *user_data
)
{
  // Add some basic description about the crash. This is called after the GPU crash happens, but before
  // the actual GPU crash dump callback. The provided data is included in the crash dump and can be
  // retrieved using GFSDK_Aftermath_GpuCrashDump_GetDescription().

  //addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, "crude_engine");
  //addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "v1.0");
  //addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined, "This is a GPU crash dump example.");
  //addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined + 1, "Engine State: Rendering.");
  //addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined + 2, "More user-defined information...");
  //
  //// Include command line if provided
  //if (!m_commandLine.empty())
  //{
  //    std::string clPrefixed = std::string("CommandLine: ") + m_commandLine;
  //    addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined + 3, clPrefixed.c_str());
  //}
}

static void
crude_gfx_gpu_crash_tracker_shader_debug_info_callback_
(
  _In_ void const                                         *shader_debug_info,
  _In_ uint32 const                                        shader_debug_info_size,
  _In_ void                                               *user_data
) 
{
  crude_gfx_gpu_crash_tracker                             *tracker;
  GFSDK_Aftermath_ShaderDebugInfoIdentifier                gfsdk_identifier;

  tracker = CRUDE_CAST( crude_gfx_gpu_crash_tracker*, user_data );
  
  mtx_lock( &tracker->mutex );

  gfsdk_identifier = CRUDE_COMPOUNT_EMPTY( GFSDK_Aftermath_ShaderDebugInfoIdentifier );
  CRUDE_GFX_AFTERMATH_CHECK_ERROR( GFSDK_Aftermath_GetShaderDebugInfoIdentifier( GFSDK_Aftermath_Version_API, shader_debug_info, shader_debug_info_size, &gfsdk_identifier ) );

  //// Store information for decoding of GPU crash dumps with shader address mapping
  //// from within the application.
  //std::vector<uint8_t> data((uint8_t*)pShaderDebugInfo, (uint8_t*)pShaderDebugInfo + shaderDebugInfoSize);
  //m_shaderDebugInfo[identifier].swap(data);
  //
  //
  //// Write to file for later in-depth analysis of crash dumps with Nsight Graphics
  //WriteShaderDebugInformationToFile(identifier, pShaderDebugInfo, shaderDebugInfoSize);
}

static void
crude_gfx_gpu_crash_tracker_shader_debug_info_lookup_callback_
(
  _In_ GFSDK_Aftermath_ShaderDebugInfoIdentifier const    *identifier,
  _In_ PFN_GFSDK_Aftermath_SetData                         set_shader_debug_info,
  _In_ void                                               *user_data
)
{
  crude_gfx_gpu_crash_tracker                             *tracker;

  tracker = CRUDE_CAST( crude_gfx_gpu_crash_tracker*, user_data );

  //auto i_debugInfo = m_shaderDebugInfo.find(identifier);
  //if (i_debugInfo == m_shaderDebugInfo.end())
  //{
  //  return;
  //}
  //
  //setShaderDebugInfo(i_debugInfo->second.data(), uint32_t(i_debugInfo->second.size()));
}

static void
crude_gfx_gpu_crash_tracker_shader_lookup_callback_
(
  _In_ GFSDK_Aftermath_ShaderBinaryHash const             *shader_hash,
  _In_ PFN_GFSDK_Aftermath_SetData                         set_shader_binary,
  _In_ void                                               *user_data
)
{
  crude_gfx_gpu_crash_tracker                             *tracker;

  tracker = CRUDE_CAST( crude_gfx_gpu_crash_tracker*, user_data );

  //// Find shader binary data for the shader hash in the shader database.
  //std::vector<uint8_t> shaderBinary;
  //if (!m_shaderDatabase.FindShaderBinary(shaderHash, shaderBinary))
  //{
  //    // Early exit, nothing found. No need to call setShaderBinary.
  //    return;
  //}
  //
  //// Let the GPU crash dump decoder know about the shader data
  //// that was found.
  //setShaderBinary(shaderBinary.data(), uint32_t(shaderBinary.size()));
}

static void
crude_gfx_gpu_crash_tracker_shader_source_debug_info_lookup_callback_
(
  _In_ GFSDK_Aftermath_ShaderDebugName const              *shader_debug_name,
  _In_ PFN_GFSDK_Aftermath_SetData                         set_shader_binary,
  _In_ void                                               *user_data
)
{
  crude_gfx_gpu_crash_tracker                             *tracker;

  tracker = CRUDE_CAST( crude_gfx_gpu_crash_tracker*, user_data );
//    // Find source debug info for the shader DebugName in the shader database.
//    std::vector<uint8_t> shaderBinary;
//    if (!m_shaderDatabase.FindShaderBinaryWithDebugData(shaderDebugName, shaderBinary))
//    {
//        // Early exit, nothing found. No need to call setShaderBinary.
//        return;
//    }
//
//    // Let the GPU crash dump decoder know about the shader debug data that was
//    // found.
//    setShaderBinary(shaderBinary.data(), uint32_t(shaderBinary.size()));
}


static void
crude_gfx_gpu_crash_tracker_write_gpu_crash_dump_to_file_
(
  _In_ crude_gfx_gpu_crash_tracker                        *tracker,
  _In_ void const                                         *gpu_crash_dump,
  _In_ uint32 const                                        gpu_crash_dump_size
)
{
  char                                                    *json_buffer;
  char                                                    *application_name;
  char                                                    *crashdump_relative_filepath;
  GFSDK_Aftermath_GpuCrashDump_Decoder                     gfsdk_decoder;
  GFSDK_Aftermath_GpuCrashDump_BaseInfo                    gfsdk_base_info;
  uint32                                                   application_name_length, crashdump_relative_filepath_length, json_size;

  gfsdk_decoder = CRUDE_COMPOUNT_EMPTY( GFSDK_Aftermath_GpuCrashDump_Decoder );
  CRUDE_GFX_AFTERMATH_CHECK_ERROR( GFSDK_Aftermath_GpuCrashDump_CreateDecoder( GFSDK_Aftermath_Version_API, gpu_crash_dump, gpu_crash_dump_size, &gfsdk_decoder ) );
  CRUDE_GFX_AFTERMATH_CHECK_ERROR( GFSDK_Aftermath_GpuCrashDump_GetBaseInfo( gfsdk_decoder, &gfsdk_base_info ) );
  CRUDE_GFX_AFTERMATH_CHECK_ERROR( GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize( gfsdk_decoder, GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, &application_name_length ) );

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( application_name, application_name_length, crude_heap_allocator_pack( tracker->allocator ) );
  application_name[ 0 ] = 0;
  
  CRUDE_GFX_AFTERMATH_CHECK_ERROR( GFSDK_Aftermath_GpuCrashDump_GetDescription( gfsdk_decoder, GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, application_name_length, application_name ) );
  
  crashdump_relative_filepath_length = application_name_length + 1024;
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( crashdump_relative_filepath, crashdump_relative_filepath_length, crude_heap_allocator_pack( tracker->allocator ) );
  
  crude_snprintf( crashdump_relative_filepath, crashdump_relative_filepath_length, "%s-%i-%i.nv-gpudmp", application_name, gfsdk_base_info.pid, tracker->crash_count );

  crude_write_file( crashdump_relative_filepath, gpu_crash_dump, gpu_crash_dump_size );

  json_size = 0;
  CRUDE_GFX_AFTERMATH_CHECK_ERROR( GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
    gfsdk_decoder,
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
    GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
    crude_gfx_gpu_crash_tracker_shader_debug_info_lookup_callback_,
    crude_gfx_gpu_crash_tracker_shader_lookup_callback_,
    crude_gfx_gpu_crash_tracker_shader_source_debug_info_lookup_callback_,
    tracker,
    &json_size ) );
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( json_buffer, json_size, crude_heap_allocator_pack( tracker->allocator ) );

  CRUDE_GFX_AFTERMATH_CHECK_ERROR( GFSDK_Aftermath_GpuCrashDump_GetJSON( gfsdk_decoder, json_size, json_buffer ) );

  crude_snprintf( crashdump_relative_filepath, crashdump_relative_filepath_length, "%s-%i-%i.nv-gpudmp.json", application_name, gfsdk_base_info.pid, tracker->crash_count );
  crude_write_file( crashdump_relative_filepath, json_buffer, json_size);

  CRUDE_GFX_AFTERMATH_CHECK_ERROR( GFSDK_Aftermath_GpuCrashDump_DestroyDecoder( gfsdk_decoder ) );

  ++tracker->crash_count;
}

static void
crude_gfx_gpu_device_crash_dump_callback_
(
  _In_ void const                                         *gpu_crash_dump,
  _In_ uint32 const                                        gpu_crash_dump_size,
  _In_ void                                               *user_data
)
{
  crude_gfx_gpu_crash_tracker                             *tracker;

  tracker = CRUDE_CAST( crude_gfx_gpu_crash_tracker*, user_data );

  mtx_lock( &tracker->mutex );
  crude_gfx_gpu_crash_tracker_write_gpu_crash_dump_to_file_( tracker, gpu_crash_dump, gpu_crash_dump_size );
  mtx_unlock( &tracker->mutex );
}

typedef struct crude_gfx_gpu_device_shader_database_
{
} crude_gfx_gpu_device_shader_database_;

static void
crude_gfx_gpu_device_shader_database_initialize_
(
  _In_ crude_gfx_gpu_device_shader_database_              *database
)
{
}

void
crude_gfx_gpu_crash_tracker_initialize
(
  _In_ crude_gfx_gpu_crash_tracker                        *tracker,
  _In_ crude_heap_allocator                               *allocator
)
{
  tracker->crash_count = 0;
  tracker->allocator = allocator;
  mtx_init( &tracker->mutex, mtx_plain | mtx_recursive );

  crude_gfx_gpu_device_shader_database_initialize_( tracker->shader_database );

  CRUDE_GFX_AFTERMATH_CHECK_ERROR( GFSDK_Aftermath_EnableGpuCrashDumps(
    GFSDK_Aftermath_Version_API,
    GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
    GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,
    crude_gfx_gpu_device_crash_dump_callback_,
    crude_gfx_gpu_crash_tracker_shader_debug_info_callback_,
    crude_gfx_gpu_crash_tracker_crash_dump_description_callback_,
    crude_gfx_gpu_crash_tracker_resolve_marker_callback_,
    tracker ) );
}

void
crude_gfx_gpu_crash_tracker_deinitialize
(
  _In_ crude_gfx_gpu_crash_tracker                        *tracker
)
{
  GFSDK_Aftermath_DisableGpuCrashDumps( );
  mtx_destroy( &tracker->mutex );
}

void
crude_gfx_gpu_crash_tracker_handle_device_lost
(
  _In_ crude_gfx_gpu_crash_tracker                        *tracker
)
{
  int64                                                    start_time;
  GFSDK_Aftermath_CrashDump_Status                         gfsdk_status;

  start_time = crude_time_now( );

  gfsdk_status = GFSDK_Aftermath_CrashDump_Status_Unknown;
  CRUDE_GFX_AFTERMATH_CHECK_ERROR( GFSDK_Aftermath_GetCrashDumpStatus( &gfsdk_status ) );

  while
  (
    gfsdk_status != GFSDK_Aftermath_CrashDump_Status_CollectingDataFailed && 
    gfsdk_status != GFSDK_Aftermath_CrashDump_Status_Finished &&
    crude_time_delta_seconds( start_time, crude_time_now( ) ) < 3.f 
  )
  {
    timespec                                               timespec;
    timespec.tv_sec = 0.05;
    thrd_sleep( &timespec, NULL );

    CRUDE_GFX_AFTERMATH_CHECK_ERROR( GFSDK_Aftermath_GetCrashDumpStatus( &gfsdk_status) );
  }
  
  if ( gfsdk_status != GFSDK_Aftermath_CrashDump_Status_Finished )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Unexpected crash dump status %i", gfsdk_status );
  }
  
  CRUDE_DEBUG_BREAK;
}

#endif