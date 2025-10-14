
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
//#define SAMPLE_IRRADIANCE
//#define PROBE_UPDATE_VISIBILITY
#define CALCULATE_PROBE_OFFSETS

#include "crude/platform.glsli"
#include "crude/scene.glsli"
#include "crude/debug.glsli"
#include "crude/light.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

//#define PROBE_STATUSES_DEBUG
#define PROBE_IRRADIANCE_DEBUG

#define CRUDE_PROBE_STATUS_OFF 0
#define CRUDE_PROBE_STATUS_ACTIVE 4
#define CRUDE_PROBE_STATUS_UNINITIALIZED 6
#define EPSILON 0.0001f

CRUDE_UNIFORM( DDGIConstants, 10 )
{
  ivec3                                                    probe_counts;
  int                                                      probe_rays;
  vec3                                                     probe_spacing;
  int                                                      probe_update_offset;
  vec3                                                     probe_grid_position;
  float                                                    max_probe_offset;

  uint                                                     radiance_output_index;
  uint                                                     indirect_output_index;
  uint                                                     normal_texture_index;
  uint                                                     depth_fullscreen_texture_index;
  
  uint                                                     grid_irradiance_output_index;
  uint                                                     grid_visibility_texture_index;
  uint                                                     probe_offset_texture_index;
  float                                                    hysteresis;
  
  mat4                                                     random_rotation;

  int                                                      irradiance_texture_width;
  int                                                      irradiance_texture_height;
  int                                                      irradiance_side_length;
  int                                                      visibility_texture_width;

  int                                                      visibility_texture_height;
  int                                                      visibility_side_length;
  float                                                    self_shadow_bias;
  float                                                    infinite_bounces_multiplier;
  
  vec3                                                     reciprocal_probe_spacing;
};

vec2 uv_nearest( ivec2 pixel, vec2 texture_size )
{
  vec2 uv = pixel + .5;
  return uv / texture_size;
}

int probe_indices_to_index( ivec3 probe_coords )
{
  return int( probe_coords.x + probe_coords.y * probe_counts.x + probe_coords.z * probe_counts.x * probe_counts.y );
}

vec3 grid_indices_to_world_no_offsets( ivec3 grid_indices )
{
  return grid_indices * probe_spacing + probe_grid_position;
}

vec3 grid_indices_to_world( ivec3 grid_indices, int probe_index )
{
  const int probe_counts_xy = probe_counts.x * probe_counts.y;
  ivec2 probe_offset_sampling_coordinates = ivec2( probe_index % probe_counts_xy, probe_index / probe_counts_xy );
  vec3 probe_offset = CRUDE_TEXTURE_FETCH( probe_offset_texture_index, probe_offset_sampling_coordinates, 0 ).rgb;
  return grid_indices_to_world_no_offsets( grid_indices ) + probe_offset;
}

ivec3 world_to_grid_indices( vec3 world_position )
{
  return clamp( ivec3(( world_position - probe_grid_position ) * reciprocal_probe_spacing ), ivec3( 0 ), probe_counts - ivec3( 1 ) );
}

ivec3 probe_index_to_grid_indices( int probe_index )
{
  int probe_x = probe_index % probe_counts.x;
  int probe_counts_xy = probe_counts.x * probe_counts.y;

  int probe_y = ( probe_index % probe_counts_xy ) / probe_counts.x;
  int probe_z = probe_index / probe_counts_xy;

  return ivec3( probe_x, probe_y, probe_z );
}

int get_probe_index_from_pixels( ivec2 pixels, int probe_with_border_side, int full_texture_width )
{
  int probes_per_side = full_texture_width / probe_with_border_side;
  return int( pixels.x / probe_with_border_side ) + probes_per_side * int( pixels.y / probe_with_border_side );
}

vec2 oct_encode( vec3 v )
{
  float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
  vec2 result = v.xy * (1.0 / l1norm);
  if (v.z < 0.0)
  {
    result = (1.0 - abs(result.yx)) * crude_sign_not_zero(result.xy);
  }
  return result;
}

vec3 oct_decode( vec2 o )
{
  vec3 v = vec3( o.x, o.y, 1.0 - abs( o.x ) - abs( o.y ) );
  if ( v.z < 0.0 )
  {
    v.xy = ( 1.0 - abs( v.yx ) ) * crude_sign_not_zero( v.xy );
  }
  return normalize( v );
}

