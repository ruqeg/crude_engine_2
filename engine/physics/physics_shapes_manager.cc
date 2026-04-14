#include <thirdparty/cgltf/cgltf.h>

#include <engine/core/hashmapstr.h>
#include <engine/core/file.h>
#include <engine/physics/physics.h>

#include <engine/physics/physics_shapes_manager.h>

static cgltf_data*
crude_physics_shapes_manager_gltf_parse_
(
  _In_ crude_heap_allocator                               *gltf_allocator,
  _In_ char const                                         *gltf_path
);

static void
crude_physics_shapes_manager_gltf_load_nodes_
(
  _In_ crude_physics_shapes_manager                       *manager,
  _In_ cgltf_data                                         *gltf,
  _In_ cgltf_node                                        **gltf_nodes,
  _In_ uint32                                              gltf_nodes_count,
  _Out_ JPH::Array< JPH::Triangle >                       *jph_triangles,
  _In_ XMMATRIX                                            parent_to_world
);

static crude_physics_mesh_shape_handle
crude_physics_shapes_manager_load_mesh_shape_from_gltf_
(
  _In_ crude_physics_shapes_manager                       *manager,
  _In_ char const                                         *gltf_realtive_filepath
);

void
crude_physics_shapes_manager_initialize
(
  _In_ crude_physics_shapes_manager                       *manager,
  _In_ crude_physics_shapes_manager_creation const        *creation
)
{
  manager->allocator = creation->allocator;
  manager->cgltf_temporary_allocator = creation->cgltf_temporary_allocator;
  manager->physics_manager = creation->physics_manager;
  manager->resources_absolute_directory = creation->resources_absolute_directory;
#if CRUDE_DEVELOP
  manager->model_renderer_resources_manager = creation->model_renderer_resources_manager;
#endif

  CRUDE_HASHMAPSTR_INITIALIZE( manager->mesh_shape_relative_filepath_to_hadle, crude_heap_allocator_pack( manager->allocator ) );
  crude_resource_pool_initialize( &manager->mesh_shape_resource_pool, crude_heap_allocator_pack( manager->allocator ), 256, sizeof( crude_physics_mesh_shape_container ) );
  crude_string_buffer_initialize( &manager->gltf_absolute_filepath_string_buffer, CRUDE_RKILO( 16 ), crude_heap_allocator_pack( manager->allocator ) );
}

void
crude_physics_shapes_manager_deinitialize
(
  _In_ crude_physics_shapes_manager                       *manager
)
{
  crude_physics_shapes_manager_clear( manager );
  CRUDE_HASHMAPSTR_DEINITIALIZE( manager->mesh_shape_relative_filepath_to_hadle );
  crude_resource_pool_deinitialize( &manager->mesh_shape_resource_pool );
  crude_string_buffer_deinitialize( &manager->gltf_absolute_filepath_string_buffer );
}

crude_physics_mesh_shape_handle
crude_physics_shapes_manager_get_mesh_shape_handle
(
  _In_ crude_physics_shapes_manager                       *manager,
  _In_ char const                                         *relative_filepath
)
{
  crude_physics_mesh_shape_container                      *mesh_shape_container;
  crude_physics_mesh_shape_handle                          mesh_shape_handle;
  int64                                                    handle_index;

  handle_index = CRUDE_HASHMAPSTR_GET_INDEX( manager->mesh_shape_relative_filepath_to_hadle, relative_filepath );
  if ( handle_index != -1 )
  {
    return manager->mesh_shape_relative_filepath_to_hadle[ handle_index ].value;
  }

  mesh_shape_handle = crude_physics_shapes_manager_load_mesh_shape_from_gltf_( manager, relative_filepath );
  mesh_shape_container = crude_physics_shapes_manager_access_mesh_shape( manager, mesh_shape_handle );
  CRUDE_HASHMAPSTR_SET( manager->mesh_shape_relative_filepath_to_hadle, CRUDE_COMPOUNT( crude_string_link, { mesh_shape_container->relative_filepath } ), mesh_shape_handle );
#if CRUDE_DEVELOP
  crude_gfx_model_renderer_resources_instance_initialize(
    &mesh_shape_container->debug_model_renderer_resource_instance,
    manager->model_renderer_resources_manager,
    crude_gfx_model_renderer_resources_manager_get_gltf_model( manager->model_renderer_resources_manager, relative_filepath, NULL ) );
  CRUDE_HASHMAPSTR_SET( manager->mesh_shape_relative_filepath_to_hadle, CRUDE_COMPOUNT( crude_string_link, { mesh_shape_container->relative_filepath } ), mesh_shape_handle );
#endif
  return mesh_shape_handle;
}

crude_physics_mesh_shape_container*
crude_physics_shapes_manager_access_mesh_shape
(
  _In_ crude_physics_shapes_manager                       *manager,
  _In_ crude_physics_mesh_shape_handle                     handle
)
{
  return CRUDE_CAST( crude_physics_mesh_shape_container*, crude_resource_pool_access_resource( &manager->mesh_shape_resource_pool, handle.index ) );
}

