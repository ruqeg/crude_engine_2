
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#define PROBE_RAYTRACER
#define CRUDE_RAYGEN

#include "crude/platform.glsli"
#include "crude/scene.glsli"
#include "crude/light.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#define CRUDE_PROBE_STATUS_OFF 0
#define CRUDE_PROBE_STATUS_SLEEP 1
#define CRUDE_PROBE_STATUS_ACTIVE 4
#define CRUDE_PROBE_STATUS_UNINITIALIZED 6
#define EPSILON 0.0001f

#if defined( PROBE_RAYTRACER )
#extension GL_EXT_ray_tracing : enable

struct crude_ray_payload
{
  vec3                                                     radiance;
  float                                                    distance;
};
#endif /* PROBE_RAYTRACER */

layout(set=CRUDE_MATERIAL_SET, binding=1, row_major, std430) readonly buffer CrudeMeshDraws
{
  crude_mesh_draw                                          mesh_draws[];
};

layout(set=CRUDE_MATERIAL_SET, binding=2, row_major, std430) readonly buffer CrudeMeshInstancesDraws
{
  crude_mesh_instance_draw                                 mesh_instance_draws[];
};

layout(set=CRUDE_MATERIAL_SET, binding=40, std140) readonly buffer DDGIConstants
{
  ivec3                                                    probe_counts;
  int                                                      probe_rays;
  vec3                                                     probe_spacing;
  uint                                                     radiance_output_index;
  mat4                                                     random_rotation;
  vec3                                                     probe_grid_position;
  int                                                      irradiance_texture_width;
  int                                                      irradiance_texture_height;
  int                                                      irradiance_side_length;
  float                                                    hysteresis;
  int                                                      visibility_texture_width;
  int                                                      visibility_texture_height;
  int                                                      visibility_side_length;
  float                                                    self_shadow_bias;
};

layout(set=CRUDE_MATERIAL_SET, binding=41, std430) buffer ProbeStatusSSBO
{
  uint                                                     probe_status[];
};


int k_read_table[6] = {5, 3, 1, -1, -3, -5};

int crude_get_probe_index_from_pixels( ivec2 pixels, int probe_with_border_side, int full_texture_width )
{
 int probes_per_side = full_texture_width / probe_with_border_side;
  return int(pixels.x / probe_with_border_side) + probes_per_side * int(pixels.y / probe_with_border_side);
}

float sign_not_zero(in float k) {
    return (k >= 0.0) ? 1.0 : -1.0;
}

vec2 sign_not_zero2(in vec2 v) {
    return vec2(sign_not_zero(v.x), sign_not_zero(v.y));
}

// Returns a unit vector. Argument o is an octahedral vector packed via oct_encode,
// on the [-1, +1] square
vec3 oct_decode(vec2 o) {
    vec3 v = vec3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));
    if (v.z < 0.0) {
        v.xy = (1.0 - abs(v.yx)) * sign_not_zero2(v.xy);
    }
    return normalize(v);
}
// Compute normalized oct coord, mapping top left of top left pixel to (-1,-1) and bottom right to (1,1)
vec2 normalized_oct_coord(ivec2 fragCoord, int probe_side_length) {

    int probe_with_border_side = probe_side_length + 2;
    vec2 octahedral_texel_coordinates = ivec2((fragCoord.x - 1) % probe_with_border_side, (fragCoord.y - 1) % probe_with_border_side);

    octahedral_texel_coordinates += vec2(0.5f);
    octahedral_texel_coordinates *= (2.0f / float(probe_side_length));
    octahedral_texel_coordinates -= vec2(1.0f);

    return octahedral_texel_coordinates;
}

vec3 crude_spherical_fibonacci( float i, float n )
{
  const float PHI = sqrt( 5.0f ) * 0.5 + 0.5;
#define madfrac( A, B ) ( ( A ) * ( B ) - floor( ( A ) * ( B ) ) )
  float phi       = 2.0 * PI * madfrac( i, PHI - 1 );
  float cos_theta = 1.0 - ( 2.0 * i + 1.0 ) * ( 1.0 / n );
  float sin_theta = sqrt( clamp( 1.0 - cos_theta * cos_theta, 0.0f, 1.0f ) );

  return vec3( cos( phi ) * sin_theta, sin( phi ) * sin_theta, cos_theta );
#undef madfrac
}

