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

cbuffer cbCustomisable : register (b2)
{
	// slider colour min -1.0 max 1.0
};

float4 mainImage(PS_INPUT input) : SV_Target
{
	// Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution.xy;

    // Time varying pixel colour
    float3 col = 0.5 + 0.5*cos(iTime+uv.xyx);
	
	return float4(col, 1.0);
}