vec2 normalized_oct_coord(ivec2 fragCoord, int probe_side_length)
{
  int probe_with_border_side = probe_side_length + 2;
  vec2 octahedral_texel_coordinates = ivec2((fragCoord.x - 1) % probe_with_border_side, (fragCoord.y - 1) % probe_with_border_side);

  octahedral_texel_coordinates += vec2(0.5f);
  octahedral_texel_coordinates *= (2.0f / float(probe_side_length));
  octahedral_texel_coordinates -= vec2(1.0f);

  return octahedral_texel_coordinates;
}

vec2 get_probe_uv( vec3 direction, int probe_index, int full_texture_width, int full_texture_height, int probe_side_length )
{
  // Get octahedral coordinates (-1,1)
  vec2 octahedral_coordinates = oct_encode(normalize(direction));
  // TODO: use probe index for this.
  const float probe_with_border_side = float(probe_side_length) + 2.0f;
  const int probes_per_row = (full_texture_width) / int(probe_with_border_side);
  // Get probe indices in the atlas
  ivec2 probe_indices = ivec2((probe_index % probes_per_row), 
                               (probe_index / probes_per_row));
    
  // Get top left atlas texels
  vec2 atlas_texels = vec2( probe_indices.x * probe_with_border_side, probe_indices.y * probe_with_border_side );
  // Account for 1 pixel border
  atlas_texels += vec2(1.0f);
  // Move to center of the probe area
  atlas_texels += vec2(probe_side_length * 0.5f);
  // Use octahedral coordinates (-1,1) to move between internal pixels, no border
  atlas_texels += octahedral_coordinates * (probe_side_length * 0.5f);
  // Calculate final uvs
  const vec2 uv = atlas_texels / vec2(float(full_texture_width), float(full_texture_height));
  return uv;
}

vec3 sample_irradiance( vec3 world_position, vec3 normal, vec3 camera_position )
{
  const float minimum_distance_between_probes = 1.0f;

  vec3 v = normalize( camera_position.xyz - world_position );
  
  vec3 bias_vector = ( normal * 0.2f + v * 0.8f) * ( 0.75f * minimum_distance_between_probes ) * self_shadow_bias;
  vec3 biased_world_position = world_position + bias_vector;

  /* Sample at world position + probe offset reduces shadow leaking. */
  ivec3 base_grid_indices = world_to_grid_indices( biased_world_position );
  vec3 base_probe_world_position = grid_indices_to_world_no_offsets( base_grid_indices );

  /* Alpha is how far from the floor(currentVertex) position. on [0, 1] for each axis. */
  vec3 alpha = clamp( ( biased_world_position - base_probe_world_position ) , vec3( 0.0f ), vec3( 1.0f ) );

  vec3 sum_irradiance = vec3( 0.0f );
  float sum_weight = 0.0f;

  /* Iterate over adjacent probe cage */
  for ( int i = 0; i < 8; ++i )
  {
    /* Compute the offset grid coord and clamp to the probe grid boundary. Offset = 0 or 1 along each axis */
    ivec3 offset = ivec3( i, i >> 1, i >> 2 ) & ivec3( 1 );
    ivec3 probe_grid_coord = clamp( base_grid_indices + offset, ivec3( 0 ), probe_counts - ivec3( 1 ) );
    int probe_index = probe_indices_to_index( probe_grid_coord );

    vec3 probe_pos = grid_indices_to_world( probe_grid_coord, probe_index );
    vec3 trilinear = mix( 1.0 - alpha, alpha, offset );
    float weight = 1.0;

    vec3 probe_to_biased_point_direction = biased_world_position - probe_pos;
    float distance_to_biased_point = length( probe_to_biased_point_direction );
    probe_to_biased_point_direction *= 1.0 / distance_to_biased_point;

    vec2 probe_visibility_uv = get_probe_uv( probe_to_biased_point_direction, probe_index, visibility_texture_width, visibility_texture_height, visibility_side_length );
    vec2 visibility = CRUDE_TEXTURE_LOD( grid_visibility_texture_index, probe_visibility_uv, 0 ).rg;
    float mean_distance_to_occluder = visibility.x;
    float mean_distance_to_occluder2 = visibility.y;

    float chebyshev_weight = 1.0;
    if ( distance_to_biased_point > mean_distance_to_occluder )
    {
      float variance = abs( ( mean_distance_to_occluder * mean_distance_to_occluder ) - mean_distance_to_occluder2 );
      
      /* http://www.punkuser.net/vsm/vsm_paper.pdf */
      const float distance_diff = distance_to_biased_point - mean_distance_to_occluder;
      chebyshev_weight = variance / ( variance + ( distance_diff * distance_diff ) );
      chebyshev_weight = max( ( chebyshev_weight * chebyshev_weight * chebyshev_weight ), 0.0f );
    }

    chebyshev_weight = max(0.05f, chebyshev_weight);
    weight *= chebyshev_weight;

    weight = max( 0.000001, weight );

    const float crushThreshold = 0.2f;
    if ( weight < crushThreshold )
    {
      weight *= ( weight * weight ) * ( 1.f / ( crushThreshold * crushThreshold ) );
    }

    vec2 probe_irradiance_uv = get_probe_uv( normal, probe_index, irradiance_texture_width, irradiance_texture_height, irradiance_side_length );

    vec3 probe_irradiance = CRUDE_TEXTURE_LOD( grid_irradiance_output_index, probe_irradiance_uv, 0 ).rgb;

    weight *= trilinear.x * trilinear.y * trilinear.z + 0.001f;

    sum_irradiance += weight * probe_irradiance;
    sum_weight += weight;
  }

  vec3 net_irradiance = sum_irradiance / sum_weight;
  vec3 irradiance = 0.5f * PI * net_irradiance * 0.95f;
  return irradiance;
}

