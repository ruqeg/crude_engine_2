#include <core/log.h>
#include <core/profiler.h>
#include <scene/scripts_components.h>
#include <scene/scene_components.h>
#include <platform/platform_components.h>
#include <player_controller_components.h>

#include <player_controller_system.h>

CRUDE_ECS_OBSERVER_DECLARE( crude_player_controller_creation_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_player_controller_update_system_ );

static void
crude_player_controller_creation_observer_
(
  _In_ ecs_iter_t *it
)
{
  crude_player_controller *player_controllers_per_entity = ecs_field( it, crude_player_controller, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_player_controller *player_controller = &player_controllers_per_entity[ i ];
    crude_entity entity = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
  }
}

static void
crude_player_controller_update_system_
(
  _In_ ecs_iter_t *it
)
{
  crude_transform *transforms_per_entity = ecs_field( it, crude_transform, 0 );
  crude_player_controller *player_controllere_per_entity = ecs_field( it, crude_player_controller, 1 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_transform                                       *transform, *pivot1_node_transform, *pivot2_node_transform;
    crude_player_controller                               *player_controller;
    crude_input const                                     *input;
    crude_entity                                           node, pivot1_node, pivot2_node;

    transform = &transforms_per_entity[ i ];
    player_controller = &player_controllere_per_entity[ i ];
    
    node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    if ( !player_controller->enabled )
    {
      continue;
    }

    input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( player_controller->entity_input, crude_input );
    
    pivot1_node = crude_ecs_lookup_entity_from_parent( node, "pivot1" );
    pivot2_node = crude_ecs_lookup_entity_from_parent( node, "pivot1.pivot2" );

    pivot1_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( pivot1_node, crude_transform );
    pivot2_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( pivot2_node, crude_transform );

    //XMQuaternionMultiply( );
    //XMStoreFloat4( &pivot1_node_transform->rotation, XMQuaternionMultiply( XMLoadFloat4( &pivot1_node_transform->rotation ), XMQuaternionRotationRollPitchYaw( ) ) );
    //XMStoreFloat4( &pivot2_node_transform->rotation, XMQuaternionMultiply( XMLoadFloat4( &pivot2_node_transform->rotation ), XMQuaternionRotationRollPitchYaw( ) ) );
    //pivot1_node_transform->rotation.y = crude_lerp_angle( pivot1_node_transform->rotation.y, player_controller->new_angle.y, player_controller->camera_following_speed * it->delta_time );
    //pivot2_node_transform->rotation.x = crude_lerp_angle( pivot1_node_transform->rotation.x, player_controller->new_angle.x, player_controller->camera_following_speed * it->delta_time );
    // 
    //int32 moving_forward = input->keys[ 'w' ].current - input->keys[ 's' ].current;
    //int32 moving_up = input->keys[ 'e' ].current - input->keys[ 'q' ].current;
    //int32 moving_right = input->keys[ 'd' ].current - input->keys[ 'a' ].current;
    //
    //XMVECTOR translation = XMLoadFloat3( &transforms[ i ].translation );
    //XMMATRIX node_to_world = crude_transform_node_to_world( CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } ), &transforms[ i ] );
    //
    //XMVECTOR basis_right = XMVector3Normalize( node_to_world.r[ 0 ] );
    //XMVECTOR basis_up = XMVector3Normalize( node_to_world.r[ 1 ] );
    //XMVECTOR basis_forward = XMVector3Normalize( node_to_world.r[ 2 ] );
    //
    //if ( moving_right )
    //{
    //  translation = XMVectorAdd( translation, XMVectorScale( basis_right, free_cameras[i].moving_speed_multiplier.x * moving_right * it->delta_time ) );
    //}
    //if ( moving_forward )
    //{
    //  translation = XMVectorAdd( translation, XMVectorScale( basis_forward, free_cameras[i].moving_speed_multiplier.z * moving_forward * it->delta_time ) );
    //}
    //if ( moving_up )
    //{
    //  translation = XMVectorAdd( translation, XMVectorScale( basis_up, free_cameras[i].moving_speed_multiplier.y * moving_up * it->delta_time ) );
    //}
    //XMStoreFloat3( &transforms[ i ].translation, translation );
    

    if ( input->mouse.right.current )
    {
      XMVECTOR                                             pivot1_rotation, pivot2_rotation;
		  
      pivot1_rotation = XMLoadFloat4( &pivot1_node_transform->rotation );
      pivot2_rotation = XMLoadFloat4( &pivot2_node_transform->rotation );

      pivot1_rotation = XMQuaternionMultiply( pivot1_rotation, XMQuaternionRotationAxis( g_XMIdentityR1, player_controller->rotation_speed * input->mouse.rel.x )  );
      pivot2_rotation = XMQuaternionMultiply( pivot2_rotation, XMQuaternionRotationAxis( g_XMIdentityR0, player_controller->rotation_speed * input->mouse.rel.y )  );

      //pivot2_rotation = XMQuaternionMultiply( pivot2_rotation, );

      XMStoreFloat4( &pivot1_node_transform->rotation, pivot1_rotation );
      XMStoreFloat4( &pivot2_node_transform->rotation, pivot2_rotation );

		 // rotating_angle.x = -input->mouse.rel.y * player_controller->rotation_speed;
     // rotating_angle.y = -input->mouse.rel.x * player_controller->rotation_speed;
     //
     // XMVECTOR rotation = XMLoadFloat4( &transforms[ i ].rotation );
     // XMVECTOR camera_up = XMVectorGetY( basis_up ) > 0.0f ? g_XMIdentityR1 : XMVectorNegate( g_XMIdentityR1 );
     //
     // rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( basis_right, -free_cameras[ i ].rotating_speed_multiplier.y * input->mouse.rel.y ) );
     // rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( camera_up, -free_cameras[ i ].rotating_speed_multiplier.x * input->mouse.rel.x ) );
     //
     //
     //
		 // player_controller->new_angle.x = CRUDE_CLAMP( player_controller->new_angle.x, CRUDE_DEG_TO_RAD( -46 ), CRUDE_DEG_TO_RAD( 46 ) );
    }
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_player_controller_system )
{
  ECS_MODULE( world, crude_player_controller_system );
  
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_player_controller_components );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_player_controller_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_player_controller ) },
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_player_controller_creation_observer_, EcsOnSet, { 
    { .id = ecs_id( crude_player_controller ) }
  } );
}