#include <engine/core/string.h>
#include <engine/core/memory.h>
#include <engine/core/assert.h>
#include <engine/physics/physics.h>
#include <engine/graphics/imgui.h>
#include <engine/scene/node_manager.h>
#include <engine/core/profiler.h>
#include <engine/scene/scene_ecs.h>

#include <engine/physics/physics_ecs.h>

/**********************************************************
 *
 *                 Components
 *
 *********************************************************/
ECS_COMPONENT_DECLARE( crude_physics_character );
ECS_COMPONENT_DECLARE( crude_physics_character_handle );
ECS_COMPONENT_DECLARE( crude_physics_static_body );
ECS_COMPONENT_DECLARE( crude_physics_static_body_handle );

CRUDE_COMPONENT_STRING_DEFINE( crude_physics_character, "crude_physics_character" );
CRUDE_COMPONENT_STRING_DEFINE( crude_physics_character_handle, "crude_physics_character_handle" );

CRUDE_COMPONENT_STRING_DEFINE( crude_physics_static_body, "crude_physics_static_body" );
CRUDE_COMPONENT_STRING_DEFINE( crude_physics_static_body_handle, "crude_physics_static_body_handle" );

void
crude_physics_components_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager
)
{
  CRUDE_ECS_MODULE( world, crude_physics_components );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_physics_character );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_physics_character_handle );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_physics_static_body );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_physics_static_body_handle );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_physics_character );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_physics_character );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_physics_character );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_physics_character_handle );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_physics_static_body );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_physics_static_body );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_physics_static_body );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_physics_static_body_handle );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_physics_character )
{
  component->height = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "height" ) );
  component->radius = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "radius" ) );
  component->friction = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "friction" ) );
  component->max_slop_angle = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "max_slop_angle" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_physics_character )
{
  cJSON *static_body_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( static_body_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_physics_character ) ) );
  cJSON_AddItemToObject( static_body_json, "height", cJSON_CreateNumber( component->height ) );
  cJSON_AddItemToObject( static_body_json, "radius", cJSON_CreateNumber( component->radius ) );
  cJSON_AddItemToObject( static_body_json, "friction", cJSON_CreateNumber( component->friction ) );
  cJSON_AddItemToObject( static_body_json, "max_slop_angle", cJSON_CreateNumber( component->max_slop_angle ) );
  return static_body_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_physics_character )
{
  bool                                                     modified;

  CRUDE_IMGUI_START_OPTIONS;
  
  modified = false;

  CRUDE_IMGUI_OPTION( "Character Height", {
    modified |= ImGui::DragFloat( "##Character Height", &component->height, 0.1f );  
    } );

  CRUDE_IMGUI_OPTION( "Character Radius", {
    modified |= ImGui::DragFloat( "##Character Radius", &component->radius, 0.1f );  
    } );

  if ( modified )
  {
    CRUDE_ENTITY_SET_COMPONENT( world, node, crude_physics_character, { *component } );
  }
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_physics_character_handle )
{
  crude_physics_character_container                       *character_container;
  JPH::RMat44                                              jph_wolrd_transform;
  JPH::Vec3                                                jph_velocity;

  character_container = crude_physics_access_character( manager->physics_manager, *component );
  
  jph_wolrd_transform = character_container->jph_character_class->GetWorldTransform( );
  jph_velocity = character_container->jph_character_class->GetLinearVelocity( );

  ImGui::LabelText( "World Transform", "%f %f %f", jph_wolrd_transform.GetTranslation( ).GetX( ), jph_wolrd_transform.GetTranslation( ).GetY( ), jph_wolrd_transform.GetTranslation( ).GetZ( ) );
  ImGui::LabelText( "World Rotation", "%f %f %f %f", jph_wolrd_transform.GetQuaternion( ).GetX( ), jph_wolrd_transform.GetQuaternion( ).GetY( ), jph_wolrd_transform.GetQuaternion( ).GetZ( ), jph_wolrd_transform.GetQuaternion( ).GetW( ) );
  ImGui::LabelText( "Velocity", "%f %f %f", jph_velocity.GetX( ), jph_velocity.GetY( ), jph_velocity.GetZ( ) );
}


CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_physics_static_body )
{
  component->type = CRUDE_CAST( crude_physics_static_body_shape_type, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "shape_type" ) ) );
  if ( component->type == CRUDE_PHYSICS_STATIC_BODY_SHAPE_TYPE_BOX )
  {
    cJSON *box_json = cJSON_GetObjectItem( component_json, "box" );
    crude_parse_json_to_float3( &component->box.extent, cJSON_GetObjectItemCaseSensitive( box_json, "extent" ) );
  }
  else if ( component->type == CRUDE_PHYSICS_STATIC_BODY_SHAPE_TYPE_MESH )
  {
    cJSON *mesh_json = cJSON_GetObjectItem( component_json, "mesh" );
    component->mesh.handle = crude_physics_shapes_manager_get_mesh_shape_handle( manager->physics_manager->physics_shapes_manager, cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( mesh_json, "relative_filepath" ) ) );
  }
  component->layers = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "layers" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_physics_static_body )
{
  cJSON *static_body_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( static_body_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_physics_static_body ) ) );
  cJSON_AddItemToObject( static_body_json, "shape_type",cJSON_CreateNumber( component->type ) );
  if ( component->type == CRUDE_PHYSICS_STATIC_BODY_SHAPE_TYPE_BOX )
  {
    cJSON *box_json = cJSON_CreateObject( );
    cJSON_AddItemToObject( box_json, "extent", cJSON_CreateFloatArray( &component->box.extent.x, 3 ) );
    cJSON_AddItemToObject( static_body_json, "box", box_json );
  }
  else if ( component->type == CRUDE_PHYSICS_STATIC_BODY_SHAPE_TYPE_MESH )
  {
    cJSON *mesh_json = cJSON_CreateObject( );
    crude_physics_mesh_shape_container *mesh_shape = crude_physics_shapes_manager_access_mesh_shape( manager->physics_manager->physics_shapes_manager, component->mesh.handle );
    cJSON_AddItemToObject( mesh_json, "relative_filepath", cJSON_CreateString( mesh_shape->relative_filepath ) );
    cJSON_AddItemToObject( static_body_json, "mesh", mesh_json );
  }
  cJSON_AddItemToObject( static_body_json, "layers", cJSON_CreateNumber( component->layers ) );
  return static_body_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_physics_static_body )
{
  bool                                                     modified;

  CRUDE_IMGUI_START_OPTIONS;
  
  char const* physic_static_body_types[ ] =
  {
    "Box",
    "Mesh"
  };

  CRUDE_ASSERT( CRUDE_COUNTOF( physic_static_body_types ) == CRUDE_PHYSICS_STATIC_BODY_SHAPE_TYPE_COUNT );

  modified = false;

  CRUDE_IMGUI_OPTION( "Type", {
    int32 type = component->type;
    if ( ImGui::Combo( "##Shape", &type, physic_static_body_types, CRUDE_COUNTOF( physic_static_body_types ) ) )
    {
      component->type = CRUDE_CAST( crude_physics_static_body_shape_type, type );
      if ( component->type == CRUDE_PHYSICS_STATIC_BODY_SHAPE_TYPE_MESH )
      {
        component->mesh.handle.index = -1;
      }
      modified = true;
    }
    } );

  if ( component->type == CRUDE_PHYSICS_STATIC_BODY_SHAPE_TYPE_BOX )
  {
    CRUDE_IMGUI_OPTION( "Box Extent", {
      modified |= ImGui::DragFloat3( "##Box Extent", &component->box.extent.x, 0.1f, 0.f, 0.f, "%.3f", ImGuiSliderFlags_ColorMarkers );  
      } );
  }
  else if ( component->type == CRUDE_PHYSICS_STATIC_BODY_SHAPE_TYPE_MESH )
  {
    if ( component->mesh.handle.index != -1 )
    {
      crude_physics_mesh_shape_container *shape_container = crude_physics_shapes_manager_access_mesh_shape( manager->physics_manager->physics_shapes_manager, component->mesh.handle );
      ImGui::Text( "\"%s\"", shape_container->relative_filepath );
    }
    else
    {
      ImGui::Text( "\"Empty\"" );
    }
    if ( ImGui::BeginDragDropTarget( ) )
    {
      ImGuiPayload const                                  *im_payload;
      char                                                *replace_relative_filepath;
    
      im_payload = ImGui::AcceptDragDropPayload( "crude_content_browser_file" );
      if ( im_payload )
      {
        replace_relative_filepath = CRUDE_CAST( char*, im_payload->Data );
        if ( strstr( replace_relative_filepath, ".gltf" ) )
        {
          component->mesh.handle = crude_physics_shapes_manager_get_mesh_shape_handle( manager->physics_manager->physics_shapes_manager, replace_relative_filepath );
          modified = true;
        }
      }
      ImGui::EndDragDropTarget();
    }
  }

  CRUDE_IMGUI_OPTION( "Layer", {
    modified |= ImGui::CheckboxFlags( "s", &component->layers, g_crude_jph_layer_non_moving );
    ImGui::SameLine( );
    modified |= ImGui::CheckboxFlags( "m", &component->layers, g_crude_jph_layer_moving );
    ImGui::SameLine( );
    modified |= ImGui::CheckboxFlags( "0", &component->layers, g_crude_jph_layer_custom0 );
    ImGui::SameLine( );
    modified |= ImGui::CheckboxFlags( "1", &component->layers, g_crude_jph_layer_custom1 );
    ImGui::SameLine( );
    modified |= ImGui::CheckboxFlags( "2", &component->layers, g_crude_jph_layer_custom2 );
    ImGui::SameLine( );
    modified |= ImGui::CheckboxFlags( "3", &component->layers, g_crude_jph_layer_custom3 );
    ImGui::SameLine( );
    modified |= ImGui::CheckboxFlags( "4", &component->layers, g_crude_jph_layer_custom4 );
    } );

  if ( modified )
  {
    CRUDE_ENTITY_SET_COMPONENT( world, node, crude_physics_static_body, { *component } );
  }
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_physics_static_body_handle )
{
  crude_physics_static_body_container                     *static_body_container;
  JPH::BodyInterface                                      *jph_body_interface_class;
  JPH::RMat44                                              jph_wolrd_transform;

  static_body_container = crude_physics_access_static_body( manager->physics_manager, *component );

  jph_body_interface_class = &manager->physics_manager->jph_physics_system_class->GetBodyInterface( );  
  jph_wolrd_transform = jph_body_interface_class->GetWorldTransform( static_body_container->jph_body_class );

  ImGui::LabelText( "World Transform", "%f %f %f", jph_wolrd_transform.GetTranslation( ).GetX( ), jph_wolrd_transform.GetTranslation( ).GetY( ), jph_wolrd_transform.GetTranslation( ).GetZ( ) );
  ImGui::LabelText( "World Rotation", "%f %f %f %f", jph_wolrd_transform.GetQuaternion( ).GetX( ), jph_wolrd_transform.GetQuaternion( ).GetY( ), jph_wolrd_transform.GetQuaternion( ).GetZ( ), jph_wolrd_transform.GetQuaternion( ).GetW( ) );
}

