#include <thirdparty/cgltf/cgltf.h>

#include <engine/core/hash_map.h>
#include <engine/core/file.h>

#include <engine/scene/collisions_resources_manager.h>

static cgltf_data*
crude_collisions_resources_manager_gltf_parse_
(
  _In_ crude_heap_allocator                               *gltf_allocator,
  _In_ char const                                         *gltf_path
)
{
  cgltf_data                                              *gltf;
  cgltf_result                                             result;
  cgltf_options                                            gltf_options;
  crude_allocator_container                                gltf_allocator_container;

  gltf_allocator_container = crude_heap_allocator_pack( gltf_allocator );

  gltf_options = CRUDE_COMPOUNT_EMPTY( cgltf_options );
  gltf_options.memory.alloc_func = gltf_allocator_container.allocate;
  gltf_options.memory.free_func = gltf_allocator_container.deallocate;
  gltf_options.memory.user_data = gltf_allocator_container.ctx;

  gltf = NULL;
  result = cgltf_parse_file( &gltf_options, gltf_path, &gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, false, "Failed to parse gltf file: %s", gltf_path );
    return NULL;
  }

  result = cgltf_load_buffers( &gltf_options, gltf, gltf_path );
  if ( result != cgltf_result_success )
  {
    CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, false, "Failed to load buffers from gltf file: %s", gltf_path );
    return NULL;
  }

  result = cgltf_validate( gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, false, "Failed to validate gltf file: %s", gltf_path );
    return NULL;
  }

  return gltf;
}


