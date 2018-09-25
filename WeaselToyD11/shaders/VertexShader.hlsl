//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer cbNeverChanges : register ( b0 )
{
	float4 Col1;
};

cbuffer cbChangesEveryFrame : register ( b1 )
{
	float4 Col2;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

struct VS_INPUT
{
    float4 pos : POSITION;
    float2 uv : UV;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
	float2 uv  : UV;
};

PS_INPUT main( VS_INPUT input )
{
	PS_INPUT res = (PS_INPUT)0;
	
	res.pos = input.pos;
	res.uv = input.uv;
	
    return res;
}