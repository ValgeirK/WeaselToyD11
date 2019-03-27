
WeaselToy Direct11 ReadMe Instructions <br /> Copyright Weaseltron Entertainment 2018
======================================================================================

## PURPOSE 

WeaselToy is a windows application developed by Weaseltron Entertainment for prototyping
and trying out pixel shaders.

inspired by ShaderToy

## First Use

-When starting for the first time, without a 'settings.ini' file, you get prompted with
 the option to insert the path to a text editor. If this is left empty it will default
 to notepad. This can be changed later through the 'Default Editor' drop down in the
 'Control Panel' window.
 
 
-The application should show the 'Control Panel' window, the 'ShaderToy' window and the
 'Resources' window. If there is a compile error in one of the pixel shaders then the
 'Shader Error Message' window will also be visible and displaying the compile error.
 
## Usage

-The 'ShaderToy' window has 5 tabs at the top where you can change between the main
 scene or onto one of the other 4 buffers. 'Expand' fills out the application window with
 the current shader. In the bottom left you have access to the shader that is being used
 to display the current image. If there is a shader compilation error then it will default
 to a colorful shader.
 
 
-The 'Resources' window shows what resources you have access to. It can be a texture or
 a buffer. You can see the texture you have set and if it is a buffer you can see what
 the buffer contains. The texture format currently supported is .dds. By adding to the
 directory with the other textures you can use your own textures. The textures already
 included come from https://freestocktextures.com/
	x You can double click on a texture to open it in the default viewer,
	  or you can right click to open the explorer in the directory. 
	x By left clicking on a texture, 'Choose File' pops up and you can change textures.
	x By right clicking you can change the type of resource, to none if you don't need
	  the current resource, or you can change a texture to buffer for example.
	  
	  
-The 'Control Panel' window shows a lot of controls, such as showing what project you
 are in and you can copy the project to a new location if you want to create a backup,
 you can create a new project or load an existing project.
 
 You can also use stop and pause, where stop resets the iTime to 0.0, but pause stops
 updating it. The play speed multiplies the delta time, allowing you to speed up or 
 slow down the shader.
 
 You can also reload the shaders, if you don't have auto reload on. This can be done
 by pressing the 'Reload Shaders' or pressing F5.
 
 You can make the application full-screen by pressing the 'Go Fullscreen' button or
 by pressing Alt+Return. There are known issues if some other application is full-screen.
 
 The 'Default Editor' has some built in options and a chance to insert a custom editor.
 This is used when opening the shader, either through the link on the 'ShaderToy'
 window or by clicking an error in the error window.
 
 The desired resolution can be entered into the resolution fields, by selecting it from
 the dropdown or possibly by dragging and resizing the 'ShaderToy' window. If the
 application is resized then it will also resize the ImGui windows.
 
 The 'Constant Buffer Info' node shows the constant buffer values.
 
 The 'Customizable Shader Parameters' only shows values when a constant buffer of that name
 is in the pixel shader and is initialised and used (otherwise dxc will strip the variable).
 
 
-The 'Customizable Shader Parameters' shows the shader variables in the 

	cbuffer cbCustomisable : register (b2)
	{
	};

 section.
 It currently only supports floats and ints. But the input types are managed through
 comments in headers. The commands are:
	* // input variableName step # 			( f.ex. // input colour step 0.1)
	* // slider variableName min # max # 	( f.ex. // slider colour min -1.0 max 1.0)
	* // colourEdit variableName			( f.ex. // colourEdit colour)

 It allows colourEdit and colorEdit. The step, min and max values might not work if
 your variable is an int but you want the step to be a 0.1; If a variable is missing
 the comment headers, then it will be defaulted to the input type.


-There is Renderdoc support, requiring Renderdoc version 1.2 or higher. If Renderdoc is not installed or the version is lower than 1.2 then the checkbox will get unchecked on startup. The Renderdoc dll gets linked on startup so adding it requires a restart of the program.

 
 
 
## Differences compared to ShaderToy.com

- Textures can be added. They just need to be int the textures directory
- Customisable shader parameters
- Longer shaders in ShaderToy can take too long to compile, killing the app. This is not
  an issue.
- Disable VSync to time your shaders.

-Currently when copying code in GLSL from shadertoy it usually contains f.ex. vec3(1.0) 
 which has to be changed to float3(1.0, 1.0, 1.0) in HLSL since it does not support the 
 notation of float3(1.0).


-GLSL has support for multiplying scalars/vectors/matrices, HLSL requires the use of mul().

-ShaderToy samples textures using texture(iChannel0, ...), need to change this 
 according to the HLSL standard of tx0.Sample(iChannel0, ...)

 
 
-In ShaderToy global constant(const) variables are often declared. One option is to 
 change them to 'static const' variables.
 Global variables that get their values changed in functions, do also need to be 
 made static.




-ShaderToy uses the function
		
		void mainImage( out vec4 fragColor, in vec2 fragCoord )
		
 as the main function to draw, I am only using 

		float4 mainImage( PS_INPUT input ) : SV_Target
		
 switching this out and making sure that we are returning the color for the pixel 
 instead of setting it as the value of fragColor.
 
*** LICENCE ***
 
 
 
 Links that might be of assistance
 
-Convert jpg to dds https://www.aconvert.com/image/jpg-to-dds/
  
 
-Mapping between GLSL and HLSL 
 https://anteru.net/blog/2016/mapping-between-hlsl-and-glsl/index.html
 
- Some free stock textures https://freestocktextures.com/