#if defined( PROBE_RAYTRACER )

#extension GL_EXT_ray_tracing : enable

struct ray_payload
{
  vec3                                                     radiance;
  float                                                    distance;
};

CRUDE_UNIFORM( SceneConstant, 0 ) 
{
  crude_scene                                              scene;
};

CRUDE_RBUFFER( MeshDraws, 1 )
{
  crude_mesh_draw                                          mesh_draws[];
};

CRUDE_RBUFFER( MeshInstancesDraws, 2 )
{
  crude_mesh_instance_draw                                 mesh_instance_draws[];
};

CRUDE_RBUFFER( Lights, 3 ) 
{
  crude_light                                              lights[];
};

layout(set=CRUDE_MATERIAL_SET, binding=5) uniform accelerationStructureEXT as;

#if defined( CRUDE_RAYGEN )

layout(location=0) rayPayloadEXT ray_payload payload;

void main()
{
  ivec2 pixel_coord = ivec2( gl_LaunchIDEXT.xy );
  int probe_index = pixel_coord.y + probe_update_offset;
  int ray_index = pixel_coord.x;

  int probe_counts = probe_counts.x * probe_counts.y * probe_counts.z;
  if ( probe_index >= probe_counts )
  {
    return;
  }

  ivec3 probe_grid_indices = probe_index_to_grid_indices( probe_index );
  vec3 ray_origin = grid_indices_to_world( probe_grid_indices, probe_index );
  vec3 direction = normalize( crude_spherical_fibonacci( ray_index, probe_rays ) * mat3( random_rotation ) );
  payload.radiance = vec3( 0 );
  payload.distance = 0;

  traceRayEXT( as, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, ray_origin, 0.0, direction, 100.0, 0 );

  imageStore( global_images_2d[ radiance_output_index ], ivec2( ray_index, probe_index ), vec4( payload.radiance, payload.distance ) );
}

#endif /* CRUDE_RAYGEN */

#if defined( CRUDE_CLOSEST_HIT )

layout(location=0) rayPayloadInEXT ray_payload payload;

hitAttributeEXT vec2 barycentric_weights;

float attenuation_square_falloff(vec3 position_to_light, float light_inverse_radius) {
    const float distance_square = dot(position_to_light, position_to_light);
    const float factor = distance_square * light_inverse_radius * light_inverse_radius;
    const float smoothFactor = max(1.0 - factor * factor, 0.0);
    return (smoothFactor * smoothFactor) / max(distance_square, 1e-4);
}