static void
crude_gfx_model_renderer_resources_manager_gltf_load_nodes_
(
  _In_ crude_collisions_resources_manager                 *manager,
  _In_ cgltf_data                                         *gltf,
  _In_ cgltf_node                                        **gltf_nodes,
  _In_ uint32                                              gltf_nodes_count,
  _In_ crude_octree                                       *octree,
  _In_ XMMATRIX                                            parent_to_world
)
{ 
  for ( uint32 i = 0u; i < gltf_nodes_count; ++i )
  {
    XMMATRIX                                               node_to_world;
    XMMATRIX                                               node_to_parent;
    XMVECTOR                                               translation;
    XMVECTOR                                               rotation;
    XMVECTOR                                               scale;

    if ( gltf_nodes[ i ]->has_translation )
    {
      translation = XMVectorSet( gltf_nodes[ i ]->translation[ 0 ], gltf_nodes[ i ]->translation[ 1 ], gltf_nodes[ i ]->translation[ 2 ], 1  );
    }
    else
    {
      translation = XMVectorZero( );
    }

    if ( gltf_nodes[ i ]->has_scale )
    {
      scale = XMVectorSet( gltf_nodes[ i ]->scale[ 0 ], gltf_nodes[ i ]->scale[ 1 ], gltf_nodes[ i ]->scale[ 2 ], 1 );
    }
    else
    {
      scale = XMVectorReplicate( 1.f );
    }

    if ( gltf_nodes[ i ]->has_rotation )
    {
      rotation = XMVectorSet( gltf_nodes[ i ]->rotation[ 0 ], gltf_nodes[ i ]->rotation[ 1 ], gltf_nodes[ i ]->rotation[ 2 ], gltf_nodes[ i ]->rotation[ 3 ] );
    }
    else
    {
      rotation = XMQuaternionIdentity( );
    }

    node_to_parent = XMMatrixAffineTransformation( scale, XMVectorZero( ), rotation, translation );

    if ( gltf_nodes[ i ]->has_matrix )
    {
      XMVECTOR                                             decompose_scale;
      XMVECTOR                                             decompose_rotation_quat;
      XMVECTOR                                             decompose_translation;
      XMFLOAT4X4                                           gltf_node_matrix;

      CRUDE_ASSERT( !gltf_nodes[ i ]->has_translation );
      CRUDE_ASSERT( !gltf_nodes[ i ]->has_scale );
      CRUDE_ASSERT( !gltf_nodes[ i ]->has_rotation );
      gltf_node_matrix = CRUDE_COMPOUNT( XMFLOAT4X4, { gltf_nodes[ i ]->matrix } );
      node_to_parent = XMLoadFloat4x4( &gltf_node_matrix );
    }
    
    node_to_world = XMMatrixMultiply( node_to_parent, parent_to_world );

    if ( gltf_nodes[ i ]->mesh )
    {
      for ( uint32 pi = 0; pi < gltf_nodes[ i ]->mesh->primitives_count; ++pi )
      {
        XMFLOAT3                                          *positions;
        cgltf_primitive                                   *mesh_primitive;
        uint16                                            *indices;
        uint32                                             vertices_count;
        uint32                                             indices_count;
        
        mesh_primitive = &gltf_nodes[ i ]->mesh->primitives[ pi ];
        positions = NULL;

        indices_count = mesh_primitive->indices->count;
        indices = CRUDE_CAST( uint16*, CRUDE_CAST( uint8*, mesh_primitive->indices->buffer_view->buffer->data ) + mesh_primitive->indices->buffer_view->offset + mesh_primitive->indices->offset );
  
        CRUDE_ASSERT( mesh_primitive->indices->type == cgltf_type_scalar );
        CRUDE_ASSERT( mesh_primitive->indices->component_type == cgltf_component_type_r_16u ); // change ray tracing index property in geometry 

        vertices_count = mesh_primitive->attributes[ 0 ].data->count;
  
        for ( uint32 i = 0; i < mesh_primitive->attributes_count; ++i )
        {
          cgltf_attribute *attribute = &mesh_primitive->attributes[ i ];
          CRUDE_ASSERT( vertices_count == attribute->data->count );

          uint8 *attribute_data = CRUDE_STATIC_CAST( uint8*, attribute->data->buffer_view->buffer->data ) + attribute->data->buffer_view->offset + attribute->data->offset;
          switch ( attribute->type )
          {
          case cgltf_attribute_type_position:
          {
            CRUDE_ASSERT( attribute->data->type == cgltf_type_vec3 );
            CRUDE_ASSERT( attribute->data->stride == sizeof( XMFLOAT3 ) );
            positions = CRUDE_CAST( XMFLOAT3*, attribute_data );
            break;
          }
          }
        }
        
        CRUDE_ASSERT( positions );

        for ( uint32 i = 0; i < indices_count; i += 3 )
        {
          XMVECTOR                                        t0, t1, t2;
          
          t0 = XMVector4Transform( XMVectorSetW( XMLoadFloat3( &positions[ indices[ i ] ] ), 1 ), node_to_world );
          t1 = XMVector4Transform( XMVectorSetW( XMLoadFloat3( &positions[ indices[ i + 1 ] ] ), 1 ), node_to_world );
          t2 = XMVector4Transform( XMVectorSetW( XMLoadFloat3( &positions[ indices[ i + 2 ] ] ), 1 ), node_to_world );
          
          crude_octree_add_triangle( octree, t0, t1, t2 );
        }
      }
    }

    crude_gfx_model_renderer_resources_manager_gltf_load_nodes_( manager, gltf, gltf_nodes[ i ]->children, gltf_nodes[ i ]->children_count, octree, node_to_parent );
  }
}

