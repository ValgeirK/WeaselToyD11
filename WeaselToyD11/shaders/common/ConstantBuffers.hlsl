#include "common/common.hlsl"

//--------------------------------------------------------------------------------------
// constant Buffer Variables
//--------------------------------------------------------------------------------------

cbuffer cbNeverChanges : register (b0)
{
	float4 iChannelResolution[NUMBER_OF_CHANNELS];
	float4 iResolution;
};

cbuffer cbChangesEveryFrame : register (b1)
{
	float4	 iMouse;
	float4   iDate;
	float    iTime;
	float    iTimeDelta;
	int		 iFrame;
	int		 Padding;
};