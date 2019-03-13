#define vec2 float2
#define vec3 float3
#define vec4 float4
#define mat2 float2x2
#define mat3 float3x3
#define mat4 float4x4


#define gl_ClipDistance			SV_ClipDistance
#define gl_CullDistance			SV_CullDistance
#define gl_SampleMaskIn			SV_Coverage
#define gl_SampleMaskIn 		SV_Coverage
#define gl_FragDepth			SV_Depth
#define gl_GlobalInvocationID	SV_DispatchTreadID
#define gl_TessCord				SV_DomainLocation
#define gl_WorkGroupID			SV_GroupID
#define gl_LocalInvocationIndex	SV_GroupIndex
#define gl_LocalInvocationID 	SV_GroupThreadID
#define gl_InvoationID 			SV_GSInstanceID
#define gl_TessLevelInner		SV_InsideTessFactor
#define gl_InstanceID			SV_InstanceID
#define gl_InstanceIndex		SV_InstanceID
#define gl_FrontFacing			SV_IsFrontFace
#define gl_InvocationID 		SV_OutputControlPointID
#define gl_Position				SV_Position
#define gl_FragCoord			SV_Position
#define gl_PrimitiveID			SV_PrimitiveID
#define gl_Layer 				SV_RenderTargetArrayIndex
#define gl_SampleID				SV_SampleIndex
#define gl_FragStencilRef		SV_StencilRef
#define gl_TessLevelOuter		SV_TessFactor
#define gl_VertexID 			SV_VertexID
#define gl_VertexIndex 			SV_VertexID
#define gl_ViewportIndex 		SV_ViewportArrayIndex

#define gl_PointSize			PSIZE
#define gl_PointCoord			SV_Position
#define gl_FragColor			SV_Target
#define gl_FragData0			SV_Target0
#define gl_FragData1			SV_Target1
#define gl_FragData2			SV_Target2
#define gl_FragData3			SV_Target3
#define gl_FragData4			SV_Target4
#define gl_FragData5			SV_Target5
#define gl_FragData6			SV_Target6
#define gl_FragData7			SV_Target7

	

#define atan(x,y) 				atan2(y,x)
#define dFdx 					ddx
#define dFdxCoarse				ddx_coarse
#define dFdxFine				ddx_fine
#define dFdy					ddy
#define dFdyCoarse				ddy_coarse
#define dFdyFine				ddy_fine
#define interpolateAtCentroid	EvaluateAttributeAtCentroid
#define interpolateAtSample		EvaluateAttributeAtSample
#define interpolateAtOffset		EvaluateAttributeSnapped
#define fract 					frac
#define mix 					lerp
#define fma						mad

#define inversesqrt				rsqrt
#define mod 					fmod


#define fragCoord (input.uv * iResolution.xy)