//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

#include "common/ConstantBuffers.hlsl"
#include "common/macroGLSL.hlsl"

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 uv : UV;
};

float4 main(PS_INPUT input) : SV_Target
{
	// Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    vec3 tex1 = tx4.Sample(iChannel4, uv).xyz;
    vec3 tex2 = tx6.Sample(iChannel6, uv).xyz;

    // Time varying pixel color
    vec3 col = (tex1 + tex2) / 2;

    return vec4(col, 1.0);
}