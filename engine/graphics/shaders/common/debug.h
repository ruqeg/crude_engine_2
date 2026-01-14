
#ifndef CRUDE_DEBUG_GLSLI
#define CRUDE_DEBUG_GLSLI

#define CRUDE_DEBUG_2D_LINES_OFFSET ( 1000 )

struct crude_debug_cube_instance
{
  vec3                                                     translation;
  uint                                                     padding1;
  vec4                                                     color;
  vec3                                                     scale;
  uint                                                     padding2;
};

struct crude_debug_line_vertex
{
  vec3                                                     position;
  uint                                                     color;
};

CRUDE_RBUFFER_REF( DebugCountsRef )
{
  uint                                                     debug_lines_3d_vertices_count;
  uint                                                     debug_lines_3d_instances_count;
  uint                                                     debug_lines_3d_first_vertex;
  uint                                                     debug_lines_3d_first_instance;
  uint                                                     debug_lines_2d_vertices_count;
  uint                                                     debug_lines_2d_instances_count;
  uint                                                     debug_lines_2d_first_vertex;
  uint                                                     debug_lines_2d_first_instance;
  uint                                                     debug_cubes_vertices_count;
  uint                                                     debug_cubes_instances_count;
  uint                                                     debug_cubes_first_vertex;
  uint                                                     debug_cubes_first_instance;
};

CRUDE_RBUFFER_REF( DebugLinesVerticesRef )
{
  crude_debug_line_vertex                                  data[];
};

CRUDE_RBUFFER_REF( DebugCubesInstancesRef )
{
  crude_debug_cube_instance                                data[];
};

void crude_debug_draw_line_coloru
(
  in DebugLinesVerticesRef                                 debug_line_vertices,
  in DebugCountsRef                                        debug_counts,
  in vec3                                                  start,
  in vec3                                                  end,
  in uint                                                  start_color,
  in uint                                                  end_color
)
{
  uint offset = atomicAdd( debug_counts.debug_lines_3d_vertices_count, 2 );

  debug_line_vertices.data[ offset ].position = start;
  debug_line_vertices.data[ offset ].color = start_color;

  debug_line_vertices.data[ offset + 1 ].position = end;
  debug_line_vertices.data[ offset + 1 ].color = end_color;
}

void crude_debug_draw_line
(
  in DebugLinesVerticesRef                                 debug_line_vertices,
  in DebugCountsRef                                        debug_counts,
  in vec3                                                  start,
  in vec3                                                  end,
  in vec4                                                  start_color,
  in vec4                                                  end_color
)
{
  crude_debug_draw_line_coloru( debug_line_vertices, debug_counts, start, end, crude_vec4_to_rgba( start_color ), crude_vec4_to_rgba( end_color ) );
}

void crude_debug_draw_box
(
  in DebugLinesVerticesRef                                 debug_line_vertices,
  in DebugCountsRef                                        debug_counts,
  in vec3                                                  mn,
  in vec3                                                  mx,
  in vec4                                                  color
)
{
  const float x0 = mn.x;
  const float y0 = mn.y;
  const float z0 = mn.z;
  const float x1 = mx.x;
  const float y1 = mx.y;
  const float z1 = mx.z;

  uint color_uint = crude_vec4_to_rgba( color );

  crude_debug_draw_line_coloru( debug_line_vertices, debug_counts, vec3( x0, y0, z0 ), vec3( x0, y1, z0 ), color_uint, color_uint );
  crude_debug_draw_line_coloru( debug_line_vertices, debug_counts, vec3( x0, y1, z0 ), vec3( x1, y1, z0 ), color_uint, color_uint );
  crude_debug_draw_line_coloru( debug_line_vertices, debug_counts, vec3( x1, y1, z0 ), vec3( x1, y0, z0 ), color_uint, color_uint );
  crude_debug_draw_line_coloru( debug_line_vertices, debug_counts, vec3( x1, y0, z0 ), vec3( x0, y0, z0 ), color_uint, color_uint );
  crude_debug_draw_line_coloru( debug_line_vertices, debug_counts, vec3( x0, y0, z0 ), vec3( x0, y0, z1 ), color_uint, color_uint );
  crude_debug_draw_line_coloru( debug_line_vertices, debug_counts, vec3( x0, y1, z0 ), vec3( x0, y1, z1 ), color_uint, color_uint );
  crude_debug_draw_line_coloru( debug_line_vertices, debug_counts, vec3( x1, y1, z0 ), vec3( x1, y1, z1 ), color_uint, color_uint );
  crude_debug_draw_line_coloru( debug_line_vertices, debug_counts, vec3( x1, y0, z0 ), vec3( x1, y0, z1 ), color_uint, color_uint );
  crude_debug_draw_line_coloru( debug_line_vertices, debug_counts, vec3( x0, y0, z1 ), vec3( x0, y1, z1 ), color_uint, color_uint );
  crude_debug_draw_line_coloru( debug_line_vertices, debug_counts, vec3( x0, y1, z1 ), vec3( x1, y1, z1 ), color_uint, color_uint );
  crude_debug_draw_line_coloru( debug_line_vertices, debug_counts, vec3( x1, y1, z1 ), vec3( x1, y0, z1 ), color_uint, color_uint );
  crude_debug_draw_line_coloru( debug_line_vertices, debug_counts, vec3( x1, y0, z1 ), vec3( x0, y0, z1 ), color_uint, color_uint );
}

