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
  crude_resource_pool_initialize( &audio->sounds_groups, crude_heap_allocator_pack( audio->allocator ), 512, sizeof( ma_sound_group ) );

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
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_AUDIO, "Failed initialize audio device!" );
    return;
  }

  lma_engine_config = ma_engine_config_init();
  lma_engine_config.pDevice = &audio->lma_device;

  if ( ma_engine_init( &lma_engine_config, &audio->lma_engine ) != MA_SUCCESS )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_AUDIO, "Failed initialize audio engine!" );
    return;
  }
  
  if ( ma_fence_init( &audio->lma_fence ) != MA_SUCCESS )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_AUDIO, "Failed initialize audio fence!" );
    return;
  }
}

void
crude_audio_device_deinitialize
(
  _In_ crude_audio_device                                  *audio
)
{
  ma_fence_uninit( &audio->lma_fence );
  ma_engine_uninit( &audio->lma_engine );
  ma_device_uninit( &audio->lma_device );
  ma_context_uninit( &audio->lma_context );
  crude_resource_pool_deinitialize( &audio->sounds );
  crude_resource_pool_deinitialize( &audio->sounds_groups );
}

void
crude_audio_device_wait_wait_till_uploaded
(
  _In_ crude_audio_device                                  *audio
)
{
  ma_fence_wait( &audio->lma_fence );
}

crude_sound_group_handle
crude_audio_device_create_sound_group
(
  _In_ crude_audio_device                                  *audio
)
{
  ma_sound_group                                          *lma_sound_group;
  crude_sound_group_handle                                 sound_group_handle; 
  ma_result                                                result;
  ma_uint32                                                sounnd_flags;

  sound_group_handle.index = crude_resource_pool_obtain_resource( &audio->sounds_groups );
  lma_sound_group = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds_groups, sound_group_handle.index ) );
  result = ma_sound_group_init( &audio->lma_engine, 0u, NULL, lma_sound_group );
  if ( result != MA_SUCCESS )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_AUDIO, "Failed load sound group!" );
    return CRUDE_SOUND_GROUP_HANDLE_INVALID;
  }
  return sound_group_handle;
}

void
crude_audio_device_destroy_sound_group
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_group_handle                             sound_group_handle
)
{
  ma_sound_group                                          *lma_sound_group;
  lma_sound_group = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds, sound_group_handle.index ) );
  ma_sound_group_uninit( lma_sound_group );
  crude_resource_pool_release_resource(  &audio->sounds_groups, sound_group_handle.index );
}

void
crude_audio_device_start_sound_group
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_group_handle                             sound_group_handle
)
{
  ma_sound_group                                          *lma_sound_group;
  lma_sound_group = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds, sound_group_handle.index ) );
  ma_sound_group_start( lma_sound_group );
}

void
crude_audio_device_stop_sound_group
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_group_handle                             sound_group_handle
)
{
  ma_sound_group                                          *lma_sound_group;
  lma_sound_group = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds, sound_group_handle.index ) );
  ma_sound_group_stop( lma_sound_group );
}

