#version 330 core
in vec2 texCoord;
flat in int channel;

uniform sampler2D tex0;
uniform sampler2D palette;

out vec4 fragColor;

vec4 indexedSample(vec2 texCoord)
{
	vec4 color =  texture(palette, vec2(texture(tex0, texCoord)[channel], 0));
	return vec4(vec3(color), clamp((color.r+color.g+color.b)*256, 0, 1));
}

vec4 BilinearSmoothstepSample (vec2 P)
{
	const float c_textureSize = 1024;
	const float c_onePixel = 1.f/1024;
	vec2 pixel = P * c_textureSize + 0.5;
	
	vec2 frac = fract(pixel);
	pixel = (floor(pixel) / c_textureSize) - vec2(c_onePixel/2.0);

	vec4 C11 = indexedSample(pixel);
	vec4 C21 = indexedSample(pixel + vec2( c_onePixel , 0.0));
	vec4 C12 = indexedSample(pixel + vec2( 0.0        , c_onePixel));
	vec4 C22 = indexedSample(pixel + vec2( c_onePixel , c_onePixel));

	vec4 x1 = mix(C11, C21, smoothstep(0.2, 0.8, frac.x));
	vec4 x2 = mix(C12, C22, smoothstep(0.2, 0.8, frac.x));
	return mix(x1, x2, smoothstep(0.2, 0.8, frac.y));
}

void main(void)
{
	fragColor = indexedSample(texCoord);
}