ivec3 crude_probe_index_to_grid_indices( int probe_index )
{
  const int probe_x = probe_index % probe_counts.x;
  const int probe_counts_xy = probe_counts.x * probe_counts.y;

  const int probe_y = ( probe_index % probe_counts_xy ) / probe_counts.x;
  const int probe_z = probe_index / probe_counts_xy;

  return ivec3( probe_x, probe_y, probe_z );
}

vec3 crude_grid_indices_to_world_no_offsets( ivec3 grid_indices )
{
  return grid_indices * probe_spacing + probe_grid_position;
}

vec3 crude_grid_indices_to_world( ivec3 grid_indices, int probe_index )
{
  const int probe_counts_xy = probe_counts.x * probe_counts.y;
  ivec2 probe_offset_sampling_coordinates = ivec2(probe_index % probe_counts_xy, probe_index / probe_counts_xy);
  vec3 probe_offset = vec3(0);

  return crude_grid_indices_to_world_no_offsets( grid_indices ) + probe_offset;
}

#if defined( PROBE_RAYTRACER ) && defined( CRUDE_CLOSEST_HIT )
layout(location=0) rayPayloadInEXT crude_ray_payload payload;

hitAttributeEXT vec2 barycentric_weights;

void main()
{
  vec3 radiance = vec3( 0 );
  float distance = 0.0f;
  if ( gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT )
  {
    distance = gl_RayTminEXT + gl_HitTEXT;
    distance *= -0.2;      
    payload.radiance = radiance;
    payload.distance = distance;
    return;  
  }
  
  uint mesh_index = mesh_instance_draws[ gl_GeometryIndexEXT ].mesh_draw_index;
  crude_mesh_draw mesh = mesh_draws[ mesh_index ];

  int_array_type index_buffer = int_array_type( mesh.index_buffer );
  int i0 = index_buffer[ gl_PrimitiveID * 3 ].v;
  int i1 = index_buffer[ gl_PrimitiveID * 3 + 1 ].v;
  int i2 = index_buffer[ gl_PrimitiveID * 3 + 2 ].v;

  float_array_type vertex_buffer = float_array_type( mesh.position_buffer );
  vec4 p0 = vec4( vertex_buffer[ i0 * 3 + 0 ].v, vertex_buffer[ i0 * 3 + 1 ].v, vertex_buffer[ i0 * 3 + 2 ].v, 1.0 );
  vec4 p1 = vec4( vertex_buffer[ i1 * 3 + 0 ].v, vertex_buffer[ i1 * 3 + 1 ].v, vertex_buffer[ i1 * 3 + 2 ].v, 1.0 );
  vec4 p2 = vec4( vertex_buffer[ i2 * 3 + 0 ].v, vertex_buffer[ i2 * 3 + 1 ].v, vertex_buffer[ i2 * 3 + 2 ].v, 1.0 );

  mat4 model_to_world = mesh_instance_draws[ gl_GeometryIndexEXT ].model_to_world;
  vec4 p0_world = p0 * model_to_world;
  vec4 p1_world = p1 * model_to_world;
  vec4 p2_world = p2 * model_to_world;

  vec2_array_type texcoord_buffer = vec2_array_type( mesh.texcoord_buffer );
  vec2 uv0 = texcoord_buffer[ i0 ].v;
  vec2 uv1 = texcoord_buffer[ i1 ].v;
  vec2 uv2 = texcoord_buffer[ i2 ].v;

  float b = barycentric_weights.x;
  float c = barycentric_weights.y;
  float a = 1 - b - c;

  vec2 uv = ( a * uv0 + b * uv1 + c * uv2 );
  vec3 albedo = textureLod( global_textures[ nonuniformEXT( mesh.textures.x ) ], uv, 3 ).rgb;

  float_array_type normals_buffer = float_array_type( mesh.normal_buffer );
  vec3 n0 = vec3( normals_buffer[ i0 * 3 + 0 ].v, normals_buffer[ i0 * 3 + 1 ].v, normals_buffer[ i0 * 3 + 2 ].v );
  vec3 n1 = vec3( normals_buffer[ i1 * 3 + 0 ].v, normals_buffer[ i1 * 3 + 1 ].v, normals_buffer[ i1 * 3 + 2 ].v );
  vec3 n2 = vec3( normals_buffer[ i2 * 3 + 0 ].v, normals_buffer[ i2 * 3 + 1 ].v, normals_buffer[ i2 * 3 + 2 ].v );
  vec3 normal = a * n0 + b * n1 + c * n2;

  mat3 world_to_model = mat3( mesh_instance_draws[ gl_GeometryIndexEXT ].world_to_model );
  normal = normal * world_to_model;

  vec3 world_position = a * p0_world.xyz + b * p1_world.xyz + c * p2_world.xyz;

  // TODO
  crude_light light = lights[ 0 ];

  vec3 position_to_light = light.world_position - world_position;
  vec3 l = normalize( position_to_light );
  float NoL = clamp(dot(normal, l), 0.0, 1.0);

  vec3 light_to_position = light.world_position - world_position;
  float light_distance = length( light_to_position );

  float attenuation = max( 1.f - pow( light_distance / light.radius, 2.f ), 0.f );
  attenuation = attenuation * attenuation;
  vec3 light_intensity = vec3(0.0f);
  if ( attenuation > 0.001f && NoL > 0.001f )
  {
    light_intensity += ( light.intensity * attenuation * NoL ) * light.color;
  }

  vec3 diffuse = albedo * light_intensity;
  radiance = diffuse;
  distance = gl_RayTminEXT + gl_HitTEXT;

  payload.radiance = radiance;
  payload.distance = distance;
}
#endif /* PROBE_RAYTRACER && CRUDE_CLOSEST_HIT */


