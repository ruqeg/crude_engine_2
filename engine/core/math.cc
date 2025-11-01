#include <stdlib.h>
#include <cJSON.h>

#include <core/math.h>
#include <core/assert.h>

CRUDE_COMPONENT_STRING_DEFINE( XMFLOAT2, "float2" );
CRUDE_COMPONENT_STRING_DEFINE( XMFLOAT3, "float3" );
CRUDE_COMPONENT_STRING_DEFINE( XMFLOAT4, "float4" );

float32
crude_random_unit_f32
(
)
{
  return CRUDE_CAST( float32, ( rand( ) - RAND_MAX / 2 ) ) / ( RAND_MAX / 2 );
}

float32
crude_lerp_angle
(
  _In_ float32                                             from,
  _In_ float32                                             to,
  _In_ float32                                             weight
)
{
  float32 difference = fmod( to - from, XM_2PI );
  float32 distance = fmod( 2.0 * difference, XM_2PI ) - difference;
  return from + distance * weight;
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( XMFLOAT2 )
{
  CRUDE_ASSERT( cJSON_GetArraySize( component_json ) == 2 );

  component->x = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 0 ) ) );
  component->y = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 1 ) ) );
  return true;
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( XMFLOAT3 )
{
  CRUDE_ASSERT( cJSON_GetArraySize( component_json ) == 3 );
  component->x = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 0 ) ) );
  component->y = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 1 ) ) );
  component->z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 2 ) ) );
  return true;
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( XMFLOAT4 )
{
  CRUDE_ASSERT( cJSON_GetArraySize( component_json ) == 4 );
  component->x = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 0 ) ) );
  component->y = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 1 ) ) );
  component->z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 2 ) ) );
  component->w = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( component_json , 3 ) ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( XMFLOAT2 )
{
  return cJSON_CreateFloatArray( &component->x, 2 );
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( XMFLOAT3 )
{
  return cJSON_CreateFloatArray( &component->x, 3 );
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( XMFLOAT4 )
{
  return cJSON_CreateFloatArray( &component->x, 4 );
}