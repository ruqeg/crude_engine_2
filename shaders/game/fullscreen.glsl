
#ifdef CRUDE_VALIDATOR_LINTING
#extension GL_GOOGLE_include_directive : enable
#define CRUDE_STAGE_FRAGMENT
//#define CRUDE_STAGE_VERTEX
#define GAME_POSTPROCESSING

#include "crude/platform.glsli"
#include "crude/debug.glsli"
#include "crude/scene.glsli"
#endif /* CRUDE_VALIDATOR_LINTING */

#if defined( GAME_POSTPROCESSING ) 

CRUDE_UNIFORM( SceneConstant, 0 ) 
{
  crude_scene                                              scene;
};

CRUDE_PUSH_CONSTANT
{
  vec3                                                     player_position;
  uint                                                     depth_texture_index;
  vec4                                                     fog_color;
  vec4                                                     pulse_color;
  uint                                                     pbr_texture_index;
  float                                                    fog_distance;
  float                                                    fog_coeff;
  float                                                    wave_size;
  float                                                    wave_texcoord_scale;
  float                                                    wave_absolute_frame_scale;
  float                                                    aberration_strength_scale;
  float                                                    aberration_strength_offset;
  float                                                    aberration_strength_sin_affect;
  float                                                    pulse_frame_scale;
  float                                                    pulse_scale;
  float                                                    pulse_distance_coeff;
  float                                                    pulse_distance;
  float                                                    star_coeff;
};

#if defined( CRUDE_STAGE_VERTEX )

layout(location=0) out vec2 out_texcoord;

const vec4 fullscreen_vertices[ 3 ] =
{
  vec4( -1.0f, -1.0f, 1.0f, 1.0f ),
  vec4( -1.0f,  3.0f, 1.0f, 1.0f ),
  vec4(  3.0f, -1.0f, 1.0f, 1.0f )
};

const vec2 fullscreen_texcoord[ 3 ] =
{
  vec2( 0.0f, 1.0f ),
  vec2( 0.0f, -1.0f ),
  vec2( 2.0f, 1.0f )
};

void main()
{
  out_texcoord = fullscreen_texcoord[ gl_VertexIndex ];
  gl_Position = fullscreen_vertices[ gl_VertexIndex ];
}

#endif /* CRUDE_STAGE_VERTEX */

#if defined( CRUDE_STAGE_FRAGMENT ) 

layout(location = 0) out vec4 out_radiance;

layout(location=0) in vec2 in_texcoord;

vec3 mod289(vec3 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
    return mod289(((x * 34.0) + 1.0) * x);
}

vec4 taylorInvSqrt(vec4 r) {
    return 1.79284291400159 - 0.85373472095314 * r;
}

// Main 3D Simplex Noise function
float crude_simplex_noise3d(vec3 v) {
    const vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);
    
    // First corner
    vec3 i = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);
    
    // Other corners
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);
    
    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;
    
    // Permutations
    i = mod289(i);
    vec4 p = permute(permute(permute(
                i.z + vec4(0.0, i1.z, i2.z, 1.0))
              + i.y + vec4(0.0, i1.y, i2.y, 1.0))
              + i.x + vec4(0.0, i1.x, i2.x, 1.0));
    
    // Gradients
    float n_ = 0.142857142857; // 1/7
    vec3 ns = n_ * D.wyz - D.xzx;
    
    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);
    
    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);
    
    vec4 x = x_ * ns.x + ns.yyyy;
    vec4 y = y_ * ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);
    
    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);
    
    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));
    
    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;
    
    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);
    
    // Normalise gradients
    vec4 norm = taylorInvSqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;
    
    // Mix final noise value
    vec4 m = max(0.6 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
    m = m * m;
    
    return 42.0 * dot(m * m, vec4(dot(p0, x0), dot(p1, x1), dot(p2, x2), dot(p3, x3)));
}

