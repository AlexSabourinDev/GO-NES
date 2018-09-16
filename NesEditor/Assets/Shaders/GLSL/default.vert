#version 450
#pragma shader_stage(vertex)

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

out gl_PerVertex
{
    vec4 gl_Position;
};

layout (binding = 0) uniform UBO 
{
	vec2 uniform_ScreenSize;
	uint uniform_FontTileWidth;
	uint uniform_FontTileHeight;
	uint uniform_FontWidth;
	uint uniform_FontHeight;
};

layout (location = 0) in vec2 in_Position;
layout (location = 1) in vec2 in_InstancePosition;
layout (location = 2) in vec2 in_InstanceScale;
layout (location = 3) in vec4 in_InstanceColor;
layout (location = 4) in uint in_TextStringSize;
layout (location = 5) in uvec4 in_TextString;

layout (location = 0) flat out vec4 var_Color;
layout (location = 1) flat out uint var_StringSize;
layout (location = 2) flat out uvec4 var_String;
layout (location = 3) out vec2 var_UV;

void main()
{
	vec2 screenPos = in_Position * in_InstanceScale + in_InstancePosition;
	vec2 unitPos = screenPos / uniform_ScreenSize;
	unitPos.y = 1.0 - unitPos.y;
	unitPos = unitPos * 2.0 - 1.0;

	gl_Position = vec4(unitPos, 0.0, 1.0);
	var_Color = in_InstanceColor;
	var_StringSize = in_TextStringSize;
	var_String = in_TextString;
	var_UV = in_Position;
	var_UV.y = 1.0 - var_UV.y;
	
}