/**********************************************************
 *
 *                 Systems
 *
 *********************************************************/
CRUDE_ECS_SYSTEM_DECLARE( crude_physics_system );

CRUDE_ECS_SYSTEM_DECLARE( crude_physics_character_pre_simulation_system_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_physics_character_post_simulation_system_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_physics_static_body_post_simulation_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_character_destroy_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_character_create_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_static_body_destroy_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_static_body_create_observer_ );

static void
crude_physics_character_create_observer_
(
  _In_ ecs_iter_t                                         *it
);

static void
crude_physics_character_destroy_observer_ 
(
  _In_ ecs_iter_t                                         *it
);

static void
crude_physics_static_body_create_observer_
(
  _In_ ecs_iter_t                                         *it
);

static void
crude_physics_static_body_destroy_observer_ 
(
  _In_ ecs_iter_t                                         *it
);

static void
crude_physics_character_pre_simulation_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
crude_physics_character_post_simulation_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
crude_physics_static_body_post_simulation_system_
(
  _In_ ecs_iter_t                                         *it
);

void
crude_physics_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_physics_system_context                       *ctx
)
{
  crude_physics_components_import( world, manager );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_character_create_observer_, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_physics_character ), .oper = EcsAnd }
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_character_destroy_observer_, EcsOnRemove, ctx, { 
    { .id = ecs_id( crude_physics_character ), .oper = EcsAnd }
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_static_body_create_observer_, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_physics_static_body ), .oper = EcsAnd }
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_static_body_destroy_observer_, EcsOnRemove, ctx, { 
    { .id = ecs_id( crude_physics_static_body ), .oper = EcsAnd }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_physics_character_pre_simulation_system_, crude_ecs_on_pre_physics_update, ctx, { 
    { .id = ecs_id( crude_physics_character_handle ) },
    { .id = ecs_id( crude_transform ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_physics_character_post_simulation_system_, crude_ecs_on_post_physics_update, ctx, { 
    { .id = ecs_id( crude_physics_character_handle ) },
    { .id = ecs_id( crude_transform ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_physics_static_body_post_simulation_system_, crude_ecs_on_post_physics_update, ctx, { 
    { .id = ecs_id( crude_physics_static_body_handle ) },
    { .id = ecs_id( crude_transform ) }
  } );
}

void
crude_physics_character_create_observer_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_physics_system_context                            *ctx;
  crude_physics_character                                 *character_per_entity;

  CRUDE_PROFILER_ZONE_NAME( "crude_physics_character_create_observer_" );

  ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  character_per_entity = ecs_field( it, crude_physics_character, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_character                               *character;
    crude_physics_character_handle                         character_handle;
    crude_physics_character_creation                       character_creation;
    
    character = &character_per_entity[ i ];

    if ( CRUDE_ENTITY_HAS_COMPONENT( it->world, it->entities[ i ], crude_physics_character_handle ) )
    {
      crude_physics_destroy_character_instant( ctx->physics, *CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( it->world, it->entities[ i ], crude_physics_character_handle ) );
    }
    
    character_creation = crude_physics_character_creation_empty( );
    character_creation.character_height_standing = character->height;
    character_creation.character_radius_standing = character->radius;
    character_creation.friction = character->friction;
    character_creation.max_slop_angle = character->max_slop_angle;

    character_handle = crude_physics_create_character( ctx->physics, &character_creation );
    CRUDE_ENTITY_SET_COMPONENT( it->world, it->entities[ i ], crude_physics_character_handle, { character_handle } );
  }
}

void
crude_physics_character_destroy_observer_ 
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_physics_system_context                            *ctx;
  crude_physics_character                                 *character_per_entity;

  CRUDE_PROFILER_ZONE_NAME( "crude_physics_character_destroy_observer_" );

  ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  character_per_entity = ecs_field( it, crude_physics_character, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_character                               *character;
    crude_physics_character_handle const                  *character_handle;
    
    character = &character_per_entity[ i ];

    character_handle = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( it->world, it->entities[ i ], crude_physics_character_handle );

    if ( character_handle )
    {
      crude_physics_destroy_character_instant( ctx->physics, *character_handle );
    }
  }
  
  CRUDE_PROFILER_ZONE_END;
}

void
crude_physics_static_body_create_observer_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_physics_system_context                            *ctx;
  crude_physics_static_body                               *static_body_per_entity;

  CRUDE_PROFILER_ZONE_NAME( "crude_physics_static_body_create_observer_" );

  ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  static_body_per_entity = ecs_field( it, crude_physics_static_body, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_static_body                             *static_body;
    crude_physics_static_body_handle                       static_body_handle;
    crude_physics_static_body_creation                     static_body_creation;
    
    static_body = &static_body_per_entity[ i ];
    
    if ( CRUDE_ENTITY_HAS_COMPONENT( it->world, it->entities[ i ], crude_physics_static_body_handle ) )
    {
      crude_physics_destroy_static_body_instant( ctx->physics, *CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( it->world, it->entities[ i ], crude_physics_static_body_handle ) );
    }
      
    static_body_creation = crude_physics_static_body_creation_empty( );
    static_body_creation.type = static_body->type;
    static_body_creation.box.extent = static_body->box.extent;
    static_body_creation.mesh.handle = static_body->mesh.handle;
    static_body_creation.entity = it->entities[ i ];
    static_body_creation.layers = static_body->layers;

    if ( static_body_creation.type == CRUDE_PHYSICS_STATIC_BODY_SHAPE_TYPE_MESH && static_body_creation.mesh.handle.index == -1 )
    {
      CRUDE_ENTITY_REMOVE_COMPONENT( it->world, it->entities[ i ], crude_physics_static_body_handle );
      continue;
    }
    
    static_body_handle = crude_physics_create_static_body( ctx->physics, &static_body_creation );
    CRUDE_ENTITY_SET_COMPONENT( it->world, it->entities[ i ], crude_physics_static_body_handle, { static_body_handle } );
  }
}

void
crude_physics_static_body_destroy_observer_ 
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_physics_system_context                            *ctx;
  crude_physics_static_body                               *static_body_per_entity;

  CRUDE_PROFILER_ZONE_NAME( "crude_physics_static_body_destroy_observer_" );

  ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  static_body_per_entity = ecs_field( it, crude_physics_static_body, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_static_body                             *static_body;
    crude_physics_static_body_handle const                *static_body_handle;
    
    static_body = &static_body_per_entity[ i ];

    static_body_handle = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( it->world, it->entities[ i ], crude_physics_static_body_handle );

    if ( static_body_handle )
    {
      crude_physics_destroy_static_body_instant( ctx->physics, *static_body_handle );
    }
  }
  
  CRUDE_PROFILER_ZONE_END;
}

void
crude_physics_character_pre_simulation_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_physics_system_context                            *ctx;
  crude_physics_character_handle                          *character_handle_per_entity;
  crude_transform                                         *transform_per_entity;

  CRUDE_PROFILER_ZONE_NAME( "crude_physics_character_pre_simulation_system_" );

  ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  character_handle_per_entity = ecs_field( it, crude_physics_character_handle, 0 );
  transform_per_entity = ecs_field( it, crude_transform, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_character_handle                        *character_handle;
    crude_transform                                       *transform;
    crude_physics_character_container                     *character_container;
    XMVECTOR                                               rotation_diff;
    XMVECTOR                                               translation_diff;
    JPH::RMat44                                            jph_world_transform;
    
    character_handle = &character_handle_per_entity[ i ];
    transform = &transform_per_entity[ i ]; 
    
    character_container = crude_physics_access_character( ctx->physics, character_handle[ i ] );
    character_container->jph_character_class->PostSimulation( 0.05f );

    rotation_diff = XMQuaternionMultiply(
      crude_jph_quat_to_vector( character_container->jph_character_class->GetRotation( ) ),
      XMQuaternionInverse(
        crude_jph_quat_to_vector( character_container->manually_stored_transform.GetQuaternion( ) )
      ) );

    translation_diff = XMVectorSubtract(
      crude_jph_vec3_to_vector( character_container->jph_character_class->GetPosition( ) ),
      crude_jph_vec3_to_vector( character_container->manually_stored_transform.GetTranslation( ) ) );

    XMStoreFloat3( &transform->translation, XMVectorAdd( XMLoadFloat3( &transform->translation ), translation_diff ) );
    XMStoreFloat4( &transform->rotation, XMQuaternionMultiply( XMLoadFloat4( &transform->rotation ), rotation_diff ) );
  }
cleanup:
  CRUDE_PROFILER_ZONE_END;
}