void main()
{
  vec3 radiance = vec3( 0 );
  float distance = 0.0f;
  
  if ( gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT )
  {
    distance = gl_RayTminEXT + gl_HitTEXT;
    distance *= -0.2;        
  }
  else
  {
    uint mesh_index = mesh_instance_draws[ gl_GeometryIndexEXT ].mesh_draw_index;
    crude_mesh_draw mesh_draw = mesh_draws[ mesh_index ];

    int_array_type index_buffer = int_array_type( mesh_draw.index_buffer );
    int i0 = index_buffer[ gl_PrimitiveID * 3 ].v;
    int i1 = index_buffer[ gl_PrimitiveID * 3 + 1 ].v;
    int i2 = index_buffer[ gl_PrimitiveID * 3 + 2 ].v;

    float_array_type vertex_buffer = float_array_type( mesh_draw.position_buffer );
    vec4 p0 = vec4(
      vertex_buffer[ i0 * 3 + 0 ].v,
      vertex_buffer[ i0 * 3 + 1 ].v,
      vertex_buffer[ i0 * 3 + 2 ].v,
      1.0
    );
    vec4 p1 = vec4(
      vertex_buffer[ i1 * 3 + 0 ].v,
      vertex_buffer[ i1 * 3 + 1 ].v,
      vertex_buffer[ i1 * 3 + 2 ].v,
      1.0
    );
    vec4 p2 = vec4(
      vertex_buffer[ i2 * 3 + 0 ].v,
      vertex_buffer[ i2 * 3 + 1 ].v,
      vertex_buffer[ i2 * 3 + 2 ].v,
      1.0
    );

    mat4 model_to_world = mesh_instance_draws[ gl_GeometryIndexEXT ].model_to_world;
    vec4 p0_world = p0 * model_to_world;
    vec4 p1_world = p1 * model_to_world;
    vec4 p2_world = p2 * model_to_world;

    vec2_array_type texcoord_buffer = vec2_array_type( mesh_draw.texcoord_buffer );
    vec2 texcoord0 = texcoord_buffer[ i0 ].v;
    vec2 texcoord1 = texcoord_buffer[ i1 ].v;
    vec2 texcoord2 = texcoord_buffer[ i2 ].v;

    float b = barycentric_weights.x;
    float c = barycentric_weights.y;
    float a = 1 - b - c;

    vec2 texcoord = ( a * texcoord0 + b * texcoord1 + c * texcoord2 );

    vec3 albedo = CRUDE_TEXTURE_LOD( mesh_draw.textures.x, texcoord, 0 ).rgb;
    // Don't work for some reason if ( mesh_draw.textures.x != CRUDE_TEXTURE_INVALID )

    float_array_type normals_buffer = float_array_type( mesh_draw.normal_buffer );
    vec3 n0 = vec3(
      normals_buffer[ i0 * 3 + 0 ].v,
      normals_buffer[ i0 * 3 + 1 ].v,
      normals_buffer[ i0 * 3 + 2 ].v
    );
    vec3 n1 = vec3(
      normals_buffer[ i1 * 3 + 0 ].v,
      normals_buffer[ i1 * 3 + 1 ].v,
      normals_buffer[ i1 * 3 + 2 ].v
    );
    vec3 n2 = vec3(
      normals_buffer[ i2 * 3 + 0 ].v,
      normals_buffer[ i2 * 3 + 1 ].v,
      normals_buffer[ i2 * 3 + 2 ].v
    );

    vec3 normal = a * n0 + b * n1 + c * n2;
    mat3 world_to_model = mat3( mesh_instance_draws[ gl_GeometryIndexEXT ].world_to_model );
    normal = normal * world_to_model;

    vec3 world_position = a * p0_world.xyz + b * p1_world.xyz + c * p2_world.xyz;

    // TODO calculate lighting.
    crude_light light = lights[ 0 ];

    vec3 position_to_light = light.world_position - world_position;
    vec3 l = normalize( position_to_light );
    float ndotl = clamp( dot( normal, l ), 0.0, 1.0 );

    float attenuation = attenuation_square_falloff( position_to_light, 1.0f / light.radius );

    vec3 light_intensity = vec3( 0.0f );
    if ( attenuation > 0.001f && ndotl > 0.001f )
    {
      light_intensity += attenuation * ndotl * light.color * light.intensity;
    }

    vec3 diffuse = albedo * light_intensity;
    diffuse += albedo * sample_irradiance( world_position, normal, scene.camera.position.xyz ) * infinite_bounces_multiplier;

    radiance = diffuse;
    distance = gl_RayTminEXT + gl_HitTEXT;
  }

  payload.radiance = radiance;
  payload.distance = distance;
}

#endif /* CRUDE_CLOSEST_HIT */

#if defined( CRUDE_MISS )

layout(location=0) rayPayloadInEXT ray_payload payload;

void main()
{
  payload.radiance = scene.background_color * scene.background_intensity;
  payload.distance = 1000;
}

#endif /* CRUDE_MISS */

#endif /* PROBE_RAYTRACER */

#if defined( PROBE_UPDATE_IRRADIANCE ) || defined( PROBE_UPDATE_VISIBILITY )

