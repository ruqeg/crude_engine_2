#pragma once

#define CRUDE_PHYSICS_JOLT_MAX_BODIES                      1024
#define CRUDE_PHYSICS_JOLT_NUM_BODIES_MUTEXES              0
#define CRUDE_PHYSICS_JOLT_MAX_BODIES_PAIRS                1024
#define CRUDE_PHYSICS_JOLT_MAX_CONTACT_CONSTRAINTS         1024
#define CRUDE_PHYSICS_JOLT_DELTA_TIME                      ( 1 / 60.f )
// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
#define CRUDE_PHYSICS_JOLT_COLLISIONS_STEPS                1

#define CRUDE_PHYSICS_OCTREE_RELATIVE_FILEPATH_LENGTH_MAX  1024