#pragma once

#include <core/math.h>
#include <core/array.h>

/**
 * 
 * Particle Struct
 * 
 */
typedef struct crude_physics_particle
{
  /**
    * Holds the linear position of the particle in
    * world space.
    */
  crude_float3                                             position;
  /*
   * Holds the linear velocity of the particle in
   * world space
   */
  crude_float3                                             velocity;
  /*
   * Holds the acceleration of the particle. This value
   * can be used to set acceleration due to gravity or
   * any other constants acceleration;
   */
  crude_float3                                             acceleration;
  /**
    * Holds the accumulated force to be applied at the next
    * simulation iteration only. This value is zeroed at each
    * integration step;
    */
  crude_float3                                             force_accum;
  /**
    * Holds the amount of damping applied to linear motion. 
    */
  float32                                                  damping;
  /**
    * Holds the inverse of the mass of the particle.
    * Useful to simulate objects with infinite mass 
    * (immovable).
    */
  float32                                                  inverse_mass;
} crude_physics_particle;

/**
 * 
 * Springs
 * 
 */
typedef struct crude_physics_particle_spring
{
  /** The particle at the other end of the string. */
  crude_physics_particle                                  *particle;
  /** Holds the spring constant. */
  float32                                                  spring_constant;
  /** Holds the rest length of the spring. */
  float32                                                  rest_length;
} crude_physics_particle_spring;

/**
 * 
 * Particle Functions
 * 
 */
CRUDE_API void
crude_physics_particle_integrate
(
  _In_ crude_physics_particle                             *particle,
  _In_ float32                                             duration
);

CRUDE_API void
crude_physics_particle_add_force
(
  _In_ crude_physics_particle                             *particle,
  _In_ crude_vector const                                  force
);

CRUDE_API void
crude_physics_particle_clear_force_accum
(
  _In_ crude_physics_particle                             *particle
);

/**
 * 
 * Springs Functions
 * 
 */
CRUDE_API void
crude_physics_particle_spring_update_force
(
  _In_ crude_physics_particle_spring                      *spring,
  _In_ crude_physics_particle                             *particle,
  _In_ float32                                             duration
);