void
crude_physics_shapes_manager_clear
(
  _In_ crude_physics_shapes_manager                       *manager
)
{
  for ( uint32 i = 0; i < CRUDE_HASHMAPSTR_CAPACITY( manager->mesh_shape_relative_filepath_to_hadle ); ++i )
  {
    if ( crude_hashmapstr_backet_key_hash_valid( manager->mesh_shape_relative_filepath_to_hadle[ i ].key.key_hash ) )
    {
      crude_physics_mesh_shape_container *mesh_shape_container = CRUDE_CAST( crude_physics_mesh_shape_container*, crude_resource_pool_access_resource( &manager->mesh_shape_resource_pool, manager->mesh_shape_relative_filepath_to_hadle[ i ].value.index ) );
      mesh_shape_container->jph_shape_class.~Ref( );
      
#if CRUDE_DEVELOP
      crude_gfx_model_renderer_resources_instance_deinitialize( &mesh_shape_container->debug_model_renderer_resource_instance );
#endif
      crude_resource_pool_release_resource( &manager->mesh_shape_resource_pool, manager->mesh_shape_relative_filepath_to_hadle[ i ].value.index );
    }
    manager->mesh_shape_relative_filepath_to_hadle[ i ].key.key_hash = CRUDE_HASHMAPSTR_BACKET_STATE_EMPTY;
  }

  crude_string_buffer_clear( &manager->gltf_absolute_filepath_string_buffer );
}

cgltf_data*
crude_physics_shapes_manager_gltf_parse_
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

void
crude_physics_shapes_manager_gltf_load_nodes_
(
  _In_ crude_physics_shapes_manager                       *manager,
  _In_ cgltf_data                                         *gltf,
  _In_ cgltf_node                                        **gltf_nodes,
  _In_ uint32                                              gltf_nodes_count,
  _Out_ JPH::Array< JPH::Triangle >                       *jph_triangles,
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
        CRUDE_ASSERT( mesh_primitive->indices->component_type == cgltf_component_type_r_16u );

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
          JPH::Triangle                                   jph_triangle;
          XMVECTOR                                        t0, t1, t2;
          
          t0 = XMVector4Transform( XMVectorSetW( XMLoadFloat3( &positions[ indices[ i ] ] ), 1 ), node_to_world );
          t1 = XMVector4Transform( XMVectorSetW( XMLoadFloat3( &positions[ indices[ i + 1 ] ] ), 1 ), node_to_world );
          t2 = XMVector4Transform( XMVectorSetW( XMLoadFloat3( &positions[ indices[ i + 2 ] ] ), 1 ), node_to_world );
          
          jph_triangle = CRUDE_COMPOUNT( JPH::Triangle, { crude_vector_to_jph_vec3( t0 ), crude_vector_to_jph_vec3( t1 ), crude_vector_to_jph_vec3( t2 ) } );
          jph_triangles->push_back(  jph_triangle );
        }
      }
    }

    crude_physics_shapes_manager_gltf_load_nodes_( manager, gltf, gltf_nodes[ i ]->children, gltf_nodes[ i ]->children_count, jph_triangles, node_to_parent );
  }
}

crude_physics_mesh_shape_handle
crude_physics_shapes_manager_load_mesh_shape_from_gltf_
(
  _In_ crude_physics_shapes_manager                       *manager,
  _In_ char const                                         *gltf_relative_filepath
)
{
  cgltf_data                                              *gltf;
  crude_physics_mesh_shape_container                      *mesh_shape;
  char                                                    *gltf_absolute_filepath;
  JPH::BodyInterface                                      *jph_body_interface_class;
  JPH::Array< JPH::Triangle >                              jph_triangles;
  JPH::MeshShapeSettings                                   jph_shape_settings_class;
  JPH::ShapeSettings::ShapeResult                          jph_shape_result_class;
  crude_physics_mesh_shape_handle                          mesh_shape_handle;
  
  gltf_absolute_filepath = crude_string_buffer_append_use_f( &manager->gltf_absolute_filepath_string_buffer, "%s%s", manager->resources_absolute_directory, gltf_relative_filepath );

  /* Parse gltf */
  gltf = crude_physics_shapes_manager_gltf_parse_( manager->cgltf_temporary_allocator, gltf_absolute_filepath );
  if ( !gltf )
  {
    goto cleanup;
  }
  
  mesh_shape_handle = CRUDE_COMPOUNT( crude_physics_mesh_shape_handle, { crude_resource_pool_obtain_resource( &manager->mesh_shape_resource_pool ) } );
  mesh_shape = CRUDE_CAST( crude_physics_mesh_shape_container*, crude_resource_pool_access_resource( &manager->mesh_shape_resource_pool, mesh_shape_handle.index ) );

  crude_string_copy( mesh_shape->relative_filepath, gltf_relative_filepath, sizeof( mesh_shape->relative_filepath ) );

  for ( uint32 i = 0; i < gltf->scenes_count; ++i )
  {
    crude_physics_shapes_manager_gltf_load_nodes_( manager, gltf, gltf->scene[ i ].nodes, gltf->scene[ i ].nodes_count, &jph_triangles, XMMatrixIdentity( ) );
  }
  
  jph_body_interface_class = &manager->physics_manager->jph_physics_system_class->GetBodyInterface( );
  
  jph_shape_settings_class = CRUDE_COMPOUNT( JPH::MeshShapeSettings, { jph_triangles } );
  jph_shape_settings_class.SetEmbedded( );
  
  jph_shape_result_class = jph_shape_settings_class.Create( );
  CRUDE_CXX_CONSTRUCTOR( &mesh_shape->jph_shape_class, JPH::Ref< JPH::Shape >, jph_shape_result_class.Get( ) );

cleanup:
  if ( gltf )
  {
    cgltf_free( gltf );
  }

  return mesh_shape_handle;
}