#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) out vec4 outColor;

void main()
{
  outColor = vec4( 1, 0.5, 0.5, 1.0 );
}