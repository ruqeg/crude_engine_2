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
  ma_engine_read_pcm_frames( &audio->lma_engine, output, frame_count, NULL );
}

void
crude_audio_device_initialize
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_heap_allocator                                *allocator
)
{
  ma_device_info                                          *lma_playback_infos;
  ma_uint32                                                lma_playback_count;
  ma_engine_config                                         lma_engine_config;
  ma_device_config                                         lma_device_config;
  uint32                                                   selected_device_index;

  audio->allocator = allocator;

  crude_resource_pool_initialize( &audio->sounds, crude_heap_allocator_pack( audio->allocator ), 512, sizeof( ma_sound ) );

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
  lma_device_config.playback.format = ma_format_f32;
  lma_device_config.playback.channels = 2;
  lma_device_config.sampleRate = 48000;
  lma_device_config.dataCallback = crude_audio_device_data_callback;
  lma_device_config.pUserData = audio;

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

crude_sound_handle
crude_audio_device_create_sound
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_creation const                          *creation
)
{
  ma_sound                                                *lma_sound;
  crude_sound_handle                                       sound_handle;
  ma_result                                                result;

  sound_handle.index = crude_resource_pool_obtain_resource( &audio->sounds );
  lma_sound = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds, sound_handle.index ) );
  result = ma_sound_init_from_file( &audio->lma_engine, creation->absolute_filepath, MA_SOUND_FLAG_DECODE, NULL, NULL, lma_sound );
  if ( result != MA_SUCCESS)
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_AUDIO, "Failed load sound \"%s\"", creation->absolute_filepath );
    return CRUDE_SOUND_HANDLE_INVALID;
  }
  ma_sound_set_looping( lma_sound, creation->looping );
  return sound_handle;
}

void
crude_audio_device_sound_start
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_handle                                   sound_handle
)
{ 
  ma_sound                                                *lma_sound;
  lma_sound = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds, sound_handle.index ) );
  ma_sound_start( lma_sound );
}

void
crude_audio_device_destroy_sound
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_handle                                   sound_handle
)
{
  ma_sound                                                *lma_sound;
  lma_sound = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds, sound_handle.index ) );
  ma_sound_uninit( lma_sound );
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
  crude_resource_pool_deinitialize( &audio->sounds );
}