void main()
{ 
  float max_offset = wave_size + aberration_strength_scale + aberration_strength_offset;
  vec2 texcoord = in_texcoord * ( 1.f - 2.f * max_offset ) + max_offset;

  /* Drunk effect */
  float wave_x = sin( texcoord.y * wave_texcoord_scale + scene.absolute_time * wave_absolute_frame_scale ) * wave_size;
  float wave_y = cos( texcoord.x * wave_texcoord_scale + scene.absolute_time * wave_absolute_frame_scale ) * wave_size;
  
  vec2 drunk_texcoord = vec2( texcoord.x + wave_x, texcoord.y + wave_y ); 
  
  float aberration_strength = sin( scene.absolute_time * aberration_strength_sin_affect ) * aberration_strength_scale + aberration_strength_offset;

  vec2 offset = vec2( aberration_strength, aberration_strength );

  vec2 drunk_with_aberration_r_texcoord = clamp( ( drunk_texcoord + offset ), 0, 1 ); 
  vec2 drunk_with_aberration_g_texcoord = clamp( ( drunk_texcoord ), 0, 1 ); 
  vec2 drunk_with_aberration_b_texcoord = clamp( ( drunk_texcoord - offset ), 0, 1 ); 

  vec3 drunk_radiance;
  vec3 drunk_depth;
  drunk_depth.r = CRUDE_TEXTURE_FETCH( depth_texture_index, ivec2( drunk_with_aberration_r_texcoord * ( scene.resolution - vec2( 1 ) ) ), 0 ).r;
  drunk_depth.g = CRUDE_TEXTURE_FETCH( depth_texture_index, ivec2( drunk_with_aberration_g_texcoord * ( scene.resolution - vec2( 1 ) ) ), 0 ).r;
  drunk_depth.b = CRUDE_TEXTURE_FETCH( depth_texture_index, ivec2( drunk_with_aberration_b_texcoord * ( scene.resolution - vec2( 1 ) ) ), 0 ).r;
  drunk_radiance.r = CRUDE_TEXTURE_LOD( pbr_texture_index, drunk_with_aberration_r_texcoord, 0 ).r;
  drunk_radiance.g = CRUDE_TEXTURE_LOD( pbr_texture_index, drunk_with_aberration_g_texcoord, 0 ).g;
  drunk_radiance.b = CRUDE_TEXTURE_LOD( pbr_texture_index, drunk_with_aberration_b_texcoord, 0 ).b;

  /* Light effect */
  vec3 drunk_pixel_world_position_r = crude_world_position_from_depth( drunk_with_aberration_r_texcoord, drunk_depth.r, scene.camera.clip_to_world );
  vec3 drunk_pixel_world_position_g = crude_world_position_from_depth( drunk_with_aberration_g_texcoord, drunk_depth.g, scene.camera.clip_to_world );
  vec3 drunk_pixel_world_position_b = crude_world_position_from_depth( drunk_with_aberration_b_texcoord, drunk_depth.b, scene.camera.clip_to_world );

  float drunk_visbility_r = length( player_position - drunk_pixel_world_position_r );
  float drunk_visbility_g = length( player_position - drunk_pixel_world_position_g );
  float drunk_visbility_b = length( player_position - drunk_pixel_world_position_b );

  vec3 drunk_with_fog_radiance;
  drunk_with_fog_radiance.r = ( drunk_depth.r != 1.f ) ? ( mix( drunk_radiance.x, fog_color.a * fog_color.x, clamp( pow( drunk_visbility_r / fog_distance, fog_coeff ), 0, 1 ) ) ) :  fog_color.a * fog_color.x;
  drunk_with_fog_radiance.g = ( drunk_depth.g != 1.f ) ? ( mix( drunk_radiance.y, fog_color.a * fog_color.y, clamp( pow( drunk_visbility_g / fog_distance, fog_coeff ), 0, 1 ) ) ) :  fog_color.a * fog_color.y;
  drunk_with_fog_radiance.b = ( drunk_depth.b != 1.f ) ? ( mix( drunk_radiance.z, fog_color.a * fog_color.z, clamp( pow( drunk_visbility_b / fog_distance, fog_coeff ), 0, 1 ) ) ) :  fog_color.a * fog_color.z;
  
  /* Pulse with heartbeat effect */
  vec2 texcoord_to_center = in_texcoord - vec2( 0.5 );
  float texcoord_to_center_length = length( texcoord_to_center * scene.resolution_ratio );
  float pulse = abs( cos( scene.absolute_time * pulse_frame_scale) ) * pulse_scale;
  vec3 pulse_radiance = mix( drunk_with_fog_radiance, pulse_color.rgb * pulse_color.a, pulse * clamp( pow( texcoord_to_center_length / pulse_distance, pulse_distance_coeff ), 0, 1 ) );

  /* Stars */
  vec3 pixel_direction = normalize( vec3( ( in_texcoord.x - 0.5 )  * scene.resolution_ratio, (0.5 - in_texcoord.y), 1 ) ) * mat3( scene.camera.view_to_world );
  float noise = crude_simplex_noise3d( 200 * pixel_direction );

  if ( ( star_coeff > 0.1f ) && (drunk_depth.r == 1.f) )
  {
    float star = ( clamp( noise, 0.8f, 1.f ) - 0.8 ) / 0.2f;
    pulse_radiance = vec3( star );
  }
  out_radiance = vec4( pulse_radiance, 1.f );
  //out_radiance = vec4( out_radiance.b, out_radiance.g, out_radiance.r, out_radiance.a );
}

#endif /* CRUDE_STAGE_FRAGMENT */

#endif /* GAME_POSTPROCESSING */