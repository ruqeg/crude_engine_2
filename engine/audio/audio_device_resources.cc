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