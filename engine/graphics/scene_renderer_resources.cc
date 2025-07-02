#include <graphics/scene_renderer_resources.h>

bool
crude_gfx_mesh_is_transparent
(
  _In_ crude_gfx_mesh_cpu                                 *mesh
)
{
  return ( mesh->flags & ( CRUDE_GFX_DRAW_FLAGS_ALPHA_MASK | CRUDE_GFX_DRAW_FLAGS_TRANSPARENT_MASK ) ) != 0;
}