layout(set=CRUDE_MATERIAL_SET, binding=0, rgba16f) uniform image2D irradiance_image;
layout(set=CRUDE_MATERIAL_SET, binding=1, rg16f) uniform image2D visibility_image;

layout(local_size_x=8, local_size_y=8, local_size_z=1) in;

int read_table[ 6 ] =
{
  5, 3, 1, -1, -3, -5
};

void main()
{
  ivec3 coords = ivec3( gl_GlobalInvocationID.xyz );

#if defined( PROBE_UPDATE_IRRADIANCE )
  int probe_texture_width = irradiance_texture_width;
  int probe_texture_height = irradiance_texture_height;
  int probe_side_length = irradiance_side_length;
#else
  int probe_texture_width = visibility_texture_width;
  int probe_texture_height = visibility_texture_height;
  int probe_side_length = visibility_side_length;
#endif

  if ( coords.x >= probe_texture_width || coords.y >= probe_texture_height )
  {
    return;
  }

  uint probe_with_border_side = probe_side_length + 2;
  uint probe_last_pixel = probe_side_length + 1;

  int probe_index = get_probe_index_from_pixels( coords.xy, int( probe_with_border_side ), probe_texture_width );

  bool border_pixel = ( ( gl_GlobalInvocationID.x % probe_with_border_side ) == 0 ) || ( ( gl_GlobalInvocationID.x % probe_with_border_side ) == probe_last_pixel );
  border_pixel = border_pixel || ( ( gl_GlobalInvocationID.y % probe_with_border_side ) == 0) || ( ( gl_GlobalInvocationID.y % probe_with_border_side ) == probe_last_pixel );

  if ( !border_pixel )
  {
    vec4 result = vec4( 0.f );

    const float energy_conservation = 0.95;
    
    uint backfaces = 0;
    uint max_backfaces = uint( probe_rays * 0.1f );

    for ( int ray_index = 0; ray_index < probe_rays; ++ray_index )
    {
      ivec2 sample_position = ivec2( ray_index, probe_index );
      vec3 ray_direction = normalize( crude_spherical_fibonacci( ray_index, probe_rays ) * mat3( random_rotation ) );

      vec3 texel_direction = oct_decode( normalized_oct_coord( coords.xy, probe_side_length ) );
      
      float weight = max( 0.0, dot( texel_direction, ray_direction ) );

      float distance2 = CRUDE_TEXTURE_FETCH( radiance_output_index, sample_position, 0 ).w;
      if ( distance2 < 0.0f )
      {
        ++backfaces;
        
        if ( backfaces >= max_backfaces )
        {
          return;
        }
        
        continue;
      }

#if defined( PROBE_UPDATE_IRRADIANCE )
      if ( weight >= EPSILON )
      {
        vec3 radiance = CRUDE_TEXTURE_FETCH( radiance_output_index, sample_position, 0 ).rgb;
        radiance.rgb *= energy_conservation;

        result += vec4( radiance * weight, weight );
      }
#else
      float probe_max_ray_distance = 1.0f * 1.5f;

      weight = pow( weight, 2.5f );
      if ( weight >= EPSILON )
      {
        float distance = CRUDE_TEXTURE_FETCH( radiance_output_index, sample_position, 0 ).w;
        distance = min( abs( distance ), probe_max_ray_distance );
        vec3 value = vec3( distance, distance * distance, 0 );
        result += vec4( value * weight, weight );
      }
#endif
    }

    if ( result.w > EPSILON )
    {
      result.xyz /= result.w;
      result.w = 1.0f;
    }

#if defined( PROBE_UPDATE_IRRADIANCE )
    vec4 previous_value = imageLoad( irradiance_image, coords.xy );
#else
    vec2 previous_value = imageLoad( visibility_image, coords.xy ).xy;
#endif

#if defined( PROBE_UPDATE_IRRADIANCE )
    result = mix( result, previous_value, hysteresis );
    imageStore(irradiance_image, coords.xy, result );
#else
    result.rg = mix( result.rg, previous_value, hysteresis );
    imageStore(visibility_image, coords.xy, vec4(result.rg, 0, 1));
#endif
    return;
  }

  groupMemoryBarrier( );
  barrier( );

  uint probe_pixel_x = gl_GlobalInvocationID.x % probe_with_border_side;
  uint probe_pixel_y = gl_GlobalInvocationID.y % probe_with_border_side;
  bool corner_pixel = ( probe_pixel_x == 0 || probe_pixel_x == probe_last_pixel ) && ( probe_pixel_y == 0 || probe_pixel_y == probe_last_pixel );
  bool row_pixel = ( probe_pixel_x > 0 && probe_pixel_x < probe_last_pixel );

  ivec2 source_pixel_coordinate = coords.xy;

  if ( corner_pixel )
  {
    source_pixel_coordinate.x += probe_pixel_x == 0 ? probe_side_length : -probe_side_length;
    source_pixel_coordinate.y += probe_pixel_y == 0 ? probe_side_length : -probe_side_length;
  }
  else if ( row_pixel )
  {
    source_pixel_coordinate.x += read_table[probe_pixel_x - 1];
    source_pixel_coordinate.y += ( probe_pixel_y > 0 ) ? -1 : 1;
  }
  else
  {
    source_pixel_coordinate.x += ( probe_pixel_x > 0 ) ? -1 : 1;
    source_pixel_coordinate.y += read_table[ probe_pixel_y - 1 ];
  }

#if defined( PROBE_UPDATE_IRRADIANCE )
  vec4 copied_data = imageLoad( irradiance_image, source_pixel_coordinate );
#else
  vec4 copied_data = imageLoad( visibility_image, source_pixel_coordinate );
#endif

#if defined( PROBE_UPDATE_IRRADIANCE )
  imageStore( irradiance_image, coords.xy, copied_data );
#else
  imageStore( visibility_image, coords.xy, copied_data );
#endif
}

