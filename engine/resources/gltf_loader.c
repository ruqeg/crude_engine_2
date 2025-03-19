#include <cgltf.h>
#include <stb_ds.h>

#include <core/assert.h>
#include <core/log.h>
#include <graphics/renderer.h>

#include <resources/gltf_loader.h>

static int32
crude_get_attribute_accessor_index
(
  _In_ cgltf_attribute *attributes,
  _In_ uint32           attributes_count,
  _In_ char const      *attribute_name
)
{
  for ( uint32 i = 0; i < attributes_count; ++i )
  {
    if ( strcmp( attributes[ i ].data, attribute_name ) == 0 )
    {
      return attributes[ i ].index;
    }
  }
  return -1;
}

static void
crude_get_mesh_vertex_buffer
(
  _In_ cgltf_data             *gltf,
  _In_ crude_scene            *scene,
  _In_ int32                   accessor_index,
  _Out_ crude_buffer_handle   *buffer_handle,
  _Out_ uint32                *buffer_offset
)
{
  if ( accessor_index == -1 )
  {
    return;
  }

  cgltf_accessor *buffer_accessor = &gltf->accessors[ accessor_index ];
  cgltf_buffer_view *buffer_view = buffer_accessor->buffer_view;
  crude_buffer_resource *buffer_gpu = &scene->buffers[ cgltf_buffer_view_index( gltf, buffer_accessor->buffer_view ) ];
  
  *buffer_handle = buffer_gpu->handle;
  *buffer_offset = buffer_accessor->offset;
}

void
crude_load_gltf_from_file
(
  _In_ crude_renderer  *renderer,
  _In_ char const      *path,
  _Out_ crude_scene    *scene
)
{
  cgltf_options gltf_options = { 0 };
  cgltf_data *gltf = NULL;
  cgltf_result result = cgltf_parse_file( &gltf_options, path, &gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to parse gltf file: %s", path );
  }
  
  result = cgltf_load_buffers( &gltf_options, gltf, path );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to load buffers from gltf file: %s", path );
  }
  
  result = cgltf_validate( gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to validate gltf file: %s", path );
  }
  
  scene->buffers = NULL;
  arrsetcap( scene->buffers, gltf->buffer_views_count );

  for ( uint32 buffer_view_index = 0; buffer_view_index < gltf->buffer_views_count; ++buffer_view_index )
  {
    cgltf_buffer_view *buffer_view = &gltf->buffer_views[ buffer_view_index ];
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
  arrsetcap( scene->mesh_draws, gltf->meshes_count );

  cgltf_scene *root_scene = gltf->scene;

  for ( uint32 node_index = 0; node_index < root_scene->nodes_count; ++node_index )
  {
    cgltf_node *node = root_scene->nodes[ node_index ];
  
    if ( !node->mesh )
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
      
      int32 position_accessor_index = crude_get_attribute_accessor_index( mesh_primitive->attributes, mesh_primitive->attributes_count, "POSITION" );
      int32 tangent_accessor_index = crude_get_attribute_accessor_index( mesh_primitive->attributes, mesh_primitive->attributes_count, "TANGENT" );
      int32 normal_accessor_index = crude_get_attribute_accessor_index( mesh_primitive->attributes, mesh_primitive->attributes_count, "NORMAL" );
      int32 texcoord_accessor_index = crude_get_attribute_accessor_index( mesh_primitive->attributes, mesh_primitive->attributes_count, "TEXCOORD_0" );
      
      crude_get_mesh_vertex_buffer( gltf, scene, position_accessor_index, &mesh_draw.position_buffer, &mesh_draw.position_offset );
      crude_get_mesh_vertex_buffer( gltf, scene, tangent_accessor_index, &mesh_draw.tangent_buffer, &mesh_draw.tangent_offset );
      crude_get_mesh_vertex_buffer( gltf, scene, normal_accessor_index, &mesh_draw.normal_buffer, &mesh_draw.normal_offset );
      crude_get_mesh_vertex_buffer( gltf, scene, texcoord_accessor_index, &mesh_draw.texcoord_buffer, &mesh_draw.texcoord_offset );
      
      cgltf_accessor *indices_accessor = mesh_primitive->indices;
      cgltf_buffer_view *indices_buffer_view = indices_accessor->buffer_view;
      
      crude_buffer_resource *indices_buffer_gpu = &scene->buffers[ cgltf_buffer_view_index( gltf, indices_accessor->buffer_view ) ];
      mesh_draw.index_buffer = indices_buffer_gpu->handle;
      mesh_draw.index_offset = indices_accessor->offset;
      mesh_draw.primitive_count = indices_accessor->count;

      crude_descriptor_set_creation ds_creation = {
        .resources = { renderer->gpu->ubo_buffer.index },
        .bindings = { 0u },
        .layout = renderer->gpu->descriptor_set_layout_handle,
        .num_resources = 1,
        .name = "ds_1"
      };
      mesh_draw.descriptor_set = crude_create_descriptor_set( renderer->gpu, &ds_creation );
      arrpush( scene->mesh_draws, mesh_draw );
    }
  }

  cgltf_free( gltf );
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