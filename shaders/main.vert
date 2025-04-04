#version 450

layout(location=0) in vec3 position;
layout(location=1) in vec4 tangent;
layout(location=2) in vec3 normal;
layout(location=3) in vec2 texcoord0;

layout(location = 0) out vec4 outPosition;

layout(std140, binding = 0) uniform LocalConstants
{
  mat4 viewToClip;
};

void main()
{
  outPosition = viewToClip * vec4( position, 1.0 );
}
