//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register( t0 );
SamplerState samLinear : register( s0 );

cbuffer cbNeverChanges : register ( b0 )
{
	float4 Col1;
};

cbuffer cbChangesEveryFrame : register ( b1 )
{

	float4 Col2;
};

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : UV;
};

float4 main( PS_INPUT input ) : SV_Target
{
    return txDiffuse.Sample(samLinear, input.uv) * Col2;
}
