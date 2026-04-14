#include <thirdparty/stb/stb_image.h>

#include <engine/graphics/asynchronous_loader_manager.h>

#include <engine/graphics/texture_manager.h>

void
crude_gfx_texture_manager_initialize
(
  _In_ crude_gfx_texture_manager                          *manager,
  _In_ crude_gfx_asynchronous_loader                      *asynchronous_loader,
  _In_ crude_heap_allocator                               *texture_manager_allocator
)
{
  manager->asynchronous_loader = asynchronous_loader;
  manager->texture_manager_allocator = texture_manager_allocator;
  CRUDE_HASHMAPSTR_INITIALIZE( manager->texture_relative_filepath_to_handle, crude_heap_allocator_pack( texture_manager_allocator ) );
}

void
crude_gfx_texture_manager_deinitialize
(
  _In_ crude_gfx_texture_manager                          *manager
)
{
  crude_gfx_texture_manager_clear( manager );
  CRUDE_HASHMAPSTR_DEINITIALIZE( manager->texture_relative_filepath_to_handle );
}

void
crude_gfx_texture_manager_clear
(
  _In_ crude_gfx_texture_manager                          *manager
)
{
  for ( uint32 i = 0; i < CRUDE_HASHMAPSTR_CAPACITY( manager->texture_relative_filepath_to_handle ); ++i )
  {
    if ( crude_hashmapstr_backet_key_hash_valid( manager->texture_relative_filepath_to_handle[ i ].key.key_hash ) )
    {
      crude_gfx_destroy_texture_instant( manager->asynchronous_loader->gpu, manager->texture_relative_filepath_to_handle[ i ].value );
    }
    manager->texture_relative_filepath_to_handle[ i ].key.key_hash = CRUDE_HASHMAPSTR_BACKET_STATE_EMPTY;
  }
  CRUDE_HASHMAPSTR_DEINITIALIZE( manager->texture_relative_filepath_to_handle );
  CRUDE_HASHMAPSTR_INITIALIZE( manager->texture_relative_filepath_to_handle, crude_heap_allocator_pack( manager->texture_manager_allocator ) );
}

crude_gfx_texture_handle
crude_gfx_texture_manager_get_texture
(
  _In_ crude_gfx_texture_manager                          *manager,
  _In_ char const                                         *relative_filepath,
  _In_ char const                                         *absolute_filepath
)
{
  crude_gfx_texture_handle                               texture_handle;
  crude_gfx_texture_creation                             texture_creation;
  int32                                                  texture_index, comp, width, height;
  
  texture_index = CRUDE_HASHMAPSTR_GET_INDEX( manager->texture_relative_filepath_to_handle, relative_filepath );
  if ( texture_index != -1 )
  {
    return manager->texture_relative_filepath_to_handle[ texture_index ].value;
  }

  stbi_info( absolute_filepath, &width, &height, &comp );
    
  CRUDE_ASSERT( ( width > 0 ) && ( height > 0 ) );

  texture_creation = crude_gfx_texture_creation_empty();
  texture_creation.initial_data = NULL;
  texture_creation.width = width;
  texture_creation.height = height;
  texture_creation.depth = 1u;
  texture_creation.flags = 0u;
  texture_creation.format = VK_FORMAT_R8G8B8A8_UNORM;
  texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
  texture_creation.subresource.mip_level_count = log2( CRUDE_MAX( width, height ) ) + 1;
  crude_string_copy( texture_creation.name, relative_filepath, sizeof( texture_creation.name ) );

  texture_handle = crude_gfx_create_texture( manager->asynchronous_loader->gpu, &texture_creation );
  crude_gfx_asynchronous_loader_request_texture_data( manager->asynchronous_loader, absolute_filepath, texture_handle );

  CRUDE_HASHMAPSTR_SET( manager->texture_relative_filepath_to_handle, CRUDE_COMPOUNT( crude_string_link, { texture_creation.name } ), texture_handle );

  return texture_handle;
}