#endif // PROBE_UPDATE_IRRADIANCE || PROBE_UPDATE_VISIBILITY

#if defined( CALCULATE_PROBE_OFFSETS )

CRUDE_PUSH_CONSTANT( PushConstants )
{
  uint                                                     first_frame;
};

layout(local_size_x=32, local_size_y=1, local_size_z=1) in;

void main()
{
  ivec3 coords = ivec3( gl_GlobalInvocationID.xyz );
  int probe_index = coords.x;

  int closest_backface_index = -1;
  float closest_backface_distance = 100000000.f;

  int closest_frontface_index = -1;
  float closest_frontface_distance = 100000000.f;

  int farthest_frontface_index = -1;
  float farthest_frontface_distance = 0;
  
  int backfaces_count = 0;

  int total_probes = probe_counts.x * probe_counts.y * probe_counts.z;
  if ( probe_index >= total_probes )
  {
    return;
  }

  for ( int ray_index = 0; ray_index < probe_rays; ++ray_index )
  {
    ivec2 ray_tex_coord = ivec2( ray_index, probe_index );

    float d_front = CRUDE_TEXTURE_FETCH( radiance_output_index, ray_tex_coord, 0 ).w;
    float d_back = -d_front;

    if ( d_back > 0.f ) /* not sure about >= 0.f for CALCULATE_PROBE_STATUSES */
    {
      backfaces_count += 1;
      if ( d_back < closest_backface_distance )
      {
        closest_backface_distance = d_back;
        closest_backface_index = ray_index;
      }
    }

    if ( d_front > 0.0f )
    {
      if ( d_front < closest_frontface_distance )
      {
        closest_frontface_distance = d_front;
        closest_frontface_index = ray_index;
      }
      else if ( d_front > farthest_frontface_distance )
      {
        farthest_frontface_distance = d_front;
        farthest_frontface_index = ray_index;
      }
    }
  }

  // TODO double check this one
  vec3 full_offset = vec3( 10000.f );
  vec3 cell_offset_limit = max_probe_offset * probe_spacing;

  vec4 current_offset = vec4( 0.f );

  if ( first_frame == 0 )
  {
    int probe_counts_xy = probe_counts.x * probe_counts.y;
    ivec2 probe_offset_sampling_coordinates = ivec2( probe_index % probe_counts_xy, probe_index / probe_counts_xy );
    current_offset.rgb = CRUDE_TEXTURE_FETCH( probe_offset_texture_index, probe_offset_sampling_coordinates, 0 ).rgb;
  }

  bool inside_geometry = ( float( backfaces_count ) / probe_rays ) > 0.25f;
  if ( inside_geometry && ( closest_backface_index != -1 ) )
  {
    vec3 closest_backface_direction = closest_backface_distance * normalize( crude_spherical_fibonacci( closest_backface_index, probe_rays ) * mat3( random_rotation ) );
    
    vec3 positive_offset = ( current_offset.xyz + cell_offset_limit ) / closest_backface_direction;
    vec3 negative_offset = ( current_offset.xyz - cell_offset_limit ) / closest_backface_direction;
    vec3 maximum_offset = vec3( max( positive_offset.x, negative_offset.x ), max( positive_offset.y, negative_offset.y ), max( positive_offset.z, negative_offset.z ) );

    float direction_scale_factor = min( min( maximum_offset.x, maximum_offset.y ), maximum_offset.z ) - 0.001f;
    full_offset = current_offset.xyz - closest_backface_direction * direction_scale_factor;
  }
  else if ( closest_frontface_distance < 0.05f )
  {
    vec3 farthest_direction = min( 0.2f, farthest_frontface_distance) * normalize( crude_spherical_fibonacci( farthest_frontface_index, probe_rays ) * mat3(random_rotation) );
    vec3 closest_direction = normalize( crude_spherical_fibonacci( closest_frontface_index, probe_rays ) * mat3( random_rotation ) );

    if ( dot( farthest_direction, closest_direction ) < 0.5f )
    {
      full_offset = current_offset.xyz + farthest_direction;
    }
  } 

  if ( all( lessThan( abs( full_offset ), cell_offset_limit ) ) )
  {
    current_offset.xyz = full_offset;
  }

  const int probe_counts_xy = probe_counts.x * probe_counts.y;
  ivec2 probe_offset_store_coordinates = ivec2( probe_index % probe_counts_xy, probe_index / probe_counts_xy );
  imageStore( global_images_2d[ probe_offset_texture_index ], probe_offset_store_coordinates, current_offset );
}