void crude_debug_draw_line_2d_coloru
(
  in DebugLinesVerticesRef                                 debug_line_vertices,
  in DebugCountsRef                                        debug_counts,
  in vec2                                                  start,
  in vec2                                                  end,
  in uint                                                  start_color,
  in uint                                                  end_color )
{
  uint offset = CRUDE_DEBUG_2D_LINES_OFFSET + atomicAdd( debug_counts.debug_lines_2d_vertices_count, 2 );

  debug_line_vertices.data[ offset ].position = vec3( start.xy, 0 );
  debug_line_vertices.data[ offset ].color = start_color;

  debug_line_vertices.data[ offset + 1 ].position = vec3( end.xy, 0 );
  debug_line_vertices.data[ offset + 1 ].color = end_color;
}

void crude_debug_draw_2d_line
(
  in DebugLinesVerticesRef                                 debug_line_vertices,
  in DebugCountsRef                                        debug_counts,
  in vec2                                                  start,
  in vec2                                                  end,
  in vec4                                                  start_color,
  in vec4                                                  end_color
)
{
  crude_debug_draw_line_2d_coloru( debug_line_vertices, debug_counts, start, end, crude_vec4_to_rgba( start_color ), crude_vec4_to_rgba( end_color ) );
}

void crude_debug_draw_2d_box
(
  in DebugLinesVerticesRef                                 debug_line_vertices,
  in DebugCountsRef                                        debug_counts,
  in vec2                                                  min,
  in vec2                                                  max,
  in vec4                                                  color
)
{
  uint color_uint = crude_vec4_to_rgba(color);

  crude_debug_draw_line_2d_coloru( debug_line_vertices, debug_counts, vec2(min.x, min.y), vec2(max.x, min.y), color_uint, color_uint );
  crude_debug_draw_line_2d_coloru( debug_line_vertices, debug_counts, vec2(max.x, min.y), vec2(max.x, max.y), color_uint, color_uint );
  crude_debug_draw_line_2d_coloru( debug_line_vertices, debug_counts, vec2(max.x, max.y), vec2(min.x, max.y), color_uint, color_uint );
  crude_debug_draw_line_2d_coloru( debug_line_vertices, debug_counts, vec2(min.x, max.y), vec2(min.x, min.y), color_uint, color_uint );
}

void crude_debug_draw_cube
(
  in DebugCubesInstancesRef                                debug_cube_instances,
  in DebugCountsRef                                        debug_counts,
  in vec3                                                  translation,
  in vec3                                                  scale,
  in vec4                                                  color
)
{
  uint offset = atomicAdd( debug_counts.debug_cubes_instances_count, 1 );

  debug_cube_instances.data[ offset ].translation = translation;
  debug_cube_instances.data[ offset ].scale = scale;
  debug_cube_instances.data[ offset ].color = color;
}

#endif /* CRUDE_DEBUG_GLSLI */