void
crude_physics_character_post_simulation_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_physics_system_context                            *ctx;
  crude_physics_character_handle                          *character_handle_per_entity;
  crude_transform                                         *transform_per_entity;

  CRUDE_PROFILER_ZONE_NAME( "crude_physics_character_post_simulation_system_" );

  ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  character_handle_per_entity = ecs_field( it, crude_physics_character_handle, 0 );
  transform_per_entity = ecs_field( it, crude_transform, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_character_handle                        *character_handle;
    crude_transform                                       *transform;
    crude_physics_character_container                     *character_container;
    XMMATRIX                                               node_to_world;
    XMVECTOR                                               scale, translation, rotation;
    
    character_handle = &character_handle_per_entity[ i ];
    transform = &transform_per_entity[ i ]; 
    
    character_container = crude_physics_access_character( ctx->physics,*character_handle );

    node_to_world = crude_transform_node_to_world( it->world, it->entities[ i ], transform );

    XMMatrixDecompose( &scale, &rotation, &translation, node_to_world );

    character_container->jph_character_class->SetPositionAndRotation( crude_vector_to_jph_vec3( translation ), crude_vector_to_jph_quat( rotation ) );
    character_container->manually_stored_transform = character_container->jph_character_class->GetWorldTransform( );
  }
cleanup:
  CRUDE_PROFILER_ZONE_END;
}

