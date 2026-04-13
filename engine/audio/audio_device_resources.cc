#include <engine/audio/audio_device_resources.h>

crude_sound_creation
crude_sound_creation_empty
(
)
{
  crude_sound_creation creation = CRUDE_COMPOUNT_EMPTY( crude_sound_creation );
  creation.attenuation_model = CRUDE_AUDIO_SOUND_ATTENUATION_MODEL_INVERSE;
  creation.rolloff = 1.f;
  creation.sound_group_handle = CRUDE_SOUND_GROUP_HANDLE_INVALID;
  return creation;
}

crude_audio_player
crude_audio_player_empty
(
)
{
  crude_audio_player audio_player = CRUDE_COMPOUNT_EMPTY( crude_audio_player );
  audio_player.looping = true;
  audio_player.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
  audio_player.relative_filepath[ 0 ] = 0;
  audio_player.stream = true;
  return audio_player;
}