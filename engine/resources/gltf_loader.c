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
  cgltf_data *gltf_data = NULL;
  cgltf_result result = cgltf_parse_file( &gltf_options, path, &gltf_data );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to parse gltf file: %s", path );
  }
  
  result = cgltf_load_buffers( &gltf_options, gltf_data, path );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to load buffers from gltf file: %s", path );
  }
  
  result = cgltf_validate( gltf_data );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to validate gltf file: %s", path );
  }
  
  scene->buffers = NULL;
  arrsetlen( scene->buffers, gltf_data->buffer_views_count );

  for ( uint32 buffer_view_index = 0; buffer_view_index < gltf_data->buffer_views_count; ++buffer_view_index )
  {
    cgltf_buffer_view *buffer_view = &gltf_data->buffer_views[ buffer_view_index ];
    cgltf_buffer *buffer = buffer_view->buffer;
  
    uint8* data = ( uint8* )buffer->data + buffer_view->offset;
  
    if ( buffer_view->name == NULL )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_FILEIO, "Bufer name is null: %u", buffer_view_index );
    }

    crude_buffer_creation buffer_creation = {
      .initial_data = data,
      .usage = CRUDE_RESOURCE_USAGE_TYPE_IMMUTABLE,
      .size = buffer_view->size,
      .type_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      .name = buffer->name
    };
    crude_buffer_resource *buffer_resource = crude_gfx_renderer_create_buffer( renderer, &buffer_creation );
    arrpush( scene->buffers, *buffer_resource );
  }

  scene->mesh_draws = NULL;
  arrsetlen( scene->mesh_draws, gltf_data->meshes_count );

  cgltf_scene *root_gltf_scene = gltf_data->scene;

  for ( uint32 node_index = 0; node_index < root_gltf_scene->nodes_count; ++node_index )
  {
    cgltf_node *node = root_gltf_scene->nodes[ node_index ];
  
    if ( node->mesh )
    {
      continue;
    }
  
    crude_float3 node_scale = { 1.0f, 1.0f, 1.0f };
    if ( node->has_scale )
    {
      node_scale = ( crude_float3 ){ node->scale[0], node->scale[1], node->scale[2] };
    }
  
    for ( uint32 primitive_index = 0; primitive_index < node->mesh->primitives_count; ++primitive_index )
    {
        crude_mesh_draw mesh_draw = { .scale = node_scale };
    
        cgltf_primitive *mesh_primitive = &node->mesh->primitives[ primitive_index ];
    
        int32 const position_accessor_index = gltf_get_attribute_accessor_index( mesh_primitive->attributes, mesh_primitive->attributes_count, "POSITION" );
        int32 const tangent_accessor_index = gltf_get_attribute_accessor_index( mesh_primitive->attributes, mesh_primitive->attributes_count, "TANGENT" );
        int32 const normal_accessor_index = gltf_get_attribute_accessor_index( mesh_primitive->attributes, mesh_primitive->attributes_count, "NORMAL" );
        int32 const texcoord_accessor_index = gltf_get_attribute_accessor_index( mesh_primitive->attributes, mesh_primitive->attributes_count, "TEXCOORD_0" );
    
        get_mesh_vertex_buffer( scene, position_accessor_index, mesh_draw.position_buffer, mesh_draw.position_offset );
        get_mesh_vertex_buffer( scene, tangent_accessor_index, mesh_draw.tangent_buffer, mesh_draw.tangent_offset );
        get_mesh_vertex_buffer( scene, normal_accessor_index, mesh_draw.normal_buffer, mesh_draw.normal_offset );
        get_mesh_vertex_buffer( scene, texcoord_accessor_index, mesh_draw.texcoord_buffer, mesh_draw.texcoord_offset );
    
        // Create index buffer
        glTF::Accessor& indices_accessor = scene.gltf_scene.accessors[ mesh_primitive.indices ];
        glTF::BufferView& indices_buffer_view = scene.gltf_scene.buffer_views[ indices_accessor.buffer_view ];
        BufferResource& indices_buffer_gpu = scene.buffers[ indices_accessor.buffer_view ];
        mesh_draw.index_buffer = indices_buffer_gpu.handle;
        mesh_draw.index_offset = indices_accessor.byte_offset == glTF::INVALID_INT_VALUE ? 0 : indices_accessor.byte_offset;
        mesh_draw.primitive_count = indices_accessor.count;
    
        // Create material
        glTF::Material& material = scene.gltf_scene.materials[ mesh_primitive.material ];
    
        bool transparent = get_mesh_material( renderer, scene, material, mesh_draw );
    
        if ( transparent) {
            if ( material.double_sided ) {
                mesh_draw.material = material_no_cull_transparent;
            } else {
                mesh_draw.material = material_cull_transparent;
            }
        } else {
            if ( material.double_sided ) {
                mesh_draw.material = material_no_cull_opaque;
            } else {
                mesh_draw.material = material_cull_opaque;
            }
        }
    
        scene.mesh_draws.push( mesh_draw );
    }
  }
    }

    qsort( scene.mesh_draws.data, scene.mesh_draws.size, sizeof( MeshDraw ), mesh_material_compare );

  cgltf_free( gltf_scene );
}

void
crude_unload_gltf_from_file
(
  _In_ crude_renderer  *renderer,
  _In_ crude_scene     *scene
)
{
  arrfree( scene->mesh_draws );
  arrfree( scene->buffers );
}