#if defined( PROBE_RAYTRACER ) && defined( CRUDE_RAYGEN )
layout(location=0) rayPayloadEXT crude_ray_payload payload;

void main()
{
  const ivec2 pixel_coord = ivec2( gl_LaunchIDEXT.xy );
  const int probe_index = pixel_coord.y;
  const int ray_index = pixel_coord.x;

  const bool skip_probe = ( probe_status[ probe_index ] == CRUDE_PROBE_STATUS_OFF ) || ( probe_status[ probe_index ] == CRUDE_PROBE_STATUS_UNINITIALIZED );
  if ( skip_probe )
  {
    return;
  }

  ivec3 probe_grid_indices = crude_probe_index_to_grid_indices( probe_index );
  vec3 ray_origin = crude_grid_indices_to_world( probe_grid_indices, probe_index );
  vec3 direction = normalize( mat3( random_rotation ) * crude_spherical_fibonacci( ray_index, probe_rays ) );
  payload.radiance = vec3(0);
  payload.distance = 0;

  traceRayEXT( acceleration_structure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, ray_origin, 0.0, direction, 100.0, 0 );

  imageStore( global_images_2d[ radiance_output_index ], ivec2( ray_index, probe_index ), vec4( payload.radiance, payload.distance ) );
}
#endif /* PROBE_RAYTRACER && CRUDE_RAYGEN */

 
#if defined( PROBE_RAYTRACER ) && defined( CRUDE_MISS )
layout(location=0) rayPayloadInEXT crude_ray_payload payload;

void main()
{
  payload.radiance = vec3( 0.529, 0.807, 0.921 );
  payload.distance = 1000;
}
#endif /* PROBE_RAYTRACER && CRUDE_MISS */

#if defined( CRUDE_TSD_PROBE_UPDATE_IRRADIANCE_COMP )
layout(set=CRUDE_MATERIAL_SET, binding=41, rgba16f) uniform image2D irradiance_image;

