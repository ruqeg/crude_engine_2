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

CRUDE_COMPONENT_STRING_DEFINE( crude_physics_character, "crude_physics_character" );
CRUDE_COMPONENT_STRING_DEFINE( crude_physics_character_handle, "crude_physics_character_handle" );

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
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_physics_character );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_physics_character );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_physics_character );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_physics_character_handle );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_physics_character )
{
  component->character_height_standing = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "character_height_standing" ) );
  component->character_radius_standing = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "character_radius_standing" ) );
  component->friction = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "friction" ) );
  component->max_slop_angle = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "max_slop_angle" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_physics_character )
{
  cJSON *static_body_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( static_body_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_physics_character ) ) );
  cJSON_AddItemToObject( static_body_json, "character_height_standing", cJSON_CreateNumber( component->character_height_standing ) );
  cJSON_AddItemToObject( static_body_json, "character_radius_standing", cJSON_CreateNumber( component->character_radius_standing ) );
  cJSON_AddItemToObject( static_body_json, "friction", cJSON_CreateNumber( component->friction ) );
  cJSON_AddItemToObject( static_body_json, "max_slop_angle", cJSON_CreateNumber( component->max_slop_angle ) );
  return static_body_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_physics_character )
{
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

/**********************************************************
 *
 *                 Systems
 *
 *********************************************************/
CRUDE_ECS_SYSTEM_DECLARE( crude_physics_system );

CRUDE_ECS_SYSTEM_DECLARE( crude_physics_character_pre_simulation_system_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_physics_character_post_simulation_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_character_destroy_observer_  );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_character_create_observer_  );

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
crude_physics_character_pre_simulation_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
crude_physics_character_post_simulation_system_
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

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_physics_character_pre_simulation_system_, crude_ecs_on_pre_physics_update, ctx, { 
    { .id = ecs_id( crude_physics_character_handle ) },
    { .id = ecs_id( crude_transform ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_physics_character_post_simulation_system_, crude_ecs_on_post_physics_update, ctx, { 
    { .id = ecs_id( crude_physics_character_handle ) },
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
    
    character = &character_per_entity[ i ];

    if ( CRUDE_ENTITY_HAS_COMPONENT( it->world, it->entities[ i ], crude_physics_character_handle ) )
    {
      // !TODO
    }
    else
    {
      crude_physics_character_creation                     character_creation;
      
      character_creation = crude_physics_character_creation_empty( );
      character_creation.character_height_standing = character->character_height_standing;
      character_creation.character_radius_standing = character->character_radius_standing;
      character_creation.friction = character->friction;
      character_creation.max_slop_angle = character->max_slop_angle;

      character_handle = crude_physics_create_character( ctx->physics, &character_creation );
      CRUDE_ENTITY_SET_COMPONENT( it->world, it->entities[ i ], crude_physics_character_handle, { character_handle } );
    }
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
    
    character_container = crude_physics_access_character( ctx->physics, character_handle[ i ] );

    node_to_world = crude_transform_node_to_world( it->world, it->entities[ i ], transform );

    XMMatrixDecompose( &scale, &rotation, &translation, node_to_world );

    character_container->jph_character_class->SetPositionAndRotation( crude_vector_to_jph_vec3( translation ), crude_vector_to_jph_quat( rotation ) );
    character_container->manually_stored_transform = character_container->jph_character_class->GetWorldTransform( );
  }
cleanup:
  CRUDE_PROFILER_ZONE_END;
}