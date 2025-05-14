#include <core/assert.h>

#include <physics/physics.h>

/**
 * 
 * Particle Functions
 * 
 */
void
crude_physics_particle_integrate
(
  _In_ crude_physics_particle                   *particle,
  _In_ float32                                   duration
)
{
  CRUDE_ASSERT( duration > 0.f );

  crude_vector position = crude_load_float3( &particle->position );
  crude_vector velocity = crude_load_float3( &particle->velocity );
  crude_vector acceleration = crude_load_float3( &particle->acceleration );
  crude_vector force_accum = crude_load_float3( &particle->force_accum );

  position = crude_vec_scale_add( position, velocity, duration );
  crude_vector resulting_acc = crude_vec_scale_add( acceleration, force_accum, particle->inverse_mass );
  velocity = crude_vec_scale_add( velocity, resulting_acc, duration );
  velocity = crude_vec_scale( velocity, crude_pow( particle->damping, duration ) );

  crude_store_float3( &particle->position, position );
  crude_store_float3( &particle->velocity, velocity);
}

void
crude_physics_particle_add_force
(
  _In_ crude_physics_particle                   *particle,
  _In_ crude_vector const                        force
)
{
  crude_vector force_accum = crude_load_float3( &particle->force_accum );
  force_accum = crude_vec_add( force_accum, force );
  crude_store_float3( &particle->force_accum, force_accum );
}

void
crude_physics_particle_clear_force_accum
(
  _In_ crude_physics_particle                   *particle
)
{
  crude_store_float3( &particle->force_accum, crude_vec_zero() );
}

/**
 * 
 * Springs Functions
 * 
 */
void
crude_physics_particle_spring_update_force
(
  _In_ crude_physics_particle_spring            *spring,
  _In_ crude_physics_particle                   *particle,
  _In_ float32                                   duration
)
{
  crude_vector force;

  crude_vector particle_positon = crude_load_float3( &particle->position );
  crude_vector spring_positon = crude_load_float3( &spring->particle->position );

  force = crude_vec_subtract( particle_positon, spring_positon );
 
  crude_vector magnitude = crude_vec_length3( force );
  magnitude = crude_vec_abs( crude_vec_subtract( magnitude, crude_vec_replicate( spring->rest_length ) ) );
  magnitude = crude_vec_scale( magnitude, spring->spring_constant );

  force = crude_vec_normalize3( force );
  force = crude_vec_multiply( force, crude_vec_negate( magnitude ) );
  crude_physics_particle_add_force( particle, force );
}