void
crude_audio_device_sound_group_set_volume
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_group_handle                             sound_group_handle,
  _In_ float32                                              volume
)
{
  ma_sound_group                                          *lma_sound_group;
  lma_sound_group = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds, sound_group_handle.index ) );
  ma_sound_group_set_volume( lma_sound_group, volume );
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
  ma_uint32                                                sounnd_flags;

  sound_handle.index = crude_resource_pool_obtain_resource( &audio->sounds );
  lma_sound = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds, sound_handle.index ) );

  sounnd_flags = MA_SOUND_FLAG_NO_PITCH;
  if ( creation->decode )
  {
    sounnd_flags |= MA_SOUND_FLAG_DECODE;
  }
  if ( creation->async_loading )
  {
    sounnd_flags |= MA_SOUND_FLAG_ASYNC;
  }
  if ( creation->stream )
  {
    sounnd_flags |= MA_SOUND_FLAG_STREAM;
  }

  if ( creation->looping )
  {
    sounnd_flags |= MA_SOUND_FLAG_LOOPING;
  }
  
  result = ma_sound_init_from_file( &audio->lma_engine, creation->absolute_filepath, sounnd_flags, NULL, creation->async_loading ? &audio->lma_fence : NULL, lma_sound );
  if ( result != MA_SUCCESS )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_AUDIO, "Failed load sound \"%s\"", creation->absolute_filepath );
    return CRUDE_SOUND_HANDLE_INVALID;
  }

  ma_sound_set_positioning( lma_sound, CRUDE_CAST( ma_positioning, creation->positioning ) );
  ma_sound_set_attenuation_model( lma_sound, CRUDE_CAST( ma_attenuation_model, creation->attenuation_model ) );

  if ( creation->max_distance )
  {
    ma_sound_set_max_distance( lma_sound, creation->max_distance );
  }

  if ( creation->min_distance )
  {
    ma_sound_set_min_distance( lma_sound, creation->min_distance );
  }

  if ( creation->rolloff )
  {
    ma_sound_set_rolloff( lma_sound, creation->rolloff );
  }
  
  return sound_handle;
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
  crude_resource_pool_release_resource(  &audio->sounds, sound_handle.index );
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
crude_audio_device_sound_stop
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_handle                                   sound_handle
)
{ 
  ma_sound                                                *lma_sound;
  lma_sound = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds, sound_handle.index ) );
  ma_sound_stop( lma_sound );
}

bool
crude_audio_device_sound_is_playing
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_handle                                   sound_handle
)
{
  ma_sound                                                *lma_sound;
  lma_sound = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds, sound_handle.index ) );
  return ma_sound_is_playing( lma_sound );
}


void
crude_audio_device_sound_set_translation
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_handle                                   sound_handle,
  _In_ XMVECTOR                                             translation
)
{
  ma_sound                                                *lma_sound;
  lma_sound = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds, sound_handle.index ) );
  ma_sound_set_position( lma_sound, XMVectorGetX( translation ), XMVectorGetY( translation ), XMVectorGetZ( translation ) );
}

void
crude_audio_device_sound_set_volume
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_handle                                   sound_handle,
  _In_ float32                                              volume
)
{
  ma_sound                                                *lma_sound;
  lma_sound = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds, sound_handle.index ) );
  ma_sound_set_volume( lma_sound, volume );
}

void
crude_audio_device_sound_set_attenuation_model
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_handle                                   sound_handle,
  _In_ crude_audio_sound_attenuation_model                  attenuation_model
)
{
  ma_sound                                                *lma_sound;
  lma_sound = CRUDE_CAST( ma_sound*, crude_resource_pool_access_resource( &audio->sounds, sound_handle.index ) );
  ma_sound_set_attenuation_model( lma_sound, CRUDE_CAST( ma_attenuation_model, attenuation_model ) );
}

void
crude_audio_device_listener_set_local_to_world
(
  _In_ crude_audio_device                                  *audio,
  _In_ XMMATRIX                                             local_to_world
)
{
  XMFLOAT3                                                 forward;
  XMFLOAT3                                                 translation;

  XMStoreFloat3( &translation, local_to_world.r[ 3 ] );
  XMStoreFloat3( &forward, XMVector3TransformNormal( XMVectorSet( 0, 0, -1, 0 ), local_to_world ) );

  ma_engine_listener_set_position( &audio->lma_engine, 0, translation.x, translation.y, translation.z );
  ma_engine_listener_set_direction( &audio->lma_engine, 0, forward.x, forward.y, forward.z );
}

void
crude_audio_device_set_global_volume
(
  _In_ crude_audio_device                                  *audio,
  _In_ float32                                              volume
)
{
  ma_engine_set_volume( &audio->lma_engine, volume );
}