//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register( t0 );
SamplerState samLinear : register( s0 );

#ifdef CPLUSPLUS

#define CONSTANTBUFFER_BEGIN( name, slot )    	struct name {
#define CONSTANTBUFFER_END()					};
#define Vec4	DirectX::XMFLOAT4

#elif defined HLSL

#define CONSTANTBUFFER_BEGIN( name, slot )    	cbuffer name : register ( b#slot ) {
#define CONSTANTBUFFER_END()					};
#define Vec4	float4

#endif

CONSTANTBUFFER_BEGIN( cbNeverChanges, 0 );
Vec4 	Col1;
CONSTANTBUFFER_END();

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
