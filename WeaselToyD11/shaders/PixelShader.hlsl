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
	float colour = 0.5;
	// input density step 0.1
	float density = 0.5;
	// colourEdit edit
	float3 edit = float3(1.0, 1.0, 1.0);
	
	float def;
};

float4 mainImage(PS_INPUT input) : SV_Target
{
	// Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution.xy;
	
	float3 col = (0, 0, 0);
	
	float weaselTime = 5.0;
		
	if(iTime > weaselTime)
	{    
        float3 tex1 = tx0.Sample(iChannel0, uv).xyz;

        // Time varying pixel colour
        col = colour + density*cos(def*iTime+uv.xyx);
	
		col = (col + tex1 + edit) / 3;
	}
	else
	{	
	    float3 tex1 = tx1.Sample(iChannel1, uv).xyz;
		col = tex1 * (weaselTime - iTime);
	}
	
	return float4(col, 1.0); 
}