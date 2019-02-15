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
    float2 uv = fragCoord/iResolution.xy;

    float3 tex1 = tx4.Sample(iChannel4, uv).xyz;
    float3 tex2 = tx6.Sample(iChannel6, uv).xyz;

    // Time varying pixel color
    float3 col = (tex1 + tex2) / 2;

    return float4(col, 1.0);
}