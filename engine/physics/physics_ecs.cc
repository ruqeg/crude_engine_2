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
ECS_COMPONENT_DECLARE( crude_physics_static_body_handle );
ECS_COMPONENT_DECLARE( crude_physics_character_body_handle );
ECS_COMPONENT_DECLARE( crude_physics_collision_shape );

CRUDE_COMPONENT_STRING_DEFINE( crude_physics_static_body_handle, "crude_physics_static_body_handle" );
CRUDE_COMPONENT_STRING_DEFINE( crude_physics_character_body_handle, "crude_physics_character_body_handle" );
CRUDE_COMPONENT_STRING_DEFINE( crude_physics_collision_shape, "crude_physics_collision_shape" );

void
crude_physics_components_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager
)
{
  CRUDE_ECS_MODULE( world, crude_physics_components );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_physics_static_body_handle );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_physics_character_body_handle );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_physics_collision_shape );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_physics_static_body_handle );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_physics_character_body_handle );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_physics_collision_shape );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_static_body_handle )
{
  crude_physics_static_body_handle *previous_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, node, crude_physics_static_body_handle );
  if ( previous_handle )
  {
    crude_physics_resources_manager_destroy_static_body( manager->physics_resources_manager, *previous_handle );
  }

  *component = crude_physics_resources_manager_create_static_body( manager->physics_resources_manager, node );
  crude_physics_static_body *static_body = crude_physics_resources_manager_access_static_body( manager->physics_resources_manager, *component );
  static_body->layer = cJSON_GetNumberValue( cJSON_GetObjectItem( component_json, "layer" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_static_body_handle )
{
  cJSON *static_body_json = cJSON_CreateObject( );

  crude_physics_static_body *static_body = crude_physics_resources_manager_access_static_body( manager->physics_resources_manager, *component );

  cJSON_AddItemToObject( static_body_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_physics_static_body_handle ) ) );
  cJSON_AddItemToObject( static_body_json, "layer", cJSON_CreateNumber( static_body->layer ) );
  return static_body_json;
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_character_body_handle )
{
  crude_physics_character_body_handle *previous_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( world, node, crude_physics_character_body_handle );
  if ( previous_handle )
  {
    crude_physics_resources_manager_destroy_character_body( manager->physics_resources_manager, *previous_handle );
  }
  *component = crude_physics_resources_manager_create_character_body( manager->physics_resources_manager, node );
  crude_physics_character_body *character_body = crude_physics_resources_manager_access_character_body( manager->physics_resources_manager, *component );
  character_body->mask = cJSON_GetNumberValue( cJSON_GetObjectItem( component_json, "mask" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_character_body_handle )
{
  cJSON *dynamic_body_json = cJSON_CreateObject( );
  crude_physics_character_body *character_body = crude_physics_resources_manager_access_character_body( manager->physics_resources_manager, *component );
  cJSON_AddItemToObject( dynamic_body_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_physics_character_body_handle ) ) );
  cJSON_AddItemToObject( dynamic_body_json, "mask", cJSON_CreateNumber( character_body->mask ) );
  return dynamic_body_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_physics_character_body_handle )
{
  crude_physics_character_body *character_body = crude_physics_resources_manager_access_character_body( manager->physics_resources_manager, *component );
  ImGui::Text( "Mask" );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "0", &character_body->mask, 1 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "1", &character_body->mask, 1 << 2 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "2", &character_body->mask, 1 << 3 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "3", &character_body->mask, 1 << 4 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "4", &character_body->mask, 1 << 5 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "5", &character_body->mask, 1 << 6 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "6", &character_body->mask, 1 << 7 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "7", &character_body->mask, 1 << 8 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "8", &character_body->mask, 1 << 9 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "9", &character_body->mask, 1 << 10 );
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_physics_static_body_handle )
{
  crude_physics_static_body *static_body = crude_physics_resources_manager_access_static_body( manager->physics_resources_manager, *component );
  ImGui::Text( "Layer" );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "0", &static_body->layer, 1 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "1", &static_body->layer, 1 << 2 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "2", &static_body->layer, 1 << 3 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "3", &static_body->layer, 1 << 4 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "4", &static_body->layer, 1 << 5 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "5", &static_body->layer, 1 << 6 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "6", &static_body->layer, 1 << 7 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "7", &static_body->layer, 1 << 8 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "8", &static_body->layer, 1 << 9 );
  ImGui::SameLine( );
  ImGui::CheckboxFlags( "9", &static_body->layer, 1 << 10 );
  
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_physics_collision_shape )
{
  crude_memory_set( component, 0, sizeof( crude_physics_collision_shape ) );
  
  component->type = crude_physics_collision_shape_string_to_type( cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( component_json, "shape_type" ) ) );
  if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
  {
    crude_parse_json_to_float3( &component->box.half_extent, cJSON_GetObjectItemCaseSensitive( component_json, "half_extent" ) );
  }
  else if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
  {
    component->sphere.radius = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "radius" ) );
  }
  else if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_MESH )
  {
    char const *local_relative_path = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( component_json, "path" ) );
    crude_string_copy( component->mesh.model_relative_filepath, local_relative_path, sizeof( component->mesh.model_relative_filepath ) );
    component->mesh.octree_handle = crude_collisions_resources_manager_get_octree_handle( manager->collisions_resources_manager, component->mesh.model_relative_filepath );
  }
  else
  {
    CRUDE_ASSERT( false );
  }

  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_physics_collision_shape )
{
  cJSON *collision_shape_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( collision_shape_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_physics_collision_shape ) ) );
  cJSON_AddItemToObject( collision_shape_json, "shape_type", cJSON_CreateString( crude_physics_collision_shape_type_to_string( component->type ) ) );
  if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
  {
    cJSON_AddItemToObject( collision_shape_json, "half_extent", cJSON_CreateFloatArray( &component->box.half_extent.x, 3 ) );
  }
  else if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
  {
    cJSON_AddItemToObject( collision_shape_json, "radius", cJSON_CreateNumber( component->sphere.radius ) );
  }
  else if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_MESH )
  {
    cJSON_AddItemToObject( collision_shape_json, "path", cJSON_CreateString( component->mesh.model_relative_filepath ) );
  }
  else
  {
    CRUDE_ASSERT( false );
  }

  return collision_shape_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_physics_collision_shape )
{
  if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
  {
    ImGui::Text( "Type: Box" );
    if ( ImGui::DragFloat3( "Half Extent", &component->box.half_extent.x, 0.01 ) )
    {
      CRUDE_ENTITY_COMPONENT_MODIFIED( world, node, crude_physics_collision_shape );
    }
  }
  else if ( component->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
  {
    ImGui::Text( "Type: Sphere" );
    if ( ImGui::DragFloat( "Radius", &component->sphere.radius, 0.01 ) )
    {
      CRUDE_ENTITY_COMPONENT_MODIFIED( world, node, crude_physics_collision_shape );
    }
  }
}

/**********************************************************
 *
 *                 Systems
 *
 *********************************************************/
CRUDE_ECS_SYSTEM_DECLARE( crude_physics_system );

CRUDE_ECS_OBSERVER_DECLARE( crude_physics_static_body_handle_destroy_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_character_body_handle_destroy_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_physics_update_system_ );

static void
crude_physics_static_body_handle_destroy_observer_ 
(
  ecs_iter_t *it
)
{
  crude_physics_system_context *ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  crude_physics_static_body_handle *static_body_handle_per_entity = ecs_field( it, crude_physics_static_body_handle, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_resources_manager_destroy_static_body( ctx->physics->manager, static_body_handle_per_entity[ i ] );
  }
}

static void
crude_physics_character_body_handle_destroy_observer_ 
(
  ecs_iter_t *it
)
{
  crude_physics_system_context *ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  crude_physics_character_body_handle *character_body_handle_per_entity = ecs_field( it, crude_physics_character_body_handle, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_resources_manager_destroy_character_body( ctx->physics->manager, character_body_handle_per_entity[ i ] );
  }
}

static void
crude_physics_update_system_ 
(
  ecs_iter_t *it
)
{
  crude_physics_character_body_handle                     *character_body_handle_per_entity;
  crude_physics_collision_shape                           *collision_shape_per_entity;
  crude_transform                                         *transform_per_entity;
  crude_physics_system_context                            *ctx;
  float32                                                  delta_time;
  
  CRUDE_PROFILER_ZONE_NAME( "crude_physics_update_system_" );

  ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  character_body_handle_per_entity = ecs_field( it, crude_physics_character_body_handle, 0 );
  collision_shape_per_entity = ecs_field( it, crude_physics_collision_shape, 1 );
  transform_per_entity = ecs_field( it, crude_transform, 2 );
  
  if ( !ctx->physics->simulation_enabled )
  {
    goto cleanup;
  }

  delta_time = CRUDE_MIN( it->delta_time, 0.016f );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_transform                                       *transform;
    crude_physics_collision_shape                         *collision_shape;
    crude_physics_character_body                          *character_body;
    crude_entity                                           character_body_node;
    ecs_iter_t                                             static_body_handle_it;
    crude_physics_character_body_handle                    character_body_handle;
    XMMATRIX                                               node_to_parent;
    XMMATRIX                                               parent_to_world;
    XMMATRIX                                               node_to_world;
    XMVECTOR                                               translation;
    XMVECTOR                                               velocity;
    
    character_body_handle = character_body_handle_per_entity[ i ];
    collision_shape = &collision_shape_per_entity[ i ];
    transform = &transform_per_entity[ i ];

    character_body_node = crude_entity_from_iterator( it, i );

    character_body = crude_physics_resources_manager_access_character_body( ctx->physics->manager, character_body_handle );

    velocity = XMLoadFloat3( &character_body->velocity );
    
    parent_to_world = crude_transform_parent_to_world( it->world, character_body_node );
    node_to_parent = crude_transform_node_to_parent( transform );
    node_to_world = XMMatrixMultiply( node_to_parent, parent_to_world );
    
    translation = XMVectorAdd( node_to_world.r[ 3 ], velocity * delta_time );

    character_body->on_floor = false;

    static_body_handle_it = ecs_query_iter( it->world, ctx->physics->static_body_handle_query );
    while ( ecs_query_next( &static_body_handle_it ) )
    {
      crude_physics_collision_shape                       *second_collision_shape_per_entity;
      crude_transform                                     *second_transform_per_entity;
      crude_physics_static_body_handle                    *second_body_handle_per_entity;

      second_body_handle_per_entity = ecs_field( &static_body_handle_it, crude_physics_static_body_handle, 0 );
      second_collision_shape_per_entity = ecs_field( &static_body_handle_it, crude_physics_collision_shape, 1 );
      second_transform_per_entity = ecs_field( &static_body_handle_it, crude_transform, 2 );

      for ( uint32 static_body_index = 0; static_body_index < static_body_handle_it.count; ++static_body_index )
      {
        crude_physics_collision_shape                     *second_collision_shape;
        crude_transform                                   *second_transform;
        crude_physics_static_body_handle                  *second_body_handle;
        crude_physics_static_body                         *second_body;
        crude_entity                                       second_body_node;
        XMMATRIX                                           second_transform_mesh_to_world;
        XMVECTOR                                           second_translation, closest_point;
        bool                                               intersected;
        
        second_collision_shape = &second_collision_shape_per_entity[ static_body_index ];
        second_transform = &second_transform_per_entity[ static_body_index ];
        second_body_handle = &second_body_handle_per_entity[ static_body_index ];
        second_body_node = crude_entity_from_iterator( &static_body_handle_it, static_body_index );
     
        second_body = crude_physics_resources_manager_access_static_body( ctx->physics->manager, *second_body_handle );

        if ( !second_body->enabeld )
        {
          continue;
        }

        if ( !( character_body->mask & second_body->layer ) )
        {
          continue;
        }

        second_transform_mesh_to_world = crude_transform_node_to_world( it->world, second_body_node, second_transform );
        second_translation = second_transform_mesh_to_world.r[ 3 ];

        if ( second_collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
        {
          XMVECTOR                                           second_box_half_extent;

          second_box_half_extent = XMLoadFloat3( &second_collision_shape->box.half_extent );
          closest_point = crude_closest_point_to_obb( translation, second_translation, second_box_half_extent, second_transform_mesh_to_world );
          intersected = crude_intersection_sphere_obb( closest_point, translation, collision_shape->sphere.radius );
        }
        else if ( second_collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_MESH )
        {
          crude_octree *octree = crude_collisions_resources_manager_access_octree( ctx->physics->collision_manager, second_collision_shape->mesh.octree_handle );
          closest_point = crude_octree_closest_point( octree, translation );
          intersected = crude_intersection_sphere_triangle( closest_point, translation, collision_shape->sphere.radius );
        }
        else
        {
          CRUDE_ASSERT( false );
        }
        
        if ( intersected )
        {
          crude_physics_collision_callback_container_fun( character_body->callback_container, character_body_node, second_body_node, second_body->layer );

          if ( character_body->mask & 1 )
          {
            XMVECTOR                                         closest_point_to_translation;
            float32                                          translation_to_closest_point_projected_length;

            closest_point_to_translation = XMVectorSubtract( translation, closest_point );

            translation = XMVectorAdd( closest_point, XMVectorScale( XMVector3Normalize( closest_point_to_translation ), collision_shape->sphere.radius ) );
              
            closest_point_to_translation = XMVectorSubtract( translation, closest_point );
            translation_to_closest_point_projected_length = -1.f * XMVectorGetX( XMVector3Dot( closest_point_to_translation, XMVectorSet( 0, -1, 0, 1 ) ) );
            if ( translation_to_closest_point_projected_length > collision_shape->sphere.radius * 0.75f && translation_to_closest_point_projected_length < collision_shape->sphere.radius + 0.00001f ) // !TODO it works, so why not ahahah ( i like this solution :D )
            {
              character_body->on_floor = true;
            }
          }
        }
      }
    }

    XMStoreFloat3( &transform->translation, XMVectorSubtract( translation, parent_to_world.r[ 3 ] ) );
  }

cleanup:
  CRUDE_PROFILER_ZONE_END;
}

void
crude_physics_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_physics_system_context                       *ctx
)
{
  crude_physics_components_import( world, manager );

  //CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_static_body_handle_destroy_observer_, EcsOnRemove, ctx, { 
  //  { .id = ecs_id( crude_physics_static_body_handle ) }
  //} );
  //
  //CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_character_body_handle_destroy_observer_, EcsOnRemove, ctx, { 
  //  { .id = ecs_id( crude_physics_character_body_handle ) }
  //} );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_physics_update_system_, EcsOnUpdate, ctx, { 
    { .id = ecs_id( crude_physics_character_body_handle ) },
    { .id = ecs_id( crude_physics_collision_shape ) },
    { .id = ecs_id( crude_transform ) }
  } );
}