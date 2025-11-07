#pragma once

#include <core/ecs.h>
#include <core/math.h>
#include <physics/physics_components.h>

typedef struct crude_physics_creation
{
  uint32                                                   num_threads;
  /**
   * This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
   * Note: For a real project use something in the order of 65536.
   */
  uint32                                                   max_rigid_bodies;
  /**
   * This determines how many mutexes to allocate to protect rigid bodies from concurrent access.
   * Set it to 0 for the default settings.
   */
  uint32                                                   num_body_mutexes;
  /**
   * This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
   * body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
   * too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
   * Note: For a real project use something in the order of 65536.
   */
  uint32                                                   max_body_pairs;
  /**
   * This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
   * number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
   * Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
   */
  uint32                                                    max_contact_constraints;

  uint32                                                    temporary_allocator_size;
} crude_physics_creation;


/************************************************
 *
 * Functions Declaratin
 * 
 ***********************************************/
CRUDE_ECS_MODULE_IMPORT_DECL( crude_physics_system );

CRUDE_API void
crude_physics_initialize
(
  _In_ crude_physics_creation const                       *creation
);

CRUDE_API void
crude_physics_deinitialize
(
);

CRUDE_API void
crude_physics_update
(
  _In_ float32                                             delta_time
);

CRUDE_API crude_physics_creation
crude_physics_creation_empty
(
);

CRUDE_API XMVECTOR
crude_physics_body_get_center_of_mass_translation
(
  _In_ crude_physics_body_handle const                    *dynamic_body
);

CRUDE_API void
crude_physics_body_set_translation
(
  _In_ crude_physics_body_handle const                    *dynamic_body,
  _In_ XMVECTOR                                            position
);

CRUDE_API void
crude_physics_body_set_rotation
(
  _In_ crude_physics_body_handle const                    *dynamic_body,
  _In_ XMVECTOR                                            rotation
);

CRUDE_API void
crude_physics_body_set_linear_velocity
(
  _In_ crude_physics_body_handle const                    *dynamic_body,
  _In_ XMVECTOR                                            velocity
);

CRUDE_API void
crude_physics_body_add_linear_velocity
(
  _In_ crude_physics_body_handle const                    *dynamic_body,
  _In_ XMVECTOR                                            velocity
);

CRUDE_API XMVECTOR
crude_physics_body_get_linear_velocity
(
  _In_ crude_physics_body_handle const                    *dynamic_body
);

CRUDE_API void
crude_physics_body_set_scale
(
  _In_ crude_physics_body_handle const                    *body,
  _In_ XMVECTOR                                            scale
);

CRUDE_API XMVECTOR
crude_physics_body_get_rotation
(
  _In_ crude_physics_body_handle const                    *body
);