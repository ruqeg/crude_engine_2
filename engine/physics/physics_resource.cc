#include <engine/core/assert.h>
#include <engine/core/string.h>

#include <engine/physics/physics_resource.h>

crude_physics_character
crude_physics_character_empty
(
)
{
  crude_physics_character character = CRUDE_COMPOUNT_EMPTY( crude_physics_character );
  character.character_height_standing = 1.35 ;
  character.character_radius_standing = 0.3;
  character.friction = 0.5;
  character.max_slop_angle = XM_PIDIV4;
  return character;
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