void
crude_physics_static_body_post_simulation_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_physics_system_context                            *ctx;
  crude_physics_static_body_handle                        *static_body_handle_per_entity;
  crude_transform                                         *transform_per_entity;
  JPH::BodyInterface                                      *jph_body_interface_class;

  CRUDE_PROFILER_ZONE_NAME( "crude_physics_static_body_post_simulation_system_" );

  ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  static_body_handle_per_entity = ecs_field( it, crude_physics_static_body_handle, 0 );
  transform_per_entity = ecs_field( it, crude_transform, 1 );

  jph_body_interface_class = &ctx->physics->jph_physics_system_class->GetBodyInterface( );  

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_static_body_handle                      *static_body_handle;
    crude_transform                                       *transform;
    crude_physics_static_body_container                   *static_body_container;
    XMMATRIX                                               node_to_world;
    XMVECTOR                                               scale, translation, rotation;
    
    static_body_handle = &static_body_handle_per_entity[ i ];
    transform = &transform_per_entity[ i ]; 
    
    static_body_container = crude_physics_access_static_body( ctx->physics, *static_body_handle );

    node_to_world = crude_transform_node_to_world( it->world, it->entities[ i ], transform );

    XMMatrixDecompose( &scale, &rotation, &translation, node_to_world );

    jph_body_interface_class->SetPositionAndRotation( static_body_container->jph_body_class, crude_vector_to_jph_vec3( translation ), crude_vector_to_jph_quat( rotation ), JPH::EActivation::DontActivate );
  }
cleanup:
  CRUDE_PROFILER_ZONE_END;
}