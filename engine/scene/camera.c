#include <scene/camera.h>

void
crude_calculate_camera
(
  _Out_ crude_camera *camera,
  _In_ float32        fov_radians,
  _In_ float32        aspect_ratio,
  _In_ float32        near_z,
  _In_ float32        far_z
)
{
  camera->aspect_ratio = aspect_ratio;
  camera->far_z = far_z;
  camera->near_z = near_z;
  camera->fov_radians = fov_radians;

  crude_matrix view_to_clip_matrix = crude_mat_perspective_fov_lh( fov_radians, aspect_ratio, near_z, far_z );
  crude_matrix clip_to_view_matrix = crude_mat_inverse( NULL, view_to_clip_matrix );
  crude_store_float4x4( &camera->view_to_clip_float4x4, view_to_clip_matrix );
  crude_store_float4x4( &camera->clip_to_view_float4x4, clip_to_view_matrix );
}