static crude_octree_handle
crude_collisions_resources_manager_load_octree_from_gltf_
(
  _In_ crude_collisions_resources_manager                 *manager,
  _In_ char const                                         *gltf_path
)
{
  cgltf_data                                              *gltf;
  crude_octree                                            *octree;
  crude_octree_handle                                      octree_handle;
  char                                                     gltf_directory[ 512 ];
  uint64                                                   temporary_allocator_marker, meshes_count, images_offset, samplers_offset, buffers_offset;

  /* Parse gltf */
  gltf = crude_collisions_resources_manager_gltf_parse_( manager->cgltf_temporary_allocator, gltf_path );
  if ( !gltf )
  {
    goto cleanup;
  }
  
  octree_handle = CRUDE_COMPOUNT( crude_octree_handle, { crude_resource_pool_obtain_resource( &manager->octree_resource_pool ) } );
  octree = CRUDE_CAST( crude_octree*, crude_resource_pool_access_resource( &manager->octree_resource_pool, octree_handle.index ) );

  crude_octree_initialize( octree, crude_heap_allocator_pack( manager->allocator ) );

  crude_memory_copy( gltf_directory, gltf_path, sizeof( gltf_directory ) );
  crude_file_directory_from_path( gltf_directory );
  
  for ( uint32 i = 0; i < gltf->scenes_count; ++i )
  {
    crude_gfx_model_renderer_resources_manager_gltf_load_nodes_( manager, gltf, gltf->scene[ i ].nodes, gltf->scene[ i ].nodes_count, octree, XMMatrixIdentity( ) );
  }

cleanup:
  if ( gltf )
  {
    cgltf_free( gltf );
  }
  return octree_handle;
}

void
crude_collisions_resources_manager_initialize
(
  _In_ crude_collisions_resources_manager                 *manager,
  _In_ crude_heap_allocator                               *allocator,
  _In_ crude_heap_allocator                               *cgltf_temporary_allocator
)
{
  manager->allocator = allocator;
  manager->cgltf_temporary_allocator = cgltf_temporary_allocator;

  CRUDE_HASHMAP_INITIALIZE( manager->name_hashed_to_octree_resource_hadle, crude_heap_allocator_pack( allocator ) );
  crude_resource_pool_initialize( &manager->octree_resource_pool, crude_heap_allocator_pack( allocator ), CRUDE_SCENE_OCTREE_RESOURCE_POOl_SIZE, sizeof( crude_octree ) );
}

void
crude_collisions_resources_manager_deinitialize
(
  _In_ crude_collisions_resources_manager                 *manager
)
{
  for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( manager->name_hashed_to_octree_resource_hadle ); ++i )
  {
    if ( crude_hashmap_backet_key_valid( manager->name_hashed_to_octree_resource_hadle[ i ].key ) )
    {
      crude_octree *octree = CRUDE_CAST( crude_octree*, crude_resource_pool_access_resource( &manager->octree_resource_pool, manager->name_hashed_to_octree_resource_hadle[ i ].value.index ) );
      crude_octree_deinitialize( octree );
      crude_resource_pool_release_resource( &manager->octree_resource_pool, manager->name_hashed_to_octree_resource_hadle[ i ].value.index );
    }
  }
  CRUDE_HASHMAP_DEINITIALIZE( manager->name_hashed_to_octree_resource_hadle );
  crude_resource_pool_deinitialize( &manager->octree_resource_pool );
}

crude_octree_handle
crude_collisions_resources_manager_get_octree_handle
(
  _In_ crude_collisions_resources_manager                 *manager,
  _In_ char const                                         *filepath
)
{
  crude_octree_handle                                      handle;
  int64                                                    handle_index;
  uint64                                                   name_hashed;

  name_hashed = crude_hash_string( filepath, 0 );
  handle_index = CRUDE_HASHMAP_GET_INDEX( manager->name_hashed_to_octree_resource_hadle, name_hashed );
  if ( handle_index != -1 )
  {
    return manager->name_hashed_to_octree_resource_hadle[ handle_index ].value;
  }

  handle = crude_collisions_resources_manager_load_octree_from_gltf_( manager, filepath );
  CRUDE_HASHMAP_SET( manager->name_hashed_to_octree_resource_hadle, name_hashed, handle );
  return handle;
}

crude_octree*
crude_collisions_resources_manager_access_octree
(
  _In_ crude_collisions_resources_manager                 *manager,
  _In_ crude_octree_handle                                 handle
)
{
  return CRUDE_CAST( crude_octree*, crude_resource_pool_access_resource( &manager->octree_resource_pool, handle.index ) );
}