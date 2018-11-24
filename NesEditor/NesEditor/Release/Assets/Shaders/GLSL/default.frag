#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage( fragment )

layout( location = 0 ) out vec4 out_Color;

layout (location = 0) flat in vec4 var_Color;
layout (location = 1) flat in uint var_StringSize;
layout (location = 2) flat in uvec4 var_String;
layout (location = 3) in vec2 var_UV;

layout (binding = 0) uniform UBO 
{
	vec2 uniform_ScreenSize;
	uint uniform_FontTileWidth;
	uint uniform_FontTileHeight;
	uint uniform_FontWidth;
	uint uniform_FontHeight;
};

layout (binding = 1) uniform sampler2D uniform_FontImage;

float median(float r, float g, float b) 
{
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
	if(var_StringSize > 0)
	{
		uint charIndex = uint(floor(float(var_StringSize) * min(var_UV.x, 0.999)));
		uint char4Block = uint(floor(float(charIndex) / 4.0));
		uint char4Offset = charIndex % 4;
		
		uint char = (var_String[char4Block] & (0xFF << (char4Offset * 8))) >> (char4Offset * 8);
		
		uvec2 tileDimensions = uvec2(
			uint(floor(uniform_FontWidth / uniform_FontTileWidth)),
			uint(floor(uniform_FontHeight / uniform_FontTileHeight)));
		
		float tileRange = float(tileDimensions.x * uniform_FontTileWidth) / float(uniform_FontWidth);
		
		vec2 tileUV = vec2(tileRange / float(tileDimensions.x), 1.0 / float(tileDimensions.y));
		vec2 uvStart = vec2(
			float(char % tileDimensions.x) * tileUV.x,
			floor(float(char) / float(tileDimensions.x)) * tileUV.y);
		
		float normalizedXSize = 1.0 / float(var_StringSize);
		float normalizedX = mod(var_UV.x / normalizedXSize, 1.0);
		
		vec2 charUV = vec2(uvStart.x + tileUV.x * normalizedX, uvStart.y + tileUV.y * var_UV.y);
		
		vec4 fontColor = texture(uniform_FontImage, charUV);
		out_Color = var_Color * fontColor.r * 1.5;
	}
	else
	{
		out_Color = var_Color;
	}
}