void main()
{
  ivec3 coords = ivec3( gl_GlobalInvocationID.xyz );

  int probe_texture_width = irradiance_texture_width;
  int probe_texture_height = irradiance_texture_height;
  int probe_side_length = irradiance_side_length;
  
  if ( coords.x >= probe_texture_width || coords.y >= probe_texture_height )
  {
    return;
  }

  uint probe_with_border_side = probe_side_length + 2;
  uint probe_last_pixel = probe_side_length + 1;

  int probe_index = crude_get_probe_index_from_pixels(coords.xy, int(probe_with_border_side), probe_texture_width);

  bool border_pixel = ( ( gl_GlobalInvocationID.x % probe_with_border_side ) == 0 ) || ( ( gl_GlobalInvocationID.x % probe_with_border_side ) == probe_last_pixel );
  border_pixel = border_pixel || ( ( gl_GlobalInvocationID.y % probe_with_border_side ) == 0 ) || ( ( gl_GlobalInvocationID.y % probe_with_border_side ) == probe_last_pixel );

  if ( !border_pixel )
  {
    vec4 result = vec4( 0 );
    float energy_conservation = 0.95;

    uint backfaces = 0;
    uint max_backfaces = uint( probe_rays * 0.1f );

    for ( int ray_index = 0; ray_index < probe_rays; ++ray_index )
    {
      ivec2 sample_position = ivec2( ray_index, probe_index );
      vec3 ray_direction = normalize( crude_spherical_fibonacci( ray_index, probe_rays ) * mat3( random_rotation ) );
      vec3 texel_direction = oct_decode( normalized_oct_coord( coords.xy, probe_side_length ) );
      float weight = max(0.0, dot(texel_direction, ray_direction));
      float distance2 = texelFetch(global_textures[nonuniformEXT(radiance_output_index)], sample_position, 0).w;
      if ( distance2 < 0.0f )
      {
        ++backfaces;

        if (backfaces >= max_backfaces)
          return;

        continue;
      }

      if (weight >= EPSILON)
      {
        vec3 radiance = texelFetch(global_textures[nonuniformEXT(radiance_output_index)], sample_position, 0).rgb;
        radiance.rgb *= energy_conservation;
        result += vec4(radiance * weight, weight);
      }
    }

    if (result.w > EPSILON)
    {
      result.xyz /= result.w;
      result.w = 1.0f;
    }

    vec4 previous_value = imageLoad( irradiance_image, coords.xy );

    //if ( use_perceptual_encoding() ) {
      result.rgb = pow(result.rgb, vec3(1.0f / 5.0f));    
    //}

    result = mix( result, previous_value, hysteresis );
    imageStore(irradiance_image, coords.xy, result);

        // NOTE: returning here.
        return;
    }

    // Wait for all local threads to have finished to copy the border pixels.
    groupMemoryBarrier();
    barrier();

    // Copy border pixel calculating source pixels.
    const uint probe_pixel_x = gl_GlobalInvocationID.x % probe_with_border_side;
    const uint probe_pixel_y = gl_GlobalInvocationID.y % probe_with_border_side;
    bool corner_pixel = (probe_pixel_x == 0 || probe_pixel_x == probe_last_pixel) &&
                        (probe_pixel_y == 0 || probe_pixel_y == probe_last_pixel);
    bool row_pixel = (probe_pixel_x > 0 && probe_pixel_x < probe_last_pixel);

    ivec2 source_pixel_coordinate = coords.xy;

    if ( corner_pixel ) {
        source_pixel_coordinate.x += probe_pixel_x == 0 ? probe_side_length : -probe_side_length;
        source_pixel_coordinate.y += probe_pixel_y == 0 ? probe_side_length : -probe_side_length;

        //if (show_border_type()) {
        //    source_pixel_coordinate = ivec2(2,2);
        //}
    }
    else if ( row_pixel ) {
        source_pixel_coordinate.x += k_read_table[probe_pixel_x - 1];
        source_pixel_coordinate.y += (probe_pixel_y > 0) ? -1 : 1;

        //if (show_border_type()) {
        //    source_pixel_coordinate = ivec2(3,3);
        //}
    }
    else {
        source_pixel_coordinate.x += (probe_pixel_x > 0) ? -1 : 1;
        source_pixel_coordinate.y += k_read_table[probe_pixel_y - 1];

        //if (show_border_type()) {
        //    source_pixel_coordinate = ivec2(4,4);
        //}
    }

    vec4 copied_data = imageLoad( irradiance_image, source_pixel_coordinate );

    // Debug border source coordinates
    //if ( show_border_source_coordinates() ) {
    //    copied_data = vec4(coords.xy, source_pixel_coordinate);
    //}

    // Debug border with color red
    //if (show_border_vs_inside()) {
    //    copied_data = vec4(1,0,0,1);
    //}

    imageStore( irradiance_image, coords.xy, copied_data );
}
#endif /* CRUDE_TSD_PROBE_UPDATE_IRRADIANCE_COMP */