#endif /* CALCULATE_PROBE_OFFSETS */


#if defined( SAMPLE_IRRADIANCE )

CRUDE_UNIFORM( SceneConstant, 0 ) 
{
  crude_scene                                              scene;
};

CRUDE_PUSH_CONSTANT( PushConstants )
{
  uint                                                     output_resolution_half;
};

ivec2 pixel_offsets[] =
{
  ivec2( 0, 0 ),
  ivec2( 0, 1 ),
  ivec2( 1, 0 ),
  ivec2( 1, 1 )
};

layout(local_size_x=8, local_size_y=8, local_size_z=1) in;

void main()
{
  ivec3 coords = ivec3( gl_GlobalInvocationID.xyz );

  int resolution_divider = output_resolution_half == 1 ? 2 : 1;
  vec2 screen_uv = uv_nearest( coords.xy, scene.resolution / resolution_divider );
    
  float raw_depth = 1.0f;
  int chosen_hiresolution_sample_index = 0;
  if ( output_resolution_half == 1 )
  {
    float closer_depth = 0.f;
    for ( int i = 0; i < 4; ++i )
    {
      float depth = CRUDE_TEXTURE_FETCH( depth_fullscreen_texture_index, ( coords.xy ) * 2 + pixel_offsets[ i ], 0 ).r;

      if ( closer_depth < depth )
      {
        closer_depth = depth;
        chosen_hiresolution_sample_index = i;
      }
    }

    raw_depth = closer_depth;
  }
  else
  {
    raw_depth = CRUDE_TEXTURE_FETCH( depth_fullscreen_texture_index, coords.xy, 0).r;
  }

  if ( raw_depth == 1.0f )
  {
    imageStore( global_images_2d[ indirect_output_index ], coords.xy, vec4( 0, 0, 0, 1 ) );
    return;
  }

  vec3 normal = vec3( 0 );

  if ( output_resolution_half == 1 )
  {
    vec2 encoded_normal = CRUDE_TEXTURE_FETCH( normal_texture_index, (coords.xy) * 2 + pixel_offsets[ chosen_hiresolution_sample_index ], 0).rg;
    normal = normalize(crude_octahedral_decode(encoded_normal));
  }
  else
  {
    vec2 encoded_normal = CRUDE_TEXTURE_FETCH( normal_texture_index, coords.xy, 0 ).rg;
    normal = crude_octahedral_decode(encoded_normal);
  }

  vec3 pixel_world_position = crude_world_position_from_depth( screen_uv, raw_depth, scene.camera.clip_to_world );
  vec3 irradiance = sample_irradiance( pixel_world_position, normal, scene.camera.position.xyz );

  imageStore( global_images_2d[ indirect_output_index ], coords.xy, vec4( irradiance, 1 ) );
}

#endif /* SAMPLE_IRRADIANCE */

#if defined( PROBE_DEBUG )

