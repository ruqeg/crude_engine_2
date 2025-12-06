#include <engine/core/log.h>

#include <engine/audio/audio_device.h>

static void
crude_audio_device_data_callback
(
  _In_ ma_device                                          *lma_device,
  _In_ void                                               *output,
  _In_ const void                                         *input,
  _In_ ma_uint32                                           frame_count
)
{
  crude_audio_device *audio = CRUDE_CAST( crude_audio_device*, lma_device->pUserData );
  //ma_engine_read_pcm_frames( &audio->lma_engine, output, frame_count, NULL );
}

void
crude_audio_device_initialize
(
  _In_ crude_audio_device                                  *audio
)
{
  ma_device_info                                          *lma_playback_infos;
  ma_uint32                                                lma_playback_count;
  ma_engine_config                                         lma_engine_config;
  ma_device_config                                         lma_device_config;
  uint32                                                   selected_device_index;

  if ( ma_context_init( NULL, 0, NULL, &audio->lma_context ) != MA_SUCCESS )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_AUDIO, "Can't iniitalize audio context!" );
    return;
  }
  
  if ( ma_context_get_devices( &audio->lma_context, &lma_playback_infos, &lma_playback_count, NULL, NULL ) != MA_SUCCESS )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_AUDIO, "Can't get audio devices list!" );
    return;
  }

  for ( uint32 i = 0; i < lma_playback_count; ++i )
  {
    CRUDE_LOG_INFO( CRUDE_CHANNEL_AUDIO, "Avalivable \"%s\" device.", lma_playback_infos[ i ].name );
    if ( lma_playback_infos[ i ].isDefault )
    {
      selected_device_index = i;
      CRUDE_LOG_INFO( CRUDE_CHANNEL_AUDIO, "Selected \"%s\" device.", lma_playback_infos[ i ].name );
    }
  }

  lma_device_config = ma_device_config_init(ma_device_type_playback);
  lma_device_config.playback.pDeviceID = &lma_playback_infos[ selected_device_index ].id;
  lma_device_config.playback.format = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
  lma_device_config.playback.channels = 2;               // Set to 0 to use the device's native channel count.
  lma_device_config.sampleRate = 48000;           // Set to 0 to use the device's native sample rate.
  lma_device_config.dataCallback = crude_audio_device_data_callback;   // This function will be called when miniaudio needs more data.
  lma_device_config.pUserData = audio;   // Can be accessed from the device object (device.pUserData).

  if ( ma_device_init( NULL, &lma_device_config, &audio->lma_device ) != MA_SUCCESS )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_AUDIO, "Can't initialize audio device!" );
    return;
  }

  lma_engine_config = ma_engine_config_init();
  lma_engine_config.pDevice = &audio->lma_device;

  if ( ma_engine_init( &lma_engine_config, &audio->lma_engine ) != MA_SUCCESS )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_AUDIO, "Can't initialize audio engine!" );
    return;
  }
}

void
crude_audio_device_deinitialize
(
  _In_ crude_audio_device                                  *audio
)
{
  ma_engine_uninit( &audio->lma_engine );
  ma_device_uninit( &audio->lma_device );
  ma_context_uninit( &audio->lma_context );
}