#if defined( CRUDE_TSD_PROBE_UPDATE_VISIBILITY_COMP )
layout(set=CRUDE_MATERIAL_SET, binding=41, rgba16f) uniform image2D visibility_image;

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main() {

    ivec3 coords = ivec3(gl_GlobalInvocationID.xyz);

    int probe_texture_width = visibility_texture_width;
    int probe_texture_height = visibility_texture_height;
    int probe_side_length = visibility_side_length;

    // Early out for 1 pixel border around all image and outside bound pixels.
    if (coords.x >= probe_texture_width || coords.y >= probe_texture_height) {
        return;
    }

    const uint probe_with_border_side = probe_side_length + 2;
    const uint probe_last_pixel = probe_side_length + 1;

    int probe_index = crude_get_probe_index_from_pixels(coords.xy, int(probe_with_border_side), probe_texture_width);

    // Check if thread is a border pixel
    bool border_pixel = ((gl_GlobalInvocationID.x % probe_with_border_side) == 0) || ((gl_GlobalInvocationID.x % probe_with_border_side ) == probe_last_pixel );
    border_pixel = border_pixel || ((gl_GlobalInvocationID.y % probe_with_border_side) == 0) || ((gl_GlobalInvocationID.y % probe_with_border_side ) == probe_last_pixel );

    // Perform full calculations
    if ( !border_pixel ) {
        vec4 result = vec4(0);

        const float energy_conservation = 0.95;

        uint backfaces = 0;
        uint max_backfaces = uint(probe_rays * 0.1f);

        for ( int ray_index = 0; ray_index < probe_rays; ++ray_index ) {
            ivec2 sample_position = ivec2( ray_index, probe_index );

            vec3 ray_direction = normalize( mat3(random_rotation) * crude_spherical_fibonacci(ray_index, probe_rays) );

            vec3 texel_direction = oct_decode(normalized_oct_coord(coords.xy, probe_side_length));

            float weight = max(0.0, dot(texel_direction, ray_direction));

            float distance2 = texelFetch(global_textures[nonuniformEXT(radiance_output_index)], sample_position, 0).w;
            if ( distance2 < 0.0f ) {
                ++backfaces;

                // Early out: only blend ray radiance into the probe if the backface threshold hasn't been exceeded
                if (backfaces >= max_backfaces)
                    return;

                continue;
            }

            // TODO: spacing is 1.0f
            float probe_max_ray_distance = 1.0f * 1.5f;

            // Increase or decrease the filtered distance value's "sharpness"
            weight = pow(weight, 2.5f);

            if (weight >= EPSILON) {
                float distance = texelFetch(global_textures[nonuniformEXT(radiance_output_index)], sample_position, 0).w;
                // Limit
                distance = min(abs(distance), probe_max_ray_distance);
                vec3 value = vec3(distance, distance * distance, 0);
                // Storing the sum of the weights in alpha temporarily
                result += vec4(value * weight, weight);
            }
        }

        if (result.w > EPSILON) {
            result.xyz /= result.w;
            result.w = 1.0f;
        }

        // Read previous frame value
        vec2 previous_value = imageLoad( visibility_image, coords.xy ).rg;

        result.rg = mix( result.rg, previous_value, hysteresis );
        imageStore(visibility_image, coords.xy, vec4(result.rg, 0, 1));

        // NOTE: returning here.
        return;
    }

    // Wait for all local threads to have finished to copy the border pixels.
    groupMemoryBarrier();
    barrier();

    // Copy border pixel calculating source pixels.
    const uint probe_pixel_x = gl_GlobalInvocationID.x % probe_with_border_side;
    const uint probe_pixel_y = gl_GlobalInvocationID.y % probe_with_border_side;
    bool corner_pixel = (probe_pixel_x == 0 || probe_pixel_x == probe_last_pixel) &&
                        (probe_pixel_y == 0 || probe_pixel_y == probe_last_pixel);
    bool row_pixel = (probe_pixel_x > 0 && probe_pixel_x < probe_last_pixel);

    ivec2 source_pixel_coordinate = coords.xy;

    if ( corner_pixel ) {
        source_pixel_coordinate.x += probe_pixel_x == 0 ? probe_side_length : -probe_side_length;
        source_pixel_coordinate.y += probe_pixel_y == 0 ? probe_side_length : -probe_side_length;
    }
    else if ( row_pixel ) {
        source_pixel_coordinate.x += k_read_table[probe_pixel_x - 1];
        source_pixel_coordinate.y += (probe_pixel_y > 0) ? -1 : 1;
    }
    else {
        source_pixel_coordinate.x += (probe_pixel_x > 0) ? -1 : 1;
        source_pixel_coordinate.y += k_read_table[probe_pixel_y - 1];
    }

    vec4 copied_data = imageLoad( visibility_image, source_pixel_coordinate );


    imageStore( visibility_image, coords.xy, copied_data );
}
#endif /* CRUDE_TSD_PROBE_UPDATE_VISIBILITY_COMP */

