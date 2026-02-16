#define IMGUI_DEFINE_MATH_OPERATORS
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/imgui_internal.h>

#include <thirdparty/imgui/backends/imgui_impl_sdl3.h>
#include <thirdparty/imgui/backends/imgui_impl_vulkan.h>

#if CRUDE_DEVELOP
#include <thirdparty/ImGuizmo/ImGuizmo.h>
#include <thirdparty/imgui-node-editor/imgui_node_editor.h>
#endif

#include <engine/core/math.h>

#define CRUDE_IMGUI_START_OPTIONS uint32 __crude_total_width = CRUDE_MIN( ImGui::GetContentRegionAvail().x * 0.75, 250 );
#define CRUDE_IMGUI_OPTION( text, decl )\
{\
  ImGui::Text( text );\
  ImGui::SameLine( __crude_total_width );\
  ImGui::SetNextItemWidth( __crude_total_width ); \
  ##decl\
  ImGui::Separator( );\
}