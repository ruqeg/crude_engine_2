#include <engine/audio/audio_components.h>

ECS_COMPONENT_DECLARE( crude_audio_listener );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_audio_components )
{
  ECS_MODULE( world, crude_audio_components );
  ECS_COMPONENT_DEFINE( world, crude_audio_listener );
}