#if defined( CRUDE_TSD_CALCULATE_PROBE_STATUS )

layout( push_constant ) uniform PushConstants
{
  uint                                                     first_frame;
};

void main()
{
  ivec3 coords = ivec3( gl_GlobalInvocationID.xyz );
  int offset = 0;

  int probe_index = coords.x;

  int closest_backface_index = -1;
  float closest_backface_distance = 100000000.f;

  int closest_frontface_index = -1;
  float closest_frontface_distance = 100000000.f;

  int farthest_frontface_index = -1;
  float farthest_frontface_distance = 0;

  int backfaces_count = 0;
  uint flag = first_frame == 1 ? CRUDE_PROBE_STATUS_UNINITIALIZED : probe_status[probe_index];

    // Worst case, view and normal contribute in the same direction, so need 2x self-shadow bias.
  vec3 outerBounds = normalize(probe_spacing) * (length(probe_spacing) + (2.0f * self_shadow_bias));

    for (int ray_index = 0; ray_index < probe_rays; ++ray_index) {

        ivec2 ray_tex_coord = ivec2(ray_index, probe_index);

        // Distance is negative if we hit a backface
        float d_front = texelFetch(global_textures[nonuniformEXT(radiance_output_index)], ray_tex_coord, 0).w;
        float d_back = -d_front;

        //Backface test backface -> position.w < 0.0f
        if (d_back > 0.0f) {
            backfaces_count += 1;
            if (d_back < closest_backface_distance) {
                // This distance is negative on a backface hit
                closest_backface_distance = d_back;
                // Recompute ray direction
                closest_backface_index = ray_index;
            }
        }

        if (d_front > 0.0f) {
            // Need to check all frontfaces to see if any are wihtin shading range.
            vec3 frontFaceDirection = d_front * normalize( mat3(random_rotation) * crude_spherical_fibonacci(ray_index, probe_rays) );
            if (all(lessThan(abs(frontFaceDirection), outerBounds))) {
                // There is a static surface being shaded by this probe. Make it "just vigilant".
                flag = CRUDE_PROBE_STATUS_ACTIVE;
            }
            if (d_front < closest_frontface_distance) {
                closest_frontface_distance = d_front;
                closest_frontface_index = ray_index;
            } else if (d_front > farthest_frontface_distance) {
                farthest_frontface_distance = d_front;
                farthest_frontface_index = ray_index;
            }
        }
    }

    // If there's a close backface AND you see more than 25% backfaces, assume you're inside something.
    if (closest_backface_index != -1 && (float(backfaces_count) / probe_rays) > 0.25f) {
        // At this point, we were just in a wall, so set probe to "Off".
        flag = CRUDE_PROBE_STATUS_OFF;
    }
    else if (closest_frontface_index == -1) {
        // Probe sees only backfaces and sky, so set probe to "Off".
       flag = CRUDE_PROBE_STATUS_OFF;
    }
    else if (closest_frontface_distance < 0.05f) {
        // We hit no backfaces and a close frontface (within 2 cm). Set to "Newly Vigilant".
        flag = CRUDE_PROBE_STATUS_ACTIVE;
    } 

    // Write probe status
    probe_status[probe_index] = flag;
}
#endif /* CRUDE_TSD_CALCULATE_PROBE_STATUS */