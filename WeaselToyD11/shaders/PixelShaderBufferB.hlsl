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

    // Time varying pixel color
    float3 col = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));

	return float4(col, 1.0);
}