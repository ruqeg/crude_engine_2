#include <cgltf.h>
#include <stb_ds.h>

#include <core/assert.h>
#include <core/log.h>
#include <graphics/renderer.h>

#include <resources/gltf_loader.h>

void
crude_load_gltf_from_file
(
  _In_ crude_renderer  *renderer,
  _In_ char const      *path,
  _Out_ crude_scene    *scene
)
{
	cgltf_options gltf_options = { 0 };
	cgltf_data *gltf_scene = NULL;
	cgltf_result result = cgltf_parse_file( &gltf_options, path, &gltf_scene );
	if ( result != cgltf_result_success )
	{
		CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to parse gltf file: %s", path );
	}

  result = cgltf_load_buffers( &gltf_options, gltf_scene, path );
	if ( result != cgltf_result_success )
	{
		CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to load buffers from gltf file: %s", path );
	}
  
  scene->buffers = NULL;
  arrsetlen( scene->buffers, gltf_scene->buffer_views_count );

  for ( uint32 buffer_index = 0; buffer_index < gltf_scene->buffer_views_count; ++buffer_index )
  {
    cgltf_buffer_view *buffer = &gltf_scene->buffer_views[ buffer_index ];
  
    uint8* data = ( uint8* )buffer->data + buffer->offset;
  
    if ( buffer->name == NULL )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_FILEIO, "Bufer name is null: %u", buffer_index );
    }

    crude_buffer_creation buffer_creation = {
      .initial_data = data,
      .usage = CRUDE_RESOURCE_USAGE_TYPE_IMMUTABLE,
      .size = buffer->size,
      .type_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      .name = buffer->name
    };
    crude_buffer_resource *buffer_resource = crude_gfx_renderer_create_buffer( renderer, &buffer_creation );
    arrpush( scene->buffers, *buffer_resource );
  }

  scene->mesh_draws = NULL;
  arrsetlen( scene->mesh_draws, gltf_scene->meshes_count );

  arrfree( scene->mesh_draws );
  arrfree( scene->buffers );
	cgltf_free( gltf_scene );
}