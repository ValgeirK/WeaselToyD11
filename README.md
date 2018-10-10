################################################################################################################
								WeaselToy Direct11 ReadMe Instructions
################################################################################################################

-Currently when copying code in GLSL from shadertoy it usually contains f.ex. vec3(1.0) which has to be changed to
 float3(1.0, 1.0, 1.0) in HLSL since it does not support the notation of float3(1.0).






-GLSL can multiply matrices and vectors togeather by using *, HLSL needs to use the method mul()






-ShaderToy samples textures using texture(iChannel0, ...), need to change this according to the HLSL standard of
 tx0.Sample(iChannel0, ...)
 
 
 
 
 
 
-In ShaderToy global constant(const) variables are often declared, which need to be changed to 'static const' variables.
 Global variables that get their values changed in functions, do also need to be made static.






-ShaderToy uses the function
		
		void mainImage( out vec4 fragColor, in vec2 fragCoord )
		
 as the main function to draw, I am only using 

		float4 main( PS_INPUT input ) : SV_Target
		
 switching this out and making sure that we are returning the color for the pixel instead of setting it as the value
 of fragColor.
 
 
-Convert jpg to dds https://www.aconvert.com/image/jpg-to-dds/
 
 
 
-Mapping between GLSL and HLSL https://anteru.net/blog/2016/mapping-between-hlsl-and-glsl/index.html