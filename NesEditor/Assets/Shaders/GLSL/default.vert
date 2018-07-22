#version 450
#pragma shader_stage(vertex)

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

out gl_PerVertex
{
    vec4 gl_Position;
};

layout (location = 0) in vec2 in_Position;
layout (location = 1) in vec2 in_InstancePosition;
layout (location = 2) in vec2 in_InstanceScale;

void main()
{
	gl_Position = vec4(in_Position * in_InstanceScale + in_InstancePosition, 0.0, 1.0);
}