CRUDE_PUSH_CONSTANT( PushConstants )
{
  uint                                                     probe_debug_flags;
};

layout(local_size_x=32, local_size_y=1, local_size_z=1) in;

void main()
{
  ivec3 coords = ivec3( gl_GlobalInvocationID.xyz );
  int probe_index = coords.x;

  int probe_counts = probe_counts.x * probe_counts.y * probe_counts.z;
  if ( probe_index >= probe_counts )
  {
    return;
  }

  ivec3 probe_grid_indices = probe_index_to_grid_indices( probe_index );
  vec3 ray_origin = grid_indices_to_world( probe_grid_indices, probe_index );
  
  if ( ( probe_debug_flags & 1 ) == 1 )
  {
    int closest_backface_index = -1;
    float closest_backface_distance = 100000000.f;

    int closest_frontface_index = -1;
    float closest_frontface_distance = 100000000.f;

    int farthest_frontface_index = -1;
    float farthest_frontface_distance = 0;
  
    int backfaces_count = 0;

    int offset = 0;
    uint flag = CRUDE_PROBE_STATUS_UNINITIALIZED;
    vec3 outer_bounds = normalize( probe_spacing ) * ( length( probe_spacing ) + ( 2.f * self_shadow_bias ) );

    for ( int ray_index = 0; ray_index < probe_rays; ++ray_index )
    {
      ivec2 ray_tex_coord = ivec2( ray_index, probe_index );

      float d_front = CRUDE_TEXTURE_FETCH( radiance_output_index, ray_tex_coord, 0 ).w;
      float d_back = -d_front;

      if ( d_back > 0.f ) /* not sure about >= 0.f for CALCULATE_PROBE_STATUSES */
      {
        backfaces_count += 1;
        if ( d_back < closest_backface_distance )
        {
          closest_backface_distance = d_back;
          closest_backface_index = ray_index;
        }
      }

      if ( d_front > 0.0f )
      {
        vec3 probe_ray = crude_spherical_fibonacci( ray_index, probe_rays ) * mat3( random_rotation );
        vec3 front_face_direction = d_front * probe_ray;
        if ( all( lessThan( abs( front_face_direction ), outer_bounds ) ) )
        {
          flag = CRUDE_PROBE_STATUS_ACTIVE;
        }
        
        if ( d_front < closest_frontface_distance )
        {
          closest_frontface_distance = d_front;
          closest_frontface_index = ray_index;
        }
        else if ( d_front > farthest_frontface_distance )
        {
          farthest_frontface_distance = d_front;
          farthest_frontface_index = ray_index;
        }
      }
    }

    const bool inside_geometry = closest_backface_index != -1 && ( float( backfaces_count ) / probe_rays ) > 0.25f;
    if ( inside_geometry )
    {
      flag = CRUDE_PROBE_STATUS_OFF;
    }
    else if ( closest_frontface_index == -1 )
    {
      flag = CRUDE_PROBE_STATUS_OFF;
    }
    else if ( closest_frontface_distance < 0.05f )
    {
      flag = CRUDE_PROBE_STATUS_ACTIVE;
    }

    if ( flag == CRUDE_PROBE_STATUS_ACTIVE )
    {
      crude_debug_draw_cube( ray_origin, vec3( 0.1 ), vec4( 0, 1, 0, 1 )  );
    }
    else
    {
      crude_debug_draw_cube( ray_origin, vec3( 0.1 ), vec4( 1, 0, 0, 1 )  );
    }
  }

  if ( ( probe_debug_flags & 2 ) == 2 )
  {
    vec3 radiance = vec3( 0.f );

    const float energy_conservation = 0.95;
    
    for ( int ray_index = 0; ray_index < probe_rays; ++ray_index )
    {
      ivec2 sample_position = ivec2( ray_index, probe_index );
      vec3 ray_direction = normalize( crude_spherical_fibonacci( ray_index, probe_rays ) * mat3( random_rotation ) );

      float distance2 = CRUDE_TEXTURE_FETCH( radiance_output_index, sample_position, 0 ).w;
      if ( distance2 < 0.0f )
      { 
        continue;
      }

      radiance += energy_conservation * CRUDE_TEXTURE_FETCH( radiance_output_index, sample_position, 0 ).rgb;
    }

    crude_debug_draw_cube( ray_origin, vec3( 0.1 ), vec4( radiance / probe_rays, 1 ) ); // fine for now, maybe move to pixel shader for models with proper calculation for debug
  }
}
#endif /* PROBE_DEBUG */