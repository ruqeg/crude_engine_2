#include <engine/core/assert.h>
#include <engine/core/string.h>

#include <engine/physics/physics_resource.h>

crude_physics_character
crude_physics_character_empty
(
)
{
  crude_physics_character character = CRUDE_COMPOUNT_EMPTY( crude_physics_character );
  character.height = 1.35f;
  character.radius = 0.3f;
  character.friction = 0.5f;
  character.max_slop_angle = XM_PIDIV4;
  return character;
}

crude_physics_static_body
crude_physics_static_body_empty
(
)
{
  crude_physics_static_body body = CRUDE_COMPOUNT_EMPTY( crude_physics_static_body );
  body.type = CRUDE_PHYSICS_BODY_SHAPE_TYPE_BOX;
  body.box.extent.x = 1.f;
  body.box.extent.y = 1.f;
  body.box.extent.z = 1.f;
  return body;
}

crude_physics_kinematic_body
crude_physics_kinematic_body_empty
(
)
{
  crude_physics_kinematic_body body = CRUDE_COMPOUNT_EMPTY( crude_physics_kinematic_body );
  body.type = CRUDE_PHYSICS_BODY_SHAPE_TYPE_BOX;
  body.box.extent.x = 1.f;
  body.box.extent.y = 1.f;
  body.box.extent.z = 1.f;
  return body;
}

void
crude_jph_transform_to_transform
(
  _Out_ crude_transform                                   *transform,
  _In_ JPH::RMat44                                        *jph_transform
)
{
  JPH::Quat                                                jph_quaternion;

  transform->translation.x = jph_transform->GetTranslation( ).GetX( );
  transform->translation.z = jph_transform->GetTranslation( ).GetY( );
  transform->translation.y = jph_transform->GetTranslation( ).GetZ( );

  transform->scale.x = 1;
  transform->scale.y = 1;
  transform->scale.z = 1;

  jph_quaternion = jph_transform->GetQuaternion( );
  transform->rotation.x = jph_quaternion.GetX( );
  transform->rotation.y = jph_quaternion.GetY( );
  transform->rotation.z = jph_quaternion.GetZ( );
  transform->rotation.w = jph_quaternion.GetW( );
}

JPH::RVec3
crude_float3_to_jph_vec3
(
  _In_ XMFLOAT3                                           *float3
)
{
  return JPH::Vec3( float3->x, float3->y, float3->z );
}

JPH::Quat
crude_float4_to_jph_quat
(
  _In_ XMFLOAT4                                           *float4
)
{
  return JPH::Quat( float4->x, float4->y, float4->z, float4->w );
}

JPH::RVec3
crude_vector_to_jph_vec3
(
  _In_ XMVECTOR                                            vector
)
{
  return JPH::RVec3( XMVectorGetX( vector ), XMVectorGetY( vector ), XMVectorGetZ( vector ) );
}

JPH::Quat
crude_vector_to_jph_quat
(
  _In_ XMVECTOR                                            vector
)
{
  return JPH::Quat( XMVectorGetX( vector ), XMVectorGetY( vector ), XMVectorGetZ( vector ), XMVectorGetW( vector ) );
}

XMVECTOR
crude_jph_vec3_to_vector
(
  _In_ JPH::RVec3                                          v
)
{
  return XMVectorSet( v.GetX( ), v.GetY( ), v.GetZ( ), 0.f );
}

CRUDE_API XMVECTOR
crude_jph_quat_to_vector
(
  _In_ JPH::Quat                                           q
)
{
  return XMVectorSet( q.GetX( ), q.GetY( ), q.GetZ( ), q.GetW( ) );
}