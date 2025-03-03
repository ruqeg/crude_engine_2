#include <cgltf.h>
#include <stb_ds.h>

#include <core/assert.h>
#include <core/log.h>
#include <graphics/renderer.h>

#include <resources/gltf_loader.h>

typedef struct crude_mesh_draw
{
  uint32 todo;
} crude_mesh_draw;

typedef struct crude_scene
{
	crude_buffer_resource *buffers;
  crude_mesh_draw       *mesh_draws;
} crude_scene;

void
crude_load_gltf_from_file
(
  _In_ char const    *path,
  _Out_ crude_scene  *scene
)
{
	cgltf_options gltf_options = { 0 };
	cgltf_data *gltf_scene = NULL;
	cgltf_result result = cgltf_parse_file( &gltf_options, path, &gltf_scene );
	if ( result != cgltf_result_success )
	{
		CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to parse gltf file: %s", path );
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
  
    VkBufferUsageFlags flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    //crude_buffer_resource *buffer_resource = renderer.create_buffer( flags, ResourceUsageType::Immutable, buffer.byte_length, data, buffer_name );
    CRUDE_ASSERT( buffer->name );
  
    //arrpush( scene->buffers, *buffer_resource );
  }

  scene->mesh_draws = NULL;
  arrsetlen( scene->mesh_draws, gltf_scene->meshes_count );

  arrfree( scene->mesh_draws );
  arrfree( scene->buffers );
	cgltf_free( gltf_scene );
}